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

	mTextures.push_back( make_pair( "Original", gl::Texture::create( loadImage( loadAsset( "compression_test.png" ) ) ) ) );

#if ! defined( CINDER_GLES ) || defined( CINDER_GL_ANGLE )
	mTextures.push_back( make_pair( "DXT1", gl::Texture::createFromDds( loadAsset( "compression_test_dxt1.dds" ) ) ) );
	mTextures.push_back( make_pair( "DXT3", gl::Texture::createFromDds( loadAsset( "compression_test_dxt3.dds" ) ) ) );
	mTextures.push_back( make_pair( "DXT5", gl::Texture::createFromDds( loadAsset( "compression_test_dxt5.dds" ) ) ) );
#endif
	if( gl::isExtensionAvailable( "GL_ARB_texture_compression_bptc" ) )
		mTextures.push_back( make_pair( "BC7", gl::Texture::createFromDds( loadAsset( "compression_test_bc7.dds" ) ) ) );
	else
		console() << "This GL implementation doesn't support BC7 textures" << std::endl;
	
	if( gl::isExtensionAvailable( "GL_OES_compressed_ETC1_RGB8_texture" ) || gl::getVersion() >= make_pair(4,3) ) {
		mTextures.push_back( make_pair( "ETC1", gl::Texture::createFromKtx( loadAsset( "compression_test_etc1.ktx" ) ) ) );
	}
	else
		console() << "This GL implementation doesn't support ETC1 textures" << std::endl;
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

CINDER_APP_NATIVE( CompressedTextureApp, RendererGl( RendererGl::Options().version( 4, 3 ) ) )