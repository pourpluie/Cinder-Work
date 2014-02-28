#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Environment.h"

#include "cinder/Utilities.h"
#include "cinder/Log.h"
#include <vector>

#if defined( CINDER_MSW )
	#include "glload/wgl_all.h"
#elif defined( CINDER_MAC )
	#include <OpenGL/OpenGL.h>
#endif

using namespace std;

namespace cinder { namespace gl {

Context* context()
{
	return Context::getCurrent();
}

void enableVerticalSync( bool enable )
{
#if defined( CINDER_MAC )
	GLint sync = ( enable ) ? 1 : 0;
	::CGLSetParameter( ::CGLGetCurrentContext(), kCGLCPSwapInterval, &sync );
#elif defined( CINDER_MSW ) && ! defined( CINDER_GL_ANGLE )
	GLint sync = ( enable ) ? 1 : 0;
	if( wglext_EXT_swap_control )
		::wglSwapIntervalEXT( sync );
#endif
}

bool isVerticalSyncEnabled()
{
#if defined( CINDER_MAC )
	GLint enabled;
	::CGLGetParameter( ::CGLGetCurrentContext(), kCGLCPSwapInterval, &enabled );
	return enabled > 0;
#elif defined( CINDER_MSW ) && ! defined( CINDER_GL_ANGLE )
	if( wglext_EXT_swap_control )
		return ::wglGetSwapIntervalEXT() > 0;
	else
		return true;
#else
	return true;
#endif
}

bool isExtensionAvailable( const std::string &extName )
{
	return env()->isExtensionAvailable( extName );
}

std::pair<GLint,GLint> getVersion()
{
	//hard-coded for now
#if defined( CINDER_GLES )
	return std::make_pair( (GLint)2, (GLint)0 );
#else
	static bool	sInitialized = false;
	static pair<GLint,GLint> sVersion;
	if( ! sInitialized ) {
		// adapted from LoadOGL
		const char *strVersion = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
		GLint major = 0, minor = 0;
		const char *strDotPos = NULL;
		int iLength = 0;
		char strWorkBuff[10];

		strDotPos = strchr( strVersion, '.' );
		if( ! strDotPos )
			return std::make_pair( 0, 0 );

		iLength = (int)((ptrdiff_t)strDotPos - (ptrdiff_t)strVersion);
		strncpy(strWorkBuff, strVersion, iLength);
		strWorkBuff[iLength] = '\0';

		major = atoi(strWorkBuff);
		strDotPos = strchr( strVersion + iLength + 1, ' ' );
		if( ! strDotPos ) { // No extra data. Take the whole rest of the string.
			strcpy( strWorkBuff, strVersion + iLength + 1 );
		}
		else {
			// Copy only up until the space.
			int iLengthMinor = (int)((ptrdiff_t)strDotPos - (ptrdiff_t)strVersion); 
			iLengthMinor = iLengthMinor - (iLength + 1);
			strncpy( strWorkBuff, strVersion + iLength + 1, iLengthMinor );
			strWorkBuff[iLengthMinor] = '\0';
		}

		minor = atoi( strWorkBuff );
		sVersion = std::make_pair( major, minor );
		sInitialized = true;
	}

	return sVersion;
#endif
}

std::string getVersionString()
{
	const GLubyte* s = glGetString( GL_VERSION );

	return std::string( reinterpret_cast<const char*>( s ) );
}

GlslProgRef	getStockShader( const class ShaderDef &shader )
{
	return context()->getStockShader( shader );
}

void bindStockShader( const class ShaderDef &shaderDef )
{
	auto ctx = gl::context();
	auto shader = ctx->getStockShader( shaderDef );
	ctx->bindGlslProg( shader );
}

void setDefaultShaderVars()
{
	gl::context()->setDefaultShaderVars();
}

void clear( const ColorA& color, bool clearDepthBuffer )
{
	clearColor( color );
	if ( clearDepthBuffer ) {
		depthMask( GL_TRUE );
		clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	} else {
		clear( GL_COLOR_BUFFER_BIT );
	}
}
    
void clear( GLbitfield mask )
{
    glClear( mask );
}
    
void clearColor( const ColorA &color )
{
    glClearColor( color.r, color.g, color.b, color.a );
}
	
void clearDepth( const double depth )
{
#if ! defined( CINDER_GLES )
    glClearDepth( depth );
#else
	glClearDepthf( depth );
#endif
}
    
void clearDepth( const float depth )
{
    glClearDepthf( depth );
}
    
void clearStencil( const int s )
{
    glClearStencil( s );
}
    
void colorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
    glColorMask( red, green, blue, alpha );
}
    
void depthMask( GLboolean flag )
{
    gl::context()->depthMask( flag );
}
	
void stencilMask( GLboolean mask )
{
	glStencilMask( mask );
}
    
void stencilFunc( GLenum func, GLint ref, GLuint mask )
{
    glStencilFunc( func, ref, mask );
}
    
void stencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
    glStencilOp( fail, zfail, zpass );
}

std::pair<Vec2i, Vec2i> getViewport()
{
	auto view = gl::context()->getViewport();
	return view;
}

void viewport( int x, int y, int width, int height )
{
	viewport( Vec2i( x, y ), Vec2i( width, height ) ) ;
}

void viewport( const Vec2i &position, const Vec2i &size )
{
	gl::context()->viewport( std::pair<Vec2i, Vec2i>( position, size ) );
}

void pushViewport( const Vec2i &position, const Vec2i &size )
{
	gl::context()->pushViewport( std::pair<Vec2i, Vec2i>( position, size ) );
}

void popViewport()
{
	gl::context()->popViewport();
}

std::pair<Vec2i, Vec2i> getScissor()
{
	auto scissor = gl::context()->getScissor();
	return scissor;
}

void scissor( int x, int y, int width, int height )
{
	scissor( Vec2i( x, y ), Vec2i( width, height ) );
}

void scissor( const Vec2i &position, const Vec2i &dimension )
{
	gl::context()->setScissor( std::pair<Vec2i, Vec2i>( position, dimension ) );
}

void enable( GLenum state, bool enable )
{
	gl::context()->enable( state, enable );
}

void enableAlphaBlending( bool premultiplied )
{
	auto ctx = gl::context();
	ctx->enable( GL_BLEND );
	if( ! premultiplied ) {
		ctx->blendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		ctx->blendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	}
}

void disableAlphaBlending()
{
	gl::disable( GL_BLEND );
}

void enableAdditiveBlending()
{
	auto ctx = gl::context();
	ctx->enable( GL_BLEND );
	ctx->blendFunc( GL_SRC_ALPHA, GL_ONE );
}

void disableDepthRead()
{
	gl::disable( GL_DEPTH_TEST );
}

void enableDepthRead( bool enable )
{
	gl::enable( GL_DEPTH_TEST, enable );
}

void enableDepthWrite( bool enable )
{
	gl::context()->depthMask( enable ? GL_TRUE : GL_FALSE );
}

void disableDepthWrite()
{
	gl::context()->depthMask( GL_FALSE );
}
    
void enableStencilTest( bool enable )
{
    gl::enable( GL_STENCIL_TEST, enable );
}
    
void disableStencilTest()
{
    gl::disable( GL_STENCIL_TEST );
}

void setMatrices( const ci::Camera& cam )
{
	auto ctx = context();
	ctx->getModelViewStack().back() = cam.getModelViewMatrix();
	ctx->getProjectionStack().back() = cam.getProjectionMatrix();
}

void setModelView( const ci::Matrix44f &m )
{
	context()->getModelViewStack().back() = m;
}

void setModelView( const ci::Camera& cam )
{
	context()->getModelViewStack().back() = cam.getModelViewMatrix();
}

void setProjection( const ci::Camera& cam )
{
	context()->getProjectionStack().back() = cam.getProjectionMatrix();
}

void setProjection( const ci::Matrix44f &m )
{
	context()->getProjectionStack().back() = m;
}

void pushModelView()
{
	auto ctx = context();
	ctx->getModelViewStack().push_back( ctx->getModelViewStack().back() );
}

void popModelView()
{
	context()->getModelViewStack().pop_back();
}

void pushProjection()
{
	auto ctx = context();
	ctx->getProjectionStack().push_back( ctx->getProjectionStack().back() );
}

void popProjection()
{
	context()->getProjectionStack().pop_back();
}

void pushMatrices()
{
	auto ctx = context();
	ctx->getModelViewStack().push_back( ctx->getModelViewStack().back() );
	ctx->getProjectionStack().push_back( ctx->getProjectionStack().back() );
}

void popMatrices()
{
	auto ctx = context();
	ctx->getModelViewStack().pop_back();
	ctx->getProjectionStack().pop_back();
}

void multModelView( const ci::Matrix44f& mtx )
{
	auto ctx = gl::context();
	ctx->getModelViewStack().back() *= mtx;
}

void multProjection( const ci::Matrix44f& mtx )
{
	auto ctx = gl::context();
	ctx->getProjectionStack().back() *= mtx;
}

Matrix44f getModelView()
{
	auto ctx = gl::context();
	return ctx->getModelViewStack().back();
}

Matrix44f getProjection()
{
	auto ctx = gl::context();
	return ctx->getProjectionStack().back();
}

Matrix44f getModelViewProjection()
{
	auto ctx = context();
	return ctx->getProjectionStack().back() * ctx->getModelViewStack().back();
}

Matrix33f calcNormalMatrix()
{
	Matrix33f mv = getModelView().subMatrix33( 0, 0 );
	mv.invert( FLT_MIN );
	mv.transpose();
	return mv;
}

void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	auto ctx = gl::context();
	
	CameraPersp cam( screenWidth, screenHeight, fovDegrees, nearPlane, farPlane );
	ctx->getProjectionStack().back() = cam.getProjectionMatrix();
	ctx->getModelViewStack().back() = cam.getModelViewMatrix();
	if( originUpperLeft ) {
		ctx->getModelViewStack().back().scale( Vec3f( 1.0f, -1.0f, 1.0f ) );					// invert Y axis so increasing Y goes down.
		ctx->getModelViewStack().back().translate( Vec3f( 0.0f, (float)-screenHeight, 0.0f ) ); // shift origin up to upper-left corner.
	}
}

void setMatricesWindowPersp( const ci::Vec2i& screenSize, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	setMatricesWindowPersp( screenSize.x, screenSize.y, fovDegrees, nearPlane, farPlane, originUpperLeft );
}

void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft )
{
	auto ctx = gl::context();
	ctx->getModelViewStack().back().setToIdentity();
	if( originUpperLeft )
		ctx->getProjectionStack().back().setRows(	Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
													Vec4f( 0.0f, 2.0f / -(float)screenHeight, 0.0f, 1.0f ),
													Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
													Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	else
		ctx->getProjectionStack().back().setRows(	Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
													Vec4f( 0.0f, 2.0f / (float)screenHeight, 0.0f, -1.0f ),
													Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
													Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );		
}

void setMatricesWindow( const ci::Vec2i& screenSize, bool originUpperLeft )
{
	setMatricesWindow( screenSize.x, screenSize.y, originUpperLeft );
}

void rotate( float angleDegrees, float xAxis, float yAxis, float zAxis )
{
	auto ctx = gl::context();
//	ctx->getModelViewStack().back().rotate( Vec3f( toRadians( xAxis ), toRadians( yAxis ), toRadians( zAxis ) ) );
	ctx->getModelViewStack().back().rotate( Vec3f( xAxis, yAxis, zAxis ), toRadians( angleDegrees ) );
}

void rotate( const cinder::Quatf &quat )
{
	cinder::Vec3f axis;
	float angle;
	quat.getAxisAngle( &axis, &angle );
	if( math<float>::abs( angle ) > EPSILON_VALUE )
		rotate( cinder::toDegrees( angle ), axis.x, axis.y, axis.z );
}
	
void scale( const ci::Vec3f& v )
{
	auto ctx = gl::context();
	ctx->getModelViewStack().back().scale( v );
}

void translate( const ci::Vec3f& v )
{
	auto ctx = gl::context();
	ctx->getModelViewStack().back().translate( v );
}
	
void begin( GLenum mode )
{
	auto ctx = gl::context();
	ctx->immediate().begin( mode );
}

void end()
{
	auto ctx = gl::context();
	
	if( ctx->immediate().empty() )
		return;
	else {
		GlslProgScope GlslProgScope( ctx->getStockShader( ShaderDef().color() ) );
		ctx->immediate().draw();
		ctx->immediate().clear();
	}
}

#if ! defined( CINDER_GLES )
void bindBufferBase( GLenum target, int index, BufferObjRef buffer )
{
	gl::context()->bindBufferBase( target, index, buffer );
}
	
void beginTransformFeedback( GLenum primitiveMode )
{
	gl::context()->beginTransformFeedback( primitiveMode );
}

void pauseTransformFeedback()
{
	gl::context()->pauseTransformFeedback();
}

void resumeTransformFeedback()
{
	gl::context()->resumeTransformFeedback();
}

void endTransformFeedback()
{
	gl::context()->endTransformFeedback();
}
#endif

void color( float r, float g, float b )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( ColorAf( r, g, b, 1.0f ) );
}

void color( float r, float g, float b, float a )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( ColorAf( r, g, b, a ) );
}

void color( const ci::Color &c )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( c );
}

void color( const ci::ColorA &c )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( c );
}

void color( const ci::Color8u &c )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( c );
}

void color( const ci::ColorA8u &c )
{
	auto ctx = gl::context();
	ctx->setCurrentColor( c );
}

void texCoord( float s, float t )
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( s, t );
}

void texCoord( float s, float t, float r )
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( s, t, r );
}

void texCoord( float s, float t, float r, float q )
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( s, t, r, q );
}

void texCoord( const ci::Vec2f &v )
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( v.x, v.y );
}

void texCoord( const ci::Vec3f &v )
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( v );
}

void texCoord( const ci::Vec4f &v )	
{
	auto ctx = gl::context();
	ctx->immediate().texCoord( v );
}

void vertex( float x, float y )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( Vec4f( x, y, 0, 1 ), ctx->getCurrentColor() );
}

void vertex( float x, float y, float z )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( Vec4f( x, y, z, 1 ), ctx->getCurrentColor() );
}

void vertex( float x, float y, float z, float w )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( Vec4f( x, y, z, w ), ctx->getCurrentColor() );
}

void vertex( const ci::Vec2f &v )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( Vec4f( v.x, v.y, 0, 1 ), ctx->getCurrentColor() );
}

void vertex( const ci::Vec3f &v )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( Vec4f( v.x, v.y, v.z, 1 ), ctx->getCurrentColor() );
}
	
void vertex( const ci::Vec4f &v )
{
	auto ctx = gl::context();
	ctx->immediate().vertex( v, ctx->getCurrentColor() );
}

#if ! defined( CINDER_GLES )
void polygonMode( GLenum face, GLenum mode )
{
	gl::context()->polygonMode( face, mode );
}

void enableWireframe()
{
	gl::polygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

void disableWireframe()
{
	gl::polygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

bool isWireframeEnabled()
{
	return gl::context()->getPolygonMode( GL_FRONT_AND_BACK ) == GL_LINE;
}

#endif

void draw( const VboMeshRef& mesh )
{
	auto ctx = gl::context();
	auto curShader = ctx->getGlslProg();
	if( ! curShader )
		return;
	
	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
	mesh->buildVao( curShader );
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	mesh->drawImpl();
	ctx->popVao();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Attributes
void vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	context()->vertexAttribPointer( index, size, type, normalized, stride, pointer );
}

#if ! defined( CINDER_GLES )
void vertexAttribIPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	context()->vertexAttribIPointer( index, size, type, stride, pointer );
}
#endif

void enableVertexAttribArray( GLuint index )
{
	context()->enableVertexAttribArray( index );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffers
void vertexAttrib1f( GLuint index, float v0 )
{
	context()->vertexAttrib1f( index, v0 );
}

void vertexAttrib2f( GLuint index, float v0, float v1 )
{
	context()->vertexAttrib2f( index, v0, v1 );
}

void vertexAttrib3f( GLuint index, float v0, float v1, float v2 )
{
	context()->vertexAttrib3f( index, v0, v1, v2 );
}

void vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 )
{
	context()->vertexAttrib4f( index, v0, v1, v2, v3 );
}

void bindBuffer( const BufferObjRef &buffer )
{
	context()->bindBuffer( buffer->getTarget(), buffer->getId() );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// toGL conversion functions
GLenum toGl( geom::Primitive prim )
{
	switch( prim ) {
		case geom::Primitive::LINES:
			return GL_LINES;
		break;
		case geom::Primitive::TRIANGLES:
			return GL_TRIANGLES;
		break;
		case geom::Primitive::TRIANGLE_STRIP:
			return GL_TRIANGLE_STRIP;
		break;
		case geom::Primitive::TRIANGLE_FAN:
			return GL_TRIANGLE_FAN;
		default:
			return 0; // no clear right choice here
	}
}

geom::Primitive toGeomPrimitive( GLenum prim )
{
	switch( prim ) {
		case GL_LINES:
			return geom::Primitive::LINES;
		break;
		case GL_TRIANGLES:
			return geom::Primitive::TRIANGLES;
		break;
		case GL_TRIANGLE_STRIP:
			return geom::Primitive::TRIANGLE_STRIP;
		break;
		case GL_TRIANGLE_FAN:
			return geom::Primitive::TRIANGLE_FAN;
		default:
			return geom::Primitive( 65535 ); // no clear right choice here
	}
}

std::string uniformSemanticToString( UniformSemantic uniformSemantic )
{
	switch( uniformSemantic ) {
		case UNIFORM_MODELVIEW: return "UNIFORM_MODELVIEW";
		case UNIFORM_MODELVIEWPROJECTION: return "UNIFORM_MODELVIEWPROJECTION";
		case UNIFORM_PROJECTION: return "UNIFORM_PROJECTION";
		case UNIFORM_NORMAL_MATRIX: return "UNIFORM_NORMAL_MATRIX";
		case UNIFORM_WINDOW_SIZE: return "UNIFORM_WINDOW_SIZE";
		case UNIFORM_ELAPSED_SECONDS: return "UNIFORM_ELAPSED_SECONDS";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw*
void drawArrays( GLenum mode, GLint first, GLsizei count )
{
	context()->drawArrays( mode, first, count );
}

void drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	context()->drawElements( mode, count, type, indices );
}

void drawCube( const Vec3f &c, const Vec3f &size )
{
	GLfloat sx = size.x * 0.5f;
	GLfloat sy = size.y * 0.5f;
	GLfloat sz = size.z * 0.5f;
	GLfloat vertices[24*3]={c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,		// +X
							c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,		// +Y
							c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,		// +Z
							c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	// -X
							c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	// -Y
							c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz};	// -Z


	static GLfloat normals[24*3]={ 1,0,0,	1,0,0,	1,0,0,	1,0,0,
								  0,1,0,	0,1,0,	0,1,0,	0,1,0,
									0,0,1,	0,0,1,	0,0,1,	0,0,1,
								  -1,0,0,	-1,0,0,	-1,0,0,	-1,0,0,
								  0,-1,0,	0,-1,0,  0,-1,0,0,-1,0,
								  0,0,-1,	0,0,-1,	0,0,-1,	0,0,-1};

	static GLubyte colors[24*4]={	255,0,0,255,	255,0,0,255,	255,0,0,255,	255,0,0,255,	// +X = red
									0,255,0,255,	0,255,0,255,	0,255,0,255,	0,255,0,255,	// +Y = green
									0,0,255,255,	0,0,255,255,	0,0,255,255,	0,0,255,255,	// +Z = blue
									0,255,255,255,	0,255,255,255,	0,255,255,255,	0,255,255,255,	// -X = cyan
									255,0,255,255,	255,0,255,255,	255,0,255,255,	255,0,255,255,	// -Y = purple
									255,255,0,255,	255,255,0,255,	255,255,0,255,	255,255,0,255 };// -Z = yellow

	static GLfloat texs[24*2]={	0,1,	1,1,	1,0,	0,0,
								1,1,	1,0,	0,0,	0,1,
								0,1,	1,1,	1,0,	0,0,							
								1,1,	1,0,	0,0,	0,1,
								1,0,	0,0,	0,1,	1,1,
								1,0,	0,0,	0,1,	1,1 };

	static GLubyte elements[6*6] ={	0, 1, 2, 0, 2, 3,
									4, 5, 6, 4, 6, 7,
									8, 9,10, 8, 10,11,
									12,13,14,12,14,15,
									16,17,18,16,18,19,
									20,21,22,20,22,23 };
	
	Context *ctx = gl::context();
	GlslProgRef curShader = ctx->getGlslProg();
	if( ! curShader )
		return;

	bool hasPositions = curShader->hasAttribSemantic( geom::Attrib::POSITION );
	bool hasNormals = curShader->hasAttribSemantic( geom::Attrib::NORMAL );
	bool hasTextureCoords = curShader->hasAttribSemantic( geom::Attrib::TEX_COORD_0 );
	bool hasColors = curShader->hasAttribSemantic( geom::Attrib::COLOR );
	
	size_t totalArrayBufferSize = 0;
	if( hasPositions )
		totalArrayBufferSize += sizeof(float)*24*3;
	if( hasNormals )
		totalArrayBufferSize += sizeof(float)*24*3;
	if( hasTextureCoords )
		totalArrayBufferSize += sizeof(float)*24*2;
	if( hasColors )
		totalArrayBufferSize += 24*4;
	
	VaoRef vao = Vao::create();
	VboRef defaultArrayVbo = ctx->getDefaultArrayVbo( totalArrayBufferSize );
	BufferScope vboScp( defaultArrayVbo );
	VboRef elementVbo = ctx->getDefaultElementVbo( 6*6 );

	VaoScope vaoScope( vao );
	elementVbo->bind();
	size_t curBufferOffset = 0;
	if( hasPositions ) {
		int loc = curShader->getAttribSemanticLocation( geom::Attrib::POSITION );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)curBufferOffset );
		defaultArrayVbo->bufferSubData( curBufferOffset, sizeof(float)*24*3, vertices );
		curBufferOffset += sizeof(float)*24*3;
	}
	if( hasNormals ) {
		int loc = curShader->getAttribSemanticLocation( geom::Attrib::NORMAL );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)curBufferOffset );
		defaultArrayVbo->bufferSubData( curBufferOffset, sizeof(float)*24*3, normals );
		curBufferOffset += sizeof(float)*24*3;
	}
	if( hasTextureCoords ) {
		int loc = curShader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)curBufferOffset );
		defaultArrayVbo->bufferSubData( curBufferOffset, sizeof(float)*24*2, texs );
		curBufferOffset += sizeof(float)*24*2;
	}
	if( hasColors ) {
		int loc = curShader->getAttribSemanticLocation( geom::Attrib::COLOR );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)curBufferOffset );
		defaultArrayVbo->bufferSubData( curBufferOffset, 24*4, colors );
		curBufferOffset += 24*4;
	}
	
	elementVbo->bufferSubData( 0, 36, elements );

	ctx->setDefaultShaderVars();
	ctx->drawElements( GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0 );
}

void draw( const TextureRef &texture, const Area &srcArea, const Rectf &dstRect )
{
	auto ctx = context();
	GlslProgRef shader = ctx->getStockShader( ShaderDef().texture( texture ).color() );
	GlslProgScope GlslProgScope( shader );
	TextureBindScope texBindScope( texture );

	shader->uniform( "uTex0", 0 );

	GLfloat data[8+8]; // both verts and texCoords
	GLfloat *verts = data, *texCoords = data + 8;
	
	Rectf texRect = texture->getAreaTexCoords( srcArea );

	verts[0*2+0] = dstRect.getX2(); texCoords[0*2+0] = texRect.x2;
	verts[0*2+1] = dstRect.getY1(); texCoords[0*2+1] = texRect.y1;
	verts[1*2+0] = dstRect.getX1(); texCoords[1*2+0] = texRect.x1;
	verts[1*2+1] = dstRect.getY1(); texCoords[1*2+1] = texRect.y1;
	verts[2*2+0] = dstRect.getX2(); texCoords[2*2+0] = texRect.x2;
	verts[2*2+1] = dstRect.getY2(); texCoords[2*2+1] = texRect.y2;
	verts[3*2+0] = dstRect.getX1(); texCoords[3*2+0] = texRect.x1;
	verts[3*2+1] = dstRect.getY2(); texCoords[3*2+1] = texRect.y2;
	
	VboRef defaultVbo = ctx->getDefaultArrayVbo( sizeof(float)*16 );
	BufferScope vboScp( defaultVbo );
	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
		defaultVbo->bufferSubData( 0, sizeof(float)*16, data );
		int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
		if( posLoc >= 0 ) {
			enableVertexAttribArray( posLoc );
			vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
		}
		int texLoc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
		if( texLoc >= 0 ) {
			enableVertexAttribArray( texLoc );
			vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*8) );
		}
	ctx->getDefaultVao()->replacementBindEnd();
	
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	ctx->popVao();
}

void draw( const TextureRef &texture, const Rectf &dstRect )
{
	draw( texture, texture->getBounds(), dstRect );
}


void draw( const TextureRef &texture, const Vec2f &dstOffset )
{
	if( ! texture )
		return;
	draw( texture, texture->getBounds(), Rectf( texture->getBounds() ) + dstOffset );
}

void drawSolidRect( const Rectf& r )
{
	drawSolidRect( r, Rectf( 0, 0, 1, 1 ) );
}

void drawSolidRect( const Rectf &r, const Rectf &texcoords )
{
	auto ctx = context();
	GLfloat data[8+8]; // both verts and texCoords
	GLfloat *verts = data, *texCoords = data + 8;
	
	verts[0*2+0] = r.getX2(); texCoords[0*2+0] = texcoords.getX2();
	verts[0*2+1] = r.getY1(); texCoords[0*2+1] = texcoords.getY1();
	verts[1*2+0] = r.getX1(); texCoords[1*2+0] = texcoords.getX1();
	verts[1*2+1] = r.getY1(); texCoords[1*2+1] = texcoords.getY1();
	verts[2*2+0] = r.getX2(); texCoords[2*2+0] = texcoords.getX2();
	verts[2*2+1] = r.getY2(); texCoords[2*2+1] = texcoords.getY2();
	verts[3*2+0] = r.getX1(); texCoords[3*2+0] = texcoords.getX1();
	verts[3*2+1] = r.getY2(); texCoords[3*2+1] = texcoords.getY2();
	
	VaoRef vao = Vao::create();
	VaoScope vaoScope( vao );
	VboRef defaultVbo = ctx->getDefaultArrayVbo( sizeof(float)*16 );
	BufferScope bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, sizeof(float)*16, data );
	
	gl::GlslProgRef shader = ctx->getGlslProg();
	int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}
	int texLoc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
	if( texLoc >= 0 ) {
		enableVertexAttribArray( texLoc );
		vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*8) );
	}
	
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void drawStrokedRect( const Rectf &rect )
{
	GLfloat verts[8];
	verts[0] = rect.x1;	verts[1] = rect.y1;
	verts[2] = rect.x2;	verts[3] = rect.y1;
	verts[4] = rect.x2;	verts[5] = rect.y2;
	verts[6] = rect.x1;	verts[7] = rect.y2;

	auto ctx = context();
	VaoScope vaoScope( Vao::create() );
	VboRef defaultVbo = ctx->getDefaultArrayVbo( 8 * sizeof( float ) );
	BufferScope bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, 8 * sizeof( float ), verts );

	gl::GlslProgRef shader = ctx->getGlslProg();
	int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}

	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_LINE_LOOP, 0, 4 );
}

void drawStrokedRect( const Rectf &rect, float lineWidth )
{
	const float halfWidth = lineWidth / 2;
	GLfloat verts[32];
	verts[0] = rect.x1 + halfWidth;		verts[1] = rect.y2 - halfWidth;		// left bar
	verts[2] = rect.x1 + halfWidth;		verts[3] = rect.y1 + halfWidth;
	verts[4] = rect.x1 - halfWidth;		verts[5] = rect.y2 + halfWidth;
	verts[6] = rect.x1 - halfWidth;		verts[7] = rect.y1 - halfWidth;
	verts[8] = rect.x1 + halfWidth;		verts[9] = rect.y1 + halfWidth;
	verts[10] = rect.x2 - halfWidth;	verts[11] = rect.y1 + halfWidth;	// upper bar
	verts[12] = rect.x1 - halfWidth;	verts[13] = rect.y1 - halfWidth;
	verts[14] = rect.x2 + halfWidth;	verts[15] = rect.y1 - halfWidth;
	verts[16] = rect.x2 - halfWidth;	verts[17] = rect.y1 + halfWidth;	// right bar
	verts[18] = rect.x2 - halfWidth;	verts[19] = rect.y2 - halfWidth;
	verts[20] = rect.x2 + halfWidth;	verts[21] = rect.y1 - halfWidth;
	verts[22] = rect.x2 + halfWidth;	verts[23] = rect.y2 + halfWidth;
	verts[24] = rect.x2 - halfWidth;	verts[25] = rect.y2 - halfWidth;	// bottom bar
	verts[26] = rect.x1 + halfWidth;	verts[27] = rect.y2 - halfWidth;
	verts[28] = rect.x2 + halfWidth;	verts[29] = rect.y2 + halfWidth;
	verts[30] = rect.x1 - halfWidth;	verts[31] = rect.y2 + halfWidth;

	auto ctx = context();
	VaoScope vaoScope( Vao::create() );
	VboRef defaultVbo = ctx->getDefaultArrayVbo( 32 * sizeof( float ) );
	BufferScope bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, 32 * sizeof( float ), verts );

	gl::GlslProgRef shader = ctx->getGlslProg();
	int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}

	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 16 );
}

void drawSolidCircle( const Vec2f &center, float radius, int numSegments )
{
	auto ctx = context();
	gl::GlslProgRef shader = ctx->getGlslProg();
	if( ! shader )
		return;

	VaoRef vao = Vao::create();
	VaoScope vaoScope( vao );

	if( numSegments <= 0 ) {
		numSegments = (int)math<double>::floor( radius * M_PI * 2 );
	}
	if( numSegments < 3 ) numSegments = 3;
	size_t numVertices = numSegments + 2;

	size_t worstCaseSize = numVertices * sizeof(float) * ( 2 + 2 + 3 );
	VboRef defaultVbo = ctx->getDefaultArrayVbo( worstCaseSize );
	BufferScope vboScp( defaultVbo );

	size_t dataSizeBytes = 0;
	
	size_t vertsOffset, texCoordsOffset, normalsOffset;
	int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)dataSizeBytes );
		vertsOffset = dataSizeBytes;
		dataSizeBytes += numVertices * 2 * sizeof(float);
	}
	int texLoc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
	if( texLoc >= 0 ) {
		enableVertexAttribArray( texLoc );
		vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)dataSizeBytes );
		texCoordsOffset = dataSizeBytes;
		dataSizeBytes += numVertices * 2 * sizeof(float);
	}
	int normalLoc = shader->getAttribSemanticLocation( geom::Attrib::NORMAL );
	if( normalLoc >= 0 ) {
		enableVertexAttribArray( normalLoc );
		vertexAttribPointer( normalLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(dataSizeBytes) );
		normalsOffset = dataSizeBytes;
		dataSizeBytes += numVertices * 3 * sizeof(float);
	}

	unique_ptr<uint8_t[]> data( new uint8_t[dataSizeBytes] );
	Vec2f *verts = ( posLoc >= 0 ) ? reinterpret_cast<Vec2f*>( data.get() + vertsOffset ) : nullptr;
	Vec2f *texCoords = ( texLoc >= 0 ) ? reinterpret_cast<Vec2f*>( data.get() + texCoordsOffset ) : nullptr;
	Vec3f *normals = ( normalLoc >= 0 ) ? reinterpret_cast<Vec3f*>( data.get() + normalsOffset ) : nullptr;
	
	if( verts )
		verts[0] = center;
	if( texCoords )
		texCoords[0] = Vec2f( 0.5f, 0.5f );
	if( normals )
		normals[0] = Vec3f::zAxis();
	const float tDelta = 1.0f / numSegments * 2 * (float)M_PI;
	float t = 0;
	for( int s = 0; s <= numSegments; s++ ) {
		const Vec2f unit( math<float>::cos( t ), math<float>::sin( t ) );
		if( verts )
			verts[s+1] = center + unit * radius;
		if( texCoords )
			texCoords[s+1] = unit * 0.5f + Vec2f( 0.5f, 0.5f );
		if( normals )
			normals[s+1] = Vec3f::zAxis();
		t += tDelta;
	}

	defaultVbo->bufferSubData( 0, dataSizeBytes, data.get() );
	
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLE_FAN, 0, numSegments + 2 );	
}

void drawSphere( const Vec3f &center, float radius, int segments )
{
	auto ctx = gl::context();
	auto glslProg = ctx->getGlslProg();
	if( ! glslProg )
		return;
	//auto batch = gl::Batch::create( geom::Sphere().center( center ).radius( radius ).segments( segments ).normals().texCoords(), glslProg );
	//batch->draw();
	
	
	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
	gl::VboMeshRef mesh = gl::VboMesh::create( geom::Sphere().center( center ).radius( radius ).segments( segments ).enable( geom::Attrib::NORMAL ).enable( geom::Attrib::TEX_COORD_0 ), ctx->getDefaultArrayVbo(), ctx->getDefaultElementVbo() );
	mesh->buildVao( glslProg );
	ctx->getDefaultVao()->replacementBindEnd();	
	ctx->setDefaultShaderVars();
	mesh->drawImpl();
	ctx->popVao();
}

void drawBillboard( const Vec3f &pos, const Vec2f &scale, float rotationRadians, const Vec3f &bbRight, const Vec3f &bbUp, const Rectf &texCoords )
{
	auto ctx = context();
	gl::GlslProgRef glslProg = ctx->getGlslProg();
	if( ! glslProg )
		return;

	GLfloat data[12+8]; // both verts and texCoords
	Vec3f *verts = (Vec3f*)data;
	float *texCoordsOut = data + 12;

	float sinA = math<float>::sin( rotationRadians );
	float cosA = math<float>::cos( rotationRadians );
	
	verts[0] = pos + bbRight * ( -0.5f * scale.x * cosA - 0.5f * sinA * scale.y ) + bbUp * ( -0.5f * scale.x * sinA + 0.5f * cosA * scale.y );
	texCoordsOut[0*2+0] = texCoords.getX1(); texCoordsOut[0*2+1] = texCoords.getY1();
	verts[1] = pos + bbRight * ( -0.5f * scale.x * cosA - -0.5f * sinA * scale.y ) + bbUp * ( -0.5f * scale.x * sinA + -0.5f * cosA * scale.y );
	texCoordsOut[1*2+0] = texCoords.getX1(); texCoordsOut[1*2+1] = texCoords.getY2();
	verts[2] = pos + bbRight * ( 0.5f * scale.x * cosA - 0.5f * sinA * scale.y ) + bbUp * ( 0.5f * scale.x * sinA + 0.5f * cosA * scale.y );
	texCoordsOut[2*2+0] = texCoords.getX2(); texCoordsOut[2*2+1] = texCoords.getY1();
	verts[3] = pos + bbRight * ( 0.5f * scale.x * cosA - -0.5f * sinA * scale.y ) + bbUp * ( 0.5f * scale.x * sinA + -0.5f * cosA * scale.y );
	texCoordsOut[3*2+0] = texCoords.getX2(); texCoordsOut[3*2+1] = texCoords.getY2();
	
	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
	VboRef defaultVbo = ctx->getDefaultArrayVbo( sizeof(float)*20 );
	BufferScope bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, sizeof(float)*20, data );
	
	int posLoc = glslProg->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}
	int texLoc = glslProg->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
	if( texLoc >= 0 ) {
		enableVertexAttribArray( texLoc );
		vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*12) );
	}
	
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	ctx->popVao();
}

GLenum getError()
{
	return glGetError();
}

std::string getErrorString( GLenum err )
{
	switch( err ) {
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		default:
			return "";
	}
}

void checkError()
{
	GLenum errorFlag = getError();
	if( errorFlag != GL_NO_ERROR ) {
		CI_LOG_E( "glGetError flag set: " << getErrorString( errorFlag ) );
		CI_ASSERT( 0 );
	}
}

} } // namespace cinder::gl
