#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Texture.h"

using namespace std;

namespace cinder { namespace gl {

ShaderDef::ShaderDef()
	: mTextureMapping( false ), mTextureMappingRectangleArb( false ), mSolidColor( false )
{
}

ShaderDef& ShaderDef::texture( const TextureRef &texture )
{
	mTextureMapping = true;
#if ! defined( CINDER_GLES )
	if( texture && texture->getTarget() == GL_TEXTURE_RECTANGLE_ARB )
		mTextureMappingRectangleArb = true;
#endif
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
#if ! defined( CINDER_GLES )
	if( rhs.mTextureMappingRectangleArb != mTextureMappingRectangleArb )
		return rhs.mTextureMappingRectangleArb;
#endif		
	if( rhs.mSolidColor != mSolidColor )
		return rhs.mSolidColor;
	
	return false;
}

} } // namespace cinder::gl