/*
 * NOT FINISHED
 */

#pragma once

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/geomIo.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class VboMesh> VboMeshRef;
	
void draw( const VboMeshRef& vbo );
void drawRange( const VboMeshRef& vbo, GLint start, GLsizei count );

class VboMesh {
  public:
	static VboMeshRef	create( const geom::Source &source );
	static VboMeshRef	create( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<std::pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo = VboRef() );

	//! Constructs a VAO that matches \a this to GlslProg \a shader
	VaoRef		buildVao( const GlslProgRef &shader );
	//! Issues a glDraw* call, but without binding a VAO or sending shader vars. Consider gl::draw( VboMeshRef ) instead
	void		drawImpl();

	//! Returns the number of vertices in the mesh
	uint32_t	getNumVertices() const { return mNumVertices; }
	//! Returns the number of indices for indexed geometry, otherwise 0
	uint32_t	getNumIndices() const { return mNumIndices; }
	//! Returns the primitive type, such as GL_TRIANGLES, GL_TRIANGLE_STRIP, etc
	GLenum		getGlPrimitive() const { return mGlPrimitive; }
	//! Returns the data type of the indices contained in element vbo; either GL_UNSIGNED_SHORT or GL_UNSIGNED_INT
	GLenum		getIndexDataType() const { return mIndexType; }

	//! Returns the VBO containing the elements of the mesh, or a NULL for non-indexed geometry
	VboRef		getElementVbo() { return mElements; }

	//! Builds and returns a vector of VboRefs for the vertex data of the mesh
	std::vector<VboRef>									getVertexArrayVbos();
	//! Returns the vector of pairs of (BufferLayout,VboRef) for the vertex data of the mesh
	std::vector<std::pair<geom::BufferLayout,VboRef>>&	getVertexArrayLayoutVbos() { return mVertexArrayVbos; }

  protected:
	VboMesh( const geom::Source &source );
	VboMesh( uint32_t numVertices, uint32_t numIndices, GLenum glPrimitive, GLenum indexType, const std::vector<std::pair<geom::BufferLayout,VboRef>> &vertexArrayBuffers, const VboRef &indexVbo );

	uint32_t			mNumVertices, mNumIndices;
	GLenum				mGlPrimitive;
	GLenum				mIndexType;


	std::vector<std::pair<geom::BufferLayout,VboRef>>	mVertexArrayVbos;
	VboRef												mElements;
	
/*  public:	
	class Layout {
	  public:
		enum
		{
			NONE, STATIC, DYNAMIC
		} typedef Usage;
		
		Layout();
		
		GLuint	getColorAttribLocation() const;
		void	setColorAttribLocation( GLuint index );
		
		GLuint	getNormalAttribLocation() const;
		void	setNormalAttribLocation( GLuint index );
		
		GLuint	getPositionAttribLocation() const;
		void	setPositionAttribLocation( GLuint index );
		
		GLuint	getTexCoordAttribLocation() const;
		void	setTexCoordAttribLocation( GLuint index );
		
		Usage	getColorUsage() const;
		void	setColorUsage( Usage u );
		
		Usage	getNormalUsage() const;
		void	setNormalUsage( Usage u );
		
		Usage	getPositionUsage() const;
		void	setPositionUsage( Usage u );
		
		Usage	getTexCoordUsage() const;
		void	setTexCoordUsage( Usage u );
		
		Usage	getIndexUsage() const;
		void	setIndexUsage( Usage u );
	  protected:
		GLuint	mAttrColor;
		GLuint	mAttrNormal;
		GLuint	mAttrPosition;
		GLuint	mAttrTexCoord;
		
		Usage	mUsageColor;
		Usage	mUsageIndex;
		Usage	mUsageNormal;
		Usage	mUsagePosition;
		Usage	mUsageTexCoord;
	};
	
	/////////////////////////////////////////////////////////////////

	static VboMeshRef	create( size_t numVertices, const Layout& layout = Layout(), GLenum mode = GL_TRIANGLES );
	static VboMeshRef	create( size_t numIndices, size_t numVertices, const Layout& layout = Layout(), GLenum mode = GL_TRIANGLES );
	static VboMeshRef	create( const ci::TriMesh& mesh, const Layout& layout = Layout() );
	
//	void	bind() const;
	void	unbind() const;
	
	void	bufferIndices( const std::vector<GLuint>& indices );
	void	bufferColors( const std::vector<ci::ColorAf>& colors );
	void	bufferNormals( const std::vector<ci::Vec3f>& normals );
	void	bufferPositions( const std::vector<ci::Vec3f>& positions );
	void	bufferTexCoords( const std::vector<ci::Vec2f>& texCoords );
	void	bufferTexCoords( const std::vector<ci::Vec3f>& texCoords );
	void	bufferTexCoords( const std::vector<ci::Vec4f>& texCoords );
	
	const Layout&	getLayout() const { return mLayout; }
	Layout&			getLayout() { return mLayout; }

  protected:
	VboMesh( size_t numVertices, const Layout& layout, GLenum mode );
	VboMesh( size_t numIndices, size_t numVertices, const Layout& layout, GLenum mode );
	VboMesh( const ci::TriMesh& mesh, const Layout& layout );
	
	void	initializeBuffers();
	
	VaoRef	mVao;
	VboRef	mVboIndices;
	VboRef	mVboVerticesStatic;
	VboRef	mVboVerticesDynamic;
	
	Layout	mLayout;
	GLenum	mMode;
	size_t	mNumIndices;
	size_t	mNumVertices;
	size_t	mOffsetColor;
	size_t	mOffsetNormal;
	size_t	mOffsetPosition;
	size_t	mOffsetTexCoord;
	
	friend void		ci::gl::draw( const VboMeshRef& mesh );
	friend void		ci::gl::drawRange( const VboMeshRef& mesh, GLint start, GLsizei count );*/
};

} }
