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

#include "cinder/gl/Environment.h"
#include "cinder/gl/Context.h"

#if defined( CINDER_MAC )
	#include <OpenGL/OpenGL.h>
#elif defined( CINDER_COCOA_TOUCH )
	#import <OpenGLES/EAGL.h>
#endif

using namespace std;

namespace cinder { namespace gl {

extern Environment* allocateEnvironmentCore();
extern Environment* allocateEnvironmentLegacy();
extern Environment* allocateEnvironmentEs2();
static Environment *sEnvironment = NULL;

#if ! defined( CINDER_GLES )
void Environment::setCore()
{
	if( ! sEnvironment ) {
		sEnvironment = allocateEnvironmentCore();
	}
}

void Environment::setLegacy()
{
	if( ! sEnvironment ) {
		sEnvironment = allocateEnvironmentLegacy();
	}
}

#else

void Environment::setEs2()
{
	if( ! sEnvironment ) {
		sEnvironment = allocateEnvironmentEs2();
	}
}

#endif

Environment* env()
{
	assert( sEnvironment );
	return sEnvironment;
}

namespace {
void destroyPlatformData( Context::PlatformData *data )
{
#if defined( CINDER_MAC )
	auto platformData = dynamic_cast<PlatformDataMac*>( data );
	::CGLDestroyContext( platformData->mCglContext );
#elif defined( CINDER_COCOA_TOUCH )
	[(EAGLContext*)mPlatformContext release];
#elif defined( CINDER_GL_ANGLE )
#elif defined( CINDER_MSW )
	auto platformData = dynamic_cast<PlatformDataMsw*>( data );
	::wglMakeCurrent( NULL, NULL );
	::wglDeleteContext( platformData->mGlrc );
#endif

	delete data;
}
} // anonymous namespace

ContextRef Environment::createSharedContext( const Context *sharedContext )
{
#if defined( CINDER_MAC )
	auto sharedContextPlatformData = dynamic_pointer_cast<PlatformDataMac>( sharedContext->getPlatformData() );
	CGLContextObj prevContext = ::CGLGetCurrentContext();
	CGLContextObj sharedContextCgl = sharedContextPlatformData->mCglContext;
	CGLPixelFormatObj sharedContextPixelFormat = ::CGLGetPixelFormat( sharedContextCgl );
	CGLContextObj cglContext;
	if( ::CGLCreateContext( sharedContextPixelFormat, sharedContextCgl, (CGLContextObj*)&cglContext ) != kCGLNoError ) {
		throw ExcContextAllocation();
	}

	::CGLSetCurrentContext( cglContext );
	shared_ptr<Context::PlatformData> platformData = shared_ptr<Context::PlatformData>( new PlatformDataMac( cglContext ), destroyPlatformData );	
#elif defined( CINDER_COCOA_TOUCH )
	EAGLContext *prevContext = [EAGLContext currentContext];
	EAGLContext *sharedContextEagl = (EAGLContext*)sharedContext->getPlatformContext();
	EAGLSharegroup *sharegroup = sharedContextEagl.sharegroup;
	platformContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
	[EAGLContext setCurrentContext:(EAGLContext*)platformContext];
#elif defined( CINDER_GL_ANGLE )

#elif defined( CINDER_MSW )
	// save the current context so we can restore it
	HGLRC prevContext = ::wglGetCurrentContext();
	HDC prevDc = ::wglGetCurrentDC();
	auto sharedContextPlatformData = dynamic_pointer_cast<PlatformDataMsw>( sharedContext->getPlatformData() );
	HGLRC sharedContextRc = sharedContextPlatformData->mGlrc;
	HDC sharedContextDc = sharedContextPlatformData->mDc;
	HGLRC rc = ::wglCreateContext( sharedContextDc );
	::wglMakeCurrent( NULL, NULL );
	if( ! ::wglShareLists( sharedContextRc, rc ) ) {
		throw ExcContextAllocation();
	}
	::wglMakeCurrent( sharedContextDc, rc );
	shared_ptr<Context::PlatformData> platformData = shared_ptr<Context::PlatformData>( new PlatformDataMsw( rc, sharedContextDc ), destroyPlatformData );
#endif

	ContextRef result( new Context( platformData ) );
	env()->initializeFunctionPointers();

#if defined( CINDER_MAC )
	::CGLSetCurrentContext( prevContext );
#elif defined( CINDER_COCOA_TOUCH )
	[EAGLContext setCurrentContext:prevContext];
#elif defined( CINDER_MSW )
	::wglMakeCurrent( prevDc, prevContext );
#endif

	return result;
}

void Environment::makeContextCurrent( const Context *context )
{
#if defined( CINDER_MAC )
	auto platformData = dynamic_pointer_cast<PlatformDataMac>( context->getPlatformData() );
	::CGLSetCurrentContext( platformData->mCglContext );
#elif defined( CINDER_COCOA_TOUCH )
	[EAGLContext setCurrentContext:(EAGLContext*)mPlatformContext];
#elif defined( CINDER_MSW )
	auto platformData = dynamic_pointer_cast<PlatformDataMsw>( context->getPlatformData() );
	if( ! ::wglMakeCurrent( platformData->mDc, platformData->mGlrc ) ) {
		// DWORD error = GetLastError();
	}
#endif
}

} } // namespace cinder::gl