//
//  XfoImplSoftware.cpp
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/16/13.
//
//

// Concrete implementation of XFO using "software" emulation

#include "Xfo.h"
#include "TestContext.h"
#include "cinder/gl/Vbo.h"
#include <iostream>

#include <map>
using namespace std;

namespace cinder { namespace gl {

class XfoImplSoftware : public Xfo {
public:
	
	XfoImplSoftware();
	~XfoImplSoftware() {}
	
	void bindImpl( class TestContext *context );
	void unbindImpl( class TestContext *context );
	void setIndex( int index, VboRef vbo );
	
protected:
	friend class TestContext;
};

XfoRef createXfoImplSoftware()
{
	cout << "creating the software version" << endl;
	return XfoRef( new XfoImplSoftware() );
}

XfoImplSoftware::XfoImplSoftware()
{
	mId = 0;
}
	
void XfoImplSoftware::setIndex( int index, VboRef vbo )
{
	bool changed = false;
	auto exists = mBufferBases.find( index );
	if( exists == mBufferBases.end() ) {
		mBufferBases.insert( std::pair<int, VboRef>( index, vbo ) );
		changed = true;
	}
	else {
		if( exists->second != vbo ) {
			exists->second = vbo;
			changed = true;
		}
	}
	if( changed && mAlreadyBound ) {
		std::cout << "Index has been changed and therefore I'm binding it again" << std::endl;
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, index, vbo->getId() );
	}
}
	
void XfoImplSoftware::bindImpl( TestContext *context )
{
	cout << "in software bindImpl" << endl;
	mAlreadyBound = true;
	for( auto bufferIt = mBufferBases.begin(); bufferIt != mBufferBases.end(); bufferIt++ ) {
		cout << "I'm in bindImpl and I'm binding bufferbase" << endl;
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, bufferIt->first, bufferIt->second->getId() );
	}
}
	
void XfoImplSoftware::unbindImpl( TestContext *context )
{
	mAlreadyBound = false;
}
	
}}
