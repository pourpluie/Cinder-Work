#pragma once

#include "cinder/app/App.h"
#include "cinder/Exception.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

namespace cinder { namespace gl {
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<class Fbo> FboRef;

class Fbo
{
	
public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//! Creates a FBO
	static FboRef   create( int32_t width, int32_t height, int32_t msaaSamples = 0 );
	
	~Fbo();
	
	void            bindFramebuffer();
	void            bindFramebuffer() const;
	void            unbindFramebuffer();
	void            unbindFramebuffer() const;
	
	void            bindTexture( int32_t textureUnit = 0 );
	void            bindTexture( int32_t textureUnit = 0 ) const;
	void            unbindTexture();
	void            unbindTexture() const;
	
	TextureRef		getTexture();
	TextureRef		getTexture() const;
	
	int32_t         getHeight();
	int32_t         getHeight() const;
	int32_t         getWidth();
	int32_t         getWidth() const;
	
	ci::Area        getBounds();
	ci::Area        getBounds() const;
	ci::Vec2i       getSize();
	ci::Vec2i       getSize() const;
	
private:
	Fbo( int32_t width, int32_t height, int32_t msaaSamples );
	
	void            checkStatus();
	void            checkStatus() const;
	
	int32_t         mMsaaSamples;
	
	int32_t         mHeight;
	int32_t         mWidth;
	
	TextureRef		mColorTexture;
	TextureRef		mDepthTexture;
	
	GLuint          mColorBuffer;
	GLuint          mDepthBuffer;
	GLuint          mFrameBuffer;
	
	GLuint          mResolveBuffer;
};
    
} } // namespace cinder::gl
