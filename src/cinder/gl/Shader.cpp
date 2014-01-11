#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Texture.h"

using namespace std;

namespace cinder { namespace gl {

ShaderDef::ShaderDef()
	: mTextureMapping( false ), mTextureMappingRectangleArb( false ), mColor( false )
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

ShaderDef& ShaderDef::texture( GLenum target )
{
	mTextureMapping = true;
#if ! defined( CINDER_GLES )
	if( target == GL_TEXTURE_RECTANGLE_ARB )
		mTextureMappingRectangleArb = true;
#endif
	return *this;
}

ShaderDef& ShaderDef::color()
{
	mColor = true;
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
	if( rhs.mColor != mColor )
		return rhs.mColor;
	
	return false;
}

} } // namespace cinder::gl