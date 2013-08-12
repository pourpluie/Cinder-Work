#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ImageIo.h"
#include "cinder/Text.h"

#if defined( CINDER_GL_ANGLE )
	#include "cinder/app/RendererAngle.h"
	namespace cinder { namespace app {
		typedef RendererAngle RendererT;
	} }
#else
	#include "cinder/app/RendererGl.h"
	namespace cinder { namespace app {
		typedef RendererGl RendererT;
	} }

	void checkGlStatus() {}

#endif

#include <stdlib.h>

#include <boost/format.hpp>

#define DRAW_COUNT 40

using namespace ci;
using namespace ci::app;
using namespace ci::gl;
using namespace std;

class AngleTest1App : public AppNative {
  public:
	void setup();
	void update();
	void draw();

	void drawBasicTest();
	void drawManyTextures();
	void drawManyTexturesUnwinded();

	gl::TextureRef		mTex;
	Font mFont;
};

void AngleTest1App::setup()
{
	checkGlStatus();

	console() << "gl version: " << gl::getVersionString() << endl;

	const char *extStr = reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) );

	console() << "extensions:" << endl << extStr << endl; 

	mTex = gl::Texture::create( loadImage( loadAsset( "texture.jpg" ) ) );
	checkGlStatus();

	mFont = Font( Font::getDefault().getName(), 20.0f );

	// TODO: check for extensions that we think are unavailable
	// - vbo map/unmap: GL_OES_mapbuffer
	// - vao's: GL_OES_vertex_array_object

	gl::bindStockShader( gl::ShaderDef().color() );
	gl::enableAlphaBlending();

	checkGlStatus();
}

void AngleTest1App::update()
{
	if( getElapsedFrames() % 100 == 0 )
		console() << "fps: " << getAverageFps() << endl;

}

void AngleTest1App::draw()
{
	checkGlStatus();

	gl::clear( Color( 0.0f, 1.0f, 0.0f ) );

	drawBasicTest();
	checkGlStatus();
}

void AngleTest1App::drawBasicTest()
{
	gl::color( Color::white() );
	gl::draw( mTex, getWindowBounds() );

	Vec2i center = getWindowCenter();
	int k = 100;

	gl::pushMatrices();
		gl::translate( getWindowCenter() );
		gl::rotate( getElapsedSeconds() * 30.0f );
		gl::color( 0.0f, 0.0f, 0.8f, 0.6f );
		gl::drawSolidRect( Rectf( Area( -k, -k, k, k ) ) );
	gl::popMatrices();

	checkGlStatus();

	TextLayout layout;
	layout.setBorder( 6, 6 );
	layout.clear( ColorA( 0.0f, 0.0f, 0.0f, 0.5f ) );
	layout.setColor( ColorA::gray( 0.85f ) );
	layout.setFont( mFont );
	auto fpsFormatted = boost::format( "fps: %0.2f" ) % getAverageFps();
	layout.addLine( fpsFormatted.str() );

	auto infoTex = gl::Texture::create( layout.render( true ) );
	gl::draw( infoTex, getWindowSize() - infoTex->getSize() );

	//gl::draw( mInfoTex, getWindowSize() - mInfoTex->getSize() );
}

void AngleTest1App::drawManyTextures()
{
	for( int i =0; i < DRAW_COUNT; i++ ) {
		gl::draw( mTex, getWindowBounds() );
	}
}

void AngleTest1App::drawManyTexturesUnwinded()
{

	////////////////////////////////////////////////////////////////
	// gl::draw( tex, rect )
	auto texture = mTex;
	Context *ctx = gl::context();
	GlslProgRef shader = ctx->getStockShader( ShaderDef().texture( texture ).color() );
	ShaderScope shaderScope( shader );

	texture->bind();
	shader->uniform( "uTex0", 0 );

	GLfloat data[8+8]; // both verts and texCoords
	GLfloat *verts = data, *texCoords = data + 8;

	Rectf rect = getWindowBounds();

	verts[0*2+0] = rect.getX2(); texCoords[0*2+0] = texture->getRight();
	verts[0*2+1] = rect.getY1(); texCoords[0*2+1] = texture->getTop();
	verts[1*2+0] = rect.getX1(); texCoords[1*2+0] = texture->getLeft();
	verts[1*2+1] = rect.getY1(); texCoords[1*2+1] = texture->getTop();
	verts[2*2+0] = rect.getX2(); texCoords[2*2+0] = texture->getRight();
	verts[2*2+1] = rect.getY2(); texCoords[2*2+1] = texture->getBottom();
	verts[3*2+0] = rect.getX1(); texCoords[3*2+0] = texture->getLeft();
	verts[3*2+1] = rect.getY2(); texCoords[3*2+1] = texture->getBottom();

	VaoRef vao = Vao::create();
	VaoScope vaoScope( vao );
	VboRef arrayVbo = ctx->getDefaultArrayVbo( sizeof(data) );
	arrayVbo->bind();
	arrayVbo->bufferData( sizeof(data), data, GL_DYNAMIC_DRAW );

	int posLoc = shader->getAttribSemanticLocation( ATTRIB_POSITION );
	enableVertexAttribArray( posLoc );
	vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	int texLoc = shader->getAttribSemanticLocation( ATTRIB_TEX_COORD_0 );
	enableVertexAttribArray( texLoc );	
	vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*8) );

	gl::setDefaultShaderVars();

	for( int i = 0; i < DRAW_COUNT; i++ ) {
		gl::drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}
}

CINDER_APP_NATIVE( AngleTest1App, RendererT )
