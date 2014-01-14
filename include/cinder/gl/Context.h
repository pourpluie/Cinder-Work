#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/Matrix44.h"
#include "cinder/Vector.h"
#include "cinder/gl/Shader.h"

#include <boost/noncopyable.hpp>
#include <vector>
#include <map>

namespace cinder { namespace gl {

class Context;
typedef std::shared_ptr<Context>		ContextRef;
class Vbo;
typedef std::shared_ptr<Vbo>			VboRef;
class Vao;
typedef std::shared_ptr<Vao>			VaoRef;
class BufferObj;
typedef std::shared_ptr<BufferObj>		BufferObjRef;
#if ! defined( CINDER_GLES )
class Xfo;
typedef std::shared_ptr<Xfo>			XfoRef;
#endif
class Texture;
typedef std::shared_ptr<Texture>		TextureRef;
class GlslProg;
typedef std::shared_ptr<GlslProg>		GlslProgRef;
class Fbo;
typedef std::shared_ptr<Fbo>			FboRef;
class VertBatch;
typedef std::shared_ptr<VertBatch>		VertBatchRef;

class Context {
  public:
	struct PlatformData {
		virtual ~PlatformData() {}
	};

	//! Creates a new OpenGL context, sharing resources and pixel format with sharedContext. This (essentially) must be done from the primary thread on MSW. ANGLE doesn't support multithreaded use. Destroys the platform Context on destruction.
	static ContextRef	create( const Context *sharedContext );	
	//! Creates based on an existing platform-specific GL context. \a platformContext is CGLContextObj on Mac OS X, EAGLContext on iOS, HGLRC on MSW. \a platformContext is an HDC on MSW and ignored elsewhere. Does not assume ownership of the platform's context.
	static ContextRef	createFromExisting( const std::shared_ptr<PlatformData> &platformData );	

	~Context();

	//! Returns the platform-specific OpenGL Context. CGLContextObj on Mac OS X, EAGLContext on iOS
	const std::shared_ptr<PlatformData>		getPlatformData() const { return mPlatformData; }

	//! Makes this the currently active OpenGL Context
	void			makeCurrent() const;
	//! Returns the thread's currently active OpenGL Context
	static Context*	getCurrent();

	//! Returns a reference to the stack of ModelView matrices
	std::vector<Matrix44f>&	getModelViewStack() { return mModelViewStack; }
	//! Returns a const reference to the stack of ModelView matrices
	const std::vector<Matrix44f>&	getModelViewStack() const { return mModelViewStack; }
	//! Returns a reference to the stack of Projection matrices
	std::vector<Matrix44f>&			getProjectionStack() { return mProjectionStack; }
	//! Returns a const reference to the stack of Projection matrices
	const std::vector<Matrix44f>&	getProjectionStack() const { return mProjectionStack; }
	
	//! Binds a VAO. Consider using a VaoScope instead.
	void		bindVao( const VaoRef &vao );
	//! Pushes and binds the VAO \a vao
	void		pushVao( const VaoRef &vao );
	//! Duplicates and pushes the current VAO binding 
	void		pushVao();
	//! Pops the current VAO binding
	void		popVao();
	//! Returns the currently bound VAO
	VaoRef		getVao();

	//! Analogous to glViewport(). Sets the viewport based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively
	void					viewport( const std::pair<Vec2i, Vec2i> &viewport );
	//! Pushes the viewport based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively
	void					pushViewport( const std::pair<Vec2i, Vec2i> &viewport );
	//! Duplicates and pushes the top of the Viewport stack
	void					pushViewport();
	//! Pops the viewport
	void					popViewport();
	//! Returns a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively of the viewport
	std::pair<Vec2i, Vec2i>	getViewport();

	//! Sets the scissor box based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively	
	void					setScissor( const std::pair<Vec2i, Vec2i> &scissor );
	//! Pushes the scissor box based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively	
	void					pushScissor( const std::pair<Vec2i, Vec2i> &scissor );
	//! Duplicates and pushes the top of the Scissor box stack
	void					pushScissor();
	//! Pushes the scissor box based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively	
	void					popScissor();
	//! Returns a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively of the scissor box
	std::pair<Vec2i, Vec2i>	getScissor();

	//! Analogous to glBindBuffer( \a target, \a id )
	void		bindBuffer( GLenum target, GLuint id );
	//! Pushes and binds buffer object \a id for the target \a target
	void		pushBufferBinding( GLenum target, GLuint id );
	//! Duplicates and pushes the buffer binding for the target \a target
	void		pushBufferBinding( GLenum target );
	//! Pops the buffer binding for the target \a target
	void		popBufferBinding( GLenum target );
	//! Returns the current object binding for \a target. If not cached, queries the GL for the current value (and caches it).
	GLuint		getBufferBinding( GLenum target );
	//! Updates the binding stack without rebinding. Not generally necessary to call directly.
	void		reflectBufferBinding( GLenum target, GLuint id );
	//! No-op if \a id wasn't bound to \a target, otherwise reflects the binding as 0 (in accordance with what GL has done automatically)
	void		bufferDeleted( GLenum target, GLuint id );
	//! Marks the cache of \a target's buffer binding as invalid. Typically called when a VAO is unbound, against GL_ELEMENT_ARRAY_BUFFER.
	void		invalidateBufferBindingCache( GLenum target );

	//! Binds GLSL program \a prog. Analogous to glUseProgram()
	void			bindGlslProg( const GlslProgRef &prog );
	//! Pushes and binds GLSL program \a prog.
	void			pushGlslProg( const GlslProgRef &prog );
	//! Duplicates and pushes the top of the GlslProg stack.
	void			pushGlslProg();
	//! Pops the GlslProg stack
	void			popGlslProg();
	//! Returns the currently bound GlslProg
	GlslProgRef		getGlslProg();
	
#if ! defined( CINDER_GLES )
	void bindBufferBase( GLenum target, int index, BufferObjRef ref );

	void xfoBind( XfoRef xfo );
	void beginTransformFeedback( GLenum primitiveMode );
	void pauseTransformFeedback();
	void resumeTransformFeedback();
	void endTransformFeedback();
	XfoRef xfoGet();
#endif

	//! Analogous to glBindTexture( \a target, \a textureId )
	void		bindTexture( GLenum target, GLuint textureId );
	//! Pushes and binds \a textureId for the target \a target
	void		pushTextureBinding( GLenum target, GLuint textureId );
	//! Duplicates and pushes the texture binding for the target \a target
	void		pushTextureBinding( GLenum target );
	//! Pops the texture binding for the target \a target
	void		popTextureBinding( GLenum target );	
	//! Returns the current texture binding for \a target. If not cached, queries the GL for the current value (and caches it).
	GLuint		getTextureBinding( GLenum target );
	//! No-op if texture wasn't bound to target, otherwise reflects the binding as 0 (in accordance with what GL has done automatically)
	void		textureDeleted( GLenum target, GLuint textureId );

	//! Sets the active texture unit; expects values relative to \c 0, \em not GL_TEXTURE0
	void		setActiveTexture( uint8_t textureUnit );
	//! Pushes and sets the active texture unit; expects values relative to \c 0, \em not GL_TEXTURE0
	void		pushActiveTexture( uint8_t textureUnit );
	//! Duplicates and pushes the active texture unit
	void		pushActiveTexture();
	//! Sets the active texture unit; expects values relative to \c 0, \em not GL_TEXTURE0
	void		popActiveTexture();	
	//! Returns the active texture unit with values relative to \c 0, \em not GL_TEXTURE0
	uint8_t		getActiveTexture();

	//! Analogous to glBindFramebuffer()
	void		bindFramebuffer( const FboRef &fbo, GLenum target = GL_FRAMEBUFFER );
	//! Analogous to glBindFramebuffer(). Prefer the FboRef variant when possible. This does not allow gl::Fbo to mark itself as needing multisample resolution.
	void		bindFramebuffer( GLenum target, GLuint framebuffer );
	//! Pushes and sets the active framebuffer.
	void		pushFramebuffer( const FboRef &fbo, GLenum target = GL_FRAMEBUFFER );
	//! Pushes and sets the active framebuffer. Prefer the FboRef variant when possible. This does not allow gl::Fbo to mark itself as needing multisample resolution.
	void		pushFramebuffer( GLenum target, GLuint framebuffer = GL_FRAMEBUFFER );
	//! Duplicates and pushes the active framebuffer.
	void		pushFramebuffer( GLenum target = GL_FRAMEBUFFER );
	//! Pops the active framebuffer
	void		popFramebuffer( GLuint framebuffer = GL_FRAMEBUFFER );
	//! Unbinds the current FBO (binding the default (screen) framebuffer)
	void		unbindFramebuffer();
	//! Returns the ID of the framebuffer currently bound to \a target
	GLuint		getFramebuffer( GLenum target = GL_FRAMEBUFFER );

	//! Analogous to glEnable() or glDisable(). Enables or disables OpenGL capability \a cap
	void		setBoolState( GLenum cap, GLboolean value );
	//! Pushes and sets the state stack for OpenGL capability \a cap to \a value.
	void		pushBoolState( GLenum cap, GLboolean value );
	//! Duplicates and pushes the state stack for OpenGL capability \a cap
	void		pushBoolState( GLenum cap );
	//! Pops the state stack for OpenGL capability \a cap
	void		popBoolState( GLenum cap );
	//! Synonym for setBoolState(). Enables or disables OpenGL capability \a cap.
	void		enable( GLenum cap, GLboolean value = true );
	//! Analogous to glIsEnabled(). Returns whether a given OpenGL capability is enabled or not
	GLboolean	getBoolState( GLenum cap );
	//! Enables or disables OpenGL capability \a cap. Calls \a setter rather than glEnable or glDisable. Not generally necessary to call directly.
	void		setBoolState( GLenum cap, GLboolean value, const std::function<void(GLboolean)> &setter );

	//! Analogous glBlendFunc(). Consider using a BlendScope instead.
	void		blendFunc( GLenum sfactor, GLenum dfactor );
	//! Analogous to glBlendFuncSeparate(). Consider using a BlendScope instead.
	void		blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
	//! Analogous to glBlendFuncSeparate, but pushes values rather than replaces them
	void		pushBlendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
	//! Duplicates and pushes the glBlendFunc state stack.
	void		pushBlendFuncSeparate();
	//! Analogous to glBlendFuncSeparate, but pushes values rather than replaces them
	void		popBlendFuncSeparate();
	//! Returns the current values for glBendFuncs
	void		getBlendFuncSeparate( GLenum *resultSrcRGB, GLenum *resultDstRGB, GLenum *resultSrcAlpha, GLenum *resultDstAlpha );

	//! Analogous to glDepthMask()
	void		depthMask( GLboolean enable );

#if ! defined( CINDER_GLES )
	//! Parallels glPolygonMode()
	void		polygonMode( GLenum face, GLenum mode );
#endif
	
	void		sanityCheck();
	void		printState( std::ostream &os ) const;

	// Vertex Attributes
	//! Analogous to glEnableVertexAttribArray()
	void		enableVertexAttribArray( GLuint index );
	//! Analogous to glDisableVertexAttribArray()
	void		disableVertexAttribArray( GLuint index );
	//! Analogous to glVertexAttribPointer()
	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	//! Analogous to glVertexAttribDivisor()
	void		vertexAttribDivisor( GLuint index, GLuint divisor );
	//! Analogous to glVertexAttrib1f()
	void		vertexAttrib1f( GLuint index, float v0 );
	//! Analogous to glVertexAttrib2f()
	void		vertexAttrib2f( GLuint index, float v0, float v1 );
	//! Analogous to glVertexAttrib3f()
	void		vertexAttrib3f( GLuint index, float v0, float v1, float v2 );
	//! Analogous to glVertexAttrib4f()
	void		vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 );

	//! Analogous to glDrawArrays()
	void		drawArrays( GLenum mode, GLint first, GLsizei count );
	//! Analogous to glDrawElements()
	void		drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
#if ! defined( CINDER_GLES )
	//! Analogous to glDrawArraysInstanced()
	void		drawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
	//! Analogous to glDrawElementsInstanced()
	void		drawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount );
#endif // ! defined( CINDER_GLES )

	//! Returns the current active color, used in immediate-mode emulation and as UNIFORM_COLOR
	const ColorAf&	getCurrentColor() const { return mColor; }
	void			setCurrentColor( const ColorAf &color ) { mColor = color; }
	GlslProgRef		getStockShader( const ShaderDef &shaderDef );
	void			setDefaultShaderVars();

	//! Returns default VBO for vertex array data, ensuring it is at least \a requiredSize bytes. Designed for use with convenience functions.
	VboRef			getDefaultArrayVbo( size_t requiredSize = 0 );
	//! Returns default VBO for element array data, ensuring it is at least \a requiredSize bytes. Designed for use with convenience functions.
	VboRef			getDefaultElementVbo( size_t requiredSize = 0 );
	//! Returns default VAO, designed for use with convenience functions.
	VaoRef			getDefaultVao();

	//! Returns a reference to the immediate mode emulation structure. Generally use gl::begin() and friends instead.
	VertBatch&		immediate() { return *mImmediateMode; }

  protected:
	//! Returns \c true if \a value is different from the previous top of the stack
	template<typename T>
	bool		pushStackState( std::vector<T> &stack, T value );
	//! Returns \c true if the new top of \a stack is different from the previous top, or the stack is empty
	template<typename T>
	bool		popStackState( std::vector<T> &stack );
	//! Returns \c true if \a value is different from the previous top of the stack
	template<typename T>
	bool		setStackState( std::vector<T> &stack, T value );
	//! Returns \c true if \a result is valid; will return \c false when \a stack was empty
	template<typename T>
	bool		getStackState( std::vector<T> &stack, T *result );

	std::map<ShaderDef,GlslProgRef>		mStockShaders;
	
	std::map<GLenum,std::vector<int>>	mBufferBindingStack;
	std::vector<GlslProgRef>			mGlslProgStack;
	std::vector<VaoRef>					mVaoStack;
	
#if ! defined( CINDER_GLES )
	XfoRef								mCachedXfo;
#endif
	
	// Blend state stacks
	std::vector<GLint>					mBlendSrcRgbStack, mBlendDstRgbStack;
	std::vector<GLint>					mBlendSrcAlphaStack, mBlendDstAlphaStack;

#if defined( CINDER_GLES ) && (! defined( CINDER_COCOA_TOUCH ))
	std::vector<GLint>			mFramebufferStack;
#else
	std::vector<GLint>			mReadFramebufferStack, mDrawFramebufferStack;
#endif
	
	std::map<GLenum,std::vector<GLboolean>>	mBoolStateStack;
	std::map<GLenum,std::vector<GLint>>		mTextureBindingStack;
	std::vector<uint8_t>					mActiveTextureStack;
	GLenum						mCachedFrontPolygonMode, mCachedBackPolygonMode;
	
	VaoRef						mDefaultVao;
	VboRef						mDefaultArrayVbo[4], mDefaultElementVbo;
	uint8_t						mDefaultArrayVboIdx;
	VertBatchRef				mImmediateMode;
	
  private:
	Context( const std::shared_ptr<PlatformData> &platformData );
  
	std::shared_ptr<PlatformData>	mPlatformData;
	
	std::vector<std::pair<Vec2i,Vec2i>>		mViewportStack;
	std::vector<std::pair<Vec2i,Vec2i>>		mScissorStack;

	VaoRef						mImmVao; // Immediate-mode VAO
	VboRef						mImmVbo; // Immediate-mode VBO
	
	ci::ColorAf					mColor;	
	std::vector<Matrix44f>		mModelViewStack;
	std::vector<Matrix44f>		mProjectionStack;

	friend class				Environment;
	friend class				EnvironmentEs2Profile;
	friend class				EnvironmentCoreProfile;
	friend class				EnvironmentCompatibilityProfile;
	
	friend class				Texture;
};


struct VaoScope : public boost::noncopyable {
	VaoScope( const VaoRef &vao )
		: mCtx( gl::context() )
	{
		mCtx->pushVao( vao );
	}
	
	~VaoScope() {
		mCtx->popVao();
	}
  private:
	Context		*mCtx;
};

struct BufferScope : public boost::noncopyable {
	BufferScope( const BufferObjRef &bufferObj );
	BufferScope( GLenum target, GLuint id )
		: mCtx( gl::context() ), mTarget( target )
	{
		mCtx->pushBufferBinding( target, id );
	}

	~BufferScope() {
		mCtx->popBufferBinding( mTarget );
	}
  private:
	Context		*mCtx;
	GLenum		mTarget;
};

struct StateScope : public boost::noncopyable {
	StateScope( GLenum cap, GLboolean value )
		: mCtx( gl::context() ), mCap( cap )
	{
		mCtx->pushBoolState( cap, value );
	}

	~StateScope() {
		mCtx->popBoolState( mCap );
	}
  private:
	Context		*mCtx;
	GLenum		mCap;
};

struct BlendScope : public boost::noncopyable
{
	BlendScope( GLboolean enable );
	//! Parallels glBlendFunc(), and implicitly enables blending
	BlendScope( GLenum sfactor, GLenum dfactor );
	//! Parallels glBlendFuncSeparate(), and implicitly enables blending
	BlendScope( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
	~BlendScope();
	
  private:
	Context		*mCtx;
	bool		mSaveFactors; // whether we should also set the blend factors rather than just the blend state
};

struct GlslProgScope : public boost::noncopyable
{
	GlslProgScope( const GlslProgRef &prog )
		: mCtx( gl::context() )
	{
		mCtx->pushGlslProg( prog );
	}

	// this is for convenience
	GlslProgScope( const std::shared_ptr<const GlslProg> &prog )
		: mCtx( gl::context() )
	{
		mCtx->pushGlslProg( std::const_pointer_cast<GlslProg>( prog ) );
	}

	~GlslProgScope()
	{
		mCtx->popGlslProg();
	}

  private:
	Context		*mCtx;
};

struct FramebufferScope : public boost::noncopyable
{
	FramebufferScope( const FboRef &fbo, GLenum target = GL_FRAMEBUFFER );
	//! Prefer the FboRef variant when possible. This does not allow gl::Fbo to mark itself as needing multisample resolution.
	FramebufferScope( GLenum target, GLuint framebufferId );
	~FramebufferScope();
	
  private:
	Context		*mCtx;
	GLenum		mTarget;
};

struct ActiveTextureScope : public boost::noncopyable
{
	//! Sets the currently active texture through glActiveTexture. Expects values relative to \c 0, \em not GL_TEXTURE0
	ActiveTextureScope( uint8_t textureUnit )
		: mCtx( gl::context() )
	{
		mCtx->pushActiveTexture( textureUnit );
	}
	
	~ActiveTextureScope()
	{
		mCtx->popActiveTexture();
	}
	
  private:
	Context		*mCtx;
};

struct TextureBindScope : public boost::noncopyable
{
	TextureBindScope( GLenum target, GLuint textureId )
		: mCtx( gl::context() ), mTarget( target )
	{
		mCtx->pushTextureBinding( mTarget, textureId );
	}

	TextureBindScope( const TextureBaseRef &texture )
		: mCtx( gl::context() ), mTarget( texture->getTarget() )
	{
		mCtx->pushTextureBinding( mTarget, texture->getId() );
	}
	
	~TextureBindScope()
	{
		mCtx->popTextureBinding( mTarget );
	}
	
  private:
	Context		*mCtx;
	GLenum		mTarget;
};
	
struct ScissorScope : public boost::noncopyable
{
	//! Implicitly enables scissor test
	ScissorScope( const Vec2i &lowerLeftPostion, const Vec2i &dimension )
		: mCtx( gl::context() )
	{
		mCtx->pushBoolState( GL_SCISSOR_TEST, GL_TRUE );
		mCtx->pushScissor( std::pair<Vec2i, Vec2i>( lowerLeftPostion, dimension ) ); 
	}

	//! Implicitly enables scissor test	
	ScissorScope( int lowerLeftX, int lowerLeftY, int width, int height )
		: mCtx( gl::context() )
	{
		mCtx->pushBoolState( GL_SCISSOR_TEST, GL_TRUE );
		mCtx->pushScissor( std::pair<Vec2i, Vec2i>( Vec2i( lowerLeftX, lowerLeftY ), Vec2i( width, height ) ) );		
	}
	
	~ScissorScope()
	{
		mCtx->popBoolState( GL_SCISSOR_TEST );
		mCtx->popScissor();
	}
	
  private:
	Context					*mCtx;
};

struct ViewportScope : public boost::noncopyable
{
	ViewportScope( const Vec2i &lowerLeftPostion, const Vec2i &dimension )
		: mCtx( gl::context() )
	{
		mCtx->pushViewport( std::pair<Vec2i, Vec2i>( lowerLeftPostion, dimension ) ); 
	}

	ViewportScope( int lowerLeftX, int lowerLeftY, int width, int height )
		: mCtx( gl::context() )
	{
		mCtx->pushViewport( std::pair<Vec2i, Vec2i>( Vec2i( lowerLeftX, lowerLeftY ), Vec2i( width, height ) ) );		
	}
	
	~ViewportScope()
	{
		mCtx->popViewport();
	}
	
  private:
	Context					*mCtx;
};

struct ModelViewScope : public boost::noncopyable {
	ModelViewScope()	{ gl::pushModelView(); }
	~ModelViewScope()	{ gl::popModelView(); }
};

struct ProjectionScope : public boost::noncopyable {
	ProjectionScope()	{ gl::pushProjection(); }
	~ProjectionScope()	{ gl::popProjection(); }
};

struct MatricesScope : public boost::noncopyable {
	MatricesScope()		{ gl::pushMatrices(); }
	~MatricesScope()	{ gl::popMatrices(); }
};

class ExcContextAllocation : public Exception {
};

} } // namespace cinder::gl
