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
// Source

namespace { // these are helper functions for copyData() and copyDataMultAdd

template<uint8_t SRCDIM, uint8_t DSTDIM>
void copyDataImpl( const float *srcData, size_t numElements, size_t dstStrideBytes, float *dstData )
{
	static const float sFillerData[4] = { 0, 0, 0, 1 };
	const uint8_t MINDIM = (SRCDIM < DSTDIM) ? SRCDIM : DSTDIM;
	
	if( dstStrideBytes == 0 )
		dstStrideBytes = DSTDIM * sizeof(float);
	
	for( size_t v = 0; v < numElements; ++v ) {
		uint8_t d;
		for( d = 0; d < MINDIM; ++d ) {
			dstData[d] = srcData[d];
		}
		for( ; d < DSTDIM; ++d ) {
			dstData[d] = sFillerData[d];
		}
		srcData += SRCDIM;
		dstData = (float*)((uint8_t*)dstData + dstStrideBytes);
	}
}

template<uint8_t DSTDIM>
void copyDataMultAddImpl( const float *srcData, size_t numElements, size_t dstStrideBytes, float *dstData, const Vec3f &mult, const Vec3f &add )
{
	static const float sFillerData[4] = { 0, 0, 0, 1 };
	const uint8_t MINDIM = (3 < DSTDIM) ? 3 : DSTDIM;
	
	if( dstStrideBytes == 0 )
		dstStrideBytes = DSTDIM * sizeof(float);
	
	for( size_t v = 0; v < numElements; ++v ) {
		uint8_t d;
		for( d = 0; d < MINDIM; ++d ) {
			dstData[d] = srcData[d] * mult[d] + add[d];
		}
		for( ; d < DSTDIM; ++d ) {
			dstData[d] = sFillerData[d];
		}
		srcData += 3;
		dstData = (float*)((uint8_t*)dstData + dstStrideBytes);
	}
}

} // anonymous namespace

void Source::copyData( uint8_t srcDimensions, const float *srcData, size_t numElements, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData )
{
	// we can get away with a memcpy
	if( (srcDimensions == dstDimensions) && (dstStrideBytes == 0) ) {
		memcpy( dstData, srcData, numElements * srcDimensions * sizeof(float) );
	}
	else {
		switch( srcDimensions ) {
			case 2:
				switch( dstDimensions ) {
					case 2: copyDataImpl<2,2>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 3: copyDataImpl<2,3>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 4: copyDataImpl<2,4>( srcData, numElements, dstStrideBytes, dstData ); break;
					default: throw ExcIllegalDestDimensions();
				}
			break;
			case 3:
				switch( dstDimensions ) {
					case 2: copyDataImpl<3,2>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 3: copyDataImpl<3,3>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 4: copyDataImpl<3,4>( srcData, numElements, dstStrideBytes, dstData ); break;
					default: throw ExcIllegalDestDimensions();
				}
			break;
			case 4:
				switch( dstDimensions ) {
					case 2: copyDataImpl<4,2>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 3: copyDataImpl<4,3>( srcData, numElements, dstStrideBytes, dstData ); break;
					case 4: copyDataImpl<4,4>( srcData, numElements, dstStrideBytes, dstData ); break;
					default: throw ExcIllegalDestDimensions();
				}
			break;
			default:
				throw ExcIllegalSourceDimensions();
		}
	}
}

void Source::copyDataMultAdd( uint8_t srcDimensions, const float *srcData, size_t numElements,
		uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec3f &mult, const Vec3f &add )
{
	switch( dstDimensions) {
		case 2: copyDataMultAddImpl<2>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 3: copyDataMultAddImpl<3>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 4: copyDataMultAddImpl<4>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		default: throw ExcIllegalDestDimensions();
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Cube
float Cube::sVertices[24*3] = {  1.0f, 1.0f, 1.0f,   1.0f,-1.0f, 1.0f,	 1.0f,-1.0f,-1.0f,   1.0f, 1.0f,-1.0f,	// +X
								 1.0f, 1.0f, 1.0f,   1.0f, 1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f, 1.0f, 1.0f,	// +Y
								 1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,   1.0f,-1.0f, 1.0f,	// +Z
								-1.0f, 1.0f, 1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f,-1.0f, 1.0f,	// -X
								-1.0f,-1.0f,-1.0f,   1.0f,-1.0f,-1.0f,   1.0f,-1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,	// -Y
								 1.0f,-1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,   1.0f, 1.0f,-1.0f };// -Z

uint16_t Cube::sIndices[6*6] ={	0, 1, 2, 0, 2, 3,
								4, 5, 6, 4, 6, 7,
								8, 9,10, 8, 10,11,
								12,13,14,12,14,15,
								16,17,18,16,18,19,
								20,21,22,20,22,23 };
								
float Cube::sColors[24*3]	={  1,0,0,	1,0,0,	1,0,0,	1,0,0,		// +X = red
								0,1,0,	0,1,0,	0,1,0,	0,1,0,		// +Y = green
								0,0,1,	0,0,1,	0,0,1,	0,0,1,		// +Z = blue
								0,1,1,	0,1,1,	0,1,1,	0,1,1,		// -X = cyan
								1,0,1,	1,0,1,	1,0,1,	1,0,1,		// -Y = purple
								1,1,0,	1,1,0,	1,1,0,	1,1,0 };	// -Z = yellow
								
float Cube::sTexCoords[24*2]={	0,0,	1,0,	1,1,	0,1,
								1,0,	1,1,	0,1,	0,0,
								0,0,	1,0,	1,1,	0,1,							
								1,0,	1,1,	0,1,	0,0,
								1,1,	0,1,	0,0,	1,0,
								1,1,	0,1,	0,0,	1,0 };
								
float Cube::sNormals[24*3]=	{	1,0,0,	1,0,0,	1,0,0,	1,0,0,
								0,1,0,	0,1,0,	0,1,0,	0,1,0,
								0,0,1,	0,0,1,	0,0,1,	0,0,1,
								-1,0,0,	-1,0,0,	-1,0,0,	-1,0,0,
								0,-1,0,	0,-1,0,  0,-1,0,0,-1,0,
								0,0,-1,	0,0,-1,	0,0,-1,	0,0,-1 };


Cube::Cube()
	: mPos( Vec3f::zero() ), mScale( Vec3f::one() )
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

void Cube::copyAttrib( Attrib attr, uint8_t dimensions, size_t stride, float *dest ) const
{
	switch( attr ) {
		case Attrib::POSITION:
			copyDataMultAdd( 3, sVertices, 24, dimensions, stride, dest, mScale, mPos );
		break;
		case Attrib::COLOR:
			copyData( 3, sColors, 24, dimensions, stride, dest );
		break;
		case Attrib::TEX_COORD_0:
			copyData( 2, sTexCoords, 24, dimensions, stride, dest );
		break;
		case Attrib::NORMAL:
			copyData( 3, sNormals, 24, dimensions, stride, dest );
		break;
		default:
			throw ExcMissingAttrib();
	}
}

void Cube::copyIndices( uint16_t *dest ) const
{
	memcpy( dest, sIndices, sizeof(uint16_t) * 36 );
}

void Cube::copyIndices( uint32_t *dest ) const
{
	for( int i = 0; i < 36; ++i )
		dest[i] = sIndices[i];
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
		default:
			return 0;
	}	
}

} } // namespace cinder::geo