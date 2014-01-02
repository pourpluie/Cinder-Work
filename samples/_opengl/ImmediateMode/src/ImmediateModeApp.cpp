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
	void draw();
};

void ImmediateModeApp::setup()
{
	gl::bindStockShader( gl::ShaderDef().color() );
}

void ImmediateModeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );	

	gl::VertBatch vb( GL_TRIANGLES );
		vb.color( 1, 0, 0 );
		vb.vertex( getWindowWidth() / 2, 50 );
		vb.color( 0, 1, 0 );
		vb.vertex( getWindowWidth() - 50, getWindowHeight() - 50 );
		vb.color( 0, 0, 1 );
		vb.vertex( 50, getWindowHeight() - 50 );
	vb.draw();
}

CINDER_APP_NATIVE( ImmediateModeApp, RendererGl )
