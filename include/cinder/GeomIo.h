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
#include "cinder/Matrix.h"
#include "cinder/BSpline.h"

namespace cinder { namespace geom {

class Target;
typedef std::shared_ptr<class Source>	SourceRef;

// keep this incrementing by 1 only; some code relies on that for iterating; add corresponding entry to sAttribNames
enum class Attrib { POSITION, COLOR, TEX_COORD_0, TEX_COORD_1, TEX_COORD_2, TEX_COORD_3,
					NORMAL, TANGENT, BITANGET, BONE_INDEX, BONE_WEIGHT, 
					CUSTOM_0, CUSTOM_1, CUSTOM_2, CUSTOM_3, CUSTOM_4, CUSTOM_5, CUSTOM_6, CUSTOM_7, CUSTOM_8, CUSTOM_9,
					NUM_ATTRIBS };
extern std::string sAttribNames[(int)Attrib::NUM_ATTRIBS];
enum class Primitive { TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN }; 

//! Debug utility which returns the name of \a attrib as a std::string
std::string attribToString( Attrib attrib );

class BufferLayout {
  public:
	struct AttribInfo {
		AttribInfo( const Attrib &attrib, uint8_t dims, size_t stride, size_t offset, uint32_t instanceDivisor = 0 )
			: mAttrib( attrib ), mDims( dims ), mStride( stride ), mOffset( offset ), mInstanceDivisor( instanceDivisor )
		{}
	
		Attrib		getAttrib() const { return mAttrib; }
		uint8_t		getDims() const { return mDims; }
		size_t		getStride() const { return mStride; }
		size_t		getOffset() const { return mOffset;	}
		uint32_t	getInstanceDivisor() const { return mInstanceDivisor; }
		
	  protected:
		Attrib		mAttrib;
		int32_t		mDims;
		size_t		mStride;
		size_t		mOffset;
		uint32_t	mInstanceDivisor;
	}; 


	BufferLayout() {}
	
	void append( const Attrib &attrib, uint8_t dims, size_t stride, size_t offset, uint32_t instanceDivisor = 0 ) {
		mAttribs.push_back( AttribInfo( attrib, dims, stride, offset, instanceDivisor ) );
	}
	
	//! Returns the AttribInfo for a given Attrib, and throws ExcMissingAttrib if it is not available
	AttribInfo		getAttribInfo( Attrib attrib ) const;
	//! Returns whether a given Attrib is present in the BufferLayout
	bool			hasAttrib( Attrib attrib ) const;
	//! Returns the dimensions for a given Attrib, or 0 if it is not in the BufferLayout
	uint8_t			getAttribDims( Attrib attrib ) const;
	//! Returns a vector of all present Attribs
	const std::vector<AttribInfo>&	getAttribs() const { return mAttribs; }
  protected:
	std::vector<AttribInfo>		mAttribs;
};

void copyData( uint8_t srcDimensions, const float *srcData, size_t numElements, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData );

class Source {
  public:
	virtual size_t		getNumVertices() const = 0;
	virtual size_t		getNumIndices() const = 0;
	virtual Primitive	getPrimitive() const = 0;	
	virtual uint8_t		getAttribDims( Attrib attr ) const = 0;	
	
	virtual void		loadInto( Target *target ) const = 0;
	
/*
	//! Always copy indices; generate them when they don't exist. Copies getNumVertices() indices when indices don't exist.
	void				forceCopyIndices( uint16_t *dest ) const;
	//! Always copy indices; generate them when they don't exist. Copies getNumVertices() indices when indices don't exist.
	void				forceCopyIndices( uint32_t *dest ) const;
	//! Returns the number of indices that will be copied by forceCopyIndicesTriangles( uint16_t *dest );
	size_t				getNumIndicesTriangles() const;
	//! Always copy indices appropriate for a \c Primitive::TRIANGLES; generate them when they don't exist. Copies getNumIndicesTriangles().
	void				forceCopyIndicesTriangles( uint16_t *dest ) const;
	//! Always copy indices appropriate for a \c Primitive::TRIANGLES; generate them when they don't exist. Copies getNumIndicesTriangles().
	void				forceCopyIndicesTriangles( uint32_t *dest ) const;
*/	
  protected:  
	static void	copyDataMultAdd( const float *srcData, size_t numElements, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec2f &mult, const Vec2f &add );
	static void	copyDataMultAdd( const float *srcData, size_t numElements, uint8_t dstDimensions, size_t dstStrideBytes, float *dstData, const Vec3f &mult, const Vec3f &add );
	
	//! Builds a sequential list of vertices to simulate an indexed geometry when Source is non-indexed. Assumes \a dest contains storage for getNumVertices() entries
	void	copyIndicesNonIndexed( uint16_t *dest ) const;
	//! Builds a sequential list of vertices to simulate an indexed geometry when Source is non-indexed. Assumes \a dest contains storage for getNumVertices() entries
	void	copyIndicesNonIndexed( uint32_t *dest ) const;
	template<typename T>
	void forceCopyIndicesTrianglesImpl( T *dest ) const;
	
};

class Target {
  public:
  	virtual Primitive	getPrimitive() const = 0;
	virtual uint8_t		getAttribDims( Attrib attr ) const = 0;	
  
	virtual void	copyAttrib( Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count ) = 0;
	virtual void	copyIndices( Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex ) = 0;
	
	//! For non-indexed geometry, this generates appropriate indices and then calls the copyIndices() virtual method.
	void	generateIndices( Primitive sourcePrimitive, size_t sourceNumIndices );
	
  protected:
	void copyIndexData( const uint32_t *source, size_t numIndices, uint32_t *target );
	void copyIndexData( const uint32_t *source, size_t numIndices, uint16_t *target );
	void copyIndexDataForceTriangles( Primitive primitive, const uint32_t *source, size_t numIndices, uint32_t *target );
	void copyIndexDataForceTriangles( Primitive primitive, const uint32_t *source, size_t numIndices, uint16_t *target );
};

class Rect : public Source {
  public:
	//! Defaults to having positions, normals and texCoords
	Rect();
	
	Rect&		colors( bool enable = true ) { mHasColor = enable; return *this; }
	Rect&		texCoords( bool enable = true ) { mHasTexCoord0 = enable; return *this; }
	Rect&		normals( bool enable = true ) { mHasNormals = enable; return *this; }
	Rect&		position( const Vec2f &pos ) { mPos = pos; return *this; }
	Rect&		scale( const Vec2f &scale ) { mScale = scale; return *this; }
	Rect&		scale( float s ) { mScale = Vec2f( s, s ); return *this; }
  
	virtual size_t		getNumVertices() const override { return 4; }
	virtual size_t		getNumIndices() const override { return 0; }
	virtual Primitive	getPrimitive() const override { return Primitive::TRIANGLE_STRIP; }
	virtual uint8_t		getAttribDims( Attrib attr ) const override;
	virtual void		loadInto( Target *target ) const override;
	
	Vec2f		mPos, mScale;
	bool		mHasColor;
	bool		mHasTexCoord0;
	bool		mHasNormals;
	
	static float	sPositions[4*2];
	static float	sColors[4*3];
	static float	sTexCoords[4*2];
	static float	sNormals[4*3];
};

class Cube : public Source {
  public:
	//! Defaults to having positions, normals and texCoords
	Cube();
	
	Cube&		colors( bool enable = true ) { mHasColor = enable; return *this; }
	Cube&		texCoords( bool enable = true ) { mHasTexCoord0 = enable; return *this; }
	Cube&		normals( bool enable = true ) { mHasNormals = enable; return *this; }
  
	virtual size_t		getNumVertices() const override { return 24; }
	virtual size_t		getNumIndices() const override { return 36; }	
	virtual Primitive	getPrimitive() const override { return Primitive::TRIANGLES; }	
	virtual uint8_t		getAttribDims( Attrib attr ) const override;
	virtual void		loadInto( Target *target ) const override;

  protected:	
	bool		mHasColor;
	bool		mHasTexCoord0;
	bool		mHasNormals;
	
	static float	sPositions[24*3];
	static float	sColors[24*3];
	static float	sTexCoords[24*2];
	static float	sNormals[24*3];	
	
	static uint32_t	sIndices[36];
};

class Teapot : public Source {
  public:
	Teapot();
	
	Teapot&		texCoords() { mHasTexCoord0 = true; return *this; }
	Teapot&		normals() { mHasNormals = true; return *this; }
	Teapot&		subdivision( int sub );
  
	virtual size_t		getNumVertices() const override;
	virtual size_t		getNumIndices() const override;
	virtual Primitive	getPrimitive() const override { return Primitive::TRIANGLES; }
	virtual void		loadInto( Target *target ) const override;
	virtual uint8_t		getAttribDims( Attrib attr ) const override;

  protected:
	void			calculate() const;
	void			updateVertexCounts() const;
  
	static void		generatePatches( float *v, float *n, float *tc, uint32_t *el, int grid );
	static void		buildPatchReflect( int patchNum, float *B, float *dB, float *v, float *n, float *tc, unsigned int *el,
                                    int &index, int &elIndex, int &tcIndex, int grid, bool reflectX, bool reflectY );
	static void		buildPatch( Vec3f patch[][4], float *B, float *dB, float *v, float *n, float *tc, 
								unsigned int *el, int &index, int &elIndex, int &tcIndex, int grid, const Matrix33f reflect, bool invertNormal );
	static void		getPatch( int patchNum, Vec3f patch[][4], bool reverseV );
	static void		computeBasisFunctions( float *B, float *dB, int grid );
	static Vec3f	evaluate( int gridU, int gridV, const float *B, const Vec3f patch[][4] );
	static Vec3f	evaluateNormal( int gridU, int gridV, const float *B, const float *dB, const Vec3f patch[][4] );

	int			mSubdivision;
	bool		mHasTexCoord0;
	bool		mHasNormals;

	mutable	size_t						mNumVertices;
	mutable size_t						mNumIndices;
	mutable std::unique_ptr<float>		mPositions;
	mutable std::unique_ptr<float>		mTexCoords;
	mutable std::unique_ptr<float>		mNormals;	
	mutable std::unique_ptr<uint32_t>	mIndices;
	
	static const uint8_t	sPatchIndices[][16];
	static const float		sCurveData[][3];
};

class Circle : public Source {
  public:
	Circle();

	Circle&		texCoords() { mHasTexCoord0 = true; return *this; }
	Circle&		normals() { mHasNormals = true; return *this; }
	Circle&		center( const Vec2f &center ) { mCenter = center; return *this; }
	Circle&		radius( float radius );	
	Circle&		segments( int segments );
  
	void		loadInto( Target *target ) const;
	virtual size_t		getNumVertices() const override;
	virtual size_t		getNumIndices() const override { return 0; }
	virtual Primitive	getPrimitive() const override { return Primitive::TRIANGLE_FAN; }
	virtual uint8_t		getAttribDims( Attrib attr ) const override;

  private:
	void	updateVertexCounts();
	void	calculate() const;

	bool		mHasTexCoord0;
	bool		mHasNormals;
	Vec2f		mCenter;
	float		mRadius;
	int			mRequestedSegments, mNumSegments;

	size_t		mNumVertices;
	mutable std::unique_ptr<Vec2f>		mPositions;
	mutable std::unique_ptr<Vec2f>		mTexCoords;
	mutable std::unique_ptr<Vec3f>		mNormals;	
};

#if 0
class SplineExtrusion : public Source {
  public:
	SplineExtrusion( const std::function<Vec3f(float)> &pathCurve, int pathSegments, float radius, int radiusSegments );

	SplineExtrusion&		texCoords() { mHasTexCoord0 = true; return *this; }
	SplineExtrusion&		normals() { mHasNormals = true; return *this; }
	
	virtual size_t		getNumVertices() const override;
	virtual size_t		getNumIndices() const override;
	virtual Primitive	getPrimitive() const override { return Primitive::TRIANGLES; }
	
	virtual bool		hasAttrib( Attrib attr ) const override;
	virtual bool		canProvideAttrib( Attrib attr ) const override;
	virtual uint8_t		getAttribDims( Attrib attr ) const override;
	virtual void		copyAttrib( Attrib attr, uint8_t dims, size_t stride, float *dest ) const override;

	virtual void		copyIndices( uint16_t *dest ) const override;
	virtual void		copyIndices( uint32_t *dest ) const override;	

  protected:
	Vec2f		mPos, mScale;
  	bool		mHasTexCoord0;
	bool		mHasNormals;

	void		calculateCurve( const std::function<Vec3f(float)> &pathCurve, int pathSegments, float radius, int radiusSegments );
	void		calculate();

	mutable bool						mCalculationsCached;
	mutable	int32_t						mNumVertices;
	mutable int32_t						mNumIndices;
	mutable std::unique_ptr<float>		mVertices;
	mutable std::unique_ptr<float>		mTexCoords;
	mutable std::unique_ptr<float>		mNormals;	
	mutable std::unique_ptr<uint32_t>	mIndices;
};
#endif

class Exc : public Exception {
};

class ExcMissingAttrib : public Exception {
};

class ExcIllegalSourceDimensions : public Exception {
};

class ExcIllegalDestDimensions : public Exception {
};

class ExcIllegalPrimitiveType : public Exception {
};

class ExcNoIndices : public Exception {
};

// Attempt to store >65535 indices into a uint16_t
class ExcInadequateIndexStorage : public Exception {
};

} } // namespace cinder::geom
