/*
 Copyright (c) 2012, The Cinder Project, All rights reserved.

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

#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/Camera.h"
#import <Cocoa/Cocoa.h>

#import "cinder/app/AppImplCocoaRendererGl.h"
#import "cinder/app/CinderView.h"

#import "cinder/app/App.h"
#include "cinder/app/Renderer.h"
#include "cinder/gl/Environment.h"
#include <iostream>

// This is only here so that we can override isOpaque, which is necessary
// for the ScreenSaverView to show it
@interface AppImplCocoaTransparentGlView : NSOpenGLView 
{}
@end

@implementation AppImplCocoaTransparentGlView
- (BOOL)isOpaque
{ return NO; }
// For whatever reasons, rightMouseDown does not get propagated along the normal responder chain
- (void)rightMouseDown:(NSEvent *)theEvent
{
	[[self superview] rightMouseDown:theEvent];
}

@end

@implementation AppImplCocoaRendererGl

- (id)initWithFrame:(NSRect)frame cinderView:(NSView*)aCinderView app:(cinder::app::App*)aApp renderer:(cinder::app::RendererGl*)aRenderer sharedRenderer:(cinder::app::RendererGlRef)sharedRenderer withRetina:(BOOL)retinaEnabled
{
	self = [super init];
//	self = [super initWithFrame:frame cinderView:aCinderView app:aApp];
	cinderView = aCinderView;
	
	renderer = aRenderer;

	cinder::app::RendererGl::Options options = renderer->getOptions();
	NSOpenGLPixelFormat* fmt = [AppImplCocoaRendererGl defaultPixelFormat:options.getAntiAliasing() legacy:(options.getCoreProfile()==false)];
	GLint aaSamples;
	[fmt getValues:&aaSamples forAttribute:NSOpenGLPFASamples forVirtualScreen:0];

	NSRect bounds = NSMakeRect( 0, 0, frame.size.width, frame.size.height );
	view = [[AppImplCocoaTransparentGlView alloc] initWithFrame:bounds pixelFormat:fmt];
if( ! view )
	NSLog( @"Unable to allocate GL view" );

	// if we've been passed a context to share with, replace our NSOpenGLContext with a new one that shares
	if( sharedRenderer ) {
		assert( typeid( *sharedRenderer ) == typeid( cinder::app::RendererGl ) );
		NSOpenGLContext *newContext = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:sharedRenderer->getNsOpenGlContext()];
		[view setOpenGLContext:newContext];
		[newContext release];
	}

	[cinderView addSubview:view];
	
	if( retinaEnabled )
		[view setWantsBestResolutionOpenGLSurface:YES];
	
	if( options.getCoreProfile() )
		cinder::gl::Environment::setCore();
	else
		cinder::gl::Environment::setLegacy();
	
	CGLContextObj cglContext = (CGLContextObj)[[view openGLContext] CGLContextObj];
	auto platformData = std::shared_ptr<cinder::gl::Context::PlatformData>( new cinder::gl::PlatformDataMac( cglContext ) );
	mContext = cinder::gl::Context::createFromExisting( platformData );
	mContext->makeCurrent();

	GLint swapInterval = 1;
	[[view openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

	return self;
}

- (NSOpenGLView*)view
{
	return view;
}

- (CGLContextObj)getCglContext
{
	return (CGLContextObj)[[view openGLContext] CGLContextObj];
}

- (CGLPixelFormatObj)getCglPixelFormat
{
	return (CGLPixelFormatObj)[[view pixelFormat] CGLPixelFormatObj];
}

- (NSOpenGLContext *)getNsOpenGlContext
{
	return [view openGLContext];
}

- (NSOpenGLPixelFormat *)getNSOpenGLPixelFormat
{
	return [view pixelFormat];
}

- (void)makeCurrentContext
{
	mContext->makeCurrent();
}

- (void)flushBuffer
{	
	[[NSOpenGLContext currentContext] flushBuffer];
}

- (void)drawRect:(NSRect)rect
{
	[view drawRect:rect];
}

- (void)setFrameSize:(CGSize)newSize
{
//	[super setFrameSize:newSize];
	[view setFrameSize:NSSizeFromCGSize( newSize )];
}

- (void)defaultResize
{
	NSSize nsSize = [view frame].size;
	NSSize backingSize = [view convertSizeToBacking:nsSize];
	glViewport( 0, 0, backingSize.width, backingSize.height );
	cinder::CameraPersp cam( nsSize.width, nsSize.height, 60.0f );

	ci::gl::setProjection( cam.getProjectionMatrix() );
	ci::gl::setModelView( cam.getModelViewMatrix() );
	ci::gl::scale( 1.0f, -1.0f, 1.0f );           // invert Y axis so increasing Y goes down.
	ci::gl::translate( 0.0f, (float)-nsSize.height, 0.0f );       // shift origin up to upper-left corner.
}

- (BOOL)acceptsFirstResponder
{
	return NO;
}

- (void)dealloc 
{
	[super dealloc];
}

// For whatever reason, NSOpenGLView doesn't seem to get a drawRect call in response to a setNeedsDisplay:YES
- (BOOL)needsDrawRect
{
	return YES;
}

+ (NSOpenGLPixelFormat*)defaultPixelFormat:(int)antialiasLevel legacy:(BOOL)legacy
{
	NSOpenGLPixelFormat *result = nil;
	if( antialiasLevel == cinder::app::RendererGl::AA_NONE ) {
		NSOpenGLPixelFormatAttribute attributes[] = {
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAOpenGLProfile, (legacy)?NSOpenGLProfileVersionLegacy:NSOpenGLProfileVersion3_2Core,
			NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)24,
/*kCGLPFAStencilSize, (CGLPixelFormatAttribute) 8,*/
			(NSOpenGLPixelFormatAttribute)0
		};

		result = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	}
	else {
		NSOpenGLPixelFormatAttribute attributes[] = {
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAOpenGLProfile, (legacy)?NSOpenGLProfileVersionLegacy:NSOpenGLProfileVersion3_2Core,			
			NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)24,
/*	kCGLPFAStencilSize, (CGLPixelFormatAttribute) 8,*/
	        NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute)1, 
			NSOpenGLPFASamples, (NSOpenGLPixelFormatAttribute)cinder::app::RendererGl::sAntiAliasingSamples[antialiasLevel],
			NSOpenGLPFAMultisample,
			(NSOpenGLPixelFormatAttribute)0
		};
		
		result = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	}

	assert( result );
	return [result autorelease];
}

@end
