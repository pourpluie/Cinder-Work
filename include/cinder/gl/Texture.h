#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/Rect.h"
#include "cinder/Stream.h"
#include "cinder/Exception.h"
#include "cinder/DataSource.h"

#include <vector>
#include <utility>
#include <array>

#if defined( CINDER_GLES )
#define GL_BLUE		0x1905
#define GL_GREEN	0x1904
#define GL_RED		0x1903
#endif

namespace cinder { namespace gl {

typedef std::shared_ptr<class TextureBase>		TextureBaseRef;
typedef std::shared_ptr<class Texture>			TextureRef;
typedef class Texture							Texture2d;
typedef std::shared_ptr<Texture2d>				Texture2dRef;
typedef std::shared_ptr<class Texture3d>		Texture3dRef;
typedef std::shared_ptr<class TextureCubeMap>	TextureCubeMapRef;

class TextureBase {
  public:
	virtual ~TextureBase();

	//! the Texture's internal format, which is the format that OpenGL stores the texture data in memory. Common values include \c GL_RGB, \c GL_RGBA and \c GL_LUMINANCE
	GLint			getInternalFormat() const;
	//! the ID number for the texture, appropriate to pass to calls like \c glBindTexture()
	GLuint			getId() const { return mTextureId; }
	//! the target associated with texture. Typical values are \c GL_TEXTURE_2D and \c GL_TEXTURE_RECTANGLE_ARB
	GLenum			getTarget() const { return mTarget; }
	//!	Binds the Texture's texture to its target in the multitexturing unit \c GL_TEXTURE0 + \a textureUnit
	void 			bind( uint8_t textureUnit = 0 ) const;
	//!	Unbinds the Texture currently bound in the Texture's target
	void			unbind( uint8_t textureUnit = 0 ) const;

	//! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
	void			setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
	//! Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT and \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
	void			setWrapS( GLenum wrapS );
	//! Sets the vertical wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
	void			setWrapT( GLenum wrapT );
#if ! defined( CINDER_GLES )
	//! Sets the depth wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
	void			setWrapR( GLenum wrapR );
#endif	
	/** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution.
	 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
	void			setMinFilter( GLenum minFilter );
	/** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution.
	 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR **/
	void			setMagFilter( GLenum magFilter );
	//! Sets the anisotropic filtering amount. A value greater than 1.0 "enables" anisotropic filtering. Maximum of getMaxMaxAnisotropy();
	void			setMaxAnisotropy( GLfloat maxAnisotropy );
	//! Returns whether the Texture has Mipmapping enabled
	bool			hasMipmapping() const { return mMipmapping; }
	
	//! Returns the appropriate parameter to glGetIntegerv() for a specific target; ie GL_TEXTURE_2D -> GL_TEXTURE_BINDING_2D. Returns 0 on failure.
	static GLenum	getBindingConstantForTarget( GLenum target );
	//! Converts a SurfaceChannelOrder into an appropriate OpenGL dataFormat and type
	static void		SurfaceChannelOrderToDataFormatAndType( const SurfaceChannelOrder &sco, GLint *dataFormat, GLenum *type );
	//! Returns whether a given OpenGL dataFormat contains an alpha channel
	static bool		dataFormatHasAlpha( GLint dataFormat );
	//! Returns whether a give OpenGL dataFormat contains color channels
	static bool		dataFormatHasColor( GLint dataFormat );
	//! calculate the size of mipMap for the corresponding level
	static Vec2i	calcMipLevelSize( int level, GLint width, GLint height );
	//! Returns the maximum anisotropic filtering maximum allowed by the hardware
	static GLfloat	getMaxMaxAnisotropy();

	//! Returns the Texture's swizzle mask (corresponding to \c GL_TEXTURE_SWIZZLE_RGBA)	
	std::array<GLint,4>	getSwizzleMask() const { return mSwizzleMask; }
	//! Returns whether this hardware supports texture swizzling (via \c GL_TEXTURE_SWIZZLE_RGBA)
	static bool		supportsHardwareSwizzle();

	struct Format {			
		//! Specifies the texture's target. The default is \c GL_TEXTURE_2D
		void	setTarget( GLenum target ) { mTarget = target; }
		//! Sets the texture's target to be \c GL_TEXTURE_RECTANGLE. Not available in OpenGL ES.
#if ! defined( CINDER_GLES )
		void	setTargetRect() { mTarget = GL_TEXTURE_RECTANGLE; }
#endif
		
		//! Enables or disables mipmapping. Default is disabled.
		void	enableMipmapping( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; }
			
		//! Sets the Texture's internal format. A value of -1 implies selecting the best format for the context.
		void	setInternalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; }
		//! Sets the Texture's internal format to be automatically selected based on the context.
		void	setAutoInternalFormat() { mInternalFormat = -1; }
		
		void	setPixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; }
			
		void	setPixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; }
		
		//! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
		void	setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
		//! Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
		void	setWrapS( GLenum wrapS ) { mWrapS = wrapS; }
		//! Sets the vertical wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
		void	setWrapT( GLenum wrapT ) { mWrapT = wrapT; }
#if ! defined( CINDER_GLES )
		//! Sets the depth wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_REPEAT, \c GL_CLAMP_TO_EDGE, etc. Default is \c GL_CLAMP_TO_EDGE.
		void	setWrapR( GLenum wrapR ) { mWrapR = wrapR; }
#endif
		//! Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution. Default is \c GL_LINEAR unless mipmapping is enabled, in which case \c GL_LINEAR_MIPMAP_LINEAR
		void	setMinFilter( GLenum minFilter ) { mMinFilter = minFilter; mMinFilterSpecified = true; }
		//! Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution. Default is \c GL_LINEAR.
		void	setMagFilter( GLenum magFilter ) { mMagFilter = magFilter; }
		//! Sets the anisotropic filter amount. A value greater than 1.0 "enables" anisotropic filtering. Maximum of getMaxMaxAnisotropy();
		void    setMaxAnisotropy( GLfloat maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; }
		
		//! Returns the texture's target
		GLenum	getTarget() const { return mTarget; }
		//! Returns whether the texture has mipmapping enabled
		bool	hasMipmapping() const { return mMipmapping; }
		
		//! Returns the Texture's internal format. A value of -1 implies automatic selection of the internal format based on the context.
		GLint	getInternalFormat() const { return mInternalFormat; }
		//! Returns whether the Texture's internal format will be automatically selected based on the context.
		bool	isAutoInternalFormat() const { return mInternalFormat == -1; }
		
		//! Returns the Texture's pixel format. A value of -1 implies automatic selection of the pixel format based on the context.
		GLenum	getPixelDataFormat() const { return mPixelDataFormat; }
		//! Returns the Texture's data type. A value of -1 implies automatic selection of the data type based on the context.
		GLint	getPixelDataType() const { return mInternalFormat; }
		
		//! Returns the horizontal wrapping behavior for the texture coordinates.
		GLenum	getWrapS() const { return mWrapS; }
		//! Returns the vertical wrapping behavior for the texture coordinates.
		GLenum	getWrapT() const { return mWrapT; }
		//! Returns the depth wrapping behavior for the texture coordinates.
#if ! defined( CINDER_GLES )
		GLenum	getWrapR() const { return mWrapR; }
#endif
		//! Returns the texture minifying function, which is used whenever the pixel being textured maps to an area greater than one texture element.
		GLenum	getMinFilter() const { return mMinFilter; }
		//! Returns the texture magnifying function, which is used whenever the pixel being textured maps to an area less than or equal to one texture element.
		GLenum	getMagFilter() const { return mMagFilter; }
		//! Returns the texture anisotropic filtering amount
		GLfloat getMaxAnisotropy() const { return mMaxAnisotropy; }
		
		//! Sets the swizzle mask corresponding to \c GL_TEXTURE_SWIZZLE_RGBA. Expects \c GL_RED through \c GL_ALPHA, or \c GL_ONE or \c GL_ZERO
		void	setSwizzleMask( const std::array<GLint,4> &swizzleMask ) { mSwizzleMask = swizzleMask; }
		//! Returns the swizzle mask corresponding to \c GL_TEXTURE_SWIZZLE_RGBA.
		const std::array<GLint,4>&	getSwizzleMask() const { return mSwizzleMask; }
		
	protected:
		Format();
	
		GLenum				mTarget;
		GLenum				mWrapS, mWrapT, mWrapR;
		GLenum				mMinFilter, mMagFilter;
		bool				mMipmapping;
		bool				mMinFilterSpecified;
		GLfloat				mMaxAnisotropy;
		GLint				mInternalFormat;
		GLint				mPixelDataFormat;
		GLenum				mPixelDataType;
		std::array<GLint,4>	mSwizzleMask;
		
		friend class TextureBase;
	};

  protected:
	TextureBase();
	TextureBase( GLenum target, GLuint textureId, GLint internalFormat );
	
	void			initParams( Format &format, GLint defaultInternalFormat );
	
	GLenum				mTarget;
	GLuint				mTextureId;
	mutable GLint		mInternalFormat;
	bool				mMipmapping;
	bool				mDoNotDispose;
	std::array<GLint,4>	mSwizzleMask;	
};
	
class Texture : public TextureBase {
  public:
	struct Format : public TextureBase::Format {
		//! Default constructor, sets the target to \c GL_TEXTURE_2D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
		Format() : TextureBase::Format() {}

		//! Chaining functions for Format class.
		Format& target( GLenum target ) { mTarget = target; return *this; }
		Format& mipmap( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; return *this; }
		//! Sets the maximum amount of anisotropic filtering. A value greater than 1.0 "enables" anisotropic filtering. Maximum of getMaxMaxAnisotropy();
		Format& maxAnisotropy( float maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; return *this; }
		Format& internalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; return *this; }
		Format& wrap( GLenum wrap ) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
		Format& wrapS( GLenum wrapS ) { mWrapS = wrapS; return *this; }
		Format& wrapT( GLenum wrapT ) { mWrapT = wrapT; return *this; }
#if ! defined( CINDER_GLES )
		Format& wrapR( GLenum wrapR ) { mWrapR = wrapR; return *this; }
#endif
		Format& minFilter( GLenum minFilter ) { setMinFilter( minFilter ); return *this; }
		Format& magFilter( GLenum magFilter ) { setMagFilter( magFilter ); return *this; }
		//! Corresponds to the 'format' parameter of glTexImage*(). Defaults to match the internalFormat
		Format& pixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; return *this; }
		//! Corresponds to the 'type' parameter of glTexImage*(). Defaults to \c GL_UNSIGNED_BYTE
		Format& pixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; return *this; }
		
		friend Texture;
	};
	
	//! Constructs a texture of size(\a width, \a height) and allocates storage.
	static TextureRef	create( int width, int height, Format format = Format() );
	/** \brief Constructs a texture of size(\a width, \a height), storing the data in internal format \a aInternalFormat. Pixel data is provided by \a data and is expected to be interleaved and in format \a dataFormat, for which \c GL_RGB or \c GL_RGBA would be typical values. **/
	static TextureRef	create( const unsigned char *data, int dataFormat, int width, int height, Format format = Format() );
	/** \brief Constructs a texture based on the contents of \a surface. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	static TextureRef	create( const Surface8u &surface, Format format = Format() );
	/** \brief Constructs a texture based on the contents of \a surface. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	static TextureRef	create( const Surface32f &surface, Format format = Format() );
	/** \brief Constructs a texture based on the contents of \a channel. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	static TextureRef	create( const Channel8u &channel, Format format = Format() );
	/** \brief Constructs a texture based on the contents of \a channel. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	static TextureRef	create( const Channel32f &channel, Format format = Format() );
	/** \brief Constructs a texture based on \a imageSource. A default value of -1 for \a internalFormat chooses an appropriate internal format based on the contents of \a imageSource. **/
	static TextureRef	create( ImageSourceRef imageSource, Format format = Format() );
	//! Constructs a Texture based on an externally initialized OpenGL texture. \a doNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
	static TextureRef	create( GLenum aTarget, GLuint aTextureID, int width, int height, bool doNotDispose );
#if ! defined( CINDER_GLES )
	//! Constructs a Texture from a DDS file. Returns a nullptr if the creation fails.
	static TextureRef	createFromDds( const DataSourceRef &dataSource, Format format = Format() );
#endif

	/** Designed to accommodate texture where not all pixels are "clean", meaning the maximum texture coordinate value may not be 1.0 (or the texture's width in \c GL_TEXTURE_RECTANGLE_ARB) **/
	void			setCleanTexCoords( float maxU, float maxV );
	
	//! Replaces the pixels of a texture with contents of \a surface. Expects \a surface's size to match the Texture's.
	void			update( const Surface &surface, int mipLevel = 0 );
	//! Replaces the pixels of a texture with contents of \a surface. Expects \a surface's size to match the Texture's.
	void			update( const Surface32f &surface, int mipLevel = 0 );
	/** \brief Replaces the pixels of a texture with contents of \a surface. Expects \a area's size to match the Texture's.
	 \todo Method for updating a subrectangle with an offset into the source **/
	void			update( const Surface &surface, const Area &area, int mipLevel = 0 );
	//! Replaces the pixels of a texture with contents of \a channel. Expects \a channel's size to match the Texture's.
	void			update( const Channel32f &channel, int mipLevel = 0 );
	//! Replaces the pixels of a texture with contents of \a channel. Expects \a area's size to match the Texture's.
	void			update( const Channel8u &channel, const Area &area, int mipLevel = 0 );
	
	//! calculates and sets the total levels of mipmap
	GLint			getNumMipLevels() const;
	//! the width of the texture in pixels
	GLint			getWidth() const;
	//! the height of the texture in pixels
	GLint			getHeight() const;
	//! the width of the texture in pixels accounting for its "clean" area - \sa getCleanBounds()
	GLint			getCleanWidth() const;
	//! the height of the texture in pixels accounting for its "clean" area - \sa getCleanBounds()
	GLint			getCleanHeight() const;
	//! the size of the texture in pixels
	Vec2i			getSize() const { return Vec2i( getWidth(), getHeight() ); }
	//! the aspect ratio of the texture (width / height)
	float			getAspectRatio() const { return getWidth() / (float)getHeight(); }
	//! the Area defining the Texture's bounds in pixels: [0,0]-[width,height]
	Area			getBounds() const { return Area( 0, 0, getWidth(), getHeight() ); }
	//! the Area defining the Texture's clean pixel bounds in pixels: [0,0]-[width*maxU,height*maxV]
	Area			getCleanBounds() const { return Area( 0, 0, getCleanWidth(), getCleanHeight() ); }
	//! whether the texture has an alpha channel
	bool			hasAlpha() const;
	//!	These return the right thing even when the texture coordinate space is flipped
	float			getLeft() const;
	float			getRight() const;
	float			getTop() const;
	float			getBottom() const;
	//!	These do not correspond to "top" and "right" when the texture is flipped
	float			getMaxU() const;
	float			getMaxV() const;
	//! Returns the UV coordinates which correspond to the pixels contained in \a area. Does not compensate for clean coordinates. Does compensate for flipping.
	Rectf			getAreaTexCoords( const Area &area ) const;
	//!	whether the texture is flipped vertically
	bool			isFlipped() const { return mFlipped; }
	//!	Marks the texture as being flipped vertically or not
	void			setFlipped( bool aFlipped = true ) { mFlipped = aFlipped; }
	
	//! Returns an ImageSource pointing to this Texture
	ImageSourceRef	createSource();
	
	// These constructors are not protected to allow for shared_ptr's with custom deleters
	/** Consider Texture::create() instead. Constructs a texture of size(\a width, \a height), storing the data in internal format \a aInternalFormat. **/
	Texture( int width, int height, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture of size(\a width, \a height), storing the data in internal format \a aInternalFormat. Pixel data is provided by \a data and is expected to be interleaved and in format \a dataFormat, for which \c GL_RGB or \c GL_RGBA would be typical values. **/
	Texture( const unsigned char *data, int dataFormat, int width, int height, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture based on the contents of \a surface. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	Texture( const Surface8u &surface, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture based on the contents of \a surface. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	Texture( const Surface32f &surface, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture based on the contents of \a channel. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	Texture( const Channel8u &channel, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture based on the contents of \a channel. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	Texture( const Channel32f &channel, Format format = Format() );
	/** Consider Texture::create() instead. Constructs a texture based on \a imageSource. A default value of -1 for \a internalFormat chooses an appropriate internal format based on the contents of \a imageSource. **/
	Texture( ImageSourceRef imageSource, Format format = Format() );
	//! Consider Texture::create() instead. Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
	Texture( GLenum aTarget, GLuint aTextureID, int width, int height, bool doNotDispose );

  protected:
	void	initData( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format );
	void	initData( const float *data, GLint dataFormat, const Format &format );
	void	initData( ImageSourceRef imageSource, const Format &format );

	mutable GLint	mWidth, mHeight, mCleanWidth, mCleanHeight;
	float			mMaxU, mMaxV;
	bool			mFlipped;
	
	friend class TextureCache;
};

#ifndef CINDER_GLES
class Texture3d : public TextureBase {
  public:
	struct Format : public TextureBase::Format {
		//! Default constructor, sets the target to \c GL_TEXTURE_3D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
		Format() : TextureBase::Format() { mTarget = GL_TEXTURE_3D; }

		//! Chaining functions for Format class.
		//! Sets the target, defaults to \c GL_TEXTURE_3D, also supports \c GL_TEXTURE_2D_ARRAY
		Format& target( GLenum target ) { mTarget = target; return *this; }
		Format& mipmap( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; return *this; }
		//! Sets the maximum amount of anisotropic filtering. A value greater than 1.0 "enables" anisotropic filtering. Maximum of getMaxMaxAnisotropy();
		Format& maxAnisotropy( float maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; return *this; }
		Format& internalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; return *this; }
		Format& wrap( GLenum wrap ) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
		Format& wrapS( GLenum wrapS ) { mWrapS = wrapS; return *this; }
		Format& wrapT( GLenum wrapT ) { mWrapT = wrapT; return *this; }
#if ! defined( CINDER_GLES )
		Format& wrapR( GLenum wrapR ) { mWrapR = wrapR; return *this; }
#endif
		Format& minFilter( GLenum minFilter ) { setMinFilter( minFilter ); return *this; }
		Format& magFilter( GLenum magFilter ) { setMagFilter( magFilter ); return *this; }
		Format& pixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; return *this; }
		Format& pixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; return *this; }
		
		friend Texture3d;
	};

	static Texture3dRef create( GLint width, GLint height, GLint depth, Format format = Format() );
	static Texture3dRef create( GLint width, GLint height, GLint depth, GLenum dataFormat, const uint8_t *data, Format format = Format() );	
  
	void	update( const Surface &surface, int depth, int mipLevel = 0 );
	
	//! Returns the width of the texture in pixels
	GLint			getWidth() const { return mWidth; }
	//! Returns the height of the texture in pixels
	GLint			getHeight() const { return mHeight; }
	//! Returns the depth of the texture, which is the number of images in a texture array, or the depth of a 3D texture measured in pixels
	GLint			getDepth() const { return mDepth; }
	//! the aspect ratio of the texture (width / height)
	float			getAspectRatio() const { return getWidth() / (float)getHeight(); }
	//! the Area defining the Texture's 2D bounds in pixels: [0,0]-[width,height]
	Area			getBounds() const { return Area( 0, 0, getWidth(), getHeight() ); }

  protected:
  	Texture3d( GLint width, GLint height, GLint depth, Format format );
	Texture3d( GLint width, GLint height, GLint depth, GLenum dataFormat, const uint8_t *data, Format format );
	GLint		mWidth, mHeight, mDepth;
};
#endif

class TextureCubeMap : public TextureBase
{
  public:
  	struct Format : public TextureBase::Format {
		//! Default constructor, sets the target to \c GL_TEXTURE_CUBE_MAP, wrap to \c GL_CLAMP_TO_EDGE, disables mipmapping, the internal format to "automatic"
		Format();

		//! Chaining functions for Format class.
		Format& target( GLenum target ) { mTarget = target; return *this; }
		Format& mipmap( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; return *this; }
		//! Sets the maximum amount of anisotropic filtering. A value greater than 1.0 "enables" anisotropic filtering. Maximum of getMaxMaxAnisotropy();
		Format& maxAnisotropy( float maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; return *this; }
		Format& internalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; return *this; }
		Format& wrap( GLenum wrap ) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
		Format& wrapS( GLenum wrapS ) { mWrapS = wrapS; return *this; }
		Format& wrapT( GLenum wrapT ) { mWrapT = wrapT; return *this; }
#if ! defined( CINDER_GLES )
		Format& wrapR( GLenum wrapR ) { mWrapR = wrapR; return *this; }		
#endif // ! defined( CINDER_GLES )
		Format& minFilter( GLenum minFilter ) { setMinFilter( minFilter ); return *this; }
		Format& magFilter( GLenum magFilter ) { setMagFilter( magFilter ); return *this; }
		
		friend TextureCubeMap;
	};
  
	static TextureCubeMapRef	create( int32_t width, int32_t height, const Format &format = Format() );
	static TextureCubeMapRef	createHorizontalCross( const ImageSourceRef &imageSource, const Format &format = Format() );
	//! Expects images ordered { +X, -X, +Y, -Y, +Z, -Z }
	static TextureCubeMapRef	create( const ImageSourceRef images[6], const Format &format = Format() );
	
  protected:
	TextureCubeMap( int32_t width, int32_t height, Format format );
	TextureCubeMap( const Surface8u images[6], Format format );
	
	GLint		mWidth, mHeight;
};

typedef std::shared_ptr<class TextureCache> TextureCacheRef;

class TextureCache
{
  public:
	static TextureCacheRef create();
	static TextureCacheRef create( const Surface8u &prototypeSurface, const Texture::Format &format );
	
	TextureRef		cache( const Surface8u &data );
  protected:
	TextureCache();
	TextureCache( const Surface8u &prototypeSurface, const Texture::Format &format );
		
	void			markTextureAsFree( int id );
	
	int				mWidth;
	int				mHeight;
	Texture::Format	mFormat;
	
	int										mNextId;
	std::vector<std::pair<int,TextureRef>>	mTextures;
};


class SurfaceConstraintsGLTexture : public SurfaceConstraints
{
  public:
	virtual SurfaceChannelOrder getChannelOrder( bool alpha ) const { return ( alpha ) ? SurfaceChannelOrder::BGRA : SurfaceChannelOrder::BGR; }
	virtual int32_t				getRowBytes( int requestedWidth, const SurfaceChannelOrder &sco, int elementSize ) const { return requestedWidth * elementSize * sco.getPixelInc(); }
};

class TextureDataExc : public Exception {
  public:	
	TextureDataExc( const std::string &message ) : mMessage( message )	{}

	virtual const char* what() const throw()	{ return mMessage.c_str(); }
	
  protected:
	std::string		mMessage;
};

class TextureResizeExc : public TextureDataExc {
  public:
	TextureResizeExc( const std::string &message, const Vec2i &updateSize, const Vec2i &textureSize );
};

	
} } // namespace cinder::gl
