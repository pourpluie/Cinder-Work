#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/Fill.h"
#include "cinder/ip/Resize.h"
#include "cinder/Camera.h"
#include "cinder/Surface.h"

#include "Resources.h"

#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class TextureMipmappingApp : public AppNative {
public:
    void prepareSettings( Settings *settings ) { settings->enableMultiTouch( false ); }
	void setup();
	void update();
	void draw();
    void mouseUp( MouseEvent event );
    
    void renderFilterButtons();
    bool buttonContains( const Rectf &rect, const Vec2i &touch );
    void bindMinAndAnisoChange( const gl::TextureRef texture );
    
    void createGlGenMip( const gl::Texture::Format &mFormat );
    void createUserGenMip( const gl::Texture::Format &mFormat );
    void createUserResizedGenMip( const gl::Texture::Format &mFormat);
    
    gl::GlslProgRef         mShader;
    
    gl::TextureRef          mUserCreatedMipmap;
    gl::TextureRef          mGlGeneratedMipmap;
    gl::TextureRef          mUserResizedMipmap;
    Surface                 mCheckerBoard;
    int                     mTextureChoice;
    
    CameraPersp             mCam;
    Matrix44f               mPlaneRotation;
    Matrix44f               mPlaneTranslation;

    float                   mCamXLook;
    
    bool                    forward;
    
    // Button variables
    bool                    mMinFilterPushed;
    int                     mMinFilterChoice;
    Surface                 mMinFilterImage;
    gl::TextureRef          mMinTexture;
    Rectf                   mMinRect;
    
    bool                    mAnisoFilterPushed;
    float                   mAnisoFilterAmount;
    Surface                 mAnisoFilterImage;
    gl::TextureRef          mAnisoTexture;
    float                   mMaxAnisoFilterAmount;
    Rectf                   mAnisoRect;
    
};

void TextureMipmappingApp::setup()
{
    gl::bindStockShader( gl::ShaderDef().texture() );
    
#if ! defined( CINDER_GLES )
    mCheckerBoard = Surface( loadImage( loadResource( NON_POT_CHECKER ) ) );
#else
    mCheckerBoard = Surface( loadImage( loadResource( CHECKER_BOARD ) ) );
#endif
    mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_LIN ) ) );
    mAnisoTexture = gl::Texture::create( loadImage( loadResource( ANISOTROPIC ) ) );
    
    mAnisoFilterAmount = mMinFilterChoice = mTextureChoice = mCamXLook = 0;
    mAnisoFilterPushed = mMinFilterPushed = forward = false;
    
    // getting max Anisotropic maximum sampling available on the graphics card
    mMaxAnisoFilterAmount = gl::Texture::getMaxAnisotropicMax();
    
    mCam.setPerspective( 60, getWindowAspectRatio(), 1, 10000 );
    
    // Creating the 3 Texture formats
    gl::Texture::Format mFormat;
    mFormat.magFilter( GL_LINEAR ).minFilter( GL_LINEAR_MIPMAP_LINEAR )
        .anisotropicMax( 4.0f ).wrapT( GL_REPEAT ).wrapS( GL_REPEAT ).target( GL_TEXTURE_2D ).glMipMap();
    
    createGlGenMip( mFormat );
    
    //Turning off auto mipmap generation for the next two user generated mipmaps
    mFormat.enableMipmapping( false );
    
    createUserGenMip( mFormat );
    
    createUserResizedGenMip( mFormat );
    
    mPlaneTranslation.translate( Vec3f( - 10000 / 2, - getWindowHeight() / 2, 0 ) );
    mPlaneRotation.rotate( Vec3f( 1, 0, 0 ), toRadians( 85.0f ) );
    
    mMinRect = Rectf( ( getWindowWidth() / 4 ) - 50 , ( getWindowHeight() / 10 ) , ( getWindowWidth() / 4 ) + 50, ( getWindowHeight() / 10 ) + 30 );
    mAnisoRect = Rectf( ( getWindowWidth() / 4 ) * 3 - 50, ( getWindowHeight() / 10 ) , ( getWindowWidth() / 4 ) * 3 + 50, ( getWindowHeight() / 10 ) + 30 );
}

void TextureMipmappingApp::update()
{
    
    if( mCamXLook > 2 ) {
        forward = true;
    }
    else if ( mCamXLook < -2 ) {
        forward = false;
    }
    
    if( forward ) {
        mCamXLook -= 0.01f;
    }
    else {
        mCamXLook += 0.01f;
    }
    
    mCam.lookAt( Vec3f( 0, 0, -1 ), Vec3f( mCamXLook, 0, 1 ) );
}

void TextureMipmappingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    
    switch ( mTextureChoice ) {
        case 0:
            bindMinAndAnisoChange( mGlGeneratedMipmap );
            break;
        case 1:
            bindMinAndAnisoChange( mUserCreatedMipmap );
            break;
        case 2:
            bindMinAndAnisoChange( mUserResizedMipmap );
        default:
            break;
    }
    
    gl::pushModelView();
        gl::setMatrices( mCam );
        gl::multModelView( mPlaneTranslation );
        gl::multModelView( mPlaneRotation );
        gl::drawSolidRect( Rectf( 0, 0, 10000, 10000 ), Rectf( 0, 0, 50, 50 ) );
    gl::popModelView();
    
    renderFilterButtons();
}

void TextureMipmappingApp::mouseUp( MouseEvent event )
{
    int y = toPixels( event.getY() );
    int windowSection = toPixels( getWindowHeight() ) / 3 ;
    
    // CREATING A DIVISION IN THE SCREEN TO BIND DIFFERENT TEXTURES
    if ( buttonContains( mMinRect, event.getPos() ) ) {
        mMinFilterPushed = true;
        mMinFilterChoice++;
        if ( mMinFilterChoice > 3 )
            mMinFilterChoice = 0;
    }
    else if ( buttonContains( mAnisoRect, event.getPos() ) ) {
        mAnisoFilterAmount = ( (float) ( event.getPos().x - mAnisoRect.getUpperLeft().x ) / (float) 100 ) * mMaxAnisoFilterAmount;
        mAnisoFilterPushed = true;
    }
    else if ( y < windowSection ) {
        mTextureChoice = 0;
        mMinFilterPushed = true;
    }
    else if ( y < ( windowSection * 2 ) ) {
        mTextureChoice = 1;
        mMinFilterPushed = true;
    }
    else if ( y < ( windowSection * 3 ) ) {
        mTextureChoice = 2;
        mMinFilterPushed = true;
    }
    
}

void TextureMipmappingApp::renderFilterButtons()
{
    if ( mMinFilterPushed ) {
        switch ( mMinFilterChoice ) {
            case 0:
                mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_LIN ) ) );
                break;
            case 1:
                mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_NEA ) ) );
                break;
            case 2:
                mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_NEA_LIN ) ) );
                break;
            case 3:
                mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_NEA_NEA ) ) );
                break;
            default:
                break;
        }
        mMinFilterPushed = false;
    }
    
    gl::pushModelView();
        gl::setMatricesWindow( getWindowSize() );
    
        mMinTexture->bind();
            gl::drawSolidRect( mMinRect );
        mMinTexture->unbind();
    
        mAnisoTexture->bind();
            gl::drawSolidRect( mAnisoRect );
        mAnisoTexture->unbind();
    gl::popModelView();
}

bool TextureMipmappingApp::buttonContains( const Rectf &rect, const Vec2i &touch )
{
    return touch.x > rect.getUpperLeft().x && touch.x < rect.getLowerRight().x  && touch.y > rect.getUpperLeft().y && touch.y < rect.getLowerRight().y;
}

void TextureMipmappingApp::bindMinAndAnisoChange( const gl::TextureRef texture )
{
    texture->bind();
    
    if ( mMinFilterPushed ) {
        switch ( mMinFilterChoice ) {
            case 0:
                texture->setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
                break;
            case 1:
                texture->setMinFilter( GL_LINEAR_MIPMAP_NEAREST );
                break;
            case 2:
                texture->setMinFilter( GL_NEAREST_MIPMAP_LINEAR );
                break;
            case 3:
                texture->setMinFilter( GL_NEAREST_MIPMAP_NEAREST );
                break;
            default:
                break;
        }
    }
    if ( mAnisoFilterPushed ) {
        texture->setAnisotropicMax( mAnisoFilterAmount );
        mAnisoFilterPushed = false;
    }
}

void TextureMipmappingApp::createGlGenMip( const gl::Texture::Format &mFormat )
{
    // GL CREATED MIPMAP
    // mFormat holds the enable mipmapping flag
    mGlGeneratedMipmap = gl::Texture::create( mCheckerBoard, mFormat );
}

void TextureMipmappingApp::createUserGenMip( const gl::Texture::Format &mFormat )
{
    // USER GENERATED MIPMAPS WITH DIFFERENT COLORS TO SHOW THE MIPMAP GENERATION
    Surface* mSurface = new Surface( 512, 512, false );
    ip::fill( mSurface, Color( CM_HSV, 1, 1, 1 ) );
    mUserCreatedMipmap = gl::Texture::create( *mSurface, mFormat );
    int mipLevels = mUserCreatedMipmap->getNumMipLevels();
    
    for( int level = 1; level < mipLevels; ++level ) {
        float hue = static_cast<float>( level ) / static_cast<float>( mipLevels );
        ip::fill( mSurface, Color( CM_HSV, hue, 1, 1 ) );
        mUserCreatedMipmap->update( *mSurface, level );
    }
    delete mSurface;
}

void TextureMipmappingApp::createUserResizedGenMip( const gl::Texture::Format &mFormat)
{
    // USER RESIZED CHECKERBOARD MIPMAP
    mUserResizedMipmap = gl::Texture::create( mCheckerBoard, mFormat );
    int mipLevels = mUserResizedMipmap->getNumMipLevels();
    
    for( int level = 1; level < mipLevels; level++ ) {
        Vec2i mipSize = gl::Texture::calcMipLevelSize( level, mCheckerBoard.getWidth(), mCheckerBoard.getHeight() );
        mUserResizedMipmap->update( ip::resizeCopy( mCheckerBoard, Area( Vec2i::zero(), Vec2i( mCheckerBoard.getWidth(), mCheckerBoard.getHeight() ) ), mipSize ), level );
    }
}

CINDER_APP_NATIVE( TextureMipmappingApp, RendererGl )
