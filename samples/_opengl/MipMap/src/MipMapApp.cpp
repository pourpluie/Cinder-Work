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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This struct just acts as storage for the filter states and textures.
struct FilterControl {
public:
    
    FilterControl( const Rectf &minRect, const Rectf &textureRect, const Rectf &anisoRect, const Rectf &anisoLevelRect,
                  const Area &scissorArea, const gl::TextureRef minTexture, const gl::TextureRef textureTexture, const float maxAniso )
    :   mMinFilterPushed( false ), mMinFilterChoice( 0 ), mMinRect( minRect ),
        mAnisoFilterPushed( false ), mAnisoFilterMax( maxAniso ), mAnisoRect( anisoRect ),
        mTexturePushed( false ), mTextureChoice( 0 ), mTextureRect( textureRect ),
        mScissor( scissorArea ), mAnisoLevelRect( anisoLevelRect ),
        mMinTexture( minTexture ), mTextureTexture( textureTexture )
    {
    }
    
    void setLevelRect( int x ) { mAnisoLevelRect.x2 = x; };
    
    void setMinTexture( const gl::TextureRef minTexture ) { mMinTexture = minTexture; }
    void setTextureTexture( const gl::TextureRef textureTexture ) { mTextureTexture = textureTexture; }

    // Plane Textures
    gl::TextureRef  mUserCreatedMipmap;
    gl::TextureRef  mGlGeneratedMipmap;
    gl::TextureRef  mUserResizedMipmap;
    
    // Button variables
    bool            mMinFilterPushed;
    int             mMinFilterChoice;
    Rectf           mMinRect;
    gl::TextureRef  mMinTexture;
    
    bool            mAnisoFilterPushed;
    float           mAnisoFilterMax;
    Rectf           mAnisoRect;
    Rectf           mAnisoLevelRect;
    
    bool            mTexturePushed;
    int             mTextureChoice;
    Rectf           mTextureRect;
    gl::TextureRef  mTextureTexture;
    
    Area            mScissor;
};

//////////////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<FilterControl> FilterControlRef;

//////////////////////////////////////////////////////////////////////////////////////////////

class TextureMipmappingApp : public AppNative {
public:
    void    prepareSettings( Settings *settings ) { settings->enableMultiTouch( false ); }
	void    setup();
	void    update();
	void    draw();
    
    void    renderPlaneTexture( FilterControlRef f );
    void    renderFilterButtons( FilterControlRef f );
    void    bindMinAndAnisoChange( const gl::TextureRef texture, FilterControlRef f );
    
    void    mouseDrag( MouseEvent event );
    void    mouseUp( MouseEvent event );
    void    upContains( FilterControlRef f, const Vec2i &pos );
    bool    buttonContains( const Rectf &rect, const Vec2i &touch );
    
    void    createAnisoLevelTex();
    void    createGlGenMip( const gl::Texture::Format &mFormat, FilterControlRef f );
    void    createUserGenMip( const gl::Texture::Format &mFormat, FilterControlRef f );
    void    createUserResizedGenMip( const gl::Texture::Format &mFormat, FilterControlRef filter );
    
    Surface                 mCheckerBoard;
    
    CameraPersp             mCam;
    Matrix44f               mPlaneRotation;
    Matrix44f               mPlaneTranslation;
    
    gl::TextureRef          mAnisoTexture;
    gl::TextureRef          mAnisoLevelTexture;
    
    float                   mMaxAnisoFilterAmount;
    
    FilterControlRef        mLeftControl;
    FilterControlRef        mRightControl;
    
    bool                    right;
    float                   pan;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextureMipmappingApp::setup()
{
    gl::bindStockShader( gl::ShaderDef().texture() );
    
    // This if def is here to make sure whether you're using gl::Es2 or desktop gl.
    // It illustrates the fact that unfortunately Es2 does not support non-power-of-two
    // textures.
#if ! defined( CINDER_GLES )
    mCheckerBoard = Surface( loadImage( loadResource( NON_POT_CHECKER ) ) );
#else
    mCheckerBoard = Surface( loadImage( loadResource( CHECKER_BOARD ) ) );
#endif
    
    pan = 0.0f;
    right = false;
    
    mCam.setPerspective( 60, getWindowAspectRatio(), 1, 10000 );
    mCam.lookAt( Vec3f( 0, 0, -1 ), Vec3f( 0, 0, 1 ) );
    
    mPlaneTranslation.translate( Vec3f( - 10000 / 2, - getWindowHeight() / 2, 0 ) );
    mPlaneRotation.rotate( Vec3f( 1, 0, 0 ), toRadians( 85.0f ) );
    
    int widthFraction = getWindowWidth() / 6;
    int heightFraction = getWindowHeight() / 10;
    
    // getting max Anisotropic maximum sampling available on the graphics card above 1
    mMaxAnisoFilterAmount = gl::Texture::getMaxMaxAnisotropy() - 1.0f;
    
    mLeftControl = std::make_shared<FilterControl>( Rectf( widthFraction - 50, heightFraction * 1, widthFraction + 50, heightFraction * 1 + 30 ),
                                                   Rectf( widthFraction - 50, heightFraction * 2, widthFraction + 50, heightFraction * 2 + 30 ),
                                                   Rectf( widthFraction - 50, heightFraction * 3, widthFraction + 50, heightFraction * 3 + 30 ),
                                                   Rectf( widthFraction - 50, heightFraction * 3, widthFraction + 50, heightFraction * 3 + 30 ),
                                                   Area( 0, 0, getWindowWidth() / 2, getWindowHeight() ),
                                                   gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_LIN ) ) ),
                                                   gl::Texture::create( loadImage( loadResource( GL_GEN ) ) ),
                                                   mMaxAnisoFilterAmount );
    
    mRightControl = std::make_shared<FilterControl>( Rectf( widthFraction * 5 - 50, heightFraction * 1, widthFraction * 5 + 50, heightFraction * 1 + 30 ),
                                                    Rectf( widthFraction * 5 - 50, heightFraction * 2, widthFraction * 5 + 50, heightFraction * 2 + 30 ),
                                                    Rectf( widthFraction * 5 - 50, heightFraction * 3, widthFraction * 5 + 50, heightFraction * 3 + 30 ),
                                                    Rectf( widthFraction * 5 - 50, heightFraction * 3, widthFraction * 5 + 50, heightFraction * 3 + 30 ),
                                                    Area( getWindowWidth() / 2, 0, getWindowWidth(), getWindowHeight() ),
                                                    gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_LIN ) ) ),
                                                    gl::Texture::create( loadImage( loadResource( GL_GEN ) ) ),
                                                    mMaxAnisoFilterAmount );
    
    
    
    
    // Creating the 3 Texture formats
    gl::Texture::Format mFormat;
    mFormat.magFilter( GL_LINEAR )
        .minFilter( GL_LINEAR_MIPMAP_LINEAR )
        .maxAnisotropy( mMaxAnisoFilterAmount )
        .wrapT( GL_REPEAT ).wrapS( GL_REPEAT )
        .target( GL_TEXTURE_2D )
        .mipMap();
    
    mAnisoTexture = gl::Texture::create( loadImage( loadResource( ANISOTROPIC ) ) );
    createAnisoLevelTex();
    
    // This function creates a texture refs and allows gl to Generate the mipmaps
    // as you can see below this call, we reset the value of mipmap to be false
    createGlGenMip( mFormat, mLeftControl );
    createGlGenMip( mFormat, mRightControl );
    
    //Turning off auto mipmap generation for the next two user generated mipmaps
    mFormat.enableMipmapping( false );
    
    // This function creates a texture ref based upon a source that can be dynamically
    // created and it uses cinder tools like ip::fill and Surface to place data in the
    // texture. This is more for demonstration of what mipmapping is than anything else.
    createUserGenMip( mFormat, mLeftControl );
    createUserGenMip( mFormat, mRightControl );
    
    // This function creates a texture ref from an existing Surface, namely the one
    // we gave above for the glGeneratedMipmap, and uses ip::resize and user defined
    // filter to create the different mipmap levels
    createUserResizedGenMip( mFormat, mLeftControl );
    createUserResizedGenMip( mFormat, mRightControl );
}

void TextureMipmappingApp::update()
{
    
    if( pan > 2 ) {
        right = false;
    }
    else if( pan < -2 ) {
        right = true;
    }
    
    if( right ) {
        pan += 0.01f;
    }
    else {
        pan -= 0.01f;
    }
    
    mCam.lookAt( Vec3f( 0, 0, -1 ), Vec3f( pan, 0, 1 ) );
    
}

void TextureMipmappingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    gl::enableAlphaBlending();
    
    gl::pushModelView();
        gl::setMatrices( mCam );
        gl::multModelView( mPlaneTranslation );
        gl::multModelView( mPlaneRotation );
    
        renderPlaneTexture( mLeftControl );
        renderPlaneTexture( mRightControl );
    
    gl::popModelView();
    
    gl::pushModelView();
        gl::setMatricesWindow( getWindowSize() );
    
        renderFilterButtons( mLeftControl );
        renderFilterButtons( mRightControl );
    
    gl::popModelView();
}

void TextureMipmappingApp::renderPlaneTexture( FilterControlRef f )
{
    // This creates the scissor effect of showing both sides for contrast.
    // it takes an area with origin at lower left and width and height
    // like glViewport
    gl::ScissorScope myScissor( f->mScissor );
    
    switch( f->mTextureChoice ) {
        case 0:
            bindMinAndAnisoChange( f->mGlGeneratedMipmap, f );
        break;
        case 1:
            bindMinAndAnisoChange( f->mUserCreatedMipmap, f );
        break;
        case 2:
            bindMinAndAnisoChange( f->mUserResizedMipmap, f );
        break;
        default:
        break;
    }
    // Drawing this enormous so that we can see each mip level.
    gl::drawSolidRect( Rectf( 0, 0, 10000, 10000 ), Rectf( 0, 0, 50, 50 ) );
}

void TextureMipmappingApp::renderFilterButtons( FilterControlRef f )
{
    if( f->mMinFilterPushed ) {
        switch( f->mMinFilterChoice ) {
            case 0:
                f->mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_LIN ) ) );
            break;
            case 1:
                f->mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_LIN_NEA ) ) );
            break;
            case 2:
                f->mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_NEA_LIN ) ) );
            break;
            case 3:
                f->mMinTexture = gl::Texture::create( loadImage( loadResource( MIN_FILTER_NEA_NEA ) ) );
            break;
            default:
            break;
        }
        f->mMinFilterPushed = false;
    }
    if( f->mTexturePushed ) {
        switch( f->mTextureChoice ) {
            case 0:
                f->mTextureTexture = gl::Texture::create( loadImage( loadResource( GL_GEN ) ) );
            break;
            case 1:
                f->mTextureTexture = gl::Texture::create( loadImage( loadResource( USER_GEN ) ) );
            break;
            case 2:
                f->mTextureTexture = gl::Texture::create( loadImage( loadResource( USER_RESIZE ) ) );
            break;
            default:
            break;
        }
        f->mTexturePushed = false;
    }
    
    f->mMinTexture->bind();
        gl::drawSolidRect( f->mMinRect );
    f->mMinTexture->unbind();
    
    f->mTextureTexture->bind();
        gl::drawSolidRect( f->mTextureRect );
    f->mTextureTexture->unbind();
    
    mAnisoTexture->bind();
        gl::drawSolidRect( f->mAnisoRect );
    mAnisoTexture->unbind();
    
    mAnisoLevelTexture->bind();
        gl::drawSolidRect( f->mAnisoLevelRect );
    mAnisoLevelTexture->unbind();
    
}

void TextureMipmappingApp::bindMinAndAnisoChange( const gl::TextureRef texture, FilterControlRef f )
{
    texture->bind();
    
    if( f->mMinFilterPushed ) {
        switch( f->mMinFilterChoice ) {
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
    if( f->mAnisoFilterPushed ) {
        f->setLevelRect( f->mAnisoLevelRect.x1 + ( 100 * ( f->mAnisoFilterMax / mMaxAnisoFilterAmount ) ) );
        texture->setMaxAnisotropy( f->mAnisoFilterMax + 1.0f );
        f->mAnisoFilterPushed = false;
    }
}

void TextureMipmappingApp::mouseDrag( MouseEvent event )
{
    if( buttonContains( mLeftControl->mAnisoRect, event.getPos() ) ) {
        mLeftControl->mAnisoFilterMax = ( static_cast<float>( event.getPos().x - mLeftControl->mAnisoRect.getUpperLeft().x ) / 100 ) * mMaxAnisoFilterAmount;
        mLeftControl->mAnisoFilterPushed = true;
    }
    else if( buttonContains( mRightControl->mAnisoRect, event.getPos() ) ) {
        mRightControl->mAnisoFilterMax = ( static_cast<float>( event.getPos().x - mRightControl->mAnisoRect.getUpperLeft().x ) / 100 ) * mMaxAnisoFilterAmount;
        mRightControl->mAnisoFilterPushed = true;
    }
}

void TextureMipmappingApp::mouseUp( MouseEvent event )
{
    // CREATING A DIVISION IN THE SCREEN TO BIND DIFFERENT TEXTURES
    upContains( mLeftControl, event.getPos() );
    upContains( mRightControl, event.getPos() );
}

void TextureMipmappingApp::upContains( FilterControlRef f, const Vec2i &pos )
{
    if( buttonContains( f->mMinRect, pos ) ) {
        f->mMinFilterPushed = true;
        f->mMinFilterChoice++;
        if( f->mMinFilterChoice > 3 )
            f->mMinFilterChoice = 0;
    }
    else if( buttonContains( f->mAnisoRect, pos ) ) {
        f->mAnisoFilterMax = ( static_cast<float>( pos.x - f->mAnisoRect.getUpperLeft().x ) / 100 ) * mMaxAnisoFilterAmount;
        f->mAnisoFilterPushed = true;
    }
    else if( buttonContains( f->mTextureRect, pos ) ) {
        f->mMinFilterPushed = true;
        f->mTexturePushed = true;
        f->mAnisoFilterPushed = true;
        f->mTextureChoice++;
        if( f->mTextureChoice > 2)
            f->mTextureChoice = 0;
    }
}

bool TextureMipmappingApp::buttonContains( const Rectf &rect, const Vec2i &touch )
{
    return touch.x > rect.getUpperLeft().x && touch.x < rect.getLowerRight().x  && touch.y > rect.getUpperLeft().y && touch.y < rect.getLowerRight().y;
}

void TextureMipmappingApp::createAnisoLevelTex()
{
    Surface* mSurface = new Surface( 100, 30, true );
    ip::fill( mSurface, ColorA( 0.4f, 0.4f, 1.0f, 0.5f ) );
    mAnisoLevelTexture = gl::Texture::create( *mSurface );
    
    delete mSurface;
}

void TextureMipmappingApp::createGlGenMip( const gl::Texture::Format &mFormat, FilterControlRef f )
{
    // GL CREATED MIPMAP
    // mFormat holds the enable mipmapping flag
    f->mGlGeneratedMipmap = gl::Texture::create( mCheckerBoard, mFormat );
}

void TextureMipmappingApp::createUserGenMip( const gl::Texture::Format &mFormat, FilterControlRef f )
{
    // USER GENERATED MIPMAPS WITH DIFFERENT COLORS TO SHOW THE MIPMAP GENERATION
    Surface* mSurface = new Surface( 512, 512, false );
    ip::fill( mSurface, Color( CM_HSV, 1, 1, 1 ) );
    f->mUserCreatedMipmap = gl::Texture::create( *mSurface, mFormat );
    int mipLevels = f->mUserCreatedMipmap->getNumMipLevels();
    
    for( int level = 1; level < mipLevels; ++level ) {
        float hue = static_cast<float>( level ) / static_cast<float>( mipLevels );
        ip::fill( mSurface, Color( CM_HSV, hue, 1, 1 ) );
        f->mUserCreatedMipmap->update( *mSurface, level );
    }
    delete mSurface;
}

void TextureMipmappingApp::createUserResizedGenMip( const gl::Texture::Format &mFormat, FilterControlRef f )
{
    // USER RESIZED CHECKERBOARD MIPMAP
    f->mUserResizedMipmap = gl::Texture::create( mCheckerBoard, mFormat );
    int mipLevels = f->mUserResizedMipmap->getNumMipLevels();
    
    for( int level = 1; level < mipLevels; level++ ) {
        Vec2i mipSize = gl::Texture::calcMipLevelSize( level, mCheckerBoard.getWidth(), mCheckerBoard.getHeight() );
        f->mUserResizedMipmap->update( ip::resizeCopy( mCheckerBoard, Area( Vec2i::zero(), mCheckerBoard.getSize() ), mipSize, FilterSincBlackman() ), level );
    }
}

CINDER_APP_NATIVE( TextureMipmappingApp, RendererGl )
