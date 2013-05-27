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

class CoreProfileApp : public AppNative {
  public:
	virtual void		setup() override;
	virtual void		update() override;
	virtual void		draw() override;

  private:
	ci::gl::GlslProgRef	mShader;
	ci::gl::TextureRef	mTexture;
	ci::gl::VboRef		mVbo;
	ci::gl::VaoRef		mVao;
	float				mSecondTriRotation;
};

void CoreProfileApp::setup()
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
	mVbo->bufferSubData( vVertices, sizeof(vVertices), 0 );
	mVbo->bufferSubData( vTexCoords, sizeof(vTexCoords), sizeof(vVertices) );

	mVao = gl::Vao::create();

	mShader->bind();
//	mShader->uniform( "uTexture", 0 );
//	mShader->uniform( "uTexEnabled", true );
	mShader->bindAttribLocation( "aPosition", 0 );
	mShader->bindAttribLocation( "aTexCoord", 2 );

	int pos = mShader->getAttribLocation( "aPosition" );
	int tex = mShader->getAttribLocation( "aTexCoord" );
	console() << "pos: " << pos << " " << " tex " << tex << std::endl;
	
	mVao->vertexAttribPointer( mVbo, pos, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	mVao->vertexAttribPointer( mVbo, tex, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*9) );
//	glVertexAttribPointer( pos, 3, GL_FLOAT, GL_FALSE, 0, 0 );
//	glEnableVertexAttribArray( pos );
//	glVertexAttribPointer( tex, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*9) );
//	glEnableVertexAttribArray( tex );
	
	gl::setMatricesWindowPersp( getWindowSize() );
	
	mSecondTriRotation = 0;
}

void CoreProfileApp::update()
{
	gl::rotate( Vec3f( 0, 0, 0.1f ) );
	mSecondTriRotation += -0.15f;
}

void CoreProfileApp::draw()
{
	gl::clear();

	gl::enableAdditiveBlending();

	mShader->bind();
	{
		mShader->uniform( "uModelViewProjection", gl::getProjection() * gl::getModelView() );
		gl::VaoScope vaoBind( mVao->getId() );
		gl::BufferScope vboBind( mVbo->getTarget(), mVbo->getId() );
		gl::drawArrays( GL_TRIANGLES, 0, 3 );
	}
	{
		gl::pushModelView();
		gl::rotate( Vec3f( 0, 0, mSecondTriRotation ) );
		mShader->uniform( "uModelViewProjection", gl::getProjection() * gl::getModelView() );
		gl::VaoScope vaoBind( mVao->getId() );
		gl::BufferScope vboBind( mVbo->getTarget(), mVbo->getId() );
		gl::drawArrays( GL_TRIANGLES, 0, 3 );
		gl::popModelView();
	}
	mShader->unbind();
}

auto renderOptions = RendererGl::Options().coreProfile();
CINDER_APP_NATIVE( CoreProfileApp, RendererGl( renderOptions ) )
