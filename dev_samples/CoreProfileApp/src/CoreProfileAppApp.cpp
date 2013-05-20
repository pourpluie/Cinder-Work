#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CoreProfileAppApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void CoreProfileAppApp::setup()
{
}

void CoreProfileAppApp::mouseDown( MouseEvent event )
{
}

void CoreProfileAppApp::update()
{
}

void CoreProfileAppApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( CoreProfileAppApp, RendererGl )
