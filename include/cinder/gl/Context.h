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
#include "cinder/gl/Vao.h"

#include <vector>

namespace cinder { namespace gl {

class Vbo;
typedef std::shared_ptr<Vbo>			VboRef;
class Vao;
typedef std::shared_ptr<Vao>			VaoRef;

class Texture;
	
class Context {
  public:
	typedef std::map<Shader::UniformOptions, ShaderRef> ShaderMap;
	
	Context();
	~Context();
	
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
	VaoRef						mVao;
	VboRef						mVbo;
	
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
	friend class				Fog;
	friend class				Light;
	friend class				Material;
	friend class				Texture;
};

} } // namespace cinder::gl