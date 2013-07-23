#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Batch.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ImmediateModeApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void ImmediateModeApp::setup()
{
	gl::bindStockShader( gl::ShaderDef().color() );
}

void ImmediateModeApp::mouseDown( MouseEvent event )
{
}

void ImmediateModeApp::update()
{
}

void ImmediateModeApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::setMatricesWindow( getWindowSize() );

	gl::color( Colorf( 1.0, 0.5f, 0.25f ) );
	
	gl::VertBatch vb( GL_TRIANGLES );
		vb.color( 1, 0, 0 );
		vb.vertex( getWindowWidth() / 2, 50 );
		vb.color( 0, 1, 0 );
		vb.vertex( getWindowWidth() - 50, getWindowHeight() - 50 );
		vb.color( 0, 0, 1 );
		vb.vertex( 50, getWindowHeight() - 50 );
	vb.draw();
}

#if 1
auto options = RendererGl::Options().coreProfile( false );
#else
auto options = RendererGl::Options();
#endif
CINDER_APP_NATIVE( ImmediateModeApp, RendererGl( options ) )
