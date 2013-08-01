#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/fog.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Utilities.h"

#if defined( CINDER_MAC )
	#include <OpenGL/OpenGL.h>
#elif defined( CINDER_COCOA_TOUCH )
	#import <OpenGLES/EAGL.h>
#endif

#include "cinder/app/App.h"

using namespace std;

// ES 2 Multisampling is available on iOS via an extension
#if ! defined( CINDER_GLES ) || ( defined( CINDER_COCOA_TOUCH ) )
	#define SUPPORTS_FBO_MULTISAMPLING
	#if defined( CINDER_COCOA_TOUCH )
		#define GL_READ_FRAMEBUFFER					GL_READ_FRAMEBUFFER_APPLE
		#define GL_DRAW_FRAMEBUFFER					GL_DRAW_FRAMEBUFFER_APPLE
		#define GL_READ_FRAMEBUFFER_BINDING			GL_READ_FRAMEBUFFER_BINDING_APPLE
		#define GL_DRAW_FRAMEBUFFER_BINDING			GL_DRAW_FRAMEBUFFER_BINDING_APPLE
	#endif
#endif

namespace cinder { namespace gl {

#if defined( CINDER_COCOA )
static pthread_key_t sThreadSpecificCurrentContextKey;
static bool sThreadSpecificCurrentContextInitialized = false;
#elif defined( _MSC_VER )
__declspec(thread) Context *sThreadSpecificCurrentContext = NULL;
#else
thread_local Context *sThreadSpecificCurrentContext = NULL;
#endif

Context::Context( void *platformContext, void *platformContextAdditional )
	: mPlatformContext( platformContext ), mPlatformContextAdditional( platformContextAdditional ),
	mColor( ColorAf::white() ), mFogEnabled( false ), mLighting( false ), mMaterialEnabled( false ),
	mMode( GL_TRIANGLES ), mNormal( Vec3f( 0.0f, 0.0f, 1.0f ) ), mTexCoord( Vec4f::zero() ),
	mCachedActiveTexture( 0 ), mWireframe( false )
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	,mCachedFramebuffer( -1 )
#else
	,mCachedReadFramebuffer( -1 ), mCachedDrawFramebuffer( -1 )
#endif
#if ! defined( CINDER_GLES )
	,mCachedFrontPolygonMode( GL_FILL ), mCachedBackPolygonMode( GL_FILL )
#endif
{

	// setup default VAO
#if ! defined( CINDER_GLES )
	mCachedVao = mDefaultVao = Vao::create();
	mDefaultVao->bindImpl( NULL );
#endif

	clear();
	mModelView.push_back( Matrix44f() );
	mModelView.back().setToIdentity();
	mProjection.push_back( Matrix44f() );
	mProjection.back().setToIdentity();
}

Context::~Context()
{
	clear();
}

ContextRef Context::create( const Context *sharedContext )
{
	void *platformContext = NULL;
	void *platformContextAdditional = NULL;
#if defined( CINDER_MAC )
	CGLContextObj sharedContextCgl = (CGLContextObj)sharedContext->getPlatformContext();
	CGLPixelFormatObj sharedContextPixelFormat = ::CGLGetPixelFormat( sharedContextCgl );
	if( ::CGLCreateContext( sharedContextPixelFormat, sharedContextCgl, (CGLContextObj*)&platformContext ) != kCGLNoError ) {
		throw ExcContextAllocation();
	}

	::CGLSetCurrentContext( (CGLContextObj)platformContext );
#elif defined( CINDER_COCOA_TOUCH )
	EAGLContext *sharedContextEagl = (EAGLContext*)sharedContext->getPlatformContext();
	EAGLSharegroup *sharegroup = sharedContextEagl.sharegroup;
	platformContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
	[EAGLContext setCurrentContext:(EAGLContext*)platformContext];
#elif defined( CINDER_MSW )
	// save the current context so we can restore it
	HGLRC prevContext = ::wglGetCurrentContext();
	HDC prevDc = ::wglGetCurrentDC();
	HGLRC sharedContextWgl = (HGLRC)sharedContext->getPlatformContext();
	platformContextAdditional = sharedContext->mPlatformContextAdditional;
	platformContext = ::wglCreateContext( (HDC)platformContextAdditional );
	::wglMakeCurrent( NULL, NULL );
	if( ! ::wglShareLists( sharedContextWgl, (HGLRC)platformContext ) ) {
		DWORD error = GetLastError();
		error = error + 0;
	}
	::wglMakeCurrent( (HDC)platformContextAdditional, (HGLRC)platformContext );
#endif

	ContextRef result( std::shared_ptr<Context>( new Context( platformContext, platformContextAdditional ) ) );
	env()->initializeFunctionPointers();
	env()->initializeContextDefaults( result.get() );

#if defined( CINDER_MSW )
	::wglMakeCurrent( prevDc, prevContext );
#endif

	return result;
}

ContextRef Context::createFromExisting( void *platformContext, void *platformContextAdditional )
{
	env()->initializeFunctionPointers();
	ContextRef result( std::shared_ptr<Context>( new Context( platformContext, platformContextAdditional ) ) );
	env()->initializeContextDefaults( result.get() );

	return result;
}

void Context::makeCurrent() const
{
#if defined( CINDER_MAC )
	::CGLSetCurrentContext( (CGLContextObj)mPlatformContext );
#elif defined( CINDER_COCOA_TOUCH )
	[EAGLContext setCurrentContext:(EAGLContext*)mPlatformContext];
#elif defined( CINDER_MSW )
	if( ! ::wglMakeCurrent( (HDC)mPlatformContextAdditional, (HGLRC)mPlatformContext ) ) {
		DWORD error = GetLastError();
		error = error + 0;
	}
#endif

#if defined( CINDER_COCOA )
	if( ! sThreadSpecificCurrentContextInitialized ) {
		pthread_key_create( &sThreadSpecificCurrentContextKey, NULL );
		sThreadSpecificCurrentContextInitialized = true;
	}
	pthread_setspecific( sThreadSpecificCurrentContextKey, this );
#else
	sThreadSpecificCurrentContext = const_cast<Context*>( this );
#endif
}

Context* Context::getCurrent()
{
#if defined( CINDER_COCOA )
	if( ! sThreadSpecificCurrentContextInitialized ) {
		return NULL;
	}
	return reinterpret_cast<Context*>( pthread_getspecific( sThreadSpecificCurrentContextKey ) );
#else
	return sThreadSpecificCurrentContext;
#endif
}

//////////////////////////////////////////////////////////////////
// VAO
void Context::vaoBind( const VaoRef &vao )
{
	if( mCachedVao != vao ) {
		if( mCachedVao )
			mCachedVao->unbindImpl( this );
		if( vao ) {
			vao->bindImpl( this );
		}
		
		mCachedVao = vao;
	}
}

VaoRef Context::vaoGet()
{
	return mCachedVao;
}

//////////////////////////////////////////////////////////////////
// Buffer
void Context::bindBuffer( GLenum target, GLuint id )
{
	if( mCachedBuffer[target] != id ) {
		mCachedBuffer[target] = id;
		glBindBuffer( target, id );
		if( target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER ) {
			VaoRef vao = vaoGet();
			if( vao ) {
				vao->reflectBindBuffer( target, id );
			}
		}
	}
}

GLuint Context::getBufferBinding( GLenum target )
{
	auto cachedIt = mCachedBuffer.find( target );
	if( (cachedIt == mCachedBuffer.end()) || ( cachedIt->second == -1 ) ) {
		GLint queriedInt = -1;
		switch( target ) {
			case GL_ARRAY_BUFFER:
				glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &queriedInt );
			break;
			case GL_ELEMENT_ARRAY_BUFFER:
				glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
			break;
			default:
				; // warning?
		}
		
		mCachedBuffer[target] = queriedInt;
		return (GLuint)queriedInt;
	}
	else
		return (GLuint)cachedIt->second;
}

void Context::invalidateBufferBinding( GLenum target )
{
	mCachedBuffer[target] = -1;
}

//////////////////////////////////////////////////////////////////
// Shader
void Context::bindShader( const GlslProgRef &prog )
{
	if( mCachedGlslProg != prog ) {
		mCachedGlslProg = prog;
		if( prog )
			glUseProgram( prog->getHandle() );
		else
			glUseProgram( 0 );
	}
}

void Context::unbindShader()
{
	mCachedGlslProg = GlslProgRef();
	glUseProgram( 0 );
}

GlslProgRef Context::getCurrentShader()
{
	return mCachedGlslProg;
}

//////////////////////////////////////////////////////////////////
// ActiveTexture
void Context::activeTexture( GLenum textureUnit )
{
	if( mCachedActiveTexture != textureUnit ) {
		mCachedActiveTexture = textureUnit;
		glActiveTexture( textureUnit );
	}
}

GLenum Context::getActiveTexture()
{
	return mCachedActiveTexture;
}


//////////////////////////////////////////////////////////////////
// Framebuffers
void Context::bindFramebuffer( GLenum target, GLuint framebuffer )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	if( target == GL_FRAMEBUFFER ) {
		if( framebuffer != mCachedFramebuffer ) {
			mCachedFramebuffer = framebuffer;
			glBindFramebuffer( target, framebuffer );
		}
	}
	else {
		//throw gl::Exception( "Illegal target for Context::bindFramebuffer" );	
	}
#else
	if( target == GL_FRAMEBUFFER ) {
		if( framebuffer != mCachedReadFramebuffer || framebuffer != mCachedDrawFramebuffer ) {
			mCachedReadFramebuffer = mCachedDrawFramebuffer = framebuffer;
			glBindFramebuffer( target, framebuffer );
		}
	}
	else if( target == GL_READ_FRAMEBUFFER ) {
		if( framebuffer != mCachedReadFramebuffer ) {
			mCachedReadFramebuffer = framebuffer;
			glBindFramebuffer( target, framebuffer );
		}
	}
	else if( target == GL_DRAW_FRAMEBUFFER ) {
		if( framebuffer != mCachedDrawFramebuffer ) {
			mCachedDrawFramebuffer = framebuffer;
			glBindFramebuffer( target, framebuffer );
		}
	}
	else {
		//throw gl::Exception( "Illegal target for Context::bindFramebuffer" );	
	}
#endif
}

void Context::bindFramebuffer( const FboRef &fbo )
{
	bindFramebuffer( GL_FRAMEBUFFER, fbo->getId() );
}

void Context::unbindFramebuffer()
{
	bindFramebuffer( GL_FRAMEBUFFER, 0 );
}

GLuint Context::getFramebufferBinding( GLenum target )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	if( target == GL_FRAMEBUFFER ) {
		if( mCachedFramebuffer == -1 )
			glGetIntegerv( GL_FRAMEBUFFER_BINDING, &mCachedFramebuffer );
		return (GLuint)mCachedFramebuffer;			
	}
	else {
		//throw gl::Exception( "Illegal target for getFramebufferBinding" );
		return 0; // 	
	}
#else
	if( target == GL_READ_FRAMEBUFFER ) {
		if( mCachedReadFramebuffer == -1 )
			glGetIntegerv( GL_READ_FRAMEBUFFER_BINDING, &mCachedReadFramebuffer );
		return (GLuint)mCachedReadFramebuffer;
	}
	else if( target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER ) {
		if( mCachedDrawFramebuffer == -1 )
			glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &mCachedDrawFramebuffer );
		return (GLuint)mCachedDrawFramebuffer;
	}
	else {
		//throw gl::Exception( "Illegal target for getFramebufferBinding" );
		return 0; // 
	}
#endif
}

//////////////////////////////////////////////////////////////////
// States
template<>
void Context::stateSet<GLboolean>( GLenum cap, GLboolean value )
{
	auto cached = mCachedStateBoolean.find( cap );
	if( cached == mCachedStateBoolean.end() || cached->second != value ) {
		mCachedStateBoolean[cap] = value;
		if( value )
			glEnable( cap );
		else
			glDisable( cap );
	}	
}

void Context::enable( GLenum cap, GLboolean value )
{
	stateSet<GLboolean>( cap, value );
}

template<>
GLboolean Context::stateGet<GLboolean>( GLenum cap )
{
	auto cached = mCachedStateBoolean.find( cap );
	if( cached == mCachedStateBoolean.end() ) {
		GLboolean result = glIsEnabled( cap );
		mCachedStateBoolean[cap] = result;
		return result;
	}
	else
		return cached->second;
}

template<>
GLint Context::stateGet<GLint>( GLenum pname )
{
	auto cached = mCachedStateInt.find( pname );
	if( cached == mCachedStateInt.end() ) {
		GLint result;
		glGetIntegerv( pname, &result );
		mCachedStateInt[pname] = result;
		return result;
	}
	else
		return cached->second;
}

/////////////////////////////////////////////////////////////////////////////////////
void Context::sanityCheck()
{
return;
	GLint queriedInt;

	// GL_ARRAY_BUFFER
	glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &queriedInt );
	assert( mCachedBuffer[GL_ARRAY_BUFFER] == queriedInt );

	// GL_ELEMENT_ARRAY_BUFFER
	glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
	assert( mCachedBuffer[GL_ELEMENT_ARRAY_BUFFER] == queriedInt );

	// (VAO) GL_VERTEX_ARRAY_BINDING
#if defined( CINDER_GLES )
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING_OES, &queriedInt );
#else
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &queriedInt );
#endif
//	assert( mCachedVao == queriedInt );
}

void Context::printState( std::ostream &os ) const
{
	GLint queriedInt;
	
	glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &queriedInt );	
	os << "{ARRAY_BUFFER:" << queriedInt << ", ";

	glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
	os << "GL_ELEMENT_ARRAY_BUFFER:" << queriedInt << ", ";

#if defined( CINDER_GLES )
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING_OES, &queriedInt );
#else
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &queriedInt );
#endif
	os << "GL_VERTEX_ARRAY_BINDING:" << queriedInt << "}" << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Attributes
void Context::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	VaoRef vao = vaoGet();
	if( vao )
		vao->vertexAttribPointerImpl( index, size, type, normalized, stride, pointer );
}

void Context::enableVertexAttribArray( GLuint index )
{
	VaoRef vao = vaoGet();
	if( vao )
		vao->enableVertexAttribArrayImpl( index );
}

void Context::vertexAttrib1f( GLuint index, float v0 )
{
	glVertexAttrib1f( index, v0 );
}

void Context::vertexAttrib2f( GLuint index, float v0, float v1 )
{
	glVertexAttrib2f( index, v0, v1 );
}

void Context::vertexAttrib3f( GLuint index, float v0, float v1, float v2 )
{
	glVertexAttrib3f( index, v0, v1, v2 );
}

void Context::vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 )
{
	glVertexAttrib4f( index, v0, v1, v2, v3 );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Context::blendFunc( GLenum sfactor, GLenum dfactor )
{
	blendFuncSeparate( sfactor, dfactor, sfactor, dfactor );
}

void Context::blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	if( ( mCachedStateInt[GL_BLEND_SRC_RGB] != srcRGB ) ||
		( mCachedStateInt[GL_BLEND_DST_RGB] != dstRGB ) ||
		( mCachedStateInt[GL_BLEND_SRC_ALPHA] != srcAlpha ) ||
		( mCachedStateInt[GL_BLEND_DST_ALPHA] != dstAlpha ) )
	{
		glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
		mCachedStateInt[GL_BLEND_SRC_RGB] = srcRGB;
		mCachedStateInt[GL_BLEND_DST_RGB] = dstRGB;
		mCachedStateInt[GL_BLEND_SRC_ALPHA] = srcAlpha;
		mCachedStateInt[GL_BLEND_DST_ALPHA] = dstAlpha;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// DepthMask
void Context::depthMask( GLboolean enable )
{
	if( mCachedStateBoolean[GL_DEPTH_WRITEMASK] != enable ) {
		mCachedStateBoolean[GL_DEPTH_WRITEMASK] = enable;
		glDepthMask( enable );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// PolygonMode
#if ! defined( CINDER_GLES )
void Context::polygonMode( GLenum face, GLenum mode )
{
	if( face == GL_FRONT_AND_BACK ) {
		if( mCachedFrontPolygonMode != mode || mCachedBackPolygonMode != mode ) {
			mCachedFrontPolygonMode = mCachedBackPolygonMode = mode;
			glPolygonMode( GL_FRONT_AND_BACK, mode );
		}
	}
	else if( face == GL_FRONT ) {
		if( mCachedFrontPolygonMode != mode ) {
			mCachedFrontPolygonMode = mode;
			glPolygonMode( GL_FRONT, mode );
		}
	}
	else if( face == GL_BACK ) {
		if( mCachedBackPolygonMode != mode ) {
			mCachedBackPolygonMode = mode;
			glPolygonMode( GL_BACK, mode );
		}		
	}
}

#endif // defined( CINDER_GLES )

const ColorAf& Context::getCurrentColor() const
{
	return mColor;
}

GlslProgRef	Context::getStockShader( const ShaderDef &shaderDef )
{
	auto existing = mStockShaders.find( shaderDef );
	if( existing == mStockShaders.end() ) {
		auto result = gl::env()->buildShader( shaderDef );
		mStockShaders[shaderDef] = result;
		return result;
	}
	else
		return existing->second;
}

VboRef Context::getDefaultArrayVbo( size_t requiredSize )
{
	if( ! mDefaultArrayVbo ) {
		mDefaultArrayVbo = Vbo::create( GL_ARRAY_BUFFER, requiredSize );
	}
	else if( requiredSize > mDefaultArrayVbo->getSize() ) {
		mDefaultArrayVbo = Vbo::create( GL_ARRAY_BUFFER, requiredSize );
	}
	
	return mDefaultArrayVbo;
}

VboRef Context::getDefaultElementVbo( size_t requiredSize )
{
	if( ! mDefaultElementVbo ) {
		mDefaultElementVbo = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, requiredSize );
	}
	if( requiredSize > mDefaultElementVbo->getSize() ) {
		mDefaultElementVbo = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, requiredSize );
	}
	
	return mDefaultElementVbo;
}

///////////////////////////////////////////////////////////////////////////////////////////
void Context::clear()
{
	mVertices.clear();
}
	
void Context::pushBack( const ci::Vec4f &v )
{
	Vertex vertex;		
	vertex.mColor		= mColor;
	vertex.mNormal		= mNormal;
	vertex.mPosition	= v.xyz();
	vertex.mTexCoord	= mTexCoord;
	
	mVertices.push_back( vertex );
}

///////////////////////////////////////////////////////////////////////////////////////////
// BufferScope
BufferScope::BufferScope( const BufferObjRef &bufferObj )
	: mCtx( gl::context() ), mTarget( bufferObj->getTarget() )
{
	mPrevId = mCtx->getBufferBinding( mTarget );
	mCtx->bindBuffer( mTarget, bufferObj->getId() );
}

///////////////////////////////////////////////////////////////////////////////////////////
// BlendScope
BlendScope::BlendScope( GLboolean enable )
	: mCtx( gl::context() ), mSaveFactors( false )
{
	mPrevBlend = mCtx->stateGet<GLboolean>( GL_BLEND );
	mCtx->stateSet( GL_BLEND, enable );
}

//! Parallels glBlendFunc(), implicitly enables blending
BlendScope::BlendScope( GLenum sfactor, GLenum dfactor )
	: mCtx( gl::context() ), mSaveFactors( true )
{
	mPrevBlend = mCtx->stateGet<GLboolean>( GL_BLEND );
	mPrevSrcRgb = mCtx->stateGet<GLint>( GL_BLEND_SRC_RGB );
	mPrevDstRgb = mCtx->stateGet<GLint>( GL_BLEND_DST_RGB );
	mPrevSrcAlpha = mCtx->stateGet<GLint>( GL_BLEND_SRC_ALPHA );
	mPrevDstAlpha = mCtx->stateGet<GLint>( GL_BLEND_DST_ALPHA );
	mCtx->stateSet<GLboolean>( GL_BLEND, GL_TRUE );
	mCtx->blendFuncSeparate( sfactor, dfactor, sfactor, dfactor );
}

//! Parallels glBlendFuncSeparate(), implicitly enables blending
BlendScope::BlendScope( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
	: mCtx( gl::context() ), mSaveFactors( true )
{
	mPrevBlend = mCtx->stateGet<GLboolean>( GL_BLEND );
	mPrevSrcRgb = mCtx->stateGet<GLint>( GL_BLEND_SRC_RGB );
	mPrevDstRgb = mCtx->stateGet<GLint>( GL_BLEND_DST_RGB );
	mPrevSrcAlpha = mCtx->stateGet<GLint>( GL_BLEND_SRC_ALPHA );
	mPrevDstAlpha = mCtx->stateGet<GLint>( GL_BLEND_DST_ALPHA );
	mCtx->stateSet<GLboolean>( GL_BLEND, GL_TRUE );
	mCtx->blendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

BlendScope::~BlendScope()
{
	mCtx->stateSet( GL_BLEND, mPrevBlend );
	if( mSaveFactors )
		mCtx->blendFuncSeparate( mPrevSrcRgb, mPrevDstRgb, mPrevSrcAlpha, mPrevDstAlpha );
}

///////////////////////////////////////////////////////////////////////////////////////////
// FramebufferScope
FramebufferScope::FramebufferScope()
	: mCtx( gl::context() ), mTarget( GL_FRAMEBUFFER )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	mPrevFramebuffer = mCtx->getFramebufferBinding( GL_FRAMEBUFFER );
#else
	mPrevReadFramebuffer = mCtx->getFramebufferBinding( GL_READ_FRAMEBUFFER );
	mPrevDrawFramebuffer = mCtx->getFramebufferBinding( GL_DRAW_FRAMEBUFFER );
#endif
}

FramebufferScope::FramebufferScope( const FboRef &fbo, GLenum target )
	: mCtx( gl::context() ), mTarget( target )
{
	saveState();
	fbo->bindFramebuffer();
}

FramebufferScope::FramebufferScope( GLenum target, GLuint framebuffer )
	: mCtx( gl::context() ), mTarget( target )
{
	saveState();
	mCtx->bindFramebuffer( target, framebuffer );
}

void FramebufferScope::saveState()
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	mPrevFramebuffer = mCtx->getFramebufferBinding( GL_FRAMEBUFFER );
#else
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_READ_FRAMEBUFFER )
		mPrevReadFramebuffer = mCtx->getFramebufferBinding( GL_READ_FRAMEBUFFER );
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_DRAW_FRAMEBUFFER )
		mPrevDrawFramebuffer = mCtx->getFramebufferBinding( GL_DRAW_FRAMEBUFFER );
#endif
}

FramebufferScope::~FramebufferScope()
{	
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	mCtx->bindFramebuffer( GL_FRAMEBUFFER, mPrevFramebuffer );
#else
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_READ_FRAMEBUFFER )
		mCtx->bindFramebuffer( GL_READ_FRAMEBUFFER, mPrevReadFramebuffer );
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_DRAW_FRAMEBUFFER )
		mCtx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mPrevDrawFramebuffer );
#endif
}


} } // namespace cinder::gl