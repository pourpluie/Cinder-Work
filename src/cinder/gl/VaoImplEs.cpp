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

// Concrete implementation of VAO for OpenGL ES 2.
// Should only be instantiated by Vao::create() in the presence of GL_OES_vertex_array_object

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

class VaoImplEs : public Vao {
  public:
	virtual ~VaoImplEs();
	
	VaoImplEs();

	// Does the actual "work" of binding the VAO; called by Context
	virtual void	bindImpl( class Context *context ) override;
	virtual void	unbindImpl( class Context *context ) override;
	virtual void	enableVertexAttribArrayImpl( GLuint index ) override;
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) override;
	
	friend class Context;
};

// Called by Vao::create()
VaoRef createVaoImplEs()
{
	return VaoRef( new VaoImplEs );
}
	
VaoImplEs::VaoImplEs()
{
	mId	= 0;

	glGenVertexArraysOES( 1, &mId );
}

VaoImplEs::~VaoImplEs()
{
	glDeleteVertexArraysOES( 1, &mId );
}


void VaoImplEs::bindImpl( Context *context )
{
	glBindVertexArrayOES( mId );

	if( context )
		invalidateContext( context );
}

void VaoImplEs::unbindImpl( Context *context )
{
	glBindVertexArrayOES( 0 );

	if( context )
		invalidateContext( context );
}

void VaoImplEs::enableVertexAttribArrayImpl( GLuint index )
{
	glEnableVertexAttribArray( index );
}

void VaoImplEs::vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

} }
