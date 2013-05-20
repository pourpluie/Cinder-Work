#include "cinder/gl/Pbo.h"

// This class reserved for ES2 migration to Core Profile

#if defined( GL_PIXEL_UNPACK_BUFFER )

namespace cinder { namespace gl {
	
PboRef Pbo::create( GLenum target )
{
	return PboRef( new Pbo( target ) );
}
	
Pbo::Pbo( GLenum target )
: BufferObj( target )
{
	mUsage = GL_STREAM_DRAW;
}

} }

#endif
