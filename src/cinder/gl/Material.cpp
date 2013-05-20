#include "cinder/gl/Material.h"

#include "cinder/gl/Manager.h"

namespace cinder { namespace gl {

Material::Material( const ColorAf& color, float ambient, float diffuse, float specular, float emissive )
: mAmbient( ambient ), mColor( color ), mDiffuse( diffuse ), mEmissive( emissive ), mSpecular( specular )
{
}
	
void Material::enable( bool enabled )
{
	ManagerRef manager			= Manager::get();
	manager->mMaterialEnabled	= enabled;
	manager->mMaterial			= enabled ? *this : Material();
}

float Material::getAmbient() const
{
	return mAmbient;
}

void Material::setAmbient( float value )
{
	mAmbient = value;
}

const ColorAf& Material::getColor() const
{
	return mColor;
}

void Material::setColor( const ColorAf& color )
{
	mColor = color;
}

float Material::getDiffuse() const
{
	return mDiffuse;
}

void Material::setDiffuse( float value )
{
	mDiffuse = value;
}

float Material::getEmissive() const
{
	return mEmissive;
}

void Material::setEmissive( float value )
{
	mEmissive = value;
}
	
float Material::getSpecular() const
{
	return mSpecular;
}
	
void Material::setSpecular( float value )
{
	mSpecular = value;
}
	
bool Material::operator==( const Material& rhs ) const
{
	return mAmbient	== rhs.mAmbient		&&
	mColor			== rhs.mColor		&&
	mDiffuse		== rhs.mDiffuse		&&
	mEmissive		== rhs.mEmissive	&&
	mSpecular		== rhs.mSpecular;
}

bool Material::operator!=( const Material& rhs ) const
{
	return !( *this == rhs );
}

bool Material::operator<( const Material& rhs ) const
{
	return *this != rhs;
}

} }