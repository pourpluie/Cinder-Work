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
	//! Analogous to glMapBuffer(). \a access must be \c GL_READ_ONLY, \c GL_WRITE_ONLY, or \c GL_READ_WRITE. On iOS ES 2 only \c GL_WRITE_ONLY_OES is valid.
	void*				map( GLenum access ) const;
	//! Analogous to glMapBufferRange(). On iOS ES 2 only \c GL_WRITE_ONLY_OES is valid.
	void*				mapBufferRange( GLintptr offset, GLsizeiptr length, GLbitfield access ) const;
	void				unmap() const;
#endif
	
	GLuint				getId() const { return mId; }
	size_t				getSize() const;
	
	GLenum				getTarget() const { return mTarget; }
	void				setTarget( GLenum target );
	
	GLenum				getUsage() const;
	void				setUsage( GLenum usage );

	//! Returns the appropriate parameter to glGetIntegerv() for a specific target; ie GL_ARRAY_BUFFER -> GL_ARRAY_BUFFER_BINDING. Returns 0 on failure.
	static GLuint		getBindingConstantForTarget( GLenum target );
	
  protected:
	BufferObj( GLenum target );
	BufferObj( GLenum target, GLsizeiptr allocationSize, const void *data, GLenum usage );
	
	GLuint				mId;
	size_t				mSize;
	GLenum				mTarget;
	GLenum				mUsage;
};
	
} }
