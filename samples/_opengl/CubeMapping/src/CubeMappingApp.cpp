#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/GlslProg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CubeMappingApp : public AppNative {
  public:
	void setup();
	void resize();
	void update();
	void draw();
	
	gl::TextureCubeMapRef	mCubeMap;
	gl::GlslProgRef			mEnvMapGlsl;
	gl::BatchRef			mTeapotBatch;
	Matrix44f				mObjectRotation;
	CameraPersp				mCam;
};

void CubeMappingApp::setup()
{
	mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "env_map.jpg" ) ) );

/*	ImageSourceRef images[6] = { loadImage( loadAsset( "posx.jpg" ) ), loadImage( loadAsset( "negx.jpg" ) ),
							loadImage( loadAsset( "posy.jpg" ) ), loadImage( loadAsset( "negy.jpg" ) ),
							loadImage( loadAsset( "posz.jpg" ) ), loadImage( loadAsset( "negz.jpg" ) ) };
	ImageSourceRef images[6] = { loadImage( loadAsset( "night_posx.png" ) ), loadImage( loadAsset( "night_negx.png" ) ),
							loadImage( loadAsset( "night_negy.png" ) ), loadImage( loadAsset( "night_posy.png" ) ),
							loadImage( loadAsset( "night_posz.png" ) ), loadImage( loadAsset( "night_negz.png" ) ) };

	mCubeMap = gl::TextureCubeMap::create( images );*/

#if defined( CINDER_GLES )
	mEnvMapGlsl = gl::GlslProg::create( loadAsset( "env_map_es2.vert" ), loadAsset( "env_map_es2.frag" ) );
#else
	mEnvMapGlsl = gl::GlslProg::create( loadAsset( "env_map.vert" ), loadAsset( "env_map.frag" ) );
#endif

//	mTeapotBatch = gl::Batch::create( geom::Teapot().normals().subdivision( 5 ), mEnvMapGlsl );
	mTeapotBatch = gl::Batch::create( geom::Sphere().normals().segments( 30 ).radius( 3 ), mEnvMapGlsl );
	mEnvMapGlsl->uniform( "uCubeMapTex", 0 );

	mCam.lookAt( Vec3f( 3, 2, 4 ), Vec3f::zero() );
	mObjectRotation.setToIdentity();

	gl::enableDepthRead();
	gl::enableDepthWrite();	
}

void CubeMappingApp::resize()
{
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
}

void CubeMappingApp::update()
{
	mObjectRotation.rotate( Vec3f( 0.0, 1.0, 0.0 ).normalized(), 0.01f );
}

void CubeMappingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::setMatrices( mCam );

	mCubeMap->bind();
	gl::pushMatrices();
		gl::multModelView( mObjectRotation );
		mTeapotBatch->draw();
	gl::popMatrices();
}

CINDER_APP_NATIVE( CubeMappingApp, RendererGl )
