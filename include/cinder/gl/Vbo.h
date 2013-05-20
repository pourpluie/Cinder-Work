#pragma once

#include "cinder/gl/BufferObj.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class Vbo> VboRef;

class Vbo : public BufferObj
{
public:
	static VboRef	create( GLenum target = GL_ARRAY_BUFFER );
protected:
	explicit Vbo( GLenum target );
};
	
} }
