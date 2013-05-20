#pragma once

// For 
#if defined( GL_PIXEL_UNPACK_BUFFER )

#include "gl/BufferObj.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class Pbo> PboRef;

class Pbo : public BufferObj
{
public:
	static PboRef	create( GLenum target = GL_PIXEL_UNPACK_BUFFER );
protected:
	explicit Pbo( GLenum target );
};
	
} }

#endif
