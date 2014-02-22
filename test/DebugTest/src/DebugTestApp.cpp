#include "cinder/app/AppBasic.h"
//#include "cinder/app/Renderer.h"

#include "cinder/Log.h"

#define CI_ASSERT_DEBUG_BREAK
#include "cinder/CinderAssert.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DebugTestApp : public AppBasic {
	void setup();

	void keyDown( KeyEvent event );
};

void DebugTestApp::setup()
{
//	CI_ASSERT( false );
//	CI_ASSERT_MSG( false, "blarg" );

	int i = 0;
	CI_VERIFY( ( i += 1 ) );

//	log::reset( new log::LoggerNSLog );

	log::add( new log::LoggerFileThreadSafe );

	CI_LOG_I( "i = " << i ); // should be 1 in both debug and release
}

void DebugTestApp::keyDown( KeyEvent event )
{
	CI_LOG_I( "event char: " << event.getChar() << ", code: " << event.getCode() );
}

CINDER_APP_BASIC( DebugTestApp, Renderer2d )
