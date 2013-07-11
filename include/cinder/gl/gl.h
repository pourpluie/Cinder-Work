#pragma once

#include "cinder/Cinder.h"

#if ! defined( CINDER_COCOA_TOUCH )
	#include "glload/gl_all.h"
#else
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
	#define CINDER_GLES
	#define CINDER_GLES2
#endif

#include "cinder/Area.h"
#include "cinder/Rect.h"
#include "cinder/Exception.h"
#include "cinder/Color.h"
#include "cinder/Camera.h"
#include "cinder/Matrix44.h"

namespace cinder { namespace gl {

enum UniformSemantic {
	UNIFORM_MODELVIEWPROJECTION, UNIFORM_COLOR
};

enum AttribSemantic {
	ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_TEX_COORD_0, ATTRIB_COLOR
};

class Vbo;
typedef std::shared_ptr<Vbo>		VboRef;
class VboMesh;
typedef std::shared_ptr<VboMesh>	VboMeshRef;
class Texture;
typedef std::shared_ptr<Texture>	TextureRef;
class BufferObj;
typedef std::shared_ptr<BufferObj>	BufferObjRef;
class GlslProg;
typedef std::shared_ptr<GlslProg>	GlslProgRef;

class Context* context();
class Environment* env();

void enableVerticalSync( bool enable = true );
bool isVerticalSyncEnabled();
bool isExtensionAvailable( const std::string &extName );

GlslProgRef	getStockShader( const class ShaderDef &shader );
void setDefaultShaderUniforms();

void clear( const ColorA &color = ColorA::black(), bool clearDepthBuffer = true );

Area getViewport();
void setViewport( const Area &area );

void enable( GLenum state, bool enable = true );
inline void disable( GLenum state ) { enable( state, false ); }

void enableAlphaBlending( bool premultiplied = false );
void disableAlphaBlending();
void enableAdditiveBlending();

void enableAlphaTest( float value = 0.5f, int func = GL_GREATER );
void disableAlphaTest();

void enableLighting( bool enable = true );
void disableLighting();

void enableWireframe( bool enable = true );
void disableWireframe();
	
void disableDepthRead();
void disableDepthWrite();
void enableDepthRead( bool enable = true );
void enableDepthWrite( bool enable = true );

void setMatrices( const ci::Camera &cam );
void setModelView( const ci::Matrix44f &m );
void setModelView( const ci::Camera &cam );
void setProjection( const ci::Camera &cam );
void setProjection( const ci::Matrix44f &m );
void pushModelView();
void popModelView();
void pushModelView( const ci::Camera &cam );
void pushProjection( const ci::Camera &cam );
void pushMatrices();
void popMatrices();
void multModelView( const ci::Matrix44f &mtx );
void multProjection( const ci::Matrix44f &mtx );

Matrix44f getModelView();
Matrix44f getProjection();
Matrix33f calcNormalMatrix();

void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
void setMatricesWindowPersp( const ci::Vec2i &screenSize, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft = true );
void setMatricesWindow( const ci::Vec2i &screenSize, bool originUpperLeft = true );

void rotate( const ci::Vec3f& v );
void scale( const ci::Vec3f& v );
inline void scale( float x, float y, float z ) { scale( Vec3f( x, y, z ) ); }
void translate( const ci::Vec3f& v );
inline void translate( float x, float y, float z ) { translate( Vec3f( x, y, z ) ); }
	
void begin( GLenum mode );
void end();

void color( float r, float g, float b );
void color( float r, float g, float b, float a );
void color( const ci::Color &c );
void color( const ci::ColorA &c );
void color( const ci::Color8u &c );
void color( const ci::ColorA8u &c );

void normal( const ci::Vec3f &v );

void texCoord( float s );
void texCoord( float s, float t );
void texCoord( float s, float t, float r );
void texCoord( float s, float t, float r, float q );
void texCoord( const ci::Vec2f &v );
void texCoord( const ci::Vec3f &v );
void texCoord( const ci::Vec4f &v );

void vertex( float x, float y );
void vertex( float x, float y, float z );
void vertex( float x, float y, float z, float w );
void vertex( const ci::Vec2f &v );
void vertex( const ci::Vec3f &v );
void vertex( const ci::Vec4f &v );

#if ! defined( CINDER_GLES )
void polygonMode( GLenum face, GLenum mode );
#endif

void draw( const VboMeshRef &mesh );
void draw( const TextureRef &texture, const Vec2f &offset = Vec2f::zero() );
void draw( const TextureRef &texture, const Rectf &rect );
void drawRange( const VboMeshRef& mesh, GLint start = 0, GLsizei count = 0 );
void draw( const VboRef &vbo );
void drawRange( const VboRef& vbo, GLint start = 0, GLsizei count = 0 );

//! Renders a solid cube centered at \a center of size \a size. Normals and created texture coordinates are generated.
void drawCube( const Vec3f &center, const Vec3f &size );
//! Renders a solid cube centered at \a center of size \a size. Each face is assigned a unique color.
void drawColorCube( const Vec3f &center, const Vec3f &size );
void drawTorus( float outerRadius, float innerRadius, int longitudeSegments, int latitudeSegments );


// Vertex Attributes
//! Analogous to glVertexAttribPointer
void	vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
//! Analogous to glEnableVertexAttribArray
void	enableVertexAttribArray( GLuint index );

void		vertexAttrib1f( GLuint index, float v0 );
inline void	vertexAttrib( GLuint index, float v0 ) { vertexAttrib1f( index, v0 ); }
void		vertexAttrib2f( GLuint index, float v0, float v1 );
inline void	vertexAttrib( GLuint index, float v0, float v1 ) { vertexAttrib2f( index, v0, v1 ); }
void		vertexAttrib3f( GLuint index, float v0, float v1, float v2 );
inline void	vertexAttrib( GLuint index, float v0, float v1, float v2 ) { vertexAttrib3f( index, v0, v1, v2 ); }
inline void	vertexAttrib( GLuint index, float v0, float v1 );
void		vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 );
inline void	vertexAttrib( GLuint index, float v0, float v1, float v2, float v3 ) { vertexAttrib4f( index, v0, v1, v2, v3 ); }

// Buffers
void	bindBuffer( const BufferObjRef &buffer );

void	drawArrays( GLenum mode, GLint first, GLsizei count );
void	drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
	
GLenum getError();
std::string getErrorString( GLenum err );

struct SaveTextureBindState
{
	SaveTextureBindState( GLint target );
	~SaveTextureBindState();
private:
	GLint	mTarget;
	GLint	mOldID;
};

struct BoolState
{
	BoolState( GLint target );
	~BoolState();
private:
	GLint		mTarget;
	GLboolean	mOldValue;
};

struct ClientBoolState
{
	ClientBoolState( GLint target );
	~ClientBoolState();
private:
	GLint		mTarget;
	GLboolean	mOldValue;
};

struct SaveColorState
{
	SaveColorState();
	~SaveColorState();
private:
	GLfloat		mOldValues[ 4 ];
};

class Exception : public cinder::Exception {
};

class ExceptionUnknownTarget : public Exception {
};

} }
