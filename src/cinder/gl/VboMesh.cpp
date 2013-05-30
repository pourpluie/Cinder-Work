/*
 * NOT FINISHED
 */

#include "cinder/gl/VboMesh.h"

namespace cinder { namespace gl {

using namespace std;

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

VboMesh::VboMesh( size_t numVertices, const VboMesh::Layout& layout, GLenum mode )
: mLayout( layout ), mMode( mode ), mNumIndices( 0 ), mNumVertices( numVertices )
{
}

VboMesh::VboMesh( size_t numIndices, size_t numVertices, const VboMesh::Layout& layout, GLenum mode )
: mLayout( layout ), mMode( mode ), mNumIndices( numIndices ), mNumVertices( numVertices )
{
}

VboMesh::VboMesh( const TriMesh& mesh, const VboMesh::Layout& layout )
: mLayout( layout ), mMode( GL_TRIANGLES ), mNumIndices( mesh.getNumIndices() ),
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

} }
