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
#include "cinder/gl/Batch.h"
#include "cinder/Utilities.h"

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

Context::Context( const std::shared_ptr<PlatformData> &platformData )
	: mPlatformData( platformData ),
	mColor( ColorAf::white() ), mCachedActiveTexture( 0 )
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
	mDefaultVao = Vao::create();
	mVaoStack.push_back( mDefaultVao );
	mDefaultVao->setContext( this );
	mDefaultVao->bindImpl( NULL );
#endif

	mImmediateMode = gl::VertBatch::create();
	
	GLint params[ 4 ];
	glGetIntegerv( GL_VIEWPORT, params );
	mViewport = std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) );
    
    glGetIntegerv( GL_SCISSOR_BOX, params );
    mScissor = std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) );
    
    mModelViewStack.push_back( Matrix44f() );
	mModelViewStack.back().setToIdentity();
	mProjectionStack.push_back( Matrix44f() );
	mProjectionStack.back().setToIdentity();
	mGlslProgStack.push_back( GlslProgRef() );
}

Context::~Context()
{
	if( getCurrent() == this ) {
		env()->makeContextCurrent( NULL );

	#if defined( CINDER_COCOA )
		pthread_setspecific( sThreadSpecificCurrentContextKey, NULL );
	#else
		sThreadSpecificCurrentContext = (Context*)( nullptr );
	#endif
	}
}

ContextRef Context::create( const Context *sharedContext )
{
	return env()->createSharedContext( sharedContext );

}

ContextRef Context::createFromExisting( const std::shared_ptr<PlatformData> &platformData )
{
	env()->initializeFunctionPointers();
	ContextRef result( std::shared_ptr<Context>( new Context( platformData ) ) );

	return result;
}

void Context::makeCurrent() const
{
	env()->makeContextCurrent( this );

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
void Context::bindVao( const VaoRef &vao )
{
	VaoRef prevVao = getVao();

	if( mVaoStack.empty() || (prevVao != vao) ) {
		if( ! mVaoStack.empty() )
			mVaoStack.back() = vao;
		if( prevVao )
			prevVao->unbindImpl( this );
		if( vao )
			vao->bindImpl( this );
	}
}

void Context::pushVao( const VaoRef &vao )
{
	VaoRef prevVao = getVao();
	mVaoStack.push_back( vao );
	if( prevVao )
		prevVao->unbindImpl( this );
	if( vao )
		vao->bindImpl( this );
}

void Context::popVao()
{
	VaoRef prevVao = getVao();

	if( ! mVaoStack.empty() ) {
		mVaoStack.pop_back();
		if( ! mVaoStack.empty() ) {
			if( prevVao != mVaoStack.back() ) {
				if( prevVao )
					prevVao->unbindImpl( this );
				if( mVaoStack.back() )
					mVaoStack.back()->bindImpl( this );
			}
		}
		else {
			if( prevVao )
				prevVao->unbindImpl( this );
		}
	}
}

VaoRef Context::getVao()
{
	if( ! mVaoStack.empty() )
		return mVaoStack.back();
	else
		return VaoRef();
}

//////////////////////////////////////////////////////////////////
// Viewport
	
void Context::setViewport( const std::pair<Vec2i, Vec2i> &viewport )
{
	mViewport = viewport;
	glViewport( mViewport.first.x, mViewport.first.y, mViewport.second.x, mViewport.second.y );
}
    
//////////////////////////////////////////////////////////////////
// Scissor Test
void Context::setScissor( const std::pair<Vec2i, Vec2i> &scissor )
{
	mScissor = scissor;
	glScissor( mScissor.first.x, mScissor.first.y, mScissor.second.x, mScissor.second.y );
}

//////////////////////////////////////////////////////////////////
// Buffer
void Context::bindBuffer( GLenum target, GLuint id )
{
	auto cachedIt = mBufferBindingStack.find( target );
	if( (cachedIt == mBufferBindingStack.end()) || cachedIt->second.empty() || ( cachedIt->second.back() != id ) ) {
		// first time we've met this target; start an empty stack
		if( cachedIt == mBufferBindingStack.end() )
			mBufferBindingStack[target] = vector<int>();
		mBufferBindingStack[target].push_back( id );
		// for these targets we need to alert the VAO
		if( target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER ) {
			VaoRef vao = getVao();
			if( vao )
				vao->reflectBindBufferImpl( target, id );
		}
		else {
			glBindBuffer( target, id );
		}
	}
}

void Context::pushBufferBinding( GLenum target, GLuint id )
{
	auto cachedIt = mBufferBindingStack.find( target );
	int existing = -1;
	if( cachedIt == mBufferBindingStack.end() )
		mBufferBindingStack[target] = vector<int>();
	else if( ! cachedIt->second.empty() )
		existing = cachedIt->second.back();

	mBufferBindingStack[target].push_back( id );
	
	if( existing != id )
		glBindBuffer( target, id );
}

void Context::popBufferBinding( GLenum target )
{
	GLuint existing = getBufferBinding( target );
	auto cachedIt = mBufferBindingStack.find( target );
	if( ( cachedIt == mBufferBindingStack.end() ) && ( ! cachedIt->second.empty() ) ) {
		cachedIt->second.pop_back();
		if( ( ! cachedIt->second.empty() ) && ( existing != cachedIt->second.back() ) )
			glBindBuffer( target, cachedIt->second.back() );
	}
}

GLuint Context::getBufferBinding( GLenum target )
{
	auto cachedIt = mBufferBindingStack.find( target );
	if( (cachedIt == mBufferBindingStack.end()) || cachedIt->second.empty() || ( cachedIt->second.back() == -1 ) ) {
		GLint queriedInt = -1;
		switch( target ) {
			case GL_ARRAY_BUFFER:
				glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &queriedInt );
			break;
			case GL_ELEMENT_ARRAY_BUFFER:
				glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
			break;
			case GL_PIXEL_PACK_BUFFER:
				glGetIntegerv( GL_PIXEL_PACK_BUFFER_BINDING, &queriedInt );
			break;
			case GL_PIXEL_UNPACK_BUFFER:
				glGetIntegerv( GL_PIXEL_UNPACK_BUFFER_BINDING, &queriedInt );
			break;
			case GL_TEXTURE_BUFFER:
				glGetIntegerv( GL_TEXTURE_BINDING_BUFFER, &queriedInt );
			break;
			case GL_TRANSFORM_FEEDBACK_BUFFER:
				glGetIntegerv( GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, &queriedInt );
			break;
			case GL_UNIFORM_BUFFER:
				glGetIntegerv( GL_UNIFORM_BUFFER_BINDING, &queriedInt );
			break;
			default:
				; // warning?
		}
		
		// first time we've met this target; start an empty stack
		if( cachedIt == mBufferBindingStack.end() )
			mBufferBindingStack[target] = vector<int>();
		mBufferBindingStack[target].back() = queriedInt;
		return (GLuint)queriedInt;
	}
	else
		return (GLuint)cachedIt->second.back();
}

void Context::invalidateBufferBinding( GLenum target )
{
	// first time we've met this target; start an empty stack
	if( mBufferBindingStack.find(target) == mBufferBindingStack.end() ) {
		mBufferBindingStack[target] = vector<int>();
		mBufferBindingStack[target].push_back( -1 );
	}
	else
		mBufferBindingStack[target].back() = -1;
}

//////////////////////////////////////////////////////////////////
// Shader
void Context::pushGlslProg( const GlslProgRef &prog )
{
	GlslProgRef prevGlsl = getGlslProg();

	mGlslProgStack.push_back( prog );
	if( prog != prevGlsl ) {
		if( prog )
			glUseProgram( prog->getHandle() );
		else
			glUseProgram( 0 );
	}
}

void Context::popGlslProg()
{
	GlslProgRef prevGlsl = getGlslProg();

	if( ! mGlslProgStack.empty() ) {
		mGlslProgStack.pop_back();
		if( ! mGlslProgStack.empty() ) {
			if( prevGlsl != mGlslProgStack.back() ) {
				if( mGlslProgStack.back() )
					glUseProgram( mGlslProgStack.back()->getHandle() );
				else
					glUseProgram( 0 );
			}
		}
	}
}

void Context::bindGlslProg( const GlslProgRef &prog )
{
	if( mGlslProgStack.empty() || (mGlslProgStack.back() != prog) ) {
		if( ! mGlslProgStack.empty() )
			mGlslProgStack.back() = prog;
		if( prog )
			glUseProgram( prog->getHandle() );
		else
			glUseProgram( 0 );
	}
}

GlslProgRef Context::getGlslProg()
{
	if( ! mGlslProgStack.empty() )
		return mGlslProgStack.back();
	else
		return GlslProgRef();
}

//////////////////////////////////////////////////////////////////
// TextureBinding
void Context::bindTexture( GLenum target, GLuint texture )
{
	auto cachedIt = mCachedTextureBinding.find( target );
	if( ( cachedIt == mCachedTextureBinding.end() ) || ( cachedIt->second != texture ) ) {
		mCachedTextureBinding[target] = texture;
		glBindTexture( target, texture );
	}
}

void Context::textureDeleted( GLenum target, GLuint textureId )
{
	auto cachedIt = mCachedTextureBinding.find( target );
	if( cachedIt != mCachedTextureBinding.end() && cachedIt->second == textureId ) {
		// GL will have set the binding to 0 for target, so let's do the same
		mCachedTextureBinding[target] = 0;
	}
}

GLenum Context::getTextureBinding( GLenum target )
{
	auto cachedIt = mCachedTextureBinding.find( target );
	if( (cachedIt == mCachedTextureBinding.end()) || ( cachedIt->second == -1 ) ) {
		GLint queriedInt = -1;
		GLenum targetBinding = Texture::getBindingConstantForTarget( target );
		if( target > 0 ) {
			glGetIntegerv( targetBinding, &queriedInt );
		}
		else {
			return 0; // warning?
		}
		
		mCachedTextureBinding[target] = queriedInt;
		return (GLenum)queriedInt;
	}
	else
		return (GLenum)cachedIt->second;

}

//////////////////////////////////////////////////////////////////
// ActiveTexture
void Context::activeTexture( uint8_t textureUnit )
{
	if( mCachedActiveTexture != textureUnit ) {
		mCachedActiveTexture = textureUnit;
		glActiveTexture( GL_TEXTURE0 + textureUnit );
	}
}

uint8_t Context::getActiveTexture()
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
void Context::setBoolState( GLenum cap, GLboolean value )
{
	bool needsToBeSet = true;
	auto cached = mStateStackBoolean.find( cap );
	if( ( cached != mStateStackBoolean.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	if( cached == mStateStackBoolean.end() ) {
		mStateStackBoolean[cap] = vector<GLboolean>();
		mStateStackBoolean[cap].push_back( value );
	}
	else
		mStateStackBoolean[cap].back() = value;
	if( needsToBeSet ) {
		if( value )
			glEnable( cap );
		else
			glDisable( cap );
	}	
}

void Context::setBoolState( GLenum cap, GLboolean value, const std::function<void(GLboolean)> &setter )
{
	bool needsToBeSet = true;
	auto cached = mStateStackBoolean.find( cap );
	if( ( cached != mStateStackBoolean.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	if( cached == mStateStackBoolean.end() ) {
		mStateStackBoolean[cap] = vector<GLboolean>();
		mStateStackBoolean[cap].push_back( value );
	}
	else
		mStateStackBoolean[cap].back() = value;
	if( needsToBeSet )
		setter( value );
}

void Context::pushBoolState( GLenum cap, GLboolean value )
{
	bool needsToBeSet = true;
	auto cached = mStateStackBoolean.find( cap );
	if( ( cached != mStateStackBoolean.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	else if( cached == mStateStackBoolean.end() )
		mStateStackBoolean[cap] = vector<GLboolean>();
	mStateStackBoolean[cap].push_back( value );
	if( needsToBeSet ) {
		if( value )
			glEnable( cap );
		else
			glDisable( cap );
	}	
}

void Context::popBoolState( GLenum cap )
{
	auto cached = mStateStackBoolean.find( cap );
	if( ( cached != mStateStackBoolean.end() ) && ( ! cached->second.empty() ) ) {
		GLboolean prevValue = cached->second.back();
		cached->second.pop_back();
		if( ! cached->second.empty() ) {
			if( cached->second.back() != prevValue ) {
				if( cached->second.back() )
					glEnable( cap );
				else
					glDisable( cap );
			}
		}
	}
}

void Context::enable( GLenum cap, GLboolean value )
{
	setBoolState( cap, value );
}

GLboolean Context::getBoolState( GLenum cap )
{
	auto cached = mStateStackBoolean.find( cap );
	if( ( cached == mStateStackBoolean.end() ) || cached->second.empty() ) {
		GLboolean result = glIsEnabled( cap );
		if( cached == mStateStackBoolean.end() )
			mStateStackBoolean[cap] = vector<GLboolean>();
		mStateStackBoolean[cap].push_back( result );
		return result;
	}
	else
		return cached->second.back();
}

bool Context::pushIntState( std::vector<GLint> &stack, GLint value )
{
	bool needsToBeSet = true;
	if( ( ! stack.empty() ) && ( stack.back() == value ) )
		needsToBeSet = false;
	stack.push_back( value );
	return needsToBeSet;
}

bool Context::popIntState( std::vector<GLint> &stack )
{
	if( ! stack.empty() ) {
		GLint prevValue = stack.back();
		stack.pop_back();
		if( ! stack.empty() )
			return stack.back() != prevValue;
		else
			return true;
	}
	else
		return true;
}

bool Context::setIntState( std::vector<GLint> &stack, GLint value )
{
	bool needsToBeSet = true;
	if( ( ! stack.empty() ) && ( stack.back() == value ) )
		needsToBeSet = false;
	else if( stack.empty() )
		stack.push_back( value );
	else
		stack.back() = value;
	return needsToBeSet;
}

bool Context::getIntState( std::vector<GLint> &stack, GLint *result )
{
	if( stack.empty() )
		return false;
	else {
		*result = stack.back();
		return true;
	}
}

/*
template<>
GLint Context::getState<GLint>( GLenum cap )
{
	auto cached = mStateStackInt.find( cap );
	if( ( cached == mStateStackInt.end() ) || cached->second.empty() ) {
		GLint result;
		glGetIntegerv( cap, &result );
		if( cached->second.empty() )
			cached->second = vector<GLint>();
		mStateStackInt[cap].push_back( result );
		return result;
	}
	else
		return cached->second.back();
}*/

/////////////////////////////////////////////////////////////////////////////////////
// This routine confirms that the ci::gl::Context's understanding of the world matches OpenGL's
void Context::sanityCheck()
{
return;
	GLint queriedInt;

	// assert cached GL_ARRAY_BUFFER is correct
	glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &queriedInt );
	assert( getBufferBinding( GL_ARRAY_BUFFER ) == queriedInt );

	// assert cached GL_ELEMENT_ARRAY_BUFFER is correct
	glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &queriedInt );
	assert( getBufferBinding( GL_ELEMENT_ARRAY_BUFFER ) == queriedInt );

	// assert cached (VAO) GL_VERTEX_ARRAY_BINDING is correct
#if defined( CINDER_GLES )
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING_OES, &queriedInt );
#else
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &queriedInt );
#endif
//	assert( mCachedVao == queriedInt );

	// assert the various texture bindings are correct
	for( auto& cachedTextureBinding : mCachedTextureBinding ) {
		GLenum target = cachedTextureBinding.first;
		GLenum textureBindingConstant = Texture::getBindingConstantForTarget( target );
		glGetIntegerv( textureBindingConstant, &queriedInt );
		GLenum cachedTextureId = (GLenum)queriedInt;
		assert( queriedInt == cachedTextureId );
	}
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
	VaoRef vao = getVao();
	if( vao )
		vao->vertexAttribPointerImpl( index, size, type, normalized, stride, pointer );
}

void Context::enableVertexAttribArray( GLuint index )
{
	VaoRef vao = getVao();
	if( vao )
		vao->enableVertexAttribArrayImpl( index );
}


void Context::disableVertexAttribArray( GLuint index )
{
	VaoRef vao = getVao();
	if( vao )
		vao->disableVertexAttribArrayImpl( index );
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
	bool needsChange = setIntState( mBlendSrcRgbStack, srcRGB );
	needsChange = setIntState( mBlendDstRgbStack, dstRGB ) || needsChange;
	needsChange = setIntState( mBlendSrcAlphaStack, srcAlpha ) || needsChange;
	needsChange = setIntState( mBlendDstAlphaStack, dstAlpha ) || needsChange;
	if( needsChange )
		glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

void Context::pushBlendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	bool needsChange = pushIntState( mBlendSrcRgbStack, srcRGB );
	needsChange = pushIntState( mBlendDstRgbStack, dstRGB ) || needsChange;
	needsChange = pushIntState( mBlendSrcAlphaStack, srcAlpha ) || needsChange;
	needsChange = pushIntState( mBlendDstAlphaStack, dstAlpha ) || needsChange;
	if( needsChange )
		glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

void Context::popBlendFuncSeparate()
{
	bool needsChange = popIntState( mBlendSrcRgbStack );
	needsChange = popIntState( mBlendDstRgbStack ) || needsChange;
	needsChange = popIntState( mBlendSrcAlphaStack ) || needsChange;
	needsChange = popIntState( mBlendDstAlphaStack ) || needsChange;
	if( needsChange && ( ! mBlendSrcRgbStack.empty() ) && ( ! mBlendSrcAlphaStack.empty() ) && ( ! mBlendDstRgbStack.empty() ) && ( ! mBlendDstAlphaStack.empty() ) )
		glBlendFuncSeparate( mBlendSrcRgbStack.back(), mBlendDstRgbStack.back(), mBlendSrcAlphaStack.back(), mBlendDstAlphaStack.back() );
}

///////////////////////////////////////////////////////////////////////////////////////////
// DepthMask
void Context::depthMask( GLboolean enable )
{
	setBoolState( GL_DEPTH_WRITEMASK, enable, glDepthMask );
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

///////////////////////////////////////////////////////////////////////////////////////////
// draw*
void Context::drawArrays( GLenum mode, GLint first, GLsizei count )
{
	glDrawArrays( mode, first, count );
}

void Context::drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	glDrawElements( mode, count, type, indices );
}

///////////////////////////////////////////////////////////////////////////////////////////
// Shaders
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

void Context::setDefaultShaderVars()
{
	auto ctx = gl::context();
	auto glslProg = ctx->getGlslProg();
	if( glslProg ) {
		auto uniforms = glslProg->getUniformSemantics();
		for( auto unifIt = uniforms.cbegin(); unifIt != uniforms.end(); ++unifIt ) {
			switch( unifIt->second ) {
				case UNIFORM_MODELVIEW:
					glslProg->uniform( unifIt->first, gl::getModelView() ); break;
				case UNIFORM_MODELVIEWPROJECTION:
					glslProg->uniform( unifIt->first, gl::getModelViewProjection() ); break;
				case UNIFORM_PROJECTION:
					glslProg->uniform( unifIt->first, gl::getProjection() ); break;
				case UNIFORM_NORMAL_MATRIX:
					glslProg->uniform( unifIt->first, gl::calcNormalMatrix() ); break;
			}
		}

		auto attribs = glslProg->getAttribSemantics();
		for( auto attribIt = attribs.begin(); attribIt != attribs.end(); ++attribIt ) {
			switch( attribIt->second ) {
				case geom::Attrib::COLOR: {
					int loc = glslProg->getAttribLocation( attribIt->first );
					ColorA c = ctx->getCurrentColor();
					gl::vertexAttrib4f( loc, c.r, c.g, c.b, c.a );
				}
				break;
				default:
					;
			}
		}
	}
}

VaoRef Context::getDefaultVao()
{
	if( ! mDefaultVao ) {
		mDefaultVao = Vao::create();
	}

	return mDefaultVao;
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
// BufferScope
BufferScope::BufferScope( const BufferObjRef &bufferObj )
	: mCtx( gl::context() ), mTarget( bufferObj->getTarget() )
{
	mCtx->pushBufferBinding( mTarget, bufferObj->getId() );
}

///////////////////////////////////////////////////////////////////////////////////////////
// BlendScope
BlendScope::BlendScope( GLboolean enable )
	: mCtx( gl::context() ), mSaveFactors( false )
{
	mCtx->pushBoolState( GL_BLEND, enable );
}

//! Parallels glBlendFunc(), implicitly enables blending
BlendScope::BlendScope( GLenum sfactor, GLenum dfactor )
	: mCtx( gl::context() ), mSaveFactors( true )
{
	mCtx->pushBoolState( GL_BLEND, GL_TRUE );
	mCtx->pushBlendFuncSeparate( sfactor, dfactor, sfactor, dfactor );
}

//! Parallels glBlendFuncSeparate(), implicitly enables blending
BlendScope::BlendScope( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
	: mCtx( gl::context() ), mSaveFactors( true )
{
	mCtx->pushBoolState( GL_BLEND, GL_TRUE );
	mCtx->pushBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

BlendScope::~BlendScope()
{
	mCtx->popBoolState( GL_BLEND );
	if( mSaveFactors )
		mCtx->popBlendFuncSeparate();
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

///////////////////////////////////////////////////////////////////////////////////////////

} } // namespace cinder::gl
