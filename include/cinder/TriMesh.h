/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

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

#include <vector>
#include "cinder/Vector.h"
#include "cinder/AxisAlignedBox.h"
#include "cinder/DataSource.h"
#include "cinder/DataTarget.h"
#include "cinder/Matrix.h"
#include "cinder/Color.h"
#include "cinder/Rect.h"
#include "cinder/GeomIo.h"

namespace cinder {

typedef std::shared_ptr<class TriMesh>		TriMeshRef;
	
class TriMesh : public geom::Source {
 public:
	class Format {
	  public:
		Format&		positions( uint8_t dims = 3 ) { mPositionsDims = dims; return *this; }
		Format&		normals() { mNormalsDims = 3; return *this; }
		Format&		colors( uint8_t dims = 3 ) { mColorsDims = dims; return *this; }
		//! Enables and establishes the dimensions of texture coords for unit 0
		Format&		texCoords( uint8_t dims = 2 ) { mTexCoords0Dims = dims; return *this; }
		//! Enables and establishes the dimensions of texture coords for unit 0
		Format&		texCoords0( uint8_t dims = 2 ) { mTexCoords0Dims = dims; return *this; }
		//! Enables and establishes the dimensions of texture coords for unit 1
		Format&		texCoords1( uint8_t dims = 2 ) { mTexCoords1Dims = dims; return *this; }
		//! Enables and establishes the dimensions of texture coords for unit 2
		Format&		texCoords2( uint8_t dims = 2 ) { mTexCoords2Dims = dims; return *this; }
		//! Enables and establishes the dimensions of texture coords for unit 3
		Format&		texCoords3( uint8_t dims = 2 ) { mTexCoords3Dims = dims; return *this; }
		
		uint8_t		mPositionsDims, mNormalsDims, mColorsDims;
		uint8_t		mTexCoords0Dims, mTexCoords1Dims, mTexCoords2Dims, mTexCoords3Dims;
	};

	static TriMeshRef	create( const Format &format ) { return TriMeshRef( new TriMesh( format ) ); }
	static TriMeshRef	create( const geom::Source &source ) { return TriMeshRef( new TriMesh( source ) ); }

	TriMesh( const Format &format );
	TriMesh( const geom::Source &source );
	
	void		clear();
	
	bool		hasNormals() const { return mNormalsDims > 0; }
	bool		hasColors() const { return mColorsDims > 0; }
	bool		hasColorsRgb() const { return mColorsDims == 3; }
	bool		hasColorsRgba() const { return mColorsDims == 4; }
	//! Returns whether the TriMesh has texture coordinates for unit 0
	bool		hasTexCoords() const { return mTexCoords0Dims > 0; }
	//! Returns whether the TriMesh has texture coordinates for unit 0
	bool		hasTexCoords0() const { return mTexCoords0Dims > 0; }
	//! Returns whether the TriMesh has texture coordinates for unit 1
	bool		hasTexCoords1() const { return mTexCoords1Dims > 0; }
	//! Returns whether the TriMesh has texture coordinates for unit 2
	bool		hasTexCoords2() const { return mTexCoords2Dims > 0; }
	//! Returns whether the TriMesh has texture coordinates for unit 3
	bool		hasTexCoords3() const { return mTexCoords3Dims > 0; }

	//! Creates a vertex which can be referred to with appendTriangle() or appendIndices() */
	void		appendVertex( const Vec2f &v ) { appendVertices( &v, 1 ); }
	//! Creates a vertex which can be referred to with appendTriangle() or appendIndices() */
	void		appendVertex( const Vec3f &v ) { appendVertices( &v, 1 ); }
	//! Appends multiple vertices to the TriMesh which can be referred to with appendTriangle() or appendIndices() */
	void		appendVertices( const Vec2f *verts, size_t num );
	//! Appends multiple vertices to the TriMesh which can be referred to with appendTriangle() or appendIndices() */
	void		appendVertices( const Vec3f *verts, size_t num );
	//! Appends multiple vertices to the TriMesh which can be referred to with appendTriangle() or appendIndices() */
	void		appendVertices( const Vec4d *verts, size_t num );
	//! Appends a single normal  */
	void		appendNormal( const Vec3f &v ) { mNormals.push_back( v ); }
	//! appendNormals functions similarly to the appendVertices method, appending multiple normals to be associated with the triangle faces. Normals and triangles are associated by index, so if you have created 3 vertices and one Triangle, you would append a single normal for the face of the triangles */
	void		appendNormals( const Vec3f *normals, size_t num );
	//! this sets the color used by a triangle generated by the TriMesh
	void		appendColorRgb( const Color &rgb ) { appendColors( &rgb, 1 ); }
	//! this sets the color used by a triangle generated by the TriMesh
	void		appendColorRgba( const ColorA &rgba ) { appendColors( &rgba, 1 ); }

	//! Synonym for appendTexCoord0; appends a texture coordinate for unit 0
	void		appendTexCoord( const Vec2f &v ) { appendTexCoords0( &v, 1 ); }
	//!	appends a 2D texture coordinate for unit 0
	void		appendTexCoord0( const Vec2f &v ) { appendTexCoords0( &v, 1 ); }
	//! appends a 2D texture coordinate for unit 1
	void		appendTexCoord1( const Vec2f &v ) { appendTexCoords1( &v, 1 ); }
	//! appends a 2D texture coordinate for unit 2
	void		appendTexCoord2( const Vec2f &v ) { appendTexCoords2( &v, 1 ); }
	//! appends a 2D texture coordinate for unit 3
	void		appendTexCoord3( const Vec2f &v ) { appendTexCoords3( &v, 1 ); }

	//!	appends a 3D texture coordinate for unit 0
	void		appendTexCoord0( const Vec3f &v ) { appendTexCoords0( &v, 1 ); }
	//! appends a 3D texture coordinate for unit 1
	void		appendTexCoord1( const Vec3f &v ) { appendTexCoords1( &v, 1 ); }
	//! appends a 3D texture coordinate for unit 2
	void		appendTexCoord2( const Vec3f &v ) { appendTexCoords2( &v, 1 ); }
	//! appends a 3D texture coordinate for unit 3
	void		appendTexCoord3( const Vec3f &v ) { appendTexCoords3( &v, 1 ); }

	//!	appends a 4D texture coordinate for unit 0
	void		appendTexCoord0( const Vec4f &v ) { appendTexCoords0( &v, 1 ); }
	//! appends a 4D texture coordinate for unit 1
	void		appendTexCoord1( const Vec4f &v ) { appendTexCoords1( &v, 1 ); }
	//! appends a 4D texture coordinate for unit 2
	void		appendTexCoord2( const Vec4f &v ) { appendTexCoords2( &v, 1 ); }
	//! appends a 4D texture coordinate for unit 3
	void		appendTexCoord3( const Vec4f &v ) { appendTexCoords3( &v, 1 ); }
	
	//! Appends multiple RGB colors to the TriMesh
	void		appendColors( const Color *rgbs, size_t num );
	//! Appends multiple RGBA colors to the TriMesh
	void		appendColors( const ColorA *rgbas, size_t num );
	
	//! Appends multiple 2D texcoords for unit 0
	void		appendTexCoords0( const Vec2f *texCoords, size_t num );
	//! Appends multiple 2D texcoords for unit 1
	void		appendTexCoords1( const Vec2f *texCoords, size_t num );
	//! Appends multiple 2D texcoords for unit 2
	void		appendTexCoords2( const Vec2f *texCoords, size_t num );
	//! Appends multiple 2D texcoords for unit 3
	void		appendTexCoords3( const Vec2f *texCoords, size_t num );
	
	//! Appends multiple 3D texcoords for unit 0
	void		appendTexCoords0( const Vec3f *texCoords, size_t num );
	//! Appends multiple 3D texcoords for unit 1
	void		appendTexCoords1( const Vec3f *texCoords, size_t num );
	//! Appends multiple 3D texcoords for unit 2
	void		appendTexCoords2( const Vec3f *texCoords, size_t num );
	//! Appends multiple 3D texcoords for unit 3
	void		appendTexCoords3( const Vec3f *texCoords, size_t num );

	//! Appends multiple 4D texcoords for unit 0
	void		appendTexCoords0( const Vec4f *texCoords, size_t num );
	//! Appends multiple 4D texcoords for unit 1
	void		appendTexCoords1( const Vec4f *texCoords, size_t num );
	//! Appends multiple 4D texcoords for unit 2
	void		appendTexCoords2( const Vec4f *texCoords, size_t num );
	//! Appends multiple 4D texcoords for unit 3
	void		appendTexCoords3( const Vec4f *texCoords, size_t num );
	
	/*! after creating three vertices, pass the indices of the three vertices to create a triangle from them. Until this is done, unlike in an OpenGL triangle strip, the 
	 triangle will not actually be generated and stored by the TriMesh
	*/
	void		appendTriangle( uint32_t v0, uint32_t v1, uint32_t v2 )
	{ mIndices.push_back( v0 ); mIndices.push_back( v1 ); mIndices.push_back( v2 ); }
	//! Appends \a num vertices to the TriMesh pointed to by \a indices
	void		appendIndices( const uint32_t *indices, size_t num );

	//! Returns the total number of indices contained by a TriMesh.
	size_t		getNumIndices() const override { return mIndices.size(); }
	//! Returns the total number of triangles contained by a TriMesh.
	size_t		getNumTriangles() const { return mIndices.size() / 3; }
	//! Returns the total number of indices contained by a TriMesh.
	virtual size_t	getNumVertices() const override { if( mPositionsDims ) return mPositions.size() / mPositionsDims; else return 0; }

	//! Puts the 3 vertices of triangle number \a idx into \a a, \a b and \a c. Assumes vertices are 3D
	void		getTriangleVertices( size_t idx, Vec3f *a, Vec3f *b, Vec3f *c ) const;
	//! Puts the 3 vertices of triangle number \a idx into \a a, \a b and \a c. Assumes vertices are 2D
	void		getTriangleVertices( size_t idx, Vec2f *a, Vec2f *b, Vec2f *c ) const;


	//! Returns all the vertices for a mesh in a std::vector as Vec<DIM>f. For example, to get 3D vertices, call getVertices<3>().
	template<uint8_t DIM>
	const typename VECDIM<DIM,float>::TYPE*	getVertices() const { assert(mPositionsDims==DIM); return (typename VECDIM<DIM,float>::TYPE*)mPositions.data(); }
	//! Returns all the vertices for a mesh in a std::vector as Vec<DIM>f. For example, to get 3D vertices, call getVertices<3>().
	template<uint8_t DIM>
	typename VECDIM<DIM,float>::TYPE*		getVertices() { assert(mPositionsDims==DIM); return (typename VECDIM<DIM,float>::TYPE*)mPositions.data(); }
	//! Returns all the normals for a mesh in a std::vector as Vec3f objects. There will be one of these for each triangle face in the mesh
	std::vector<Vec3f>&				getNormals() { return mNormals; }
	//! Returns all the normals for a mesh in a std::vector as Vec3f objects. There will be one of these for each triangle face in the mesh
	const std::vector<Vec3f>&		getNormals() const { return mNormals; }
	/*//! Returns a std::vector of RGB colors of the triangles faces. There will be one of these for each triangle face in the mesh
	std::vector<Color>&				getColorsRgb() { return mColorsRGB; }
	//! Returns a std::vector of RGB colors of the triangles faces. There will be one of these for each triangle face in the mesh
	const std::vector<Color>&		getColorsRgb() const { return mColorsRGB; }
	//! Returns a std::vector of RGBA colors of the triangles faces. There will be one of these for each triangle face in the mesh
	std::vector<ColorA>&			getColorsRgba() { return mColorsRGBA; }
	//! Returns a std::vector of RGBA colors of the triangles faces. There will be one of these for each triangle face in the mesh
	const std::vector<ColorA>&		getColorsRgba() const { return mColorsRGBA; }
	//! Returns a std::vector of Texture coordinates as Vec2fs. There will be one texture coord for each vertex in the TriMesh
	std::vector<Vec2f>&				getTexCoords() { return mTexCoords; }	
	//! Returns a std::vector of Texture coordinates as Vec2fs. There will be one texture coord for each vertex in the TriMesh
	const std::vector<Vec2f>&		getTexCoords() const { return mTexCoords; }	*/
	//! Trimesh indices are ordered such that the indices of triangle T are { indices[T*3+0], indices[T*3+1], indices[T*3+2] }
	std::vector<uint32_t>&			getIndices() { return mIndices; }		
	//! Trimesh indices are ordered such that the indices of triangle T are { indices[T*3+0], indices[T*3+1], indices[T*3+2] }
	const std::vector<uint32_t>&	getIndices() const { return mIndices; }		

	//! Calculates the bounding box of all vertices. Fails if the positions are not 3D.
	AxisAlignedBox3f	calcBoundingBox() const;
	//! Calculates the bounding box of all vertices as transformed by \a transform. Fails if the positions are not 3D.
	AxisAlignedBox3f	calcBoundingBox( const Matrix44f &transform ) const;

	//! This allows you read a TriMesh in from a data file, for instance an .obj file. At present .obj and .dat files are supported
	void		read( DataSourceRef in );
	//! This allows to you write a mesh out to a data file. At present .obj and .dat files are supported.
	void		write( DataTargetRef out ) const;

	//! Adds or replaces normals by calculating them from the vertices and faces. Requires 3D vertices.
	void		recalculateNormals();
	

	//! Create TriMesh from vectors of vertex data.
/*	static TriMesh		create( std::vector<uint32_t> &indices, const std::vector<ColorAf> &colors,
							   const std::vector<Vec3f> &normals, const std::vector<Vec3f> &positions,
							   const std::vector<Vec2f> &texCoords );*/

	/*! Subdivide vectors of vertex data into a TriMesh \a division times. Division less
	 than 2 returns the original mesh. */
	static TriMesh		subdivide( std::vector<uint32_t> &indices, const std::vector<ci::ColorAf>& colors, 
								  const std::vector<Vec3f> &normals, const std::vector<Vec3f> &positions,
								  const std::vector<Vec2f> &texCoords,
								  uint32_t division = 2, bool normalize = false );

	//! Subdivide a TriMesh \a division times. Division less than 2 returns the original mesh.
	static TriMesh		subdivide( const TriMesh &triMesh, uint32_t division = 2, bool normalize = false );	

	// geom::Source virtuals
	virtual geom::Primitive		getPrimitive() const override { return geom::Primitive::TRIANGLES; }
	
	virtual uint8_t		getAttribDims( geom::Attrib attr ) const override;	
	
	virtual void		copyIndices( uint16_t *dest ) const override;
	virtual void		copyIndices( uint32_t *dest ) const override;
	

  private:  
	void		copyAttrib( geom::Attrib attr, uint8_t dims, size_t stride, const float *srcData, size_t count ) const;

	uint8_t		mPositionsDims, mNormalsDims, mColorsDims;
	uint8_t		mTexCoords0Dims, mTexCoords1Dims, mTexCoords2Dims, mTexCoords3Dims;
  
	std::vector<float>		mPositions;
	std::vector<float>		mColors;
	std::vector<Vec3f>		mNormals; // always dim=3
	std::vector<float>		mTexCoords0, mTexCoords1, mTexCoords2, mTexCoords3;
	std::vector<uint32_t>	mIndices;
	
	friend class TriMeshGeomTarget;
};

} // namespace cinder
