#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
VboRef Vbo::create( GLenum target )
{
	return VboRef( new Vbo( target ) );
}

VboRef Vbo::create( GLenum target, GLsizeiptr allocationSize, const void *data )
{
	return VboRef( new Vbo( target, allocationSize, data ) );
}
	
Vbo::Vbo( GLenum target )
	: BufferObj( target )
{
}

Vbo::Vbo( GLenum target, GLsizeiptr allocationSize, const void *data )
	: BufferObj( target, allocationSize, data )
{
}

} } // namespace cinder::gl
