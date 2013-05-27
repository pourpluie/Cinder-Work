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

class Context {
  public:
	typedef std::map<Shader::UniformOptions, ShaderRef> ShaderMap;
	
	Context();
	~Context();

	void		vaoBind( GLuint id );
	GLuint		vaoGet();
	void		vaoRestore( GLuint id );
	void		vaoPrepareUse();

	void			bufferBind( GLenum target, GLuint id );
	GLuint			bufferGet( GLenum target );
	void			bufferRestore( GLenum target, GLuint id );
	void			bufferPrepareUse( GLenum target );

	template<typename T>
	void			stateSet( GLenum cap, T value );
	void			enable( GLenum cap, GLboolean value = true );
	template<typename T>
	T				stateGet( GLenum cap );
	template<typename T>
	void			stateRestore( GLenum cap, T value );
	template<typename T>
	void			statePrepareUse( GLenum cap );
	void			statesPrepareUse();
	
	void		prepareDraw();

	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	void		enableVertexAttribArray( GLuint index );

	GLuint						mActiveVao, mTrueVao;
	std::map<GLenum,GLuint>		mActiveBuffer, mTrueBuffer;
	std::map<GLenum,GLboolean>	mActiveStateBoolean, mTrueStateBoolean;
	
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
	VaoScope( GLuint id ) 
		: mCtx( gl::context() )
	{
		mPrevId = mCtx->vaoGet();
		mCtx->vaoBind( id );
	}
	
	~VaoScope() {
		mCtx->vaoRestore( mPrevId );
	}
  private:
	Context		*mCtx;
	GLuint		mPrevId;
};

struct BufferScope : public boost::noncopyable {
	BufferScope( GLenum target, GLuint id )
		: mCtx( gl::context() ), mTarget( target )
	{
		mPrevId = mCtx->bufferGet( target );
		mCtx->bufferBind( target, id );
	}

	~BufferScope() {
		mCtx->bufferRestore( mTarget, mPrevId );
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
		mCtx->stateRestore<T>( mCap, mPrevValue );
	}
  private:
	Context		*mCtx;
	GLenum		mCap;
	T			mPrevValue;
};

} } // namespace cinder::gl