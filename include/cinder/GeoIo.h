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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Exception.h"
#include "cinder/Vector.h"

namespace cinder { namespace geo {

enum class Attrib { POSITION, COLOR, TEX_COORD_0, NORMAL, TANGENT, BITANGET }; 

class Source {
  public:
	virtual size_t		getNumVerts() const = 0;
	
	virtual bool		hasAttrib( Attrib attr ) const = 0;
	virtual bool		canProvideAttrib( Attrib attr ) const = 0;
	virtual uint8_t		getAttribDims( Attrib attr ) const = 0;	
	virtual void		copyAttrib( Attrib attr, uint8_t dims, size_t stride, float *dest ) const = 0;
	
	virtual size_t		getNumIndices() const = 0;
	virtual void		copyIndices( uint16_t *dest ) const = 0;
	virtual void		copyIndices( uint32_t *dest ) const = 0;
	
  protected:
	static void	copyData( uint8_t srcDimensions, const float *srcData, size_t dataLength, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData );
	static void	copyDataMultAdd( uint8_t srcDimensions, const float *srcData, size_t dataLength,
							uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec3f &mult, const Vec3f &add );
};

class Cube : public Source {
  public:
	Cube();
	
	Cube&		colors() { mHasColor = true; return *this; }
	Cube&		texCoords() { mHasTexCoord0 = true; return *this; }
	Cube&		normals() { mHasNormals = true; return *this; }
	Cube&		position( const Vec3f &pos ) { mPos = pos; return *this; }
	Cube&		scale( const Vec3f &scale ) { mScale = scale; return *this; }
	Cube&		scale( float s ) { mScale = Vec3f( s, s, s ); return *this; }
  
	virtual size_t		getNumVerts() const override { return 24; }
	
	virtual bool		hasAttrib( Attrib attr ) const override;
	virtual bool		canProvideAttrib( Attrib attr ) const override;
	virtual uint8_t		getAttribDims( Attrib attr ) const override;
	virtual void		copyAttrib( Attrib attr, uint8_t dims, size_t stride, float *dest ) const override ;
	
	virtual size_t		getNumIndices() const override { return 36; }
	virtual void		copyIndices( uint16_t *dest ) const override;
	virtual void		copyIndices( uint32_t *dest ) const override;
	
	Vec3f		mPos, mScale;
	bool		mHasColor;
	bool		mHasTexCoord0;
	bool		mHasNormals;
	
	static float	sVertices[24*3];
	static float	sColors[24*3];
	static uint16_t	sIndices[36];
};

class Exc : public Exception {
};

class ExcMissingAttrib : public Exception {
};

class ExcIllegalSourceDimensions : public Exception {
};

class ExcIllegalDestDimensions : public Exception {
};

} } // namespace cinder::geo
