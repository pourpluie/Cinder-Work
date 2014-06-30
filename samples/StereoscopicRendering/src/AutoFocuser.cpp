/*
 Copyright (c) 2012-2014, The Cinder Project, All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org
 
 Portions of this code (C) Paul Houx
 All rights reserved.

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

#include "AutoFocuser.h"
#include "cinder/Cinder.h"
#include "cinder/Camera.h"

using namespace ci;

void AutoFocuser::autoFocus( CameraStereo *cam, const Area &area, GLuint buffer )
{
	if( ! cam->isStereoEnabled() )
		return;

	// Create or resize buffers
	createBuffers( area );

	// Blit (multi-sampled) depth buffer to (non-multi-sampled) auto focus buffer
	// (they need to be the same size for this to work!)
	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, buffer );
	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, mFboLarge->getId() );
	glBlitFramebufferEXT( area.x1, area.y1, area.x2, area.y2, 
		0, 0, area.x2 - area.x1, area.y2 - area.y1, GL_DEPTH_BUFFER_BIT, GL_NEAREST );

	// Create a downsampled copy for the auto focus test
	glBindFramebuffer( GL_READ_FRAMEBUFFER, mFboLarge->getId() );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFboSmall->getId() );
	glBlitFramebuffer( 0, 0, area.x2 - area.x1, area.y2 - area.y1,
		0, 0, AF_WIDTH, AF_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST );

	// Load depth samples into buffer
	glBindFramebuffer( GL_READ_FRAMEBUFFER, mFboSmall->getId() );
	glReadPixels(0, 0, AF_WIDTH, AF_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, &mBuffer.front());

	// Restore currently bound buffer
	glBindFramebuffer( GL_READ_FRAMEBUFFER, buffer );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, buffer );
	
	// Find minimum value 
	std::vector<GLfloat>::const_iterator itr = std::min_element(mBuffer.begin(), mBuffer.end());

	size_t p = itr - mBuffer.begin();
	
	// Convert to actual distance from camera
	float nearClip = cam->getNearClip();
	float farClip = cam->getFarClip();
	float depth = 2.0f * (*itr) - 1.0f;
	float z = 2.0f * nearClip * farClip / ( farClip + nearClip - depth * ( farClip - nearClip ) );
	
	// Perform auto focussing
	z = math<float>::max( nearClip, z * mDepth );
	cam->setConvergence( cam->getConvergence() + mSpeed * ( z - cam->getConvergence() ), true );
}

void AutoFocuser::createBuffers( const Area &area )
{
	int width = area.getWidth();
	int height = area.getHeight();

	if( !mFboLarge || !mFboSmall || mFboLarge->getWidth() != width || mFboLarge->getHeight() != height )
	{
		gl::Fbo::Format fmt;
		fmt.enableColorBuffer(false);
		fmt.enableDepthBuffer(true);

		mFboLarge = gl::Fbo::create( width, height, fmt );
		mFboSmall = gl::Fbo::create( AF_WIDTH, AF_HEIGHT, fmt );
	}

	int size = AF_WIDTH * AF_HEIGHT;
	if( mBuffer.size() != size )
	{
		mBuffer.resize( size );
		mBuffer.assign( size, 0.0f );
	}
}