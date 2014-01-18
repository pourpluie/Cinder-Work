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

// Relevant OpenGL Extensions
// * OES_packed_depth_stencil http://www.khronos.org/registry/gles/extensions/OES/OES_packed_depth_stencil.txt
//	 * DEPTH_STENCIL_OES - <format> param of Tex(Sub)Image2D, the <internalformat> param of TexImage2D
//	 * UNSIGNED_INT_24_8_OES - <type> param of Tex(Sub)Image2D
//	 * DEPTH24_STENCIL8_OES - <internalformat> param of RenderbufferStorage
// * EXT_packed_depth_stencil http://www.opengl.org/registry/specs/EXT/packed_depth_stencil.txt
// * http://www.khronos.org/registry/gles/extensions/ANGLE/ANGLE_framebuffer_multisample.txt

#include "cinder/gl/gl.h" // must be first
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Context.h"

using namespace std;

#if (! defined( CINDER_GLES )) || defined( CINDER_COCOA_TOUCH ) || defined( CINDER_GL_ANGLE )
	#define SUPPORTS_MULTISAMPLE
	#if defined( CINDER_COCOA_TOUCH )
		#define glRenderbufferStorageMultisample	glRenderbufferStorageMultisampleAPPLE
		#define glResolveMultisampleFramebuffer		glResolveMultisampleFramebufferAPPLE
		#define GL_READ_FRAMEBUFFER					GL_READ_FRAMEBUFFER_APPLE
		#define GL_DRAW_FRAMEBUFFER					GL_DRAW_FRAMEBUFFER_APPLE
	#elif defined( CINDER_GL_ANGLE )
		#define glRenderbufferStorageMultisample	glRenderbufferStorageMultisampleANGLE
		#define GL_READ_FRAMEBUFFER					GL_READ_FRAMEBUFFER_ANGLE
		#define GL_DRAW_FRAMEBUFFER					GL_DRAW_FRAMEBUFFER_ANGLE
	#endif
#endif

#if ! defined( CINDER_GLES )
	#define MAX_COLOR_ATTACHMENT	GL_COLOR_ATTACHMENT15
#else
	#define MAX_COLOR_ATTACHMENT	GL_COLOR_ATTACHMENT0
#endif

namespace cinder {
namespace gl {

GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderbuffer
RenderbufferRef Renderbuffer::create( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	return RenderbufferRef( new Renderbuffer( width, height, internalFormat, msaaSamples, coverageSamples ) );
}

Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	mWidth = width;
	mHeight = height;
	mInternalFormat = internalFormat;
	mSamples = msaaSamples;
	mCoverageSamples = coverageSamples;
#if defined( CINDER_MSW ) && ( ! defined( CINDER_GL_ANGLE ) )
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

#if defined( SUPPORTS_MULTISAMPLE )
  #if defined( CINDER_MSW )  && ( ! defined( CINDER_GLES ) )
	if( mCoverageSamples ) // create a CSAA buffer
		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
	else
  #endif
		if( mSamples )
			glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
		else
			glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
#elif defined( CINDER_GLES )
	// this is gross, but GL_RGBA & GL_RGB are not suitable internal formats for Renderbuffers. We know what you meant though.
	if( mInternalFormat == GL_RGBA )
		mInternalFormat = GL_RGBA8_OES;
	else if( mInternalFormat == GL_RGB )
		mInternalFormat = GL_RGB8_OES;
	else if( mInternalFormat == GL_DEPTH_COMPONENT )
		mInternalFormat = GL_DEPTH_COMPONENT24_OES;
		
	if( mSamples )
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
	else
		glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
#endif	
}

Renderbuffer::~Renderbuffer()
{
	if( mId )
		glDeleteRenderbuffers( 1, &mId );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	mColorTextureFormat = getDefaultColorTextureFormat( true );
	mColorBufferInternalFormat = getDefaultColorInternalFormat( true );
	mColorTexture = true;
	mColorBuffer = false;
	
	mDepthTextureFormat = getDefaultDepthTextureFormat();
	mDepthBufferInternalFormat = getDefaultDepthInternalFormat();
	mDepthBuffer = true;
	mDepthTexture = false;
	
	mSamples = 0;
	mCoverageSamples = 0;
	mStencilBuffer = false;
}

GLint Fbo::Format::getDefaultColorInternalFormat( bool alpha )
{
#if defined( CINDER_GLES )
	return GL_RGBA;
#else
	return GL_RGBA8;
#endif
}

GLint Fbo::Format::getDefaultDepthInternalFormat()
{
#if defined( CINDER_GLES )
	return GL_DEPTH_COMPONENT24_OES;
#else
	return GL_DEPTH_COMPONENT24;
#endif
}

Texture::Format	Fbo::Format::getDefaultColorTextureFormat( bool alpha )
{
	if( alpha )
		return Texture::Format().internalFormat( GL_RGBA ).pixelDataFormat( GL_RGBA ).pixelDataType( GL_UNSIGNED_BYTE );
	else
		return Texture::Format().internalFormat( GL_RGB ).pixelDataFormat( GL_RGB ).pixelDataType( GL_UNSIGNED_BYTE );
}

Texture::Format	Fbo::Format::getDefaultDepthTextureFormat()
{
#if defined( CINDER_GLES )
	return Texture::Format().internalFormat( GL_DEPTH_COMPONENT ).pixelDataFormat( GL_DEPTH_COMPONENT ).pixelDataType( GL_UNSIGNED_SHORT );
#else
	return Texture::Format().internalFormat( GL_DEPTH_COMPONENT24 ).pixelDataFormat( GL_DEPTH_COMPONENT ).pixelDataType( GL_FLOAT );
#endif
}

// Returns the +stencil complement of a given internalFormat; ie GL_DEPTH_COMPONENT24 -> GL_DEPTH_COMPONENT24
void Fbo::Format::getDepthStencilFormats( GLint depthInternalFormat, GLint *resultInternalFormat, GLenum *resultPixelDataType )
{
	switch( depthInternalFormat ) {
#if defined( CINDER_GLES )
		case GL_DEPTH24_STENCIL8_OES:
			*resultInternalFormat = GL_DEPTH_STENCIL_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
		case GL_DEPTH_STENCIL_OES:
			*resultInternalFormat = GL_DEPTH_STENCIL_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
		case GL_DEPTH_COMPONENT:
			*resultInternalFormat = GL_DEPTH_STENCIL_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
#else
		case GL_DEPTH24_STENCIL8:
			*resultInternalFormat = GL_DEPTH24_STENCIL8; *resultPixelDataType = GL_UNSIGNED_INT_24_8;
		case GL_DEPTH32F_STENCIL8:
			*resultInternalFormat = GL_DEPTH32F_STENCIL8; *resultPixelDataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
		case GL_DEPTH_COMPONENT24:
			*resultInternalFormat = GL_DEPTH24_STENCIL8; *resultPixelDataType = GL_UNSIGNED_INT_24_8;
		break;
		case GL_DEPTH_COMPONENT32F:
			*resultInternalFormat = GL_DEPTH32F_STENCIL8; *resultPixelDataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
#endif		
	}
}

Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, RenderbufferRef buffer, RenderbufferRef multisampleBuffer )
{
	mAttachmentsBuffer[attachmentPoint] = buffer;
	mAttachmentsMultisampleBuffer[attachmentPoint] = multisampleBuffer;
	mAttachmentsTexture.erase( attachmentPoint );
	return *this;
}

Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, Texture2dRef texture, RenderbufferRef multisampleBuffer )
{
	mAttachmentsTexture[attachmentPoint] = texture;
	mAttachmentsMultisampleBuffer[attachmentPoint] = multisampleBuffer;
	mAttachmentsBuffer.erase( attachmentPoint );
	return *this;
}

void Fbo::Format::removeAttachment( GLenum attachmentPoint )
{
	mAttachmentsBuffer.erase( attachmentPoint );
	mAttachmentsMultisampleBuffer.erase( attachmentPoint );	
	mAttachmentsTexture.erase( attachmentPoint );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo
FboRef Fbo::create( int width, int height, const Format &format )
{
	return FboRef( new Fbo( width, height, format ) );
}

FboRef Fbo::create( int width, int height, bool alpha, bool depth, bool stencil )
{
	return FboRef( new Fbo( width, height, alpha, depth, stencil ) );
}

Fbo::Fbo( int width, int height, const Format &format )
	: mWidth( width ), mHeight( height ), mFormat( format ), mId( 0 ), mMultisampleFramebufferId( 0 )
{
	init();
}

Fbo::Fbo( int width, int height, bool alpha, bool depth, bool stencil )
	: mWidth( width ), mHeight( height ), mId( 0 ), mMultisampleFramebufferId( 0 )
{
	mFormat.mColorTextureFormat = Format::getDefaultColorTextureFormat( alpha );
	mFormat.mDepthBuffer = depth;
	mFormat.mStencilBuffer = stencil;
	
	init();
}

Fbo::~Fbo()
{
	if( mId )
		glDeleteFramebuffers( 1, &mId );
	if( mMultisampleFramebufferId )
		glDeleteFramebuffers( 1, &mMultisampleFramebufferId );
}

void Fbo::initMultisamplingSettings( bool *useMsaa, bool *useCsaa )
{
#if defined( CINDER_MSW ) && ( ! defined( CINDER_GLES ) )
	static bool csaaSupported = ( glext_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif
	*useCsaa = csaaSupported && ( mFormat.mCoverageSamples > mFormat.mSamples );
	*useMsaa = ( mFormat.mCoverageSamples > 0 ) || ( mFormat.mSamples > 0 );
	if( *useCsaa )
		*useMsaa = false;

	if( mFormat.mSamples > getMaxSamples() )
		mFormat.mSamples = getMaxSamples();
}

// Iterate the Format's requested attachments and create any we don't already have attachments for
// After calling this function, mFormat's mAttachmentsTexture and mAttachmentsBuffer should be 1:1 with what will be instantiated
void Fbo::initFormatAttachments()
{
	// Create the default color attachment if there's not already something on GL_COLOR_ATTACHMENT0
	bool preexistingColorAttachment = mFormat.mAttachmentsTexture.count( GL_COLOR_ATTACHMENT0 ) || mFormat.mAttachmentsBuffer.count( GL_COLOR_ATTACHMENT0 );
	if( mFormat.mColorBuffer && ( ! preexistingColorAttachment ) ) {
		mFormat.mAttachmentsBuffer[GL_COLOR_ATTACHMENT0] = Renderbuffer::create( mWidth, mHeight, mFormat.mColorBufferInternalFormat );
	}
	else if( mFormat.mColorTexture && ( ! preexistingColorAttachment ) ) {
		mFormat.mAttachmentsTexture[GL_COLOR_ATTACHMENT0] = Texture::create( mWidth, mHeight, mFormat.mColorTextureFormat );
	}
	
	// Create the default depth(+stencil) attachment if there's not already something on GL_DEPTH_ATTACHMENT || GL_DEPTH_STENCIL_ATTACHMENT
#if defined( CINDER_GLES )
	bool preexistingDepthAttachment = mFormat.mAttachmentsTexture.count( GL_DEPTH_ATTACHMENT ) || mFormat.mAttachmentsBuffer.count( GL_DEPTH_ATTACHMENT );
#else
	bool preexistingDepthAttachment = mFormat.mAttachmentsTexture.count( GL_DEPTH_ATTACHMENT ) || mFormat.mAttachmentsBuffer.count( GL_DEPTH_ATTACHMENT )
										|| mFormat.mAttachmentsTexture.count( GL_DEPTH_STENCIL_ATTACHMENT ) || mFormat.mAttachmentsBuffer.count( GL_DEPTH_STENCIL_ATTACHMENT );
#endif
	if( mFormat.mDepthBuffer && ( ! preexistingDepthAttachment ) ) {
		if( mFormat.mStencilBuffer ) {
			GLint internalFormat;
			GLenum pixelDataType;
			Format::getDepthStencilFormats( mFormat.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
			RenderbufferRef depthStencilBuffer = Renderbuffer::create( mWidth, mHeight, internalFormat );
#if defined( CINDER_GLES )
			mFormat.mAttachmentsBuffer[GL_DEPTH_ATTACHMENT] = depthStencilBuffer;
			mFormat.mAttachmentsBuffer[GL_STENCIL_ATTACHMENT] = depthStencilBuffer;
#else
			mFormat.mAttachmentsBuffer[GL_DEPTH_STENCIL_ATTACHMENT] = Renderbuffer::create( mWidth, mHeight, internalFormat );
#endif
		}
		else {
			mFormat.mAttachmentsBuffer[GL_DEPTH_ATTACHMENT] = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthBufferInternalFormat );
		}
	}
	else if( mFormat.mDepthTexture && ( ! preexistingDepthAttachment ) ) {
		if( mFormat.mStencilBuffer ) {
			// first determine formats for depth texture with stencil added
			GLint stencilInternalFormat;
			GLenum stencilPixelDataType;
			Format::getDepthStencilFormats( mFormat.mDepthTextureFormat.getInternalFormat(), &stencilInternalFormat, &stencilPixelDataType );
			Texture::Format textureFormat = mFormat.mDepthTextureFormat;
			textureFormat.setInternalFormat( stencilInternalFormat );
			textureFormat.setPixelDataType( stencilPixelDataType );
#if defined( CINDER_GLES )
			textureFormat.setPixelDataFormat( GL_DEPTH_STENCIL_OES );
#else
			textureFormat.setPixelDataFormat( GL_DEPTH_STENCIL );
#endif			

			TextureRef depthStencilTexture = Texture::create( mWidth, mHeight, textureFormat );
#if defined( CINDER_GLES )			
			mFormat.mAttachmentsTexture[GL_DEPTH_ATTACHMENT] = depthStencilTexture;
			mFormat.mAttachmentsTexture[GL_STENCIL_ATTACHMENT] = depthStencilTexture;
#else
			mFormat.mAttachmentsTexture[GL_DEPTH_STENCIL_ATTACHMENT] = depthStencilTexture;
#endif
		}
		else {
			mFormat.mAttachmentsTexture[GL_DEPTH_ATTACHMENT] = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
		}		
	}
	else if( mFormat.mStencilBuffer ) { // stencil only
		GLint internalFormat = GL_STENCIL_INDEX8;
		RenderbufferRef stencilBuffer = Renderbuffer::create( mWidth, mHeight, internalFormat );
		mFormat.mAttachmentsBuffer[GL_STENCIL_ATTACHMENT] = stencilBuffer;
	}
}

// call glDrawBuffers against all color attachments
void Fbo::setAllDrawBuffers()
{
#if ! defined( CINDER_GLES )
	vector<GLenum> drawBuffers;
	for( const auto &bufferAttachment : mAttachmentsBuffer )
		if( bufferAttachment.first >= GL_COLOR_ATTACHMENT0 && bufferAttachment.first <= MAX_COLOR_ATTACHMENT )
			drawBuffers.push_back( bufferAttachment.first );

	for( const auto &textureAttachment : mAttachmentsTexture )
		if( textureAttachment.first >= GL_COLOR_ATTACHMENT0 && textureAttachment.first <= MAX_COLOR_ATTACHMENT )
			drawBuffers.push_back( textureAttachment.first );

	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
	else
		glDrawBuffer( GL_NONE );
#endif
}

void Fbo::init()
{
	// allocate the framebuffer itself
	glGenFramebuffers( 1, &mId );
	FramebufferScope fbScp( GL_FRAMEBUFFER, mId );

	// After calling this function, mFormat's mAttachmentsTexture and mAttachmentsBuffer should be 1:1 with what will be instantiated
	initFormatAttachments();

	// determine multisampling settings
	bool useMsaa, useCsaa;
	initMultisamplingSettings( &useMsaa, &useCsaa );

	if( useCsaa || useMsaa )
		initMultisample( useCsaa );
	
	// attach Renderbuffers
	for( auto &bufferAttachment : mFormat.mAttachmentsBuffer ) {
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, bufferAttachment.first, GL_RENDERBUFFER, bufferAttachment.second->getId() );
		mAttachmentsBuffer[bufferAttachment.first] = bufferAttachment.second;
	}
	
	// attach Textures
	for( auto &textureAttachment : mFormat.mAttachmentsTexture ) {
		glFramebufferTexture2D( GL_FRAMEBUFFER, textureAttachment.first, textureAttachment.second->getTarget(), textureAttachment.second->getId(), 0 );
		mAttachmentsTexture[textureAttachment.first] = textureAttachment.second;
	}
	
	setAllDrawBuffers();
			
	FboExceptionInvalidSpecification exc;
	if( ! checkStatus( &exc ) ) { // failed creation; throw
		throw exc;
	}
	
	mNeedsResolve = false;
	mNeedsMipmapUpdate = false;
}

void Fbo::initMultisample( bool csaa )
{
	glGenFramebuffers( 1, &mMultisampleFramebufferId );
	FramebufferScope fbScp( GL_FRAMEBUFFER, mMultisampleFramebufferId );

	// create mirror Multisample Renderbuffers for any Buffer attachments in the primary FBO
	for( auto &bufferAttachment : mFormat.mAttachmentsBuffer ) {
		auto existing = mFormat.mAttachmentsMultisampleBuffer.find( bufferAttachment.first );
		// if there's no existing multisample buffer attachment or it's null
		if( existing == mFormat.mAttachmentsMultisampleBuffer.end() || ( ! existing->second ) )
			mFormat.mAttachmentsMultisampleBuffer[bufferAttachment.first] = Renderbuffer::create( mWidth, mHeight, bufferAttachment.second->getInternalFormat(), mFormat.mSamples, mFormat.mCoverageSamples );
	}

	// create mirror Multisample Renderbuffers for any Texture attachments in the primary FBO
	for( auto &bufferAttachment : mFormat.mAttachmentsTexture ) {
		auto existing = mFormat.mAttachmentsMultisampleBuffer.find( bufferAttachment.first );
		// if there's no existing multisample buffer attachment or it's null
		if( existing == mFormat.mAttachmentsMultisampleBuffer.end() || ( ! existing->second ) )
			mFormat.mAttachmentsMultisampleBuffer[bufferAttachment.first] = Renderbuffer::create( mWidth, mHeight, bufferAttachment.second->getInternalFormat(), mFormat.mSamples, mFormat.mCoverageSamples );
	}

	// attach Multisample Renderbuffers as requested by the Format
	for( auto &bufferAttachment : mFormat.mAttachmentsMultisampleBuffer ) {
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, bufferAttachment.first, GL_RENDERBUFFER, bufferAttachment.second->getId() );
		mAttachmentsMultisampleBuffer[bufferAttachment.first] = bufferAttachment.second;
	}

	FboExceptionInvalidSpecification ignoredException;
	if( ! checkStatus( &ignoredException ) ) { // failure
		mAttachmentsMultisampleBuffer.clear();
		glDeleteFramebuffers( 1, &mMultisampleFramebufferId );
		mMultisampleFramebufferId = 0;
	}
}

Texture2dRef Fbo::getColorTexture()
{
	auto attachedTextureIt = mAttachmentsTexture.find( GL_COLOR_ATTACHMENT0 );
	if( attachedTextureIt != mAttachmentsTexture.end() && ( typeid(*attachedTextureIt->second) == typeid(Texture2d) ) ) {
		resolveTextures();
		updateMipmaps( GL_COLOR_ATTACHMENT0 );
		return static_pointer_cast<Texture2d>( attachedTextureIt->second );
	}
	else
		return Texture2dRef();
}

Texture2dRef Fbo::getDepthTexture()
{
	TextureBaseRef result;
	
	// search for a depth attachment
	auto attachedTextureIt = mAttachmentsTexture.find( GL_DEPTH_ATTACHMENT );
	if( attachedTextureIt != mAttachmentsTexture.end() )
		result = attachedTextureIt->second;
#if ! defined( CINDER_GLES )
	else { // search for a depth+stencil attachment
		attachedTextureIt = mAttachmentsTexture.find( GL_DEPTH_STENCIL_ATTACHMENT );
		if( attachedTextureIt != mAttachmentsTexture.end() )
			result = attachedTextureIt->second;
	}
#endif
	if( result && ( typeid(*result) == typeid(Texture2d) ) ) {
		resolveTextures();
		updateMipmaps( attachedTextureIt->first );
        return static_pointer_cast<Texture2d>( result );
	}
	else
		return Texture2dRef();
}

TextureBaseRef Fbo::getTexture( GLenum attachment )
{
	auto attachedTextureIt = mAttachmentsTexture.find( attachment );
	if( attachedTextureIt != mAttachmentsTexture.end() ) {
		resolveTextures();
		updateMipmaps( attachment );
		return attachedTextureIt->second;
	}
	else
		return TextureBaseRef();
}

void Fbo::bindTexture( int textureUnit, GLenum attachment )
{
	auto tex = getTexture( attachment );
	if( tex )
		tex->bind( textureUnit );
}

void Fbo::unbindTexture( int textureUnit, GLenum attachment )
{
	auto tex = getTexture( attachment );
	if( tex )
		tex->unbind( textureUnit );
}

void Fbo::resolveTextures() const
{
	if( ! mNeedsResolve )
		return;

#if defined( CINDER_GL_ANGLE )
	if( mMultisampleFramebufferId ) {
		FramebufferScope drawFbScp( GL_DRAW_FRAMEBUFFER, mId );
		FramebufferScope readFbScp( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );
		
		glBlitFramebufferANGLE( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
	}
#elif defined( SUPPORTS_MULTISAMPLE ) && defined( CINDER_GLES )
	// iOS-specific multisample resolution code
	if( mMultisampleFramebufferId ) {
		FramebufferScope drawFbScp( GL_DRAW_FRAMEBUFFER_APPLE, mId );
		FramebufferScope readFbScp( GL_READ_FRAMEBUFFER_APPLE, mMultisampleFramebufferId );
		
		glResolveMultisampleFramebuffer();
	}
#elif defined( SUPPORTS_MULTISAMPLE )
	// if this FBO is multisampled, resolve it, so it can be displayed
	if( mMultisampleFramebufferId ) {
		auto ctx = context();

		ctx->pushFramebuffer( GL_DRAW_FRAMEBUFFER, mId );
		ctx->pushFramebuffer( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );
		
        vector<GLenum> drawBuffers;
		for( GLenum c = GL_COLOR_ATTACHMENT0; c <= MAX_COLOR_ATTACHMENT; ++c ) {
            auto colorAttachmentIt = mAttachmentsTexture.find( c );
            if( colorAttachmentIt != mAttachmentsTexture.end() ) {
                glDrawBuffer( colorAttachmentIt->first );
                glReadBuffer( colorAttachmentIt->first );
				glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
                drawBuffers.push_back( colorAttachmentIt->first );
            }
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		ctx->bindFramebuffer( GL_FRAMEBUFFER, mMultisampleFramebufferId );
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
		
		ctx->popFramebuffer( GL_DRAW_FRAMEBUFFER );
		ctx->popFramebuffer( GL_READ_FRAMEBUFFER );		
	}
#endif

	mNeedsResolve = false;
}

void Fbo::updateMipmaps( GLenum attachment ) const
{
	if( ! mNeedsMipmapUpdate )
		return;
	else {
		auto textureIt = mAttachmentsTexture.find( attachment );
		if( textureIt != mAttachmentsTexture.end() ) {
			TextureBindScope textureBind( textureIt->second );
			glGenerateMipmap( textureIt->second->getTarget() );
		}
	}

	mNeedsMipmapUpdate = false;
}

void Fbo::markAsDirty()
{
	if( mMultisampleFramebufferId )
		mNeedsResolve = true;

	for( const auto &textureAttachment : mAttachmentsTexture ) {
		if( textureAttachment.second->hasMipmapping() )
			mNeedsMipmapUpdate = true;
	}
}

void Fbo::bindFramebuffer( GLenum target )
{
	// This in turn will call bindFramebufferImpl; indirection is so that the Context can update its cache of the active Fbo
	gl::context()->bindFramebuffer( shared_from_this(), target );
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
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: incomplete attachment" );
		return false;
#if ! defined( CINDER_GLES )
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing draw buffer" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing read buffer" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: incomplete multisample" );
		break;
#else
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: not all attached images have the same number of samples" );
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
	FramebufferScope readScp( GL_READ_FRAMEBUFFER, mId );
	FramebufferScope drawScp( GL_DRAW_FRAMEBUFFER, dst.getId() );

	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	FramebufferScope readScp( GL_READ_FRAMEBUFFER, mId );
	FramebufferScope drawScp( GL_DRAW_FRAMEBUFFER, 0 );
	
	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
{
	FramebufferScope readScp( GL_READ_FRAMEBUFFER, GL_NONE );
	FramebufferScope drawScp( GL_DRAW_FRAMEBUFFER, mId );

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
