#pragma once

#include "cinder/Color.h"

namespace cinder { namespace gl {
	
class Fog
{
public:
	Fog();
	
	void			enable( bool enabled = true );
	
	const ColorAf&	getColor() const;
	void			setColor( const ColorAf& c );
	
	float			getDensity() const;
	void			setDensity( float value );
	
	float			getEnd() const;
	void			setEnd( float value );
	
	float			getScale() const;
	void			setScale( float value );
	
	float			getStart() const;
	void			setStart( float value );
	
	bool			operator==( const Fog& rhs ) const;
	bool			operator!=( const Fog& rhs ) const;
	bool			operator<( const Fog& rhs ) const;
protected:
	ColorAf			mColor;
	float			mDensity;
	float			mEnd;
	float			mScale;
	float			mStart;
};

} }
