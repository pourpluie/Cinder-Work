//
//  TestContext.h
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/16/13.
//
//

#pragma once

#include "Xfo.h"

namespace cinder { namespace gl {

typedef std::shared_ptr<class TestContext> TestContextRef;
	
class TestContext {
public:
	static TestContextRef create();
	TestContext();
	~TestContext() {}
	
	void xfoBind( XfoRef xfo );
	void bindBufferBase( GLenum target, int index, VboRef vbo );
	
	void beginTransformFeedback( GLenum primitiveMode );
	void pauseTransformFeedback();
	void resumeTransformFeedback();
	void endTransformFeedback();
	
	XfoRef xfoGet();
	
	static TestContextRef testContext()
	{
		static TestContextRef mTestContext;
		if( mTestContext ) {
			std::cout << "in testContext and there's already a testContext" << std::endl;
			return mTestContext;
		}
		else {
			std::cout << "in testContext and there isn't already a testContext" << std::endl;
			mTestContext = TestContext::create();
			return mTestContext;
		}
	}
	
private:
	XfoRef	mCachedXfo;
	XfoRef	mDefaultXfo;
};
	

	
static TestContextRef testContextOverall;

void bindBufferBase( GLenum target, int index, VboRef vbo );
	
void beginTransformFeedback( GLenum primitiveMode );
void endTransformFeedback();
void resumeTransformFeedback();
void pauseTransformFeedback();
	
struct XfoScope : public boost::noncopyable {
	XfoScope( const XfoRef &xfo )
	: mCtx( gl::TestContext::testContext() )
	{
	}
	~XfoScope();
	
private:
	TestContextRef mCtx;
};
	
}}
