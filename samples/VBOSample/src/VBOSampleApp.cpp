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


/*** This sample demonstrates the VboMesh class by creating a grid as a VboMesh.
 * The mesh has static indices and texture coordinates, but the vertex positions are dynamic. **/
class VboSampleApp : public AppBasic {
 public:
	void setup();
	void update();
	void draw();

	static const int VERTICES_X = 250, VERTICES_Z = 50;

	gl::VboMeshRef	mVboMesh;
	gl::BatchRef	mBatch;
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
	vector<Vec3f> colors;
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
			colors.push_back( Vec3f( x / (float)VERTICES_X, 1, z / (float)VERTICES_Z ) );
		}
	}

	mVboMesh = gl::VboMesh::create( totalVertices, GL_TRIANGLES,
									{ gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).
											attrib( geom::TEX_COORD_0, 2 ).attrib( geom::COLOR, 3 ),
									  gl::VboMesh::Layout().usage( GL_STREAM_DRAW ).attrib( geom::POSITION, 3 ) },
									indices.size(), GL_UNSIGNED_INT );
	
	mVboMesh->bufferAttrib( geom::TEX_COORD_0, texCoords );
	mVboMesh->bufferAttrib( geom::COLOR, colors );
	mVboMesh->bufferIndices( sizeof(uint32_t) * indices.size(), indices.data() );
	
	mTexture = gl::Texture::create( loadImage( loadResource( RES_IMAGE ) ) );
	
	mBatch = gl::Batch::create( mVboMesh, gl::getStockShader( gl::ShaderDef().texture().color() ) );
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
			*positions++ = Vec3f( x / (float)VERTICES_X, height, z / (float)VERTICES_Z );
		}
	}
	positions.unmap();
}

void VboSampleApp::draw()
{
	// this pair of lines is the standard way to clear the screen in OpenGL
	gl::clear( Color( 0.15f, 0.15f, 0.15f ) );
	gl::setMatrices( mCamera );

	gl::scale( Vec3f( 10, 10, 10 ) );
	gl::ScopedTextureBind texBind( mTexture );

	mBatch->draw();
}

CINDER_APP_BASIC( VboSampleApp, RendererGl )
