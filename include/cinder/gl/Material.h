#pragma once

#include "cinder/Color.h"

namespace cinder { namespace gl {

class Material
{
public:
	Material( const ci::ColorAf &color = ci::ColorAf::gray( 0.5f ), float ambient = 1.0f,
			 float diffuse = 1.0f, float specular = 1.0f, float emissive = 0.0f );
	
	void				enable( bool enabled = true );
	
	float				getAmbient() const;
	void				setAmbient( float value );
	
	const ci::ColorAf&	getColor() const;
	void				setColor( const ci::ColorAf &color );
	
	float				getDiffuse() const;
	void				setDiffuse( float value );
	
	float				getEmissive() const;
	void				setEmissive( float value );
	
	float				getSpecular() const;
	void				setSpecular( float value );
	
	bool				operator==( const Material& rhs ) const;
	bool				operator!=( const Material& rhs ) const;
	bool				operator<( const Material& rhs ) const;
protected:
	float				mAmbient;
	ci::ColorAf			mColor;
	float				mDiffuse;
	float				mEmissive;
	float				mSpecular;
};
	
} }
