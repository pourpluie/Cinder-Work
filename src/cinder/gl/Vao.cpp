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

/*	The VAO class abstracts Vertex Array Objects in OpenGL through 3 implementation classes,
		VaoImplEs for OpenGL ES 2
		VaoImplCore for desktop OpenGL, both Core and Compatibility profile with approriate extensions
		VaoImplSoftware for implementations without a native VAO class
	
	We don't support the old fixed function data (ie glVertexPointer() and friends).
	
	ci::gl::VAO caches the following state:
		* ELEMENT_ARRAY_BUFFER_BINDING
		* VERTEX_ATTRIB_ARRAY_BUFFER_BINDING per attribute
		* All individual attribute data
	
	The full list is in Table 6.4 of the OpenGL 3.2 Core Profile spec,
		http://www.opengl.org/registry/doc/glspec32.core.20090803.pdf
	
	Note that ARRAY_BUFFER_BINDING is NOT cached
*/

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"

#include <set>

using namespace std;

namespace cinder { namespace gl {

// defined in VaoImplEs
#if defined( CINDER_GLES )
extern VaoRef createVaoImplEs();
#else
extern VaoRef createVaoImplCore();
#endif
extern VaoRef createVaoImplSoftware();

VaoRef Vao::create()
{
#if defined( CINDER_GLES )
	#if defined( CINDER_COCOA_TOUCH )
		return createVaoImplEs();
	#elif defined( CINDER_GL_ANGLE )
		return createVaoImplSoftware();
	#else
		if( env()->supportsHardwareVao() )
			return createVaoImplEs();
		else
			return createVaoImplSoftware();
	#endif
#else
	return createVaoImplCore();
//	return createVaoImplSoftware();
#endif
}

Vao::Vao()
	: mCtx( gl::context() )
{
}

void Vao::setContext( Context *context )
{
	mCtx = context;
}

void Vao::bind()
{
	// this will "come back" by calling bindImpl if it's necessary
	mCtx->bindVao( shared_from_this() );
}

void Vao::unbind() const
{
	// this will "come back" by calling bindImpl if it's necessary
	mCtx->bindVao( nullptr );
}

void Vao::swap( const VaoCacheRef &vao )
{
	swap( vao->getLayout() );
}

void Vao::swap( const Vao::Layout &newLayout )
{
	VaoScope vaoScope( shared_from_this() );

	Vao::Layout originalLayout = mLayout;
	
	auto oldArrayBufferBinding = mCtx->getBufferBinding( GL_ARRAY_BUFFER );
	
	// gather all the array buffer bindings
	set<GLuint> arrayBufferBindings;
	for( const auto &attrib : newLayout.mVertexAttribs )
		arrayBufferBindings.insert( attrib.second.mArrayBufferBinding );

	// iterate all the represented array buffer bindings, bind them, and call glVertexAttribPointer
	for( auto &arrayBufferBinding : arrayBufferBindings ) {
		mCtx->bindBuffer( GL_ARRAY_BUFFER, arrayBufferBinding );
		// iterate all attributes to find the ones whose mArrayBufferBinding is 'arrayBufferBinding'
		for( auto &attrib : newLayout.mVertexAttribs ) {
			if( attrib.second.mArrayBufferBinding == arrayBufferBinding ) {
				// does 'this' have an attribute for this location? 
				if( originalLayout.mVertexAttribs.find( attrib.first ) != originalLayout.mVertexAttribs.end() ) {
					// since we already have this attribute location, only enable/disable if layout's is different
					if( originalLayout.mVertexAttribs[attrib.first].mEnabled != attrib.second.mEnabled ) {
						if( attrib.second.mEnabled )
							enableVertexAttribArrayImpl( attrib.first );
						else
							disableVertexAttribArrayImpl( attrib.first );
					}
				}
				else {
					if( attrib.second.mEnabled )
						enableVertexAttribArrayImpl( attrib.first );
				}
				// TODO: test for redundancy here (attrib is the same as what's already in originalLayout
				vertexAttribPointerImpl( attrib.first, attrib.second.mSize, attrib.second.mType, attrib.second.mNormalized,
							attrib.second.mStride, attrib.second.mPointer );
			}
		}
	}

	// iterate all the vertex attribs in 'this' which are not enabled in layout and disable them
	for( auto &attrib : originalLayout.mVertexAttribs ) {
		auto existing = newLayout.mVertexAttribs.find( attrib.first );
		if( attrib.second.mEnabled && ( ( existing == newLayout.mVertexAttribs.end() ) || (! existing->second.mEnabled) ) )
			disableVertexAttribArrayImpl( attrib.first );
	}

	// finally, this->mLayout becomes 'newLayout'
	mLayout = newLayout;

	mCtx->bindBuffer( GL_ARRAY_BUFFER, oldArrayBufferBinding );
	mCtx->bindBuffer( GL_ELEMENT_ARRAY_BUFFER, mLayout.mElementArrayBufferBinding );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vao::Layout
Vao::Layout::Layout()
	: mElementArrayBufferBinding( 0 ), mCachedArrayBufferBinding( 0xFFFFFFFF )
{
}

void Vao::Layout::bindBuffer( GLenum target, GLuint buffer )
{
	if( target == GL_ARRAY_BUFFER )
		mCachedArrayBufferBinding = buffer;
	else if( target == GL_ELEMENT_ARRAY_BUFFER )
		mElementArrayBufferBinding = buffer;
}

void Vao::Layout::enableVertexAttribArray( GLuint index )
{
	auto existing = mVertexAttribs.find( index );
	if( existing != mVertexAttribs.end() ) {
		existing->second.mEnabled = true;
	}
	else {
		mVertexAttribs[index] = VertexAttrib();
		mVertexAttribs[index].mEnabled = true;
	}
}

void Vao::Layout::disableVertexAttribArray( GLuint index )
{
	auto existing = mVertexAttribs.find( index );
	if( existing != mVertexAttribs.end() ) {
		existing->second.mEnabled = false;
	}
	else {
		mVertexAttribs[index] = VertexAttrib();
		mVertexAttribs[index].mEnabled = false;
	}
}

void Vao::Layout::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	auto existing = mVertexAttribs.find( index );
	bool enabled = ( existing != mVertexAttribs.end() ) && ( existing->second.mEnabled );
assert( mCachedArrayBufferBinding != 0 );
	mVertexAttribs[index] = Vao::VertexAttrib( size, type, normalized, stride, pointer, mCachedArrayBufferBinding );
	mVertexAttribs[index].mEnabled = enabled;
}

std::ostream& operator<<( std::ostream &lhs, const VaoRef &rhs )
{
	lhs << *rhs;
	return lhs;
}

std::ostream& operator<<( std::ostream &lhs, const Vao &rhs )
{
	lhs << "ID: " << rhs.getId() << std::endl;
	lhs << rhs.getLayout();
	
	return lhs;
}

std::ostream& operator<<( std::ostream &lhs, const Vao::Layout &rhs )
{
	lhs << "Cached ARRAY_BUFFER binding: " << rhs.mCachedArrayBufferBinding << "  ELEMENT_ARRAY_BUFFER_BINDING: " << rhs.mElementArrayBufferBinding << std::endl;
	lhs << "{" << std::endl;
	for( auto &attrib : rhs.mVertexAttribs ) {
		lhs << " Loc: " << attrib.first << std::endl;
		lhs << "        Enabled: " << ( attrib.second.mEnabled ? "TRUE" : "FALSE" ) << std::endl;
		lhs << "           Size: " << attrib.second.mSize << std::endl;
		lhs << "           Type: " << gl::typeToString( attrib.second.mType ) << "(" << attrib.second.mType << ")" << std::endl;
		lhs << "     Normalized: " << ( attrib.second.mNormalized ? "TRUE" : "FALSE" ) << std::endl;
		lhs << "         Stride: " << attrib.second.mStride << std::endl;
		lhs << "        Pointer: " << attrib.second.mPointer << "(" << (size_t)attrib.second.mPointer << ")" << std::endl;
		lhs << "   Array Buffer: " << attrib.second.mArrayBufferBinding << std::endl;
	}
	lhs << "}";

	return lhs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VaoCache
VaoCacheRef VaoCache::create()
{
	return VaoCacheRef( new VaoCache() );
}

VaoCache::VaoCache()
{
}

void VaoCache::bindImpl( class Context *context )
{
	if( context ) {
		context->reflectBufferBinding( GL_ELEMENT_ARRAY_BUFFER, mLayout.mElementArrayBufferBinding );
		mLayout.mCachedArrayBufferBinding = context->getBufferBinding( GL_ARRAY_BUFFER );
	}
}

void VaoCache::unbindImpl( class Context *context )
{
}

void VaoCache::enableVertexAttribArrayImpl( GLuint index )
{
	mLayout.enableVertexAttribArray( index );
}

void VaoCache::disableVertexAttribArrayImpl( GLuint index )
{
	mLayout.disableVertexAttribArray( index );
}

void VaoCache::vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	mLayout.vertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void VaoCache::reflectBindBufferImpl( GLenum target, GLuint buffer )
{
	mLayout.bindBuffer( target, buffer );
	glBindBuffer( target, buffer );
}

} }
