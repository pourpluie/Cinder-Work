#pragma once

#include "cinder/gl/BufferObj.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class Vbo> VboRef;

class Vbo : public BufferObj {
  public:
	static VboRef	create( GLenum target );
	static VboRef	create( GLenum target, GLsizeiptr allocationSize, const void *data = NULL, GLenum usage = GL_STATIC_DRAW );
	template<typename T>
	static VboRef	create( GLenum target, const std::vector<T> &v, GLenum usage = GL_STATIC_DRAW )
		{ return create( target, v.size() * sizeof(T), v.data(), usage ); }
	
  protected:
	Vbo( GLenum target );
	Vbo( GLenum target, GLsizeiptr allocationSize, const void *data, GLenum usage );
};
	
} } // namespace cinder::gl