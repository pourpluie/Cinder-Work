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
#include "cinder/gl/BufferObj.h"
#include <memory>
#include <vector>
#include <map>

#include <ostream>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;
class VaoCache;
typedef std::shared_ptr<VaoCache> VaoCacheRef;

class Vao : public std::enable_shared_from_this<Vao> {
  public:
	struct Layout;
	
	static VaoRef		create();
	virtual ~Vao() {}
	
	void	bind();
	void	unbind() const;

	GLuint			getId() const { return mId; }
	const Layout&	getLayout() const { return mLayout; }

	//! This is meant to allow efficient replacement of a VAO without allocating a new Vao
	void	swap( const VaoCacheRef &cache );

	//! This is meant to allow efficient replacement of a VAO without allocating a new Vao
	void	swap( const Layout &layout );

	struct VertexAttrib {
		VertexAttrib()
			: mEnabled( false ), mSize( 0 ), mType( GL_FLOAT ), mNormalized( false ), mStride( 0 ), mPointer( 0 ), mArrayBufferBinding( 0 )
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
		
		//! The equivalent of glBindBuffer( \a target, \a binding )
		void	bindBuffer( GLenum target, GLuint buffer );
		//! The equivalent of glEnableVertexAttribArray( \a index )
		void	enableVertexAttribArray( GLuint index );
		//! The equivalent of glDisableVertexAttribArray( \a index )
		void	disableVertexAttribArray( GLuint index );
		//! Does not enable the vertex attrib
		void	vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
		
		GLuint							mElementArrayBufferBinding;
		GLuint							mCachedArrayBufferBinding; // this represent a cache of the Context's value, but VAOs do not record GL_ARRAY_BUFFER_BINDING
		std::map<GLuint,VertexAttrib>	mVertexAttribs;

		friend class Vao;
	};
	
  protected:
	Vao();

	//! only necessary when VAO is created without
	void setContext( Context *context );

	// Does the actual work of binding the VAO; called by Context
	virtual void	bindImpl( class Context *context ) = 0;
	// Does the actual work of unbinding the VAO; called by Context
	virtual void	unbindImpl( class Context *context ) = 0;
	// Analogous to glEnableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	enableVertexAttribArrayImpl( GLuint index ) = 0;
	// Analogous to glDisableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	disableVertexAttribArrayImpl( GLuint index ) = 0;
	// Analogous to glVertexAttribPointer(). Expects this to be the currently bound VAO; called by Context
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) = 0;
	// Caches the currently bound buffer; called by Context when GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER changes
	virtual void	reflectBindBufferImpl( GLenum target, GLuint buffer ) = 0;

	GLuint							mId;
	Context							*mCtx;
	Layout							mLayout;

	friend Context;
	friend std::ostream& operator<<( std::ostream &lhs, const VaoRef &rhs );
	friend std::ostream& operator<<( std::ostream &lhs, const Vao &rhs );
};

class VaoCache : public Vao {
  public:
	static VaoCacheRef	create();

  protected:
	VaoCache();

	// Does the actual work of binding the VAO; called by Context
	virtual void	bindImpl( class Context *context ) override;
	// Does the actual work of unbinding the VAO; called by Context
	virtual void	unbindImpl( class Context *context ) override;
	// Analogous to glEnableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	enableVertexAttribArrayImpl( GLuint index ) override;
	// Analogous to glDisableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	disableVertexAttribArrayImpl( GLuint index ) override;
	// Analogous to glVertexAttribPointer(). Expects this to be the currently bound VAO; called by Context
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) override;
	virtual void	reflectBindBufferImpl( GLenum target, GLuint buffer ) override;
};

// Convenience method for dumping VAO contents to a std::ostream
std::ostream& operator<<( std::ostream &lhs, const VaoRef &rhs );
// Convenience method for dumping VAO contents to a std::ostream
std::ostream& operator<<( std::ostream &lhs, const Vao &rhs );
// Convenience method for dumping Vao::Layout contents to a std::ostream
std::ostream& operator<<( std::ostream &lhs, const Vao::Layout &rhs );
	
} }
