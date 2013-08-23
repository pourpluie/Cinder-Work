#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Batch.h"
#include "cinder/Capture.h"
#include "cinder/Camera.h"
#include "cinder/Text.h"

using namespace ci;
using namespace ci::app;

class RotatingCubeApp : public AppNative {
  public:	
	void setup();
	void resize();
	void update();
	void draw();
	
	CameraPersp			mCam;
	Matrix44f			mCubeRotation;
	gl::BatchRef		mCubeBatch;
	gl::TextureRef		mTexture;
	gl::GlslProgRef		mGlsl;
};

void RotatingCubeApp::setup()
{
	disableFrameRate();
	mCam.lookAt( Vec3f( 3, 2, 4 ), Vec3f::zero() );
	mCubeRotation.setToIdentity();
	
	mTexture = gl::Texture::create( loadImage( loadAsset( "texture.jpg" ) ) );

#if defined( CINDER_GLES )
	mGlsl = gl::GlslProg::create( loadAsset( "shader_es2.vert" ), loadAsset( "shader_es2.frag" ) );
#else
	mGlsl = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
#endif
//mGlsl = gl::getStockShader( gl::ShaderDef().texture() );
	//mCubeBatch = gl::Batch::create( geo::Rect(), mGlsl );
//	mCubeBatch = gl::Batch::create( geo::Cube(), mGlsl );
	mCubeBatch = gl::Batch::create( geo::Teapot().subdivision( 6 ).scale( 0.5f ), mGlsl );

	gl::enableDepthWrite();
	gl::enableDepthRead();
}

void RotatingCubeApp::resize()
{
	// now tell our Camera that the window aspect ratio has changed
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
	
	// and in turn, let OpenGL know we have a new camera
	gl::setMatrices( mCam );
}

void RotatingCubeApp::update()
{
	// Rotate the cube by .03 radians around an arbitrary axis
	mCubeRotation.rotate( Vec3f( 1, 1.3, 0.5 ).normalized(), 0.03f );
}

void RotatingCubeApp::draw()
{
	gl::clear( Color::black() );
	gl::pushMatrices();
		gl::multModelView( mCubeRotation );
		mCubeBatch->draw();
	gl::popMatrices();
}


CINDER_APP_NATIVE( RotatingCubeApp, RendererGl )