/*
 Copyright (c) 2013, The Cinder Project, All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

// Concrete implementation of VAO using "software" emulation

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"

#include <map>
using namespace std;

namespace cinder { namespace gl {

class VaoImplSoftware : public Vao {
  public:
	virtual ~VaoImplSoftware();
	
	VaoImplSoftware();

	// Does the actual "work" of binding the VAO; called by Context
	virtual void	bindImpl( Context *context ) override;
	virtual void	unbindImpl( Context *context ) override;
	virtual void	enableVertexAttribArrayImpl( GLuint index ) override;
	virtual void	disableVertexAttribArrayImpl( GLuint index ) override;
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) override;
	virtual void	vertexAttribDivisorImpl( GLuint index, GLuint divisor ) override;	
	virtual void	reflectBindBufferImpl( GLenum target, GLuint buffer ) override;

  protected:
	static size_t	sIdCounter;

	friend class Context;
};

size_t VaoImplSoftware::sIdCounter = 0;

// Called by Vao::create()
VaoRef createVaoImplSoftware()
{
	return VaoRef( new VaoImplSoftware );
}
	
VaoImplSoftware::VaoImplSoftware()
{
	mId = ++sIdCounter; // is this adequate? 4billion VAO allocations seems remote; the ID is not really necessary except for debugging
}

VaoImplSoftware::~VaoImplSoftware()
{
}

void VaoImplSoftware::enableVertexAttribArrayImpl( GLuint index )
{
	mLayout.enableVertexAttribArray( index );
	
	glEnableVertexAttribArray( index );
}

void VaoImplSoftware::disableVertexAttribArrayImpl( GLuint index )
{
	mLayout.disableVertexAttribArray( index );

	glDisableVertexAttribArray( index );
}

void VaoImplSoftware::bindImpl( Context *context )
{
	if( ! context )
		return;
	
	auto oldBuffer = context->getBufferBinding( GL_ARRAY_BUFFER );

	for( auto attribIt = mLayout.mVertexAttribs.begin(); attribIt != mLayout.mVertexAttribs.end(); ++attribIt ) {
		if( attribIt->second.mEnabled ) {
			glEnableVertexAttribArray( attribIt->first );
			glBindBuffer( GL_ARRAY_BUFFER, attribIt->second.mArrayBufferBinding );
			glVertexAttribPointer( attribIt->first, attribIt->second.mSize, attribIt->second.mType, attribIt->second.mNormalized, attribIt->second.mStride, attribIt->second.mPointer );
		}
	}

	context->bindBuffer( GL_ELEMENT_ARRAY_BUFFER, mLayout.mElementArrayBufferBinding );
	context->bindBuffer( GL_ARRAY_BUFFER, oldBuffer );
}

void VaoImplSoftware::unbindImpl( Context *context )
{
	for( auto attribIt = mLayout.mVertexAttribs.begin(); attribIt != mLayout.mVertexAttribs.end(); ++attribIt ) {
		if( attribIt->second.mEnabled ) {
			glDisableVertexAttribArray( attribIt->first );
		}
	}
}

void VaoImplSoftware::vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	mLayout.vertexAttribPointer( index, size, type, normalized, stride, pointer );

	glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void VaoImplSoftware::vertexAttribDivisorImpl( GLuint index, GLuint divisor )
{
	mLayout.vertexAttribDivisor( index, divisor );

#if ! defined( CINDER_GLES )
	glVertexAttribDivisor( index, divisor );
#endif
}


void VaoImplSoftware::reflectBindBufferImpl( GLenum target, GLuint buffer )
{
	mLayout.bindBuffer( target, buffer );

	glBindBuffer( target, buffer );
}

} }
