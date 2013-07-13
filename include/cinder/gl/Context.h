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

class Context {
  public:
	~Context();
	static ContextRef	create();
	
	void		vaoBind( const VaoRef &vao );
	VaoRef		vaoGet();

	void		bindBuffer( GLenum target, GLuint id );
	GLuint		getBufferBinding( GLenum target );
	//! Marks the Context's cache of the binding for \a target as invalid
	void		invalidateBufferBinding( GLenum target );

	void		bindShader( const GlslProgRef &prog );
	//! Sets the current shader to 'none'
	void		unbindShader();
	GlslProgRef	getCurrentShader();

	void		activeTexture( GLenum textureUnit );
	GLenum		getActiveTexture();

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
	//! Analogous to glVertexAttribPointer
	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	//! Analogous to glEnableVertexAttribArray
	void		enableVertexAttribArray( GLuint index );
	//! Analogous to glVertexAttrib1f
	void		vertexAttrib1f( GLuint index, float v0 );
	//! Analogous to glVertexAttrib2f
	void		vertexAttrib2f( GLuint index, float v0, float v1 );
	//! Analogous to glVertexAttrib3f
	void		vertexAttrib3f( GLuint index, float v0, float v1, float v2 );
	//! Analogous to glVertexAttrib4f
	void		vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 );

	//! Parallels glBlendFunc()
	void		blendFunc( GLenum sfactor, GLenum dfactor );
	//! Parallels glBlendFuncSeparate()
	void		blendFuncSeparate( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );

	//! Parallels glDepthMask()
	void		depthMask( GLboolean enable );

#if ! defined( CINDER_GLES )
	//! Parallels glPolygonMode()
	void		polygonMode( GLenum face, GLenum mode );
#endif

	//! Returns the current active color, used in immediate-mode emulation and as UNIFORM_COLOR
	const ColorAf&	getCurrentColor() const;
	GlslProgRef		getStockShader( const ShaderDef &shaderDef );

	VboRef			getDefaultArrayVbo( size_t requiredSize );
	VboRef			getDefaultElementVbo( size_t requiredSize );

  private:
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
	GLint						mCachedActiveTexture;
	GLenum						mCachedFrontPolygonMode, mCachedBackPolygonMode;
	
	VaoRef						mDefaultVao;	
	VboRef						mDefaultArrayVbo, mDefaultElementVbo;
	
	
  public:
	struct Vertex
	{
		ColorAf					mColor;
		Vec3f					mNormal;
		Vec3f					mPosition;
		Vec4f					mTexCoord;
		float					unused[ 2 ]; // 64
	};
	
	Fog							mFog;
	bool						mFogEnabled;
	std::vector<Light>			mLights;
	Material					mMaterial;
	bool						mMaterialEnabled;
	
	VaoRef						mImmVao; // Immediate-mode VAO
	VboRef						mImmVbo; // Immediate-mode VBO
	
	void						clear();

	bool						mLighting;
	ci::Vec3f					mNormal;
	ci::Vec4f					mTexCoord;
	bool						mWireframe;

	std::vector<Vertex>			mVertices;
	void						pushBack( const Vec4f &v );

	ci::ColorAf					mColor;	
	std::vector<Matrix44f>		mModelView;
	std::vector<Matrix44f>		mProjection;

	GLenum						mMode;

  private:
	Context();
  
  
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
	ActiveTextureScope( GLenum texture )
		: mCtx( gl::context() )
	{
		mPrevValue = mCtx->getActiveTexture();
		mCtx->activeTexture( texture );
	}
	
	~ActiveTextureScope()
	{
		mCtx->activeTexture( mPrevValue );
	}
	
  private:
	Context		*mCtx;
	GLenum		mPrevValue;
};

} } // namespace cinder::gl