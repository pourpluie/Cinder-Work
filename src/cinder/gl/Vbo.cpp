#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
VboRef Vbo::create( GLenum target )
{
	return VboRef( new Vbo( target ) );
}

VboRef Vbo::create( GLenum target, GLsizeiptr allocationSize )
{
	return VboRef( new Vbo( target, allocationSize ) );
}
	
Vbo::Vbo( GLenum target )
	: BufferObj( target )
{
}

Vbo::Vbo( GLenum target, GLsizeiptr allocationSize )
	: BufferObj( target, allocationSize )
{
}

} } // namespace cinder::gl
