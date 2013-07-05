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

#include "cinder/app/App.h"

namespace cinder { namespace gl {

using namespace std;
	
Context::Context()
	: mColor( ColorAf::white() ), mFogEnabled( false ), mLighting( false ), mMaterialEnabled( false ),
	mMode( GL_TRIANGLES ), mNormal( Vec3f( 0.0f, 0.0f, 1.0f ) ), mTexCoord( Vec4f::zero() ),
	mTextureUnit( -1 ), mWireframe( false )
#if defined( CINDER_GLES )
	,mCachedFramebuffer( -1 )
#else
	,mCachedReadFramebuffer( -1 ), mCachedDrawFramebuffer( -1 ),
	,mTrueFrontPolygonMode( GL_FILL ), mTrueBackPolygonMode( GL_FILL )
#endif
{
	env()->initializeContextDefaults( this );

	// setup default VAO
#if ! defined( CINDER_GLES )
	glGenVertexArrays( 1, &mDefaultVaoId );
	glBindVertexArray( mDefaultVaoId );
	mCachedVao = mDefaultVaoId;
#else
	mCachedVao = 0;
#endif

	mActiveFrontPolygonMode = mTrueFrontPolygonMode;
	mActiveBackPolygonMode = mTrueBackPolygonMode;

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

//////////////////////////////////////////////////////////////////
// VAO
void Context::vaoBind( GLuint id )
{
	if( mCachedVao != id ) {
		mCachedVao = id;
#if defined( CINDER_GLES )
		glBindVertexArrayOES( mCachedVao );
#else
		glBindVertexArray( mCachedVao );
#endif

		// binding a VAO invalidates other pieces of cached state
		invalidateBufferBinding( GL_ARRAY_BUFFER );
		invalidateBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
	}
}

GLuint Context::vaoGet()
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
// Framebuffers
void Context::bindFramebuffer( GLenum target, GLuint framebuffer )
{
#if defined( CINDER_GLES )
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
#if defined( CINDER_GLES )
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
	mActiveStateBoolean[cap] = value;
	
	if( mTrueStateBoolean[cap] != mActiveStateBoolean[cap] ) {
		mTrueStateBoolean[cap] = mActiveStateBoolean[cap];
		if( mTrueStateBoolean[cap] )
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
	if( (mActiveStateBoolean.find( cap ) == mActiveStateBoolean.end()) || (mTrueStateBoolean.find( cap ) == mTrueStateBoolean.end()) )
		mTrueStateBoolean[cap] = mActiveStateBoolean[cap] = glIsEnabled( cap );
		
	return mActiveStateBoolean[cap];
}

template<>
GLint Context::stateGet<GLint>( GLenum pname )
{
	if( (mActiveStateInt.find( pname ) == mActiveStateInt.end()) || (mTrueStateInt.find( pname ) == mTrueStateInt.end()) ) {
		GLint curValue;
		glGetIntegerv( pname, &curValue );
		mTrueStateInt[pname] = mActiveStateInt[pname] = curValue;
	}
		
	return mActiveStateInt[pname];
}

template<>
bool Context::stateIsDirty<GLboolean>( GLenum pname )
{
	return mTrueStateBoolean[pname] != mActiveStateBoolean[pname];
}

template<>
bool Context::stateIsDirty<GLint>( GLenum pname )
{
	return mTrueStateInt[pname] != mActiveStateInt[pname];
}

template<>
void Context::stateRestore<GLboolean>( GLenum cap, GLboolean value )
{
	mActiveStateBoolean[cap] = value;
}

template<>
void Context::statePrepareUse<GLboolean>( GLenum cap )
{
	if( (mActiveStateBoolean.find( cap ) == mActiveStateBoolean.end()) || (mTrueStateBoolean.find( cap ) == mTrueStateBoolean.end()) )
		mTrueStateBoolean[cap] = mActiveStateBoolean[cap] = glIsEnabled( cap );
		
	if( mTrueStateBoolean[cap] != mActiveStateBoolean[cap] ) {
		mTrueStateBoolean[cap] = mActiveStateBoolean[cap];
		if( mTrueStateBoolean[cap] )
			glEnable( cap );
		else
			glDisable( cap );
	}
}

void Context::statesPrepareUse()
{
	for( auto stateIt = mActiveStateBoolean.cbegin(); stateIt != mActiveStateBoolean.cend(); ++stateIt )
		statePrepareUse<GLboolean>( stateIt->first );
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
	assert( mCachedVao == queriedInt );
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

void Context::prepareDraw()
{
	blendPrepareUse();
	depthMaskPrepareUse();
#if ! defined( CINDER_GLES )
	polygonModePrepareUse();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Attributes
void Context::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void Context::enableVertexAttribArray( GLuint index )
{
	glEnableVertexAttribArray( index );
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
	blendFuncSeparateRestore( sfactor, dfactor, sfactor, dfactor );
	blendPrepareUse();	
}

void Context::blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	blendFuncSeparateRestore( srcRGB, dstRGB, srcAlpha, dstAlpha );
	blendPrepareUse();
}

void Context::blendFuncSeparateRestore( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	mActiveStateInt[GL_BLEND_SRC_RGB] = srcRGB;
	mActiveStateInt[GL_BLEND_DST_RGB] = dstRGB;
	mActiveStateInt[GL_BLEND_SRC_ALPHA] = srcAlpha;
	mActiveStateInt[GL_BLEND_DST_ALPHA] = dstAlpha;
}

void Context::blendPrepareUse()
{
	statePrepareUse<GLboolean>( GL_BLEND );

	if( stateIsDirty<GLint>( GL_BLEND_SRC_RGB ) ||
		stateIsDirty<GLint>( GL_BLEND_DST_RGB ) ||
		stateIsDirty<GLint>( GL_BLEND_SRC_ALPHA ) ||
		stateIsDirty<GLint>( GL_BLEND_DST_ALPHA ) )
	{
		GLint srcRgb = stateGet<GLint>( GL_BLEND_SRC_RGB );
		GLint dstRgb = stateGet<GLint>( GL_BLEND_DST_RGB );
		GLint srcAlpha = stateGet<GLint>( GL_BLEND_SRC_ALPHA );
		GLint dstAlpha = stateGet<GLint>( GL_BLEND_DST_ALPHA );						
	
		glBlendFuncSeparate( srcRgb, dstRgb, srcAlpha, dstAlpha );

		mTrueStateInt[GL_BLEND_SRC_RGB] = srcRgb;
		mTrueStateInt[GL_BLEND_DST_RGB] = dstRgb;
		mTrueStateInt[GL_BLEND_SRC_ALPHA] = srcAlpha;
		mTrueStateInt[GL_BLEND_DST_ALPHA] = dstAlpha;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// DepthMask
void Context::depthMask( GLboolean enable )
{
	mActiveStateBoolean[GL_DEPTH_WRITEMASK] = enable;
	depthMaskPrepareUse();
}

void Context::depthMaskPrepareUse()
{
	if( stateIsDirty<GLboolean>( GL_DEPTH_WRITEMASK ) ) {
		mTrueStateBoolean[GL_DEPTH_WRITEMASK] = mActiveStateBoolean[GL_DEPTH_WRITEMASK];
		glDepthMask( mTrueStateBoolean[GL_DEPTH_WRITEMASK] );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// PolygonMode
#if ! defined( CINDER_GLES )
void Context::polygonMode( GLenum face, GLenum mode )
{
	if( face == GL_FRONT_AND_BACK ) {
		mActiveFrontPolygonMode = mActiveBackPolygonMode = mode;
	}
	else if( face == GL_FRONT ) {
		mActiveFrontPolygonMode = mode;
	}
	else if( face == GL_BACK ) {
		mActiveBackPolygonMode = mode;
	}
}

void Context::polygonModePrepareUse()
{
	// see if we can combine into one glPolygonMode call
	// incidentally, this is the only valid option for 3.0+
	if((( mActiveFrontPolygonMode != mTrueFrontPolygonMode ) ||
		( mActiveBackPolygonMode != mTrueBackPolygonMode )) &&
		( mActiveFrontPolygonMode == mActiveBackPolygonMode ) ) {
		mTrueBackPolygonMode = mTrueFrontPolygonMode = mActiveFrontPolygonMode;
		glPolygonMode( GL_FRONT_AND_BACK, mTrueFrontPolygonMode );
	}
	else {
		if( mActiveFrontPolygonMode != mTrueFrontPolygonMode ) {
			mTrueFrontPolygonMode = mActiveFrontPolygonMode;
			glPolygonMode( GL_FRONT, mTrueFrontPolygonMode );
		}
		
		if( mActiveBackPolygonMode != mTrueBackPolygonMode ) {
			mTrueBackPolygonMode = mActiveBackPolygonMode;
			glPolygonMode( GL_BACK, mTrueBackPolygonMode );
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
// VaoScope
VaoScope::VaoScope( const VaoRef &vao )
	: mCtx( gl::context() )
{
	mPrevId = mCtx->vaoGet();
	mCtx->vaoBind( vao->getId() );
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
// ScopeBlend
ScopeBlend::ScopeBlend( GLboolean enable )
	: mCtx( gl::context() ), mSaveFactors( false )
{
	mPrevBlend = mCtx->stateGet<GLboolean>( GL_BLEND );
	mCtx->stateSet( GL_BLEND, enable );
}

//! Parallels glBlendFunc(), implicitly enables blending
ScopeBlend::ScopeBlend( GLenum sfactor, GLenum dfactor )
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
ScopeBlend::ScopeBlend( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
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

ScopeBlend::~ScopeBlend()
{
	mCtx->stateRestore( GL_BLEND, mPrevBlend );
	if( mSaveFactors )
		mCtx->blendFuncSeparateRestore( mPrevSrcRgb, mPrevDstRgb, mPrevSrcAlpha, mPrevDstAlpha );
}

///////////////////////////////////////////////////////////////////////////////////////////
// FramebufferScope
FramebufferScope::FramebufferScope()
	: mCtx( gl::context() ), mTarget( GL_FRAMEBUFFER )
{
#if defined( CINDER_GLES )
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
	mCtx->bindFramebuffer( target, fbo->getId() );
}

FramebufferScope::FramebufferScope( GLenum target, GLuint framebuffer )
	: mCtx( gl::context() ), mTarget( target )
{
	saveState();
	mCtx->bindFramebuffer( target, framebuffer );
}

void FramebufferScope::saveState()
{
#if defined( CINDER_GLES )
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
#if defined( CINDER_GLES )
	mCtx->bindFramebuffer( GL_FRAMEBUFFER, mPrevFramebuffer );
#else
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_READ_FRAMEBUFFER )
		mCtx->bindFramebuffer( GL_READ_FRAMEBUFFER, mPrevReadFramebuffer );
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_DRAW_FRAMEBUFFER )
		mCtx->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mPrevDrawFramebuffer );
#endif
}


} } // namespace cinder::gl