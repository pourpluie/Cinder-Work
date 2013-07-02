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
#include "cinder/Utilities.h"

#include "cinder/app/App.h"

namespace cinder { namespace gl {

using namespace std;
	
Context::Context()
	: mColor( ColorAf::white() ), mFogEnabled( false ), mLighting( false ), mMaterialEnabled( false ),
	mMode( GL_TRIANGLES ), mNormal( Vec3f( 0.0f, 0.0f, 1.0f ) ), mTexCoord( Vec4f::zero() ),
	mTextureUnit( -1 ), mWireframe( false ), mTrueGlslProgId( 0 )
#if ! defined( CINDER_GLES )
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
	}
}

GLuint Context::vaoGet()
{
	return mCachedVao;
}

//////////////////////////////////////////////////////////////////
// Buffer
void Context::bufferBind( GLenum target, GLuint id )
{
	mTrueBuffer[target] = mActiveBuffer[target] = id;
	glBindBuffer( target, mTrueBuffer[target] );

/*	if( mTrueBuffer.find( target ) == mTrueBuffer.end() )
		mTrueBuffer[target] = 0;
	
	if( mTrueBuffer[target] != mActiveBuffer[target] ) {
		mTrueBuffer[target] = mActiveBuffer[target];
		glBindBuffer( target, mTrueBuffer[target] );
	}*/
}

GLuint Context::bufferGet( GLenum target )
{
	auto active = mActiveBuffer.find( target );
	if( mActiveBuffer.find( target ) == mActiveBuffer.end() ) {
		mActiveBuffer[target] = 0;
		return 0;
	}
	else
		return active->second;
}

void Context::bufferRestore( GLenum target, GLuint id )
{
	mActiveBuffer[target] = id;
}

void Context::bufferPrepareUse( GLenum target )
{
	if( mActiveBuffer.find( target ) == mActiveBuffer.end() )
		mActiveBuffer[target] = 0;
	if( mTrueBuffer.find( target ) == mTrueBuffer.end() )
		mTrueBuffer[target] = 0;

	if( mTrueBuffer[target] != mActiveBuffer[target] ) {
		mTrueBuffer[target] = mActiveBuffer[target];
		glBindBuffer( target, mTrueBuffer[target] );
	}
}

//////////////////////////////////////////////////////////////////
// Shader
void Context::shaderUse( const GlslProgRef &prog )
{
	mActiveGlslProg = prog;
	shaderPrepareUse();
}

GlslProgRef Context::shaderGet()
{
	return mActiveGlslProg;
}

void Context::shaderRestore( const GlslProgRef &prog )
{
	mActiveGlslProg = prog;
}

void Context::shaderPrepareUse()
{
	if( ( (! mActiveGlslProg) && (mTrueGlslProgId != 0) ) ||
		( mActiveGlslProg && mActiveGlslProg->getHandle() != mTrueGlslProgId ) ) {
		mTrueGlslProgId = ( mActiveGlslProg ) ? mActiveGlslProg->getHandle() : 0;
		glUseProgram( mTrueGlslProgId );
	}
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
	assert( mTrueBuffer[GL_ARRAY_BUFFER] == queriedInt );

	// GL_ELEMENT_ARRAY_BUFFER
	glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
	assert( mTrueBuffer[GL_ELEMENT_ARRAY_BUFFER] == queriedInt );

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
	shaderPrepareUse();
	bufferPrepareUse( GL_ARRAY_BUFFER );
	bufferPrepareUse( GL_ELEMENT_ARRAY_BUFFER );
	blendPrepareUse();
	depthMaskPrepareUse();
#if ! defined( CINDER_GLES )
	polygonModePrepareUse();
#endif
}

void Context::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	bufferPrepareUse( GL_ARRAY_BUFFER );
	glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void Context::enableVertexAttribArray( GLuint index )
{
	glEnableVertexAttribArray( index );
}

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

///////////////////////////////////////////////////////////////////////////////////////////
void Context::clear()
{
	mVertices.clear();
}

void Context::draw()
{
	if( ! mVertices.empty() ) {
		// Choose shader
		Shader::UniformOptions options;
		options.enableColor();
		options.setLightingModel( mLighting ? Shader::LightingModel::LAMBERT : Shader::LightingModel::NONE );
		options.enableFog( mFogEnabled );
		options.enableMaterial( mMaterialEnabled );
		options.enableTexture( mTextureUnit >= 0 );
		options.setNumLights( mLights.size() );
		options.setPrecision( Shader::Precision::HIGH );
		
		bool found = false;
		for( ShaderMap::const_iterator iter = mShaders.begin(); iter != mShaders.end(); ++iter ) {
			if ( iter->first == options ) {
				found = true;
				break;
			}
		}
		if ( !found ) { // if ( mShader.find( options ) == mShaders.end() ) { // Isn't working here
			mShaders.insert( make_pair( options, Shader::create( options ) ) );
		}
		ShaderRef shader = mShaders[ options ];
		
		// Change mode to line strip for wireframes
		GLenum mode = mMode;
		if ( mWireframe && mode != GL_POINTS ) {
			mode = GL_LINE_STRIP;
		}
		
		// Buffer data
		GLsizei stride = (GLsizei)sizeof( Context::Vertex );
		if ( ! mImmVbo ) {
			mImmVbo = Vbo::create( GL_ARRAY_BUFFER );
			mImmVbo->setUsage( GL_DYNAMIC_DRAW );
		}
		mImmVbo->bufferData( ( GLuint )( mVertices.size() * stride ), &mVertices[ 0 ], GL_DYNAMIC_DRAW );
	
		// Create VAO. All shader variations have the same attribute
		// layout, so we only need to this once
		if( ! mImmVao ) {
			mImmVao = Vao::create();
			const GlslProgRef& glslProg = shader->getGlslProg();
			
			GLint offset	= 0;
			GLint size		= 4;
			Vao::Attribute attrColor( glslProg->getAttribLocation( "aColor" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrColor );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrNormal( glslProg->getAttribLocation( "aNormal" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrNormal );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrPosition( glslProg->getAttribLocation( "aPosition" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrPosition );
			offset			+= size * sizeof( float );
			
			size			= 4;
			Vao::Attribute attrTexCoord( glslProg->getAttribLocation( "aTexCoord" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrTexCoord );
			offset			+= size * sizeof( float );
		}
		
		// Set uniforms
		shader->setModelViewProjectionMatrix( mProjection.back() * mModelView.back() );
		if ( mFogEnabled ) {
			shader->setFog( mFog );
		}
		if ( mMaterialEnabled ) {
			shader->setMaterial( mMaterial );
		}
		if ( mTextureUnit >= 0 ) {
			shader->setTextureId( mTextureUnit );
		}
		if ( mLights.size() > 0 ) {
			size_t i = 0;
			for ( vector<Light>::const_iterator iter = mLights.begin(); iter != mLights.end(); ++iter, ++i ) {
				shader->setLight( i, *iter );
			}
		}
		shader->update();
		
		// Draw
		shader->bind();
		VaoScope vaoBind( mImmVao->getId() );
		BufferScope bufferBind( mImmVbo->getTarget(), mImmVbo->getId() );
		drawArrays( mode, 0, mVertices.size() );
		shader->unbind();
	}
	clear();
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
// ScopeVao
VaoScope::VaoScope( const VaoRef &vao )
	: mCtx( gl::context() )
{
	mPrevId = mCtx->vaoGet();
	mCtx->vaoBind( vao->getId() );
}

///////////////////////////////////////////////////////////////////////////////////////////
// ScopeBuffer
BufferScope::BufferScope( const BufferObjRef &bufferObj )
	: mCtx( gl::context() ), mTarget( bufferObj->getTarget() )
{
	mPrevId = mCtx->bufferGet( mTarget );
	mCtx->bufferBind( mTarget, bufferObj->getId() );
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



} } // namespace cinder::gl