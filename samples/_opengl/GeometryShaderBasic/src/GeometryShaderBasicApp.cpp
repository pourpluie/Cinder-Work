#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryShaderIntroApp : public AppNative {
public:
	void setup();
	void mouseDrag( MouseEvent event );
	void draw();
	
	void loadShader();
	void loadBuffers();
	
	gl::VaoRef		mVao;
	gl::VboRef		mDataVbo;
	gl::GlslProgRef mGlsl;
	int				mNumSides;
};

void GeometryShaderIntroApp::setup()
{
	mNumSides = 2;
	loadShader();
	loadBuffers();
}

void GeometryShaderIntroApp::mouseDrag( MouseEvent event )
{
	mNumSides = (((float)event.getX() / getWindowWidth()) * 30) + 3;
	if( mNumSides < 2 ) mNumSides = 2;
}

void GeometryShaderIntroApp::draw()
{
	gl::VaoScope vaoScope( mVao );
	gl::GlslProgScope glslScope( mGlsl );
	
	gl::clear( ColorA( 0, 0, 0, 1 ) );
	
	mGlsl->uniform( "sides", mNumSides);
	
	gl::drawArrays(GL_POINTS, 0, 1);
}

void GeometryShaderIntroApp::loadBuffers()
{
	float vertices[] = {
		//  position x,y | color r,g,b
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f
	};
	
	mVao = gl::Vao::create();
	
	mDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(vertices), vertices );
	
	gl::VaoScope	vaoScope( mVao );
	gl::BufferScope bufferScope( mDataVbo );
	
	gl::vertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	gl::enableVertexAttribArray( 0 );
	
	gl::vertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)( 2*sizeof(float) ) );
	gl::enableVertexAttribArray( 1 );
}

void GeometryShaderIntroApp::loadShader()
{
	try {
		mGlsl = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "basic.vert" ) )
										.fragment( loadAsset( "basic.frag" ) )
										.geometry( loadAsset( "basic.geom" ) )
										.attribLocation( "position", 0 )
										.attribLocation( "color", 1 ) );
	}
	catch( gl::GlslProgCompileExc ex ) {
		cout << ex.what() << endl;
		shutdown();
	}
}

CINDER_APP_NATIVE( GeometryShaderIntroApp, RendererGl )
