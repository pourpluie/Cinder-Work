#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Batch.h"
#include "cinder/Capture.h"
#include "cinder/Camera.h"
#include "cinder/Text.h"

using namespace ci;
using namespace ci::app;

class RotatingCubeApp : public AppBasic {
  public:	
	void setup();
	void resize();
	void update();
	void draw();
	
	CameraPersp			mCam;
	Matrix44f			mCubeRotation;
	gl::BatchRef		mCubeBatch;
};

void RotatingCubeApp::setup()
{
	mCam.lookAt( Vec3f( 3, 2, -3 ), Vec3f::zero() );
	mCubeRotation.setToIdentity();
	
	gl::bindStockShader( gl::ShaderDef().color() );
	mCubeBatch = gl::Batch::create( geo::Cube(), gl::getStockShader( gl::ShaderDef().color() ) );

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
	mCubeRotation.rotate( Vec3f( 1, 1, 1 ), 0.03f );
}

void RotatingCubeApp::draw()
{
	gl::clear( Color::black() );
	gl::pushMatrices();
		gl::multModelView( mCubeRotation );
		mCubeBatch->draw();
	gl::popMatrices();
}


CINDER_APP_BASIC( RotatingCubeApp, RendererGl )