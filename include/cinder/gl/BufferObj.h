#pragma once

#include "cinder/gl/gl.h"
#include <memory>

namespace cinder { namespace gl {

typedef std::shared_ptr<class BufferObj> BufferObjRef;

class BufferObj
{
public:
	static BufferObjRef	create( GLenum target = GL_ARRAY_BUFFER );
	~BufferObj();

	void				bind() const;
	void				unbind() const;
	
	void				bufferData( const GLvoid* data, GLuint size, GLenum usage );
	void				bufferSubData( const GLvoid* data, GLsizeiptr size, GLintptr offset = 0 ) const;
	
	uint8_t*			map( GLenum access ) const;
	void				unmap() const;
	
	GLuint				getId() const;
	size_t				getSize() const;
	
	GLenum				getTarget() const;
	void				setTarget( GLenum target );
	
	GLenum				getUsage() const;
	void				setUsage( GLenum usage );
	
protected:
	explicit BufferObj( GLenum target );
	
	GLuint				mId;
	size_t				mSize;
	GLenum				mTarget;
	GLenum				mUsage;
};
	
} }
