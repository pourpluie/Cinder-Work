#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Xfo.h"
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
	mColor( ColorAf::white() )
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	,mCachedFramebuffer( -1 )
#endif
#if ! defined( CINDER_GLES )
	,mCachedFrontPolygonMode( GL_FILL ), mCachedBackPolygonMode( GL_FILL ),
	mCachedXfo( nullptr )
#endif
{
	// setup default VAO
#if ! defined( CINDER_GLES )
	mDefaultVao = Vao::create();
	mVaoStack.push_back( mDefaultVao );
	mDefaultVao->setContext( this );
	mDefaultVao->bindImpl( NULL );
	
	mBufferBindingStack[GL_ARRAY_BUFFER] = vector<int>();
	mBufferBindingStack[GL_ARRAY_BUFFER].push_back( 0 );
	mBufferBindingStack[GL_ELEMENT_ARRAY_BUFFER] = vector<int>();
	mBufferBindingStack[GL_ELEMENT_ARRAY_BUFFER].push_back( 0 );
	 
	mReadFramebufferStack.push_back( 0 );
	mDrawFramebufferStack.push_back( 0 );	
#endif
	mDefaultArrayVboIdx = 0;

	mActiveTextureStack.push_back( 0 );

	mImmediateMode = gl::VertBatch::create();
	
	GLint params[4];
	glGetIntegerv( GL_VIEWPORT, params );
	mViewportStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) ) );
    
	glGetIntegerv( GL_SCISSOR_BOX, params );
	mScissorStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) ) );

	GLint queriedInt;
	glGetIntegerv( GL_BLEND_SRC_RGB, &queriedInt );
	mBlendSrcRgbStack.push_back( queriedInt );
	glGetIntegerv( GL_BLEND_DST_RGB, &queriedInt );
	mBlendDstRgbStack.push_back( queriedInt );
	glGetIntegerv( GL_BLEND_SRC_ALPHA, &queriedInt );
	mBlendSrcAlphaStack.push_back( queriedInt );
	glGetIntegerv( GL_BLEND_DST_ALPHA, &queriedInt );
	mBlendDstAlphaStack.push_back( queriedInt );
    
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
	if( setStackState( mVaoStack, vao ) ) {
		if( prevVao )
			prevVao->unbindImpl( this );
		if( vao )
			vao->bindImpl( this );	
	}
}

void Context::pushVao( const VaoRef &vao )
{
	VaoRef prevVao = getVao();
	if( pushStackState( mVaoStack, vao ) ) {
		if( prevVao )
			prevVao->unbindImpl( this );
		if( vao )
			vao->bindImpl( this );
	}
}

void Context::pushVao()
{
	mVaoStack.push_back( getVao() );
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
void Context::viewport( const std::pair<Vec2i, Vec2i> &viewport )
{
	if( setStackState( mViewportStack, viewport ) )
		glViewport( viewport.first.x, viewport.first.y, viewport.second.x, viewport.second.y );
}

void Context::pushViewport( const std::pair<Vec2i, Vec2i> &viewport )
{
	if( pushStackState( mViewportStack, viewport ) )
		glViewport( viewport.first.x, viewport.first.y, viewport.second.x, viewport.second.y );
}

void Context::pushViewport()
{
	mViewportStack.push_back( getViewport() );
}

void Context::popViewport()
{
	if( mViewportStack.empty() || popStackState( mViewportStack ) ) {
		auto viewport = getViewport();
		glViewport( viewport.first.x, viewport.first.y, viewport.second.x, viewport.second.y );
	}
}

std::pair<Vec2i, Vec2i> Context::getViewport()
{
	if( mViewportStack.empty() ) {
		GLint params[4];
		glGetIntegerv( GL_VIEWPORT, params );
		// push twice in anticipation of later pop
		mViewportStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[0], params[1] ), Vec2i( params[2], params[3] ) ) );
		mViewportStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[0], params[1] ), Vec2i( params[2], params[3] ) ) );
	}

	return mViewportStack.back();
}
    
//////////////////////////////////////////////////////////////////
// Scissor Test
void Context::setScissor( const std::pair<Vec2i, Vec2i> &scissor )
{
	if( setStackState( mScissorStack, scissor ) )
		glScissor( scissor.first.x, scissor.first.y, scissor.second.x, scissor.second.y );
}

void Context::pushScissor( const std::pair<Vec2i, Vec2i> &scissor )
{
	if( pushStackState( mScissorStack, scissor ) )
		glScissor( scissor.first.x, scissor.first.y, scissor.second.x, scissor.second.y );
}

void Context::pushScissor()
{
	mScissorStack.push_back( getScissor() );
}

//! Sets the active texture unit; expects values relative to \c 0, \em not GL_TEXTURE0
void Context::popScissor()
{
	if( mScissorStack.empty() || popStackState( mScissorStack ) ) {
		auto scissor = getScissor();
		glScissor( scissor.first.x, scissor.first.y, scissor.second.x, scissor.second.y );
	}
}

std::pair<Vec2i, Vec2i> Context::getScissor()
{
	if( mScissorStack.empty() ) {
		GLint params[4];
		glGetIntegerv( GL_SCISSOR_BOX, params ); 
		// push twice in anticipation of later pop
		mScissorStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) ) );
		mScissorStack.push_back( std::pair<Vec2i, Vec2i>( Vec2i( params[ 0 ], params[ 1 ] ), Vec2i( params[ 2 ], params[ 3 ] ) ) );
	}

	return mScissorStack.back();
}

//////////////////////////////////////////////////////////////////
// Buffer
void Context::bindBuffer( GLenum target, GLuint id )
{
	GLuint prevValue = getBufferBinding( target );
	if( prevValue != id ) {
		mBufferBindingStack[target].back() = id;
		if( target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER ) {
			VaoRef vao = getVao();
			if( vao )
				vao->reflectBindBufferImpl( target, id );
			else
				glBindBuffer( target, id );
		}
		else
			glBindBuffer( target, id );
	}
}

void Context::pushBufferBinding( GLenum target, GLuint id )
{
	pushBufferBinding( target );
	bindBuffer( target, id );
}

void Context::pushBufferBinding( GLenum target )
{
	GLuint curValue = getBufferBinding( target );
	mBufferBindingStack[target].push_back( curValue );
}

void Context::popBufferBinding( GLenum target )
{
	GLuint prevValue = getBufferBinding( target );
	auto cachedIt = mBufferBindingStack.find( target );
	cachedIt->second.pop_back();
	if( ! cachedIt->second.empty() && cachedIt->second.back() != prevValue ) {
		if( target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER ) {
			VaoRef vao = getVao();
			if( vao )
				vao->reflectBindBufferImpl( target, cachedIt->second.back() );
			else
				glBindBuffer( target, cachedIt->second.back() );
		}
		else
			glBindBuffer( target, cachedIt->second.back() );
	}
}

GLuint Context::getBufferBinding( GLenum target )
{
	auto cachedIt = mBufferBindingStack.find( target );
	if( (cachedIt == mBufferBindingStack.end()) || ( cachedIt->second.empty() ) || ( cachedIt->second.back() == -1 ) ) {
		GLint queriedInt = 0;
		GLenum targetBinding = BufferObj::getBindingConstantForTarget( target );
		if( targetBinding > 0 ) {
			glGetIntegerv( targetBinding, &queriedInt );
		}
		else
			return 0; // warning?
		
		if( mBufferBindingStack[target].empty() ) { // bad - empty stack; push twice to allow for the pop later and not lead to an empty stack
			mBufferBindingStack[target] = vector<GLint>();
			mBufferBindingStack[target].push_back( queriedInt );
			mBufferBindingStack[target].push_back( queriedInt );			
		}
		else
			mBufferBindingStack[target].back() = queriedInt;
		return (GLuint)queriedInt;
	}
	else
		return (GLuint)cachedIt->second.back();
}

void Context::reflectBufferBinding( GLenum target, GLuint id )
{
	// first time we've met this target; start an empty stack
	if( mBufferBindingStack.find(target) == mBufferBindingStack.end() ) {
		mBufferBindingStack[target] = vector<int>();
		mBufferBindingStack[target].push_back( id );
	}
	else if( ! mBufferBindingStack[target].empty() )
		mBufferBindingStack[target].back() = id;
}

void Context::bufferDeleted( GLenum target, GLuint id )
{
	// if 'id' was bound to 'target', mark 'target's binding as 0
	auto existingIt = mBufferBindingStack.find( target );
	if( existingIt != mBufferBindingStack.end() ) {
		if( mBufferBindingStack[target].back() == id )
			mBufferBindingStack[target].back() = 0;
	}
	else
		mBufferBindingStack[target].push_back( 0 );
}

void Context::invalidateBufferBindingCache( GLenum target )
{
	if( mBufferBindingStack.find(target) == mBufferBindingStack.end() ) {
		mBufferBindingStack[target] = vector<int>();
		mBufferBindingStack[target].push_back( -1 );
	}
	else if( ! mBufferBindingStack[target].empty() )
		mBufferBindingStack[target].back() = -1;
}
	
#if ! defined( CINDER_GLES )
void Context::bindBufferBase( GLenum target, int index, BufferObjRef buffer )
{
	switch (target) {
		case GL_TRANSFORM_FEEDBACK_BUFFER: {
			if( mCachedXfo ) {
				mCachedXfo->setIndex( index, buffer );
			}
			else {
				glBindBufferBase( target, index, buffer->getId() );
			}
		}
		break;
		case GL_UNIFORM_BUFFER: {
			// Soon to implement
		}
		break;
		default:
		break;
	}
}
	
//////////////////////////////////////////////////////////////////
// XFO
void Context::xfoBind( XfoRef xfo )
{
	if( xfo != mCachedXfo ) {
		if( mCachedXfo )
			mCachedXfo->unbindImpl( this );
		if( xfo ) {
			xfo->bindImpl( this );
		}
		
		mCachedXfo = xfo;
	}
}

XfoRef Context::xfoGet()
{
	return mCachedXfo;
}
	
void Context::beginTransformFeedback( GLenum primitiveMode )
{
	if( mCachedXfo ) {
		mCachedXfo->begin( primitiveMode );
	}
	else {
		glBeginTransformFeedback( primitiveMode );
	}
}

void Context::pauseTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->pause();
	}
	else {
		glPauseTransformFeedback();
	}
}

void Context::resumeTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->resume();
	}
	else {
		glResumeTransformFeedback();
	}
}

void Context::endTransformFeedback()
{
	if( mCachedXfo ) {
		mCachedXfo->end();
	}
	else {
		glEndTransformFeedback();
	}
}
#endif

//////////////////////////////////////////////////////////////////
// Shader
void Context::pushGlslProg( const GlslProgRef &prog )
{
	GlslProgRef prevGlsl = getGlslProg();

	mGlslProgStack.push_back( prog );
	if( prog != prevGlsl ) {
		if( prog )
			prog->bindImpl();
		else
			glUseProgram( 0 );
	}
}

void Context::pushGlslProg()
{
	mGlslProgStack.push_back( getGlslProg() );
}

void Context::popGlslProg()
{
	GlslProgRef prevGlsl = getGlslProg();

	if( ! mGlslProgStack.empty() ) {
		mGlslProgStack.pop_back();
		if( ! mGlslProgStack.empty() ) {
			if( prevGlsl != mGlslProgStack.back() ) {
				if( mGlslProgStack.back() )
					mGlslProgStack.back()->bindImpl();
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
			prog->bindImpl();
		else
			glUseProgram( 0 );
	}
}

GlslProgRef Context::getGlslProg()
{
	if( mGlslProgStack.empty() )
		mGlslProgStack.push_back( GlslProgRef() );
	
	return mGlslProgStack.back();
}

//////////////////////////////////////////////////////////////////
// TextureBinding
void Context::bindTexture( GLenum target, GLuint textureId )
{
	GLuint prevValue = getTextureBinding( target );
	if( prevValue != textureId ) {
		mTextureBindingStack[target].back() = textureId;
		glBindTexture( target, textureId );
	}
}

void Context::pushTextureBinding( GLenum target, GLuint textureId )
{
	pushTextureBinding( target );
	bindTexture( target, textureId );
}

void Context::pushTextureBinding( GLenum target )
{
	GLuint curValue = getTextureBinding( target );
	mTextureBindingStack[target].push_back( curValue );
}

void Context::popTextureBinding( GLenum target )
{
	auto cached = mTextureBindingStack.find( target );
	if( ( cached != mTextureBindingStack.end() ) && ( ! cached->second.empty() ) ) {
		GLint prevValue = cached->second.back();
		cached->second.pop_back();
		if( ! cached->second.empty() ) {
			if( cached->second.back() != prevValue )
				glBindTexture( target, cached->second.back() );
		}
	}
}

GLuint Context::getTextureBinding( GLenum target )
{
	auto cachedIt = mTextureBindingStack.find( target );
	if( (cachedIt == mTextureBindingStack.end()) || ( cachedIt->second.empty() ) || ( cachedIt->second.back() == -1 ) ) {
		GLint queriedInt = 0;
		GLenum targetBinding = Texture::getBindingConstantForTarget( target );
		if( targetBinding > 0 ) {
			glGetIntegerv( targetBinding, &queriedInt );
		}
		else
			return 0; // warning?
		
		if( mTextureBindingStack[target].empty() ) { // bad - empty stack; push twice to allow for the pop later and not lead to an empty stack
			mTextureBindingStack[target] = vector<GLint>();
			mTextureBindingStack[target].push_back( queriedInt );
			mTextureBindingStack[target].push_back( queriedInt );			
		}
		else
			mTextureBindingStack[target].back() = queriedInt;
		return (GLuint)queriedInt;
	}
	else
		return (GLuint)cachedIt->second.back();
}

void Context::textureDeleted( GLenum target, GLuint textureId )
{
	auto cachedIt = mTextureBindingStack.find( target );
	if( cachedIt != mTextureBindingStack.end() && ( ! cachedIt->second.empty() ) && ( cachedIt->second.back() != textureId ) ) {
		// GL will have set the binding to 0 for target, so let's do the same
		mTextureBindingStack[target].back() = 0;
	}
}

//////////////////////////////////////////////////////////////////
// ActiveTexture
void Context::setActiveTexture( uint8_t textureUnit )
{
	if( setStackState<uint8_t>( mActiveTextureStack, textureUnit ) )
		glActiveTexture( GL_TEXTURE0 + textureUnit );
}

void Context::pushActiveTexture( uint8_t textureUnit )
{
	if( pushStackState<uint8_t>( mActiveTextureStack, textureUnit ) )
		glActiveTexture( GL_TEXTURE0 + textureUnit );
}

void Context::pushActiveTexture()
{
	pushStackState<uint8_t>( mActiveTextureStack, getActiveTexture() );
}

//! Sets the active texture unit; expects values relative to \c 0, \em not GL_TEXTURE0
void Context::popActiveTexture()
{
	if( mActiveTextureStack.empty() || popStackState<uint8_t>( mActiveTextureStack ) )
		glActiveTexture( GL_TEXTURE0 + getActiveTexture() );
}

uint8_t Context::getActiveTexture()
{
	if( mActiveTextureStack.empty() ) {
		GLint queriedInt;
		glGetIntegerv( GL_ACTIVE_TEXTURE, &queriedInt );
		// push twice to account for inevitable pop to follow
		mActiveTextureStack.push_back( queriedInt - GL_TEXTURE0 );
		mActiveTextureStack.push_back( queriedInt - GL_TEXTURE0 );
	}

	return mActiveTextureStack.back();
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
		bool readRequiresBind = setStackState<GLint>( mReadFramebufferStack, framebuffer );
		bool drawRequiresBind = setStackState<GLint>( mDrawFramebufferStack, framebuffer );
		if( readRequiresBind || drawRequiresBind )
			glBindFramebuffer( GL_FRAMEBUFFER, framebuffer );
	}
	else if( target == GL_READ_FRAMEBUFFER ) {
		if( setStackState<GLint>( mReadFramebufferStack, framebuffer ) )
			glBindFramebuffer( target, framebuffer );
	}
	else if( target == GL_DRAW_FRAMEBUFFER ) {
		if( setStackState<GLint>( mDrawFramebufferStack, framebuffer ) )
			glBindFramebuffer( target, framebuffer );		
	}
	else {
		//throw gl::Exception( "Illegal target for Context::bindFramebuffer" );	
	}
#endif
}

void Context::bindFramebuffer( const FboRef &fbo, GLenum target )
{
	bindFramebuffer( target, fbo->getId() );
	fbo->markAsDirty();
}

void Context::unbindFramebuffer()
{
	bindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void Context::pushFramebuffer( const FboRef &fbo, GLenum target )
{
	pushFramebuffer( target, fbo->getId() );
	fbo->markAsDirty();	
}

void Context::pushFramebuffer( GLenum target, GLuint framebuffer )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	if( pushStackState<GLint>( mFramebufferStack, framebuffer ) )
		glBindFramebuffer( target, framebuffer );
#else
	if( target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER ) {
		if( pushStackState<GLint>( mReadFramebufferStack, framebuffer ) )
			glBindFramebuffer( GL_READ_FRAMEBUFFER, framebuffer );
	}
	if( target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER ) {
		if( pushStackState<GLint>( mDrawFramebufferStack, framebuffer ) )
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebuffer );	
	}
#endif
}

void Context::pushFramebuffer( GLenum target )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	pushStackState<GLint>( mFramebufferStack, getFramebuffer( target ) );
#else
	if( target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER ) {
		pushStackState<GLint>( mReadFramebufferStack, getFramebuffer( GL_READ_FRAMEBUFFER ) );
	}
	if( target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER ) {
		pushStackState<GLint>( mDrawFramebufferStack, getFramebuffer( GL_DRAW_FRAMEBUFFER ) );
	}
#endif
}

void Context::popFramebuffer( GLenum target )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	if( popStackState<GLint>( mFramebufferStack ) )
		if( ! mFramebufferStack.empty() )
			glBindFramebuffer( target, mFramebufferStack.back() );
#else
	if( target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER ) {
		if( popStackState<GLint>( mReadFramebufferStack ) )
			if( ! mReadFramebufferStack.empty() )
				glBindFramebuffer( target, mReadFramebufferStack.back() );
	}
	if( target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER ) {
		if( popStackState<GLint>( mDrawFramebufferStack ) )
			if( ! mDrawFramebufferStack.empty() )
				glBindFramebuffer( target, mDrawFramebufferStack.back() );
	}
#endif
}

GLuint Context::getFramebuffer( GLenum target )
{
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	if( mFramebufferStack.empty() ) {
		GLint queriedInt;
		glGetIntegerv( GL_FRAMEBUFFER_BINDING, &queriedInt );
		mFramebufferStack.push_back( queriedInt );
		mFramebufferStack.push_back( queriedInt );		
	}
	
	return mFramebufferStack.back();
#else
	if( target == GL_READ_FRAMEBUFFER ) {
		if( mReadFramebufferStack.empty() ) {
			GLint queriedInt;
			glGetIntegerv( GL_READ_FRAMEBUFFER_BINDING, &queriedInt );
			// push twice for pop to follow
			mReadFramebufferStack.push_back( queriedInt );
			mReadFramebufferStack.push_back( queriedInt );
		}
		return (GLuint)mReadFramebufferStack.back();
	}
	else if( target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER ) {
		if( mDrawFramebufferStack.empty() ) {
			GLint queriedInt;
			glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &queriedInt );
			mDrawFramebufferStack.push_back( queriedInt );
			mDrawFramebufferStack.push_back( queriedInt );
		}
		return (GLuint)mDrawFramebufferStack.back();
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
	auto cached = mBoolStateStack.find( cap );
	if( ( cached != mBoolStateStack.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	if( cached == mBoolStateStack.end() ) {
		mBoolStateStack[cap] = vector<GLboolean>();
		mBoolStateStack[cap].push_back( value );
	}
	else
		mBoolStateStack[cap].back() = value;
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
	auto cached = mBoolStateStack.find( cap );
	if( ( cached != mBoolStateStack.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	if( cached == mBoolStateStack.end() ) {
		mBoolStateStack[cap] = vector<GLboolean>();
		mBoolStateStack[cap].push_back( value );
	}
	else
		mBoolStateStack[cap].back() = value;
	if( needsToBeSet )
		setter( value );
}

void Context::pushBoolState( GLenum cap, GLboolean value )
{
	bool needsToBeSet = true;
	auto cached = mBoolStateStack.find( cap );
	if( ( cached != mBoolStateStack.end() ) && ( ! cached->second.empty() ) && ( cached->second.back() == value ) )
		needsToBeSet = false;
	else if( cached == mBoolStateStack.end() ) {
		mBoolStateStack[cap] = vector<GLboolean>();
		mBoolStateStack[cap].push_back( glIsEnabled( cap ) );
	}
	mBoolStateStack[cap].push_back( value );
	if( needsToBeSet ) {
		if( value )
			glEnable( cap );
		else
			glDisable( cap );
	}	
}

void Context::pushBoolState( GLenum cap )
{
	bool existingVal = getBoolState( cap );
	mBoolStateStack[cap].push_back( existingVal );
}

void Context::popBoolState( GLenum cap )
{
	auto cached = mBoolStateStack.find( cap );
	if( ( cached != mBoolStateStack.end() ) && ( ! cached->second.empty() ) ) {
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
	auto cached = mBoolStateStack.find( cap );
	if( ( cached == mBoolStateStack.end() ) || cached->second.empty() ) {
		GLboolean result = glIsEnabled( cap );
		if( cached == mBoolStateStack.end() )
			mBoolStateStack[cap] = vector<GLboolean>();
		// push twice to accommodate later pop
		mBoolStateStack[cap].push_back( result );
		mBoolStateStack[cap].push_back( result );
		return result;
	}
	else
		return cached->second.back();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// BlendFunc
void Context::blendFunc( GLenum sfactor, GLenum dfactor )
{
	blendFuncSeparate( sfactor, dfactor, sfactor, dfactor );
}

void Context::blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	bool needsChange = setStackState<GLint>( mBlendSrcRgbStack, srcRGB );
	needsChange = setStackState<GLint>( mBlendDstRgbStack, dstRGB ) || needsChange;
	needsChange = setStackState<GLint>( mBlendSrcAlphaStack, srcAlpha ) || needsChange;
	needsChange = setStackState<GLint>( mBlendDstAlphaStack, dstAlpha ) || needsChange;
	if( needsChange )
		glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

void Context::pushBlendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha )
{
	bool needsChange = pushStackState<GLint>( mBlendSrcRgbStack, srcRGB );
	needsChange = pushStackState<GLint>( mBlendDstRgbStack, dstRGB ) || needsChange;
	needsChange = pushStackState<GLint>( mBlendSrcAlphaStack, srcAlpha ) || needsChange;
	needsChange = pushStackState<GLint>( mBlendDstAlphaStack, dstAlpha ) || needsChange;
	if( needsChange )
		glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha );
}

void Context::pushBlendFuncSeparate()
{
	GLenum resultSrcRGB, resultDstRGB, resultSrcAlpha, resultDstAlpha;
	getBlendFuncSeparate( &resultSrcRGB, &resultDstRGB, &resultSrcAlpha, &resultDstAlpha );

	mBlendSrcRgbStack.push_back( resultSrcRGB );
	mBlendDstRgbStack.push_back( resultDstRGB );
	mBlendSrcAlphaStack.push_back( resultSrcAlpha );
	mBlendDstAlphaStack.push_back( resultDstAlpha );
}

void Context::popBlendFuncSeparate()
{
	bool needsChange = popStackState<GLint>( mBlendSrcRgbStack );
	needsChange = popStackState<GLint>( mBlendDstRgbStack ) || needsChange;
	needsChange = popStackState<GLint>( mBlendSrcAlphaStack ) || needsChange;
	needsChange = popStackState<GLint>( mBlendDstAlphaStack ) || needsChange;
	if( needsChange && ( ! mBlendSrcRgbStack.empty() ) && ( ! mBlendSrcAlphaStack.empty() ) && ( ! mBlendDstRgbStack.empty() ) && ( ! mBlendDstAlphaStack.empty() ) )
		glBlendFuncSeparate( mBlendSrcRgbStack.back(), mBlendDstRgbStack.back(), mBlendSrcAlphaStack.back(), mBlendDstAlphaStack.back() );
}

void Context::getBlendFuncSeparate( GLenum *resultSrcRGB, GLenum *resultDstRGB, GLenum *resultSrcAlpha, GLenum *resultDstAlpha )
{
	// push twice on empty to accommodate inevitable push later
	GLint queriedInt;
	if( mBlendSrcRgbStack.empty() ) {
		glGetIntegerv( GL_BLEND_SRC_RGB, &queriedInt );
		mBlendSrcRgbStack.push_back( queriedInt ); mBlendSrcRgbStack.push_back( queriedInt );
	}
	if( mBlendDstRgbStack.empty() ) {
		glGetIntegerv( GL_BLEND_DST_RGB, &queriedInt );
		mBlendDstRgbStack.push_back( queriedInt ); mBlendDstRgbStack.push_back( queriedInt );
	}
	if( mBlendSrcAlphaStack.empty() ) {
		glGetIntegerv( GL_BLEND_SRC_ALPHA, &queriedInt );
		mBlendSrcAlphaStack.push_back( queriedInt ); mBlendSrcAlphaStack.push_back( queriedInt );
	}
	if( mBlendDstAlphaStack.empty() ) {
		glGetIntegerv( GL_BLEND_DST_ALPHA, &queriedInt );
		mBlendDstAlphaStack.push_back( queriedInt ); mBlendDstAlphaStack.push_back( queriedInt );
	}
	
	*resultSrcRGB = mBlendSrcRgbStack.back();
	*resultDstRGB = mBlendDstRgbStack.back();
	*resultSrcAlpha = mBlendSrcAlphaStack.back();
	*resultDstAlpha = mBlendDstAlphaStack.back();
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

#endif // ! defined( CINDER_GLES )

//////////////////////////////////////////////////////////////////////////////////////////
// Templated stack management routines
template<typename T>
bool Context::pushStackState( std::vector<T> &stack, T value )
{
	bool needsToBeSet = true;
	if( ( ! stack.empty() ) && ( stack.back() == value ) )
		needsToBeSet = false;
	stack.push_back( value );
	return needsToBeSet;
}

template<typename T>
bool Context::popStackState( std::vector<T> &stack )
{
	if( ! stack.empty() ) {
		T prevValue = stack.back();
		stack.pop_back();
		if( ! stack.empty() )
			return stack.back() != prevValue;
		else
			return true;
	}
	else
		return true;
}

template<typename T>
bool Context::setStackState( std::vector<T> &stack, T value )
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

template<typename T>
bool Context::getStackState( std::vector<T> &stack, T *result )
{
	if( stack.empty() )
		return false;
	else {
		*result = stack.back();
		return true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// This routine confirms that the ci::gl::Context's understanding of the world matches OpenGL's
void Context::sanityCheck()
{
	GLint queriedInt;

	// assert cached (VAO) GL_VERTEX_ARRAY_BINDING is correct
	GLint trueVaoBinding;
#if defined( CINDER_GLES )
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING_OES, &trueVaoBinding );
#else
	glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &trueVaoBinding );
#endif

	VaoRef boundVao = getVao();
	if( boundVao ) {
		assert( trueVaoBinding == boundVao->mId );
		assert( getBufferBinding( GL_ARRAY_BUFFER ) == boundVao->getLayout().mCachedArrayBufferBinding );
		assert( getBufferBinding( GL_ELEMENT_ARRAY_BUFFER ) == boundVao->getLayout().mElementArrayBufferBinding );		
	}
	else
		assert( trueVaoBinding == 0 );

	// assert cached GL_ARRAY_BUFFER is correct
	GLint cachedArrayBufferBinding = getBufferBinding( GL_ARRAY_BUFFER );
	GLint trueArrayBufferBinding;
	glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &trueArrayBufferBinding );
	assert( ( cachedArrayBufferBinding == -1 ) || ( trueArrayBufferBinding == cachedArrayBufferBinding ) );

	// assert cached GL_ELEMENT_ARRAY_BUFFER is correct
	GLint cachedElementArrayBufferBinding = getBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
	GLint trueElementArrayBufferBinding;
	glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &trueElementArrayBufferBinding );
	assert( ( cachedElementArrayBufferBinding == -1 ) || ( trueElementArrayBufferBinding == cachedElementArrayBufferBinding ) );

	// assert the various texture bindings are correct
/*	for( auto& cachedTextureBinding : mTextureBindingStack ) {
		GLenum target = cachedTextureBinding.first;
		glGetIntegerv( Texture::getBindingConstantForTarget( target ), &queriedInt );
		GLenum cachedTextureId = cachedTextureBinding.second.back();
		assert( queriedInt == cachedTextureId );
	}*/
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

void Context::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	VaoRef vao = getVao();
	if( vao )
		vao->vertexAttribPointerImpl( index, size, type, normalized, stride, pointer );
}

void Context::vertexAttribDivisor( GLuint index, GLuint divisor )
{
	VaoRef vao = getVao();
	if( vao )
		vao->vertexAttribDivisorImpl( index, divisor );
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

#if ! defined( CINDER_GLES )
void Context::drawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei primcount )
{
	glDrawArraysInstanced( mode, first, count, primcount );
}

void Context::drawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount )
{
	glDrawElementsInstanced( mode, count, type, indices, primcount );
}
#endif // ! defined( CINDER_GLES )

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
	const auto &ctx = gl::context();
	const auto &glslProg = ctx->getGlslProg();
	if( glslProg ) {
		const auto &uniforms = glslProg->getUniformSemantics();
		for( const auto &unifIt : uniforms ) {
			switch( unifIt.second ) {
				case UNIFORM_MODELVIEW:
					glslProg->uniform( unifIt.first, gl::getModelView() ); break;
				case UNIFORM_MODELVIEWPROJECTION:
					glslProg->uniform( unifIt.first, gl::getModelViewProjection() ); break;
				case UNIFORM_PROJECTION:
					glslProg->uniform( unifIt.first, gl::getProjection() ); break;
				case UNIFORM_NORMAL_MATRIX:
					glslProg->uniform( unifIt.first, gl::calcNormalMatrix() ); break;
			}
		}

		const auto &attribs = glslProg->getAttribSemantics();
		for( const auto &attribIt : attribs ) {
			switch( attribIt.second ) {
				case geom::Attrib::COLOR: {
					int loc = glslProg->getAttribLocation( attribIt.first );
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
	mDefaultArrayVboIdx = ( mDefaultArrayVboIdx + 1 ) % 4;

	if( ! mDefaultArrayVbo[mDefaultArrayVboIdx] ) {
		mDefaultArrayVbo[mDefaultArrayVboIdx] = Vbo::create( GL_ARRAY_BUFFER, std::max<size_t>( 1, requiredSize ), NULL, GL_STREAM_DRAW );
	}
	else if( requiredSize > mDefaultArrayVbo[mDefaultArrayVboIdx]->getSize() ) {
		mDefaultArrayVbo[mDefaultArrayVboIdx]->ensureMinimumSize( std::max<size_t>( 1, requiredSize ) );
	}

	return mDefaultArrayVbo[mDefaultArrayVboIdx];
}

VboRef Context::getDefaultElementVbo( size_t requiredSize )
{
	if( ! mDefaultElementVbo ) {
		mDefaultElementVbo = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, requiredSize, NULL, GL_STREAM_DRAW );
	}
	if( requiredSize > mDefaultElementVbo->getSize() ) {
		mDefaultElementVbo = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, requiredSize, NULL, GL_STREAM_DRAW );
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
FramebufferScope::FramebufferScope( const FboRef &fbo, GLenum target )
	: mCtx( gl::context() ), mTarget( target )
{
	mCtx->pushFramebuffer( fbo, target );
}

FramebufferScope::FramebufferScope( GLenum target, GLuint framebufferId )
	: mCtx( gl::context() ), mTarget( target )
{
	mCtx->pushFramebuffer( target, framebufferId );
}

FramebufferScope::~FramebufferScope()
{	
#if ! defined( SUPPORTS_FBO_MULTISAMPLING )
	mCtx->bindFramebuffer( GL_FRAMEBUFFER, mPrevFramebuffer );
#else
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_READ_FRAMEBUFFER )
		mCtx->popFramebuffer( GL_READ_FRAMEBUFFER );
	if( mTarget == GL_FRAMEBUFFER || mTarget == GL_DRAW_FRAMEBUFFER )
		mCtx->popFramebuffer( GL_DRAW_FRAMEBUFFER );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////

} } // namespace cinder::gl
