#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include <vector>

using namespace std;

namespace cinder { namespace gl {

Context* context()
{
	static Context *sContext = NULL;
	if( ! sContext ) {
		sContext = new Context();
	}
	return sContext;
}

bool isExtensionAvailable( const std::string& extName )
{
	static const char *sExtStr = reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) );
	static std::map<std::string, bool> sExtMap;
	
	std::map<std::string,bool>::const_iterator extIt = sExtMap.find( extName );
	if ( extIt == sExtMap.end() ) {
		bool found		= false;
		int extNameLen	= extName.size();
		const char *p	= sExtStr;
		const char *end = sExtStr + strlen( sExtStr );
		while ( p < end ) {
			int n = strcspn( p, " " );
			if ( (extNameLen == n ) && ( strncmp( extName.c_str(), p, n) == 0 ) ) {
				found = true;
				break;
			}
			p += (n + 1);
		}
		sExtMap[ extName ] = found;
		return found;
	} else {
		return extIt->second;
	}
}

void clear( const ColorA& color, bool clearDepthBuffer )
{
	glClearColor( color.r, color.g, color.b, color.a );
	if ( clearDepthBuffer ) {
		glDepthMask( GL_TRUE );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	} else {
		glClear( GL_COLOR_BUFFER_BIT );
	}
}

Area getViewport()
{
	GLint params[ 4 ];
	glGetIntegerv( GL_VIEWPORT, params );
	Area result;
	return Area( params[ 0 ], params[ 1 ], params[ 0 ] + params[ 2 ], params[ 1 ] + params[ 3 ] );
}

void setViewport( const Area& area )
{
	glViewport( area.x1, area.y1, ( area.x2 - area.x1 ), ( area.y2 - area.y1 ) );
}

void enable( GLenum state, bool enable )
{
	context()->enable( state, enable );
}

void enableAlphaBlending( bool premultiplied )
{
	gl::enable( GL_BLEND );
	if( ! premultiplied ) {
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	}
}

void disableAlphaBlending()
{
	gl::disable( GL_BLEND );
}

void enableAdditiveBlending()
{
	gl::enable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
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
	glDepthMask( enable ? GL_TRUE : GL_FALSE );
}

void disableDepthWrite()
{
	glDepthMask( GL_FALSE );
}

void enableLighting( bool enable )
{
	auto ctx	= gl::context();
	ctx->mLighting	= enable;
}

void disableLighting()
{
	auto ctx	= gl::context();
	ctx->mLighting	= false;
}
	
void enableWireframe( bool enable )
{
	auto ctx	= gl::context();
	ctx->mWireframe = enable;
}

void disableWireframe()
{
	auto ctx	= gl::context();
	ctx->mWireframe = false;
}
	
void setMatrices( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() = cam.getModelViewMatrix();
	ctx->mProjection.back() = cam.getProjectionMatrix();
}

void setModelView( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() = cam.getModelViewMatrix();
}

void setProjection( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mProjection.back() = cam.getProjectionMatrix();
}

void pushModelView()
{
	auto ctx	= gl::context();
	ctx->mModelView.push_back( ctx->mModelView.back() );
}

void popModelView()
{
	auto ctx	= gl::context();
	ctx->mModelView.pop_back();
}

void pushProjection( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mProjection.push_back( cam.getProjectionMatrix().m );
}

void pushMatrices()
{
	auto ctx		= gl::context();
	Matrix44f modelView		= ctx->mModelView.back();
	Matrix44f projection	= ctx->mProjection.back();
	ctx->mModelView.push_back( modelView );
	ctx->mProjection.push_back( projection );
}

void popMatrices()
{
	auto ctx	= gl::context();
	if ( ctx->mModelView.size() > 1 && ctx->mProjection.size() > 1 ) {
		ctx->mModelView.pop_back();
		ctx->mProjection.pop_back();
	}
}

void multModelView( const ci::Matrix44f& mtx )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() *= mtx;
}

void multProjection( const ci::Matrix44f& mtx )
{
	auto ctx	= gl::context();
	ctx->mProjection.back() *= mtx;
}

Matrix44f getModelView()
{
	auto ctx	= gl::context();
	return ctx->mModelView.back();
}

Matrix44f getProjection()
{
	auto ctx	= gl::context();
	return ctx->mProjection.back();
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
	// TODO add perspective
	// TODO enable origin
	auto ctx	= gl::context();
	ctx->mModelView.back().setToIdentity();
	ctx->mProjection.back().setRows( Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
										Vec4f( 0.0f, 2.0f / -(float)screenHeight, 0.0f, 1.0f ),
										Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
										Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
}

void setMatricesWindowPersp( const ci::Vec2i& screenSize, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	setMatricesWindowPersp( screenSize.x, screenSize.y, fovDegrees, nearPlane, farPlane, originUpperLeft );
}

void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft )
{
	// TODO enable origin
	auto ctx	= gl::context();
	ctx->mModelView.back().setToIdentity();
	ctx->mProjection.back().setRows( Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
										Vec4f( 0.0f, 2.0f / -(float)screenHeight, 0.0f, 1.0f ),
										Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
										Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
}

void setMatricesWindow( const ci::Vec2i& screenSize, bool originUpperLeft )
{
	setMatricesWindow( screenSize.x, screenSize.y, originUpperLeft );
}

void rotate( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().rotate( Vec3f( toRadians( v.x ), toRadians( v.y ), toRadians( v.z ) ) );
}
	
void scale( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().scale( v );
}

void translate( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().translate( v );
}
	
void begin( GLenum mode )
{
	auto ctx	= gl::context();
	ctx->mMode		= mode;
	ctx->clear();
}

void end()
{
	auto ctx = gl::context();
	ctx->draw();
}

void color( float r, float g, float b )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( r, g, b, 1.0f );
}

void color( float r, float g, float b, float a )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( r, g, b, a );
}

void color( const ci::Color& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::ColorA& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::Color8u& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::ColorA8u& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void normal( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mNormal	= v;
}

void texCoord( float s )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, 0.0f, 0.0f, 0.0f );
}

void texCoord( float s, float t )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, 0.0f, 0.0f );
}

void texCoord( float s, float t, float r )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, r, 0.0f );
}

void texCoord( float s, float t, float r, float q )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, r, q );
}

void texCoord( const ci::Vec2f& v )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( v.x, v.y, 0.0f, 0.0f );
}

void texCoord( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( v.x, v.y, v.z, 0.0f );
}

void texCoord( const ci::Vec4f& v )	
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= v;
}

void vertex( float x, float y )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, 0.0f, 0.0f ) );
}

void vertex( float x, float y, float z )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, z, 0.0f ) );
}

void vertex( float x, float y, float z, float w )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, z, w ) );
}

void vertex( const ci::Vec2f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( v.x, v.y, 0.0f, 0.0f ) );
}

void vertex( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( v.x, v.y, v.z, 0.0f ) );
}
	
void vertex( const ci::Vec4f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( v );
}

void draw( const VboRef& vbo )
{
	drawRange( vbo );
}

void drawRange( const VboRef& vbo, GLint start, GLsizei count )
{
	auto ctx	= gl::context();
	GLenum mode			= ctx->mMode;
	if ( ctx->mWireframe && mode != GL_POINTS ) {
		mode = GL_LINE_STRIP;
	}
	
	vbo->bind();
	if ( vbo->getUsage() == GL_STATIC_DRAW ) {
		if ( vbo->getTarget() == GL_ELEMENT_ARRAY_BUFFER && vbo->getSize() >= sizeof( GLuint ) ) {
			if ( count == 0 ) {
				count = vbo->getSize() / sizeof( GLuint ) - start;
			}
			glDrawElements( mode, count, GL_UNSIGNED_INT, (const GLvoid*)start );
		}
	} else {
		glDrawArrays( mode, start, count );
	}
	vbo->unbind();
}

void draw( const VboMeshRef& mesh )
{
	drawRange( mesh );
}

void drawRange( const VboMeshRef& mesh, GLint start, GLsizei count )
{
	auto vaoBind( mesh->mVao );
	
	if ( mesh->mVboIndices ) {
		if ( mesh->mVboVerticesDynamic ) {
			mesh->mVboVerticesDynamic->bind();
		}
		if ( mesh->mVboVerticesStatic ) {
			mesh->mVboVerticesStatic->bind();
		}
		drawRange( mesh->mVboIndices, start, count );
		if ( mesh->mVboVerticesDynamic ) {
			mesh->mVboVerticesDynamic->unbind();
		}
		if ( mesh->mVboVerticesStatic ) {
			mesh->mVboVerticesStatic->unbind();
		}
	} else {
		if ( mesh->mVboVerticesDynamic ) {
			drawRange( mesh->mVboVerticesDynamic, start, count );
		}
		if ( mesh->mVboVerticesStatic ) {
			drawRange( mesh->mVboVerticesStatic, start, count );
		}
	}
	
	mesh->mVao->unbind();
}

void drawArrays( GLenum mode, GLint first, GLsizei count )
{
	context()->prepareDraw();
	glDrawArrays( mode, first, count );
}

GLenum getError()
{
	return glGetError();
}

std::string getErrorString( GLenum err )
{
	switch( err ) {
		case GL_NO_ERROR:
			return "GL_NO_ERROR";;
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
	
SaveTextureBindState::SaveTextureBindState( GLint target )
: mTarget( target ), mOldID( -1 )
{
	switch( target ) {
		case GL_TEXTURE_2D:
			glGetIntegerv( GL_TEXTURE_BINDING_2D, &mOldID );
			break;
		default:
			throw gl::ExceptionUnknownTarget();
	}
}

SaveTextureBindState::~SaveTextureBindState()
{
	glBindTexture( mTarget, mOldID );
}

BoolState::BoolState( GLint target )
: mTarget( target )
{
	glGetBooleanv( target, &mOldValue );
}

BoolState::~BoolState()
{
	if ( mOldValue ) {
		gl::enable( mTarget );
	} else {
		gl::disable( mTarget );
	}
}

ClientBoolState::ClientBoolState( GLint target )
: mTarget( target )
{
	mOldValue = glIsEnabled( target );
}

SaveFramebufferBinding::SaveFramebufferBinding()
{
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &mOldValue );
}

SaveFramebufferBinding::~SaveFramebufferBinding()
{
	glBindFramebuffer( GL_FRAMEBUFFER, mOldValue );
}

} }
