
#pragma once

#include "cinder/gl/BufferObj.h"

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
typedef std::shared_ptr<class Tbo> TboRef;

class Tbo {
  public:
	
	static TboRef create( const BufferObjRef &buffer, GLenum internalFormat​ );
	static TboRef create( const void *data, size_t numBytes, GLenum internalFormat, GLenum usage = GL_STATIC_DRAW );
	
	Tbo( const BufferObjRef &buffer, GLenum internalFormat );
	~Tbo();
	
	//! Binds the underlying Texture representation of your BufferObj
	void bindTexture( uint8_t textureUnit = 0  );
	//! Unbinds the underlying Texture representation of your BufferObj
	void unbindTexture( uint8_t textureUnit = 0 );
	
	//! Set or Reset the BufferObj this Texture Buffer is associated with
	void setBuffer( const BufferObjRef &buffer, GLenum internalFormat );
	
	//! Returns the gl system id for this Texture Buffer
	GLuint				getId() { return mId; }
	//! Returns the target for the Texture Buffer. \c GL_TEXTURE_BUFFER is the only allowable target.
	GLenum				getTarget() { return mTarget; }
	//! Returns the internalFormat of the Texture Buffer. It represents the way your BufferObj data is built.
	GLint				getInternalFormat() { return mInternalFormat; }
	//! Returns the BufferObj associated with this Texture Buffer currently.
	BufferObjRef&		getBufferObj() { return mBufferObj; }
	const BufferObjRef& getBufferObj() const { return mBufferObj; }
	
  private:
	GLenum			mTarget;
	GLuint			mId;
	mutable GLenum	mInternalFormat;
	BufferObjRef	mBufferObj;
};
	
#endif
	
} } // gl // cinder
