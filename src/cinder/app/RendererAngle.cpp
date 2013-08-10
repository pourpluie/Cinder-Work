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

#include "cinder/app/RendererAngle.h"

#include "cinder/app/App.h"
#include <signal.h>

#define DEBUG_GL 1

#if ! defined( CI_SCREENSAVER )
#define CI_BREAK() { __debugbreak(); }
//#define CI_BREAK() { __asm int 3; }
#else
#define CI_BREAK() 
#endif

// TODO: these gl stubs should not be necessary once gl calls go through env()
#if defined( CINDER_GL_ANGLE )

void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access)
{
	return NULL;
}

GLboolean GL_APIENTRY glUnmapBufferOES (GLenum target)
{
	return false;
}

namespace cinder { namespace app {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RendererAngle
RendererAngle::RendererAngle( const RendererAngle &renderer )
	: Renderer( renderer )
{
}

void RendererAngle::setup( App *app, HWND wnd, HDC dc, RendererRef /*sharedRenderer*/ )
{
	mApp = app;
	mWnd = wnd;
	
	EGLint configAttribList[] =
   {
       EGL_RED_SIZE,       8,
       EGL_GREEN_SIZE,     8,
       EGL_BLUE_SIZE,      8,
       EGL_ALPHA_SIZE,     EGL_DONT_CARE,
       EGL_DEPTH_SIZE,     24,
       EGL_STENCIL_SIZE,   EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, 0,
       EGL_NONE
   };
   EGLint surfaceAttribList[] =
   {
       EGL_NONE, EGL_NONE
   };

	//createEGLContext( wnd, &mDisplay, &mContext, &mSurface, configAttribList, surfaceAttribList );

	// Get Display
	mDisplay = eglGetDisplay( GetDC( wnd ) );
	assert( mDisplay != EGL_NO_DISPLAY );

	checkGlStatus();

	// Initialize EGL
	EGLint majorVersion;
	EGLint minorVersion;
	EGLBoolean status = eglInitialize( mDisplay, &majorVersion, &minorVersion );
	assert( status );

	checkGlStatus();

	// Get configs
	EGLint numConfigs;
	status = eglGetConfigs( mDisplay, NULL, 0, &numConfigs );
	assert( status );

	checkGlStatus();

	// Choose config
	EGLConfig config;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
	status = eglChooseConfig( mDisplay, configAttribList, &config, 1, &numConfigs );
	assert( status );

	checkGlStatus();

	// Create a surface
	mSurface = eglCreateWindowSurface( mDisplay, config, (EGLNativeWindowType)wnd, surfaceAttribList );
	assert( mSurface != EGL_NO_SURFACE );

	checkGlStatus();

	// Create a GL context
	mContext = eglCreateContext( mDisplay, config, EGL_NO_CONTEXT, contextAttribs );
	assert( mContext != EGL_NO_CONTEXT );

	checkGlStatus();
	
	gl::Context::EglPlatformData platformData;

	platformData.context = &mContext;
	platformData.display = &mDisplay;
	platformData.surface = &mSurface;

	mCinderContext = cinder::gl::Context::createFromExisting( &platformData );
	checkGlStatus();

	mCinderContext->makeCurrent();

	checkGlStatus();

	eglSwapInterval( mDisplay, 0 );
	checkGlStatus();
}

void RendererAngle::kill()
{
}

void RendererAngle::prepareToggleFullScreen()
{
//	mImpl->prepareToggleFullScreen();
}

void RendererAngle::finishToggleFullScreen()
{
//	mImpl->finishToggleFullScreen();
}

void RendererAngle::startDraw()
{
	checkGlStatus();

//	mImpl->makeCurrentContext();
}

void RendererAngle::finishDraw()
{
	checkGlStatus();

	eglSwapBuffers( mDisplay, mSurface );

	checkGlStatus();
}

void RendererAngle::defaultResize()
{
	checkGlStatus();

	::RECT clientRect;
	::GetClientRect( mWnd, &clientRect );
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	gl::setViewport( Area( 0, 0, width, height ) );

	cinder::CameraPersp cam( width, height, 60.0f );
	gl::setMatrices( cam );
	gl::scale( 1.0f, -1.0f, 1.0f );           // invert Y axis so increasing Y goes down.
	gl::translate( Vec3f( 0.0f, -height, 0.0f ) );       // shift origin up to upper-left corner.

	checkGlStatus();
}

void RendererAngle::makeCurrentContext()
{
	checkGlStatus();

	mCinderContext->makeCurrent();

	checkGlStatus();

}

Surface	RendererAngle::copyWindowSurface( const Area &area )
{
//	return mImpl->copyWindowContents( area );
	assert( 0 && "not implemented" );
	return Surface();
}

EGLint getEglError()
{
	return eglGetError();
}

std::string getEglErrorString( EGLint err )
{
	switch( err ) {
		case EGL_SUCCESS: return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
		default: return "unknown error code";
	}
}

void checkGlStatus()
{
#if DEBUG_GL
	EGLint lastEglError = getEglError();
	if( lastEglError != EGL_SUCCESS ) {
		ci::app::console() << "EGL ERROR: " << getEglErrorString( lastEglError ) << std::endl;
		CI_BREAK();
	}


	GLenum lastGlError = ci::gl::getError();
	if( lastGlError != GL_NO_ERROR ) {
		ci::app::console() << "GL ERROR: " << ci::gl::getErrorString( lastGlError ) << std::endl;
		CI_BREAK();
	}
#endif // DEBUG_GL
}

} } // namespace cinder::app


#endif // defined( CINDER_ANGLE_GL )
