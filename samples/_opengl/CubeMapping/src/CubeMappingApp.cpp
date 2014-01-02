#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CubeMappingApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	
	gl::TextureCubeMapRef	mCubeMap;
};

void CubeMappingApp::setup()
{
	mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "env_map.jpg" ) ) );
}

void CubeMappingApp::mouseDown( MouseEvent event )
{
}

void CubeMappingApp::update()
{
}

void CubeMappingApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( CubeMappingApp, RendererGl )
