#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ConvenienceDrawingMethodsApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void ConvenienceDrawingMethodsApp::setup()
{
}

void ConvenienceDrawingMethodsApp::mouseDown( MouseEvent event )
{
}

void ConvenienceDrawingMethodsApp::update()
{
}

void ConvenienceDrawingMethodsApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

	const int numCircles = 4;
	const float circleRadius = 40.0f;
	const float xStep = getWindowWidth() / numCircles;
	gl::color( Color::gray( 0.33f ) );
	for( int i = 0; i < numCircles; ++i ) {
		gl::drawSolidCircle( Vec2f( (i + 0.5f) * xStep, circleRadius * 1.5f ), circleRadius, i * 3 );
	}
	gl::color( Color( 1.0f, 1.0f, 0.0f ) );
	for( int i = 0; i < numCircles; ++i ) {
		gl::drawStrokedCircle( Vec2f( (i + 0.5f) * xStep, circleRadius * 1.5f ), circleRadius, i * 3 );
	}
}

CINDER_APP_NATIVE( ConvenienceDrawingMethodsApp, RendererGl )
