/*
 Copyright (c) 2013, The Cinder Project, All rights reserved.

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

#pragma once

#include "cinder/app/Renderer.h"

#if ! defined( CINDER_MSW )
	#error "RendererAngle is only supported on Microsoft Windows"
#endif

#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "EGL/egl.h"

namespace cinder { namespace app {

typedef std::shared_ptr<class RendererAngle>	RendererAngleRef;

class RendererAngle : public Renderer {
  public:
	RendererAngle() {}
	static RendererAngleRef	create() { return RendererAngleRef( new RendererAngle() ); }
	virtual RendererRef		clone() const { return RendererAngleRef( new RendererAngle( *this ) ); }
	
	virtual void setup( App *aApp, HWND wnd, HDC dc, RendererRef sharedRenderer );
	virtual void kill();
	
	virtual HWND	getHwnd() { return mWnd; }
	virtual HDC		getDc() { return mDC; }

	virtual void	prepareToggleFullScreen() override;
	virtual void	finishToggleFullScreen() override;

	virtual void startDraw() override;
	virtual void finishDraw() override;
	virtual void defaultResize() override;
	virtual void makeCurrentContext() override;
	virtual Surface	copyWindowSurface( const Area &area );

  protected:
	RendererAngle( const RendererAngle &renderer );
 
	HWND			mWnd;
	HDC				mDC;
	
	EGLContext		mContext;
	EGLDisplay		mDisplay;
	EGLSurface		mSurface;

	gl::ContextRef		mCinderContext;
};

EGLint getEglError();
std::string getEglErrorString( EGLint err );

void checkGlStatus();

} } // namespace cinder::app