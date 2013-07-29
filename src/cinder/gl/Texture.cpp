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
		glDeleteTextures( 1, &mTextureId );
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
	init( (unsigned char*)0, 0, GL_RGBA, GL_UNSIGNED_BYTE, format );
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
		format.mInternalFormat = surface.hasAlpha() ? GL_RGBA : GL_RGB;
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
	if ( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
}

void Texture::init( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format )
{
	glGenTextures( 1, &mTextureId );
	
	glBindTexture( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	if( format.mMipmapping ) {
		glGenerateMipmap( mTarget );
	}
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, type, data );
}

void Texture::init( const float *data, GLint dataFormat, const Format &format )
{
	glGenTextures( 1, &mTextureId );
	
	glBindTexture( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	if( format.mMipmapping ) {
		glGenerateMipmap( mTarget );
	}
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	} else {
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
	glBindTexture( mTarget, mTextureId );
	
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	if( format.mMipmapping )
		glGenerateMipmap( mTarget );
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
}

void Texture::update( const Surface &surface )
{
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
	if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) ) {
		throw TextureDataExc( "Invalid Texture::update() surface dimensions" );
	}
	
	glBindTexture( mTarget, mTextureId );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glTexImage2D( mTarget, 0, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, type, surface.getData() );
}

void Texture::update( const Surface32f &surface )
{
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
	if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) ) {
		throw TextureDataExc( "Invalid Texture::update() surface dimensions" );
	}
	
	glBindTexture( mTarget, mTextureId );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	// @TODO: type does not seem to be pulling out the right value..
	glTexImage2D( mTarget, 0, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, GL_FLOAT, surface.getData() );
}

void Texture::update( const Surface &surface, const Area &area )
{
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
	
	glBindTexture( mTarget, mTextureId );
	glTexSubImage2D( mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), dataFormat, type, surface.getData( area.getUL() ) );
}

void Texture::update( const Channel32f &channel )
{
	if( ( channel.getWidth() != getWidth() ) || ( channel.getHeight() != getHeight() ) ) {
		throw TextureDataExc( "Invalid Texture::update() channel dimensions" );
	}
	
	glBindTexture( mTarget, mTextureId );
	glTexSubImage2D( mTarget, 0, 0, 0, getWidth(), getHeight(), GL_LUMINANCE, GL_FLOAT, channel.getData() );
}

void Texture::update( const Channel8u &channel, const Area &area )
{
	glBindTexture( mTarget, mTextureId );
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
		
		glTexSubImage2D( mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, data.get() );
	}
	else {
		glTexSubImage2D( mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, channel.getData( area.getUL() ) );
	}
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
			*dataFormat = GL_BGRA;
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

Texture	Texture::weakClone() const
{
	gl::Texture result = Texture( mTarget, mTextureId, mWidth, mHeight, true );
	result.mInternalFormat = mInternalFormat;
	result.mFlipped = mFlipped;
	return result;
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
	glBindTexture( mTarget, mTextureId );
}

void Texture::unbind( GLuint textureUnit ) const
{
	ActiveTextureScope activeTextureScope( textureUnit );
	glBindTexture( mTarget, 0 );
}

void Texture::enableAndBind() const
{
	auto ctx = gl::context();
	ctx->enable( mTarget );
	glBindTexture( mTarget, mTextureId );
}

void Texture::disable() const
{
	glDisable( mTarget );
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
