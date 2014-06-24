#include "Resources.h"

#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Shader.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"

using namespace ci;
using namespace ci::app;

using std::vector;


/*** This sample demonstrates the VboMesh class by creating a pair of grid meshes with textures mapped to them.
 * The meshes have static indices and texture coordinates, but the vertex positions are dynamic.
 * The second mesh which shares static and index buffers with the first, but has its own dynamic buffer for positions ***/
class VboSampleApp : public AppBasic {
 public:
	void setup();
	void update();
	void draw();

	static const int VERTICES_X = 250, VERTICES_Z = 50;

	gl::VboMeshRef	mVboMesh, mVboMesh2;
	gl::BatchRef	mBatch1, mBatch2;
	gl::TextureRef	mTexture;
	CameraPersp		mCamera;
};

void VboSampleApp::setup()
{
	// setup the parameters of the Vbo
	int totalVertices = VERTICES_X * VERTICES_Z;

	// buffer our static data - the texcoords and the indices
	vector<uint32_t> indices;
	vector<Vec2f> texCoords;
	for( int x = 0; x < VERTICES_X; ++x ) {
		for( int z = 0; z < VERTICES_Z; ++z ) {
			// create a quad for each vertex, except for along the bottom and right edges
			if( ( x + 1 < VERTICES_X ) && ( z + 1 < VERTICES_Z ) ) {
				indices.push_back( (x+0) * VERTICES_Z + (z+0) );
				indices.push_back( (x+0) * VERTICES_Z + (z+1) );
				indices.push_back( (x+1) * VERTICES_Z + (z+1) );
				indices.push_back( (x+1) * VERTICES_Z + (z+1) );
				indices.push_back( (x+0) * VERTICES_Z + (z+0) );
				indices.push_back( (x+1) * VERTICES_Z + (z+0) );
			}
			// the texture coordinates are mapped to [0,1.0)
			texCoords.push_back( Vec2f( x / (float)VERTICES_X, z / (float)VERTICES_Z ) );
		}
	}

/*	geom::BufferLayout texCoordLayout = { { geom::AttribInfo( geom::TEX_COORD_0, 2, 0, 0 ) } };
	gl::VboRef texCoordsVbo = gl::Vbo::create( GL_ARRAY_BUFFER, texCoords, GL_STATIC_DRAW );
	geom::BufferLayout positionLayout = { { geom::AttribInfo( geom::POSITION, 3, 0, 0 ) } };
	gl::VboRef positionsVbo = gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(Vec3f) * totalVertices, nullptr, GL_STREAM_DRAW );
	gl::VboRef indexVbo = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW );
	mVboMesh = gl::VboMesh::create( totalVertices, GL_TRIANGLES,
									{ { texCoordLayout, texCoordsVbo }, { positionLayout, positionsVbo } },
									indices.size(), GL_UNSIGNED_INT, indexVbo );

	gl::VboRef indexVbo = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW );									
	mVboMesh = gl::VboMesh::create( totalVertices, GL_TRIANGLES,
									{ { texCoordLayout, texCoordsVbo }, { positionLayout, positionsVbo } },
									indices.size(), GL_UNSIGNED_INT, indexVbo );	
*/	
	// make a second Vbo that uses the statics from the first
	gl::VboRef indexVbo = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW );									

	mVboMesh = gl::VboMesh::create( totalVertices, GL_TRIANGLES,
									{ gl::VboMesh::Layout().attrib( geom::TEX_COORD_0, 2 ).usage( GL_STATIC_DRAW ),
									  gl::VboMesh::Layout().attrib( geom::POSITION, 3 ).usage( GL_STREAM_DRAW ) },
									indices.size(), GL_UNSIGNED_INT, indexVbo );
	
	mVboMesh->bufferAttrib( geom::TEX_COORD_0, texCoords.size() * 8, texCoords.data() );
	
	mTexture = gl::Texture::create( loadImage( loadResource( RES_IMAGE ) ) );
	
	mBatch1 = gl::Batch::create( mVboMesh, gl::getStockShader( gl::ShaderDef().texture() ) );
}

void VboSampleApp::update()
{
	const float timeFreq = 5.0f;
	const float zFreq = 3.0f;
	const float xFreq = 7.0f;
	float offset = getElapsedSeconds() * timeFreq;

	// dynamically generate our new positions based on a simple sine wave
	auto positions = mVboMesh->mapAttrib3f( geom::POSITION );
	for( int x = 0; x < VERTICES_X; ++x ) {
		for( int z = 0; z < VERTICES_Z; ++z ) {
			float height = sin( z / (float)VERTICES_Z * zFreq + x / (float)VERTICES_X * xFreq + offset ) / 5.0f;
			*positions = Vec3f( x / (float)VERTICES_X, height, z / (float)VERTICES_Z );
			++positions;
		}
	}
	positions.unmap();

	// dynamically generate our new positions based on a simple sine wave for mesh2
/*	auto positions2 = mVboMesh2->mapAttrib3f( geom::POSITION );
	for( int x = 0; x < VERTICES_X; ++x ) {
		for( int z = 0; z < VERTICES_Z; ++z ) {
			float height = sin( z / (float)VERTICES_Z * zFreq * 2 + x / (float)VERTICES_X * xFreq * 3 + offset ) / 10.0f;
			positions2[x*VERTICES_Z+z] = Vec3f( x / (float)VERTICES_X, height, z / (float)VERTICES_Z ) + Vec3f( 0, 0.5, 0 );
		}
	}
	positions2.unmap();*/
}

void VboSampleApp::draw()
{
	// this pair of lines is the standard way to clear the screen in OpenGL
	gl::clear( Color( 0.15f, 0.15f, 0.15f ) );
	gl::setMatrices( mCamera );

	gl::scale( Vec3f( 10, 10, 10 ) );
	gl::ScopedTextureBind texBind( mTexture );

	mBatch1->draw();
}

CINDER_APP_BASIC( VboSampleApp, RendererGl )
