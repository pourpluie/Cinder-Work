#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Texture3dApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	
	gl::Texture3dRef	mTex3d, mTex2dArray;
	gl::GlslProgRef		mShader3d, mShader2dArray;
	gl::BatchRef		mTex3dBatch, mTex2dArrayBatch;
};

Surface createSolid( int width, int height, Color8u c )
{
	Surface result( width, height, false );
	Surface::Iter it = result.getIter();
	while( it.line() ) {
		while( it.pixel() ) {
			it.r() = c.r;
			it.g() = c.g;
			it.b() = c.b;
		}
	}
	
	return result;
}

void Texture3dApp::setup()
{
	// GL_TEXTURE_3D
	mTex3d = gl::Texture3d::create( 256, 256, 3 );
	mTex3d->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(255,0,0) ), 0 );
	mTex3d->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(0,255,0) ), 1 );
	mTex3d->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(0,0,255) ), 2 );	

	// GL_TEXTURE_2D_ARRAY
	mTex2dArray = gl::Texture3d::create( 256, 256, 3, gl::Texture3d::Format().target( GL_TEXTURE_2D_ARRAY ) );
	mTex2dArray->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(255,0,0) ), 0 );
	mTex2dArray->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(0,255,0) ), 1 );
	mTex2dArray->update( createSolid( mTex3d->getWidth(), mTex3d->getHeight(), Color8u(0,0,255) ), 2 );	
	
	mShader3d = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader_3d.frag" ) );
	mShader3d->uniform( "uTex0", 0 );
	mShader2dArray = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader_2d_array.frag" ) );
	mShader2dArray->uniform( "uTex0", 0 );
	
	mTex3dBatch = gl::Batch::create( geom::Rect().scale( Vec2f( 200, 200 ) ).position( Vec2f( 100, 100 ) ), mShader3d );
	mTex2dArrayBatch = gl::Batch::create( geom::Rect().scale( Vec2f( 200, 200 ) ).position( Vec2f( 100, 300 ) ), mShader2dArray );
}

void Texture3dApp::mouseDown( MouseEvent event )
{
}

void Texture3dApp::update()
{
}

void Texture3dApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 

	mTex3d->bind();
	mTex3dBatch->getGlslProg()->uniform( "uTexCoord", Vec3f( 0.5, 0.5, getElapsedSeconds() / 2 ) );
	mTex3dBatch->draw();

	mTex2dArray->bind();
	mTex2dArrayBatch->getGlslProg()->uniform( "uTexCoord", Vec3f( 0.5, 0.5, getElapsedSeconds() / 2 ) );
	mTex2dArrayBatch->draw();
}

CINDER_APP_NATIVE( Texture3dApp, RendererGl )