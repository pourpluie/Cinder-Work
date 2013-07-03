#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;

namespace cinder { namespace gl {

ShaderDef::ShaderDef()
	: mTextureMapping( false )
{
}

ShaderDef& ShaderDef::texture()
{
	mTextureMapping = true;
	return *this;
}

bool ShaderDef::operator<( const ShaderDef &rhs ) const
{
	if( rhs.mTextureMapping != mTextureMapping )
		return rhs.mTextureMapping;
	
	return false;
}

} } // namespace cinder::gl