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

#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl {

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Batch
Batch::Batch( const GeoSource &source, const gl::GlslProgRef &glsl )
	: mGlsl( glsl )
{
/*	const size_t numVerts = source.getNumVerts();
	size_t dataSizeBytes = 0;
	bool hasPosition = glsl->hasAttribSemantic( ATTRIB_POSITION ) && source.hasAttrib( GeoIo::POSTION ) > 0;
	if( hasPosition )
		dataSize += numVerts * sizeof(float) * source.getAttribSize( GeoIo::POSITION );
	
	// allocate VBO dataSize
	// if we have mapBuffer, do that, else allocate temporary
	
	if( hasPosition )
		source->copyAttrib( GeoIo::POSITION, &buffer[0], positionOffset, dataStride );
	
	if( source.isIndexed() ) {
		// allocate VBO for indexed
		mIndexed = true;
		mIndicesVbo = Vbo(...);
	}*/
}

void Batch::draw()
{
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VertBatch
VertBatch::VertBatch( GLenum primType )
	: mPrimType( primType )
{
}

VertBatchRef VertBatch::create( GLenum primType )
{
	return VertBatchRef( new VertBatch( primType ) ); 
}

void VertBatch::setType( GLenum primType )
{
	mPrimType = primType;
}

void VertBatch::normal( const Vec3f &n )
{
	mNormals.push_back( n );
}

void VertBatch::color( const Colorf &color )
{
	mColors.push_back( color );
}

void VertBatch::color( const ColorAf &color )
{
	mColors.push_back( color );
}

void VertBatch::texCoord( const Vec4f &t )
{
	mTexCoords.push_back( t );
}

void VertBatch::vertex( const Vec4f &v )
{
	addVertex( Vec4f( v.x, v.y, v.z, v.w ) );
}

void VertBatch::vertex( const Vec4f &v, const ColorAf &c )
{
	mColors.push_back( c );
	addVertex( v );
}

void VertBatch::addVertex( const Vec4f &v )
{
	mVertices.push_back( v );

	if( ! mNormals.empty() ) {
		while( mNormals.size() < mVertices.size() )
			mNormals.push_back( mNormals.back() );
	}
	
	if( ! mColors.empty() ) {
		while( mColors.size() < mVertices.size() )
			mColors.push_back( mColors.back() );	
	}

	if( ! mTexCoords.empty() ) {
		while( mTexCoords.size() < mVertices.size() )
			mTexCoords.push_back( mTexCoords.back() );	
	}
}

void VertBatch::begin( GLenum primType )
{
	clear();
	mPrimType = primType;
}

void VertBatch::end()
{
}

void VertBatch::clear()
{
	mVertices.clear();
	mNormals.clear();
	mColors.clear();
	mTexCoords.clear();
	mVbo.reset();
	mVao.reset();
}

void VertBatch::draw()
{
	setupBuffers();
	VaoScope vao( mVao );
	
	gl::setDefaultShaderVars();
	gl::drawArrays( mPrimType, 0, mVertices.size() );
}

void VertBatch::setupBuffers()
{
	auto ctx = gl::context();

	const size_t verticesSizeBytes = mVertices.size() * sizeof(Vec4f);
	const size_t normalsSizeBytes = mNormals.size() * sizeof(Vec3f);
	const size_t colorsSizeBytes = mColors.size() * sizeof(ColorAf);
	const size_t texCoordsSizeBytes = mTexCoords.size() * sizeof(Vec4f);

/*	size_t vertexStride = 0;
	if( ! mVertices.empty() )
		vertexStride += sizeof(Vec4f);
	if( ! mNormals.empty() )
		vertexStride += sizeof(Vec3f);
	if( ! mColors.empty() )
		vertexStride += sizeof(ColorAf);
	if( ! mTexCoords.empty() )
		vertexStride += sizeof(Vec4f);*/

	if( ! mVbo ) {
		// calculate the room we'll need in the VBO
		size_t totalSizeBytes = 0;
		totalSizeBytes += verticesSizeBytes;
		totalSizeBytes += normalsSizeBytes;
		totalSizeBytes += colorsSizeBytes;
		totalSizeBytes += texCoordsSizeBytes;
		
		// allocate the VBO and upload the data
		mVbo = gl::Vbo::create( GL_ARRAY_BUFFER, totalSizeBytes );
		BufferScope bufferScope( mVbo );
		
		// upload positions
		GLintptr offset = 0;
		mVbo->bufferSubData( offset, verticesSizeBytes, &mVertices[0] );
		offset += verticesSizeBytes;
		
		// upload normals
		if( ! mNormals.empty() ) {
			mVbo->bufferSubData( offset, normalsSizeBytes, &mNormals[0] );
			offset += normalsSizeBytes;
		}

		// upload colors
		if( ! mColors.empty() ) {
			mVbo->bufferSubData( offset, colorsSizeBytes, &mColors[0] );
			offset += colorsSizeBytes;
		}

		// upload texCoords
		if( ! mTexCoords.empty() ) {
			mVbo->bufferSubData( offset, texCoordsSizeBytes, &mTexCoords[0] );
			offset += texCoordsSizeBytes;
		}
	}

	// Setup the VAO
	mVao = gl::Vao::create();
	GlslProgRef shader = ctx->getCurrentShader();
	VaoScope vaoScope( mVao );
	BufferScope vboScope( mVbo );
	size_t offset = 0;
	if( shader->hasAttribSemantic( ATTRIB_POSITION ) ) {
		int loc = shader->getAttribSemanticLocation( ATTRIB_POSITION );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += verticesSizeBytes;
	}

	if( shader->hasAttribSemantic( ATTRIB_NORMAL ) && ( ! mNormals.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( ATTRIB_NORMAL );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 3, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += normalsSizeBytes;
	}

	if( shader->hasAttribSemantic( ATTRIB_COLOR ) && ( ! mColors.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( ATTRIB_COLOR );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += colorsSizeBytes;
	}

	if( shader->hasAttribSemantic( ATTRIB_TEX_COORD_0 ) && ( ! mTexCoords.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( ATTRIB_TEX_COORD_0 );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
	}
	
/*	VertexLayout vtxl;
	vtxl.attrib( ATTRIB_VERTEX, &mVertices );
	if( ! mNormals.empty() )
		vtxl.attrib( ATTRIB_NORMAL, &mNormals );
	if( ! layoutMatches( mCachedLayout ) ) {
		buildBuffers( vtxl, ctx->getShader(), &mVbo, &mVao );
		mCachedLayout = vtxl;
	}*/
}

} } // namespace cinder::gl