//
//  XfoImplHardware.cpp
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/18/13.
//
//

#include "Xfo.h"
#include "TestContext.h"
#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {

class XfoImplHardware : public Xfo {
public:
	XfoImplHardware();
	~XfoImplHardware();
	
	void bindImpl( TestContext *context );
	void unbindImpl( TestContext *context );
	void setIndex( int index, VboRef vbo );
	
private:
	
};
	
XfoRef createXfoImplHardware()
{
	return XfoRef( new XfoImplHardware() );
}

XfoImplHardware::XfoImplHardware()
{
	glGenTransformFeedbacks( 1, &mId );
}

XfoImplHardware::~XfoImplHardware()
{
	glDeleteTransformFeedbacks( 1, &mId );
}
	
void XfoImplHardware::bindImpl( TestContext *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, mId );
}
	
void XfoImplHardware::unbindImpl( TestContext *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}
	
void XfoImplHardware::setIndex( int index, VboRef vbo )
{
	auto exists = mBufferBases.find( index );
	if( exists == mBufferBases.end() ) {
		mBufferBases.insert( std::pair<int, VboRef>( index, vbo ) );
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, index, vbo->getId() );
	}
	else {
		
	}
}
	
}}
