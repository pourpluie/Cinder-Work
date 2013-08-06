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

#include "cinder/gl/gl.h" // must be first
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Context.h"

using namespace std;

#if (! defined( CINDER_GLES )) || defined( CINDER_COCOA_TOUCH )
	#define SUPPORTS_MULTISAMPLE
	#if defined( CINDER_COCOA_TOUCH )
		#define glRenderbufferStorageMultisample	glRenderbufferStorageMultisampleAPPLE
		#define GL_READ_FRAMEBUFFER					GL_READ_FRAMEBUFFER_APPLE
		#define GL_DRAW_FRAMEBUFFER					GL_DRAW_FRAMEBUFFER_APPLE
	#endif
#endif

namespace cinder {
namespace gl {

GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderbuffer
RenderbufferRef Renderbuffer::create( int width, int height, GLenum internalFormat )
{
	return RenderbufferRef( new Renderbuffer( width, height, internalFormat ) );
}

RenderbufferRef Renderbuffer::create( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	return RenderbufferRef( new Renderbuffer( width, height, internalFormat, msaaSamples, coverageSamples ) );
}

Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat )
{
	init( width, height, internalFormat, 0, 0 );
}

Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	init( width, height, internalFormat, msaaSamples, coverageSamples );
}

Renderbuffer::~Renderbuffer()
{
	if( mId )
		glDeleteRenderbuffers( 1, &mId );
}

void Renderbuffer::init( int aWidth, int aHeight, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	mWidth = aWidth;
	mHeight = aHeight;
	mInternalFormat = internalFormat;
	mSamples = msaaSamples;
	mCoverageSamples = coverageSamples;
#if defined( CINDER_MSW )
	static bool csaaSupported = ( glext_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif

	glGenRenderbuffers( 1, &mId );

	if( mSamples > Fbo::getMaxSamples() )
		mSamples = Fbo::getMaxSamples();

	if( ! csaaSupported )
		mCoverageSamples = 0;

	glBindRenderbuffer( GL_RENDERBUFFER, mId );

#if defined( CINDER_MSW ) && (! defined( CINDER_GLES ))
	if( mCoverageSamples ) // create a CSAA buffer
		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
	else
#elif defined(SUPPORTS_MULTISAMPLE)
	if( mSamples ) // create a regular MSAA buffer
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
	else
#endif
		glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	mTarget = GL_TEXTURE_2D;
#if defined( CINDER_GLES )
	mColorInternalFormat = GL_RGBA;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24_OES;
	mDepthBufferAsTexture = false;
#else
	mColorInternalFormat = GL_RGBA8;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24;
	mDepthBufferAsTexture = true;
#endif
	mSamples = 0;
	mCoverageSamples = 0;
	mNumColorBuffers = 1;
	mDepthBuffer = true;
	mStencilBuffer = false;
	mMipmapping = false;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
}

void Fbo::Format::enableColorBuffer( bool colorBuffer, int numColorBuffers )
{
#if defined( CINDER_GLES )
	mNumColorBuffers = ( colorBuffer && numColorBuffers ) ? 1 : 0;
#else
	mNumColorBuffers = ( colorBuffer ) ? numColorBuffers : 0;
#endif
}

void Fbo::Format::enableDepthBuffer( bool depthBuffer, bool asTexture )
{
	mDepthBuffer = depthBuffer;
#if defined( CINDER_GLES )
	mDepthBufferAsTexture = false;
#else
	mDepthBufferAsTexture = asTexture;
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo
FboRef Fbo::create( int width, int height, Format format )
{
	return FboRef( new Fbo( width, height, format ) );
}

FboRef Fbo::create( int width, int height, bool alpha, bool color, bool depth )
{
	return FboRef( new Fbo( width, height, alpha, color, depth ) );
}

Fbo::Fbo( int width, int height, Format format )
	: mWidth( width ), mHeight( height ), mId( 0 ), mResolveFramebufferId( 0 )
{
	mFormat = format;
	init();
}

Fbo::Fbo( int width, int height, bool alpha, bool color, bool depth )
	: mWidth( width ), mHeight( height ), mId( 0 ), mResolveFramebufferId( 0 )
{
	Format format;
#if defined( CINDER_GLES )
	mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8_OES : GL_RGB8_OES;
#else
	mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8 : GL_RGB8;
#endif
	mFormat.mDepthBuffer = depth;
	mFormat.mNumColorBuffers = color ? 1 : 0;
	init();
}

Fbo::~Fbo()
{
	if( mId )
		glDeleteFramebuffers( 1, &mId );
	if( mResolveFramebufferId )
		glDeleteFramebuffers( 1, &mResolveFramebufferId );
}

void Fbo::init()
{
#if defined( CINDER_MSW ) && ( ! defined( CINDER_GLES ) )
	static bool csaaSupported = ( glext_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif
	bool useCSAA = csaaSupported && ( mFormat.mCoverageSamples > mFormat.mSamples );
	bool useMSAA = ( mFormat.mCoverageSamples > 0 ) || ( mFormat.mSamples > 0 );
	if( useCSAA )
		useMSAA = false;

	// allocate the framebuffer itself
	glGenFramebuffers( 1, &mId );
	FramebufferScope fbScp( GL_FRAMEBUFFER, mId );

	Texture::Format textureFormat;
	textureFormat.setTarget( getTarget() );
	textureFormat.setInternalFormat( getFormat().getColorInternalFormat() );
	textureFormat.setWrap( mFormat.mWrapS, mFormat.mWrapT );
	textureFormat.setMinFilter( mFormat.mMinFilter );
	textureFormat.setMagFilter( mFormat.mMagFilter );
	textureFormat.enableMipmapping( getFormat().hasMipMapping() );

	// allocate the color buffers
	for( int c = 0; c < mFormat.mNumColorBuffers; ++c ) {
		mColorTextures.push_back( Texture::create( mWidth, mHeight, textureFormat ) );
	}
	
#if ! defined( CINDER_GLES )	
	if( mFormat.mNumColorBuffers == 0 ) { // no color
		glDrawBuffer( GL_NONE );
		glReadBuffer( GL_NONE );	
	}
#endif
		
	if( ( ( ! useCSAA ) && ( ! useMSAA ) ) || ( ! initMultisample( useCSAA ) ) ) { // if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer
		vector<GLenum> drawBuffers;
		for( size_t c = 0; c < mColorTextures.size(); ++c ) {
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, getTarget(), mColorTextures[c]->getId(), 0 );
			drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
		}
#if ! defined( CINDER_GLES )
		if( ! drawBuffers.empty() )
			glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
#endif

		// allocate and attach depth texture
		if( mFormat.mDepthBuffer ) {
			if( mFormat.mDepthBufferAsTexture ) {
	#if ! defined( CINDER_GLES )			
				GLuint depthTextureId;
				glGenTextures( 1, &depthTextureId );
				glBindTexture( getTarget(), depthTextureId );
				glTexImage2D( getTarget(), 0, getFormat().getDepthInternalFormat(), mWidth, mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
				glTexParameteri( getTarget(), GL_TEXTURE_MIN_FILTER, mFormat.mMinFilter );
				glTexParameteri( getTarget(), GL_TEXTURE_MAG_FILTER, mFormat.mMagFilter );
				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_S, mFormat.mWrapS );
				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_T, mFormat.mWrapT );
// TODO: what is the replacement
//				glTexParameteri( getTarget(), GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
				mDepthTexture = Texture::create( getTarget(), depthTextureId, mWidth, mHeight, true );

				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, getTarget(), mDepthTexture->getId(), 0 );
//glFramebufferTexture2DEXT( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, getTarget(), mDepthTexture.getId(), 0 );
	#else
		throw FboExceptionInvalidSpecification( "Depth as texture not supported on OpenGL ES" ); // this should never fire in OpenGL ES
	#endif
			}
			else if( mFormat.mDepthBuffer ) { // implement depth buffer as RenderBuffer
				mDepthRenderbuffer = Renderbuffer::create( mWidth, mHeight, mFormat.getDepthInternalFormat() );
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer->getId() );
			}
		}

		FboExceptionInvalidSpecification exc;
		if( ! checkStatus( &exc ) ) { // failed creation; throw
			throw exc;
		}
	}
	
	mNeedsResolve = false;
	mNeedsMipmapUpdate = false;
}

bool Fbo::initMultisample( bool csaa )
{
	auto ctx = context();

	glGenFramebuffers( 1, &mResolveFramebufferId );
	ctx->bindFramebuffer( GL_FRAMEBUFFER, mResolveFramebufferId ); 
	
	// bind all of the color buffers to the resolve FB's attachment points
	vector<GLenum> drawBuffers;
	for( size_t c = 0; c < mColorTextures.size(); ++c ) {
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, getTarget(), mColorTextures[c]->getId(), 0 );
		drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
	}

#if ! defined( CINDER_GLES )
	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
#endif

	// see if the resolve buffer is ok
	FboExceptionInvalidSpecification ignoredException;
	if( ! checkStatus( &ignoredException ) )
		return false;

	ctx->bindFramebuffer( GL_FRAMEBUFFER, mId );

	if( mFormat.mSamples > getMaxSamples() ) {
		mFormat.mSamples = getMaxSamples();
	}

	// setup the multisampled color renderbuffers
	for( int c = 0; c < mFormat.mNumColorBuffers; ++c ) {
		GLint colorInternalFormat = mFormat.mColorInternalFormat;
#if defined( CINDER_GLES )
		// GL_RGBA & GL_RGB work as an internalFormat for textures but not RenderBuffers on ES
		if( colorInternalFormat == GL_RGBA )
			colorInternalFormat = GL_RGBA8_OES;
		else if( colorInternalFormat == GL_RGB )
			colorInternalFormat = GL_RGB8_OES;
#endif
		mMultisampleColorRenderbuffers.push_back( Renderbuffer::create( mWidth, mHeight, colorInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples ) );

		// attach the multisampled color buffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER, mMultisampleColorRenderbuffers.back()->getId() );
	}

#if ! defined( CINDER_GLES )	
	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
#endif

	if( mFormat.mDepthBuffer ) {
		// create the multisampled depth Renderbuffer
		mMultisampleDepthRenderbuffer = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );

		// attach the depth Renderbuffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mMultisampleDepthRenderbuffer->getId() );
	}

	// see if the primary framebuffer turned out ok
	return checkStatus( &ignoredException );
}

TextureRef Fbo::getTexture( int attachment )
{
	resolveTextures();
	updateMipmaps( attachment );
	return mColorTextures[attachment];
}

TextureRef Fbo::getDepthTexture()
{
	return mDepthTexture;
}

void Fbo::bindTexture( int textureUnit, int attachment )
{
	resolveTextures();
	mColorTextures[attachment]->bind( textureUnit );
	updateMipmaps( attachment );
}

void Fbo::unbindTexture()
{
	glBindTexture( getTarget(), 0 );
}

void Fbo::bindDepthTexture( int textureUnit )
{
	mDepthTexture->bind( textureUnit );
}

void Fbo::resolveTextures() const
{
	if( ! mNeedsResolve )
		return;

#if defined( SUPPORTS_MULTISAMPLE ) && defined( CINDER_GLES )
	// iOS-specific multisample resolution code
	if ( mResolveFramebufferId ) {
		FramebufferScope fbScp;
		auto ctx = context();
		
		ctx->bindFramebuffer( GL_READ_FRAMEBUFFER_APPLE, mId );
		ctx->bindFramebuffer( GL_DRAW_FRAMEBUFFER_APPLE, mResolveFramebufferId );
		
		glResolveMultisampleFramebufferAPPLE();
	}
#elif defined( SUPPORTS_MULTISAMPLE )
	// if this FBO is multisampled, resolve it, so it can be displayed
	if ( mResolveFramebufferId ) {
		FramebufferScope fbScp;
		auto ctx = context();

		ctx->bindFramebuffer( GL_READ_FRAMEBUFFER, mId );
		ctx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mResolveFramebufferId );
		
		for( size_t c = 0; c < mColorTextures.size(); ++c ) {
			glDrawBuffer( GL_COLOR_ATTACHMENT0 + c );
			glReadBuffer( GL_COLOR_ATTACHMENT0 + c );
			GLbitfield bitfield = GL_COLOR_BUFFER_BIT;
			if( mDepthTexture )
				bitfield |= GL_DEPTH_BUFFER_BIT;
			glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, bitfield, GL_NEAREST );
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		vector<GLenum> drawBuffers;
		for( size_t c = 0; c < mColorTextures.size(); ++c )
			drawBuffers.push_back( GL_COLOR_ATTACHMENT0 + c );
		ctx->bindFramebuffer( GL_FRAMEBUFFER, mId );
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
	}
#endif

	mNeedsResolve = false;
}

void Fbo::updateMipmaps( int attachment ) const
{
	if( ! mNeedsMipmapUpdate )
		return;
	else {
		TextureBindScope textureBind( getTarget(), mColorTextures[attachment]->getId() );
		glGenerateMipmap( getTarget() );
	}

	mNeedsMipmapUpdate = false;
}

void Fbo::bindFramebuffer()
{
	auto ctx = context();
	ctx->bindFramebuffer( GL_FRAMEBUFFER, mId );
	if( mResolveFramebufferId ) {
		mNeedsResolve = true;
	}
	if( mFormat.hasMipMapping() ) {
		mNeedsMipmapUpdate = true;
	}
}

void Fbo::unbindFramebuffer()
{
	context()->unbindFramebuffer();
}

bool Fbo::checkStatus( FboExceptionInvalidSpecification *resultExc )
{
	GLenum status;
	status = (GLenum) glCheckFramebufferStatus( GL_FRAMEBUFFER );
	switch( status ) {
		case GL_FRAMEBUFFER_COMPLETE:
		break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			*resultExc = FboExceptionInvalidSpecification( "Unsupported framebuffer format" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing attachment" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: duplicate attachment" );
		return false;
#if ! defined( CINDER_GLES )
/*		case GL_CONST_SUFFIX(GL_FRAMEBUFFER_INCOMPLETE_FORMATS):
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: attached images must have same format" );
		return false;*/
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing draw buffer" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing read buffer" );
		return false;
#else
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: attached images must have same dimensions" );
		return false;
#endif
#if defined( CINDER_COCOA_TOUCH )
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: not all attached images have the same number of samples" );
		return false;
#endif
		default:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer invalid: unknown reason" );
		return false;
    }
	
    return true;
}

GLint Fbo::getMaxSamples()
{
#if ! defined( CINDER_GLES )
	if( sMaxSamples < 0 ) {
		glGetIntegerv( GL_MAX_SAMPLES, &sMaxSamples);
	}
	
	return sMaxSamples;
#elif defined( CINDER_COCOA_TOUCH )
	if( sMaxSamples < 0 )
		glGetIntegerv( GL_MAX_SAMPLES_APPLE, &sMaxSamples);
	
	return sMaxSamples;
#else
	return 0;
#endif
}

GLint Fbo::getMaxAttachments()
{
#if ! defined( CINDER_GLES )
	if( sMaxAttachments < 0 ) {
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &sMaxAttachments );
	}
	
	return sMaxAttachments;
#else
	return 1;
#endif
}

#if ! defined( CINDER_GLES )
void Fbo::blitTo( Fbo dst, const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	FramebufferScope fbScp;
	auto ctx = gl::context();
	
	ctx->bindFramebuffer( GL_READ_FRAMEBUFFER, mId );
	ctx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, dst.getId() );

	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	FramebufferScope fbScp;
	auto ctx = gl::context();

	ctx->bindFramebuffer( GL_READ_FRAMEBUFFER, mId );
	ctx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );		
	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
{
	FramebufferScope fbScp;
	auto ctx = gl::context();

	ctx->bindFramebuffer( GL_READ_FRAMEBUFFER, GL_NONE );
	ctx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mId );		
	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}
#endif

FboExceptionInvalidSpecification::FboExceptionInvalidSpecification( const string &message ) throw()
	: FboException()
{
	strncpy( mMessage, message.c_str(), 255 );
}

#undef GL_SUFFIX

} } // namespace cinder::gl
