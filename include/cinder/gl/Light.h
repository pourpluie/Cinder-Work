#pragma once

#include "cinder/Color.h"

namespace cinder { namespace gl {

class Shader;

class Light
{
public:
	enum : int32_t {
		DIRECTIONAL, POINT, SPOTLIGHT
	} typedef LightType;
	
	Light( LightType type = LightType::DIRECTIONAL, size_t id = 0, const ci::ColorAf &ambient = ci::ColorAf::black(),
		  const ci::ColorAf &diffuse = ci::ColorAf::gray( 0.5f ), const ci::ColorAf &specular = ci::ColorAf::white(),
		  float shine = 50.0f, float constantAttenuation = 0.0f, float linearAttenuation = 0.0f,
		  float quadraticAttenuation = 0.0002f );
	
	void					enable( bool enabled = true );
	
	const ci::ColorAf&		getAmbient() const;
	float					getConstantAttenuation() const;
	const ci::ColorAf&		getDiffuse() const;
	const ci::Vec3f&		getDirection() const;
	size_t					getId() const;
	float					getLinearAttenuation() const;
	const ci::Vec3f&		getPosition() const;
	float					getQuadraticAttenuation() const;
	float					getShine() const;
	const ci::ColorAf&		getSpecular() const;
	LightType				getType() const;
	
	void					setAmbient( const ci::ColorAf& color );
	void					setConstantAttenuation( float value );
	void					setDiffuse( const ci::ColorAf& color );
	void					setDirection( const ci::Vec3f& direction );
	void					setLinearAttenuation( float value );
	void					setPosition( const ci::Vec3f& position );
	void					setQuadraticAttenuation( float value );
	void					setShine( float value );
	void					setSpecular( const ci::ColorAf& color );
	void					setType( LightType type );
	
	bool					operator==( const Light& rhs ) const;
	bool					operator!=( const Light& rhs ) const;
	bool					operator<( const Light& rhs ) const;
protected:
	LightType				mType;
	
	ci::ColorAf				mAmbient;
	ci::ColorAf				mDiffuse;
	ci::ColorAf				mSpecular;
	
	float					mConstantAttenuation;
	float					mLinearAttenuation;
	float					mQuadraticAttenuation;
	float					mShine;
	
	ci::Vec3f				mDirection;
	ci::Vec3f				mPosition;
	
	size_t					mId;
	
	friend class			Shader;
};
	
} }
