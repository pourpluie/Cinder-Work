/*
 Copyright (c) 2010, The Cinder Project, All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/gl/gl.h" // has to be first
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Pbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Context.h"
#include "cinder/ip/Flip.h"
#include <stdio.h>
#include <algorithm>
#include <memory>
#include <type_traits>

#if defined( CINDER_GL_ANGLE )
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT	GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE
#endif

#if ! defined( CINDER_GLES )
#define GL_LUMINANCE						GL_RED
#define GL_LUMINANCE_ALPHA					GL_RG
#endif

using namespace std;

namespace cinder { namespace gl {

class ImageSourceTexture;
class ImageTargetTexture;

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGlTexture
template<typename T>
class ImageTargetGlTexture : public ImageTarget {
  public:
	static shared_ptr<ImageTargetGlTexture> create( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha );
#if ! defined( CINDER_GLES )
	// receives a pointer to an intermediate data store, presumably a mapped PBO
	static shared_ptr<ImageTargetGlTexture> create( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha, void *data );
#endif
	
	virtual bool	hasAlpha() const { return mHasAlpha; }
	virtual void*	getRowPointer( int32_t row ) { return mData + row * mRowInc; }
	
	const void*		getData() const { return mData; }
	
  private:
	ImageTargetGlTexture( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha, void *intermediateData );
	
	const Texture		*mTexture;
	bool				mHasAlpha;
	uint8_t				mPixelInc;
	T					*mData;
	unique_ptr<T>		mDataStore; // may be NULL
	int					mRowInc;
};

/////////////////////////////////////////////////////////////////////////////////
// TextureBase
TextureBase::TextureBase()
	: mTarget( 0 ), mTextureId( 0 ), mInternalFormat( -1 ), mDoNotDispose( false ), mMipmapping( false )
{
}

TextureBase::TextureBase( GLenum target, GLuint textureId, GLint internalFormat )
	: mTarget( target ), mTextureId( textureId ), mInternalFormat( internalFormat ), mDoNotDispose( false ), mMipmapping( false )
{
}

TextureBase::~TextureBase()
{
	if ( ( mTextureId > 0 ) && ( ! mDoNotDispose ) ) {
		auto ctx = gl::context();
		if( ctx ) {
			ctx->textureDeleted( mTarget, mTextureId );
			glDeleteTextures( 1, &mTextureId );
		}
	}
}

// Expects texture to be bound
void TextureBase::initParams( Format &format, GLint defaultInternalFormat )
{
	// default is GL_REPEAT
	if( format.mWrapS != GL_REPEAT )
		glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, format.mWrapS );
	// default is GL_REPEAT
	if( format.mWrapT != GL_REPEAT )
		glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, format.mWrapT );
#if ! defined( CINDER_GLES )
	// default is GL_REPEAT
	if( format.mWrapR != GL_REPEAT )
		glTexParameteri( mTarget, GL_TEXTURE_WRAP_R, format.mWrapR );
#endif // ! defined( CINDER_GLES )

	if( format.mMipmapping && ! format.mMinFilterSpecified )
		format.mMinFilter = GL_LINEAR_MIPMAP_LINEAR;
	// default is GL_NEAREST_MIPMAP_LINEAR
	if( format.mMinFilter != GL_NEAREST_MIPMAP_LINEAR )
		glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter );

	// default is GL_LINEAR
	if( format.mMagFilter != GL_LINEAR )
		glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter );
	
	if( format.mMaxAnisotropy > 1.0f )
		glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, format.mMaxAnisotropy );

	if( format.mInternalFormat == -1 )
		mInternalFormat = defaultInternalFormat;
	else
		mInternalFormat = format.mInternalFormat;

#if defined( CINDER_GLES )		
	// by default mPixelDataFormat should match mInternalFormat
	if( format.mPixelDataFormat == -1 )
		format.mPixelDataFormat = mInternalFormat;
#else
	if( format.mPixelDataFormat == -1 ) {
		format.mPixelDataFormat = GL_RGB;
	}
#endif

	// Swizzle mask
#if ! defined( CINDER_GLES2 )
	if( supportsHardwareSwizzle() ) {
		if( format.mSwizzleMask[0] != GL_RED || format.mSwizzleMask[1] != GL_GREEN || format.mSwizzleMask[2] != GL_BLUE || format.mSwizzleMask[3] != GL_ALPHA )
			glTexParameteriv( mTarget, GL_TEXTURE_SWIZZLE_RGBA, format.mSwizzleMask.data() );
	}
#endif
	mSwizzleMask = format.mSwizzleMask;
	
	mMipmapping = format.mMipmapping;
}

GLint TextureBase::getInternalFormat() const
{
	return mInternalFormat;
}

void TextureBase::bind( uint8_t textureUnit ) const
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, mTextureId );
}

void TextureBase::unbind( uint8_t textureUnit ) const
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, 0 );
}

// Returns the appropriate parameter to glGetIntegerv() for a specific target; ie GL_TEXTURE_2D -> GL_TEXTURE_BINDING_2D
GLenum TextureBase::getBindingConstantForTarget( GLenum target )
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

void TextureBase::setWrapS( GLenum wrapS )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_S, wrapS );
}

void TextureBase::setWrapT( GLenum wrapT )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_T, wrapT );
}

#if ! defined( CINDER_GLES )
void TextureBase::setWrapR( GLenum wrapR )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_WRAP_R, wrapR );
}
#endif

void TextureBase::setMinFilter( GLenum minFilter )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_MIN_FILTER, minFilter );
}

void TextureBase::setMagFilter( GLenum magFilter )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameteri( mTarget, GL_TEXTURE_MAG_FILTER, magFilter );
}
	
void TextureBase::setMaxAnisotropy( GLfloat maxAnisotropy )
{
	TextureBindScope tbs( mTarget, mTextureId );
	glTexParameterf( mTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
}

void TextureBase::SurfaceChannelOrderToDataFormatAndType( const SurfaceChannelOrder &sco, GLint *dataFormat, GLenum *type )
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

Vec2i TextureBase::calcMipLevelSize( int mipLevel, GLint width, GLint height )
{
	width = max( 1, (int)floor( width >> mipLevel ) );
	height = max( 1, (int)floor( height >> mipLevel ) );
	
	return Vec2i( width, height );
}
	
GLfloat TextureBase::getMaxMaxAnisotropy()
{
	GLfloat maxMaxAnisotropy;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxMaxAnisotropy );
	return maxMaxAnisotropy;
}

bool TextureBase::supportsHardwareSwizzle()
{
	#if defined( CINDER_GLES2 )
		return false;
	#else
		static bool supported = ( ( gl::isExtensionAvailable( "GL_EXT_texture_swizzle" ) || gl::getVersion() >= make_pair( 3, 3 ) ) );
		return supported;
	#endif
}

/////////////////////////////////////////////////////////////////////////////////
// Texture::Format
TextureBase::Format::Format()
{
	mTarget = GL_TEXTURE_2D;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mWrapR = GL_CLAMP_TO_EDGE;
	mMinFilterSpecified = false;
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
	mMipmapping = false;
	mMipmappingSpecified = false;
	mInternalFormat = -1;
	mMaxAnisotropy = -1.0f;
	mPixelDataFormat = -1;
	mPixelDataType = GL_UNSIGNED_BYTE;
	mSwizzleMask[0] = GL_RED; mSwizzleMask[1] = GL_GREEN; mSwizzleMask[2] = GL_BLUE; mSwizzleMask[3] = GL_ALPHA;	
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
	
Texture::Texture( int width, int height, Format format )
	: mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, GL_RGBA );
	initData( (unsigned char*)0, 0, format.mPixelDataFormat, format.mPixelDataType, format );
}

Texture::Texture( const unsigned char *data, int dataFormat, int width, int height, Format format )
	: mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, GL_RGBA );
	initData( data, 0, dataFormat, GL_UNSIGNED_BYTE, format );
}

Texture::Texture( const Surface8u &surface, Format format )
	: mWidth( surface.getWidth() ), mHeight( surface.getHeight() ),
	mCleanWidth( surface.getWidth() ), mCleanHeight( surface.getHeight() ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, surface.hasAlpha() ? GL_RGBA : GL_RGB );
	
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
	
	initData( surface.getData(), surface.getRowBytes() / surface.getChannelOrder().getPixelInc(), dataFormat, type, format );
}

Texture::Texture( const Surface32f &surface, Format format )
	: mWidth( surface.getWidth() ), mHeight( surface.getHeight() ),
	mCleanWidth( surface.getWidth() ), mCleanHeight( surface.getHeight() ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
#if defined( CINDER_GLES )
	initParams( format, surface.hasAlpha() ? GL_RGBA : GL_RGB );
#else
	initParams( format, surface.hasAlpha() ? GL_RGBA32F : GL_RGB32F );
#endif
	initData( surface.getData(), surface.hasAlpha()?GL_RGBA:GL_RGB, format );
}

Texture::Texture( const Channel8u &channel, Format format )
	: mWidth( channel.getWidth() ), mHeight( channel.getHeight() ),
	mCleanWidth( channel.getWidth() ), mCleanHeight( channel.getHeight() ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, GL_LUMINANCE );
	
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
		initData( data.get(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format );
	} else {
		initData( channel.getData(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format );
	}
}

Texture::Texture( const Channel32f &channel, Format format )
	: mWidth( channel.getWidth() ), mHeight( channel.getHeight() ),
	mCleanWidth( channel.getWidth() ), mCleanHeight( channel.getHeight() ),
	mFlipped( false )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, GL_LUMINANCE );
	
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
		
		initData( data.get(), GL_LUMINANCE, format );
	}
	else {
		initData( channel.getData(), GL_LUMINANCE, format );
	}
}

Texture::Texture( const ImageSourceRef &imageSource, Format format )
	: mWidth( -1 ), mHeight( -1 ), mCleanWidth( -1 ), mCleanHeight( -1 ),
	mFlipped( false )
{
	GLint defaultInternalFormat;	
	// Set the internal format based on the image's color space
	switch( imageSource->getColorModel() ) {
		case ImageIo::CM_RGB:
			defaultInternalFormat = ( imageSource->hasAlpha() ) ? GL_RGBA : GL_RGB;
		break;
		case ImageIo::CM_GRAY: {
#if defined( CINDER_GLES )
			defaultInternalFormat = ( imageSource->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
#else
			defaultInternalFormat = ( imageSource->hasAlpha() ) ?  GL_RG : GL_RED;
			std::array<int,4> swizzleMask = { GL_RED, GL_RED, GL_RED, GL_GREEN };
			if( defaultInternalFormat == GL_RED )
				swizzleMask[3] = GL_ONE;
			format.setSwizzleMask( swizzleMask );
#endif
		} break;
		default:
			throw ImageIoExceptionIllegalColorModel( "Unsupported color model for gl::Texture construction." );
		break;
	}

	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	initParams( format, defaultInternalFormat );
	initData( imageSource, format );
}

Texture::Texture( GLenum target, GLuint textureId, int width, int height, bool doNotDispose )
	: TextureBase( target, textureId, -1 ), mWidth( width ), mHeight( height ),
	mCleanWidth( width ), mCleanHeight( height ),
	mFlipped( false )
{
	mDoNotDispose = doNotDispose;
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
}
	
void Texture::initData( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format )
{
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
}

void Texture::initData( const float *data, GLint dataFormat, const Format &format )
{
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
}

#if ! defined( CINDER_GLES )
// Called by initData( ImageSourceRef ) when the user has supplied an intermediate PBO via Format
// We map the PBO after resizing it if necessary, and then use that as a data store for the ImageTargetGlTexture
void Texture::initDataImageSourceWithPboImpl( const ImageSourceRef &imageSource, const Format &format, GLint dataFormat, ImageIo::ChannelOrder channelOrder, bool isGray, const PboRef &pbo )
{
	auto ctx = gl::context();

	ctx->pushBufferBinding( GL_PIXEL_UNPACK_BUFFER, pbo->getId() );
	// resize the PBO if necessary
	if( pbo->getSize() < imageSource->getRowBytes() * imageSource->getHeight() )
		pbo->bufferData( imageSource->getRowBytes() * imageSource->getHeight(), nullptr, GL_STREAM_DRAW );
	void *pboData = pbo->map( GL_WRITE_ONLY );
	
	if( imageSource->getDataType() == ImageIo::UINT8 ) {
		auto target = ImageTargetGlTexture<uint8_t>::create( this, channelOrder, isGray, imageSource->hasAlpha(), pboData );
		imageSource->load( target );
		pbo->unmap();
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_BYTE, nullptr );
	}
	else if( imageSource->getDataType() == ImageIo::UINT16 ) {
		auto target = ImageTargetGlTexture<uint16_t>::create( this, channelOrder, isGray, imageSource->hasAlpha(), pboData );
		imageSource->load( target );
		pbo->unmap();
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_SHORT, nullptr );
		
	}
	else {
		auto target = ImageTargetGlTexture<float>::create( this, channelOrder, isGray, imageSource->hasAlpha(), pboData );
		imageSource->load( target );
		pbo->unmap();		
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_FLOAT, nullptr );
	}
	
	ctx->popBufferBinding( GL_PIXEL_UNPACK_BUFFER );
}
#endif

// Called by initData( ImageSourceRef ) when the user has NOT supplied an intermediate PBO
void Texture::initDataImageSourceImpl( const ImageSourceRef &imageSource, const Format &format, GLint dataFormat, ImageIo::ChannelOrder channelOrder, bool isGray )
{
	if( imageSource->getDataType() == ImageIo::UINT8 ) {
		auto target = ImageTargetGlTexture<uint8_t>::create( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_BYTE, target->getData() );
	}
	else if( imageSource->getDataType() == ImageIo::UINT16 ) {
		auto target = ImageTargetGlTexture<uint16_t>::create( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_UNSIGNED_SHORT, target->getData() );
		
	}
	else {
		auto target = ImageTargetGlTexture<float>::create( this, channelOrder, isGray, imageSource->hasAlpha() );
		imageSource->load( target );
		glTexImage2D( mTarget, 0, mInternalFormat, mWidth, mHeight, 0, dataFormat, GL_FLOAT, target->getData() );
	}
}

void Texture::initData( const ImageSourceRef &imageSource, const Format &format )
{
	mWidth = mCleanWidth = imageSource->getWidth();
	mHeight = mCleanHeight = imageSource->getHeight();
	
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
	
	TextureBindScope tbs( mTarget, mTextureId );	
	if( mTarget == GL_TEXTURE_2D ) {
		mMaxU = mMaxV = 1.0f;
	}
	else {
		mMaxU = (float)mWidth;
		mMaxV = (float)mHeight;
	}
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
#if ! defined( CINDER_GLES )
	auto pbo = format.getIntermediatePbo();
	if( pbo )
		initDataImageSourceWithPboImpl( imageSource, format, dataFormat, channelOrder, isGray, pbo );
	else {
		initDataImageSourceImpl( imageSource, format, dataFormat, channelOrder, isGray );
	}	
#else
	initDataImageSourceImpl( imageSource, format, dataFormat, channelOrder, isGray );
#endif	
    if( format.mMipmapping )
		glGenerateMipmap( mTarget );
}


void Texture::update( const Surface &surface, int mipLevel )
{
	GLint dataFormat;
	GLenum type;
	if( mipLevel == 0 ) {
		SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) )
			throw TextureResizeExc( "Invalid Texture::update() surface dimensions", surface.getSize(), getSize() );

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
		if( ( surface.getWidth() != getWidth() ) || ( surface.getHeight() != getHeight() ) )
			throw TextureResizeExc( "Invalid Texture::update() surface dimensions", surface.getSize(), getSize() );

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
		if( ( channel.getWidth() != getWidth() ) || ( channel.getHeight() != getHeight() ) )
			throw TextureResizeExc( "Invalid Texture::update() channel dimensions", channel.getSize(), getSize() );

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

#if ! defined( CINDER_GLES )
void Texture::update( const PboRef &pbo, GLenum format, GLenum type, int mipLevel, size_t pboByteOffset )
{
	update( pbo, format, type, getBounds(), mipLevel, pboByteOffset );
}

void Texture::update( const PboRef &pbo, GLenum format, GLenum type, const Area &destArea, int mipLevel, size_t pboByteOffset )
{
	// TODO: warn if PBO's target is wrong
	/*
	CI_ASSERT_ERROR( pbo->getTarget() == GL_PIXEL_UNPACK_BUFFER )
	*/
	
	BufferScope bufScp( (BufferObjRef)( pbo ) );
	TextureBindScope tbs( mTarget, mTextureId );
	glTexSubImage2D( mTarget, mipLevel, destArea.getX1(), mHeight - destArea.getY2(), destArea.getWidth(), destArea.getHeight(), format, type, reinterpret_cast<const GLvoid*>( pboByteOffset ) );
}
#endif

int Texture::getNumMipLevels() const
{
	return floor( log( std::max( mWidth, mHeight ) ) / log(2) ) + 1;
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
	// find an available slot and update that if possible
	for( vector<pair<int,TextureRef>>::iterator texIt = mTextures.begin(); texIt != mTextures.end(); ++texIt ) {
		if( texIt->first == -1 ) { // this texture is available, let's use it!
			texIt->second->update( data );
			texIt->first = mNextId++;
			// normally this would be very wrong, but when the result TextureRef is destroyed, it calls markTextureAsFree rather than deleting the master texture
			return TextureRef( texIt->second.get(), std::bind( &TextureCache::markTextureAsFree, this, texIt->first ) );
		}
	}
	
	// we didn't find an available slot, so let's make a new texture
	TextureRef masterTex( new Texture( data, mFormat ) );
	mTextures.push_back( make_pair( mNextId++, masterTex ) );
	// normally this would be very wrong, but when the result TextureRef is destroyed, it calls markTextureAsFree rather than deleting the master texture
	return TextureRef( mTextures.back().second.get(), std::bind( &TextureCache::markTextureAsFree, this, mTextures.back().first ) );
}

void TextureCache::markTextureAsFree( int id )
{
	for( vector<pair<int,TextureRef> >::iterator texIt = mTextures.begin(); texIt != mTextures.end(); ++texIt ) {
		if( texIt->first == id ) { // this texture is available now, let's mark it as usable
			texIt->first = -1;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
// ImageSourceTexture
class ImageSourceTexture : public ImageSource {
  public:
	ImageSourceTexture( Texture &texture )
		: ImageSource()
	{
		mWidth = texture.getWidth();
		mHeight = texture.getHeight();
		

		GLenum format;
#if ! defined( CINDER_GLES )		
		GLint internalFormat = texture.getInternalFormat();
		switch( internalFormat ) {
			case GL_RGB: setChannelOrder( ImageIo::RGB ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::UINT8 ); format = GL_RGB; break;
			case GL_RGBA: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::UINT8 ); format = GL_RGBA; break;
			case GL_LUMINANCE: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::UINT8 ); format = GL_LUMINANCE; break;
			case GL_LUMINANCE_ALPHA: setChannelOrder( ImageIo::YA ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::UINT8 ); format = GL_LUMINANCE_ALPHA; break;
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
			case GL_R32F: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_RED; break;
			case GL_LUMINANCE32F_ARB: setChannelOrder( ImageIo::Y ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE; break;
			case GL_LUMINANCE_ALPHA32F_ARB: setChannelOrder( ImageIo::YA ); setColorModel( ImageIo::CM_GRAY ); setDataType( ImageIo::FLOAT32 ); format = GL_LUMINANCE_ALPHA; break;
			default: setChannelOrder( ImageIo::RGBA ); setColorModel( ImageIo::CM_RGB ); setDataType( ImageIo::FLOAT32 ); break;
		}
#else
		// at least on iOS, non-RGBA appears to fail on glReadPixels, so we force RGBA
		setChannelOrder( ImageIo::RGBA );
		setColorModel( ImageIo::CM_RGB );
		setDataType( ImageIo::UINT8 );
		format = GL_RGBA;
#endif

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
		mData = unique_ptr<uint8_t>( new uint8_t[mRowBytes * mHeight] );

#if defined( CINDER_GLES )
		// This line is not too awesome, however we need a TextureRef, not a Texture, for an FBO attachment. So this creates a shared_ptr with a no-op deleter
		// that won't destroy our original texture.
		TextureRef tempSharedPtr( &texture, []( const Texture* ){} );
		// The theory here is we need to build an FBO, attach the Texture to it, issue a glReadPixels against it, and the put it away		
		FboRef fbo = Fbo::create( mWidth, mHeight, gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, tempSharedPtr ).disableDepth() );
		FramebufferScope fbScp( fbo );
		glReadPixels( 0, 0, mWidth, mHeight, format, dataType, mData.get() );
#else
		gl::TextureBindScope tbScope( texture.getTarget(), texture.getId() );
		glPixelStorei( GL_PACK_ALIGNMENT, 1 );
		glGetTexImage( texture.getTarget(), 0, format, dataType, mData.get() );
#endif
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
	
	std::unique_ptr<uint8_t>	mData;
	int32_t						mRowBytes;
};

ImageSourceRef Texture::createSource()
{
	return ImageSourceRef( new ImageSourceTexture( *this ) );
}

#if ! defined( CINDER_GLES )
/////////////////////////////////////////////////////////////////////////////////
// Texture3d
Texture3dRef Texture3d::create( GLint width, GLint height, GLint depth, Format format )
{
	return Texture3dRef( new Texture3d( width, height, depth, format ) );
}

Texture3dRef Texture3d::create( GLint width, GLint height, GLint depth, GLenum dataFormat, const uint8_t *data, Format format )
{
	return Texture3dRef( new Texture3d( width, height, depth, dataFormat, data, format ) );
}

Texture3d::Texture3d( GLint width, GLint height, GLint depth, Format format )
	: mWidth( width ), mHeight( height ), mDepth( depth )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	TextureBase::initParams( format, GL_RGB );

	TextureBindScope tbs( mTarget, mTextureId );
	glTexImage3D( mTarget, 0, mInternalFormat, mWidth, mHeight, mDepth, 0, format.mPixelDataFormat, format.mPixelDataType, NULL );
}

Texture3d::Texture3d( GLint width, GLint height, GLint depth, GLenum dataFormat, const uint8_t *data, Format format )
	: mWidth( width ), mHeight( height ), mDepth( depth )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );
	TextureBase::initParams( format, GL_RGB );

	glTexImage3D( mTarget, 0, mInternalFormat, mWidth, mHeight, mDepth, 0, dataFormat, GL_UNSIGNED_BYTE, data );
}

void Texture3d::update( const Surface &surface, int depth, int mipLevel )
{
	GLint dataFormat;
	GLenum type;
	SurfaceChannelOrderToDataFormatAndType( surface.getChannelOrder(), &dataFormat, &type );
		
	Vec2i mipMapSize = calcMipLevelSize( mipLevel, getWidth(), getHeight() );
	if( surface.getSize() != mipMapSize )
		throw TextureResizeExc( "Invalid Texture::update() surface dimensions", surface.getSize(), mipMapSize );

	TextureBindScope tbs( mTarget, mTextureId );
	glTexSubImage3D( mTarget, mipLevel,
		0, 0, depth, // offsets
		mipMapSize.x, mipMapSize.y, 1, dataFormat, type, surface.getData() );
}

#endif // ! defined( CINDER_GLES )

/////////////////////////////////////////////////////////////////////////////////
// TextureCubeMap
TextureCubeMap::Format::Format()
	: TextureBase::Format()
{
	mTarget = GL_TEXTURE_CUBE_MAP;
	mMinFilter = GL_NEAREST;
	mMagFilter = GL_NEAREST;	
}

TextureCubeMapRef TextureCubeMap::create( int32_t width, int32_t height, const Format &format )
{
	return TextureCubeMapRef( new TextureCubeMap( width, height, format ) );
}

TextureCubeMapRef TextureCubeMap::createHorizontalCross( const ImageSourceRef &imageSource, const Format &format )
{
	Vec2i faceSize( imageSource->getWidth() / 4, imageSource->getHeight() / 3 );
	Area faceArea( 0, 0, faceSize.x, faceSize.y );
	
	Surface8u masterSurface( imageSource, SurfaceConstraintsDefault() );
	
	// allocate the individual face's Surfaces, ensuring rowbytes == faceSize.x * 3 through default SurfaceConstraints
	Surface8u images[6];
	for( uint8_t f = 0; f < 6; ++f )
		images[f] = Surface8u( faceSize.x, faceSize.y, masterSurface.hasAlpha(), SurfaceConstraints() );

	// copy out each individual face
	// GL_TEXTURE_CUBE_MAP_POSITIVE_X
	images[0].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 2, faceSize.y * 1 ), -Vec2i( faceSize.x * 2, faceSize.y * 1 ) );
	// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
	images[1].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 0, faceSize.y * 1 ), -Vec2i( faceSize.x * 0, faceSize.y * 1 ) );
	// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
	images[2].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 1, faceSize.y * 0 ), -Vec2i( faceSize.x * 1, faceSize.y * 0 ) );
	// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	images[3].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 1, faceSize.y * 2 ), -Vec2i( faceSize.x * 1, faceSize.y * 2 ) );
	// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
	images[4].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 1, faceSize.y * 1 ), -Vec2i( faceSize.x * 1, faceSize.y * 1 ) );
	// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	images[5].copyFrom( masterSurface, faceArea + Vec2i( faceSize.x * 3, faceSize.y * 1 ), -Vec2i( faceSize.x * 3, faceSize.y * 1 ) );
		
	return TextureCubeMapRef( new TextureCubeMap( images, format ) );
}

TextureCubeMapRef TextureCubeMap::create( const ImageSourceRef images[6], const Format &format )
{
	Surface8u surfaces[6];
	for( size_t i = 0; i < 6; ++i )
		surfaces[i] = Surface8u( images[i] );
	
	return TextureCubeMapRef( new TextureCubeMap( surfaces, format ) );
}

TextureCubeMap::TextureCubeMap( int32_t width, int32_t height, Format format )
	: mWidth( width ), mHeight( height )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );	
	TextureBase::initParams( format, GL_RGB );

	for( GLenum target = 0; target < 6; ++target )
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + target, 0, mInternalFormat, mWidth, mHeight, 0, format.mPixelDataFormat, format.mPixelDataType, NULL );
}

TextureCubeMap::TextureCubeMap( const Surface8u images[6], Format format )
{
	glGenTextures( 1, &mTextureId );
	mTarget = format.getTarget();
	TextureBindScope texBindScope( mTarget, mTextureId );	
	TextureBase::initParams( format, ( images[0].hasAlpha() ) ? GL_RGBA : GL_RGB );

	mWidth = images[0].getWidth();
	mHeight = images[0].getHeight();

	for( GLenum target = 0; target < 6; ++target )
		glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + target, 0, mInternalFormat, images[target].getWidth(), images[target].getHeight(), 0,
			( images[target].hasAlpha() ) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, images[target].getData() );
			
	if( format.mMipmapping ) 
		glGenerateMipmap( mTarget );			
}

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGlTexture
template<typename T>
shared_ptr<ImageTargetGlTexture<T>> ImageTargetGlTexture<T>::create( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha )
{
	return shared_ptr<ImageTargetGlTexture<T>>( new ImageTargetGlTexture<T>( texture, channelOrder, isGray, hasAlpha, nullptr ) );
}

#if ! defined( CINDER_GLES )
// create method receives an existing pointer which presumably is a mapped PBO
template<typename T>
shared_ptr<ImageTargetGlTexture<T>> ImageTargetGlTexture<T>::create( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore )
{
	return shared_ptr<ImageTargetGlTexture<T>>( new ImageTargetGlTexture<T>( texture, channelOrder, isGray, hasAlpha, intermediateDataStore ) );
}
#endif

template<typename T>
ImageTargetGlTexture<T>::ImageTargetGlTexture( const Texture *texture, ImageIo::ChannelOrder &channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore )
	: ImageTarget(), mTexture( texture ), mHasAlpha( hasAlpha )
{
	if( isGray ) {
		mPixelInc = mHasAlpha ? 2 : 1;
	}
	else {
		mPixelInc = mHasAlpha ? 4 : 3;
	}
	mRowInc = mTexture->getWidth() * mPixelInc;
	
	// allocate enough room to hold all these pixels if we haven't been passed a data*
	if( ! intermediateDataStore ) {
		mDataStore = std::unique_ptr<T>( new T[mTexture->getHeight() * mRowInc] );
		mData = mDataStore.get();
	}
	else
		mData = reinterpret_cast<T*>( intermediateDataStore );
	
	if( std::is_same<T,uint8_t>::value ) {
		setDataType( ImageIo::UINT8 );
	}
	else if( std::is_same<T,uint16_t>::value ) {
		setDataType( ImageIo::UINT16 );
	}
	else if( std::is_same<T,float>::value ) {
		setDataType( ImageIo::FLOAT32 );
	}
	
	setChannelOrder( channelOrder );
	setColorModel( isGray ? ImageIo::CM_GRAY : ImageIo::CM_RGB );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureData
#if defined( CINDER_GLES )
TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}
#else
TextureData::TextureData( const PboRef &pbo )
	: mPbo( pbo )
{
	if( mPbo ) {
		gl::context()->pushBufferBinding( GL_PIXEL_UNPACK_BUFFER, pbo->getId() );
		mPboMappedPtr = nullptr;
	}
}

TextureData::~TextureData()
{
	if( mPbo )
		gl::context()->popBufferBinding( GL_PIXEL_UNPACK_BUFFER );
}
#endif

void TextureData::allocateDataStore( size_t requireBytes )
{
	if( mPbo ) {
		if( mPbo->getSize() < requireBytes )
			mPbo->bufferData( requireBytes, nullptr, GL_STREAM_DRAW );
	}
	else {
		mDataStoreMem = shared_ptr<uint8_t>( new uint8_t[requireBytes] );
	}

	mDataStoreSize = requireBytes;
}

void TextureData::mapDataStore()
{
	if( mPbo )
		mPboMappedPtr = mPbo->map( GL_WRITE_ONLY );
}

void TextureData::unmapDataStore()
{
	if( mPbo )
		mPbo->unmap();
	mPboMappedPtr = nullptr;
}

void* TextureData::getDataStorePtr( size_t offset )
{
	if( mPbo ) {
		return ((uint8_t*)mPboMappedPtr) + offset;
	}
	else {
		return mDataStoreMem.get() + offset;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseKtx
namespace {
void parseKtx( const DataSourceRef &dataSource, uint32_t *resultWidth, uint32_t *resultHeight,
	uint32_t *resultDepth, uint32_t *resultInternalFormat, uint32_t *resultDataFormat, uint32_t *resultDataType, uint32_t *resultMipmapLevels, TextureData *resultData )
{
	typedef struct {
		uint8_t		identifier[12];
		uint32_t	endianness;
		uint32_t	glType;
		uint32_t	glTypeSize;
		uint32_t	glFormat;
		uint32_t	glInternalFormat;
		uint32_t	glBaseInternalFormat;
		uint32_t	pixelWidth;
		uint32_t	pixelHeight;
		uint32_t	pixelDepth;
		uint32_t	numberOfArrayElements;
		uint32_t	numberOfFaces;
		uint32_t	numberOfMipmapLevels;
		uint32_t	bytesOfKeyValueData;
	} KtxHeader;

	static const uint8_t FileIdentifier[12] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
	};
	
	KtxHeader header;
	auto ktxStream = dataSource->createStream();
	ktxStream->readData( &header, sizeof(header) );
	
	if( memcmp( header.identifier, FileIdentifier, sizeof(FileIdentifier) ) )
		throw KtxParseExc( "File identifier mismatch" );			
	
	if( header.endianness != 0x04030201 )
		throw KtxParseExc( "Only little endian currently supported" );
	
	if( header.numberOfArrayElements != 0 )
		throw KtxParseExc( "Array textures not currently supported" );

	if( header.numberOfFaces != 1 )
		throw KtxParseExc( "Cube maps not currently supported" );

	if( header.pixelDepth != 0 )
		throw KtxParseExc( "3D textures not currently supported" );
	
	*resultWidth = header.pixelWidth;
	*resultHeight = header.pixelHeight;
	*resultDepth = header.pixelDepth;
	*resultInternalFormat = header.glInternalFormat;
	*resultDataFormat = header.glFormat;
	*resultDataType = header.glType;
	*resultMipmapLevels = header.numberOfMipmapLevels;
	
	ktxStream->seekRelative( header.bytesOfKeyValueData );

	// clear output containers
	resultData->clear();
	size_t byteOffset = 0;
	for( int level = 0; level < std::max<int>( 1, header.numberOfMipmapLevels ); ++level ) {
		
		uint32_t imageSize;
		ktxStream->readData( &imageSize, sizeof(imageSize) );
		if( level == 0 ) { // if this is our first level, we need to allocate storage. If mipmapping is on we need double the memory required for the first level
			if( header.numberOfMipmapLevels > 1 )
				resultData->allocateDataStore( imageSize * 2 );
			else
				resultData->allocateDataStore( imageSize );
			resultData->mapDataStore();
		}
		// currently always 0 -> 1
		for( int arrayElement = 0; arrayElement < std::max<int>( 1, header.numberOfArrayElements ); ++arrayElement ) {
			for( int face = 0; face < header.numberOfFaces; ++face ) { // currently always 1
				for( int zSlice = 0; zSlice < header.pixelDepth + 1; ++zSlice ) { // curently always 0 -> 1
					resultData->push_back( TextureData::Level() );
					resultData->back().dataSize = imageSize;
					resultData->back().offset = byteOffset;
					if( byteOffset + imageSize > resultData->getDataStoreSize() )
						throw TextureDataStoreTooSmallExc();
					ktxStream->readData( resultData->getDataStorePtr( byteOffset ), imageSize );
					resultData->back().width = std::max<int>( 1, header.pixelWidth >> level );
					resultData->back().height = std::max<int>( 1, header.pixelHeight >> level );
					resultData->back().depth = zSlice;
					byteOffset += imageSize;
				}
				ktxStream->seekRelative( 3 - (ktxStream->tell() + 3) % 4 );
			}
		}
		ktxStream->seekRelative( 3 - (imageSize + 3) % 4 );
	}

	resultData->unmapDataStore();
}
} // anonymous namespace

TextureRef Texture::createFromKtx( const DataSourceRef &dataSource, Format format )
{
	uint32_t width, height, depth, mipmapLevels, internalFormat, dataFormat, dataType;
#if ! defined( CINDER_GLES )
	TextureData textureData( format.getIntermediatePbo() );
#else
	TextureData textureData;
#endif

	parseKtx( dataSource, &width, &height, &depth, &internalFormat, &dataFormat, &dataType, &mipmapLevels, &textureData );

	GLenum target = format.mTarget;
	GLuint texId;
	glGenTextures( 1, &texId );

	TextureRef result = Texture::create( target, texId, width, height, false );
	result->mWidth = width;
	result->mHeight = height;
	result->mInternalFormat = dataFormat;
	if( mipmapLevels > 1 && ( format.mMipmapping || ( ! format.mMipmappingSpecified ) ) )
		format.mMipmapping = true;

	TextureBindScope bindScope( result );
	result->initParams( format, dataFormat /*ignored*/ );

	glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
	int curLevel = 0;
	for( const auto &textureDataLevel : textureData.getLevels() ) {		
		if( dataType != 0 )
			glTexImage2D( result->mTarget, curLevel, internalFormat, textureDataLevel.width, textureDataLevel.height, 0, dataFormat, dataType, textureData.getDataStorePtr( textureDataLevel.offset ) );
		else
			glCompressedTexImage2D( result->mTarget, curLevel, internalFormat, textureDataLevel.width, textureDataLevel.height, 0, textureDataLevel.dataSize, textureData.getDataStorePtr( textureDataLevel.offset ) );
		++curLevel;
	}
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	return result;
}

#if defined( CINDER_GLES )
void Texture::updateFromKtx( const DataSourceRef &dataSource )
#else
void Texture::updateFromKtx( const DataSourceRef &dataSource, const PboRef &intermediatePbo )
#endif
{
	uint32_t width, height, depth, mipmapLevels, internalFormat, dataFormat, dataType;
#if defined( CINDER_GLES )
	TextureData textureData;
#else
	TextureData textureData( intermediatePbo );
#endif

	parseKtx( dataSource, &width, &height, &depth, &internalFormat, &dataFormat, &dataType, &mipmapLevels, &textureData );
	
	TextureBindScope bindScope( mTarget, mTextureId );
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
	int curLevel = 0;
	for( const auto &textureDataLevel : textureData.getLevels() ) {		
		if( dataType != 0 )
			glTexSubImage2D( mTarget, curLevel, 0, 0, textureDataLevel.width, textureDataLevel.height, dataFormat, dataType, textureData.getDataStorePtr( textureDataLevel.offset ) );
		else
			glCompressedTexSubImage2D( mTarget, curLevel, 0, 0, textureDataLevel.width, textureDataLevel.height, internalFormat, textureDataLevel.dataSize, textureData.getDataStorePtr( textureDataLevel.offset ) );
		++curLevel;
	}
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
}

#if ! defined( CINDER_GLES ) || defined( CINDER_GL_ANGLE )
TextureRef Texture::createFromDds( const DataSourceRef &dataSource, Format format )
{
	typedef struct { // DDCOLORKEY
		uint32_t dw1;
		uint32_t dw2;
	} ddColorKey;

	typedef struct  { // DDSCAPS2
		uint32_t dwCaps1;
		uint32_t dwCaps2;
		uint32_t Reserved[2];
	} ddCaps2;

	typedef struct { // DDPIXELFORMAT
		uint32_t  dwSize;
		uint32_t  dwFlags;
		uint32_t  dwFourCC;
		union {
			uint32_t  dwRGBBitCount;
			uint32_t  dwYUVBitCount;
			uint32_t  dwZBufferBitDepth;
			uint32_t  dwAlphaBitDepth;
			uint32_t  dwLuminanceBitCount;
			uint32_t  dwBumpBitCount;
			uint32_t  dwPrivateFormatBitCount;
		} ;
		union {
			uint32_t  dwRBitMask;
			uint32_t  dwYBitMask;
			uint32_t  dwStencilBitDepth;
			uint32_t  dwLuminanceBitMask;
			uint32_t  dwBumpDuBitMask;
			uint32_t  dwOperations;
		} ;
		union {
			uint32_t  dwGBitMask;
			uint32_t  dwUBitMask;
			uint32_t  dwZBitMask;
			uint32_t  dwBumpDvBitMask;
			struct {
				int32_t wFlipMSTypes;
				int32_t wBltMSTypes;
			} MultiSampleCaps;
		};
		union {
			uint32_t  dwBBitMask;
			uint32_t  dwVBitMask;
			uint32_t  dwStencilBitMask;
			uint32_t  dwBumpLuminanceBitMask;
		};
		union {
			uint32_t  dwRGBAlphaBitMask;
			uint32_t  dwYUVAlphaBitMask;
			uint32_t  dwLuminanceAlphaBitMask;
			uint32_t  dwRGBZBitMask;
			uint32_t  dwYUVZBitMask;
		} ;
	} DdPixelFormat;

	typedef struct DdSurface // this is lifted and adapted from DDSURFACEDESC2
	{
		uint32_t               dwSize;                 // size of the DDSURFACEDESC structure
		uint32_t               dwFlags;                // determines what fields are valid
		uint32_t               dwHeight;               // height of surface to be created
		uint32_t               dwWidth;                // width of input surface
		union
		{
			int32_t            lPitch;                 // distance to start of next line (return value only)
			uint32_t           dwLinearSize;           // Formless late-allocated optimized surface size
		};
		union
		{
			uint32_t           dwBackBufferCount;      // number of back buffers requested
			uint32_t           dwDepth;                // the depth if this is a volume texture 
		};
		union
		{
			uint32_t			dwMipMapCount;          // number of mip-map levels requestde
													// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
			uint32_t			dwRefreshRate;          // refresh rate (used when display mode is described)
			uint32_t			dwSrcVBHandle;          // The source used in VB::Optimize
		};
		uint32_t				dwAlphaBitDepth;        // depth of alpha buffer requested
		uint32_t				dwReserved;             // reserved
		uint32_t				lpSurface;              // pointer to the associated surface memory
		union
		{
			ddColorKey			ddckCKDestOverlay;      // color key for destination overlay use
			uint32_t			dwEmptyFaceColor;       // Physical color for empty cubemap faces
		};
		ddColorKey          ddckCKDestBlt;          // color key for destination blt use
		ddColorKey          ddckCKSrcOverlay;       // color key for source overlay use
		ddColorKey          ddckCKSrcBlt;           // color key for source blt use
		union
		{
			DdPixelFormat		ddpfPixelFormat;        // pixel format description of the surface
			uint32_t			dwFVF;                  // vertex format description of vertex buffers
		};
		ddCaps2			ddsCaps;                // direct draw surface capabilities
		uint32_t		dwTextureStage;         // stage in multitexture cascade
	} DdSurface;

	typedef struct {
		uint32_t/*DXGI_FORMAT*/					dxgiFormat;
		uint32_t/*D3D10_RESOURCE_DIMENSION*/	resourceDimension;
		uint32_t								miscFlag;
		uint32_t								arraySize;
		uint32_t								reserved;
	} DdsHeader10;

	enum { FOURCC_DXT1 = 0x31545844, FOURCC_DXT3 = 0x33545844, FOURCC_DXT5 = 0x35545844, FOURCC_DX10 = 0x30315844, DDPF_FOURCC = 0x4 };

	auto ddsStream = dataSource->createStream();
	DdSurface ddsd;
	DdsHeader10 ddsHeader10;
	char filecode[4];
	ddsStream->readData( filecode, 4 );
	if( strncmp( filecode, "DDS ", 4 ) != 0 ) { 
		throw DdsParseExc( "File identifier mismatch" );
	}

	ddsStream->readData( &ddsd, 124/*sizeof(ddsd)*/ );

	// has header 10
	if( ( ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC ) && ( ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DX10 ) ) {
		ddsStream->readData( &ddsHeader10, sizeof(DdsHeader10) );
	}

	uint32_t width = ddsd.dwWidth; 
	uint32_t height = ddsd.dwHeight; 

	// how big (worst case) is it going to be including all mipmaps?
	uint32_t maxBufSize = ( ddsd.dwMipMapCount > 1 ) ? ( width * height * 2 ) : ( width * height ); 
	unique_ptr<uint8_t> pixels( new uint8_t[maxBufSize] );

	int numMipMaps = ddsd.dwMipMapCount;
	int dataFormat;
	switch( ddsd.ddpfPixelFormat.dwFourCC ) { 
		case FOURCC_DXT1: 
			dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; 
		break; 
		case FOURCC_DXT3: 
			dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break; 
		case FOURCC_DXT5: 
			dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; 
		break;
		case FOURCC_DX10:
			switch( ddsHeader10.dxgiFormat ) {
				case 70/*DXGI_FORMAT_BC1_TYPELESS*/:
				case 71/*DXGI_FORMAT_BC1_UNORM*/:
				case 72/*DXGI_FORMAT_BC1_UNORM_SRGB*/:
					dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
				case 73/*DXGI_FORMAT_BC2_TYPELESS*/:
				case 74/*DXGI_FORMAT_BC2_UNORM*/:
				case 75/*DXGI_FORMAT_BC2_UNORM_SRGB*/:
					dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
				case 76/*DXGI_FORMAT_BC3_TYPELESS*/:
				case 77/*DXGI_FORMAT_BC3_UNORM*/:
				case 78/*DXGI_FORMAT_BC3_UNORM_SRGB*/:
					dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
#if ! defined( CINDER_GL_ANGLE )
				case 97/*DXGI_FORMAT_BC7_TYPELESS*/:
				case 98/*DXGI_FORMAT_BC7_UNORM*/:
				case 99/*DXGI_FORMAT_BC7_UNORM_SRGB*/:
					dataFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
				break;
#endif
				default:
					throw DdsParseExc( "Unsupported image format" );
			}
		break;
		default:
			throw DdsParseExc( "Unsupported image format" );
		break;
	} 

	size_t blockSizeBytes;
	if( ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC )
		blockSizeBytes = ( dataFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ) ? 8 : 16;
	else
		blockSizeBytes = ( ddsd.ddpfPixelFormat.dwRGBBitCount + 7 ) / 8;

	// Create the texture
	GLenum target = format.mTarget;
	GLuint texID;
	glGenTextures( 1, &texID );

	TextureRef result = Texture::create( target, texID, width, height, false );
	result->mWidth = width;
	result->mHeight = height;
	result->mInternalFormat = dataFormat;

	if( numMipMaps > 1 && ( format.mMipmapping || ( ! format.mMipmappingSpecified ) ) )
		format.mMipmapping = true;

	TextureBindScope bindScope( result );
	result->initParams( format, GL_RGB /*ignored*/ );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	// load the mipmaps
	for( int level = 0; level <= numMipMaps && (width || height); ++level ) { 
		int levelWidth = std::max<int>( 1, (width>>level) );
		int levelHeight = std::max<int>( 1, (height>>level) );
		int blockWidth = std::max<int>( 1, (levelWidth+3) / 4 );
		int blockHeight = std::max<int>( 1, (levelHeight+3) / 4 );
		int rowBytes = blockWidth * blockSizeBytes;

		ddsStream->readDataAvailable( pixels.get(), blockHeight * rowBytes );
		glCompressedTexImage2D( result->mTarget, level, dataFormat, levelWidth, levelHeight, 0, blockHeight * rowBytes, pixels.get() );
	}

	return result;
}
#endif // ! defined( CINDER_GLES )

TextureResizeExc::TextureResizeExc( const string &message, const Vec2i &updateSize, const Vec2i &textureSize )
	 : TextureDataExc( "" )
{
	stringstream ss;
	ss << message << ", update size: " << updateSize << ", texture size: " << textureSize << ")";
	mMessage = ss.str();
}

} } // namespace cinder::gl
