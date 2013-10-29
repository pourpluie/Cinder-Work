#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
VboRef Vbo::create( GLenum target )
{
	return VboRef( new Vbo( target ) );
}

VboRef Vbo::create( GLenum target, GLsizeiptr allocationSize, const void *data, GLenum usage )
{
	return VboRef( new Vbo( target, allocationSize, data, usage ) );
}
	
Vbo::Vbo( GLenum target )
	: BufferObj( target )
{
}

Vbo::Vbo( GLenum target, GLsizeiptr allocationSize, const void *data, GLenum usage )
	: BufferObj( target, allocationSize, data, usage )
{
}

} } // namespace cinder::gl
