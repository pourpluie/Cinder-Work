#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "Earth.h"
#include "POV.h"
#include "Resources.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Json.h"
#include "cinder/Url.h"
#include "cinder/Vector.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Context.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
//#include "cinder/gl/TileRender.h"

using namespace ci;
using namespace ci::app;

#include <vector>
#include <sstream>
using std::vector;
using std::string;
using std::istringstream;
using std::stringstream;


class EarthquakeApp : public AppNative {
 public:
	void prepareSettings( Settings *settings );
	void keyDown( KeyEvent event );
	void mouseDrag( MouseEvent event );
	void mouseMove( MouseEvent event );
	void mouseWheel( MouseEvent event );
	void parseEarthquakes( const string &url );
	void setup();
	void update();
	void draw();
	
	gl::GlslProgRef	mEarthShader;
	gl::GlslProgRef	mQuakeShader;
	
	gl::TextureRef	mStars;
	
	POV				mPov;
	Earth			mEarth;
	
	Vec2f			mLastMouse;
	Vec2f			mCurrentMouse;
	
	Vec3f			sBillboardUp, sBillboardRight;
	Vec3f			billboardVecs[2];
	
	Vec3f			mLightDir;
	
	float			mCounter;
	int				mCurrentFrame;
	bool			mSaveFrames;
	bool			mShowEarth;
	bool			mShowLand;
	bool			mShowOcean;
	bool			mShowText;
	bool			mShowQuakes;
};


void EarthquakeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
	settings->setFrameRate( 60.0f );
	settings->setResizable( true );
	settings->setFullScreen( false );
	settings->enableMultiTouch( false );
}

void EarthquakeApp::setup()
{
	gl::TextureRef earthDiffuse		= gl::Texture::create( loadImage( loadResource( RES_EARTHDIFFUSE ) ) );
	gl::TextureRef earthNormal		= gl::Texture::create( loadImage( loadResource( RES_EARTHNORMAL ) ) );
	gl::TextureRef earthMask		= gl::Texture::create( loadImage( loadResource( RES_EARTHMASK ) ) );
	earthDiffuse->setWrap( GL_REPEAT, GL_REPEAT );
	earthNormal->setWrap( GL_REPEAT, GL_REPEAT );
	earthMask->setWrap( GL_REPEAT, GL_REPEAT );

	mStars						= gl::Texture::create( loadImage( loadResource( RES_STARS_PNG ) ) );

#if defined( CINDER_GL_ES )	
	mEarthShader = gl::GlslProg::create( loadResource( "passThru_vert_es2.glsl" ), loadResource( "earth_frag_es2.glsl" ) );
	mQuakeShader = gl::GlslProg::create( loadResource( "quake_vert_es2.glsl" ), loadResource( "quake_frag_es2.glsl" ) );
#else
	mEarthShader = gl::GlslProg::create( loadResource( RES_PASSTHRU_VERT ), loadResource( RES_EARTH_FRAG ) );
	mQuakeShader = gl::GlslProg::create( loadResource( RES_QUAKE_VERT ), loadResource( RES_QUAKE_FRAG ) );
#endif

	
	mCounter		= 0.0f;
	mCurrentFrame	= 0;
	mSaveFrames		= false;
	mShowEarth		= true;
	mShowText		= true;
	mShowQuakes		= true;
	mLightDir		= Vec3f( 0.025f, 0.25f, 1.0f );
	mLightDir.normalize();
	mPov			= POV( this, ci::Vec3f( 0.0f, 0.0f, 1000.0f ), ci::Vec3f( 0.0f, 0.0f, 0.0f ) );
	mEarth			= Earth( mEarthShader, earthDiffuse, earthNormal, earthMask );
	
	parseEarthquakes( "http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_week.geojson" );
	
	mEarth.setQuakeLocTip();
}

void EarthquakeApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( ! isFullScreen() );
	}
	else if( event.getCode() == app::KeyEvent::KEY_ESCAPE ) {
		setFullScreen( false );
	}
	else if( event.getChar() == 's' ) {
		mSaveFrames = ! mSaveFrames;
	}
	else if( event.getChar() == 'e' ) {
		mShowEarth = ! mShowEarth;
	}
	else if( event.getChar() == 't' ) {
		mShowText = ! mShowText;
	}
	else if( event.getChar() == 'q' ) {
		mShowQuakes = ! mShowQuakes;
	}
	else if( event.getChar() == 'm' ) {
		mEarth.setMinMagToRender( -1.0f );
	}
	else if( event.getChar() == 'M' ) {
		mEarth.setMinMagToRender( 1.0f );
	}
	else if( event.getCode() == app::KeyEvent::KEY_UP ) {
		mPov.adjustDist( -10.0f );
	}
	else if( event.getCode() == app::KeyEvent::KEY_DOWN ) {
		mPov.adjustDist( 10.0f );
	}
/*	else if( event.getChar() == ' ' ) {
		gl::TileRender tr( 5000, 5000 );
		CameraPersp cam;
		cam.lookAt( mPov.mEye, mPov.mCenter );
		cam.setPerspective( 60.0f, tr.getImageAspectRatio(), 1, 20000 );
		tr.setMatrices( cam );
		while( tr.nextTile() ) {
			draw();
		}
		writeImage( getHomeDirectory() / "output.png", tr.getSurface() );
	}*/
}


void EarthquakeApp::mouseWheel( MouseEvent event )
{
	mPov.adjustDist( event.getWheelIncrement() * -2.0f );
}

void EarthquakeApp::mouseDrag( MouseEvent event )
{
	mouseMove( event );
}

void EarthquakeApp::mouseMove( MouseEvent event )
{
	static bool firstMouseMove = true;
	if( ! firstMouseMove )
		mLastMouse = mCurrentMouse;
	else {
		mLastMouse = event.getPos();
		firstMouseMove = false;
	}
	mCurrentMouse = event.getPos();;
	
	float xd = ( mCurrentMouse.x - mLastMouse.x ) * 0.025f;
	
	mPov.adjustAngle( -xd, mCurrentMouse.y - ( getWindowHeight() * 0.5f ) );
}

void EarthquakeApp::update()
{
	mPov.update();
	mPov.mCam.getBillboardVectors( &sBillboardRight, &sBillboardUp );
	
	//mLightDir = Vec3f( sin( mCounter ), 0.25f, cos( mCounter ) );
	mEarth.update();
	
	mCounter += 0.1f;
}

void EarthquakeApp::draw()
{
	glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );
	
	gl::enableAlphaBlending();
	gl::enableDepthRead( true );
	gl::enableDepthWrite( true );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	gl::color( 1, 1, 1, 1 );
	mStars->bind();
	gl::drawSphere( Vec3f( 0, 0, 0 ), 15000.0f, 64 );
	
	//gl::rotate( Quatf( Vec3f::zAxis(), -0.2f ) );
	//gl::rotate( Quatf( Vec3f::yAxis(), mCounter*0.1f ) );
	
	if( mShowEarth ){
		mEarthShader->bind();
		mEarthShader->uniform( "uTexDiffuse", 0 );
		mEarthShader->uniform( "uTexNormal", 1 );
		mEarthShader->uniform( "uTexMask", 2 );
		mEarthShader->uniform( "uLightDir", mLightDir );
		mEarth.draw();
	}
	
	gl::color( 1, 1, 1, 1 );
	if( mShowQuakes ){
		mQuakeShader->bind();
		mQuakeShader->uniform( "uLightDir", mLightDir );
		mEarth.drawQuakeVectors();
	}
	if( mShowText ){
		gl::enableDepthWrite( false );
		mEarth.drawQuakeLabelsOnSphere( mPov.mEyeNormal, mPov.mDist );
	}
	
	if( mSaveFrames ){
		writeImage( getHomeDirectory() / "CinderScreengrabs" / ( "Highoutput_" + toString( mCurrentFrame ) + ".png" ), copyWindowSurface() );
		mCurrentFrame++;
	}
}

void EarthquakeApp::parseEarthquakes( const string &url )
{
	try {
		const JsonTree json( loadUrl( url ) );
		for( auto &feature : json["features"].getChildren() ) {
			auto &coords = feature["geometry"]["coordinates"];
			float mag = feature["properties"]["mag"].getValue<float>();
			string title = feature["properties"]["title"].getValue();
			mEarth.addQuake( coords[0].getValue<float>(), coords[1].getValue<float>(), mag, title );
		}
	}
	catch( ... ) {
		console() << "Failed to parse." << std::endl;
	}
	
	//mEarth.addQuake( 37.7f, -122.0f, 8.6f, "San Francisco" );
}

CINDER_APP_NATIVE( EarthquakeApp, RendererGl )