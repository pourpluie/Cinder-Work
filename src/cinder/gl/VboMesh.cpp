/*
 * NOT FINISHED
 */

#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

using namespace std;



VboMeshRef VboMesh::create( const geom::Source &source )
{
	return VboMeshRef( new VboMesh( source ) );
}

VboMesh::VboMesh( const geom::Source &source )
{
	mNumVertices = source.getNumVertices();
	
	switch( source.getPrimitive() ) {
		case geom::Primitive::TRIANGLES:
			mGlPrimitive = GL_TRIANGLES;
		break;
		case geom::Primitive::TRIANGLE_STRIP:
			mGlPrimitive = GL_TRIANGLE_STRIP;
		break;
	}
	
	size_t vertexDataSizeBytes = 0;
	geom::BufferLayout bufferLayout;
	for( int attribIt = 0; attribIt < (int)geom::Attrib::NUM_ATTRIBS; ++attribIt ) {
		if( source.hasAttrib( (geom::Attrib)attribIt ) ) {
			size_t attribDim = source.getAttribDims( (geom::Attrib)attribIt );
			bufferLayout.append( (geom::Attrib)attribIt, attribDim, 0, vertexDataSizeBytes );
			vertexDataSizeBytes += attribDim * sizeof(float) * mNumVertices;
		}
	}
	
	uint8_t *buffer = new uint8_t[vertexDataSizeBytes];

	for( auto &attrInfo : bufferLayout.getAttribs() ) {
		if( source.hasAttrib( attrInfo.getAttrib() ) ) {
			source.copyAttrib( attrInfo.getAttrib(), attrInfo.getSize(), attrInfo.getStride(), (float*)&buffer[attrInfo.getOffset()] );
		}
	}
	
	VboRef vertexDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, vertexDataSizeBytes, buffer );
	delete [] buffer;
	
	mVertexArrayVbos.push_back( make_pair( bufferLayout, vertexDataVbo ) );
	
	mNumIndices = source.getNumIndices();
	if( mNumIndices ) {		
		if( mNumIndices < 65536 ) {
			mIndexType = GL_UNSIGNED_SHORT;
			uint16_t *indices = new uint16_t[mNumIndices];
			source.copyIndices( indices );
			mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, mNumIndices * sizeof(uint16_t), indices );
			delete [] indices;
		}
		else {
			mIndexType = GL_UNSIGNED_INT;
			uint32_t *indices = new uint32_t[mNumIndices];
			source.copyIndices( indices );
			mElements = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, mNumIndices * sizeof(uint32_t), indices );
			delete [] indices;
		}
	}
}

VaoRef VboMesh::buildVao( const GlslProgRef &shader )
{
	VaoRef result = Vao::create();
	VaoScope vaoScope( result );
	
	auto ctx = gl::context();
	
	// iterate all the vertex array VBOs
	for( const auto &vertArrayVbo : mVertexArrayVbos ) {
		// bind this VBO (to the current VAO)
		vertArrayVbo.second->bind();
		// now iterate the attributes associated with this VBO
		for( const auto &attribInfo : vertArrayVbo.first.getAttribs() ) {
			// get the location of the attrib semantic in the shader if it's present
			if( shader->hasAttribSemantic( attribInfo.getAttrib() ) ) {
				int loc = shader->getAttribSemanticLocation( attribInfo.getAttrib() );
				ctx->enableVertexAttribArray( loc );
				ctx->vertexAttribPointer( loc, attribInfo.getSize(), GL_FLOAT, GL_FALSE, attribInfo.getStride(), (const void*)attribInfo.getOffset() );
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

/*	{
		// PREPARE VAO
		mVao = Vao::create();
		VaoScope vaoScope( mVao );

	}*/
















#if 0

/////////////////////////////////////////////////////////////////
VboMesh::Layout::Layout()
: mAttrColor( Vao::ATTRIB_COLOR ), mAttrNormal( Vao::ATTRIB_NORMAL ),
mAttrPosition( Vao::ATTRIB_POSITION ), mAttrTexCoord( Vao::ATTRIB_TEXCOORD ),
mUsageColor( NONE ), mUsageIndex( NONE ), mUsageNormal( NONE ),
mUsagePosition( NONE ), mUsageTexCoord( NONE )
{
}

GLuint VboMesh::Layout::getColorAttribLocation() const
{
	return mAttrColor;
}

GLuint VboMesh::Layout::getNormalAttribLocation() const
{
	return mAttrNormal;
}

GLuint VboMesh::Layout::getPositionAttribLocation() const
{
	return mAttrPosition;
}

GLuint VboMesh::Layout::getTexCoordAttribLocation() const
{
	return mAttrTexCoord;
}

void VboMesh::Layout::setColorAttribLocation( GLuint index )
{
	mAttrColor = index;
}

void VboMesh::Layout::setNormalAttribLocation( GLuint index )
{
	mAttrNormal = index;
}

void VboMesh::Layout::setPositionAttribLocation( GLuint index )
{
	mAttrPosition = index;
}

void VboMesh::Layout::setTexCoordAttribLocation( GLuint index )
{
	mAttrTexCoord = index;
}
	
VboMesh::Layout::Usage VboMesh::Layout::getColorUsage() const
{
	return mUsageColor;
}

void VboMesh::Layout::setColorUsage( VboMesh::Layout::Usage u )
{
	mUsageColor = u;
}

VboMesh::Layout::Usage VboMesh::Layout::getNormalUsage() const
{
	return mUsageNormal;
}

void VboMesh::Layout::setNormalUsage( VboMesh::Layout::Usage u )
{
	mUsageNormal = u;
}

VboMesh::Layout::Usage VboMesh::Layout::getPositionUsage() const
{
	return mUsagePosition;
}

void VboMesh::Layout::setPositionUsage( VboMesh::Layout::Usage u )
{
	mUsagePosition = u;
}

VboMesh::Layout::Usage VboMesh::Layout::getTexCoordUsage() const
{
	return mUsageTexCoord;
}
	
void VboMesh::Layout::setTexCoordUsage( VboMesh::Layout::Usage u )
{
	mUsageTexCoord = u;
}
	
VboMesh::Layout::Usage VboMesh::Layout::getIndexUsage() const
{
	return mUsageIndex;
}

void VboMesh::Layout::setIndexUsage( VboMesh::Layout::Usage u )
{
	mUsageIndex = u;
}

/////////////////////////////////////////////////////////////////

VboMeshRef VboMesh::create( size_t numIndices, size_t numVertices, const VboMesh::Layout& layout, GLenum mode )
{
	return VboMeshRef( new VboMesh( numIndices, numVertices, layout, mode ) );
}
	
VboMeshRef VboMesh::create( size_t numVertices, const VboMesh::Layout& layout, GLenum mode )
{
	return VboMeshRef( new VboMesh( numVertices, layout, mode ) );
}
	
VboMeshRef VboMesh::create( const TriMesh& mesh, const VboMesh::Layout& layout )
{
	return VboMeshRef( new VboMesh( mesh, layout ) );
}

VboMesh::VboMesh( size_t numVertices, const VboMesh::Layout& layout, GLenum glPrimitive )
: mLayout( layout ), mGlPrimitive( glPrimitive ), mNumIndices( 0 ), mNumVertices( numVertices )
{
}

VboMesh::VboMesh( size_t numIndices, size_t numVertices, const VboMesh::Layout& layout, GLenum glPrimitive )
: mLayout( layout ), mGlPrimitive( glPrimitive ), mNumIndices( numIndices ), mNumVertices( numVertices )
{
}

VboMesh::VboMesh( const TriMesh& mesh, const VboMesh::Layout& layout )
: mLayout( layout ), mGlPrimitive( GL_TRIANGLES ), mNumIndices( mesh.getNumIndices() ),
mNumVertices( mesh.getNumVertices() )
{
	if ( mesh.hasColorsRGBA() && mLayout.getColorUsage() == Layout::Usage::NONE ) {
		mLayout.setColorUsage( Layout::Usage::STATIC );
	}
	if ( mesh.hasNormals() && mLayout.getNormalUsage() == Layout::Usage::NONE ) {
		mLayout.setNormalUsage( Layout::Usage::STATIC );
	}
	if ( !mesh.getVertices().empty() && mLayout.getPositionUsage() == Layout::Usage::NONE ) {
		mLayout.setPositionUsage( Layout::Usage::STATIC );
	}
	if ( mesh.hasTexCoords() && mLayout.getTexCoordUsage() == Layout::Usage::NONE ) {
		mLayout.setTexCoordUsage( Layout::Usage::STATIC );
	}
	mLayout.setIndexUsage( Layout::Usage::STATIC );
	
	if ( mVboIndices ) {
		GLuint count	= sizeof( uint32_t ) * mNumIndices;
		GLenum usage	= mLayout.getIndexUsage() == Layout::Usage::STATIC ? GL_STATIC_DRAW : GL_STREAM_DRAW;
		mVboIndices->bufferData( count, &mesh.getIndices()[ 0 ], usage );
	}
}

/*void VboMesh::bind() const
{
	mVao->bind();
	if ( mVboIndices ) {
		mVboIndices->bind();
	}
	if ( mVboVerticesDynamic ) {
		mVboVerticesDynamic->bind();
	}
	if ( mVboVerticesStatic ) {
		mVboVerticesStatic->bind();
	}
}*/

void VboMesh::unbind() const
{
	if ( mVboVerticesStatic ) {
		mVboVerticesStatic->unbind();
	}
	if ( mVboVerticesDynamic ) {
		mVboVerticesDynamic->unbind();
	}
	if ( mVboIndices ) {
		mVboIndices->unbind();
	}
	mVao->unbind();
}
	
void VboMesh::initializeBuffers()
{
	mVao = Vao::create();
	
	if ( mNumIndices > 0 ) {
		mVboIndices = Vbo::create( GL_ELEMENT_ARRAY_BUFFER );
	}
	
	if ( mNumVertices > 0 ) {
		bool hasDynamic	= mLayout.getColorUsage() == Layout::Usage::DYNAMIC || mLayout.getNormalUsage() == Layout::Usage::DYNAMIC ||
			mLayout.getPositionUsage() == Layout::Usage::DYNAMIC || mLayout.getTexCoordUsage() == Layout::Usage::DYNAMIC;
		bool hasStatic	= mLayout.getColorUsage() == Layout::Usage::STATIC || mLayout.getNormalUsage() == Layout::Usage::STATIC ||
			mLayout.getPositionUsage() == Layout::Usage::STATIC || mLayout.getTexCoordUsage() == Layout::Usage::STATIC;
		
		if ( hasDynamic ) {
			mVboVerticesDynamic = Vbo::create( GL_ARRAY_BUFFER );
			
			size_t offset = 0;
			if ( mLayout.getColorUsage() == Layout::Usage::DYNAMIC ) {
				// TODO add attribute to VAO
				offset += 4;
			}
			if ( mLayout.getNormalUsage() == Layout::Usage::DYNAMIC ) {
				// TODO add attribute to VAO
				offset += 3;
			}
			if ( mLayout.getPositionUsage() == Layout::Usage::DYNAMIC ) {
				// TODO add attribute to VAO
				offset += 3;
			}
			if ( mLayout.getTexCoordUsage() == Layout::Usage::DYNAMIC ) {
				// TODO add attribute to VAO
				offset += 4;
			}
			
			GLuint stride = offset * sizeof( GL_FLOAT ) * mNumVertices;
			mVboVerticesDynamic->bufferData( stride, 0, GL_STREAM_DRAW );
		}
		
		if ( hasStatic ) {
			mVboVerticesStatic = Vbo::create( GL_ARRAY_BUFFER );
			
			size_t offset = 0;
			if ( mLayout.getColorUsage() == Layout::Usage::STATIC ) {
				// TODO add attribute to VAO
				offset += 4 * mNumVertices;
			}
			if ( mLayout.getNormalUsage() == Layout::Usage::STATIC ) {
				// TODO add attribute to VAO
				offset += 3 * mNumVertices;
			}
			if ( mLayout.getPositionUsage() == Layout::Usage::STATIC ) {
				// TODO add attribute to VAO
				offset += 3 * mNumVertices;
			}
			if ( mLayout.getTexCoordUsage() == Layout::Usage::STATIC ) {
				// TODO add attribute to VAO
				offset += 4 * mNumVertices;
			}
			
			GLuint stride = offset * sizeof( GL_FLOAT );
			mVboVerticesStatic->bufferData( stride, 0, GL_STATIC_DRAW );
		}
	}
}
	
void VboMesh::bufferIndices( const vector<GLuint>& indices )
{
}

void VboMesh::bufferColors( const vector<ColorAf>& colors )
{
}

void VboMesh::bufferNormals( const vector<Vec3f>& normals )
{
}

void VboMesh::bufferPositions( const vector<Vec3f>& positions )
{
}

void VboMesh::bufferTexCoords( const vector<Vec2f>& texCoords )
{
}
	
void VboMesh::bufferTexCoords( const vector<Vec3f>& texCoords )
{
}
	
void VboMesh::bufferTexCoords( const vector<Vec4f>& texCoords )
{
}

#endif

} }
