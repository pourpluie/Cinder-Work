#pragma once

#include "cinder/gl/gl.h"
#include <map>

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
typedef std::shared_ptr<class TransformFeedbackObj> TransformFeedbackObjRef;

class TransformFeedbackObj : public std::enable_shared_from_this<TransformFeedbackObj> {
  public:
	
	static TransformFeedbackObjRef create();
	virtual ~TransformFeedbackObj() {}
	
	//! Binds this Transform Feedback Obj to the system.
	void bind();
	//! Unbinds this Transform Feedback Obj from the system.
	void unbind();
	
	//! Notifies the system to begin capturing Vertices
	void begin( GLenum primitiveMode );
	//! Notifies the system to pause capturing Vertices
	void pause();
	//! Notifies the system to begin capturing Vertices after a pause
	void resume();
	//! Notifies the sytstem you are finished capturing Vertices with this object
	void end();
	
	//! Returns the gl system id for this Transform Feedback Obj
	GLuint	getId() const { return mId; }
	
  protected:
	TransformFeedbackObj() {}
	
	virtual void bindImpl( class Context *context ) = 0;
	virtual void unbindImpl( class Context *context ) = 0;
	
	virtual void setIndex( int index, BufferObjRef buffer ) = 0;
	
	GLuint								mId;
	std::map<GLuint, gl::BufferObjRef>	mBufferBases;
	
	friend class Context;
};
	
#endif
	
} } // cinder // gl