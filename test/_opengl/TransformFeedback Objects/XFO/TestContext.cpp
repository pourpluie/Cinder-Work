//
//  TestContext.cpp
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/16/13.
//
//

#include "TestContext.h"

namespace cinder { namespace gl {
	
TestContextRef TestContext::create()
{
	return TestContextRef(new TestContext);
}
	
TestContext::TestContext()
: mCachedXfo( nullptr ), mDefaultXfo( nullptr )
{
}
	
void TestContext::xfoBind( XfoRef xfo )
{
	std::cout << "in xfobind and mCachedXfo is " << mCachedXfo << std::endl;
	if( xfo != mCachedXfo ) {
		if( mCachedXfo )
			mCachedXfo->unbindImpl( this );
		if( xfo ) {
			xfo->bindImpl( this );
		}
		
		mCachedXfo = xfo;
		std::cout << "in xfoBind and mCachedXfo is now " << mCachedXfo << std::endl;
	}
}
	
XfoRef TestContext::xfoGet()
{
	return mCachedXfo;
}

void TestContext::bindBufferBase( GLenum target, int index, VboRef vbo )
{
	std::cout << "in bindbufferbase and mCachedXfo: " << mCachedXfo << std::endl;
	switch (target) {
		case GL_TRANSFORM_FEEDBACK_BUFFER: {
			if( mCachedXfo ) {
				std::cout << "about to setIndex for cachedXfo: " << mCachedXfo << std::endl;
				mCachedXfo->setIndex( index, vbo );
			}
			else {
				std::cout << "it doesn't think there's a cachedXfo so it's about to make a sysXfo" << std::endl;
				if( mDefaultXfo ) {
					mDefaultXfo->setIndex( index, vbo );
				}
				else {
					mDefaultXfo = gl::Xfo::create( true );
					mDefaultXfo->setIndex( index, vbo );
				}
				mCachedXfo = mDefaultXfo;
			}
		}
		break;
		case GL_UNIFORM_BUFFER: {
			// Soon to implement
		}
		break;
		default: {
			
		}
		break;
	}
	
}
	
void TestContext::beginTransformFeedback( GLenum primitiveMode )
{
	if( mCachedXfo ) {
		mCachedXfo->begin( primitiveMode );
	}
}

void TestContext::pauseTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->pause();
	}
}

void TestContext::resumeTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->resume();
	}
}

void TestContext::endTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->end();
	}
}
	
void beginTransformFeedback( GLenum primitiveMode )
{
	TestContext::testContext()->beginTransformFeedback( primitiveMode );
}
	
void pauseTransformFeedback()
{
	TestContext::testContext()->pauseTransformFeedback();
}
	
void resumeTransformFeedback()
{
	TestContext::testContext()->resumeTransformFeedback();
}
	
void endTransformFeedback()
{
	TestContext::testContext()->endTransformFeedback();
}
	

	
void bindBufferBase( GLenum target, int index, VboRef vbo )
{
	TestContext::testContext()->bindBufferBase( target, index, vbo );
}
	
}
}