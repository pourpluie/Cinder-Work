#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
VboRef Vbo::create( GLenum target )
{
	return VboRef( new Vbo( target ) );
}
	
Vbo::Vbo( GLenum target )
: BufferObj( target )
{
}
	
} }
