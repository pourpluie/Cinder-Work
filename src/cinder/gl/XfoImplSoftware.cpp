#include "Xfo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vbo.h"
#include <iostream>

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
class XfoImplSoftware : public Xfo {
  public:
	
	XfoImplSoftware();
	~XfoImplSoftware() {}
	
	void bindImpl( class Context *context );
	void unbindImpl( class Context *context );
	void setIndex( int index, BufferObjRef buffer );
	
  protected:
	friend class Context;
};

XfoRef createXfoImplSoftware()
{
	return XfoRef( new XfoImplSoftware() );
}

XfoImplSoftware::XfoImplSoftware()
{
	mId = 0;
}

void XfoImplSoftware::setIndex( int index, BufferObjRef buffer )
{
	bool changed = false;
	
	auto exists = mBufferBases.find( index );
	if( exists == mBufferBases.end() ) {
		mBufferBases.insert( std::pair<int, BufferObjRef>( index, buffer ) );
		changed = true;
	}
	else {
		if( exists->second != buffer ) {
			exists->second = buffer;
			changed = true;
		}
	}
	
	if( changed ) {
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer->getId() );
	}
}

void XfoImplSoftware::bindImpl( Context *context )
{
	for( auto bufferIt = mBufferBases.begin(); bufferIt != mBufferBases.end(); bufferIt++ ) {
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, bufferIt->first, bufferIt->second->getId() );
	}
}

void XfoImplSoftware::unbindImpl( Context *context )
{
	
}
	
#endif
	
}} // gl // cinder