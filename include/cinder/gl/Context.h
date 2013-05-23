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

namespace cinder { namespace gl {

class Vbo;
typedef std::shared_ptr<Vbo>			VboRef;
class Vao;
typedef std::shared_ptr<Vao>			VaoRef;
class BufferObj;
typedef std::shared_ptr<BufferObj>		BufferObjRef;
class Texture;
typedef std::shared_ptr<Texture>		TextureRef;

struct VaoScope;
struct BufferScope;
struct StateScope;

class Context {
  public:
	typedef std::map<Shader::UniformOptions, ShaderRef> ShaderMap;
	
	Context();
	~Context();

	VaoScope	vaoPush( GLuint id );
	VaoScope	vaoPush( const Vao *vao );
	VaoScope	vaoPush( const VaoRef &vao );
	void		vaoBind( GLuint id );
	void		vaoRestore( GLuint id );
	void		vaoPrepareUse();

	BufferScope		bufferPush( GLenum target, GLuint id );
	BufferScope		bufferPush( const BufferObj *buffer );
	BufferScope		bufferPush( const BufferObjRef &vbuffer );
	void			bufferBind( GLenum target, GLuint id );
	void			bufferRestore( GLenum target, GLuint id );
	void			bufferPrepareUse( GLenum target );

	StateScope		enablePush( GLenum cap, bool enable = true );
	StateScope		disablePush( GLenum cap );
	void			enable( GLenum cap, bool enable = true );
	void			disable( GLenum cap ) { enable( cap, false ); }
	void			stateRestore( GLenum cap, bool enable );
	void			statePrepareUse( GLenum cap );
	void			statesPrepareUse();
	
	void		prepareDraw();

	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	void		enableVertexAttribArray( GLuint index );

	GLuint						mActiveVao, mTrueVao;
	std::map<GLenum,GLuint>		mActiveBuffer, mTrueBuffer;
	std::map<GLenum,bool>		mActiveState, mTrueState;
	
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
	int							mTextureUnit;
	
	ShaderMap					mShaders;
	VaoRef						mImmVao; // Immediate-mode VAO
	VboRef						mImmVbo; // Immediate-mode VBO
	
	void						clear();
	void						draw();

	ci::ColorAf					mColor;
	bool						mLighting;
	ci::Vec3f					mNormal;
	ci::Vec4f					mTexCoord;
	bool						mWireframe;

	std::vector<Vertex>			mVertices;
	void						pushBack( const Vec4f &v );

	std::vector<Matrix44f>		mModelView;
	std::vector<Matrix44f>		mProjection;

	GLenum						mMode;

  private:
	friend class				Environment;
	friend class				Fog;
	friend class				Light;
	friend class				Material;
	friend class				Texture;
};


struct VaoScope : public boost::noncopyable {
	VaoScope( Context *ctx, GLuint prevId ) 
		: mCtx( ctx ), mPrevId( prevId )
	{}
	
	VaoScope( const VaoScope &&rhs )
		: mCtx( rhs.mCtx ), mPrevId( rhs.mPrevId )
	{}
	
	~VaoScope() {
		mCtx->vaoRestore( mPrevId );
	}
  private:
	Context		*mCtx;
	GLuint		mPrevId;
};

struct BufferScope : public boost::noncopyable {
	BufferScope( Context *ctx, GLenum target, GLuint prevValue )
		: mCtx( ctx ), mTarget( target ), mPrevValue( prevValue )
	{}

	BufferScope( const BufferScope &&rhs )
		: mCtx( rhs.mCtx ), mTarget( rhs.mTarget ), mPrevValue( rhs.mPrevValue )
	{}
	
	~BufferScope() {
		mCtx->bufferRestore( mTarget, mPrevValue );
	}
  private:
	Context		*mCtx;
	GLenum		mTarget;
	GLuint		mPrevValue;
};

struct StateScope : public boost::noncopyable {
	StateScope( Context *ctx, GLenum cap, GLuint prevValue )
		: mCtx( ctx ), mCap( cap ), mPrevValue( prevValue )
	{}

	StateScope( const StateScope &&rhs )
		: mCtx( rhs.mCtx ), mCap( rhs.mCap ), mPrevValue( rhs.mPrevValue )
	{}
	
	~StateScope() {
		mCtx->stateRestore( mCap, mPrevValue );
	}
  private:
	Context		*mCtx;
	GLenum		mCap;
	bool		mPrevValue;
};

} } // namespace cinder::gl