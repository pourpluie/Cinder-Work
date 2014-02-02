
#include "cinder/gl/Tbo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {
	
#if ! defined( CINDER_GLES )
	
TboRef Tbo::create( const BufferObjRef &buffer, GLenum internalFormat )
{
	return TboRef( new Tbo( buffer, internalFormat ) );
}

TboRef Tbo::create( const void *data, size_t numBytes, GLenum internalFormat, GLenum usage )
{
	gl::BufferObjRef buffer = BufferObj::create( GL_TEXTURE_BUFFER, numBytes, data, usage );
	return Tbo::create( buffer, internalFormat );
}

Tbo::Tbo( const BufferObjRef &buffer, GLenum internalFormat​ )
: mTarget( GL_TEXTURE_BUFFER )
{
	glGenTextures( 1, &mId );
	setBuffer( buffer, internalFormat​ );
}

Tbo::~Tbo()
{
	glDeleteTextures( 1, &mId );
}

void Tbo::setBuffer( const BufferObjRef &buffer, GLenum internalFormat )
{
	mInternalFormat = internalFormat;
	mBufferObj = buffer;
	gl::context()->bindTexture( mTarget, mId );
	glTexBuffer( mTarget, mInternalFormat, buffer->getId() );
}

void Tbo::bindTexture( uint8_t textureUnit )
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, mId );
}

void Tbo::unbindTexture( uint8_t textureUnit )
{
	ActiveTextureScope activeTextureScope( textureUnit );
	gl::context()->bindTexture( mTarget, 0 );
}
	
#endif
	
} }

