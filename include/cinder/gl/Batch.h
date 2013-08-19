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
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Vao.h"
#include "cinder/GeoIo.h"

namespace cinder { namespace gl {

typedef std::shared_ptr<class Batch>		BatchRef;
typedef std::shared_ptr<class VertBatch>	VertBatchRef;

class Batch {
  public:
	static BatchRef		create( const geo::Source &source, const gl::GlslProgRef &glsl );
	
	Batch( const geo::Source &source, const gl::GlslProgRef &glsl );

	void			draw();
	
	GlslProgRef		mGlsl;
	
	VboRef		mVertexArray;
	VboRef		mElements;
	VaoRef		mVao;
	
	size_t		mNumVertices;
	size_t		mNumIndices;
	GLenum		mIndexType;
};

//! Cannot be shared across contexts
class VertBatch {
  public:
	VertBatch( GLenum primType = GL_POINTS );
	static VertBatchRef create( GLenum primType = GL_POINTS );

	void	setType( GLenum type );
	GLenum	getType() const { return mPrimType; }
	
	void	color( float r, float g, float b, float a = 1.0f ) { color( ColorAf( r, g, b, a ) ); }
	void	color( const Colorf &color );
	void	color( const ColorAf &color );

	void	normal( float x, float y, float z ) { normal( Vec3f( x, y, z ) ); }
	void	normal( const Vec3f &n );

	void	vertex( float x, float y, float z = 0, float w = 1 ) { vertex( Vec4f( x, y, z, w ) ); }
	void	vertex( const Vec2f &v ) { vertex( Vec4f( v.x, v.y, 0, 1 ) ); }
	void	vertex( const Vec3f &v ) { vertex( Vec4f( v.x, v.y, v.z, 1 ) ); }
	void	vertex( const Vec4f &v );
	//! Sets the vertex and the color simultaneously. Should not be called if you have called color() prior.
	void	vertex( const Vec4f &v, const ColorAf &c );

	void	texCoord( float s, float t, float r = 0, float q = 1 ) { texCoord( Vec4f( s, t, r, q ) ); }	
	void	texCoord( const Vec2f &t ) { texCoord( Vec4f( t.x, t.y, 0, 1 ) ); }
	void	texCoord( const Vec3f &t ) { texCoord( Vec4f( t.x, t.y, t.z, 1 ) ); }
	void	texCoord( const Vec4f &t );
	
	void	begin( GLenum type );
	void	end();
	void	clear();

	bool	empty() const { return mVertices.empty(); }
	
	void	draw();
	
  protected:
	void	addVertex( const Vec4f &v );
	void	setupBuffers();

	GLenum					mPrimType;

	std::vector<Vec4f>		mVertices;
	
	std::vector<Vec3f>		mNormals;
	std::vector<ColorAf>	mColors;
	std::vector<Vec4f>		mTexCoords;
	
	VaoRef					mVao;
	VboRef					mVbo;
};

} } // namespace cinder::gl