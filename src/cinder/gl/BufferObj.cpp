#include "cinder/gl/BufferObj.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

BufferObj::BufferObj( GLenum target )
	: mId( 0 ), mSize( 0 ), mTarget( target )
{
	glGenBuffers( 1, &mId );
}

BufferObj::BufferObj( GLenum target, GLsizeiptr allocationSize )
	: mId( 0 ), mTarget( target ), mSize( allocationSize )
{
	glGenBuffers( 1, &mId );
	if( allocationSize > 0 ) {
		auto bufferBind( context()->bufferPush( mTarget, mId ) );
		glBufferData( mTarget, allocationSize, NULL, GL_DYNAMIC_DRAW );
	}
}

BufferObj::~BufferObj()
{
	glDeleteBuffers( 1, &mId );
}

void BufferObj::bind() const
{
	context()->bufferBind( mTarget, mId );
}

void BufferObj::bufferData( const GLvoid *data, GLsizeiptr size, GLenum usage )
{
	auto bufferBind( context()->bufferPush( mTarget, mId ) );
	mSize = size;
	glBufferData( mTarget, mSize, data, usage );
}
	
void BufferObj::bufferSubData( const GLvoid *data, GLsizeiptr size, GLintptr offset )
{
	auto bufferBind( context()->bufferPush( mTarget, mId ) );
	glBufferSubData( mTarget, offset, size, data );
}
	
uint8_t* BufferObj::map( GLenum access ) const
{
	auto bufferBind( context()->bufferPush( mTarget, mId ) );
#if defined( CINDER_GLES )
	return reinterpret_cast<uint8_t*>( glMapBufferOES( mTarget, access ) );
#else
	return reinterpret_cast<uint8_t*>( glMapBuffer( mTarget, access ) );
#endif
}

void BufferObj::unmap() const
{
	auto bufferBind( context()->bufferPush( mTarget, mId ) );
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
	context()->bufferBind( mTarget, 0 );
}
	
} }
