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

#include "cinder/GeomIo.h"
#include <algorithm>

using namespace std;

namespace cinder { namespace geom {

std::string sAttribNames[(int)Attrib::NUM_ATTRIBS] = {
	"POSITION", "COLOR", "TEX_COORD_0", "TEX_COORD_1", "TEX_COORD_2", "TEX_COORD_3",
	"NORMAL", "TANGENT", "BITANGET", "BONE_INDEX", "BONE_WEIGHT",
	"CUSTOM_0", "CUSTOM_1", "CUSTOM_2", "CUSTOM_3", "CUSTOM_4", "CUSTOM_5", "CUSTOM_6", "CUSTOM_7", "CUSTOM_8", "CUSTOM_9"
};

std::string attribToString( Attrib attrib )
{
	if( attrib < Attrib::NUM_ATTRIBS )
		return sAttribNames[(int)attrib];
	else
		return "";
}

///////////////////////////////////////////////////////////////////////////////////////
// BufferLayout
BufferLayout::AttribInfo BufferLayout::getAttribInfo( Attrib attrib ) const
{
	for( const auto &atIt : mAttribs )
		if( atIt.getAttrib() == attrib )
			return atIt;

	throw ExcMissingAttrib();
}

bool BufferLayout::hasAttrib( Attrib attrib ) const
{
	for( const auto &atIt : mAttribs )
		if( atIt.getAttrib() == attrib )
			return true;
	
	return false;
}

uint8_t	BufferLayout::getAttribDims( Attrib attrib ) const
{
	for( const auto &atIt : mAttribs ) {
		if( atIt.getAttrib() == attrib )
			return atIt.getDims();
	}

	return 0;
}

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

void copyData( uint8_t srcDimensions, const float *srcData, size_t numElements, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData )
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


namespace { 
template<typename T>
void copyIndexDataForceTrianglesImpl( Primitive primitive, const uint32_t *source, size_t numIndices, T *target )
{
	switch( primitive ) {
		case Primitive::LINES:
		case Primitive::TRIANGLES:
			memcpy( target, source, sizeof(uint32_t) * numIndices );
		break;
		case Primitive::TRIANGLE_STRIP: { // ABC, CBD, CDE, EDF, etc
			if( numIndices < 3 )
				return;
			size_t outIdx = 0; // (012, 213), (234, 435), etc : (odd,even), (odd,even), etc
			for( size_t i = 0; i < numIndices - 2; ++i ) {
				if( i & 1 ) { // odd
					target[outIdx++] = source[i+1];
					target[outIdx++] = source[0];
					target[outIdx++] = source[i+2];
				}
				else { // even
					target[outIdx++] = source[i];
					target[outIdx++] = source[i+1];
					target[outIdx++] = source[i+2];
				}
			}
		}
		break;
		case Primitive::TRIANGLE_FAN: { // ABC, ACD, ADE, etc
			if( numIndices < 3 )
				return;
			size_t outIdx = 0;
			for( size_t i = 0; i < numIndices - 2; ++i ) {
				target[outIdx++] = source[0];
				target[outIdx++] = source[i+1];
				target[outIdx++] = source[i+2];
			}
		}
		default:
			throw ExcIllegalPrimitiveType();			
		break;
	}
}

} // anonymous namespace

bool Source::isEnabled( Attrib attrib ) const
{
	return mEnabledAttribs.count( attrib ) > 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// Target
void Target::copyIndexDataForceTriangles( Primitive primitive, const uint32_t *source, size_t numIndices, uint32_t *target )
{
	copyIndexDataForceTrianglesImpl<uint32_t>( primitive, source, numIndices, target );
}

void Target::copyIndexDataForceTriangles( Primitive primitive, const uint32_t *source, size_t numIndices, uint16_t *target )
{
	copyIndexDataForceTrianglesImpl<uint16_t>( primitive, source, numIndices, target );
}

void Target::copyIndexData( const uint32_t *source, size_t numIndices, uint32_t *target )
{
	memcpy( target, source, numIndices * sizeof(float) );
}

void Target::copyIndexData( const uint32_t *source, size_t numIndices, uint16_t *target )
{
	for( size_t v = 0; v < numIndices; ++v )
		target[v] = source[v];
}

void Target::generateIndices( Primitive sourcePrimitive, size_t sourceNumIndices )
{
	unique_ptr<uint32_t> indices( new uint32_t[sourceNumIndices] );

	uint32_t count = 0;
	std::generate( indices.get(), indices.get() + sourceNumIndices, [&] { return count++; } );
	
	uint8_t requiredBytesPerIndex = 4;
	if( sourceNumIndices < 256 )
		requiredBytesPerIndex = 1;
	else if( sourceNumIndices < 65536 )
		requiredBytesPerIndex = 2;
	// now have the target copy these indices
	copyIndices( sourcePrimitive, indices.get(), sourceNumIndices, requiredBytesPerIndex );
}

///////////////////////////////////////////////////////////////////////////////////////
// Rect
float Rect::sPositions[4*2] = { 0.5f,-0.5f,	-0.5f,-0.5f,	0.5f,0.5f,	-0.5f,0.5f };
float Rect::sColors[4*3] = { 1, 0, 1,	0, 0, 1,	1, 1, 1,	0, 1, 1 };
float Rect::sTexCoords[4*2] = { 1, 1,	0, 1,		1, 0,		0, 0 };
float Rect::sNormals[4*3] = {0, 0, 1,	0, 0, 1,	0, 0, 1,	0, 0, 1 };

Rect::Rect()
	: mPos( Vec2f::zero() ), mScale( Vec2f::one() )
{
	enable( Attrib::POSITION );	
	enable( Attrib::TEX_COORD_0 );
	enable( Attrib::NORMAL );
}

void Rect::loadInto( Target *target ) const
{
	unique_ptr<Vec2f> positions( new Vec2f[4] );
	for( size_t p = 0; p < 4; ++p )
		positions.get()[p] = Vec2f( sPositions[p*2+0], sPositions[p*2+1] ) * mScale + mPos;

	target->copyAttrib( Attrib::POSITION, 2, 0, positions.get()->ptr(), 4 );
	if( isEnabled( Attrib::COLOR ) )
		target->copyAttrib( Attrib::COLOR, 3, 0, sColors, 4 );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		target->copyAttrib( Attrib::TEX_COORD_0, 2, 0, sTexCoords, 4 );
	if( isEnabled( Attrib::NORMAL ) )
		target->copyAttrib( Attrib::NORMAL, 3, 0, sNormals, 4 );
}

uint8_t	Rect::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 2;
		case Attrib::COLOR: return isEnabled( Attrib::COLOR ) ? 3 : 0;
		case Attrib::TEX_COORD_0: return isEnabled( Attrib::TEX_COORD_0 ) ? 2 : 0;
		case Attrib::NORMAL: return isEnabled( Attrib::NORMAL ) ? 3 : 0;
		default:
			return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Cube
float Cube::sPositions[24*3] = {  1.0f, 1.0f, 1.0f,   1.0f,-1.0f, 1.0f,	 1.0f,-1.0f,-1.0f,   1.0f, 1.0f,-1.0f,	// +X
								 1.0f, 1.0f, 1.0f,   1.0f, 1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f, 1.0f, 1.0f,	// +Y
								 1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,   1.0f,-1.0f, 1.0f,	// +Z
								-1.0f, 1.0f, 1.0f,  -1.0f, 1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f,-1.0f, 1.0f,	// -X
								-1.0f,-1.0f,-1.0f,   1.0f,-1.0f,-1.0f,   1.0f,-1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,	// -Y
								 1.0f,-1.0f,-1.0f,  -1.0f,-1.0f,-1.0f,  -1.0f, 1.0f,-1.0f,   1.0f, 1.0f,-1.0f };// -Z

uint32_t Cube::sIndices[6*6] ={	0, 1, 2, 0, 2, 3,
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
{
	enable( Attrib::POSITION );
	enable( Attrib::TEX_COORD_0 );
	enable( Attrib::NORMAL );
}

uint8_t	Cube::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::COLOR: return isEnabled( Attrib::COLOR ) ? 3 : 0;
		case Attrib::TEX_COORD_0: return isEnabled( Attrib::TEX_COORD_0 ) ? 2 : 0;
		case Attrib::NORMAL: return isEnabled( Attrib::NORMAL ) ? 3 : 0;
		default:
			return 0;
	}	
}

void Cube::loadInto( Target *target ) const
{
	target->copyAttrib( Attrib::POSITION, 3, 0, sPositions, 24 );
	if( isEnabled( Attrib::COLOR ) )
		target->copyAttrib( Attrib::COLOR, 3, 0, sColors, 24 );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		target->copyAttrib( Attrib::TEX_COORD_0, 2, 0, sTexCoords, 24 );
	if( isEnabled( Attrib::NORMAL ) )
		target->copyAttrib( Attrib::NORMAL, 3, 0, sNormals, 24 );
	
	target->copyIndices( Primitive::TRIANGLES, sIndices, 36, 1 );
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
	: mSubdivision( 4 )
{
	enable( Attrib::POSITION );
	enable( Attrib::TEX_COORD_0 );
	enable( Attrib::NORMAL );
	updateVertexCounts();
}

Teapot&	Teapot::subdivision( int sub )
{
	mSubdivision = sub;
	updateVertexCounts();
	return *this;
}

size_t Teapot::getNumVertices() const
{
	return mNumVertices;
}

void Teapot::loadInto( Target *target ) const
{
	calculate();

	target->copyAttrib( Attrib::POSITION, 3, 0, (const float*)mPositions.get(), mNumVertices );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		target->copyAttrib( Attrib::TEX_COORD_0, 2, 0, (const float*)mTexCoords.get(), mNumVertices );
	if( isEnabled( Attrib::NORMAL ) )
		target->copyAttrib( Attrib::NORMAL, 3, 0, (const float*)mNormals.get(), mNumVertices );

	target->copyIndices( Primitive::TRIANGLES, mIndices.get(), mNumIndices, 4 );
}

size_t Teapot::getNumIndices() const
{
	return mNumIndices;
}

uint8_t	Teapot::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::TEX_COORD_0: return isEnabled( Attrib::TEX_COORD_0 ) ? 2 : 0;
		case Attrib::NORMAL: return isEnabled( Attrib::NORMAL ) ? 3 : 0;
		default:
			return 0;
	}
}

void Teapot::updateVertexCounts() const
{
	int numFaces = mSubdivision * mSubdivision * 32;
	mNumIndices = numFaces * 6;
	mNumVertices = 32 * (mSubdivision + 1) * (mSubdivision + 1);
}

void Teapot::calculate() const
{
	updateVertexCounts();

	mPositions = unique_ptr<float>( new float[mNumVertices * 3] );
	mTexCoords = unique_ptr<float>( new float[mNumVertices * 2] );	
	mNormals = unique_ptr<float>( new float[mNumVertices * 3] );
	mIndices = unique_ptr<uint32_t>( new uint32_t[mNumIndices] );

	generatePatches( mPositions.get(), mNormals.get(), mTexCoords.get(), mIndices.get(), mSubdivision );
}

void Teapot::generatePatches( float *v, float *n, float *tc, uint32_t *el, int grid )
{
	unique_ptr<float> B( new float[4*(grid+1)] );  // Pre-computed Bernstein basis functions
	unique_ptr<float> dB( new float[4*(grid+1)] ); // Pre-computed derivitives of basis functions
	int idx = 0, elIndex = 0, tcIndex = 0;

	// Pre-compute the basis functions  (Bernstein polynomials)
	// and their derivatives
	computeBasisFunctions( B.get(), dB.get(), grid );

	// Build each patch
	// The rim
	buildPatchReflect( 0, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The body
	buildPatchReflect( 1, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	buildPatchReflect( 2, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The lid
	buildPatchReflect( 3, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	buildPatchReflect( 4, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The bottom
	buildPatchReflect( 5, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, true, true );
	// The handle
	buildPatchReflect( 6, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	buildPatchReflect( 7, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	// The spout
	buildPatchReflect( 8, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
	buildPatchReflect( 9, B.get(), dB.get(), v, n, tc, el, idx, elIndex, tcIndex, grid, false, true );
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

///////////////////////////////////////////////////////////////////////////////////////
// Circle
Circle::Circle()
	: mRequestedSegments( -1 ), mCenter( 0, 0 ), mRadius( 1.0f )
{
	enable( Attrib::POSITION );
	enable( Attrib::TEX_COORD_0 );
	enable( Attrib::NORMAL );
	updateVertexCounts();
}

Circle&	Circle::segments( int segments )
{
	mRequestedSegments = segments;
	updateVertexCounts();
	return *this;
}

Circle&	Circle::radius( float radius )
{
	mRadius = radius;
	updateVertexCounts();
	return *this;
}

// If numSegments<0, calculate based on radius
void Circle::updateVertexCounts()
{
	if( mRequestedSegments <= 0 )
		mNumSegments = (int)math<double>::floor( mRadius * M_PI * 2 );
	else
		mNumSegments = mRequestedSegments;
	
	if( mNumSegments < 3 ) mNumSegments = 3;
	mNumVertices = mNumSegments + 1 + 1;
}

void Circle::calculate() const
{
	mPositions = unique_ptr<Vec2f>( new Vec2f[mNumVertices] );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		mTexCoords = unique_ptr<Vec2f>( new Vec2f[mNumVertices] );
	if( isEnabled( Attrib::NORMAL ) )		
		mNormals = unique_ptr<Vec3f>( new Vec3f[mNumVertices] );	

	// center
	mPositions.get()[0] = mCenter;
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		mTexCoords.get()[0] = Vec2f( 0.5f, 0.5f );
	if( isEnabled( Attrib::NORMAL ) )
		mNormals.get()[0] = Vec3f( 0, 0, 1 );
	
	// iterate the segments
	const float tDelta = 1 / (float)mNumSegments * 2.0f * 3.14159f;
	float t = 0;
	for( int s = 0; s <= mNumSegments; s++ ) {
		Vec2f unit( math<float>::cos( t ), math<float>::sin( t ) );
		mPositions.get()[s+1] = mCenter + unit * mRadius;
		if( isEnabled( Attrib::TEX_COORD_0 ) )
			mTexCoords.get()[s+1] = unit * 0.5f + Vec2f( 0.5f, 0.5f );
		if( isEnabled( Attrib::NORMAL ) )
			mNormals.get()[s+1] = Vec3f( 0, 0, 1 );
		t += tDelta;
	}
}

size_t Circle::getNumVertices() const
{
	return mNumVertices;
}

void Circle::loadInto( Target *target ) const
{
	calculate();

	target->copyAttrib( Attrib::POSITION, 2, 0, (const float*)mPositions.get(), mNumVertices );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		target->copyAttrib( Attrib::TEX_COORD_0, 2, 0, (const float*)mTexCoords.get(), mNumVertices );
	if( isEnabled( Attrib::NORMAL ) )
		target->copyAttrib( Attrib::NORMAL, 3, 0, (const float*)mNormals.get(), mNumVertices );
}

uint8_t	Circle::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 2;
		case Attrib::TEX_COORD_0: return isEnabled( Attrib::TEX_COORD_0 ) ? 2 : 0;
		case Attrib::NORMAL: return isEnabled( Attrib::NORMAL ) ? 3 : 0;
		default:
			return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Sphere

Sphere::Sphere()
	: mNumSegments( -1 ), mCenter( 0, 0, 0 ), mRadius( 1.0f ), mCalculationsCached( false )
{
	enable( Attrib::POSITION );
	enable( Attrib::NORMAL );
	enable( Attrib::TEX_COORD_0 );
}

void Sphere::calculate() const
{
	if( mCalculationsCached )
		return;

	int numSegments = mNumSegments;
	if( numSegments <= 0 )
		numSegments = std::max( 12, (int)math<double>::floor( mRadius * M_PI * 2 ) );

	calculateImplUV( numSegments, numSegments );
	mCalculationsCached = true;
}
	
void Sphere::calculateImplUV( size_t segments, size_t rings ) const
{
	mVertices.resize( segments * rings );
	mNormals.resize( segments * rings );
	mTexCoords.resize( segments * rings );
	mColors.resize( segments * rings );	
	mIndices.resize( segments * rings * 6 );

	float ringIncr = 1.0f / (float)( rings - 1 );
	float segIncr = 1.0f / (float)( segments - 1 );
	float radius = mRadius;

	bool hasNormals = isEnabled( Attrib::NORMAL );
	bool hasTexCoords = isEnabled( Attrib::TEX_COORD_0 );
	bool hasColors = isEnabled( Attrib::COLOR );

	auto vertIt = mVertices.begin();
	auto normIt = mNormals.begin();
	auto texIt = mTexCoords.begin();
	auto colorIt = mColors.begin();
	for( size_t r = 0; r < rings; r++ ) {
		for( size_t s = 0; s < segments; s++ ) {
			float x = math<float>::cos( 2 * M_PI * s * segIncr ) * math<float>::sin( M_PI * r * ringIncr );
			float y = math<float>::sin( -M_PI / 2 + M_PI * r * ringIncr );
			float z = math<float>::sin( 2 * M_PI * s * segIncr ) * math<float>::sin( M_PI * r * ringIncr );

			vertIt->set( x * radius, y * radius, z * radius );
			++vertIt;

			if( hasNormals ) {
				normIt->set( x, y, z );
				++normIt;
			}
			if( hasTexCoords ) {
				texIt->set( s * segIncr, 1.0f - r * ringIncr );
				++texIt;
			}
			if( hasColors ) {
				colorIt->set( x, y, z );
				++colorIt;
			}
		}
	}

	auto indexIt = mIndices.begin();
	for( size_t r = 0; r < rings - 1; r++ ) {
		for( size_t s = 0; s < segments - 1 ; s++ ) {
			*indexIt++ = r * segments + s;
			*indexIt++ = r * segments + ( s + 1 );
			*indexIt++ = ( r + 1 ) * segments + ( s + 1 );

			*indexIt++ = ( r + 1 ) * segments + ( s + 1 );
			*indexIt++ = ( r + 1 ) * segments + s;
			*indexIt++ = r * segments + s;
		}
	}
}

size_t Sphere::getNumVertices() const
{
	calculate();
	return mVertices.size();
}

size_t Sphere::getNumIndices() const
{
	calculate();
	return mIndices.size();
}

uint8_t Sphere::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::TEX_COORD_0: return isEnabled( Attrib::TEX_COORD_0 ) ? 2 : 0;
		case Attrib::NORMAL: return isEnabled( Attrib::NORMAL ) ? 3 : 0;
		case Attrib::COLOR: return isEnabled( Attrib::COLOR ) ? 3 : 0;
		default:
			return 0;
	}
}

void Sphere::loadInto( Target *target ) const
{
	calculate();
	if( isEnabled( Attrib::POSITION ) )
		target->copyAttrib( Attrib::POSITION, 3, 0, mVertices.data()->ptr(), mVertices.size() );
	if( isEnabled( Attrib::TEX_COORD_0 ) )
		target->copyAttrib( Attrib::TEX_COORD_0, 2, 0, mTexCoords.data()->ptr(), mTexCoords.size() );
	if( isEnabled( Attrib::NORMAL ) )
		target->copyAttrib( Attrib::NORMAL, 3, 0, mNormals.data()->ptr(), mNormals.size() );
	if( isEnabled( Attrib::COLOR ) )
		target->copyAttrib( Attrib::COLOR, 3, 0, mColors.data()->ptr(), mColors.size() );

	target->copyIndices( Primitive::TRIANGLES, mIndices.data(), mIndices.size(), 4 );	
}

///////////////////////////////////////////////////////////////////////////////////////
// SplineExtrusion
#if 0
SplineExtrusion::SplineExtrusion( const std::function<Vec3f(float)> &pathCurve, int pathSegments, float radius, int radiusSegments )
	: mCalculationsCached( false ), mScale( 1 , 1, 1 ), mPos( Vec3f::zero() )
{
	calculateCurve( pathCurve, pathSegments, radius, radiusSegments );
}

void SplineExtrusion::calculateCurve( const std::function<Vec3f(float)> &pathCurve, int pathSegments, float radius, int radiusSegments ) const
{
	mNumVertices = pathSegments * radiusSegments;
	int numTriangles = ( pathSegments - 1 ) * radiusSegments * 2;
	mNumIndices = numTriangles * 3;

	mVertices = unique_ptr<float>( new float[mNumVertices * 3] );
	mTexCoords = unique_ptr<float>( new float[mNumVertices * 2] );	
	mNormals = unique_ptr<float>( new float[mNumVertices * 3] );
	mIndices = unique_ptr<uint32_t>( new uint32_t[mNumIndices] );
	
	for( int pathSeg = 0; pathSeg < pathSegments; ++pathSeg ) {
		
	}
}

void SplineExtrusion::calculate() const
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


size_t SplineExtrusion::getNumVertices() const
{
	calculate();
	
	return mNumVertices;
}

bool SplineExtrusion::hasAttrib( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return true;
		case Attrib::TEX_COORD_0: return mHasTexCoord0;
		case Attrib::NORMAL: return mHasNormals;
		default:
			return false;
	}
}

bool SplineExtrusion::canProvideAttrib( Attrib attr ) const
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

uint8_t	SplineExtrusion::getAttribDims( Attrib attr ) const
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

void SplineExtrusion::copyAttrib( Attrib attr, uint8_t dimensions, size_t stride, float *dest ) const
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

size_t SplineExtrusion::getNumIndices() const
{
	calculate();
	
	return mNumIndices;
}

void SplineExtrusion::copyIndices( uint16_t *dest ) const
{
	calculate();
	
	for( int i = 0; i < mNumIndices; ++i )
		dest[i] = mIndices.get()[i];
}

void SplineExtrusion::copyIndices( uint32_t *dest ) const
{
	calculate();
			
	memcpy( dest, mIndices.get(), mNumIndices * sizeof(uint32_t) );		
}

#endif

} } // namespace cinder::geo