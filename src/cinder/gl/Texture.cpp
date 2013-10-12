#include "cinder/gl/gl.h" // has to be first
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Context.h"
#include <stdio.h>

#if ! defined( CINDER_GLES )
#define GL_LUMINANCE GL_RED
#define GL_LUMINANCE_ALPHA GL_RG
#endif

using namespace std;

namespace cinder { namespace gl {

class ImageSourceTexture;
class ImageTargetTexture;

TextureDataExc::TextureDataExc( const std::string &log ) throw()
{ strncpy( mMessage, log.c_str(), 16000 ); }

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGLTexture
template<typename T>
class ImageTargetGLTexture : public ImageTarget {
public:
	static shared_ptr<ImageTargetGLTexture> createRef( const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha );
	~ImageTargetGLTexture();
	
	virtual bool	hasAlpha() const { return mHasAlpha; }
	virtual void*	getRowPointer( int32_t row ) { return mData + row * mRowInc; }
	
	const void*		getData() const { return mData; }
	
private:
	ImageTargetGLTexture( const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha );
	const Texture		*mTexture;
	bool				mIsGray;
	bool				mHasAlpha;
	uint8_t				mPixelInc;
	T					*mData;
	int					mRowInc;
};

/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
Texture::Format::Format()
{
	mTarget = GL_TEXTURE_2D;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
	mMipmapping = false;
	mInternalFormat = -1;
	mMaxAnisotropy = -1.0f;
    mDepthTexture = false;
    mRenderTexture = false;
    mPixelDataFormat = -1;
    mPixelDataType = -1;
}

/////////////////////////////////////////////////////////////////////////////////
// Texture
TextureRef Texture::create( int width, int height, Format format )
{
	return TextureRef( new Texture( width, height, format ) );
}

TextureRef Texture::create( const unsigned char *data, int dataFormat, int width, int height, Format format )
{
	return TextureRef( new Texture( data, dataFormat, width, height, format ) );
}

TextureRef Texture::create( const Surface8u &surface, Format format )
{
	return TextureRef( new Texture( surface, format ) );
}

TextureRef Texture::create( const Surface32f &surface, Format format )
{
	return TextureRef( new Texture( surface, format ) );
}

TextureRef Texture::create( const Channel8u &channel, Format format )
{
	return TextureRef( new Texture( channel, format ) );
}
	
TextureRef Texture::create( const Channel32f &channel, Format format )
{
	return TextureRef( new Texture( channel, format ) );
}
	
TextureRef Texture::create( ImageSourceRef imageSource, Format format )
{
	return TextureRef( new Texture( imageSource, format ) );
}
	
TextureRef Texture::create( GLenum target, GLuint textureID, int width, int height, bool doNotDispose )
{
	return TextureRef( new Texture( target, textureID, width, height, doNotDispose ) );
}
	
Texture::~Texture()
{
	if ( ( mTextureId > 0 ) && ( ! mDoNotDispose ) ) {
		auto ctx = gl::context();
		if( ctx ) {
			gl::context()->textureDeleted( mTarget, mTextureId );
			glDeleteTextures( 1, &mTextureId );
		}
	}
}

	

Texture::Texture( int width, int height, Format format )
	: mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat == -1 ) {
		format.mInternalFormat = GL_RGBA;
	}
	mInternalFormat = format.mInternalFormat;
	mTarget = format.mTarget;

    if ( format.mRenderTexture || format.mDepthTexture ) {
        init( format );
    }
    else {
        init( (unsigned char*)0, 0, GL_RGBA, GL_UNSIGNED_BYTE, format );
    }
}

Texture::Texture( const unsigned char *data, int dataFormat, int width, int height, Format format )
	: mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat == -1 ) {
		format.mInternalFormat = GL_RGBA;
	}
	mInternalFormat = format.mInternalFormat;
	mTarget = format.mTarget;
	init( data, 0, dataFormat, GL_UNSIGNED_BYTE, format );
}

Texture::Texture( const Surface8u &surface, Format format )
	: mWidth( surface.getWidth() ), mHeight( surface.getHeight() ),
	mCleanWidth( surface.getWidth() ), mCleanHeight( surface.getHeight() ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat < 0 ) {
		format.mInternalFormat = surface.hasAlpha() ? GL_RGBA : GL_RGB;
	}
	mInternalFormat = format.mInternalFormat;
	mTarget = format.mTarget;
	
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
	
	init( surface.getData(), surface.getRowBytes() / surface.getChannelOrder().getPixelInc(), dataFormat, type, format );
}

Texture::Texture( const Surface32f &surface, Format format )
	: mWidth( surface.getWidth() ), mHeight( surface.getHeight() ),
	mCleanWidth( surface.getWidth() ), mCleanHeight( surface.getHeight() ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat < 0 ) {
#if defined( CINDER_GLES )
		format.mInternalFormat = surface.hasAlpha() ? GL_RGBA : GL_RGB;
#else		
		format.mInternalFormat = surface.hasAlpha() ? GL_RGBA32F : GL_RGB32F;
#endif
	}
	mInternalFormat	= format.mInternalFormat;
	mTarget			= format.mTarget;
	
	init( surface.getData(), surface.hasAlpha()?GL_RGBA:GL_RGB, format );
}

Texture::Texture( const Channel8u &channel, Format format )
	: mWidth( channel.getWidth() ), mHeight( channel.getHeight() ),
	mCleanWidth( channel.getWidth() ), mCleanHeight( channel.getHeight() ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat < 0 ) {
		format.mInternalFormat = GL_LUMINANCE;
	}
	
	mInternalFormat = format.mInternalFormat;
	mTarget = format.mTarget;
	
	// if the data is not already contiguous, we'll need to create a block of memory that is
	if( ( channel.getIncrement() != 1 ) || ( channel.getRowBytes() != channel.getWidth() * sizeof( uint8_t ) ) ) {
		shared_ptr<uint8_t> data( new uint8_t[ channel.getWidth() * channel.getHeight() ], checked_array_deleter<uint8_t>() );
		uint8_t* dest		= data.get();
		const int8_t inc	= channel.getIncrement();
		const int32_t width = channel.getWidth();
		for ( int y = 0; y < channel.getHeight(); ++y ) {
			const uint8_t* src = channel.getData( 0, y );
			for ( int x = 0; x < width; ++x ) {
				*dest++	= *src;
				src		+= inc;
			}
		}
		init( data.get(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format );
	} else {
		init( channel.getData(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format );
	}
}

Texture::Texture( const Channel32f &channel, Format format )
	: mWidth( channel.getWidth() ), mHeight( channel.getHeight() ),
	mCleanWidth( channel.getWidth() ), mCleanHeight( channel.getHeight() ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	if( format.mInternalFormat < 0 ) {
		format.mInternalFormat = GL_LUMINANCE;
	}
	
	mInternalFormat = format.mInternalFormat;
	mTarget = format.mTarget;
	
	// if the data is not already contiguous, we'll need to create a block of memory that is
	if( ( channel.getIncrement() != 1 ) || ( channel.getRowBytes() != channel.getWidth() * sizeof(float) ) ) {
		shared_ptr<float> data( new float[channel.getWidth() * channel.getHeight()], checked_array_deleter<float>() );
		float* dest			= data.get();
		const int8_t inc	= channel.getIncrement();
		const int32_t width = channel.getWidth();
		for( int y = 0; y < channel.getHeight(); ++y ) {
			const float* src = channel.getData( 0, y );
			for( int x = 0; x < width; ++x ) {
				*dest++ = *src;
				src		+= inc;
			}
		}
		
		init( data.get(), GL_LUMINANCE, format );
	}
	else {
		init( channel.getData(), GL_LUMINANCE, format );
	}
}

Texture::Texture( ImageSourceRef imageSource, Format format )
	: mWidth( -1 ), mHeight( -1 ), mCleanWidth( -1 ), mCleanHeight( -1 ),
	mInternalFormat( -1 ), mTextureId( 0 ), mFlipped( false ), mDoNotDispose( false )
{
	init( imageSource, format );
}

Texture::Texture( GLenum target, GLuint textureId, int width, int height, bool doNotDispose )
	: mTarget( target ), mTextureId( textureId ),
	mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mInternalFormat( -1 ), mFlipped( false ), mDoNotDispose( doNotDispose )
{
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
}
    
void Texture::init( const Format &format )
{
    glGenTextures( 1, &mTextureId );
    
    TextureBindScope tbs( mTarget, mTextureId );
    
    if ( mTarget == GL_TEXTURE_2D ) {
        mMaxU = mMaxV = 1.0f;
    }
    else {
        mMaxU = (float)mWidth;
        mMaxV = (float)mHeight;
    }
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
    glTexImage2D( mTarget, 0, format.mInternalFormat, mWidth, mHeight, 0, format.mPixelDataFormat, format.mPixelDataType, NULL );
    
    if( format.mMipmapping )
		glGenerateMipmap( mTarget );
	
	if( format.mMaxAnisotropy > 1.0f )
		glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, format.mMaxAnisotropy );
}
	
void Texture::init( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format )
{
	glGenTextures( 1, &mTextureId );
	
	TextureBindScope tbs( mTarget, mTextureId );
	
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, type, data );
    
	if( format.mMipmapping ) 
		glGenerateMipmap( mTarget );
	
	if( format.mMaxAnisotropy > 1.0f )
		glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, format.mMaxAnisotropy );
	
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	
}

void Texture::init( const float *data, GLint dataFormat, const Format &format )
{
	glGenTextures( 1, &mTextureId );
	
	TextureBindScope tbs( mTarget, mTextureId );
	
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
	
	if( data ) {
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_FLOAT, data );
	}
	else {
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, GL_LUMINANCE, GL_FLOAT, 0 );  // init to black...
	}
    
    if( format.mMipmapping ) 
		glGenerateMipmap( mTarget );
	
	if( format.mMaxAnisotropy > 1.0f )
		glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, format.mMaxAnisotropy );
	
		
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	
}

void Texture::init( ImageSourceRef imageSource, const Format &format )
{
	mTarget = format.mTarget;
	mWidth = mCleanWidth = imageSource->getWidth();
	mHeight = mCleanHeight = imageSource->getHeight();
	
	// Set the internal format based on the image's color space
	if( format.isAutoInternalFormat() ) {
		switch( imageSource->getColorModel() ) {
			case ImageIo::CM_RGB:
				mInternalFormat = ( imageSource->hasAlpha() ) ? GL_RGBA : GL_RGB;
				break;
			case ImageIo::CM_GRAY:
				mInternalFormat = ( imageSource->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
				break;
			default:
				throw ImageIoExceptionIllegalColorModel();
				break;
		}
	}
	else {
		mInternalFormat = format.mInternalFormat;
	}
	
	// setup an appropriate dataFormat/ImageTargetTexture based on the image's color space
	GLint dataFormat;
	ImageIo::ChannelOrder channelOrder;
	bool isGray = false;
	switch( imageSource->getColorModel() ) {
		case ImageSource::CM_RGB:
			dataFormat = ( imageSource->hasAlpha() ) ? GL_RGBA : GL_RGB;
			channelOrder = ( imageSource->hasAlpha() ) ? ImageIo::RGBA : ImageIo::RGB;
			break;
		case ImageSource::CM_GRAY:
			dataFormat = ( imageSource->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
			channelOrder = ( imageSource->hasAlpha() ) ? ImageIo::YA : ImageIo::Y;
			isGray = true;
			break;
		default: // if this is some other color space, we'll have to punt and go w/ RGB
			dataFormat = ( imageSource->hasAlpha() ) ? GL_RGBA : GL_RGB;
			channelOrder = ( imageSource->hasAlpha() ) ? ImageIo::RGBA : ImageIo::RGB;
			break;
	}
	
	glGenTextures( 1, &mTextureId );
	TextureBindScope tbs( mTarget, mTextureId );
	
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	if( imageSource->getDataType() == ImageIo::UINT8 ) {
		shared_ptr<ImageTargetGLTexture<uint8_t> > target = ImageTargetGLTexture<uint8_t>::createRef( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_BYTE, target->getData() );
	}
	else if( imageSource->getDataType() == ImageIo::UINT16 ) {
		shared_ptr<ImageTargetGLTexture<uint16_t> > target = ImageTargetGLTexture<uint16_t>::createRef( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_SHORT, target->getData() );
		
	}
	else {
		shared_ptr<ImageTargetGLTexture<float> > target = ImageTargetGLTexture<float>::createRef( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_FLOAT, target->getData() );
	}
	
    if( format.mMipmapping )
		glGenerateMipmap( mTarget );
	
	if( format.mMaxAnisotropy > 1.0f )
		glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, format.mMaxAnisotropy );
		
	
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
}

void Texture::update( const Surface &surface, int mipLevel )
{
	GLint dataFormat;
	GLenum type;
	if( mipLevel == 0 ) {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) ) {
			throw TextureDataExc( "Invalid Texture::update() surface dimensions" );
		}
	
		TextureBindScope tbs( mTarget, mTextureId );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( mTarget, mipLevel, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, type, surface.getData() );
	}
	else {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		
		Vec2i mipMapSize = calcMipLevelSize( mipLevel, getWidth(), getHeight() );
		
		TextureBindScope tbs( mTarget, mTextureId );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( mTarget, mipLevel, getInternalFormat(), mipMapSize.x, mipMapSize.y, 0, dataFormat, type, surface.getData() );
	}
}

void Texture::update( const Surface32f &surface, int mipLevel )
{
	GLint dataFormat;
	GLenum type;
	if( mipLevel == 0 ) {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) ) {
			throw TextureDataExc( "Invalid Texture::update() surface dimensions" );
		}
		
		TextureBindScope tbs( mTarget, mTextureId );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		// @TODO: type does not seem to be pulling out the right value..
		glTexImage2D( mTarget, mipLevel, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, GL_FLOAT, surface.getData() );
	}
	else {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		
		Vec2i mipMapSize = calcMipLevelSize( mipLevel, getWidth(), getHeight() );
		
		TextureBindScope tbs( mTarget, mTextureId );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		// @TODO: type does not seem to be pulling out the right value..
		glTexImage2D( mTarget, mipLevel, getInternalFormat(), mipMapSize.x, mipMapSize.y, 0, dataFormat, GL_FLOAT, surface.getData() );
	}
}

void Texture::update( const Surface &surface, const Area &area, int mipLevel )
{
	GLint dataFormat;
	GLenum type;

	if( mipLevel == 0 ) {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		
		TextureBindScope tbs( mTarget, mTextureId );
		glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), dataFormat, type, surface.getData( area.getUL() ) );
	}
	else {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		
		Vec2i mipMapSize = calcMipLevelSize( mipLevel, area.getWidth(), area.getHeight() );
		
		TextureBindScope tbs( mTarget, mTextureId );
		glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), mipMapSize.x, mipMapSize.y, dataFormat, type, surface.getData( area.getUL() ) );
	}
}

void Texture::update( const Channel32f &channel, int mipLevel )
{

	if( mipLevel == 0 ) {
		if( ( channel.getWidth() != getWidth() ) || ( channel.getHeight() != getHeight() ) ) {
			throw TextureDataExc( "Invalid Texture::update() channel dimensions" );
		}
		
		TextureBindScope tbs( mTarget, mTextureId );
		glTexSubImage2D( mTarget, mipLevel, 0, 0, getWidth(), getHeight(), GL_LUMINANCE, GL_FLOAT, channel.getData() );
	}
	else {
		
		Vec2i mipMapSize = calcMipLevelSize( mipLevel, getWidth(), getHeight() );
		
		TextureBindScope tbs( mTarget, mTextureId );
		glTexSubImage2D( mTarget, mipLevel, 0, 0, mipMapSize.x, mipMapSize.y, GL_LUMINANCE, GL_FLOAT, channel.getData() );
	}
}

void Texture::update( const Channel8u &channel, const Area &area, int mipLevel )
{
	TextureBindScope tbs( mTarget, mTextureId );
	if( mipLevel == 0 ) {
		// if the data is not already contiguous, we'll need to create a block of memory that is
		if( ( channel.getIncrement() != 1 ) || ( channel.getRowBytes() != channel.getWidth() * sizeof(uint8_t) ) ) {
			shared_ptr<uint8_t> data( new uint8_t[area.getWidth() * area.getHeight()], checked_array_deleter<uint8_t>() );
			uint8_t* dest		= data.get();
			const int8_t inc	= channel.getIncrement();
			const int32_t width	= area.getWidth();
			for ( int y = 0; y < area.getHeight(); ++y ) {
				const uint8_t *src = channel.getData( area.getX1(), area.getY1() + y );
				for( int x = 0; x < width; ++x ) {
					*dest++	= *src;
					src		+= inc;
				}
			}
			
			glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, data.get() );
		}
		else {
			glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, channel.getData( area.getUL() ) );
		}
	}
	else {
		Vec2i mipMapSize = calcMipLevelSize( mipLevel, area.getWidth(), area.getHeight() );
		
		// if the data is not already contiguous, we'll need to create a block of memory that is
		if( ( channel.getIncrement() != 1 ) || ( channel.getRowBytes() != channel.getWidth() * sizeof(uint8_t) ) ) {
			shared_ptr<uint8_t> data( new uint8_t[area.getWidth() * area.getHeight()], checked_array_deleter<uint8_t>() );
			uint8_t* dest		= data.get();
			const int8_t inc	= channel.getIncrement();
			const int32_t width	= area.getWidth();
			for ( int y = 0; y < area.getHeight(); ++y ) {
				const uint8_t *src = channel.getData( area.getX1(), area.getY1() + y );
				for( int x = 0; x < width; ++x ) {
					*dest++	= *src;
					src		+= inc;
				}
			}
			
			glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), mipMapSize.x, mipMapSize.y, GL_LUMINANCE, GL_UNSIGNED_BYTE, data.get() );
		}
		else {
			glTexSubImage2D( mTarget, mipLevel, area.getX1(), area.getY1(), mipMapSize.x, mipMapSize.y, GL_LUMINANCE, GL_UNSIGNED_BYTE, channel.getData( area.getUL() ) );
		}
	}
}

Vec2i Texture::calcMipLevelSize( int mipLevel, GLint width, GLint height )
{
	width = max( 1, (int)floor( width >> mipLevel ) );
	height = max( 1, (int)floor( height >> mipLevel ) );
	
	return Vec2i( width, height );
}
	
int Texture::getNumMipLevels() const
{
	return floor( log( std::max( mWidth, mHeight ) ) / log(2) ) + 1;
}

void Texture::SurfaceChannelOrderToDataFormatAndType( const SurfaceChannelOrder &sco, GLint *dataFormat, GLenum *type )
{
	switch( sco.getCode() ) {
		case SurfaceChannelOrder::RGB:
			*dataFormat = GL_RGB;
			*type = GL_UNSIGNED_BYTE;
		break;
		case SurfaceChannelOrder::RGBA:
		case SurfaceChannelOrder::RGBX:
			*dataFormat = GL_RGBA;
			*type = GL_UNSIGNED_BYTE;
		break;
		case SurfaceChannelOrder::BGRA:
		case SurfaceChannelOrder::BGRX:
#if defined( CINDER_GLES )
			*dataFormat = GL_BGRA_EXT;
#else
			*dataFormat = GL_BGRA;
#endif
			*type = GL_UNSIGNED_BYTE;
		break;
		default:
			throw TextureDataExc( "Invalid channel order" ); // this is an unsupported channel order for a texture
		break;
	}
}

bool Texture::dataFormatHasAlpha( GLint dataFormat )
{
	switch( dataFormat ) {
		case GL_RGBA:
		case GL_ALPHA:
		case GL_LUMINANCE_ALPHA:
			return true;
		break;
		default:
			return false;
	}
}

bool Texture::dataFormatHasColor( GLint dataFormat )
{
	switch( dataFormat ) {
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			return false;
	}
	
	return true;
}

void Texture::setWrapS( GLenum wrapS )
{
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, wrapS );
}

void Texture::setWrapT( GLenum wrapT )
{
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, wrapT );
}

void Texture::setMinFilter( GLenum minFilter )
{
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, minFilter );
}

void Texture::setMagFilter( GLenum magFilter )
{
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, magFilter );
}
	
void Texture::setMaxAnisotropy( GLfloat maxAnisotropy )
{
	glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
}

GLfloat Texture::getMaxMaxAnisotropy()
{
	GLfloat maxMaxAnisotropy;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxMaxAnisotropy );
	return maxMaxAnisotropy;
}
	
void Texture::setCleanTexCoords( float maxU, float maxV )
{
	mMaxU = maxU;
	mMaxV = maxV;
	
	if ( mTarget == GL_TEXTURE_2D ) {
		mCleanWidth	= getWidth() * maxU;
		mCleanHeight	= getHeight() * maxV;
	} else {
		mCleanWidth	= (int32_t)maxU;
		mCleanHeight	= (int32_t)maxV;
	}
}

bool Texture::hasAlpha() const
{
	switch( mInternalFormat ) {
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
			return true;
		break;
		default:
			return false;
		break;
	}
}

float Texture::getLeft() const
{
	return 0.0f;
}

float Texture::getRight() const
{
	return mMaxU;
}

float Texture::getTop() const
{
	return ( mFlipped ) ? getMaxV() : 0.0f;
}

GLint Texture::getInternalFormat() const
{
	return mInternalFormat;
}

GLint Texture::getWidth() const
{
	return mWidth;
}

GLint Texture::getHeight() const
{
	return mHeight;
}

GLint Texture::getCleanWidth() const
{
	return mCleanWidth;
}

GLint Texture::getCleanHeight() const
{
	return mCleanHeight;
}

Rectf Texture::getAreaTexCoords( const Area &area ) const
{
	Rectf result;
	
	if ( mTarget == GL_TEXTURE_2D ) {
		result.x1 = area.x1 / (float)getWidth();
		result.x2 = area.x2 / (float)getWidth();
		result.y1 = area.y1 / (float)getHeight();
		result.y2 = area.y2 / (float)getHeight();
	} else {
		result = Rectf( area );
	}
	
	if ( mFlipped ) {
		std::swap( result.y1, result.y2 );
	}
	
	return result;
}

float Texture::getBottom() const
{
	return ( mFlipped ) ? 0.0f : getMaxV();
}

float Texture::getMaxU() const
{
	return mMaxU;
}

float Texture::getMaxV() const
{
	return mMaxV;
}

void Texture::bind( GLuint textureUnit ) const
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, mTextureId );
}

void Texture::unbind( GLuint textureUnit ) const
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, 0 );
}

void Texture::enableAndBind() const
{
	auto ctx = gl::context();
	ctx->enable( mTarget );
	gl::context()->bindTexture( mTarget, mTextureId );
}

void Texture::disable() const
{
	gl::context()->enable( mTarget, false );
}

// Returns the appropriate parameter to glGetIntegerv() for a specific target; ie GL_TEXTURE_2D -> GL_TEXTURE_BINDING_2D
GLenum Texture::getBindingConstantForTarget( GLenum target )
{
	switch( target ) {
		case GL_TEXTURE_2D:
			return GL_TEXTURE_BINDING_2D;
		break;
		case GL_TEXTURE_CUBE_MAP:
			return GL_TEXTURE_BINDING_CUBE_MAP;
		break;
#if ! defined( CINDER_GLES )
		case GL_TEXTURE_RECTANGLE: // equivalent to GL_TEXTURE_RECTANGLE_ARB
			return GL_TEXTURE_BINDING_RECTANGLE;
		break;
		case GL_TEXTURE_1D:
			return GL_TEXTURE_BINDING_1D;
		break;
		case GL_TEXTURE_3D:
			return GL_TEXTURE_BINDING_3D;
		break;
		case GL_TEXTURE_2D_ARRAY:
			return GL_TEXTURE_BINDING_2D_ARRAY;
		break;
		case GL_TEXTURE_1D_ARRAY:
			return GL_TEXTURE_BINDING_1D_ARRAY;
		break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
		break;			
		case GL_TEXTURE_BUFFER:
			return GL_TEXTURE_BINDING_BUFFER;
		break;			
		case GL_TEXTURE_2D_MULTISAMPLE:
			return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
		break;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
		break;
#endif
		default:
			return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// TextureCache

TextureCacheRef TextureCache::create()
{
	return TextureCacheRef( new TextureCache() );
}
	
TextureCacheRef TextureCache::create( const Surface8u &prototypeSurface, const Texture::Format &format )
{
	return TextureCacheRef( new TextureCache( prototypeSurface, format ) );
}
	
TextureCache::TextureCache()
{
}
	
TextureCache::TextureCache( const Surface8u &prototypeSurface, const Texture::Format &format )
	: mWidth( prototypeSurface.getWidth() ), mHeight( prototypeSurface.getHeight() ),
	mFormat( format ), mNextId( 0 )
{
}

gl::TextureRef TextureCache::cache( const Surface8u &data )
{
	pair<int,TextureRef> *resultPair;
	
	// find an available slot and update that if possible
	bool found = false;
	for( vector<pair<int,TextureRef> >::iterator texIt = mTextures.begin(); texIt != mTextures.end(); ++texIt ) {
		if( texIt->first == -1 ) { // this texture is available, let's use it!
			resultPair = &(*texIt);
			resultPair->second->update( data );
			found = true;
			break;
		}
	}
	
	// we didn't find an available slot, so let's make a new texture
	if( ! found ) {
		TextureRef tex( new Texture( data, mFormat ), textureCacheDeallocator );
		mTextures.push_back( make_pair( -1, tex ) );
		resultPair = &mTextures.back();
	}
	
	resultPair->first	= mNextId++;
	TextureRef result	= resultPair->second;
	
	return result;
}

void TextureCache::markTextureAsFree( int id )
{
	for ( vector<pair<int,TextureRef> >::iterator texIt = mTextures.begin(); texIt != mTextures.end(); ++texIt ) {
		if ( texIt->first == id ) { // this texture is available, let's use it!
			texIt->first = -1;
			break;
		}
	}
}

void TextureCache::textureCacheDeallocator( void *aDeallocatorRefcon )
{
	pair<TextureCache*,int> *refconPair = reinterpret_cast<pair<TextureCache*,int>* >( aDeallocatorRefcon );
	refconPair->first->markTextureAsFree( refconPair->second );
	delete refconPair;
}

/////////////////////////////////////////////////////////////////////////////////
// ImageSourceTexture
#if ! defined( CINDER_GLES )
class ImageSourceTexture : public ImageSource {
  public:
	ImageSourceTexture( const Texture &texture )
		: ImageSource()
	{
		mWidth = texture.getWidth();
		mHeight = texture.getHeight();
		
		GLint internalFormat = texture.getInternalFormat();
		GLenum format;
		switch( internalFormat ) {
			case GL_RGB: setChannelOrder( ImageIo::RGB ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); format = GL_RGB; break;
			case GL_RGBA: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); format = GL_RGBA; break;
			case GL_LUMINANCE: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE; break;
			case GL_LUMINANCE_ALPHA: setChannelOrder( ImageIo::YA ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE_ALPHA; break;
#if ! defined( CINDER_GLES )
			case GL_RGBA8: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::UINT8 ); format = GL_RGBA; break; 
			case GL_RGB8: setChannelOrder( ImageIo::RGB ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::UINT8 ); format = GL_RGB; break;
			case GL_BGR: setChannelOrder( ImageIo::RGB ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); format = GL_RGB; break;
			case 0x8040/*GL_LUMINANCE8 in Legacy*/: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::UINT8 ); format = GL_LUMINANCE; break;
			case 0x8045/*GL_LUMINANCE8_ALPHA8 in Legacy*/: setChannelOrder( ImageIo::YA ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::UINT8 ); format = GL_LUMINANCE_ALPHA; break; 
			case GL_DEPTH_COMPONENT16: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::UINT16 ); format = GL_DEPTH_COMPONENT; break;
			case GL_DEPTH_COMPONENT24: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_DEPTH_COMPONENT; break;
			case GL_DEPTH_COMPONENT32: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_DEPTH_COMPONENT; break;
			case GL_RGBA32F_ARB: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); format = GL_RGBA; break; 
			case GL_RGB32F_ARB: setChannelOrder( ImageIo::RGB ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); format = GL_RGB; break;
			case GL_LUMINANCE32F_ARB: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE; break;
			case GL_LUMINANCE_ALPHA32F_ARB: setChannelOrder( ImageIo::YA ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE_ALPHA; break;
#endif
			default: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); break;
		}

		GLenum dataType = GL_UNSIGNED_BYTE;
		int dataSize = 1;
		if( mDataType == ImageIo::UINT16 ) {
			dataType = GL_UNSIGNED_SHORT;
			dataSize = 2;
		}
		else if( mDataType == ImageIo::FLOAT32 ) {
			dataType = GL_FLOAT;
			dataSize = 4;
		}
			
		mRowBytes = mWidth * ImageIo::channelOrderNumChannels( mChannelOrder ) * dataSize;
		mData = shared_ptr<uint8_t>( new uint8_t[mRowBytes * mHeight], boost::checked_array_delete<uint8_t> );
		gl::TextureBindScope tbScope( texture.getTarget(), texture.getId() );
		glPixelStorei( GL_PACK_ALIGNMENT, 1 );
		glGetTexImage( texture.getTarget(), 0, format, dataType, mData.get() );
	}

	void load( ImageTargetRef target ) {
		// get a pointer to the ImageSource function appropriate for handling our data configuration
		ImageSource::RowFunc func = setupRowFunc( target );
		
		const uint8_t *data = mData.get();
		for( int32_t row = 0; row < mHeight; ++row ) {
			((*this).*func)( target, row, data );
			data += mRowBytes;
		}
	}
	
	shared_ptr<uint8_t>	mData;
	int32_t				mRowBytes;
};

Texture::operator ImageSourceRef() const
{
	return shared_ptr<ImageSource>( new ImageSourceTexture( *this ) );
}

#endif // ! defined( CINDER_GLES )

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGLTexture
template<typename T>
shared_ptr<ImageTargetGLTexture<T> > ImageTargetGLTexture<T>::createRef( const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha )
{
	return shared_ptr<ImageTargetGLTexture<T> >( new ImageTargetGLTexture<T>( aTexture, aChannelOrder, aIsGray, aHasAlpha ) );
}

template<typename T>
ImageTargetGLTexture<T>::ImageTargetGLTexture( const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha )
: ImageTarget(), mTexture( aTexture ), mIsGray( aIsGray ), mHasAlpha( aHasAlpha )
{
	if ( mIsGray ) {
		mPixelInc = mHasAlpha ? 2 : 1;
	} else {
		mPixelInc = mHasAlpha ? 4 : 3;
	}
	mRowInc = mTexture->getWidth() * mPixelInc;
	// allocate enough room to hold all these pixels
	mData = new T[mTexture->getHeight() * mRowInc];
	
	if ( boost::is_same<T,uint8_t>::value ) {
		setDataType( ImageIo::UINT8 );
	} else if ( boost::is_same<T,uint16_t>::value ) {
		setDataType( ImageIo::UINT16 );
	} else if ( boost::is_same<T,float>::value ) {
		setDataType( ImageIo::FLOAT32 );
	}
	
	setChannelOrder( aChannelOrder );
	setColorModel( mIsGray ? ImageIo::CM_GRAY : ImageIo::CM_RGB );
}

template<typename T>
ImageTargetGLTexture<T>::~ImageTargetGLTexture()
{
	delete [] mData;
}

} } // namespace cinder::gl
