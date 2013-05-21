#pragma once

#include "cinder/gl/BufferObj.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class Vbo> VboRef;

class Vbo : public BufferObj {
  public:
	static VboRef	create( GLenum target );
	static VboRef	create( GLenum target, GLsizeiptr allocationSize );

  protected:
	explicit Vbo( GLenum target );
	explicit Vbo( GLenum target, GLsizeiptr allocationSize );
};
	
} } // namespace cinder::gl