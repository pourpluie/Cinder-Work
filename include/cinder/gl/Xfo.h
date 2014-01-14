#pragma once

#include "cinder/gl/gl.h"
#include <map>

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
typedef std::shared_ptr<class Xfo> XfoRef;

class Xfo : public std::enable_shared_from_this<Xfo>{
  public:
	
	static XfoRef create();
	virtual ~Xfo() {}
	
	void bind();
	void unbind();
	
	void begin( GLenum primitiveMode );
	void pause();
	void resume();
	void end();
	
	GLuint	getId() const { return mId; }
	
  protected:
	Xfo() {}
	
	virtual void bindImpl( class Context *context ) = 0;
	virtual void unbindImpl( class Context *context ) = 0;
	
	virtual void setIndex( int index, BufferObjRef buffer ) = 0;
	
	GLuint								mId;
	std::map<GLuint, gl::BufferObjRef>	mBufferBases;
	
	friend class Context;
};
	
#endif
	
} } // cinder // gl