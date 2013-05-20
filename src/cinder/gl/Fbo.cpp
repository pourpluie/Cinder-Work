#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

using namespace std;

namespace cinder { namespace gl {
	
//! Creates a FBO
FboRef Fbo::create( int32_t width, int32_t height, int32_t msaaSamples )
{
	return FboRef( new Fbo( width, height, msaaSamples ) );
}

Fbo::Fbo( int32_t width, int32_t height, int32_t msaaSamples )
{
	
	// Set properties
	mHeight = height;
	mMsaaSamples = msaaSamples;
	mWidth = width;
	
	// Create and bind frame buffer
	glGenFramebuffers( 1, &mFrameBuffer );
	glBindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );
	
	// Multisampling buffer
	if ( mMsaaSamples > 0 ) {
		glGenFramebuffers( 1, &mResolveBuffer );
	}
	
	// Create and bind color render buffer
	glGenRenderbuffers( 1, &mColorBuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, mColorBuffer );
	if ( mMsaaSamples > 0 ) {
#if defined( CINDER_GLES )
		glRenderbufferStorageMultisampleAPPLE( GL_RENDERBUFFER, mMsaaSamples, GL_RGBA8_OES, mWidth, mHeight );
#else
//		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mMsaaSamples, GL_RGBA8_OES, mWidth, mHeight );
#endif
	} else {
#if defined( CINDER_GLES )
		glRenderbufferStorage( GL_RENDERBUFFER, GL_RGBA8_OES, mWidth, mHeight );
#else
		glRenderbufferStorage( GL_RENDERBUFFER, GL_RGBA8, mWidth, mHeight );
#endif
	}
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mColorBuffer );
	
	// Create color texture
	Texture::Format format;
	format.setTarget( GL_TEXTURE_2D );
	format.setInternalFormat( GL_RGBA );
	format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	format.setMagFilter( GL_LINEAR );
	format.setMinFilter( GL_LINEAR );
	mColorTexture = gl::Texture::create( mWidth, mHeight, format );
	
	// Create and bind depth render buffer
	glGenRenderbuffers( 1, &mDepthBuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, mDepthBuffer );
	if ( mMsaaSamples > 0 ) {
#if defined( CINDER_GLES )
		glRenderbufferStorageMultisampleAPPLE( GL_RENDERBUFFER, mMsaaSamples, GL_DEPTH_COMPONENT16, mWidth, mHeight );
#else
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mMsaaSamples, GL_DEPTH_COMPONENT16, mWidth, mHeight );
#endif
	} else {
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mWidth, mHeight );
	}
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );
	
	// Tell frame buffer to render to texture
	if ( mMsaaSamples > 0 ) {
		glBindFramebuffer( GL_FRAMEBUFFER, mResolveBuffer );
	}
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture->getId(), 0 );
	
	checkStatus();
	
}

Fbo::~Fbo()
{
	if ( mFrameBuffer ) {
		glDeleteFramebuffers(1, &mFrameBuffer );
		mFrameBuffer = 0;
	}
	if ( mResolveBuffer ) {
		glDeleteFramebuffers(1, &mResolveBuffer );
		mResolveBuffer = 0;
	}
	if ( mColorBuffer ) {
		glDeleteRenderbuffers( 1, &mColorBuffer );
		mColorBuffer = 0;
	}
	if ( mDepthBuffer ) {
		glDeleteRenderbuffers( 1, &mDepthBuffer );
		mDepthBuffer = 0;
	}
}

void Fbo::checkStatus()
{
	GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER ) ;
	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		app::console() << "Unable to bind frame buffer\n";
	}
}
void Fbo::checkStatus() const
{
	GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER ) ;
	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		app::console() << "Unable to bind frame buffer\n";
	}
}



void Fbo::bindFramebuffer()
{
	glBindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );
}
void Fbo::bindFramebuffer() const
{
	glBindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );
}
void Fbo::unbindFramebuffer()
{
	if ( mMsaaSamples > 0 ) {
		const GLenum discards[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
#if defined( CINDER_GLES )
		glBindFramebuffer( GL_READ_FRAMEBUFFER_APPLE, mFrameBuffer );
		glResolveMultisampleFramebufferAPPLE();
		glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER_APPLE, 2, discards );
#else
//		glBindFramebuffer( GL_READ_FRAMEBUFFER, mFrameBuffer );
//		glResolveMultisampleFramebuffer();
//		glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER, 2, discards );
#endif
	}
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}
void Fbo::unbindFramebuffer() const
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	if ( mMsaaSamples > 0 ) {
#if defined( CINDER_GLES )
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER_APPLE, mResolveBuffer );
		glBindFramebuffer( GL_READ_FRAMEBUFFER_APPLE, mFrameBuffer );
		glResolveMultisampleFramebufferAPPLE();
		const GLenum discards[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
		glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER_APPLE, 2, discards );
#else
//		glBindFramebuffer( GL_DRAW_FRAMEBUFFER_APPLE, mResolveBuffer );
//		glBindFramebuffer( GL_READ_FRAMEBUFFER_APPLE, mFrameBuffer );
//		glResolveMultisampleFramebufferAPPLE();
//		const GLenum discards[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
//		glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER_APPLE, 2, discards );
#endif
	}
}



void Fbo::bindTexture( int32_t textureUnit )
{
	mColorTexture->bind( textureUnit );
}
void Fbo::bindTexture( int32_t textureUnit ) const
{
	mColorTexture->bind( textureUnit );
}
void Fbo::unbindTexture()
{
	mColorTexture->unbind();
}
void Fbo::unbindTexture() const
{
	mColorTexture->unbind();
}

TextureRef Fbo::getTexture()
{
	return mColorTexture;
}
TextureRef Fbo::getTexture() const
{
	return mColorTexture;
}



int32_t Fbo::getHeight()
{
	return mHeight;
}
int32_t Fbo::getHeight() const
{
	return mHeight;
}
int32_t Fbo::getWidth()
{
	return mWidth;
}
int32_t Fbo::getWidth() const
{
	return mWidth;
}

ci::Area Fbo::getBounds()
{
	return Area( 0, 0, mWidth, mHeight );
}
ci::Area Fbo::getBounds() const
{
	return Area( 0, 0, mWidth, mHeight );
}
ci::Vec2i Fbo::getSize()
{
	return Vec2i( mWidth, mHeight );
}
ci::Vec2i Fbo::getSize() const
{
	return Vec2i( mWidth, mHeight );
}
    
} } // namespace cinder::gl

