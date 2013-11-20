#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/Rect.h"
#include "cinder/Stream.h"

#include <vector>
#include <utility>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Texture>		TextureRef;
typedef std::shared_ptr<class Texture2d>	Texture2dRef;
typedef std::shared_ptr<class Texture3d>	Texture3dRef;

class TextureBase {
  public:
	~TextureBase();

	//! the Texture's internal format, which is the format that OpenGL stores the texture data in memory. Common values include \c GL_RGB, \c GL_RGBA and \c GL_LUMINANCE
	GLint			getInternalFormat() const;
	//! the ID number for the texture, appropriate to pass to calls like \c glBindTexture()
	GLuint			getId() const { return mTextureId; }
	//! the target associated with texture. Typical values are \c GL_TEXTURE_2D and \c GL_TEXTURE_RECTANGLE_ARB
	GLenum			getTarget() const { return mTarget; }
	//!	Binds the Texture's texture to its target in the multitexturing unit \c GL_TEXTURE0 + \a textureUnit
	void 			bind( GLuint textureUnit = 0 ) const;
	//!	Unbinds the Texture currently bound in the Texture's target
	void			unbind( GLuint textureUnit = 0 ) const;

	//! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE.
	void			setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
	/** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
	 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
	void			setWrapS( GLenum wrapS );
	/** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
	 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
	void			setWrapT( GLenum wrapT );
	/** \brief Sets the verical depth behavior when a texture coordinate falls outside the range of [0,1].
	 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. **/
	void			setWrapR( GLenum wrapR );
	/** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution.
	 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
	void			setMinFilter( GLenum minFilter );
	/** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution.
	 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR **/
	void			setMagFilter( GLenum magFilter );
	/** Sets the anisotropic filtering amount **/
	void			setMaxAnisotropy( GLfloat maxAnisotropy );
	
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
	/** Gets the maximum anisotropic filtering maximum allowed by the extension **/
	static GLfloat	getMaxMaxAnisotropy();
	

	struct Format {			
		//! Specifies the texture's target. The default is \c GL_TEXTURE_2D
		void	setTarget( GLenum target ) { mTarget = target; }
		//! Sets the texture's target to be \c GL_TEXTURE_RECTANGLE. Not available in OpenGL ES.
#if ! defined( CINDER_GLES )
		void	setTargetRect() { mTarget = GL_TEXTURE_RECTANGLE; }
#endif
		
		//! Enables or disables mipmapping. Default is disabled.
		void	enableMipmapping( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; }
		void	enableRenderTexture( bool renderTexture = true ) { mRenderTexture = renderTexture; }
		void	enableDepthTexture( bool depthTexture = true ) { mDepthTexture = depthTexture; }
			
		//! Sets the Texture's internal format. A value of -1 implies selecting the best format for the context.
		void	setInternalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; }
		//! Sets the Texture's internal format to be automatically selected based on the context.
		void	setAutoInternalFormat() { mInternalFormat = -1; }
		
		void	setPixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; }
			
		void	setPixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; }
		
		//! Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP
		void	setWrap( GLenum wrapS, GLenum wrapT ) { setWrapS( wrapS ); setWrapT( wrapT ); }
		/** \brief Sets the horizontal wrapping behavior when a texture coordinate falls outside the range of [0,1].
		 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE **/
		void	setWrapS( GLenum wrapS ) { mWrapS = wrapS; }
		/** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
		 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE. **/
		void	setWrapT( GLenum wrapT ) { mWrapT = wrapT; }
		/** \brief Sets the verical wrapping behavior when a texture coordinate falls outside the range of [0,1].
		 Possible values are \c GL_CLAMP, \c GL_REPEAT and \c GL_CLAMP_TO_EDGE. The default is \c GL_CLAMP_TO_EDGE. **/
		void	setWrapR( GLenum wrapR ) { mWrapR = wrapR; }
		/** \brief Sets the filtering behavior when a texture is displayed at a lower resolution than its native resolution. Default is \c GL_LINEAR
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void	setMinFilter( GLenum minFilter ) { mMinFilter = minFilter; }
		/** Sets the filtering behavior when a texture is displayed at a higher resolution than its native resolution. Default is \c GL_LINEAR
		 * Possible values are \li \c GL_NEAREST \li \c GL_LINEAR \li \c GL_NEAREST_MIPMAP_NEAREST \li \c GL_LINEAR_MIPMAP_NEAREST \li \c GL_NEAREST_MIPMAP_LINEAR \li \c GL_LINEAR_MIPMAP_LINEAR **/
		void	setMagFilter( GLenum magFilter ) { mMagFilter = magFilter; }
		//! Sets the anisotropic filter amount
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
		GLenum	getWrapR() const { return mWrapR; }
		//! Returns the texture minifying function, which is used whenever the pixel being textured maps to an area greater than one texture element.
		GLenum	getMinFilter() const { return mMinFilter; }
		//! Returns the texture magnifying function, which is used whenever the pixel being textured maps to an area less than or equal to one texture element.
		GLenum	getMagFilter() const { return mMagFilter; }
		//! Returns the texture anisotropic filtering amount
		GLfloat getMaxAnisotropy() const { return mMaxAnisotropy; }
		//! Returns whether the texture is a renderTexture.
		bool	isRenderTexture() const { return mRenderTexture; }
		//! Returns whether the texture is a depthTexture.
		bool	isDepthTexture() const { return mDepthTexture; }
		
	protected:
		Format();
	
		GLenum			mTarget;
		GLenum			mWrapS, mWrapT, mWrapR;
		GLenum			mMinFilter, mMagFilter;
		bool			mMipmapping;
		GLfloat         mMaxAnisotropy;
		GLint			mInternalFormat;
		GLenum			mPixelDataFormat;
		GLenum			mPixelDataType;
		bool			mRenderTexture;
		bool			mDepthTexture;
		
		friend class TextureBase;
	};

  protected:
	TextureBase();
	TextureBase( GLenum target, GLuint textureId, GLint internalFormat );
	
	void			initParams( const Format &format );
	
  	GLenum			mTarget;
	GLuint			mTextureId;
	mutable GLint	mInternalFormat;
	bool			mDoNotDispose;	
};
	
class Texture : public TextureBase {
  public:
	struct Format : public TextureBase::Format {
		//! Default constructor, sets the target to \c GL_TEXTURE_2D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
		Format() : TextureBase::Format() {}

		//! Chaining functions for Format class.
		Format& target( GLenum target ) { mTarget = target; return *this; }
		Format& mipmap( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; return *this; }
		Format& maxAnisotropy( float maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; return *this; }
		Format& internalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; return *this; }
		Format& wrap( GLenum wrap ) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
		Format& wrapS( GLenum wrapS ) { mWrapS = wrapS; return *this; }
		Format& wrapT( GLenum wrapT ) { mWrapT = wrapT; return *this; }
		Format& wrapR( GLenum wrapR ) { mWrapR = wrapR; return *this; }		
		Format& minFilter( GLenum minFilter ) { mMinFilter = minFilter; return *this; }
		Format& magFilter( GLenum magFilter ) { mMagFilter = magFilter; return *this; }
		Format& pixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; return *this; }
		Format& pixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; return *this; }
		Format& depthTexture( bool depthTexture = true ) { mDepthTexture = depthTexture; return *this; }
		Format& renderTexture( bool renderTexture = true ) { mRenderTexture = renderTexture; return *this; }
		
		friend Texture;
	};
	
	/** \brief Constructs a texture of size(\a width, \a height), storing the data in internal format \a aInternalFormat. **/
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
	
	//!	Creates a new Texture from raw DirectDraw Stream data
	static Texture	loadDds( IStreamRef ddsStream, Format format );
	
#if ! defined( CINDER_GLES )
	//! Returns an ImageSource pointing to this Texture
	operator	ImageSourceRef() const;
#endif
	
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
	
	void	init( const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format );
	void	init( const float *data, GLint dataFormat, const Format &format );
	void	init( ImageSourceRef imageSource, const Format &format );
	void	init( const Format &format );
	
	mutable GLint	mWidth, mHeight, mCleanWidth, mCleanHeight;
	float			mMaxU, mMaxV;
	bool			mFlipped;
	
	friend class TextureCache;
};

class Texture3d : public TextureBase {
  public:
	struct Format : public TextureBase::Format {
		//! Default constructor, sets the target to \c GL_TEXTURE_3D, wrap to \c GL_CLAMP, disables mipmapping, the internal format to "automatic"
		Format() : TextureBase::Format() { mTarget = GL_TEXTURE_3D; }

		//! Chaining functions for Format class.
		Format& target( GLenum target ) { mTarget = target; return *this; }
		Format& mipmap( bool enableMipmapping = true ) { mMipmapping = enableMipmapping; return *this; }
		Format& maxAnisotropy( float maxAnisotropy ) { mMaxAnisotropy = maxAnisotropy; return *this; }
		Format& internalFormat( GLint internalFormat ) { mInternalFormat = internalFormat; return *this; }
		Format& wrap( GLenum wrap ) { mWrapS = mWrapT = mWrapR = wrap; return *this; }
		Format& wrapS( GLenum wrapS ) { mWrapS = wrapS; return *this; }
		Format& wrapT( GLenum wrapT ) { mWrapT = wrapT; return *this; }
		Format& wrapR( GLenum wrapR ) { mWrapR = wrapR; return *this; }		
		Format& minFilter( GLenum minFilter ) { mMinFilter = minFilter; return *this; }
		Format& magFilter( GLenum magFilter ) { mMagFilter = magFilter; return *this; }
		Format& pixelDataFormat( GLenum pixelDataFormat ) { mPixelDataFormat = pixelDataFormat; return *this; }
		Format& pixelDataType( GLenum pixelDataType ) { mPixelDataType = pixelDataType; return *this; }
		Format& depthTexture( bool depthTexture = true ) { mDepthTexture = depthTexture; return *this; }
		Format& renderTexture( bool renderTexture = true ) { mRenderTexture = renderTexture; return *this; }
		
		friend Texture3d;
	};

	static Texture3dRef create( GLint width, GLint height, GLint depth, Format format = Format() );
  
	void	update( const Surface &surface, int depth, int mipLevel = 0 );
	
	//! Returns the width of the texture in pixels
	GLint			getWidth() const { return mWidth; }
	//! Returns the height of the texture in pixels
	GLint			getHeight() const { return mHeight; }
	//! Returns the depth of the texture, which is the number of images in a texture array, or the depth of a 3D texture measured in pixels
	GLint			getDepth() const { return mDepth; }


  protected:
  	Texture3d( GLint width, GLint height, GLint depth, Format format );
	
	GLint		mWidth, mHeight, mDepth;
};

typedef std::shared_ptr<class TextureCache> TextureCacheRef;
	
class TextureCache
{
  public:
	TextureCacheRef create();
	TextureCacheRef create( const Surface8u &prototypeSurface, const Texture::Format &format );
	
	TextureRef		cache( const Surface8u &data );
  protected:
	TextureCache();
	TextureCache( const Surface8u &prototypeSurface, const Texture::Format &format );
		
	void			markTextureAsFree( int id );
	
	int				mWidth;
	int				mHeight;
	Texture::Format	mFormat;
	
	int										mNextId;
	std::vector<std::pair<int,TextureRef> >	mTextures;
	
	static void textureCacheDeallocator( void *aDeallocatorRefcon );
};


class SurfaceConstraintsGLTexture : public SurfaceConstraints
{
  public:
	virtual SurfaceChannelOrder getChannelOrder( bool alpha ) const { return ( alpha ) ? SurfaceChannelOrder::BGRA : SurfaceChannelOrder::BGR; }
	virtual int32_t				getRowBytes( int requestedWidth, const SurfaceChannelOrder &sco, int elementSize ) const { return requestedWidth * elementSize * sco.getPixelInc(); }
};

class TextureDataExc : public std::exception
{
  public:	
	TextureDataExc( const std::string &log ) throw();
	virtual const char* what() const throw()
	{
		return mMessage;
	}
	
  private:
	char	mMessage[ 16001 ];
	GLint	mShaderType;
};

	
} } // namespace cinder::gl
