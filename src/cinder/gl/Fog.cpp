#include "cinder/gl/Fog.h"

#include "cinder/gl/Context.h"

namespace cinder { namespace gl {
	
Fog::Fog()
: mColor( ColorAf( 0.5f, 0.5f, 0.5f, 1.0f ) ), mDensity( 1.0f ),
	mEnd( 0.0f ), mScale( 1.0f ), mStart( 0.0f )
{
}

void Fog::enable( bool enabled )
{
	auto ctx		= gl::context();
//	ctx->mFogEnabled	= enabled;
//	ctx->mFog			= enabled ? *this : Fog();
}

const ColorAf& Fog::getColor() const
{
	return mColor;
}

void Fog::setColor( const ColorAf& c )
{
	mColor = c;
}

float Fog::getDensity() const
{
	return mDensity;
}

void Fog::setDensity( float value )
{
	mDensity = value;
}

float Fog::getEnd() const
{
	return mEnd;
}

void Fog::setEnd( float value )
{
	mEnd = value;
}

float Fog::getScale() const
{
	return mScale;
}

void Fog::setScale( float value )
{
	mScale = value;
}

float Fog::getStart() const
{
	return mStart;
}

void Fog::setStart( float value )
{
	mStart = value;
}
	
bool Fog::operator==( const Fog& rhs ) const
{
	return mColor	== rhs.mColor	&&
	mDensity		== rhs.mDensity	&&
	mEnd			== rhs.mEnd		&&
	mScale			== rhs.mScale	&&
	mStart			== rhs.mStart;
}

bool Fog::operator!=( const Fog& rhs ) const
{
	return !( *this == rhs );
}

bool Fog::operator<( const Fog& rhs ) const
{
	return *this != rhs;
}

} }
