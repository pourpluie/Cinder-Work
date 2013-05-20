#include "cinder/gl/BufferObj.h"

namespace cinder { namespace gl {

BufferObjRef BufferObj::create( GLenum target )
{
	return BufferObjRef( new BufferObj( target ) );
}

BufferObj::BufferObj( GLenum target )
: mId( 0 ), mSize( 0 ), mTarget( target )
{
	glGenBuffers( 1, &mId );
}

BufferObj::~BufferObj()
{
	glDeleteBuffers( 1, &mId );
}

void BufferObj::bind() const
{
	glBindBuffer( mTarget, mId );
}

void BufferObj::bufferData( const GLvoid* data, GLuint size, GLenum usage )
{
	bind();
	mSize = size;
	glBufferData( mTarget, mSize, data, usage );
}
	
void BufferObj::bufferSubData( const GLvoid* data, GLsizeiptr size, GLintptr offset ) const
{
	bind();
	glBufferSubData( mTarget, offset, size, data );
}
	
uint8_t* BufferObj::map( GLenum access ) const
{
	bind();
#if defined( CINDER_GLES )
	return reinterpret_cast<uint8_t*>( glMapBufferOES( mTarget, access ) );
#else
	return reinterpret_cast<uint8_t*>( glMapBuffer( mTarget, access ) );
#endif
}

void BufferObj::unmap() const
{
	bind();
#if defined( CINDER_GLES )	
	GLboolean result = glUnmapBufferOES( mTarget );
#else
	GLboolean result = glUnmapBuffer( mTarget );
#endif
	if ( result != GL_TRUE ) {
		//throw BufferFailedUnmapExc();
	}
}

size_t BufferObj::getSize() const
{
	return mSize;
}
	
GLenum BufferObj::getTarget() const
{
	return mTarget;
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
	glBindBuffer( mTarget, 0 );
}
	
} }
