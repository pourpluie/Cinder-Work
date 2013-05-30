#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/Utilities.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Es2App : public AppNative {
  public:
	virtual void		setup() override;
	virtual void		resize() override;
	virtual void		update() override;
	virtual void		draw() override;

  private:
	ci::gl::GlslProgRef	mShader;
	ci::gl::TextureRef	mTexture;
	ci::gl::VboRef		mVbo;
	ci::gl::VaoRef		mVao;
	float				mSecondTriRotation;
	Matrix44f			mCubeRotation;
	CameraPersp			mCam;
};

void Es2App::setup()
{
	try {
		mShader = gl::GlslProg::create( loadResource( RES_VERT_GLSL ), loadResource( RES_FRAG_GLSL ) );
	}
	catch ( gl::GlslProgCompileExc ex ) {
		console() << ex.what() << endl;
		quit();
	}
	
	mTexture = gl::Texture::create( loadImage( loadResource( RES_TEXTURE ) ) );

	GLfloat vVertices[] = {  getWindowCenter().x, 0.0f, 0.0f, 
					0, (float)getWindowHeight(), 0.0f,
					(float)getWindowWidth(), (float)getWindowHeight(), 0.0f };
	GLfloat vTexCoords[] = { 0, 0, 1, 0, 1, 1 };
	
	mVbo = gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(vVertices) + sizeof(vTexCoords) );
	mVbo->bufferSubData( 0, sizeof(vVertices), vVertices );
	mVbo->bufferSubData( sizeof( vVertices ), sizeof(vTexCoords), vTexCoords );

	mVao = gl::Vao::create();

	mShader->bind();
	mShader->uniform( "uTex0", (int)0 );
//	mShader->uniform( "uTexEnabled", true );

	int pos = mShader->getAttribLocation( "vPosition" );
	int tex = mShader->getAttribLocation( "vTexCoord0" );
	console() << "pos: " << pos << " " << " tex " << tex << std::endl;
	auto uniforms = mShader->getActiveUniformTypes();
	console() << "Uniform Types: " << std::endl;
	for( auto &uni : uniforms )
		console() << " " << uni.first << ":" << uni.second << std::endl;
	auto attrSems = mShader->getAttribSemantics();
	console() << "Attribute Semantics: " << std::endl;
	for( auto &sem : attrSems )
		console() << " " << sem.first << ":" << sem.second << std::endl;
	
//	mVao->vertexAttribPointer( mVbo, pos, 3, GL_FLOAT, GL_FALSE, 0, 0 );
//	mVao->vertexAttribPointer( mVbo, tex, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*9) );

	mCam.lookAt( Vec3f( 3, 2, -3 ), Vec3f::zero() );
	mCubeRotation.setToIdentity();
	
	gl::setMatricesWindowPersp( getWindowSize() );
	
	mSecondTriRotation = 0;
}

void Es2App::resize()
{
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
}

void Es2App::update()
{
	gl::rotate( Vec3f( 0, 0, 0.1f ) );
	mSecondTriRotation += -0.15f;
	mCubeRotation.rotate( Vec3f( 1, 1, 1 ), 0.03f );
}

void Es2App::draw()
{
	gl::clear();

	gl::enableAdditiveBlending();

	gl::ScopeShader shader( mShader );
	{
		gl::setDefaultShaderUniforms();
		gl::VaoScope vaoBind( mVao );
		gl::BufferScope vboBind( mVbo );
		gl::drawArrays( GL_TRIANGLES, 0, 3 );
	}
	{
		gl::pushModelView();
		gl::rotate( Vec3f( 0, 0, mSecondTriRotation ) );
		gl::setDefaultShaderUniforms();
//		mShader->uniform( "uModelViewProjection", gl::getProjection() * gl::getModelView() );
		gl::VaoScope vaoBind( mVao );
		gl::BufferScope vboBind( mVbo );
		gl::drawArrays( GL_TRIANGLES, 0, 3 );
		gl::popModelView();
	}
	{
		gl::pushMatrices();
			gl::setMatrices( mCam );
			gl::multModelView( mCubeRotation );
			gl::setDefaultShaderUniforms();
			gl::drawCube( Vec3f::zero(), Vec3f( 2.0f, 2.0f, 2.0f ) );
		gl::popMatrices();
	}
}

auto renderOptions = RendererGl::Options().coreProfile();
CINDER_APP_NATIVE( Es2App, RendererGl( renderOptions ) )
