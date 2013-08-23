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

using namespace std;

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
void copyDataMultAddImpl( const float *srcData, size_t numElements, size_t dstStrideBytes, float *dstData, const Vec2f &mult, const Vec2f &add )
{
	static const float sFillerData[4] = { 0, 0, 0, 1 };
	const uint8_t MINDIM = (2 < DSTDIM) ? 2 : DSTDIM;
	
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
		srcData += 2;
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

void Source::copyDataMultAdd( const float *srcData, size_t numElements,
		uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec3f &mult, const Vec3f &add )
{
	switch( dstDimensions) {
		case 2: copyDataMultAddImpl<2>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 3: copyDataMultAddImpl<3>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 4: copyDataMultAddImpl<4>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		default: throw ExcIllegalDestDimensions();
	}
}

void Source::copyDataMultAdd( const float *srcData, size_t numElements,
		uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec2f &mult, const Vec2f &add )
{
	switch( dstDimensions) {
		case 2: copyDataMultAddImpl<2>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 3: copyDataMultAddImpl<3>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		case 4: copyDataMultAddImpl<4>( srcData, numElements, dstStrideBytes, dstData, mult, add ); break;
		default: throw ExcIllegalDestDimensions();
	}
}

void Source::copyIndices( uint16_t *dest ) const
{
	throw ExcNoIndices();
}

void Source::copyIndices( uint32_t *dest ) const
{
	throw ExcNoIndices();
}


///////////////////////////////////////////////////////////////////////////////////////
// Rect
float Rect::sVertices[4*2] = { 0.5f,-0.5f,	-0.5f,-0.5f,	0.5f,0.5f,	-0.5f,0.5f };
float Rect::sColors[4*3] = { 1, 0, 1,	0, 0, 1,	1, 1, 1,	0, 1, 1 };
float Rect::sTexCoords[4*2] = { 1, 1,	0, 1,		1, 0,		0, 0 };
float Rect::sNormals[4*3] = {0, 0, 1,	0, 0, 1,	0, 0, 1,	0, 0, 1 };

Rect::Rect()
	: mPos( Vec2f::zero() ), mScale( Vec2f::one() )
{
	mHasColor = mHasTexCoord0 = mHasNormals = false;
}

bool Rect::hasAttrib( Attrib attr ) const
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

bool Rect::canProvideAttrib( Attrib attr ) const
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

void Rect::copyAttrib( Attrib attr, uint8_t dimensions, size_t stride, float *dest ) const
{
	switch( attr ) {
		case Attrib::POSITION:
			copyDataMultAdd( sVertices, 4, dimensions, stride, dest, mScale, mPos );
		break;
		case Attrib::COLOR:
			copyData( 3, sColors, 4, dimensions, stride, dest );
		break;
		case Attrib::TEX_COORD_0:
			copyData( 2, sTexCoords, 4, dimensions, stride, dest );
		break;
		case Attrib::NORMAL:
			copyData( 3, sNormals, 4, dimensions, stride, dest );
		break;
		default:
			throw ExcMissingAttrib();
	}
}

uint8_t	Rect::getAttribDims( Attrib attr ) const
{
	if( ! canProvideAttrib( attr ) )
		return 0;

	switch( attr ) {
		case Attrib::POSITION: return 2;
		case Attrib::COLOR: return 3;
		case Attrib::TEX_COORD_0: return 2;
		case Attrib::NORMAL: return 3;
		default:
			return 0;
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
			copyDataMultAdd( sVertices, 24, dimensions, stride, dest, mScale, mPos );
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

///////////////////////////////////////////////////////////////////////////////////////
// Teapot
const uint8_t Teapot::sPatchIndices[][16] = {
	// rim
	{102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	// body
	{12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27}, {24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40},
	// lid
	{96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101, 101, 0, 1, 2, 3,}, {0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117},
	// bottom
	{118, 118, 118, 118, 124, 122, 119, 121, 123, 126, 125, 120, 40, 39, 38, 37},
	// handle
	{41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56}, {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 28, 65, 66, 67},
	// spout
	{68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83}, {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95}
};

const float Teapot::sCurveData[][3] = 
{	{0.2f, 0.f, 2.7f}, {0.2f, -0.112f, 2.7f}, {0.112f, -0.2f, 2.7f}, {0.f, -0.2f, 2.7f}, {1.3375f, 0.f,
	2.53125f}, {1.3375f, -0.749f, 2.53125f}, {0.749f, -1.3375f, 2.53125f}, {0.f, -1.3375f, 2.53125f},
	{1.4375f, 0.f, 2.53125f}, {1.4375f, -0.805f, 2.53125f}, {0.805f, -1.4375f, 2.53125f}, {0.f,
	-1.4375f, 2.53125f}, {1.5f, 0.f, 2.4f}, {1.5f, -0.84f, 2.4f}, {0.84f, -1.5f, 2.4f}, {0.f, -1.5f,
	2.4f}, {1.75f, 0.f, 1.875f}, {1.75f, -0.98f, 1.875f}, {0.98f, -1.75f, 1.875f}, {0.f, -1.75f,
	1.875f}, {2.f, 0.f, 1.35f}, {2.f, -1.12f, 1.35f}, {1.12f, -2.f, 1.35f}, {0.f, -2.f, 1.35f}, {2.f,
	0.f, 0.9f}, {2.f, -1.12f, 0.9f}, {1.12f, -2.f, 0.9f}, {0.f, -2.f, 0.9f}, {-2.f, 0.f, 0.9f}, {2.f,
	0.f, 0.45f}, {2.f, -1.12f, 0.45f}, {1.12f, -2.f, 0.45f}, {0.f, -2.f, 0.45f}, {1.5f, 0.f, 0.225f},
	{1.5f, -0.84f, 0.225f}, {0.84f, -1.5f, 0.225f}, {0.f, -1.5f, 0.225f}, {1.5f, 0.f, 0.15f}, {1.5f,
	-0.84f, 0.15f}, {0.84f, -1.5f, 0.15f}, {0.f, -1.5f, 0.15f}, {-1.6f, 0.f, 2.025f}, {-1.6f, -0.3f,
	2.025f}, {-1.5f, -0.3f, 2.25f}, {-1.5f, 0.f, 2.25f}, {-2.3f, 0.f, 2.025f}, {-2.3f, -0.3f, 2.025f},
	{-2.5f, -0.3f, 2.25f}, {-2.5f, 0.f, 2.25f}, {-2.7f, 0.f, 2.025f}, {-2.7f, -0.3f, 2.025f}, {-3.f,
	-0.3f, 2.25f}, {-3.f, 0.f, 2.25f}, {-2.7f, 0.f, 1.8f}, {-2.7f, -0.3f, 1.8f}, {-3.f, -0.3f, 1.8f},
	{-3.f, 0.f, 1.8f}, {-2.7f, 0.f, 1.575f}, {-2.7f, -0.3f, 1.575f}, {-3.f, -0.3f, 1.35f}, {-3.f, 0.f,
	1.35f}, {-2.5f, 0.f, 1.125f}, {-2.5f, -0.3f, 1.125f}, {-2.65f, -0.3f, 0.9375f}, {-2.65f, 0.f,
	0.9375f}, {-2.f, -0.3f, 0.9f}, {-1.9f, -0.3f, 0.6f}, {-1.9f, 0.f, 0.6f}, {1.7f, 0.f, 1.425f}, {1.7f,
	-0.66f, 1.425f}, {1.7f, -0.66f, 0.6f}, {1.7f, 0.f, 0.6f}, {2.6f, 0.f, 1.425f}, {2.6f, -0.66f,
	1.425f}, {3.1f, -0.66f, 0.825f}, {3.1f, 0.f, 0.825f}, {2.3f, 0.f, 2.1f}, {2.3f, -0.25f, 2.1f},
	{2.4f, -0.25f, 2.025f}, {2.4f, 0.f, 2.025f}, {2.7f, 0.f, 2.4f}, {2.7f, -0.25f, 2.4f}, {3.3f, -0.25f,
	2.4f}, {3.3f, 0.f, 2.4f}, {2.8f, 0.f, 2.475f}, {2.8f, -0.25f, 2.475f}, {3.525f, -0.25f, 2.49375f},
	{3.525f, 0.f, 2.49375f}, {2.9f, 0.f, 2.475f}, {2.9f, -0.15f, 2.475f}, {3.45f, -0.15f, 2.5125f},
	{3.45f, 0.f, 2.5125f}, {2.8f, 0.f, 2.4f}, {2.8f, -0.15f, 2.4f}, {3.2f, -0.15f, 2.4f}, {3.2f, 0.f,
	2.4f}, {0.f, 0.f, 3.15f}, {0.8f, 0.f, 3.15f}, {0.8f, -0.45f, 3.15f}, {0.45f, -0.8f, 3.15f}, {0.f,
	-0.8f, 3.15f}, {0.f, 0.f, 2.85f}, {1.4f, 0.f, 2.4f}, {1.4f, -0.784f, 2.4f}, {0.784f, -1.4f, 2.4f},
	{0.f, -1.4f, 2.4f}, {0.4f, 0.f, 2.55f}, {0.4f, -0.224f, 2.55f}, {0.224f, -0.4f, 2.55f}, {0.f, -0.4f,
	2.55f}, {1.3f, 0.f, 2.55f}, {1.3f, -0.728f, 2.55f}, {0.728f, -1.3f, 2.55f}, {0.f, -1.3f, 2.55f},
	{1.3f, 0.f, 2.4f}, {1.3f, -0.728f, 2.4f}, {0.728f, -1.3f, 2.4f}, {0.f, -1.3f, 2.4f}, {0.f, 0.f,
	0.f}, {1.425f, -0.798f, 0.f}, {1.5f, 0.f, 0.075f}, {1.425f, 0.f, 0.f}, {0.798f, -1.425f, 0.f}, {0.f,
	-1.5f, 0.075f}, {0.f, -1.425f, 0.f}, {1.5f, -0.84f, 0.075f}, {0.84f, -1.5f, 0.075f} };

Teapot::Teapot()
	: mSubdivision( 4 ), mPos( Vec3f::zero() ), mScale( Vec3f::one() ), mCalculationsCached( false )
{
	mHasTexCoord0 = mHasNormals = false;
}

size_t Teapot::getNumVertices() const
{
	calculate();
	
	return mNumVertices;
}

bool Teapot::hasAttrib( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return true;
		case Attrib::TEX_COORD_0: return mHasTexCoord0;
		case Attrib::NORMAL: return mHasNormals;
		default:
			return false;
	}
}

bool Teapot::canProvideAttrib( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION:
		case Attrib::TEX_COORD_0:
		case Attrib::NORMAL:
			return true;
		default:
			return false;
	}
}

void Teapot::copyAttrib( Attrib attr, uint8_t dimensions, size_t stride, float *dest ) const
{
	calculate();

	switch( attr ) {
		case Attrib::POSITION:
			copyDataMultAdd( mVertices.get(), mNumVertices, dimensions, stride, dest, mScale, mPos );
		break;
		case Attrib::TEX_COORD_0:
			copyData( 2, mTexCoords.get(), mNumVertices, dimensions, stride, dest );
		break;
		case Attrib::NORMAL:
			copyData( 3, mNormals.get(), mNumVertices, dimensions, stride, dest );
		break;
		default:
			throw ExcMissingAttrib();
	}
}

size_t Teapot::getNumIndices() const
{
	calculate();
	
	return mNumIndices;
}

void Teapot::copyIndices( uint16_t *dest ) const
{
	calculate();
	
	for( int i = 0; i < mNumIndices; ++i )
		dest[i] = mIndices.get()[i];
}

void Teapot::copyIndices( uint32_t *dest ) const
{
	calculate();
			
	memcpy( dest, mIndices.get(), mNumIndices );		
}

uint8_t	Teapot::getAttribDims( Attrib attr ) const
{
	if( ! canProvideAttrib( attr ) )
		return 0;

	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::TEX_COORD_0: return 2;
		case Attrib::NORMAL: return 3;
		default:
			return 0;
	}
}

void Teapot::calculate() const
{
	if( mCalculationsCached )
		return;
	
	mNumVertices = 32 * (mSubdivision + 1) * (mSubdivision + 1);
	int numFaces = mSubdivision * mSubdivision * 32;
	mNumIndices = numFaces * 6;
	mVertices = unique_ptr<float>( new float[mNumVertices * 3] );
	mTexCoords = unique_ptr<float>( new float[mNumVertices * 2] );	
	mNormals = unique_ptr<float>( new float[mNumVertices * 3] );
	mIndices = unique_ptr<uint32_t>( new uint32_t[mNumIndices] );

	generatePatches( mVertices.get(), mNormals.get(), mTexCoords.get(), mIndices.get(), mSubdivision );
	
	mCalculationsCached = true;
}

void Teapot::generatePatches( float *v, float *n, float *tc, uint32_t *el, int grid )
{
	float *B = new float[4*(grid+1)];  // Pre-computed Bernstein basis functions
	float *dB = new float[4*(grid+1)]; // Pre-computed derivitives of basis functions
	int idx = 0, elIndex = 0, tcIndex = 0;

	// Pre-compute the basis functions  (Bernstein polynomials)
	// and their derivatives
	computeBasisFunctions( B, dB, grid );

	// Build each patch
	// The rim
	buildPatchReflect( 0, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The body
	buildPatchReflect( 1, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	buildPatchReflect( 2, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The lid
	buildPatchReflect( 3, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	buildPatchReflect( 4, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The bottom
	buildPatchReflect( 5, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The handle
	buildPatchReflect( 6, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	buildPatchReflect( 7, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	// The spout
	buildPatchReflect( 8, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	buildPatchReflect( 9, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );

	delete [] B;
	delete [] dB;
}

void Teapot::buildPatchReflect( int patchNum, float *B, float *dB, float *v, float *n, float *tc, unsigned int *el,
                                    int &index, int &elIndex, int &tcIndex, int grid, bool reflectX, bool reflectY )
{
    Vec3f patch[4][4];
    Vec3f patchRevV[4][4];
    getPatch( patchNum, patch, false);
    getPatch( patchNum, patchRevV, true);

    // Patch without modification
    buildPatch( patch, B, dB, v, n, tc, el, index, elIndex, tcIndex, grid, Matrix33f::identity(), true );

    // Patch reflected in x
    if( reflectX ) {
        buildPatch( patchRevV, B, dB, v, n, tc, el,
                   index, elIndex, tcIndex, grid, Matrix33f( Vec3f(-1.0f, 0.0f, 0.0f),
                                              Vec3f(0.0f, 1.0f, 0.0f),
                                              Vec3f(0.0f, 0.0f, 1.0f) ), false );
    }

    // Patch reflected in y
    if( reflectY ) {
        buildPatch( patchRevV, B, dB, v, n, tc, el, index, elIndex, tcIndex, grid, Matrix33f( Vec3f(1.0f, 0.0f, 0.0f),
                                              Vec3f(0.0f, -1.0f, 0.0f),
                                              Vec3f(0.0f, 0.0f, 1.0f) ), false );
    }

    // Patch reflected in x and y
    if( reflectX && reflectY ) {
        buildPatch( patch, B, dB, v, n, tc, el, index, elIndex, tcIndex, grid, Matrix33f( Vec3f(-1.0f, 0.0f, 0.0f),
                                              Vec3f(0.0f, -1.0f, 0.0f),
                                              Vec3f(0.0f, 0.0f, 1.0f) ), true );
    }
}

void Teapot::buildPatch( Vec3f patch[][4], float *B, float *dB, float *v, float *n, float *tc, 
                           unsigned int *el, int &index, int &elIndex, int &tcIndex, int grid, Matrix33f reflect, bool invertNormal )
{
	int startIndex = index / 3;
	float tcFactor = 1.0f / grid;

	for( int i = 0; i <= grid; i++ ) {
		for( int j = 0 ; j <= grid; j++) {
			Vec3f pt = reflect * evaluate( i, j, B, patch );
			Vec3f norm = reflect * evaluateNormal( i, j, B, dB, patch );
			if( invertNormal )
				norm = -norm;
			// awful hack due to normals discontinuity
			if( abs(pt.x) < 0.01f && abs(pt.y) < 0.01f )
				norm = ( pt.z < 1 ) ? -Vec3f::zAxis() : Vec3f::zAxis();

			v[index] = pt.x; v[index+1] = pt.z; v[index+2] = pt.y;
			n[index] = norm.x; n[index+1] = norm.z; n[index+2] = norm.y;
			tc[tcIndex] = i * tcFactor; tc[tcIndex+1] = j * tcFactor;

			index += 3;
			tcIndex += 2;
		}
	}

	for( int i = 0; i < grid; i++ ) {
		int iStart = i * (grid+1) + startIndex;
		int nextiStart = (i+1) * (grid+1) + startIndex;
		for( int j = 0; j < grid; j++) {
			el[elIndex] = iStart + j;
			el[elIndex+1] = nextiStart + j + 1;
			el[elIndex+2] = nextiStart + j;

			el[elIndex+3] = iStart + j;
			el[elIndex+4] = iStart + j + 1;
			el[elIndex+5] = nextiStart + j + 1;

			elIndex += 6;
		}
	}
}

void Teapot::getPatch( int patchNum, Vec3f patch[][4], bool reverseV )
{
	for( int u = 0; u < 4; u++) {          // Loop in u direction
		for( int v = 0; v < 4; v++ ) {     // Loop in v direction
		if( reverseV ) {
			patch[u][v] = Vec3f(
				sCurveData[sPatchIndices[patchNum][u*4+(3-v)]][0],
				sCurveData[sPatchIndices[patchNum][u*4+(3-v)]][1],
				sCurveData[sPatchIndices[patchNum][u*4+(3-v)]][2]
			);
		}
		else {
			patch[u][v] = Vec3f(
				sCurveData[sPatchIndices[patchNum][u*4+v]][0],
				sCurveData[sPatchIndices[patchNum][u*4+v]][1],
				sCurveData[sPatchIndices[patchNum][u*4+v]][2]
			);
			}
		}
	}
}

void Teapot::computeBasisFunctions( float *B, float *dB, int grid )
{
	float inc = 1.0f / grid;
	for( int i = 0; i <= grid; i++ ) {
		float t = i * inc;
		float tSqr = t * t;
		float oneMinusT = (1.0f - t);
		float oneMinusT2 = oneMinusT * oneMinusT;

		B[i*4 + 0] = oneMinusT * oneMinusT2;
		B[i*4 + 1] = 3.0f * oneMinusT2 * t;
		B[i*4 + 2] = 3.0f * oneMinusT * tSqr;
		B[i*4 + 3] = t * tSqr;

		dB[i*4 + 0] = -3.0f * oneMinusT2;
		dB[i*4 + 1] = -6.0f * t * oneMinusT + 3.0f * oneMinusT2;
		dB[i*4 + 2] = -3.0f * tSqr + 6.0f * t * oneMinusT;
		dB[i*4 + 3] = 3.0f * tSqr;
	}
}

Vec3f Teapot::evaluate( int gridU, int gridV, const float *B, const Vec3f patch[][4] )
{
	Vec3f p( Vec3f::zero() );
	for( int i = 0; i < 4; i++) {
		for( int j = 0; j < 4; j++) {
			p += patch[i][j] * B[gridU*4+i] * B[gridV*4+j];
		}
	}

	return p;
}

Vec3f Teapot::evaluateNormal( int gridU, int gridV, const float *B, const float *dB, const Vec3f patch[][4] )
{
	Vec3f du( Vec3f::zero() );
	Vec3f dv( Vec3f::zero() );

	for( int i = 0; i < 4; i++ ) {
		for( int j = 0; j < 4; j++ ) {
			du += patch[i][j] * dB[gridU*4+i] * B[gridV*4+j];
			dv += patch[i][j] * B[gridU*4+i] * dB[gridV*4+j];
		}
	}

	return du.cross( dv ).normalized();
}

} } // namespace cinder::geo