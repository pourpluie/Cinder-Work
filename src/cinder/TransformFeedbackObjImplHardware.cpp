#include "cinder/gl/TransformFeedbackObj.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
#if ! defined( CINDER_GLES )
	
class TransformFeedbackObjImplHardware : public TransformFeedbackObj {
  public:
	TransformFeedbackObjImplHardware();
	~TransformFeedbackObjImplHardware();
	
	void bindImpl( Context *context );
	void unbindImpl( Context *context );
	void setIndex( int index, BufferObjRef buffer );
	
  private:
	friend class Context;
};

TransformFeedbackObjRef createTransformFeedbackObjImplHardware()
{
	return TransformFeedbackObjRef( new TransformFeedbackObjImplHardware() );
}

TransformFeedbackObjImplHardware::TransformFeedbackObjImplHardware()
{
	glGenTransformFeedbacks( 1, &mId );
}

TransformFeedbackObjImplHardware::~TransformFeedbackObjImplHardware()
{
	glDeleteTransformFeedbacks( 1, &mId );
}

void TransformFeedbackObjImplHardware::bindImpl( Context *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, mId );
}

void TransformFeedbackObjImplHardware::unbindImpl( Context *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

void TransformFeedbackObjImplHardware::setIndex( int index, BufferObjRef buffer )
{
	auto exists = mBufferBases.find( index );
	if( exists == mBufferBases.end() ) {
		mBufferBases.insert( std::pair<int, BufferObjRef>( index, buffer ) );
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer->getId() );
	}
}
	
#endif
	
}} // gl // cinder
