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
#include "cinder/gl/VboMesh.h"

namespace cinder { namespace gl {

/////////////////////////////////////////////////////////////////////////////////////////////////
// BatchGeomTarget
class BatchGeomTarget : public geom::Target {
  public:
	BatchGeomTarget( geom::Primitive prim, const geom::BufferLayout &bufferLayout, uint8_t *data, Batch *batch )
		: mPrimitive( prim ), mBufferLayout( bufferLayout ), mData( data ), mBatch( batch )
	{}
	
	virtual geom::Primitive	getPrimitive() const override;
	virtual uint8_t	getAttribDims( geom::Attrib attr ) const override;
	virtual void copyAttrib( geom::Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count ) override;
	virtual void copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex ) override;
	
  protected:
	geom::Primitive				mPrimitive;
	const geom::BufferLayout	&mBufferLayout;
	uint8_t						*mData;
	Batch						*mBatch;
};

geom::Primitive	BatchGeomTarget::getPrimitive() const
{
	return mPrimitive;
}

uint8_t	BatchGeomTarget::getAttribDims( geom::Attrib attr ) const
{
	return mBufferLayout.getAttribDims( attr );
}

void BatchGeomTarget::copyAttrib( geom::Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count )
{
//	mMesh->copyAttrib( attr, dims, strideBytes, srcData, count );
	if( mBufferLayout.hasAttrib( attr ) ) {
		geom::BufferLayout::AttribInfo attrInfo = mBufferLayout.getAttribInfo( attr );
		copyData( dims, srcData, count, attrInfo.getDims(), attrInfo.getStride(), reinterpret_cast<float*>( mData + attrInfo.getOffset() ) ); 
	}
}

void BatchGeomTarget::copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex )
{
	mBatch->mNumIndices = numIndices;

	if( requiredBytesPerIndex <= 2 ) {
		mBatch->mIndexType = GL_UNSIGNED_SHORT;
		std::unique_ptr<uint16_t> indices( new uint16_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		mBatch->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint16_t), indices.get() );
	}
	else {
		mBatch->mIndexType = GL_UNSIGNED_INT;
		std::unique_ptr<uint32_t> indices( new uint32_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		mBatch->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), indices.get() );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Batch
BatchRef Batch::create( const VboMeshRef &vboMesh, const gl::GlslProgRef &glsl )
{
	return BatchRef( new Batch( vboMesh, glsl ) );
}

BatchRef Batch::create( const geom::Source &source, const gl::GlslProgRef &glsl )
{
	return BatchRef( new Batch( source, glsl ) );
}

BatchRef Batch::create( const geom::SourceRef &sourceRef, const gl::GlslProgRef &glsl )
{
	return BatchRef( new Batch( sourceRef, glsl ) );
}

Batch::Batch( const VboMeshRef &vboMesh, const gl::GlslProgRef &glsl )
	: mGlsl( glsl )
{
	mVertexArrayVbos = vboMesh->getVertexArrayVbos();
	mElements = vboMesh->getElementVbo();
	mVao = vboMesh->buildVao( glsl );
	mPrimitive = vboMesh->getGlPrimitive();
	mNumVertices = vboMesh->getNumVertices();
	mNumIndices = vboMesh->getNumIndices();
	mIndexType = vboMesh->getIndexDataType();
}

Batch::Batch( const geom::Source &source, const gl::GlslProgRef &glsl )
	: mGlsl( glsl )
{
	init( source, mGlsl );
}

Batch::Batch( const geom::SourceRef &sourceRef, const gl::GlslProgRef &glsl )
	: mGlsl( glsl )
{
	init( *sourceRef, mGlsl );
}

void Batch::init( const geom::Source &source, const gl::GlslProgRef &glsl )
{
	mNumVertices = source.getNumVertices();

	mPrimitive = toGl( source.getPrimitive() );

	size_t vertexDataSizeBytes = 0;
	geom::BufferLayout bufferLayout;
	for( int attribIt = 0; attribIt < (int)geom::Attrib::NUM_ATTRIBS; ++attribIt ) {
		auto attribDims = source.getAttribDims( (geom::Attrib)attribIt );
		if( attribDims > 0 ) {
			bufferLayout.append( (geom::Attrib)attribIt, attribDims, 0, vertexDataSizeBytes );
			vertexDataSizeBytes += attribDims * sizeof(float) * mNumVertices;
		}
	}

	// TODO: this should use mapBuffer when available
	std::unique_ptr<uint8_t> buffer( new uint8_t[vertexDataSizeBytes] );
	
	BatchGeomTarget target( source.getPrimitive(), bufferLayout, buffer.get(), this );
	source.loadInto( &target );

	mVertexArrayVbos.push_back( Vbo::create( GL_ARRAY_BUFFER, vertexDataSizeBytes, buffer.get() ) );
	std::vector<std::pair<geom::BufferLayout,VboRef>> vertLayoutVbos;
	vertLayoutVbos.push_back( make_pair( bufferLayout, mVertexArrayVbos.back() ) );
	
	initVao( vertLayoutVbos );
}

void Batch::initVao( const std::vector<std::pair<geom::BufferLayout,VboRef>> &vertLayoutVbos )
{
	mVao = Vao::create();
	VaoScope vaoScope( mVao );
	
	auto ctx = gl::context();
	
	// iterate all the vertex array VBOs
	for( const auto &vertArrayVbo : vertLayoutVbos ) {
		// bind this VBO (to the current VAO)
		vertArrayVbo.second->bind();
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			// get the location of the attrib semantic in the shader if it's present
			if( mGlsl->hasAttribSemantic( attribInfo.getAttrib() ) ) {
				int loc = mGlsl->getAttribSemanticLocation( attribInfo.getAttrib() );
				ctx->enableVertexAttribArray( loc );
				ctx->vertexAttribPointer( loc, attribInfo.getDims(), GL_FLOAT, GL_FALSE, attribInfo.getStride(), (const void*)attribInfo.getOffset() );
			}
		}
	}
	
	if( mNumIndices > 0 )
		mElements->bind();
}

void Batch::draw()
{
	auto ctx = gl::context();
	
	gl::ShaderScope shaderScope( mGlsl );
	gl::VaoScope vaoScope( mVao );
	ctx->setDefaultShaderVars();
	if( mNumIndices )
		ctx->drawElements( mPrimitive, mNumIndices, mIndexType, 0 );
	else
		ctx->drawArrays( mPrimitive, 0, mNumVertices );
}

void Batch::bind()
{
	mGlsl->bind();
	mVao->bind();
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
	
	auto ctx = context();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( mPrimType, 0, mVertices.size() );
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
	if( shader->hasAttribSemantic( geom::Attrib::POSITION ) ) {
		int loc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += verticesSizeBytes;
	}

	if( shader->hasAttribSemantic( geom::Attrib::NORMAL ) && ( ! mNormals.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( geom::Attrib::NORMAL );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 3, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += normalsSizeBytes;
	}

	if( shader->hasAttribSemantic( geom::Attrib::COLOR ) && ( ! mColors.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( geom::Attrib::COLOR );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
		offset += colorsSizeBytes;
	}

	if( shader->hasAttribSemantic( geom::Attrib::TEX_COORD_0 ) && ( ! mTexCoords.empty() ) ) {
		int loc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
		ctx->enableVertexAttribArray( loc );
		ctx->vertexAttribPointer( loc, 4, GL_FLOAT, false, 0, (const GLvoid*)offset );
	}
	
/*	VertexLayout vtxl;
	vtxl.attrib( ATTRIB_VERTEX, &mVertices );
	if( ! mNormals.empty() )
		vtxl.attrib( geom::Attrib::NORMAL, &mNormals );
	if( ! layoutMatches( mCachedLayout ) ) {
		buildBuffers( vtxl, ctx->getShader(), &mVbo, &mVao );
		mCachedLayout = vtxl;
	}*/
}

} } // namespace cinder::gl