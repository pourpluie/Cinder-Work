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
*/

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"

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
	: mArrayBufferBinding( 0 ), mElementArrayBufferBinding( 0 )
{
}

void Vao::bind()
{
	// this will "come back" by calling bindImpl if it's necessary
	context()->vaoBind( shared_from_this() );
}

void Vao::unbind() const
{
	// this will "come back" by calling bindImpl if it's necessary
	context()->vaoBind( nullptr );
}

void Vao::invalidateContext( Context *context )
{
	// binding a VAO invalidates other pieces of cached state
	context->invalidateBufferBinding( GL_ARRAY_BUFFER );
	context->invalidateBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
}

void Vao::reflectBindBuffer( GLenum target, GLuint buffer )
{
	if( target == GL_ARRAY_BUFFER ) {
		mArrayBufferBinding = buffer;
	}
	else if( target == GL_ELEMENT_ARRAY_BUFFER ) {
		mElementArrayBufferBinding = buffer;
	}
}

} }
