
#include "cinder/gl/BufferTexture.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {
	
#if ! defined( CINDER_GLES )
	
BufferTextureRef BufferTexture::create( const BufferObjRef &buffer, GLenum internalFormat )
{
	return BufferTextureRef( new BufferTexture( buffer, internalFormat ) );
}

BufferTextureRef BufferTexture::create( const void *data, size_t numBytes, GLenum internalFormat, GLenum usage )
{
	gl::BufferObjRef buffer = BufferObj::create( GL_TEXTURE_BUFFER, numBytes, data, usage );
	return BufferTexture::create( buffer, internalFormat );
}

BufferTexture::BufferTexture( const BufferObjRef &buffer, GLenum internalFormat​ )
: mTarget( GL_TEXTURE_BUFFER )
{
	glGenTextures( 1, &mId );
	setBuffer( buffer, internalFormat​ );
}

BufferTexture::~BufferTexture()
{
	glDeleteTextures( 1, &mId );
}

void BufferTexture::setBuffer( const BufferObjRef &buffer, GLenum internalFormat )
{
	mInternalFormat = internalFormat;
	mBufferObj = buffer;
	gl::context()->bindTexture( mTarget, mId );
	glTexBuffer( mTarget, mInternalFormat, buffer->getId() );
}

void BufferTexture::bindTexture( uint8_t textureUnit )
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, mId );
}

void BufferTexture::unbindTexture( uint8_t textureUnit )
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, 0 );
}
	
#endif
	
} }

