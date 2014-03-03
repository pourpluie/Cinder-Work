#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/PolyLine.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int cCircleRadius = 40.0f;
const int cGridStep = cCircleRadius * 2.2f;

/**
 ConvenienceDrawingMethodsApp:

 Demonstrates most of Cinder's convenience drawing methods.
 These are not the most CPU/GPU-efficient methods of drawing, but they
 are a quick way to get something on screen.
 */
class ConvenienceDrawingMethodsApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
private:
	Path2d			mPath;
	PolyLine<Vec2f>	mPolyline2D;
	PolyLine<Vec3f>	mPolyline3D;
};

void ConvenienceDrawingMethodsApp::setup()
{
	mPath.arc( Vec2f::zero(), cCircleRadius, 0.0f, M_PI * 1.66f );
	mPath.lineTo( Vec2f::zero() );

	Rand r;
	for( int i = 0; i < 50; ++i )
	{
		mPolyline2D.push_back( r.nextVec2f() * cCircleRadius );
	}
	for( int i = 0; i < 50; ++i )
	{
		mPolyline3D.push_back( r.nextVec3f() * cCircleRadius );
	}
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

	// Draw some rows of circles
	const int numCircles = 4;
	gl::color( Color( 1.0f, 1.0f, 0.0f ) );
	for( int i = 0; i < numCircles; ++i ) {
		gl::drawSolidCircle( Vec2f( (i + 1.0f) * cGridStep, cGridStep ), cCircleRadius, i * 3 );
	}

	gl::color( Color( 1.0f, 0.0f, 0.0f ) );
	for( int i = 0; i < numCircles; ++i ) {
		gl::drawStrokedCircle( Vec2f( (i + 1.0f) * cGridStep, cGridStep ), cCircleRadius, i * 3 );
	}

	// Draw a line in 2D
	gl::drawLine( Vec2f( 10.0f, cGridStep * 1.5f ), Vec2f( getWindowWidth() - 10.0f, cGridStep * 1.5f ) );

	// Draw a Path2d both stroked and filled
	gl::pushModelView();
	gl::translate( cGridStep, cGridStep * 2.0f );
	gl::draw( mPath );
	gl::translate( cGridStep, 0.0f );
	gl::color( Color( 1.0f, 1.0f, 0.0f ) );
	gl::drawSolid( mPath );
	gl::popModelView();

	// Draw a 2D PolyLine both stroked and filled
	gl::pushModelView();
	gl::translate( cGridStep, cGridStep * 3.0f );
	gl::color( Color( 1.0f, 0.0f, 0.0f ) );
	gl::draw( mPolyline2D );
	gl::translate( cGridStep, 0.0f );
	gl::color( Color( 1.0f, 1.0f, 0.0f ) );
	gl::drawSolid( mPolyline2D );
	gl::popModelView();

	// Draw a 3D PolyLine and a 3D line
	gl::pushModelView();
	gl::translate( cGridStep, cGridStep * 4.0f );
	gl::pushModelView();
	gl::rotate( getElapsedSeconds() * 45.0f, 0.0f, 1.0f, 0.0f );
	gl::color( Color( 1.0f, 0.0f, 0.0f ) );
	gl::draw( mPolyline3D );
	gl::popModelView();

	gl::popModelView();
}

CINDER_APP_NATIVE( ConvenienceDrawingMethodsApp, RendererGl )
