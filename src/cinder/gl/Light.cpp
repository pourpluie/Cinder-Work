#include "cinder/gl/Light.h"

#include "cinder/gl/Manager.h"

namespace cinder { namespace gl {

using namespace std;

Light::Light( LightType type, size_t id, const ColorAf &ambient,  const ColorAf &diffuse, const ColorAf &specular, float shine,
			 float constantAttenuation, float linearAttenuation, float quadraticAttenuation )
: mAmbient( ambient ), mConstantAttenuation( constantAttenuation ), mDiffuse( diffuse ), mDirection( Vec3f::zero() ),
mId( id ), mLinearAttenuation( linearAttenuation ), mPosition( Vec3f::zero() ), mQuadraticAttenuation( quadraticAttenuation ),
mShine( shine ), mSpecular( specular ), mType( type )
{
}

void Light::enable( bool enabled )
{
	ManagerRef manager	= Manager::get();
	if ( enabled ) {
		manager->mLights.push_back( *this );
	} else {
		for ( vector<Light>::iterator iter = manager->mLights.begin(); iter != manager->mLights.end(); ) {
			if ( *iter == *this ) {
				iter = manager->mLights.erase( iter );
			} else {
				++iter;
			}
		}
	}
}

const ColorAf& Light::getAmbient() const
{
	return mAmbient;
}

float Light::getConstantAttenuation() const
{
	return mConstantAttenuation;
}

const ColorAf& Light::getDiffuse() const
{
	return mDiffuse;
}

const Vec3f& Light::getDirection() const
{
	return mDirection;
}

float Light::getLinearAttenuation() const
{
	return mLinearAttenuation;
}

const Vec3f& Light::getPosition() const
{
	return mPosition;
}

float Light::getQuadraticAttenuation() const
{
	return mQuadraticAttenuation;
}

float Light::getShine() const
{
	return mShine;
}

const ColorAf& Light::getSpecular() const
{
	return mSpecular;
}

Light::LightType Light::getType() const
{
	return mType;
}

void Light::setAmbient( const ColorAf& color )
{
	mAmbient = color;
}

void Light::setConstantAttenuation( float value )
{
	mConstantAttenuation = value;
}

void Light::setDiffuse( const ColorAf& color )
{
	mDiffuse = color;
}

void Light::setDirection( const Vec3f& direction )
{
	mDirection = direction;
}

void Light::setLinearAttenuation( float value )
{
	mLinearAttenuation = value;
}

void Light::setPosition( const Vec3f& position )
{
	mPosition = position;
}
void Light::setQuadraticAttenuation( float value )
{
	mQuadraticAttenuation = value;
}

void Light::setShine( float value )
{
	mShine = value;
}

void Light::setSpecular( const ColorAf& color )
{
	mSpecular = color;
}

void Light::setType( LightType type )
{
	mType = type;
}
	
bool Light::operator==( const Light& rhs ) const
{
	return mAmbient				== rhs.mAmbient					&&
		mConstantAttenuation	== rhs.mConstantAttenuation		&&
		mDiffuse				== rhs.mDiffuse					&&
		mDirection				== rhs.mDirection				&&
		mLinearAttenuation		== rhs.mLinearAttenuation		&&
		mPosition				== rhs.mPosition				&&
		mQuadraticAttenuation	== rhs.mQuadraticAttenuation	&&
		mShine					== rhs.mShine					&&
		mSpecular				== rhs.mSpecular				&&
		mType					== rhs.mType;
}

bool Light::operator!=( const Light& rhs ) const
{
	return !( *this == rhs );
}

bool Light::operator<( const Light& rhs ) const
{
	return *this != rhs;
}

} }
 