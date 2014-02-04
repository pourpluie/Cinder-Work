#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl {

class ShaderDef {
  public:
	ShaderDef();

	ShaderDef&		color();	
	ShaderDef&		texture( const TextureRef &tex = TextureRef() );
	ShaderDef&		texture( GLenum target );

	bool			isTextureSwizzleDefault() const;
	std::string		getTextureSwizzleString() const;	

	bool operator<( const ShaderDef &rhs ) const;
	
  protected:
	bool					mTextureMapping;
	bool					mTextureMappingRectangleArb;
	std::array<GLint,4>		mTextureSwizzleMask;
	
	bool			mColor;
	
	friend class EnvironmentCore;
	friend class EnvironmentLegacy;
	friend class EnvironmentEs2;	
};
	
} } // namespace cinder::gl
