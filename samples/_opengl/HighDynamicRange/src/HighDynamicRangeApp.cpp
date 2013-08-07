#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class HighDynamicRangeApp : public AppNative {
  public:
	void setup() override;
	void mouseDrag( MouseEvent event ) override;	
	void update() override;
	void draw() override;
	void fileDrop( FileDropEvent event ) override;

	float				mExposure;
	gl::TextureRef		mHdrTexture;
	gl::GlslProgRef		mShader;
};

void HighDynamicRangeApp::setup()
{
//glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
//glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
//glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);


	Surface32f s = loadImage( loadAsset( "Desk_oBA2_scaled.hdr" ) );
	for( int32_t y = 0; y < s.getHeight(); ++y ) {
		for( int32_t x = 0; x < s.getWidth(); ++x ) {
			ColorA c( y / (float)s.getHeight() * 10, x / (float)s.getWidth(), y / (float)s.getHeight() );
			s.setPixel( Vec2i( x, y ), c );
		}
	}
	
	gl::Texture::Format fmt;
	fmt.setInternalFormat( GL_RGB32F );
	mHdrTexture = gl::Texture::create( s, fmt );//gl::Texture::create( loadImage( loadAsset( "Desk_oBA2_scaled.hdr" ) ) );
			
	mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "shader.vert" ) )
																.fragment( loadAsset( "shader.frag" ) ) );
	mExposure = 1.0f;
	
}

void HighDynamicRangeApp::fileDrop( FileDropEvent event )
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat( GL_RGB32F );

	mHdrTexture = gl::Texture::create( loadImage( event.getFile( 0 ) ), fmt );
	mExposure = 1.0f;
	
	Surface32f s2( *mHdrTexture );
	for( int32_t y = 0; y < s2.getHeight(); ++y ) {
		for( int32_t x = 0; x < s2.getWidth(); ++x ) {
			ColorA c = s2.getPixel( Vec2i( x, y ) );
			if( c.g > 1.0f || c.b > 1.0f ) {
				std::cout << c;
				break;
			}
		}
	}	
}

void HighDynamicRangeApp::mouseDrag( MouseEvent event )
{
	mExposure = ( event.getPos().x / (float)getWindowWidth() ) * 50;
	console() << "Exposure: " << mExposure;
}

void HighDynamicRangeApp::update()
{
}

void HighDynamicRangeApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	gl::color( Color::white() );
	gl::ShaderScope shader( mShader );
	mShader->uniform( "uExposure", mExposure );
	gl::drawSolidRect( mHdrTexture->getBounds() );
//	gl::draw( mHdrTexture, Vec2f::zero() );
}

//CINDER_APP_NATIVE( HighDynamicRangeApp, RendererGl( RendererGl::Options().coreProfile(false) ) )
CINDER_APP_NATIVE( HighDynamicRangeApp, RendererGl( RendererGl::Options() ) )
