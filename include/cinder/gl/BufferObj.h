#pragma once

#include "cinder/gl/gl.h"
#include <memory>

namespace cinder { namespace gl {

typedef std::shared_ptr<class BufferObj>	BufferObjRef;

class BufferObj {
  public:
	~BufferObj();

	void				bind() const;
	void				unbind() const;
	
	void				bufferData( GLsizeiptr size, const GLvoid *data, GLenum usage );
	void				bufferSubData( GLintptr offset, GLsizeiptr size, const GLvoid *data );

#if ! defined( CINDER_GL_ANGLE )	
	uint8_t*			map( GLenum access ) const;
	void				unmap() const;
#endif
	
	GLuint				getId() const { return mId; }
	size_t				getSize() const;
	
	GLenum				getTarget() const { return mTarget; }
	void				setTarget( GLenum target );
	
	GLenum				getUsage() const;
	void				setUsage( GLenum usage );
	
  protected:
	BufferObj( GLenum target );
	BufferObj( GLenum target, GLsizeiptr allocationSize, const void *data = NULL );
	
	GLuint				mId;
	size_t				mSize;
	GLenum				mTarget;
	GLenum				mUsage;
};
	
} }
