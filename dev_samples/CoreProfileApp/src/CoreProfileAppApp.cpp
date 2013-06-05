////////////////////////////////////////////////////////////////////
// The OpenGL® Programming Guide, 8th Edition
// Example 4.3, Cinderified

const GLuint  NumVertices = 6;

#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CoreProfileApp : public AppNative {
  public:
	virtual void		setup() override;
	virtual void		keyDown( KeyEvent event ) override;
	virtual void		draw() override;

  private:
	gl::GlslProgRef		mShader;
	gl::VaoRef			mTriangleVao;
	gl::VboRef			mVertexVbo;
};

void CoreProfileApp::setup()
{
	mTriangleVao = gl::Vao::create();
	mTriangleVao->bind();

	struct VertexData {
		GLfloat color[3];
		GLfloat position[4];
	};

	VertexData vertices[NumVertices] = {
		{{  1.00, 0.00, 0.00 }, { -0.90, -0.90 }},  // Triangle 1
		{{  0.00, 1.00, 0.00 }, {  0.85, -0.90 }},
		{{  0.00, 0.00, 1.00 }, { -0.90,  0.85 }},
		{{  0.04, 0.04, 0.04 }, {  0.90, -0.85 }},  // Triangle 2
		{{  0.40, 0.40, 0.40 }, {  0.90,  0.90 }},
		{{  1.00, 1.00, 1.00 }, { -0.85,  0.90 }}
	};

	mVertexVbo = gl::Vbo::create( GL_ARRAY_BUFFER );
	mVertexVbo->bufferData( sizeof(vertices), vertices, GL_STATIC_DRAW );

	mShader = gl::GlslProg::create( loadResource( "gouraud.vert" ), loadResource( "gouraud.frag" ) );
	mShader->bind();

	GLint vColorLoc = mShader->getAttribLocation( "vColor" );
	GLint vPosLoc = mShader->getAttribLocation( "vPosition" );

	mTriangleVao->bindBuffer( mVertexVbo );
	mTriangleVao->vertexAttribPointer( vColorLoc, 3, GL_FLOAT,
						   GL_TRUE, sizeof(VertexData), (const void*)(0) );
	mTriangleVao->vertexAttribPointer( vPosLoc, 2, GL_FLOAT,
						   GL_FALSE, sizeof(VertexData),
						   (const void*)(sizeof(vertices[0].color)) );

	mTriangleVao->enableVertexAttribArray( vColorLoc );
	mTriangleVao->enableVertexAttribArray( vPosLoc );
}

void CoreProfileApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'm': {
			static GLenum  mode = GL_FILL;

			mode = ( mode == GL_FILL ? GL_LINE : GL_FILL );
			gl::polygonMode( GL_FRONT_AND_BACK, mode );
		}
		break;
	}
}

void CoreProfileApp::draw()
{
	gl::clear();

	gl::setProjection( Matrix44f::createOrthographic( -1, -1, 1, 1, 1, -1 ) );
	gl::setModelView( Matrix44f::identity() );

	mTriangleVao->bind();
	gl::drawArrays( GL_TRIANGLES, 0, NumVertices );
}

auto renderOptions = RendererGl::Options().coreProfile();
CINDER_APP_NATIVE( CoreProfileApp, RendererGl( renderOptions ) )
