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

#if defined( CINDER_MSW ) && (! defined( CINDER_GLES ))
	if( mCoverageSamples ) // create a CSAA buffer
		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
	else
#elif defined( CINDER_GLES )
	if(mSamples) {
		glRenderbufferStorageMultisampleAPPLE( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
	}
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
#if defined( CINDER_GLES )
	mColorInternalFormat = GL_RGBA;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24_OES;
	mDepthBufferAsTexture = false;
	mDepthTextureFormat = Texture::Format().depthTexture().internalFormat( GL_DEPTH_COMPONENT ).pixelDataFormat( GL_DEPTH_COMPONENT ).pixelDataType( GL_UNSIGNED_SHORT );
    	mColorTextureFormat = Texture::Format().renderTexture().internalFormat( GL_RGBA ).pixelDataFormat( GL_RGBA ).pixelDataType( GL_UNSIGNED_BYTE );
#else
	mColorInternalFormat = GL_RGBA8;
	mDepthInternalFormat = GL_DEPTH_COMPONENT24;
	mDepthBufferAsTexture = false;
	mDepthTextureFormat = Texture::Format().depthTexture().internalFormat( GL_DEPTH_COMPONENT24 ).pixelDataFormat( GL_DEPTH_COMPONENT ).pixelDataType( GL_FLOAT );
    mColorTextureFormat = Texture::Format().renderTexture().internalFormat( GL_RGBA ).pixelDataFormat( GL_RGBA ).pixelDataType( GL_UNSIGNED_BYTE );
#endif
	mSamples = 0;
	mCoverageSamples = 0;
	mNumColorBuffers = 1;
	mDepthBuffer = true;
	mStencilBuffer = false;
}

void Fbo::Format::enableColorBuffer( bool colorBuffer, int numColorBuffers )
{
#if defined( CINDER_GLES )
	mNumColorBuffers = ( colorBuffer && numColorBuffers ) ? 1 : 0;
#else
	mNumColorBuffers = ( colorBuffer ) ? numColorBuffers : 0;
#endif
}
    
Fbo::Format& Fbo::Format::color( bool colorBuffer, int numColorBuffers )
{
#if defined( CINDER_GLES )
    mNumColorBuffers = ( colorBuffer && numColorBuffers ) ? 1 : 0;
#else
    mNumColorBuffers = ( colorBuffer ) ? numColorBuffers : 0;
#endif
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo
FboRef Fbo::create( int width, int height, Format format )
{
	return FboRef( new Fbo( width, height, format ) );
}

FboRef Fbo::create( int width, int height, bool alpha, bool color, bool depth, bool stencil )
{
	return FboRef( new Fbo( width, height, alpha, color, depth, stencil ) );
}

Fbo::Fbo( int width, int height, Format format )
	: mWidth( width ), mHeight( height ), mId( 0 ), mResolveFramebufferId( 0 )
{
	mFormat = format;
	init();
}

Fbo::Fbo( int width, int height, bool alpha, bool color, bool depth, bool stencil )
	: mWidth( width ), mHeight( height ), mId( 0 ), mResolveFramebufferId( 0 )
{
	Format format;
#if defined( CINDER_GLES )
	mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8_OES : GL_RGB8_OES;
#else
	mFormat.mColorInternalFormat = ( alpha ) ? GL_RGBA8 : GL_RGB8;
#endif
    	mFormat.mStencilBuffer = stencil;
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
	
#if ! defined( CINDER_GLES )
    // allocate the color buffers
	for( int c = 0; c < mFormat.mNumColorBuffers; ++c ) {
        	mTextures[GL_COLOR_ATTACHMENT0 + c] = Texture::create( mWidth, mHeight, mFormat.mColorTextureFormat );
	}
	
	if( mFormat.mNumColorBuffers == 0 ) { // no color
		glDrawBuffer( GL_NONE );
		glReadBuffer( GL_NONE );
	}
	
	if( ( ( ! useCSAA ) && ( ! useMSAA ) ) || ( ! initMultisample( useCSAA ) ) ) {
		// if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer
		vector<GLenum> drawBuffers;
		for( GLenum c = GL_COLOR_ATTACHMENT0; c < GL_COLOR_ATTACHMENT15 + 1; ++c ) {
            auto colorAttachment = mTextures.find( c );
            if( colorAttachment != mTextures.end() ) {
                glFramebufferTexture2D( GL_FRAMEBUFFER, colorAttachment->first, colorAttachment->second->getTarget(), colorAttachment->second->getId(), 0 );
                drawBuffers.push_back( colorAttachment->first );
            }
		}
		
		if( ! drawBuffers.empty() )
			glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
		
		// allocate and attach packed depth/stencil buffers
        if( mFormat.mDepthBufferAsTexture ) {
			gl::TextureRef depthTexture;
            if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
                if( mFormat.mDepthInternalFormat == GL_DEPTH_COMPONENT32F ) {
                    mFormat.mDepthTextureFormat.internalFormat( GL_DEPTH32F_STENCIL8 ).pixelDataFormat( GL_DEPTH_STENCIL ).pixelDataType( GL_FLOAT_32_UNSIGNED_INT_24_8_REV );
					depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
					mTextures[GL_DEPTH_STENCIL_ATTACHMENT] = depthTexture;
                    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
                }
                else {
                    mFormat.mDepthTextureFormat.internalFormat( GL_DEPTH24_STENCIL8 ).pixelDataFormat( GL_DEPTH_STENCIL ).pixelDataType( GL_UNSIGNED_INT_24_8 );
					depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
					mTextures[GL_DEPTH_STENCIL_ATTACHMENT] = depthTexture;
                    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
                }
            }
            else {
                if( mFormat.mDepthBuffer ) {
                    mFormat.mDepthTextureFormat.internalFormat( mFormat.mDepthInternalFormat );
					depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
					mTextures[GL_DEPTH_ATTACHMENT] = depthTexture;
                    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
                }
            }
        }
        else {
			gl::RenderbufferRef depthBuffer;
            if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
                if( mFormat.mDepthInternalFormat == GL_DEPTH_COMPONENT32F ) {
                    depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH32F_STENCIL8 );
					mRenderbuffers[GL_DEPTH_STENCIL_ATTACHMENT] = depthBuffer;
                    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
                }
                else {
                    depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH24_STENCIL8 );
					mRenderbuffers[GL_DEPTH_STENCIL_ATTACHMENT] = depthBuffer;
                    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
                }
            }
            else {
                if( mFormat.mDepthBuffer ) {
					depthBuffer = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthInternalFormat );
					mRenderbuffers[GL_DEPTH_ATTACHMENT] = depthBuffer;
                    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
                }
                else if( mFormat.mStencilBuffer ) {
                    throw FboExceptionInvalidSpecification( "Can't have stencil buffer without depth buffer." );
                }
            }
        }
    }
#else
    // allocate the color buffers
	if( mFormat.hasColorBuffer() ) {
		mTextures[GL_COLOR_ATTACHMENT0] = Texture::create( mWidth, mHeight, mFormat.mColorTextureFormat );
	}
    
	if( ( ( ! useCSAA ) && ( ! useMSAA ) ) || ( ! initMultisample( useCSAA ) ) ) {
		// if we don't need any variety of multisampling or it failed to initialize
		// attach all the textures to the framebuffer

        auto colorAttachment = mTextures.find( GL_COLOR_ATTACHMENT0 );
        if( colorAttachment != mTextures.end() ) {
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorAttachment->second->getTarget(), colorAttachment->second->getId(), 0 );
        }
		
		if( mFormat.mDepthBufferAsTexture ) {
			gl::TextureRef depthTexture;
			if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
                mFormat.mDepthTextureFormat.depthTexture().internalFormat( GL_DEPTH_STENCIL_OES ).pixelDataFormat( GL_DEPTH_STENCIL_OES ).pixelDataType( GL_UNSIGNED_INT_24_8_OES );
				depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
                mTextures[GL_DEPTH_STENCIL_OES] = depthTexture;
                glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
                glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
			} else {
				if( mFormat.mDepthBuffer ) {
					depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
					mTextures[GL_DEPTH_ATTACHMENT] = depthTexture;
                    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
				}
			}
		} else {
			gl::RenderbufferRef depthBuffer, stencilBuffer;
			if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
				depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH24_STENCIL8_OES );
				stencilBuffer = depthBuffer;
				mRenderbuffers[GL_DEPTH_ATTACHMENT] = depthBuffer;
				mRenderbuffers[GL_STENCIL_ATTACHMENT] = stencilBuffer;
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilBuffer->getId() );
			}
			else {
				if( mFormat.mDepthBuffer ) {
					auto depthBuffer = Renderbuffer::create( mWidth, mHeight, mFormat.getDepthInternalFormat() );
					mRenderbuffers[GL_DEPTH_ATTACHMENT] = depthBuffer;
					glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
				}
			}
		}
        
	}
	
#endif
			
	FboExceptionInvalidSpecification exc;
	if( ! checkStatus( &exc ) ) { // failed creation; throw
		throw exc;
	}
	
	mNeedsResolve = false;
	mNeedsMipmapUpdate = false;
}

bool Fbo::initMultisample( bool csaa )
{
	auto ctx = context();

	glGenFramebuffers( 1, &mResolveFramebufferId );
	ctx->bindFramebuffer( GL_FRAMEBUFFER, mResolveFramebufferId );

#if ! defined( CINDER_GLES )
    // bind all of the color buffers to the resolve FB's attachment points
    vector<GLenum> drawBuffers;
    for( GLenum c = GL_COLOR_ATTACHMENT0; c < GL_COLOR_ATTACHMENT15 + 1; ++c ) {
        auto colorAttachment = mTextures.find( c );
        if( colorAttachment != mTextures.end() ) {
            glFramebufferTexture2D( GL_FRAMEBUFFER, colorAttachment->first, colorAttachment->second->getTarget(), colorAttachment->second->getId(), 0 );
            drawBuffers.push_back( colorAttachment->first );
        }
    }
	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
	
    
    if( mFormat.mDepthBufferAsTexture ) {
		gl::TextureRef depthTexture;
        if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
            if( mFormat.mDepthInternalFormat == GL_DEPTH_COMPONENT32F ) {
                mFormat.mDepthTextureFormat.internalFormat( GL_DEPTH32F_STENCIL8 ).pixelDataFormat( GL_DEPTH_STENCIL ).pixelDataType( GL_FLOAT_32_UNSIGNED_INT_24_8_REV );
				depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
				mTextures[GL_DEPTH_STENCIL_ATTACHMENT] = depthTexture;
                glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
            }
            else {
                mFormat.mDepthTextureFormat.internalFormat( GL_DEPTH24_STENCIL8 ).pixelDataFormat( GL_DEPTH_STENCIL ).pixelDataType( GL_UNSIGNED_INT_24_8 );
				depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
				mTextures[GL_DEPTH_STENCIL_ATTACHMENT] = depthTexture;
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
            }
        }
        else {
            if( mFormat.mDepthBuffer ) {
                mFormat.mDepthTextureFormat.internalFormat( mFormat.mDepthInternalFormat );
				depthTexture = Texture::create( mWidth, mHeight, mFormat.mDepthTextureFormat );
				mTextures[GL_DEPTH_ATTACHMENT] = depthTexture;
                glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture->getTarget(), depthTexture->getId(), 0 );
            }
        }
    }
    
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
        auto mColorAttachment = Renderbuffer::create( mWidth, mHeight, mFormat.mColorInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
		mRenderbuffers[GL_COLOR_ATTACHMENT0 + c] = mColorAttachment;
		// attach the multisampled color buffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + c, GL_RENDERBUFFER, mColorAttachment->getId() );
	}
	
	if( ! drawBuffers.empty() )
		glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
    
    if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
		gl::RenderbufferRef depthBuffer;
        if( mFormat.mDepthInternalFormat == GL_DEPTH_COMPONENT32F ) {
			depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH32F_STENCIL8, mFormat.mSamples, mFormat.mCoverageSamples );
            mRenderbuffers[GL_DEPTH_STENCIL_ATTACHMENT] = depthBuffer;
            glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
        } else {
			depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH24_STENCIL8, mFormat.mSamples, mFormat.mCoverageSamples );
			mRenderbuffers[GL_DEPTH_STENCIL_ATTACHMENT] = depthBuffer;
            glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
        }
    }
    else {
        if( mFormat.mDepthBuffer ) {
			auto depthBuffer = Renderbuffer::create( mWidth, mHeight, mFormat.getDepthInternalFormat(), mFormat.mSamples, mFormat.mCoverageSamples );
			mRenderbuffers[GL_DEPTH_ATTACHMENT] = depthBuffer;
            glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
        }
        else {
            throw FboExceptionInvalidSpecification( "Can't have stencil buffer without depth buffer." );
        }
    }
    
    
#else
	
    // bind all of the color buffers to the resolve FB's attachment points
    vector<GLenum> drawBuffers;
    auto colorAttachment = mTextures.find( GL_COLOR_ATTACHMENT0 );
    if( colorAttachment != mTextures.end() ) {
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorAttachment->second->getTarget(), colorAttachment->second->getId(), 0 );
	}
    
	// see if the resolve buffer is ok
	FboExceptionInvalidSpecification ignoredException;
	if( ! checkStatus( &ignoredException ) ) {
		return false;
	}
	ctx->bindFramebuffer( GL_FRAMEBUFFER, mId );
	
	if( mFormat.mSamples > getMaxSamples() ) {
		mFormat.mSamples = getMaxSamples();
	}
	
	if(mFormat.hasColorBuffer()) {
		if( mFormat.mColorInternalFormat == GL_RGBA ) {
			mFormat.mColorInternalFormat = GL_RGBA8_OES;
		}
		else if( mFormat.mColorInternalFormat == GL_RGB ) {
			mFormat.mColorInternalFormat = GL_RGB8_OES;
		}
		auto colorBuffer = Renderbuffer::create( mWidth, mHeight, mFormat.mColorInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
		mRenderbuffers[GL_COLOR_ATTACHMENT0] = colorBuffer;
		// attach the multisampled color buffer
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer->getId() );
	}
	
	if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
		auto depthBuffer = Renderbuffer::create( mWidth, mHeight, GL_DEPTH24_STENCIL8_OES, mFormat.mSamples, mFormat.mCoverageSamples );
		mRenderbuffers[GL_DEPTH24_STENCIL8_OES] = depthBuffer;
        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
	}
	else {
        if( mFormat.mDepthBuffer ) {
			auto depthBuffer = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
			mRenderbuffers[GL_DEPTH_ATTACHMENT] = depthBuffer;
            glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer->getId() );
		}
	}
	
#endif
	// see if the primary framebuffer turned out ok
	return checkStatus( &ignoredException );
}

TextureRef Fbo::getTexture( int attachment )
{
	resolveTextures();
	updateMipmaps( GL_COLOR_ATTACHMENT0 + attachment );
	return mTextures[GL_COLOR_ATTACHMENT0 + attachment];
}

TextureRef Fbo::getDepthTexture()
{
    resolveTextures();
    if( mFormat.mDepthBuffer && mFormat.mStencilBuffer )
#if ! defined( CINDER_GLES )
        return mTextures[GL_DEPTH_STENCIL_ATTACHMENT];
    else
#else
        return mTextures[GL_DEPTH_STENCIL_OES];
    else
#endif
        return mTextures[GL_DEPTH_ATTACHMENT];
}

void Fbo::bindTexture( int textureUnit, int attachment )
{
	resolveTextures();
	mTextures[GL_COLOR_ATTACHMENT0 + attachment]->bind( textureUnit );
	updateMipmaps( GL_COLOR_ATTACHMENT0 + attachment );
}

void Fbo::bindDepthTexture( int textureUnit )
{

    if( mFormat.mDepthBuffer && mFormat.mStencilBuffer )
#if ! defined( CINDER_GLES )
        mTextures[GL_DEPTH_STENCIL_ATTACHMENT]->bind( textureUnit );
    else
#else
        mTextures[GL_DEPTH_STENCIL_OES]->bind( textureUnit );
    else
#endif
        mTextures[GL_DEPTH_ATTACHMENT]->bind( textureUnit );
}

void Fbo::unbindTexture()
{
	glBindTexture( getTarget(), 0 );
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
		
        vector<GLenum> drawBuffers;
		for( GLenum c = GL_COLOR_ATTACHMENT0; c < GL_COLOR_ATTACHMENT15 + 1; ++c ) {
            auto colorAttachment = mTextures.find( c );
            if( colorAttachment != mTextures.end() ) {
                glDrawBuffer( colorAttachment->first );
                glReadBuffer( colorAttachment->first );
                glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
                drawBuffers.push_back( colorAttachment->first );
            }
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
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
		auto mColorTexture = mTextures.find(GL_COLOR_ATTACHMENT0 + attachment);
		if( mColorTexture != mTextures.end() ) {
			TextureBindScope textureBind( getTarget(), mColorTexture->second->getId() );
			glGenerateMipmap( getTarget() );
		}
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
	if( mFormat.mColorTextureFormat.hasMipmapping() ) {
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
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: incomplete attachment" );
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
