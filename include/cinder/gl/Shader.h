#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl {

class EnvironmentCoreProfile;
class EnvironmentEs2;

class ShaderDef {
  public:
	ShaderDef();
	
	ShaderDef&		texture();
	ShaderDef&		solidColor();

	bool operator<( const ShaderDef &rhs ) const;
	
  protected:
	bool			mTextureMapping;
	bool			mSolidColor;
	
	friend EnvironmentCoreProfile;
	friend EnvironmentEs2;	
};
	
} } // namespace cinder::gl
