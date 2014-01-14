#include "Xfo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
#if ! defined( CINDER_GLES )
	
class XfoImplHardware : public Xfo {
  public:
	XfoImplHardware();
	~XfoImplHardware();
	
	void bindImpl( Context *context );
	void unbindImpl( Context *context );
	void setIndex( int index, BufferObjRef buffer );
	
  private:
	friend class Context;
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

void XfoImplHardware::bindImpl( Context *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, mId );
}

void XfoImplHardware::unbindImpl( Context *context )
{
	glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

void XfoImplHardware::setIndex( int index, BufferObjRef buffer )
{
	auto exists = mBufferBases.find( index );
	if( exists == mBufferBases.end() ) {
		mBufferBases.insert( std::pair<int, BufferObjRef>( index, buffer ) );
		glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer->getId() );
	}
}
	
#endif
	
}} // gl // cinder
