#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;

namespace cinder { namespace gl {

ShaderDef::ShaderDef()
	: mTextureMapping( false ), mSolidColor( false )
{
}

ShaderDef& ShaderDef::texture()
{
	mTextureMapping = true;
	return *this;
}

ShaderDef& ShaderDef::solidColor()
{
	mSolidColor = true;
	return *this;
}


bool ShaderDef::operator<( const ShaderDef &rhs ) const
{
	if( rhs.mTextureMapping != mTextureMapping )
		return rhs.mTextureMapping;
	if( rhs.mSolidColor != mSolidColor )
		return rhs.mSolidColor;
	
	return false;
}

} } // namespace cinder::gl