#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/Xml.h"
#include "cinder/Timeline.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"
#include "cinder/ConcurrentCircularBuffer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FlickrTestMTApp : public AppBasic {
  public:		
	void setup();
	void update();
	void draw();
	void shutdown();
	void loadImagesThreadFn( gl::ContextRef sharedGlContext );

	ConcurrentCircularBuffer<gl::TextureRef>	*mImages;

	bool					mShouldQuit;
	shared_ptr<thread>		mThread;
	gl::TextureRef			mTexture, mLastTexture;
	Anim<float>				mFade;
	double					mLastTime;
};

void FlickrTestMTApp::setup()
{
	mShouldQuit = false;
	mImages = new ConcurrentCircularBuffer<gl::TextureRef>( 5 ); // room for 5 images
	// create and launch the thread with a new gl::Context
	gl::ContextRef backgroundCtx = gl::Context::create( gl::context() );
	mThread = shared_ptr<thread>( new thread( bind( &FlickrTestMTApp::loadImagesThreadFn, this, backgroundCtx ) ) );
	mLastTime = getElapsedSeconds();

	gl::enableAlphaBlending();
}

void FlickrTestMTApp::loadImagesThreadFn( gl::ContextRef context )
{
	ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
	// we create a new Context specific to this thread, but shared with the main Context
	// so that we can generate Textures from this background thread
	context->makeCurrent();
	vector<Url>	urls;

	// parse the image URLS from the XML feed and push them into 'urls'
	const Url sunFlickrGroup = Url( "http://api.flickr.com/services/feeds/groups_pool.gne?id=52242317293@N01&format=rss_200" );
	const XmlTree xml( loadUrl( sunFlickrGroup ) );
	for( auto item = xml.begin( "rss/channel/item" ); item != xml.end(); ++item ) {
		const XmlTree &urlXml = ( ( *item / "media:content" ) );
		urls.push_back( Url( urlXml["url"] ) );
	}

	// load images as Textures into our ConcurrentCircularBuffer
	// don't create gl::Textures on a background thread
	while( ( ! mShouldQuit ) && ( ! urls.empty() ) ) {
		try {
			console() << "Loading: " << urls.back() << std::endl;
			mImages->pushFront( gl::Texture::create( loadImage( loadUrl( urls.back() ) ) ) );
			urls.pop_back();
		}
		catch( ... ) {
			// just ignore any exceptions
		}
	}
}

void FlickrTestMTApp::update()
{
	double timeSinceLastImage = getElapsedSeconds() - mLastTime;
	if( ( timeSinceLastImage > 2.5 ) && mImages->isNotEmpty() ) {
		mLastTexture = mTexture; // the "last" texture is now the current text
		
		mImages->popBack( &mTexture );
		
		mLastTime = getElapsedSeconds();
		// blend from 0 to 1 over 1.5sec
		timeline().apply( &mFade, 0.0f, 1.0f, 1.5f );
	}	
}

void FlickrTestMTApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.2f ) );

	if( mLastTexture ) {
		gl::color( 1, 1, 1, 1.0f - mFade );
		Rectf textureBounds = mLastTexture->getBounds();
		Rectf drawBounds = textureBounds.getCenteredFit( getWindowBounds(), true );
		gl::draw( mLastTexture, drawBounds );
	}
	if( mTexture ) {
		gl::color( 1, 1, 1, mFade );
		Rectf textureBounds = mTexture->getBounds();
		Rectf drawBounds = textureBounds.getCenteredFit( getWindowBounds(), true );
		gl::draw( mTexture, drawBounds );
	}
}

void FlickrTestMTApp::shutdown()
{
	mShouldQuit = true;
	mImages->cancel();
	mThread->join();
}

CINDER_APP_BASIC( FlickrTestMTApp, RendererGl( RendererGl::Options().coreProfile( true ) ) )