#include "cinder/gl/BufferObj.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

BufferObj::BufferObj( GLenum target )
	: mId( 0 ), mSize( 0 ), mTarget( target ),
#if defined( CINDER_GLES )
	mUsage( 0 ) /* GL ES default buffer usage is undefined(?) */
#else
	mUsage( GL_READ_WRITE )
#endif
{
	glGenBuffers( 1, &mId );
}

BufferObj::BufferObj( GLenum target, GLsizeiptr allocationSize, const void *data, GLenum usage )
	: mId( 0 ), mTarget( target ), mSize( allocationSize ), mUsage( usage )
{
	glGenBuffers( 1, &mId );
	
	BufferScope bufferBind( mTarget, mId );
	glBufferData( mTarget, mSize, data, mUsage );
}

BufferObj::~BufferObj()
{
	glDeleteBuffers( 1, &mId );
	
	gl::context()->bufferDeleted( mTarget, mId );
}

void BufferObj::bind() const
{
	context()->bindBuffer( mTarget, mId );
}

void BufferObj::bufferData( GLsizeiptr size, const GLvoid *data, GLenum usage )
{
	BufferScope bufferBind( mTarget, mId );
	mSize = size;
	mUsage = usage;
	glBufferData( mTarget, mSize, data, usage );
}
	
void BufferObj::bufferSubData( GLintptr offset, GLsizeiptr size, const GLvoid *data )
{
	BufferScope bufferBind( mTarget, mId );
	glBufferSubData( mTarget, offset, size, data );
}
	
void BufferObj::getBufferSubData( GLintptr offset, GLsizeiptr size, GLvoid *data )
{
	BufferScope bufferBind( mTarget, mId );
	glGetBufferSubData( mTarget, offset, size, data );
}

void BufferObj::copyData( GLsizeiptr size, const GLvoid *data )
{
	BufferScope bufferBind( mTarget, mId );
	
	if( size <= mSize )
		glBufferSubData( mTarget, 0, size, data );
	else { // need to reallocate due to inadequate size
		mSize = size;
		glBufferData( mTarget, mSize, data, mUsage );
	}
}

void BufferObj::ensureMinimumSize( GLsizeiptr minimumSize )
{
	if( mSize < minimumSize ) {
		mSize = minimumSize;
		BufferScope bufferBind( mTarget, mId );
		glBufferData( mTarget, mSize, NULL, mUsage );
	}
}

#if ! defined( CINDER_GL_ANGLE )	
void* BufferObj::map( GLenum access ) const
{
	BufferScope bufferBind( mTarget, mId );
#if defined( CINDER_GLES )
	return reinterpret_cast<void*>( glMapBufferOES( mTarget, access ) );
#else
	return reinterpret_cast<void*>( glMapBuffer( mTarget, access ) );
#endif
}

void* BufferObj::mapBufferRange( GLintptr offset, GLsizeiptr length, GLbitfield access ) const
{
	BufferScope bufferBind( mTarget, mId );
#if defined( CINDER_GLES )
	return reinterpret_cast<void*>( glMapBufferRangeEXT( mTarget, offset, length, access ) );
#else
	return reinterpret_cast<void*>( glMapBufferRange( mTarget, offset, length, access ) );
#endif
}

void BufferObj::unmap() const
{
	BufferScope bufferBind( mTarget, mId );
#if defined( CINDER_GLES )	
	GLboolean result = glUnmapBufferOES( mTarget );
#else
	GLboolean result = glUnmapBuffer( mTarget );
#endif
	if ( result != GL_TRUE ) {
		//throw BufferFailedUnmapExc();
	}
}
#endif // ! defined( CINDER_GL_ANGLE )

size_t BufferObj::getSize() const
{
	return mSize;
}
	
void BufferObj::setTarget( GLenum target )
{
	mTarget = target;
}
	
GLenum BufferObj::getUsage() const
{
	return mUsage;
}

void BufferObj::setUsage( GLenum usage )
{
	mUsage = usage;
}
	
void BufferObj::unbind() const
{
	context()->bindBuffer( mTarget, 0 );
}

GLuint BufferObj::getBindingConstantForTarget( GLenum target )
{
	switch( target ) {
		case GL_ARRAY_BUFFER:
			return GL_ARRAY_BUFFER_BINDING;
		case GL_ELEMENT_ARRAY_BUFFER:
			return GL_ELEMENT_ARRAY_BUFFER_BINDING;
#if ! defined( CINDER_GLES )
		case GL_PIXEL_PACK_BUFFER:
			return GL_PIXEL_PACK_BUFFER_BINDING;
		case GL_PIXEL_UNPACK_BUFFER:
			return GL_PIXEL_UNPACK_BUFFER_BINDING;
		case GL_TEXTURE_BUFFER:
			return GL_TEXTURE_BINDING_BUFFER;
		case GL_TRANSFORM_FEEDBACK_BUFFER:
			return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
		case GL_UNIFORM_BUFFER:
			return GL_UNIFORM_BUFFER_BINDING;
#endif
		default:
			return 0;
	}
}
	
} } // namespace cinder::gl