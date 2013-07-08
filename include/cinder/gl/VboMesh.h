/*
 * NOT FINISHED
 */

#pragma once

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class VboMesh> VboMeshRef;
	
void draw( const VboMeshRef& vbo );
void drawRange( const VboMeshRef& vbo, GLint start, GLsizei count );

class VboMesh {
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
