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
		copyData( dims, srcData, count, attrInfo.getDims(), attrInfo.getStride(), reinterpret_cast<float*>( mData + attrInfo.getOffset() ) ); 
	}
}

void VboMeshGeomTarget::copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex )
{
	mVboMesh->mNumIndices = numIndices;

	if( requiredBytesPerIndex <= 2 ) {
		mVboMesh->mIndexType = GL_UNSIGNED_SHORT;
		std::unique_ptr<uint16_t> indices( new uint16_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		mVboMesh->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint16_t), indices.get() );
	}
	else {
		mVboMesh->mIndexType = GL_UNSIGNED_INT;
		std::unique_ptr<uint32_t> indices( new uint32_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		mVboMesh->mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), indices.get() );
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// VboMesh
VboMeshRef VboMesh::create( const geom::Source &source )
{
	return VboMeshRef( new VboMesh( source ) );
}

VboMeshRef VboMesh::create( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo )
{
	return VboMeshRef( new VboMesh( numVertices, numIndices, glPrimitive, indexType, vertexArrayBuffers, indexVbo ) );
}

VboMesh::VboMesh( const geom::Source &source )
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
	std::unique_ptr<uint8_t> buffer( new uint8_t[vertexDataSizeBytes] );
	
	VboMeshGeomTarget target( source.getPrimitive(), bufferLayout, buffer.get(), this );
	source.loadInto( &target );

	VboRef vertexDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, vertexDataSizeBytes, buffer.get() );	
	mVertexArrayVbos.push_back( make_pair( bufferLayout, vertexDataVbo ) );
}

VboMesh::VboMesh( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo )
	: mNumVertices( numVertices ), mNumIndices( numIndices ), mGlPrimitive( glPrimitive ), mIndexType( indexType ), mVertexArrayVbos( vertexArrayBuffers ), mElements( indexVbo )
{
}

VaoRef VboMesh::buildVao( const GlslProgRef &shader )
{
	VaoRef result = Vao::create();
	VaoScope vaoScope( result );
	
	auto ctx = gl::context();
	
	// iterate all the vertex array VBOs; map<geom::BufferLayout,VboRef>
	for( const auto &vertArrayVbo : mVertexArrayVbos ) {
		// bind this VBO (to the current VAO)
		vertArrayVbo.second->bind();
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			// get the location of the attrib semantic in the shader if it's present
			if( shader->hasAttribSemantic( attribInfo.getAttrib() ) ) {
				int loc = shader->getAttribSemanticLocation( attribInfo.getAttrib() );
				ctx->enableVertexAttribArray( loc );
				ctx->vertexAttribPointer( loc, attribInfo.getDims(), GL_FLOAT, GL_FALSE, attribInfo.getStride(), (const void*)attribInfo.getOffset() );
			}
		}
	}
	
	if( mNumIndices > 0 )
		mElements->bind();
	
	return result;
}

void VboMesh::drawImpl()
{
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
	if( mGlPrimitive == GL_UNSIGNED_SHORT ) {
		const uint16_t *source = reinterpret_cast<const uint16_t*>( data );
		for( size_t e = 0; e < getNumIndices(); ++e )
			dest[e] = source[e];
	}
	else
		memcpy( dest, data, getNumIndices() * sizeof(uint32_t) );
		
	mElements->unmap();
}

void VboMesh::echoVertexRange( std::ostream &os, size_t startIndex, size_t endIndex )
{
	vector<string> attribSemanticNames;
	vector<vector<string>> attribData;
	vector<size_t> attribColLengths;
	vector<string> attribColLeadingSpaceStrings;

	startIndex = std::min<size_t>( startIndex, mNumVertices );
	endIndex = std::min<size_t>( endIndex, mNumVertices );

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
			for( size_t v = startIndex; v < endIndex; ++v ) {
				ostringstream ss;
				const float *dataFloat = reinterpret_cast<const float*>( (const uint8_t*)rawData + attribInfo.getOffset() + v * stride );
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

	// print attrib semantic header
	ostringstream ss;
	for( size_t a = 0; a < attribSemanticNames.size(); ++a ) {
		// character offset where we should be for this column
		size_t colStartCharIndex = 0;
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
	for( size_t v = 0; v < ( endIndex - startIndex ); ++v ) {
		ostringstream ss;
		for( size_t a = 0; a < attribSemanticNames.size(); ++a ) {
			// character offset where we should be for this column
			size_t colStartCharIndex = 0;
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
		
		std::unique_ptr<uint32_t> indices( new uint32_t[mVboMesh->getNumIndices()] );
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
