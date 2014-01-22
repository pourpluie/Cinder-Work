#pragma once

#include "cinder/Cinder.h"

#if defined( CINDER_GL_ANGLE )
	#define GL_GLEXT_PROTOTYPES
	#include "GLES2/gl2.h"
	#include "GLES2/gl2ext.h"
	#define CINDER_GLES
	#define CINDER_GLES2
	#pragma comment( lib, "libEGL.lib" )
	#pragma comment( lib, "libGLESv2.lib" )
#elif ! defined( CINDER_COCOA_TOUCH )
	#if defined( __clang__ )
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wtypedef-redefinition"
	#endif
	#if ! defined( CINDER_GL_LEGACY )
		#include "glload/gl_core.h"
	#else
		#include "glload/gl_all.h"
	#endif
	#if defined( __clang__ )
		#pragma clang diagnostic pop
	#endif
#else
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
	#define CINDER_GLES
	#define CINDER_GLES2
#endif

#include "cinder/gl/Texture.h"

#include "cinder/Area.h"
#include "cinder/Rect.h"
#include "cinder/Exception.h"
#include "cinder/Color.h"
#include "cinder/Camera.h"
#include "cinder/Matrix44.h"
#include "cinder/geomIo.h"

namespace cinder { namespace gl {

// Remember to add a matching case to uniformSemanticToString
enum UniformSemantic {
	UNIFORM_MODELVIEW,
	UNIFORM_MODELVIEWPROJECTION,
	UNIFORM_PROJECTION,
	UNIFORM_NORMAL_MATRIX
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
class Vao;
typedef std::shared_ptr<Vao>		VaoRef;

class Context* context();
class Environment* env();

void enableVerticalSync( bool enable = true );
bool isVerticalSyncEnabled();
bool isExtensionAvailable( const std::string &extName );
//! Returns the OpenGL version number as a pair<major,minor>
std::pair<GLint,GLint>	getVersion();
std::string getVersionString();

GlslProgRef	getStockShader( const class ShaderDef &shader );
void bindStockShader( const class ShaderDef &shader );
void setDefaultShaderVars();

void clear( const ColorA &color = ColorA::black(), bool clearDepthBuffer = true );
	
void clear( GLbitfield mask );
void clearColor( const ColorA &color );
void clearDepth( const double depth );
void clearDepth( const float depth );
void clearStencil( const int s );
	
void colorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
void depthMask( GLboolean flag );
void stencilMask( GLboolean mask );
	
void stencilFunc( GLenum func, GLint ref, GLuint mask );
void stencilOp( GLenum fail, GLenum zfail, GLenum zpass );

std::pair<Vec2i, Vec2i> getViewport();
void viewport( int x, int y, int width, int height );
void viewport( const Vec2i &position, const Vec2i &size );
inline void viewport( const Vec2i &size ) { viewport( Vec2f::zero(), size ); }
void pushViewport( const Vec2i &position, const Vec2i &size );
inline void pushViewport( const Vec2i &size ) { pushViewport( Vec2f::zero(), size ); }
void popViewport();

std::pair<Vec2i, Vec2i> getScissor();
void scissor( int x, int y, int width, int height );
void scissor( const Vec2i &position, const Vec2i &dimension );
	
void enable( GLenum state, bool enable = true );
inline void disable( GLenum state ) { enable( state, false ); }

void enableAlphaBlending( bool premultiplied = false );
void disableAlphaBlending();
void enableAdditiveBlending();

void enableAlphaTest( float value = 0.5f, int func = GL_GREATER );
void disableAlphaTest();

void disableDepthRead();
void disableDepthWrite();
void enableDepthRead( bool enable = true );
void enableDepthWrite( bool enable = true );

void enableStencilRead( bool enable = true );
void disableStencilRead();
void enableStencilWrite( bool enable = true );
void disableStencilWrite();

void setMatrices( const ci::Camera &cam );
void setModelView( const ci::Matrix44f &m );
void setModelView( const ci::Camera &cam );
void setProjection( const ci::Camera &cam );
void setProjection( const ci::Matrix44f &m );
void pushModelView();
void popModelView();
void pushProjection();
void popProjection();
void pushMatrices();
void popMatrices();
void multModelView( const ci::Matrix44f &mtx );
void multProjection( const ci::Matrix44f &mtx );

Matrix44f getModelView();
Matrix44f getProjection();
Matrix44f getModelViewProjection();
Matrix33f calcNormalMatrix();

void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
void setMatricesWindowPersp( const ci::Vec2i &screenSize, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft = true );
void setMatricesWindow( const ci::Vec2i &screenSize, bool originUpperLeft = true );

void rotate( float angleDegrees, float xAxis, float yAxis, float zAxis );
void rotate( const cinder::Quatf &quat );
inline void rotate( float zDegrees ) { rotate( zDegrees, 0, 0, 1 ); }

void scale( const ci::Vec3f &v );
inline void scale( float x, float y, float z ) { scale( Vec3f( x, y, z ) ); }
inline void scale( const ci::Vec2f &v ) { scale( Vec3f( v.x, v.y, 1 ) ); }
inline void scale( float x, float y ) { scale( Vec3f( x, y, 1 ) ); }

void translate( const ci::Vec3f &v );
inline void translate( float x, float y, float z ) { translate( Vec3f( x, y, z ) ); }
inline void translate( const ci::Vec2f &v ) { translate( Vec3f( v, 0 ) ); }
inline void translate( float x, float y ) { translate( Vec3f( x, y, 0 ) ); }
	
void begin( GLenum mode );
void end();

#if ! defined( CINDER_GLES )
void bindBufferBase( GLenum target, int index, BufferObjRef buffer );
	
void beginTransformFeedback( GLenum primitiveMode );
void endTransformFeedback();
void resumeTransformFeedback();
void pauseTransformFeedback();	
#endif
	
void color( float r, float g, float b );
void color( float r, float g, float b, float a );
void color( const ci::Color &c );
void color( const ci::ColorA &c );
void color( const ci::Color8u &c );
void color( const ci::ColorA8u &c );

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

//! Converts a geom::Primitive to an OpenGL primitive mode( GL_TRIANGLES, GL_TRIANGLE_STRIP, etc )
GLenum toGl( geom::Primitive prim );
//! Converts an OpenGL primitive mode( GL_TRIANGLES, GL_TRIANGLE_STRIP, etc ) to a geom::Primitive
geom::Primitive toGeomPrimitive( GLenum prim );
//! Converts an OpenGL type enum ( \c GL_BYTE series, \c GL_FLOAT_VEC3 series ) to a string, without GL_ prefix
std::string typeToString( GLenum type );
//! Converts a UniformSemantic to its name
std::string uniformSemanticToString( UniformSemantic uniformSemantic );

void draw( const VboMeshRef &mesh );
void draw( const TextureRef &texture, const Rectf &rect );
void draw( const TextureRef &texture, const Vec2f &v = Vec2f::zero() );

//! Renders a solid cube centered at \a center of size \a size. Normals and created texture coordinates are generated.
void drawCube( const Vec3f &center, const Vec3f &size );
//! Renders a solid cube centered at \a center of size \a size. Each face is assigned a unique color.
void drawColorCube( const Vec3f &center, const Vec3f &size );
//! Renders a solid sphere at \a center of radius \a radius, subdivided on both longitude and latitude into \a segments.
void drawSphere( const Vec3f &center, float radius, int segments );
//! Draws a textured quad of size \a scale that is aligned with the vectors \a bbRight and \a bbUp at \a pos, rotated by \a rotationRadians around the vector orthogonal to \a bbRight and \a bbUp.
void drawBillboard( const Vec3f &pos, const Vec2f &scale, float rotationRadians, const Vec3f &bbRight, const Vec3f &bbUp, const Rectf &texCoords = Rectf( 0, 0, 1, 1 ) );

//! Draws \a texture on the XY-plane
void drawSolidRect( const Rectf &r );
void drawSolidRect( const Rectf &r, const Rectf &texcoords );
void drawSolidCircle( const Vec2f &center, float radius, int numSegments = -1 );

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

class Exception : public cinder::Exception {
};

class ExceptionUnknownTarget : public Exception {
};

} }
