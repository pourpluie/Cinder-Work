#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Texture.h"

using namespace std;

namespace cinder { namespace gl {

ShaderDef::ShaderDef()
	: mTextureMapping( false ), mTextureMappingRectangleArb( false ), mColor( false ), mTextureSwizzleMask( { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } )
{
}

ShaderDef& ShaderDef::texture( const TextureRef &texture )
{
	mTextureMapping = true;
#if ! defined( CINDER_GLES )
	if( texture && texture->getTarget() == GL_TEXTURE_RECTANGLE_ARB )
		mTextureMappingRectangleArb = true;
#endif
	if( texture && ( ! TextureBase::supportsHardwareSwizzle() ) )
		mTextureSwizzleMask = texture->getSwizzleMask();

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

bool ShaderDef::isTextureSwizzleDefault() const
{
	return mTextureSwizzleMask[0] == GL_RED &&
			mTextureSwizzleMask[1] == GL_GREEN && 
			mTextureSwizzleMask[2] == GL_BLUE && 
			mTextureSwizzleMask[3] == GL_ALPHA;
}

// this only works with RGBA values
std::string ShaderDef::getTextureSwizzleString() const
{
	string result;
	for( int i = 0; i < 4; ++i ) {
		if( mTextureSwizzleMask[i] == GL_RED )
			result += "r";
		else if( mTextureSwizzleMask[i] == GL_GREEN )
			result += "g";
		else if( mTextureSwizzleMask[i] == GL_BLUE )
			result += "b";
		else if( mTextureSwizzleMask[i] == GL_GREEN )
			result += "a";
	}
	
	return result;
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
	else if( rhs.mTextureSwizzleMask[0] != mTextureSwizzleMask[0] )
		return mTextureSwizzleMask[0] < rhs.mTextureSwizzleMask[0];
	else if( rhs.mTextureSwizzleMask[1] != mTextureSwizzleMask[1] )
		return mTextureSwizzleMask[1] < rhs.mTextureSwizzleMask[1];	
	else if( rhs.mTextureSwizzleMask[2] != mTextureSwizzleMask[2] )
		return mTextureSwizzleMask[2] < rhs.mTextureSwizzleMask[2];	
	else if( rhs.mTextureSwizzleMask[3] != mTextureSwizzleMask[3] )
		return mTextureSwizzleMask[3] < rhs.mTextureSwizzleMask[3];	
	
	return false;
}

} } // namespace cinder::gl