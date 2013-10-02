/*
 Copyright (c) 2013, The Cinder Project
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org

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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Exception.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include <map>

namespace cinder { namespace gl {

class Renderbuffer;
typedef std::shared_ptr<Renderbuffer>	RenderbufferRef;
class Fbo;
typedef std::shared_ptr<Fbo>	FboRef;

//! Represents an OpenGL Renderbuffer, used primarily in conjunction with FBOs. Supported on OpenGL ES but multisampling is currently ignored. \ImplShared
class Renderbuffer {
  public:
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8
#if defined( CINDER_GLES )
	static RenderbufferRef create( int width, int height, GLenum internalFormat = GL_RGBA8_OES );
#else
	static RenderbufferRef create( int width, int height, GLenum internalFormat = GL_RGBA8 );
#endif
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8, MSAA samples \a msaaSamples, and CSAA samples \a coverageSamples
	static RenderbufferRef create( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples = 0 );

	~Renderbuffer();

	//! Returns the width of the Renderbuffer in pixels
	int		getWidth() const { return mWidth; }
	//! Returns the height of the Renderbuffer in pixels
	int		getHeight() const { return mHeight; }
	//! Returns the size of the Renderbuffer in pixels
	Vec2i	getSize() const { return Vec2i( mWidth, mHeight ); }
	//! Returns the bounding area of the Renderbuffer in pixels
	Area	getBounds() const { return Area( 0, 0, mWidth, mHeight ); }
	//! Returns the aspect ratio of the Renderbuffer
	float	getAspectRatio() const { return mWidth / (float)mHeight; }

	//! Returns the ID of the Renderbuffer
	GLuint	getId() const { return mId; }
	//! Returns the internal format of the Renderbuffer
	GLenum	getInternalFormat() const { return mInternalFormat; }
	//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling
	int		getSamples() const { return mSamples; }
	//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none.
	int		getCoverageSamples() const { return mCoverageSamples; }

  private:
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8
#if defined( CINDER_GLES )
	Renderbuffer( int width, int height, GLenum internalFormat = GL_RGBA8_OES );
#else
	Renderbuffer( int width, int height, GLenum internalFormat = GL_RGBA8 );
#endif
	//! Create a Renderbuffer \a width pixels wide and \a heigh pixels high, with an internal format of \a internalFormat, defaulting to GL_RGBA8, MSAA samples \a msaaSamples, and CSAA samples \a coverageSamples
	Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples = 0 );  
  
	void	init( int aWidth, int aHeight, GLenum internalFormat, int msaaSamples, int coverageSamples );
  
	int					mWidth, mHeight;
	GLuint				mId;
	GLenum				mInternalFormat;
	int					mSamples, mCoverageSamples;
};

//! Represents an OpenGL Framebuffer Object. //! Represents an instance of a font at a point size. \ImplShared
class Fbo {
  public:
	struct Format;

	//! Creates an FBO \a width pixels wide and \a height pixels high, using Fbo::Format \a format
	static FboRef create( int width, int height, Format format = Format() );
	//! Creates an FBO \a width pixels wide and \a height pixels high, with an optional alpha channel, color buffer and depth buffer
	static FboRef create( int width, int height, bool alpha, bool color = true, bool depth = true, bool stencil = false );
	~Fbo();

	//! Returns the width of the FBO in pixels
	int				getWidth() const { return mWidth; }
	//! Returns the height of the FBO in pixels
	int				getHeight() const { return mHeight; }
	//! Returns the size of the FBO in pixels
	Vec2i			getSize() const { return Vec2i( mWidth, mHeight ); }
	//! Returns the bounding area of the FBO in pixels
	Area			getBounds() const { return Area( 0, 0, mWidth, mHeight ); }
	//! Returns the aspect ratio of the FBO
	float			getAspectRatio() const { return mWidth / (float)mHeight; }
	//! Returns the Fbo::Format of this FBO
	const Format&	getFormat() const { return mFormat; }
	//! Returns the texture target for this FBO. Typically \c GL_TEXTURE_2D or \c GL_TEXTURE_RECTANGLE_ARB
	GLenum			getTarget() const { return mFormat.mColorTextureFormat.getTarget(); }

	//! Returns a reference to the color texture of the FBO. \a attachment specifies which attachment in the case of multiple color buffers
	TextureRef		getTexture( int attachment = 0 );
	//! Returns a reference to the depth texture of the FBO.
	TextureRef		getDepthTexture();	
	
	//! Binds the color texture associated with an Fbo to its target. Optionally binds to a multitexturing unit when \a textureUnit is non-zero. Optionally binds to a multitexturing unit when \a textureUnit is non-zero. \a attachment specifies which color buffer in the case of multiple attachments.
	void 			bindTexture( int textureUnit = 0, int attachment = 0 );
	//! Unbinds the texture associated with an Fbo's target
	void			unbindTexture();
	//! Binds the depth texture associated with an Fbo to its target.
	void 			bindDepthTexture( int textureUnit = 0 );
	//! Binds the Fbo as the currently active framebuffer, meaning it will receive the results of all subsequent rendering until it is unbound
	void 			bindFramebuffer();
	//! Unbinds the Fbo as the currently active framebuffer, restoring the primary context as the target for all subsequent rendering
	static void 	unbindFramebuffer();

	//! Returns the ID of the framebuffer itself. For antialiased FBOs this is the ID of the output multisampled FBO
	GLuint		getId() const { return mId; }

	//! For antialiased FBOs this returns the ID of the mirror FBO designed for reading, where the multisampled render buffers are resolved to. For non-antialised, this is the equivalent to getId()
	GLuint		getResolveId() const { if( mResolveFramebufferId ) return mResolveFramebufferId; else return mId; }

#if ! defined( CINDER_GLES )
	//! Copies to FBO \a dst from \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
	void		blitTo( Fbo dst, const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT ) const;
	//! Copies to the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
	void		blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT ) const;
	//! Copies from the screen from Area \a srcArea to \a dstArea using filter \a filter. \a mask allows specification of color (\c GL_COLOR_BUFFER_BIT) and/or depth(\c GL_DEPTH_BUFFER_BIT). Calls glBlitFramebufferEXT() and is subject to its constraints and coordinate system.
	void		blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter = GL_NEAREST, GLbitfield mask = GL_COLOR_BUFFER_BIT );
#endif

	//! Returns the maximum number of samples the graphics card is capable of using per pixel in MSAA for an Fbo
	static GLint	getMaxSamples();
	//! Returns the maximum number of color attachments the graphics card is capable of using for an Fbo
	static GLint	getMaxAttachments();
	
	struct Format {
	  public:
		//! Default constructor, sets the target to \c GL_TEXTURE_2D with an 8-bit color+alpha, a 24-bit depth texture, and no multisampling or mipmapping
		Format();

		//! Named Parameter Idiom.
		Format& colorInternalFormat( GLenum colorInternalFormat ) { mColorInternalFormat = colorInternalFormat; return *this; }
		Format& depthInternalFormat( GLenum depthInternalFormat ) { mDepthInternalFormat = depthInternalFormat; return *this; }
		Format& samples( int sample ) { mSamples = sample; return *this; }
		Format& coverageSamples( int coverageSamples ) { mCoverageSamples = coverageSamples; return *this; }
		Format& color( bool colorBuffer = true, int numColorBuffers = 1 );
		Format& colorTexFormat( Texture::Format format ) { mColorTextureFormat = format; return *this; }
		Format& depth( bool depthBuffer = true ) { mDepthBuffer = depthBuffer; return *this; }
		Format& depthTexture( bool depthBufferAsTexture = true ) { 	mDepthBufferAsTexture = depthBufferAsTexture; return *this; }
		Format& depthTexFormat( Texture::Format format ) { mDepthTextureFormat = format; mDepthInternalFormat = format.getInternalFormat(); return *this; }
		Format& stencil( bool stencilBuffer = true ) { mStencilBuffer = stencilBuffer; return *this; }
		
		//! Sets the GL internal format for the color buffer. Defaults to \c GL_RGBA8 (and \c GL_RGBA on OpenGL ES). Common options also include \c GL_RGB8 and \c GL_RGBA32F
		void	setColorInternalFormat( GLenum colorInternalFormat ) { mColorInternalFormat = colorInternalFormat; }
		//! Sets the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24. Common options also include \c GL_DEPTH_COMPONENT16 and \c GL_DEPTH_COMPONENT32
		void	setDepthInternalFormat( GLenum depthInternalFormat ) { mDepthInternalFormat = depthInternalFormat; }
		//! Sets the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling. Note that not all implementations support multisampling. Ignored on OpenGL ES.
		void	setSamples( int samples ) { mSamples = samples; }
		//! Sets the number of coverage samples used in CSAA-style antialiasing. Defaults to none. Note that not all implementations support CSAA, and is currenlty Windows-only Nvidia. Ignored on OpenGL ES.
		void	setCoverageSamples( int coverageSamples ) { mCoverageSamples = coverageSamples; }
		//! Enables or disables the creation of a color buffer for the FBO.. Creates multiple color attachments when \a numColorsBuffers >1, except on OpenGL ES which supports only 1.
		void	enableColorBuffer( bool colorBuffer = true, int numColorBuffers = 1 );
		//! Sets the Color Texture::format for use in the creation of the color buffer texture.
		void	setColorTexFormat( Texture::Format format ) { mColorTextureFormat = format; }
		//! Enables or disables the creation of a depth buffer for the FBO. If \a asTexture the depth buffer is created as a gl::Texture, obtainable via getDepthTexture(). Not supported on OpenGL ES.
		void	enableDepthBuffer( bool depthBuffer = true ) { mDepthBuffer = depthBuffer; }
		//! Enables or disables the creation of a depth buffer as a Texture. \a It is obtainable via getDepthTexture(). Not supported on OpenGL ES.
		void	enableDepthTexture( bool depthBufferAsTexture = true ) { mDepthBufferAsTexture = depthBufferAsTexture; }
		//! Sets the Depth Texture::format for use in the creation of the depth buffer texture.
		void	setDepthTexFormat( Texture::Format format ) { mDepthTextureFormat = format; mDepthInternalFormat = format.getInternalFormat(); }
		//! Enables or disables the creation of a stencil buffer for the FBO.. 
		void	enableStencilBuffer( bool stencilBuffer = true ) { mStencilBuffer = stencilBuffer; }

		//! Returns the GL internal format for the color buffer. Defaults to \c GL_RGBA8.
		GLenum	getColorInternalFormat() const { return mColorInternalFormat; }
		//! Returns the GL internal format for the depth buffer. Defaults to \c GL_DEPTH_COMPONENT24.
		GLenum	getDepthInternalFormat() const { return mDepthInternalFormat; }
		//! Returns the Texture::Format for the Color Texture.
		const Texture::Format& getColorTextureFormat() const { return mColorTextureFormat; }
		//! Returns the Texture::Format for the Depth Texture.
		const Texture::Format& getDepthTextureFormat() const { return mDepthTextureFormat; }
		//! Returns the number of samples used in MSAA-style antialiasing. Defaults to none, disabling multisampling.
		int		getSamples() const { return mSamples; }
		//! Returns the number of coverage samples used in CSAA-style antialiasing. Defaults to none. MSW only.
		int		getCoverageSamples() const { return mCoverageSamples; }
		//! Returns whether the FBO contains a color buffer
		bool	hasColorBuffer() const { return mNumColorBuffers > 0; }
		//! Returns the number of color buffers
		int		getNumColorBuffers() const { return mNumColorBuffers; }
		//! Returns whether the FBO contains a depth buffer
		bool	hasDepthBuffer() const { return mDepthBuffer; }
		//! Returns whether the FBO contains a depth buffer implemened as a texture. Always \c false on OpenGL ES.
		bool	hasDepthBufferTexture() const { return mDepthBufferAsTexture; }
		//! Returns whether the FBO contains a stencil buffer.
		bool	hasStencilBuffer() const { return mStencilBuffer; }
		
	  protected:
		GLenum			mColorInternalFormat, mDepthInternalFormat;
		int				mSamples;
		int				mCoverageSamples;
		bool			mDepthBuffer, mDepthBufferAsTexture, mStencilBuffer;
		int				mNumColorBuffers;
		Texture::Format	mColorTextureFormat;
		Texture::Format	mDepthTextureFormat;
		
		friend class Fbo;
	};

 protected:
	Fbo( int width, int height, Format format );
	Fbo( int width, int height, bool alpha, bool color, bool depth, bool stencil );
 
	void		init();
	bool		initMultisample( bool csaa );
	void		resolveTextures() const;
	void		updateMipmaps( int attachment ) const;
	bool		checkStatus( class FboExceptionInvalidSpecification *resultExc );

	int					mWidth, mHeight;
	Format				mFormat;
	GLuint				mId;
	GLuint				mResolveFramebufferId;
	std::map<GLenum, RenderbufferRef>	mRenderbuffers;
	std::map<GLenum, TextureRef>		mTextures;
	mutable bool				mNeedsResolve, mNeedsMipmapUpdate;
	
	static GLint		sMaxSamples, sMaxAttachments;
};

class FboException : public Exception {
};

class FboExceptionInvalidSpecification : public FboException {
  public:
	FboExceptionInvalidSpecification() : FboException() { mMessage[0] = 0; }
	FboExceptionInvalidSpecification( const std::string &message ) throw();
	
	virtual const char * what() const throw() { return mMessage; }
	
  private:	
	char	mMessage[256];
};

} } // namespace cinder::gl
