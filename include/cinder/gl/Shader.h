#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl {

class EnvironmentCoreProfile;
class EnvironmentEs2;

typedef std::shared_ptr<class Shader> ShaderRef;
	
class ShaderDef {
  public:
	ShaderDef();
	
	ShaderDef&		texture();

	bool operator<( const ShaderDef &rhs ) const;
	
  protected:
	bool			mTextureMapping;
	
	friend EnvironmentCoreProfile;
	friend EnvironmentEs2;	
};
	
} } // namespace cinder::gl
