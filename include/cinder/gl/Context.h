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
struct VaoScope;
struct BufferScope;

class Context {
  public:
	typedef std::map<Shader::UniformOptions, ShaderRef> ShaderMap;
	
	Context();
	~Context();

	VaoScope	vaoPush( GLuint id );
	VaoScope	vaoPush( const Vao *vao );
	VaoScope	vaoPush( const VaoRef &vao );
	void		vaoPop();
	void		vaoPrepareUse();

	BufferScope		bufferPush( GLenum target, GLuint id );
	BufferScope		bufferPush( const BufferObj *buffer );
	BufferScope		bufferPush( const BufferObjRef &vbuffer );
	void			bufferPop( GLenum target );
	void			bufferPrepareUse( GLenum target );

	
	void		prepareDraw();

	void		vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	void		enableVertexAttribArray( GLuint index );

	GLuint				mActiveVao;
	std::vector<GLuint>	mVaoStack;
	
	std::map<GLenum,GLuint>					mActiveBuffer;
	std::map<GLenum,std::vector<GLuint>>	mBufferStack;

	
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


struct VaoScope {
	VaoScope( Context *ctx ) : mCtx( ctx )
	{
	}
	
	~VaoScope() {
		mCtx->vaoPop();
	}
  private:
	Context		*mCtx;
};

struct BufferScope {
	BufferScope( Context *ctx, GLenum target )
		: mCtx( ctx ), mTarget( target )
	{
	}
	
	~BufferScope() {
		mCtx->bufferPop( mTarget );
	}
  private:
	Context		*mCtx;
	GLenum		mTarget;
};

} } // namespace cinder::gl