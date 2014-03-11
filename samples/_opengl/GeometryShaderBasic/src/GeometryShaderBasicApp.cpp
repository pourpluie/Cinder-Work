#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Batch.h"

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
	
	gl::VertBatchRef	mBatch;
	gl::GlslProgRef		mGlsl;
	int					mNumSides;
	float				mRadius;
};

void GeometryShaderIntroApp::setup()
{
	mNumSides = 2;
	mRadius = 100;
	
	loadShader();
	loadBuffers();
}

void GeometryShaderIntroApp::mouseDrag( MouseEvent event )
{
	mNumSides = (((float)event.getX() / getWindowWidth()) * 30) + 3;
	if( mNumSides < 2 ) mNumSides = 2;
	else if( mNumSides > 64 ) mNumSides = 64;
	
	mRadius = ((float)event.getY() / getWindowHeight()) * ( getWindowWidth() / 2.0f );
	if( mRadius < 1.0f ) mRadius = 1.0f;
	else if( mRadius > getWindowWidth() / 2.0f ) mRadius = getWindowWidth() / 2.0f;
}

void GeometryShaderIntroApp::draw()
{
	gl::clear( ColorA( 0, 0, 0, 1 ) );
	
	gl::ScopedGlslProg glslProg( mGlsl );
	
	gl::pushMatrices();
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	
	gl::setDefaultShaderVars();

	mGlsl->uniform( "uNumSides", mNumSides );
	mGlsl->uniform( "uRadius", mRadius );
	
	mBatch->draw();
	
	gl::popMatrices();
}

void GeometryShaderIntroApp::loadShader()
{
	try {
		mGlsl = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "basic.vert" ) )
									 .fragment( loadAsset( "basic.frag" ) )
									 .geometry( loadAsset( "basic.geom" ) )
									 .attrib( geom::Attrib::POSITION, "Position" )
									 .attrib( geom::Attrib::COLOR, "Color" )
									 .uniform( gl::UniformSemantic::UNIFORM_MODEL_VIEW, "ciModelView" )
									 .uniform( gl::UniformSemantic::UNIFORM_PROJECTION_MATRIX, "ciProjectionMatrix" ) );
	}
	catch( gl::GlslProgCompileExc ex ) {
		cout << ex.what() << endl;
		shutdown();
	}
}

void GeometryShaderIntroApp::loadBuffers()
{
	mBatch = gl::VertBatch::create();
	mBatch->vertex( Vec4f( (float)getWindowWidth() / 2.0f, (float)getWindowHeight() / 2.0f, 0.0f, 1.0f ) );
	mBatch->color( 1.0f, 0.0f, 0.0f );
}

CINDER_APP_NATIVE( GeometryShaderIntroApp, RendererGl )
