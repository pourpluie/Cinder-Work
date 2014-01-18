#include "cinder/gl/TransformFeedbackObj.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
class TransformFeedbackObjImplSoftware : public TransformFeedbackObj {
  public:
	
	TransformFeedbackObjImplSoftware();
	~TransformFeedbackObjImplSoftware() {}
	
	void bindImpl( class Context *context );
	void unbindImpl( class Context *context );
	void setIndex( int index, BufferObjRef buffer );
	
  protected:
	friend class Context;
};

TransformFeedbackObjRef createTransformFeedbackObjImplSoftware()
{
	return TransformFeedbackObjRef( new TransformFeedbackObjImplSoftware() );
}

TransformFeedbackObjImplSoftware::TransformFeedbackObjImplSoftware()
{
	mId = 0;
}

void TransformFeedbackObjImplSoftware::setIndex( int index, BufferObjRef buffer )
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

void TransformFeedbackObjImplSoftware::bindImpl( Context *context )
{
	for( auto bufferIt = mBufferBases.begin(); bufferIt != mBufferBases.end(); bufferIt++ ) {
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, bufferIt->first, bufferIt->second->getId() );
	}
}

void TransformFeedbackObjImplSoftware::unbindImpl( Context *context )
{
	
}
	
#endif
	
}} // gl // cinder