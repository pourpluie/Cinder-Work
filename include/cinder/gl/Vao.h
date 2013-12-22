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

#pragma once

#include "cinder/gl/gl.h"
#include <memory>
#include <vector>
#include <map>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;

class Vao : public std::enable_shared_from_this<Vao> {
  public:
	struct Layout;
	
	static VaoRef		create();
	virtual ~Vao() {}
	
	void	bind();
	void	unbind() const;

	GLuint	getId() const { return mId; }

	//! This is meant to allow efficient replacement of a VAO without allocating a new VaoRef
	void	swap( const Layout &layout );

	struct VertexAttrib {
		VertexAttrib()
			: mEnabled( true ), mSize( 0 ), mType( GL_FLOAT ), mNormalized( false ), mStride( 0 ), mPointer( 0 ), mArrayBufferBinding( 0 )
		{}
		
		VertexAttrib( GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer, GLuint arrayBufferBinding )
			: mEnabled( false ), mSize( size ), mType( type ), mNormalized( normalized ), mStride( stride ), mPointer( pointer ), mArrayBufferBinding( arrayBufferBinding )
		{}
		
		bool			mEnabled;
		GLint			mSize;
		GLenum			mType;
		GLboolean		mNormalized;
		GLsizei			mStride;
		const GLvoid*	mPointer;
		GLuint			mArrayBufferBinding;
	};

	//! Represent a software-only mirror of the state a VAO records. Can be used directly for efficient swapping (primarily by the gl:: convenience functions)
	struct Layout {
		Layout();
		
		//! The equivalent of glBindBuffer( GL_ARRAY_BUFFER, \a binding );
		void	bindArrayBuffer( GLuint binding );
		//! The equivalent of glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, \a binding );
		void	bindElementArrayBuffer( GLuint binding );
		void	enableVertexAttrib( 
		//! Does not enable the vertex attrib
		void	vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
		
		GLuint							mArrayBufferBinding, mElementArrayBufferBinding;
		std::map<GLuint,VertexAttrib>	mVertexAttribs;		
	};
	
  protected:
	Vao();


	// Does the actual work of binding the VAO; called by Context
	virtual void	bindImpl( class Context *context ) = 0;
	// Does the actual work of unbinding the VAO; called by Context
	virtual void	unbindImpl( class Context *context ) = 0;
	// Analogous to glEnableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	enableVertexAttribArrayImpl( GLuint index ) = 0;
	// Analogous to glVertexAttribPointer(). Expects this to be the currently bound VAO; called by Context
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) = 0;
	// Caches the currently bound buffer; called by Context when GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER changes
	void			reflectBindBuffer( GLenum target, GLuint buffer );

	// Causes Context to reflect any state cache invalidations due to binding/unbinding a VAO
	static void		invalidateContext( class Context *context );


	GLuint							mId;
	Layout							mLayout;

	friend class Context;
};
	
} }
