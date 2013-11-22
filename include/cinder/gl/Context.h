#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/Matrix44.h"
#include "cinder/Vector.h"
#include "cinder/gl/Fog.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
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
	void		vaoBind( const VaoRef &vao );
	//! Returns the currently bound VAO
	VaoRef		vaoGet();
	
	//! Returns a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively of the viewport
	const std::pair<Vec2i, Vec2i>	getViewport() const { return mViewport; }
	//! Sets the viewport based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively
	void							setViewport( const std::pair<Vec2i, Vec2i> &viewport );

	//! Returns a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively of the scissor box
	const std::pair<Vec2i, Vec2i>	getScissor() const { return mScissor; }
	//! Sets the scissor box based on a pair<Vec2i,Vec2i> representing the position of the lower-left corner and the size, respectively	
	void							setScissor( const std::pair<Vec2i, Vec2i> &scissor );

	void		bindBuffer( GLenum target, GLuint id );
	GLuint		getBufferBinding( GLenum target );
	//! Marks the Context's cache of the binding for \a target as invalid
	void		invalidateBufferBinding( GLenum target );

	void		bindShader( const GlslProgRef &prog );
	//! Sets the current shader to 'none'
	void		unbindShader();
	GlslProgRef	getCurrentShader();

	void		bindTexture( GLenum target, GLuint texture );
	//! No-op if texture wasn't bound to target, otherwise reflects the binding as 0 (in accordance with what GL has done)
	void		textureDeleted( GLenum target, GLuint textureId );
	GLuint		getTextureBinding( GLenum target );

	//! Sets the active texture unit; expects values relative to 0, \em not GL_TEXTURE0
	void		activeTexture( uint8_t textureUnit );
	//! Returns the active texture unit with values relative to 0, \em not GL_TEXTURE0
	uint8_t		getActiveTexture();

	void		bindFramebuffer( const FboRef &fbo );
	//! Prefer the FboRef variant when possible. This does not allow gl::Fbo to mark itself as needing multisample resolution.
	void		bindFramebuffer( GLenum target, GLuint framebuffer );
	void		unbindFramebuffer();
	GLuint		getFramebufferBinding( GLenum target );

	template<typename T>
	void			stateSet( GLenum cap, T value );
	void			enable( GLenum cap, GLboolean value = true );
	template<typename T>
	T				stateGet( GLenum cap );
	
	void		sanityCheck();
	void		printState( std::ostream &os ) const;

	// Vertex Attributes
	//! Analogous to glVertexAttribPointer()
	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	//! Analogous to glEnableVertexAttribArray()
	void		enableVertexAttribArray( GLuint index );
	//! Analogous to glVertexAttrib1f()
	void		vertexAttrib1f( GLuint index, float v0 );
	//! Analogous to glVertexAttrib2f()
	void		vertexAttrib2f( GLuint index, float v0, float v1 );
	//! Analogous to glVertexAttrib3f()
	void		vertexAttrib3f( GLuint index, float v0, float v1, float v2 );
	//! Analogous to glVertexAttrib4f()
	void		vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 );

	//! Analogous glBlendFunc(). Consider using a BlendScope instead.
	void		blendFunc( GLenum sfactor, GLenum dfactor );
	//! Analogous to glBlendFuncSeparate(). Consider using a BlendScope instead.
	void		blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );

	//! Analogous to glDepthMask()
	void		depthMask( GLboolean enable );

#if ! defined( CINDER_GLES )
	//! Parallels glPolygonMode()
	void		polygonMode( GLenum face, GLenum mode );
#endif

	void		drawArrays( GLenum mode, GLint first, GLsizei count );
	void		drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

	//! Returns the current active color, used in immediate-mode emulation and as UNIFORM_COLOR
	const ColorAf&	getCurrentColor() const { return mColor; }
	void			setCurrentColor( const ColorAf &color ) { mColor = color; }
	GlslProgRef		getStockShader( const ShaderDef &shaderDef );
	void			setDefaultShaderVars();

	VboRef			getDefaultArrayVbo( size_t requiredSize );
	VboRef			getDefaultElementVbo( size_t requiredSize );

	//! Returns a reference to the immediate mode emulation structure. Generally use gl::begin() and friends instead.
	VertBatch&		immediate() { return *mImmediateMode; }

  protected:
	std::map<ShaderDef,GlslProgRef>		mStockShaders;
	
	VaoRef						mCachedVao;
	std::map<GLenum,int>		mCachedBuffer;
	GlslProgRef					mCachedGlslProg;
	
#if defined( CINDER_GLES ) && (! defined( CINDER_COCOA_TOUCH ))
	GLint						mCachedFramebuffer;
#else
	GLint						mCachedReadFramebuffer, mCachedDrawFramebuffer;
#endif
	
	std::map<GLenum,GLboolean>	mCachedStateBoolean;
	std::map<GLenum,GLint>		mCachedStateInt;
	std::map<GLenum,GLint>		mCachedTextureBinding;
	GLint						mCachedActiveTexture;
	GLenum						mCachedFrontPolygonMode, mCachedBackPolygonMode;
	
	VaoRef						mDefaultVao;	
	VboRef						mDefaultArrayVbo, mDefaultElementVbo;
	VertBatchRef				mImmediateMode;
	
  private:
	Context( const std::shared_ptr<PlatformData> &platformData );
  
	std::shared_ptr<PlatformData>	mPlatformData;
	
	std::pair<Vec2i, Vec2i>		mViewport;
	std::pair<Vec2i, Vec2i>		mScissor;

	VaoRef						mImmVao; // Immediate-mode VAO
	VboRef						mImmVbo; // Immediate-mode VBO
	
	ci::ColorAf					mColor;	
	std::vector<Matrix44f>		mModelViewStack;
	std::vector<Matrix44f>		mProjectionStack;

	friend class				Environment;
	friend class				EnvironmentEs2Profile;
	friend class				EnvironmentCoreProfile;
	friend class				EnvironmentCompatibilityProfile;
	
	friend class				Fog;
	friend class				Light;
	friend class				Material;
	friend class				Texture;
};


struct VaoScope : public boost::noncopyable {
	VaoScope( const VaoRef &vao )
		: mCtx( gl::context() )
	{
		mPrevVao = mCtx->vaoGet();
		mCtx->vaoBind( vao );
	}
	
	~VaoScope() {
		mCtx->vaoBind( mPrevVao );
	}
  private:
	Context		*mCtx;
	VaoRef		mPrevVao;
};

struct BufferScope : public boost::noncopyable {
	BufferScope( const BufferObjRef &bufferObj );
	BufferScope( GLenum target, GLuint id )
		: mCtx( gl::context() ), mTarget( target )
	{
		mPrevId = mCtx->getBufferBinding( target );
		mCtx->bindBuffer( target, id );
	}

	~BufferScope() {
		mCtx->bindBuffer( mTarget, mPrevId );
	}
  private:
	Context		*mCtx;
	GLenum		mTarget;
	GLuint		mPrevId;
};

template<typename T>
struct StateScope : public boost::noncopyable {
	StateScope( GLenum cap, T value )
		: mCtx( gl::context() ), mCap( cap )
	{
		mPrevValue = mCtx->stateGet<T>( cap );
		mCtx->stateSet<T>( cap, value );
	}

	~StateScope() {
		mCtx->stateSet<T>( mCap, mPrevValue );
	}
  private:
	Context		*mCtx;
	GLenum		mCap;
	T			mPrevValue;
};

struct BlendScope : public boost::noncopyable
{
	BlendScope( GLboolean enable );
	//! Parallels glBlendFunc(), implicitly enables blending
	BlendScope( GLenum sfactor, GLenum dfactor );
	//! Parallels glBlendFuncSeparate(), implicitly enables blending
	BlendScope( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
	~BlendScope();
	
  private:
	Context		*mCtx;
	bool		mSaveFactors; // whether we should also set th blend factors rather than just the blend state
	GLboolean	mPrevBlend;
	GLint		mPrevSrcRgb, mPrevDstRgb, mPrevSrcAlpha, mPrevDstAlpha;
};

struct ShaderScope : public boost::noncopyable
{
	ShaderScope( const GlslProgRef &prog )
		: mCtx( gl::context() )
	{
		mPrevProg = mCtx->getCurrentShader();
		mCtx->bindShader( prog );
	}

	// this is for convenience
	ShaderScope( const std::shared_ptr<const GlslProg> &prog )
		: mCtx( gl::context() )
	{
		mPrevProg = mCtx->getCurrentShader();
		mCtx->bindShader( std::const_pointer_cast<GlslProg>( prog ) );
	}

	~ShaderScope()
	{
		mCtx->bindShader( mPrevProg );
	}

  private:
	Context		*mCtx;
	GlslProgRef	mPrevProg;
};

struct FramebufferScope : public boost::noncopyable
{
	FramebufferScope(); // preserves but doesn't set
	FramebufferScope( const FboRef &fbo, GLenum target = GL_FRAMEBUFFER );
	//! Prefer the FboRef variant when possible. This does not allow gl::Fbo to mark itself as needing multisample resolution.
	FramebufferScope( GLenum target, GLuint framebuffer );
	~FramebufferScope();
	
  private:
	void		saveState();

	Context		*mCtx;
	GLenum		mTarget;
#if defined( CINDER_GLES ) && ( ! defined( CINDER_COCOA_TOUCH ) )
	GLuint		mPrevFramebuffer;
#else
	GLuint		mPrevReadFramebuffer, mPrevDrawFramebuffer;
#endif
};

struct ActiveTextureScope : public boost::noncopyable
{
	//! Sets the currently active texture through glActiveTexture. Expects values relative to \c 0, \em not GL_TEXTURE0
	ActiveTextureScope( uint8_t textureUnit )
		: mCtx( gl::context() )
	{
		mPrevValue = mCtx->getActiveTexture();
		mCtx->activeTexture( textureUnit );
	}
	
	~ActiveTextureScope()
	{
		mCtx->activeTexture( mPrevValue );
	}
	
  private:
	Context		*mCtx;
	uint8_t		mPrevValue;
};

struct TextureBindScope : public boost::noncopyable
{
	TextureBindScope( GLenum target, GLuint textureId )
		: mCtx( gl::context() ), mTarget( target )
	{
		mPrevValue = mCtx->getTextureBinding( mTarget );
		mCtx->bindTexture( mTarget, textureId );
	}

	TextureBindScope( const TextureRef &texture )
		: mCtx( gl::context() ), mTarget( texture->getTarget() )
	{
		mPrevValue = mCtx->getTextureBinding( mTarget );
		mCtx->bindTexture( mTarget, texture->getId() );
	}
	
	~TextureBindScope()
	{
		mCtx->bindTexture( mTarget, mPrevValue );
	}
	
  private:
	Context		*mCtx;
	GLenum		mTarget;
	GLuint		mPrevValue;
};
	
struct ScissorScope : public boost::noncopyable
{
	ScissorScope( const Vec2i &lowerLeftPostion, const Vec2i &dimension )
		: mCtx( gl::context() ), mPrevScissor( mCtx->getScissor() )
	{
		mCtx->setScissor( std::pair<Vec2i, Vec2i>( lowerLeftPostion, dimension ) );
		mCtx->enable( GL_SCISSOR_TEST );
	}
	
	ScissorScope( int lowerLeftX, int lowerLeftY, int width, int height )
		: mCtx( gl::context() ), mPrevScissor( mCtx->getScissor() )
	{
		mCtx->setScissor( std::pair<Vec2i, Vec2i>( Vec2i( lowerLeftX, lowerLeftY ), Vec2i( width, height ) ) );
		mCtx->enable( GL_SCISSOR_TEST );
	}
	
	~ScissorScope()
	{
		mCtx->enable( GL_SCISSOR_TEST, GL_FALSE );
		mCtx->setScissor( mPrevScissor );
	}
	
  private:
	Context					*mCtx;
	GLboolean				mPrevState;
	std::pair<Vec2i, Vec2i>	mPrevScissor;
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
