#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CompressedTextureApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void draw();

	size_t								mIndex;
	float								mZoom;
	vector<pair<string,gl::TextureRef>> mTextures;
};

void CompressedTextureApp::setup()
{
	mIndex = 0;
	mZoom = 1.0f;

	mTextures.push_back( make_pair( "Original", gl::Texture::create( loadImage( loadAsset( "photo_4.jpg" ) ) ) ) );
	mTextures.push_back( make_pair( "DXT1", gl::Texture::createFromDds( loadAsset( "photo_4_dxt1.dds" ) ) ) );
#if defined( CINDER_MSW )
//	mTextures.push_back( make_pair( "BC7", gl::Texture::createFromDds( loadAsset( "photo_4_bc7.dds" ) ) ) );
#endif
}

void CompressedTextureApp::mouseDown( MouseEvent event )
{
	mIndex = ( mIndex + 1 ) % mTextures.size();
	console() << "Showing " << mTextures[mIndex].first << std::endl;
}

void CompressedTextureApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case KeyEvent::KEY_EQUALS:
			mZoom *= 2.0f;
		break;
		case KeyEvent::KEY_MINUS:
			mZoom /= 2.0f;
		break;
	}
}

void CompressedTextureApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

	gl::draw( mTextures[mIndex].second, Rectf( mTextures[mIndex].second->getBounds() ) * mZoom );
}

CINDER_APP_NATIVE( CompressedTextureApp, RendererGl )