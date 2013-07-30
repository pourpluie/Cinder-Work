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

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;

class Vao : public std::enable_shared_from_this<Vao> {
  public:
	static VaoRef		create();
	virtual ~Vao() {}
	
	void	bind();
	void	unbind() const;

	GLuint	getId() const { return mId; }
	
  protected:
	Vao();

	// Does the actual "work" of binding the VAO; called by Context
	virtual void	bindImpl( class Context *context ) = 0;
	virtual void	unbindImpl( class Context *context ) = 0;
	// Analogous to glEnableVertexAttribArray(). Expects this to be the currently bound VAO; called by Context
	virtual void	enableVertexAttribArrayImpl( GLuint index ) = 0;
	// Analogous to glVertexAttribPointer(). Expects this to be the currently bound VAO; called by Context
	virtual void	vertexAttribPointerImpl( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) = 0;
	// Causes Context to reflect any state cache invalidations due to binding/unbinding a VAO
	static void		invalidateContext( class Context *context );
	
	GLuint			mId;
	friend class Context;
};
	
} }
