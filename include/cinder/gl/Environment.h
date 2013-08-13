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

#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"

#if defined( CINDER_MAC )
	typedef struct _CGLContextObject       *CGLContextObj;
#elif defined( CINDER_COCOA_TOUCH )
	#if defined( __OBJC__ )
		@class EAGLContext;
	#else
		typedef void*	EAGLContext;
	#endif
#elif defined( CINDER_GL_ANGLE )
	typedef void*		EGLContext;
	typedef void*		EGLDisplay;
	typedef void*		EGLSurface;
	typedef void*		EGLConfig;
#endif

namespace cinder { namespace gl {

class ShaderDef;
class GlslProg;
typedef std::shared_ptr<GlslProg>		GlslProgRef;
class Context;
typedef std::shared_ptr<Context>		ContextRef;

class Environment {
  public:
	virtual void			initializeFunctionPointers() = 0;
	
	ContextRef				createSharedContext( const Context *sharedContext );
	void					makeContextCurrent( const Context *context );
	
	virtual bool			supportsHardwareVao() = 0;

	virtual std::string		generateVertexShader( const ShaderDef &shader ) = 0;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) = 0;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) = 0;

#if ! defined( CINDER_GLES )	
	static void				setCore();
	static void				setLegacy();
#else
	static void				setEs2();
#endif
};


#if defined( CINDER_COCOA_TOUCH )
struct PlatformDataIos : public Context::PlatformData {
	PlatformDataIos( EAGLContext *eaglContext )
		: mEaglContext( eaglContext )
	{}
	
	EAGLContext		*mEaglContext;
};

#elif defined( CINDER_MAC )
struct PlatformDataMac : public Context::PlatformData {
	PlatformDataMac( CGLContextObj cglContext )
		: mCglContext( cglContext )
	{}
	
	CGLContextObj		mCglContext;
};

#elif defined( CINDER_MSW ) && defined( CINDER_GL_ANGLE )
struct PlatformDataAngle : public Context::PlatformData {
	PlatformDataAngle( EGLContext context, EGLDisplay display, EGLSurface surface, EGLConfig eglConfig )
		: mContext( context ), mDisplay( display ), mSurface( surface ), mConfig( eglConfig )
	{}

	EGLContext		mContext;
	EGLDisplay		mDisplay;
	EGLSurface		mSurface;
	EGLConfig		mConfig;
};

#elif defined( CINDER_MSW ) // normal MSW desktop GL
struct PlatformDataMsw : public Context::PlatformData {
	PlatformDataMsw( HGLRC glrc, HDC dc )
		: mGlrc( glrc ), mDc( dc )
	{}

	HGLRC	mGlrc;
	HDC		mDc;
};
#endif

} } // namespace cinder::gl