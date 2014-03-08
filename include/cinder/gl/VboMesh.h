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

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/geomIo.h"

#include <ostream>
#include <vector>

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class VboMesh> VboMeshRef;
	
void draw( const VboMeshRef& vbo );
void drawRange( const VboMeshRef& vbo, GLint start, GLsizei count );

class VboMesh {
  public:
	//! Creates a VboMesh which represents the geom::Source \a source.
	static VboMeshRef	create( const geom::Source &source );
	//! Creates a VboMesh which represents the user's vertex buffer objects. Allows optional \a indexVbo to enable indexed vertices.
	static VboMeshRef	create( uint32_t numVertices, GLenum glPrimitive, const std::vector<std::pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, uint32_t numIndices = 0, GLenum indexType = GL_UNSIGNED_BYTE, const VboRef &indexVbo = VboRef() );
	//! Creates a VboMesh which represents the geom::Source \a source. Allows optional \a arrayVbo and \a elementArrayVbo in order to simplify recycling of VBOs.
	static VboMeshRef	create( const geom::Source &source, const VboRef &arrayVbo, const VboRef &elementArrayVbo );

	//! Maps a geom::Attrib to a named attribute in the GlslProg
	typedef std::map<geom::Attrib,std::string> AttributeMapping;
	//! Constructs a VAO (in the currently bound VAO) that matches \a this to GlslProg \a shader, overriding the mapping of a geom::Attrib to a named attribute via the 'a attributeMapping std::map
	void		buildVao( const GlslProgRef &shader, const AttributeMapping &attributeMapping = AttributeMapping() );

	//! Issues a glDraw* call, but without binding a VAO or sending shader vars. Consider gl::draw( VboMeshRef ) instead. Knows whether to call glDrawArrays or glDrawElements
	void		drawImpl();

	//! Returns the number of vertices in the mesh
	uint32_t	getNumVertices() const { return mNumVertices; }
	//! Returns the number of indices for indexed geometry, otherwise 0
	uint32_t	getNumIndices() const { return mNumIndices; }
	//! Returns the primitive type, such as GL_TRIANGLES, GL_TRIANGLE_STRIP, etc
	GLenum		getGlPrimitive() const { return mGlPrimitive; }
	//! Returns the data type of the indices contained in element vbo; either GL_UNSIGNED_SHORT or GL_UNSIGNED_INT
	GLenum		getIndexDataType() const { return mIndexType; }

	//! Returns 0 if \a attr is not present
	uint8_t		getAttribDims( geom::Attrib attr ) const;

	//! Returns the VBO containing the elements of the mesh, or a NULL for non-indexed geometry
	VboRef		getElementVbo() { return mElements; }

	//! Builds and returns a vector of VboRefs for the vertex data of the mesh
	std::vector<VboRef>									getVertexArrayVbos();
	//! Returns the vector of pairs of (BufferLayout,VboRef) for the vertex data of the mesh
	const std::vector<std::pair<geom::BufferLayout,VboRef>>&	getVertexArrayLayoutVbos() const { return mVertexArrayVbos; }
	//! Adds a new VBO (paired with its geom::BufferLayout) to the VboMesh
	void												appendVbo( const geom::BufferLayout &layout, const VboRef &vbo );

#if ! defined( CINDER_GL_ES )
	//! Returns a geom::Source which references 'this'. Inefficient - primarily useful for debugging. The returned geom::SourceRef should not outlive 'this' (not a shared_ptr).
	geom::SourceRef	createSource() const;
	
	//! Copies indices to \a dest. Requires \a dest to provide storage for getNumIndices() indices. Inefficient - primarily useful for debugging.
	void		downloadIndices( uint32_t *dest ) const;
	
	//! Echos all vertex data in range [\a startIndex, \a endIndex) to \a os. Inefficient - primarily useful for debugging.
	void		echoVertexRange( std::ostream &os, size_t startIndex, size_t endIndex );
	//! Echos all vertex data for the elements in range [\a startIndex, \a endIndex) to \a os. No-op for non-indexed geometry. Inefficient - primarily useful for debugging.
	void		echoElementRange( std::ostream &os, size_t startIndex, size_t endIndex );
#endif

  protected:
	VboMesh( const geom::Source &source, const VboRef &arrayVbo, const VboRef &elementArrayVbo );
	VboMesh( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<std::pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo );

	void	echoVertices( std::ostream &os, const std::vector<uint32_t> &elements, bool printElements );

	uint32_t			mNumVertices, mNumIndices;
	GLenum				mGlPrimitive;
	GLenum				mIndexType;

	std::vector<std::pair<geom::BufferLayout,VboRef>>	mVertexArrayVbos;
	VboRef												mElements;
	
	friend class VboMeshGeomTarget;
};

} } // namespace cinder::gl
