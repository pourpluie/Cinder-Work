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

#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Context.h"

using namespace std;

namespace cinder { namespace gl {

/////////////////////////////////////////////////////////////////////////////////////////////////
// VboMeshGeomSource
#if ! defined( CINDER_GLES )
class VboMeshSource : public geom::Source {
  public:
	static std::shared_ptr<VboMeshSource>	create( const gl::VboMesh *vboMesh );
	
	virtual void	loadInto( geom::Target *target ) const override;
	
	virtual size_t			getNumVertices() const override;
	virtual size_t			getNumIndices() const override;
	virtual geom::Primitive	getPrimitive() const override;
	
	virtual uint8_t			getAttribDims( geom::Attrib attr ) const override;
	
  protected:
	VboMeshSource( const gl::VboMesh *vboMesh );
  
	const gl::VboMesh		*mVboMesh;
};
#endif // ! defined( CINDER_GLES )

/////////////////////////////////////////////////////////////////////////////////////////////////
// VboMeshGeomTarget
class VboMeshGeomTarget : public geom::Target {
  public:
	VboMeshGeomTarget( geom::Primitive prim, const geom::BufferLayout &bufferLayout, uint8_t *data, VboMesh *vboMesh )
		: mPrimitive( prim ), mBufferLayout( bufferLayout ), mData( data ), mVboMesh( vboMesh )
	{
		mVboMesh->mNumIndices = 0; // this may be replaced later with a copyIndices call
	}
	
	virtual geom::Primitive	getPrimitive() const override;
	virtual uint8_t	getAttribDims( geom::Attrib attr ) const override;
	virtual void copyAttrib( geom::Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count ) override;
	virtual void copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex ) override;
	
  protected:
	geom::Primitive				mPrimitive;
	const geom::BufferLayout	&mBufferLayout;
	uint8_t						*mData;
	VboMesh						*mVboMesh;
};

geom::Primitive	VboMeshGeomTarget::getPrimitive() const
{
	return mPrimitive;
}

uint8_t	VboMeshGeomTarget::getAttribDims( geom::Attrib attr ) const
{
	return mBufferLayout.getAttribDims( attr );
}

void VboMeshGeomTarget::copyAttrib( geom::Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count )
{
//	mMesh->copyAttrib( attr, dims, strideBytes, srcData, count );
	if( mBufferLayout.hasAttrib( attr ) ) {
		geom::BufferLayout::AttribInfo attrInfo = mBufferLayout.getAttribInfo( attr );
		geom::copyData( dims, srcData, count, attrInfo.getDims(), attrInfo.getStride(), reinterpret_cast<float*>( mData + attrInfo.getOffset() ) ); 
	}
}

void VboMeshGeomTarget::copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex )
{
	mVboMesh->mNumIndices = numIndices;

	if( requiredBytesPerIndex <= 2 ) {
		mVboMesh->mIndexType = GL_UNSIGNED_SHORT;
		std::unique_ptr<uint16_t[]> indices( new uint16_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		if( ! mVboMesh->mElements )
			mVboMesh->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint16_t), indices.get() );
		else
			mVboMesh->mElements->copyData( numIndices * sizeof(uint16_t), indices.get() );
	}
	else {
		mVboMesh->mIndexType = GL_UNSIGNED_INT;
		std::unique_ptr<uint32_t[]> indices( new uint32_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		if( ! mVboMesh->mElements )
			mVboMesh->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), indices.get() );
		else
			mVboMesh->mElements->copyData( numIndices * sizeof(uint32_t), indices.get() );
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// VboMesh
VboMeshRef VboMesh::create( const geom::Source &source )
{
	return VboMeshRef( new VboMesh( source, VboRef(), VboRef() ) );
}

VboMeshRef VboMesh::create( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo )
{
	return VboMeshRef( new VboMesh( numVertices, numIndices, glPrimitive, indexType, vertexArrayBuffers, indexVbo ) );
}

VboMeshRef VboMesh::create( const geom::Source &source, const VboRef &arrayVbo, const VboRef &elementArrayVbo )
{
	return VboMeshRef( new VboMesh( source, arrayVbo, elementArrayVbo ) );
}

VboMesh::VboMesh( const geom::Source &source, const VboRef &arrayVbo, const VboRef &elementArrayVbo )
{
	mNumVertices = source.getNumVertices();

	mGlPrimitive = toGl( source.getPrimitive() );

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
	std::unique_ptr<uint8_t[]> buffer( new uint8_t[vertexDataSizeBytes] );
	
	// Set our elements VBO to elementArrayVBO, which may well be empty, so that the target doesn't blow it away. Must do this before we loadInto().
	mElements = elementArrayVbo;
	
	VboMeshGeomTarget target( source.getPrimitive(), bufferLayout, buffer.get(), this );
	source.loadInto( &target );

	VboRef vertexDataVbo = arrayVbo;
	if( ! vertexDataVbo )
		vertexDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, vertexDataSizeBytes, buffer.get() );
	else
		vertexDataVbo->copyData( vertexDataSizeBytes, buffer.get() );

	mVertexArrayVbos.push_back( make_pair( bufferLayout, vertexDataVbo ) );
}

VboMesh::VboMesh( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo )
	: mNumVertices( numVertices ), mNumIndices( numIndices ), mGlPrimitive( glPrimitive ), mIndexType( indexType ), mVertexArrayVbos( vertexArrayBuffers ), mElements( indexVbo )
{
}

void VboMesh::buildVao( const GlslProgRef &shader, const AttributeMapping &attributeMapping )
{
	auto ctx = gl::context();
	
	// iterate all the vertex array VBOs; map<geom::BufferLayout,VboRef>
	for( const auto &vertArrayVbo : mVertexArrayVbos ) {
		// bind this VBO (to the current VAO)
		vertArrayVbo.second->bind();
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			int loc = -1;
			// first see if we have a mapping in 'attributeMapping'
			auto attributeMappingIt = attributeMapping.find( attribInfo.getAttrib() );
			if( attributeMappingIt != attributeMapping.end() ) {
				loc = shader->getAttribLocation( attributeMappingIt->second );
			}
			// otherwise, try to get the location of the attrib semantic in the shader if it's present
			else if( shader->hasAttribSemantic( attribInfo.getAttrib() ) ) {
				loc = shader->getAttribSemanticLocation( attribInfo.getAttrib() );			
			}
			
			// if either the shader's mapping or 'attributeMapping' has this semantic, add it to the VAO
			if( loc != -1 ) {
				ctx->enableVertexAttribArray( loc );
				ctx->vertexAttribPointer( loc, attribInfo.getDims(), GL_FLOAT, GL_FALSE, attribInfo.getStride(), (const void*)attribInfo.getOffset() );
				if( attribInfo.getInstanceDivisor() > 0 )
					ctx->vertexAttribDivisor( loc, attribInfo.getInstanceDivisor() );
			}
		}
	}
	
	if( mNumIndices > 0 )
		mElements->bind();
}

void VboMesh::drawImpl()
{
context()->sanityCheck();
	if( mNumIndices )
		glDrawElements( mGlPrimitive, mNumIndices, mIndexType, (GLvoid*)( 0 ) );
	else
		glDrawArrays( mGlPrimitive, 0, mNumVertices );
}

std::vector<VboRef>	VboMesh::getVertexArrayVbos()
{
	std::vector<VboRef> result;
	for( auto &it : mVertexArrayVbos ) {
		result.push_back( it.second );
	}
	
	return result;
}

void VboMesh::appendVbo( const geom::BufferLayout &layout, const VboRef &vbo )
{
	mVertexArrayVbos.push_back( make_pair( layout, vbo ) );
}

uint8_t	VboMesh::getAttribDims( geom::Attrib attr ) const
{
	for( const auto &vertArrayVbo : mVertexArrayVbos ) {
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			if( attribInfo.getAttrib() == attr )
				return attribInfo.getDims();
		}
	}
	
	// not found
	return 0;
}

#if ! defined( CINDER_GLES )
geom::SourceRef	VboMesh::createSource() const
{
	return VboMeshSource::create( this );
}

void VboMesh::downloadIndices( uint32_t *dest ) const
{
	if( (! mElements) || (getNumIndices() == 0) )
		return;

#if defined( CINDER_GLES )
	const void *data = mElements->map( GL_READ_ONLY_OES );
#else
	const void *data = mElements->map( GL_READ_ONLY );
#endif
	if( mIndexType == GL_UNSIGNED_SHORT ) {
		const uint16_t *source = reinterpret_cast<const uint16_t*>( data );
		for( size_t e = 0; e < getNumIndices(); ++e )
			dest[e] = source[e];
	}
	else
		memcpy( dest, data, getNumIndices() * sizeof(uint32_t) );
		
	mElements->unmap();
}

void VboMesh::echoElementRange( std::ostream &os, size_t startIndex, size_t endIndex )
{
	if( ( mNumIndices == 0 ) || ( ! mElements ) )
		return;

	vector<uint32_t> elements;
	startIndex = std::min<size_t>( startIndex, mNumIndices );
	endIndex = std::min<size_t>( endIndex, mNumIndices );

	const void *rawData = mElements->map( GL_READ_ONLY );
	switch( mIndexType ) {
		case GL_UNSIGNED_BYTE:
			for( size_t v = startIndex; v < endIndex; ++v )
				elements.push_back( reinterpret_cast<const uint8_t*>( rawData )[v] );
		break;
		case GL_UNSIGNED_SHORT:
			for( size_t v = startIndex; v < endIndex; ++v )
				elements.push_back( reinterpret_cast<const uint16_t*>( rawData )[v] );
		break;
		case GL_UNSIGNED_INT:
			for( size_t v = startIndex; v < endIndex; ++v )
				elements.push_back( reinterpret_cast<const uint32_t*>( rawData )[v] );
		break;
		default:
			return;
	}

	echoVertices( os, elements, true );
}

void VboMesh::echoVertexRange( std::ostream &os, size_t startIndex, size_t endIndex )
{
	vector<uint32_t> elements;
	startIndex = std::min<size_t>( startIndex, mNumVertices );
	endIndex = std::min<size_t>( endIndex, mNumVertices );
	elements.resize( endIndex - startIndex );
	for( size_t s = 0; s < endIndex - startIndex; ++s )
		elements[s] = s + startIndex;
	echoVertices( os, elements, false );
}

void VboMesh::echoVertices( std::ostream &os, const vector<uint32_t> &elements, bool printElements )
{
	vector<string> attribSemanticNames;
	vector<vector<string>> attribData;
	vector<size_t> attribColLengths;
	vector<string> attribColLeadingSpaceStrings;

	// save the GL_ARRAY_BUFFER binding
	auto prevBufferBinding = gl::context()->getBufferBinding( GL_ARRAY_BUFFER );

	// iterate all the vertex array VBOs; map<geom::BufferLayout,VboRef>
	for( const auto &vertArrayVbo : mVertexArrayVbos ) {
		// map this VBO
		const void *rawData = vertArrayVbo.second->map( GL_READ_ONLY );
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			attribSemanticNames.push_back( geom::attribToString( attribInfo.getAttrib() ) );
			attribData.push_back( vector<string>() );
			size_t stride = ( attribInfo.getStride() == 0 ) ? attribInfo.getDims() * sizeof(float) : attribInfo.getStride();
			for( size_t vIt : elements ) {
				ostringstream ss;
				const float *dataFloat = reinterpret_cast<const float*>( (const uint8_t*)rawData + attribInfo.getOffset() + vIt * stride );
				for( uint8_t d = 0; d < attribInfo.getDims(); ++d ) {
					ss << dataFloat[d];
					if( d != attribInfo.getDims() - 1 )
						ss << ",";
				}
				attribData.back().push_back( ss.str() );
			}
			
			// now calculate the longest string in the column
			attribColLengths.push_back( attribSemanticNames.back().length() + 3 );
			for( auto &attribDataStr : attribData.back() ) {
				attribColLengths.back() = std::max<size_t>( attribColLengths.back(), attribDataStr.length() + 2 );
			}
		}
		
		vertArrayVbo.second->unmap();
	}

	// if we're printing indices then we need to determine the widest as a string
	size_t rowStart = 0;
	if( printElements ) {
		for( uint32_t v : elements )
			rowStart = std::max( rowStart, to_string( v ).length() );
		rowStart += 2; // account for ": "
	}

	// print attrib semantic header
	ostringstream ss;
	for( size_t a = 0; a < attribSemanticNames.size(); ++a ) {
		// character offset where we should be for this column
		size_t colStartCharIndex = rowStart;
		for( size_t sumA = 0; sumA < a; ++sumA )
			colStartCharIndex += attribColLengths[sumA];
		// offset relative to the previous
		int numSpaces = std::max<int>( colStartCharIndex - ss.str().length(), 0 );
		// center string
		numSpaces += std::max<int>( (attribColLengths[a] - (attribSemanticNames[a].length()+2)) / 2, 0 );
		for( size_t s = 0; s < numSpaces; s++ )
			ss << " ";
		ss << "<" << attribSemanticNames[a] << "> ";
	}
	os << ss.str();
	os << std::endl;

	// print data rows
	for( size_t v = 0; v < elements.size(); ++v ) {
		ostringstream ss;
		if( printElements ) {
			string elementStr = to_string( elements[v] ) + ":";
			for( size_t space = 0; space < rowStart - elementStr.length(); ++space )
				ss << ' ';
			ss << elementStr;
			ss << ' ';
		}

		for( size_t a = 0; a < attribSemanticNames.size(); ++a ) {
			// character offset where we should be for this column
			size_t colStartCharIndex = rowStart;
			for( size_t sumA = 0; sumA < a; ++sumA )
				colStartCharIndex += attribColLengths[sumA];
			// offset relative to the previous
			int numSpaces = std::max<int>( colStartCharIndex - ss.str().length(), 0 );
			// center string
			numSpaces += std::max<int>( (attribColLengths[a] - attribData[a][v].length()) / 2, 0 );
			for( size_t s = 0; s < numSpaces; s++ )
				ss << " ";
			ss << attribData[a][v];
		}
		os << ss.str();
		os << std::endl;
	}
	os << std::endl;

	// restore the GL_ARRAY_BUFFER binding
	gl::context()->bindBuffer( GL_ARRAY_BUFFER, prevBufferBinding );
}

#endif // ! defined( CINDER_GLES )


#if ! defined( CINDER_GLES )

VboMeshSource::VboMeshSource( const gl::VboMesh *vboMesh )
	: mVboMesh( vboMesh )
{
}

std::shared_ptr<VboMeshSource> VboMeshSource::create( const gl::VboMesh *vboMesh )
{
	return std::shared_ptr<VboMeshSource>( new VboMeshSource( vboMesh ) );
}

void VboMeshSource::loadInto( geom::Target *target ) const
{
	// iterate all the vertex array VBOs; map<geom::BufferLayout,VboRef>
	for( const auto &vertArrayVbo : mVboMesh->getVertexArrayLayoutVbos() ) {
		// map this VBO
		const void *rawData = vertArrayVbo.second->map( GL_READ_ONLY );
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			target->copyAttrib( attribInfo.getAttrib(), attribInfo.getDims(), attribInfo.getStride(), (const float*)((const uint8_t*)rawData + attribInfo.getOffset()), getNumVertices() );
		}
		
		vertArrayVbo.second->unmap();
	}
	
	// copy index data if it's present
	if( mVboMesh->getNumIndices() ) {
		uint8_t bytesPerIndex;
		switch( mVboMesh->getIndexDataType() ) {
			case GL_UNSIGNED_SHORT:
				bytesPerIndex = 2;
			break;
			case GL_UNSIGNED_INT:
				bytesPerIndex = 4;
			break;
		}
		
		std::unique_ptr<uint32_t[]> indices( new uint32_t[mVboMesh->getNumIndices()] );
		mVboMesh->downloadIndices( indices.get() );
		target->copyIndices( gl::toGeomPrimitive( mVboMesh->getGlPrimitive() ), indices.get(), mVboMesh->getNumIndices(), bytesPerIndex );
	}
}

size_t VboMeshSource::getNumVertices() const
{
	return mVboMesh->getNumVertices();
}

size_t VboMeshSource::getNumIndices() const
{
	return mVboMesh->getNumIndices();
}

geom::Primitive VboMeshSource::getPrimitive() const
{
	return gl::toGeomPrimitive( mVboMesh->getGlPrimitive() );
}

uint8_t VboMeshSource::getAttribDims( geom::Attrib attr ) const
{
	return mVboMesh->getAttribDims( attr );
}

#endif // ! defined( CINDER_GLES )

} } // namespace cinder::gl
