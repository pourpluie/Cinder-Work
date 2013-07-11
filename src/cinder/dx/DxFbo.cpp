// The copyright in this software is being made available under the BSD License, included below. 
// This software may be subject to other third party and contributor rights, including patent rights, 
// and no such rights are granted under this license.
//
// Copyright (c) 2013, Microsoft Open Technologies, Inc. 
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, 
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice, 
//   this list of conditions and the following disclaimer in the documentation 
//   and/or other materials provided with the distribution.
// - Neither the name of Microsoft Open Technologies, Inc. nor the names of its contributors 
//   may be used to endorse or promote products derived from this software 
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS �AS IS� AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "cinder/dx/dx.h" // must be first
#include "cinder/dx/DxFbo.h"
#include "cinder/app/AppImplMswRendererDx.h"

using namespace std;

namespace cinder {
namespace dx {

GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

// Convenience macro to append either OES or EXT appropriately to a symbol based on OGLES vs. OGL
#if defined( CINDER_GLES )
	#define GL_SUFFIX(sym) sym##OES
#else
	#define GL_SUFFIX(sym) sym##EXT
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderBuffer::Obj
//Renderbuffer::Obj::Obj()
//{
//	mWidth = mHeight = -1;
//	mId = 0;
//	mInternalFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
//	mSamples = mCoverageSamples = 0;
//}
//
//Renderbuffer::Obj::Obj( int aWidth, int aHeight, DXGI_FORMAT internalFormat, int msaaSamples, int coverageSamples )
//	: mWidth( aWidth ), mHeight( aHeight ), mInternalFormat( internalFormat ), mSamples( msaaSamples ), mCoverageSamples( coverageSamples )
//{
//#if defined( CINDER_MSW )
//	static bool csaaSupported = ( GLEE_NV_framebuffer_multisample_coverage != 0 );
//#else
//	static bool csaaSupported = false;
//#endif
//
//	GL_SUFFIX(glGenRenderbuffers)( 1, &mId );
//
//	if( mSamples > Fbo::getMaxSamples() )
//		mSamples = Fbo::getMaxSamples();
//
//	if( ! csaaSupported )
//		mCoverageSamples = 0;
//
//	GL_SUFFIX(glBindRenderbuffer)( GL_SUFFIX(GL_RENDERBUFFER_), mId );
//
//#if ! defined( CINDER_GLES )
//  #if defined( CINDER_MSW )
//	if( mCoverageSamples ) // create a CSAA buffer
//		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER_EXT, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
//	else
//  #endif
//	if( mSamples ) // create a regular MSAA buffer
//		glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, mSamples, mInternalFormat, mWidth, mHeight );
//	else
//#endif
//		GL_SUFFIX(glRenderbufferStorage)( GL_SUFFIX(GL_RENDERBUFFER_), mInternalFormat, mWidth, mHeight );
//}
//
//Renderbuffer::Obj::~Obj()
//{
//	if( mId )
//		GL_SUFFIX(glDeleteRenderbuffers)( 1, &mId );
//}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Renderbuffer::Renderbuffer
//Renderbuffer::Renderbuffer( int width, int height, DXGI_FORMAT internalFormat )
//	: mObj( new Obj( width, height, internalFormat, 0, 0 ) )
//{
//}
//Renderbuffer::Renderbuffer( int width, int height, DXGI_FORMAT internalFormat, int msaaSamples, int coverageSamples )
//	: mObj( new Obj( width, height, internalFormat, msaaSamples, coverageSamples ) )
//{
//}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Obj
Fbo::Obj::Obj()
{
	mDepthTexture = NULL;
	mDepthView = NULL;
}

Fbo::Obj::Obj( int width, int height )
	: mWidth( width ), mHeight( height )
{
	mDepthTexture = NULL;
	mDepthView = NULL;
}

Fbo::Obj::~Obj()
{
	if(mDepthTexture) mDepthTexture->Release();
	if(mDepthView) mDepthView->Release();
	for(unsigned i = 0; i < mRenderTargets.size(); ++i)
		mRenderTargets[i]->Release();
	for(unsigned i = 0; i < mColorTextures.size(); ++i)
		mColorTextures[i]->Release();
	for(unsigned i = 0; i < mColorSRVs.size(); ++i)
		mColorSRVs[i]->Release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	//mTarget = GL_TEXTURE_2D;
	mColorInternalFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mDepthInternalFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	mDepthBufferAsTexture = true;
	mSamples = 0;
	mCoverageSamples = 0;
	mNumColorBuffers = 1;
	mDepthBuffer = true;
	mStencilBuffer = false;
	mMipmapping = false;
	mWrapS = D3D11_TEXTURE_ADDRESS_CLAMP;
	mWrapT = D3D11_TEXTURE_ADDRESS_CLAMP;
	mFilter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
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


void Fbo::init()
{
	//gl::SaveFramebufferBinding bindingSaver;
	
	HRESULT hr;
	for(int c = 0; c < mObj->mFormat.mNumColorBuffers; ++c)
	{
		D3D11_TEXTURE2D_DESC texDesc = {0};
		texDesc.Width = mObj->mWidth;
		texDesc.Height = mObj->mHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = getFormat().getColorInternalFormat();
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; 
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;
		ID3D11Texture2D *texture;
		hr = getDxRenderer()->md3dDevice->CreateTexture2D(&texDesc, NULL, &texture);
		if(hr != S_OK)
			throw FboExceptionInvalidSpecification("Creating texture for render target failed");
		mObj->mColorTextures.push_back(texture);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = texDesc.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = -1;
		ID3D11ShaderResourceView *SRV;
		hr = getDxRenderer()->md3dDevice->CreateShaderResourceView(texture, &SRVDesc, &SRV);
		if(hr != S_OK)
			throw FboExceptionInvalidSpecification("Creating shader resource view for render target failed");

		ID3D11RenderTargetView *rtv;
		hr = getDxRenderer()->md3dDevice->CreateRenderTargetView(texture, NULL, &rtv);
		if(hr != S_OK)
			throw FboExceptionInvalidSpecification("Creating render target failed");
		mObj->mRenderTargets.push_back(rtv);
	}

	D3D11_TEXTURE2D_DESC descDepth = {0};
    descDepth.Width = mObj->mWidth;
    descDepth.Height = mObj->mHeight;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
	hr = getDxRenderer()->md3dDevice->CreateTexture2D( &descDepth, NULL, &mObj->mDepthTexture );
	if( hr != S_OK )
		throw FboExceptionInvalidSpecification("Creating depth texture failed");

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
	hr = getDxRenderer()->md3dDevice->CreateDepthStencilView( mObj->mDepthTexture, &descDSV, &mObj->mDepthView );
	if(hr != S_OK)
		throw FboExceptionInvalidSpecification("Creating depth view failed");

	// allocate the framebuffer itself
	//GL_SUFFIX(glGenFramebuffers)( 1, &mObj->mId );
	//GL_SUFFIX(glBindFramebuffer)( GL_SUFFIX(GL_FRAMEBUFFER_), mObj->mId );	

	//Texture::Format textureFormat;
	//textureFormat.setTarget( getTarget() );
	//textureFormat.setInternalFormat( getFormat().getColorInternalFormat() );
	//textureFormat.setWrap( mObj->mFormat.mWrapS, mObj->mFormat.mWrapT );
	//textureFormat.setFilter( mObj->mFormat.mFilter );
	//textureFormat.enableMipmapping( getFormat().hasMipMapping() );

	// allocate the color buffers
	//for( int c = 0; c < mObj->mFormat.mNumColorBuffers; ++c ) {
	//	mObj->mColorTextures.push_back( Texture( mObj->mWidth, mObj->mHeight, textureFormat ) );
	//}
//		
//	if( mObj->mFormat.mNumColorBuffers == 0 ) { // no color
//		glDrawBuffer( GL_NONE );
//		glReadBuffer( GL_NONE );	
//	}
//		
//	if( ( ( ! useCSAA ) && ( ! useMSAA ) ) || ( ! initMultisample( useCSAA ) ) ) { // if we don't need any variety of multisampling or it failed to initialize
//		// attach all the textures to the framebuffer
//		vector<GLenum> drawBuffers;
//		for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
//			GL_SUFFIX(glFramebufferTexture2D)( GL_SUFFIX(GL_FRAMEBUFFER_), GL_SUFFIX(GL_COLOR_ATTACHMENT0_) + c, getTarget(), mObj->mColorTextures[c].getId(), 0 );
//			drawBuffers.push_back( GL_SUFFIX(GL_COLOR_ATTACHMENT0_) + c );
//		}
//		if( ! drawBuffers.empty() )
//			glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
//
//		// allocate and attach depth texture
//		if( mObj->mFormat.mDepthBuffer ) {
//			if( mObj->mFormat.mDepthBufferAsTexture ) {
//				GLuint depthTextureId;
//				glGenTextures( 1, &depthTextureId );
//				glBindTexture( getTarget(), depthTextureId );
//				glTexImage2D( getTarget(), 0, getFormat().getDepthInternalFormat(), mObj->mWidth, mObj->mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
//				glTexParameteri( getTarget(), GL_TEXTURE_MIN_FILTER, mObj->mFormat.mMinFilter );
//				glTexParameteri( getTarget(), GL_TEXTURE_MAG_FILTER, mObj->mFormat.mMagFilter );
//				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_S, mObj->mFormat.mWrapS );
//				glTexParameteri( getTarget(), GL_TEXTURE_WRAP_T, mObj->mFormat.mWrapT );
//				glTexParameteri( getTarget(), GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
//				//mObj->mDepthTexture = Texture( getTarget(), depthTextureId, mObj->mWidth, mObj->mHeight, true );
//
//				glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, getTarget(), mObj->mDepthTexture.getId(), 0 );
////glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, getTarget(), mObj->mDepthTexture.getId(), 0 );
//			}
//			else if( mObj->mFormat.mDepthBuffer ) { // implement depth buffer as RenderBuffer
//				mObj->mDepthRenderbuffer = Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.getDepthInternalFormat() );
//				GL_SUFFIX(glFramebufferRenderbuffer)( GL_SUFFIX(GL_FRAMEBUFFER_), GL_SUFFIX(GL_DEPTH_ATTACHMENT_), GL_SUFFIX(GL_RENDERBUFFER_), mObj->mDepthRenderbuffer.getId() );
//			}
//		}
//
//		FboExceptionInvalidSpecification exc;
//		if( ! checkStatus( &exc ) ) { // failed creation; throw
//			throw exc;
//		}
//	}
	
	mObj->mNeedsResolve = false;
	mObj->mNeedsMipmapUpdate = false;
}

bool Fbo::initMultisample( bool csaa )
{
	//glGenFramebuffersEXT( 1, &mObj->mResolveFramebufferId );
	//glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, mObj->mResolveFramebufferId ); 
	//
	//// bind all of the color buffers to the resolve FB's attachment points
	//vector<GLenum> drawBuffers;
	//for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
	//	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + c, getTarget(), mObj->mColorTextures[c].getId(), 0 );
	//	drawBuffers.push_back( GL_COLOR_ATTACHMENT0_EXT + c );
	//}

	//if( ! drawBuffers.empty() )
	//	glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );

	//// see if the resolve buffer is ok
	//FboExceptionInvalidSpecification ignoredException;
	//if( ! checkStatus( &ignoredException ) )
	//	return false;

	//glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, mObj->mId );

	//if( mObj->mFormat.mSamples > getMaxSamples() ) {
	//	mObj->mFormat.mSamples = getMaxSamples();
	//}

	//// setup the multisampled color renderbuffers
	//for( int c = 0; c < mObj->mFormat.mNumColorBuffers; ++c ) {
	//	mObj->mMultisampleColorRenderbuffers.push_back( Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.mColorInternalFormat, mObj->mFormat.mSamples, mObj->mFormat.mCoverageSamples ) );

	//	// attach the multisampled color buffer
	//	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + c, GL_RENDERBUFFER_EXT, mObj->mMultisampleColorRenderbuffers.back().getId() );
	//}
	//
	//if( ! drawBuffers.empty() )
	//	glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );

	//if( mObj->mFormat.mDepthBuffer ) {
	//	// create the multisampled depth Renderbuffer
	//	mObj->mMultisampleDepthRenderbuffer = Renderbuffer( mObj->mWidth, mObj->mHeight, mObj->mFormat.mDepthInternalFormat, mObj->mFormat.mSamples, mObj->mFormat.mCoverageSamples );

	//	// attach the depth Renderbuffer
	//	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mObj->mMultisampleDepthRenderbuffer.getId() );
	//}

	//// see if the primary framebuffer turned out ok
	//return checkStatus( &ignoredException );
	return true;
}

Fbo::Fbo( int width, int height, Format format )
	: mObj( shared_ptr<Obj>( new Obj( width, height ) ) )
{
	mObj->mFormat = format;
	init();
}

Fbo::Fbo( int width, int height, bool alpha, bool color, bool depth )
	: mObj( shared_ptr<Obj>( new Obj( width, height ) ) )
{
	Format format;
	mObj->mFormat.mColorInternalFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mObj->mFormat.mDepthBuffer = depth;
	mObj->mFormat.mNumColorBuffers = color ? 1 : 0;
	init();
}

ID3D11Texture2D*& Fbo::getTexture( int attachment )
{
	resolveTextures();
	updateMipmaps( true, attachment );
	return mObj->mColorTextures[attachment];
}

ID3D11Texture2D*& Fbo::getDepthTexture()
{
	return mObj->mDepthTexture;
}

void Fbo::bindTexture( int textureUnit, int attachment )
{
	resolveTextures();
	getDxRenderer()->mDeviceContext->PSSetShaderResources(textureUnit, 1, &mObj->mColorSRVs[attachment]);
	updateMipmaps( false, attachment );
}

void Fbo::unbindTexture()
{
	//glBindTexture( getTarget(), 0 );
}

void Fbo::bindDepthTexture( int textureUnit )
{
	throw FboExceptionInvalidSpecification("bindDepthTexture not implemented yet");
	//getDxRenderer()->mDeviceContext->PSSetShaderResources(textureUnit, 1, &mObj->mDepthView);
}

void Fbo::resolveTextures() const
{
	if( ! mObj->mNeedsResolve )
		return;
	
	// if this FBO is multisampled, resolve it, so it can be displayed
	//if ( mObj->mResolveFramebufferId ) {
	//	SaveFramebufferBinding saveFboBinding;

	//	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mObj->mId );
	//	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, mObj->mResolveFramebufferId );
	//	
	//	for( size_t c = 0; c < mObj->mColorTextures.size(); ++c ) {
	//		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + c );
	//		glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + c );
	//		GLbitfield bitfield = GL_COLOR_BUFFER_BIT;
	//		if( mObj->mDepthTexture )
	//			bitfield |= GL_DEPTH_BUFFER_BIT;
	//		glBlitFramebufferEXT( 0, 0, mObj->mWidth, mObj->mHeight, 0, 0, mObj->mWidth, mObj->mHeight, bitfield, GL_NEAREST );
	//	}

	//	// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
	//	vector<GLenum> drawBuffers;
	//	for( size_t c = 0; c < mObj->mColorTextures.size(); ++c )
	//		drawBuffers.push_back( GL_COLOR_ATTACHMENT0_EXT + c );
	//	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, mObj->mId );
	//	glDrawBuffers( drawBuffers.size(), &drawBuffers[0] );
	//}

	mObj->mNeedsResolve = false;
}

void Fbo::updateMipmaps( bool bindFirst, int attachment ) const
{
	if( ! mObj->mNeedsMipmapUpdate )
		return;
	
	//if( bindFirst ) {
	//	SaveTextureBindState state( getTarget() );
	//	mObj->mColorTextures[attachment].bind();
	//	GL_SUFFIX(glGenerateMipmap)( getTarget() );
	//}
	//else {
	//	GL_SUFFIX(glGenerateMipmap)( getTarget() );
	//}

	mObj->mNeedsMipmapUpdate = false;
}

void Fbo::bindFramebuffer()
{
	getDxRenderer()->mDeviceContext->OMSetRenderTargets(mObj->mColorSRVs.size(), &mObj->mRenderTargets[0], mObj->mDepthView);
	//GL_SUFFIX(glBindFramebuffer)( GL_SUFFIX(GL_FRAMEBUFFER_), mObj->mId );
	//if( mObj->mResolveFramebufferId ) {
	//	mObj->mNeedsResolve = true;
	//}
	//if( mObj->mFormat.hasMipMapping() ) {
	//	mObj->mNeedsMipmapUpdate = true;
	//}
}

void Fbo::unbindFramebuffer()
{
	getDxRenderer()->mDeviceContext->OMSetRenderTargets(1, &getDxRenderer()->mMainFramebuffer, getDxRenderer()->mDepthStencilView);
	//GL_SUFFIX(glBindFramebuffer)( GL_SUFFIX(GL_FRAMEBUFFER_), 0 );
}

GLint Fbo::getMaxSamples()
{
	if( sMaxSamples < 0 ) {
		sMaxSamples = 0;
	}
	
	return sMaxSamples;
}

GLint Fbo::getMaxAttachments()
{
	if( sMaxAttachments < 0 ) {
		sMaxAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	}
	
	return sMaxAttachments;
}

void Fbo::blitTo( Fbo dst, const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	//SaveFramebufferBinding saveFboBinding;

	//glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mObj->mId );
	//glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, dst.getId() );		
	//glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	//SaveFramebufferBinding saveFboBinding;

	//glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mObj->mId );
	//glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, 0 );		
	//glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
{
	//SaveFramebufferBinding saveFboBinding;

	//glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, GL_NONE );
	//glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, mObj->mId );		
	//glBlitFramebufferEXT( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

FboExceptionInvalidSpecification::FboExceptionInvalidSpecification( const string &message ) throw()
	: FboException()
{
	strncpy_s( mMessage, message.c_str(), 255 );
}

#undef GL_SUFFIX

} } // namespace cinder::dx
