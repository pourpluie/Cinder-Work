/*
 Copyright (c) 2013, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "GeoIo.h"

namespace cinder { namespace geo {

///////////////////////////////////////////////////////////////////////////////////////
// Cube
Cube::Cube()
	: mPos( Vec3f::zero() ), mScale( Vec3f::zero() )
{
	mHasColor = mHasTexCoord0 = mHasNormals = false;
}

bool Cube::hasAttrib( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return true;
		case Attrib::COLOR: return mHasColor;
		case Attrib::TEX_COORD_0: return mHasTexCoord0;
		case Attrib::NORMAL: return mHasNormals;
		default:
			return false;
	}
}

bool Cube::canProvideAttrib( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION:
		case Attrib::COLOR:
		case Attrib::TEX_COORD_0:
		case Attrib::NORMAL:
			return true;
		default:
			return false;
	}
}

void Cube::copyAttrib( Attrib attr, uint8_t dimensions, size_t stride, void *dest ) const
{
	if( ! canProvideAttrib( attr ) )
		throw ExcMissingAttrib();
	
	switch( attr ) {
		case Attrib::POSITION:
//			GeoSource::copyData( 
		break;
	}
}

uint8_t	Cube::getAttribDims( Attrib attr ) const
{
	if( ! canProvideAttrib( attr ) )
		return 0;

	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::COLOR: return 3;
		case Attrib::TEX_COORD_0: return 2;
		case Attrib::NORMAL: return 3;
	}	
}

} } // namespace cinder::geo