#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"

class Es2App : public ci::app::AppNative
{
public:
	virtual void		draw();
	virtual void		prepareSettings( ci::app::AppNative::Settings *settings );
 	virtual void		setup();
	virtual void		update();
private:
	ci::CameraPersp		mCamera;
	ci::gl::GlslProgRef	mShader;
	ci::gl::TextureRef	mTexture;
	ci::gl::VaoRef		mVao;
	ci::gl::VboRef		mVboData;
	ci::gl::VboRef		mVboIndices;
	ci::gl::VboMeshRef	mVboMesh;
	ci::gl::VboMeshRef	mVboMeshFromTriMesh;
	
	ci::gl::Light		mLight;
	ci::gl::Material	mMaterial;
	
	void				drawPyramid();
	void				drawTexturedPyramid();
	
	struct Vertex
	{
		Vertex( const ci::Vec3f& position = ci::Vec3f::zero(),
			   const ci::ColorAf& color = ci::ColorAf::white(), 
			   const ci::Vec3f& normal = ci::Vec3f( 0.0f, 0.0f, 1.0f ),
			   const ci::Vec2f& texCoord = ci::Vec2f::zero() )
		: mColor( color ), mNormal( normal ), mPosition( position ),
		mTexCoord( ci::Vec4f( texCoord.x, texCoord.y, 0.0f, 0.0f ) )
		{
		}
		ci::ColorAf				mColor;
		ci::Vec3f				mNormal;
		ci::Vec3f				mPosition;
		ci::Vec4f				mTexCoord;
		float					unused[ 2 ];
	};
};

#include "cinder/Utilities.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;


void Es2App::draw()
{
	//gl::setViewport( getWindowBounds() );
	gl::clear( ColorAf::black() );
	gl::setMatrices( mCamera );
	
	gl::enableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enableLighting();
	
	gl::pushMatrices();
	gl::rotate( Vec3f::one() * math<float>::sin( (float)getElapsedSeconds() ) * 2.0f );
	drawPyramid();
	gl::popMatrices();
	
	gl::pushMatrices();
	gl::translate( Vec3f( 2.0f, 0.0f, -3.0f ) );
	gl::rotate( Vec3f::one() * math<float>::cos( (float)getElapsedSeconds() ) * 2.0f );
	drawTexturedPyramid();
	
	gl::pushMatrices();
	gl::translate( Vec3f( 2.0f, 0.0f, -3.0f ) );
	gl::enableWireframe();
	drawPyramid();
	gl::disableWireframe();
	gl::popMatrices();
	
	gl::translate( Vec3f( -4.0f, 0.0f, 0.0f ) );
	drawPyramid();
	
	gl::popMatrices();

	/////////////////////////
	
	/*Matrix44f matrix;
	matrix.setToIdentity();
	matrix.translate( Vec3f( 1.0f, 2.0f, -1.0f ) );
	matrix.rotate( Vec3f::one(), math<float>::sin( (float)getElapsedSeconds() ) * 2.0f );
	matrix = mCamera.getProjectionMatrix() * mCamera.getModelViewMatrix() * matrix;

	mTexture->bind();
	
	mShader->bind();
	mShader->uniform( "uModelViewProjection", matrix );
	mShader->uniform( "uTexEnabled", true );
	mShader->uniform( "uTexture", 0 );
	mVao->bind();
	mVboData->bind();
	gl::draw( mVboIndices );
	mVboData->unbind();
	mVao->unbind();
	mShader->unbind();
	
	mTexture->unbind();
	
	/////////////////////////
	
	matrix.setToIdentity();
	matrix.translate( Vec3f( -1.25f, -2.7f, 0.0f ) );
	matrix.rotate( Vec3f::one(), math<float>::cos( (float)getElapsedSeconds() ) * 4.0f );
	matrix = mCamera.getProjectionMatrix() * mCamera.getModelViewMatrix() * matrix;
	
	mShader->bind();
	mShader->uniform( "uModelViewProjection", matrix );
	mShader->uniform( "uTexEnabled", false );

	gl::enableWireframe();
	gl::draw( mVboMeshFromTriMesh );
	gl::disableWireframe();
	
	mShader->unbind();
	
	/////////////////////////
	
	matrix.setToIdentity();
	matrix.translate( Vec3f( 1.25f, -2.7f, 0.0f ) );
	matrix.rotate( Vec3f::one(), math<float>::cos( (float)getElapsedSeconds() ) * 4.0f );
	matrix = mCamera.getProjectionMatrix() * mCamera.getModelViewMatrix() * matrix;
	
	mShader->bind();
	mShader->uniform( "uModelViewProjection", matrix );
	mShader->uniform( "uTexEnabled", false );
	
	gl::draw( mVboMesh );
	
	mShader->unbind();*/
}

void Es2App::drawPyramid()
{
	gl::begin( GL_TRIANGLES );
	
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f,  1.0f ) );
	gl::color( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f,  1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f,  1.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f, -1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 1.0f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f, -1.0f ) );
	gl::color( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f, -1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f, -1.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f,  1.0f ) );
	
	gl::end();
}

void Es2App::drawTexturedPyramid()
{
	mTexture->bind();
	gl::begin( GL_TRIANGLES );
		
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.5f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.0f, 0.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f,  1.0f ) );
	gl::color( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	gl::texCoord( Vec2f( 1.0f, 0.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f,  1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.5f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	gl::texCoord( Vec2f( 1.0f, 0.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f,  1.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.0f, 0.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f, -1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 1.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.5f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.0f, 0.0f ) );
	gl::normal( Vec3f(  1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f(  1.0f, -1.0f, -1.0f ) );
	gl::color( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 1.0f, 0.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f, -1.0f ) );
	
	gl::color( ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.5f, 1.0f ) );
	gl::normal( Vec3f(  0.0f,  1.0f,  0.0f ).normalized() );
	gl::vertex( Vec3f(  0.0f,  1.0f,  0.0f ) );
	gl::color( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 1.0f, 0.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f, -1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f, -1.0f ) );
	gl::color( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	gl::texCoord( Vec2f( 0.0f, 0.0f ) );
	gl::normal( Vec3f( -1.0f, -1.0f,  1.0f ).normalized() );
	gl::vertex( Vec3f( -1.0f, -1.0f,  1.0f ) );
	
	gl::end();
	mTexture->unbind();
}

void Es2App::prepareSettings( Settings *settings )
{
	settings->enableHighDensityDisplay();
	settings->setFrameRate( 60.0f );
}

void Es2App::setup()
{
	// Shader
	try {
		mShader = gl::GlslProg::create( loadResource( RES_VERT_GLSL ), loadResource( RES_FRAG_GLSL ) );
	} catch ( gl::GlslProgCompileExc ex ) {
		console() << ex.what() << endl;
		quit();
	}
	
	// Camera
	mCamera.setPerspective( 45.0f, getWindowAspectRatio(), 0.01f, 600.0f );
	mCamera.lookAt( Vec3f( 0.0f, 0.0f, 10.0f ), Vec3f::zero() );
	
	// Texture
	mTexture = gl::Texture::create( loadImage( loadResource( RES_TEXTURE ) ) );
	
	// Light and material
	mLight.setPosition( Vec3f( 0.0f, 5.0f, 3.0f ) );
	
	mLight.setShine( 20.0f );
	mLight.setAmbient( ColorAf::white() );
	mLight.setDiffuse( ColorAf::white() );
	mLight.enable();
	mMaterial.enable();
	mMaterial.setAmbient( 1.0f );
	mMaterial.setDiffuse( 1.0f );
	mMaterial.setSpecular( 1.0f );
	mMaterial.setColor( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	////////////////////////////////////////////////////////////
	// VBO / VAO test
	
	// VBO data
	vector<Vertex> verts;
	verts.push_back( Vertex( Vec3f(  0.0f,  1.0f,  0.0f ), ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.5f, 1.0f ) ) );
	verts.push_back( Vertex( Vec3f( -1.0f, -1.0f,  1.0f ), ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f(  1.0f, -1.0f,  1.0f ), ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ), Vec3f::zero(), Vec2f( 1.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f(  0.0f,  1.0f,  0.0f ), ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.5f, 1.0f ) ) );
	verts.push_back( Vertex( Vec3f(  1.0f, -1.0f,  1.0f ), ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ), Vec3f::zero(), Vec2f( 1.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f(  1.0f, -1.0f, -1.0f ), ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f(  0.0f,  1.0f,  0.0f ), ColorAf( 1.0f, 0.0f, 1.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.5f, 1.0f ) ) );
	verts.push_back( Vertex( Vec3f(  1.0f, -1.0f, -1.0f ), ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f( -1.0f, -1.0f, -1.0f ), ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 1.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f(  0.0f,  1.0f,  0.0f ), ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.5f, 1.0f ) ) );
	verts.push_back( Vertex( Vec3f( -1.0f, -1.0f, -1.0f ), ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 1.0f, 0.0f ) ) );
	verts.push_back( Vertex( Vec3f( -1.0f, -1.0f,  1.0f ), ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ), Vec3f::zero(), Vec2f( 0.0f, 0.0f ) ) );
	
	vector<GLuint> indices;
	for ( GLuint i = 0; i < 12; ++i ) {
		indices.push_back( i );
	}

	// VBO
	mVboData = gl::Vbo::create( GL_ARRAY_BUFFER );
	mVboData->bufferData( &verts[ 0 ], (GLuint)( verts.size() * sizeof( Vertex ) ), GL_STATIC_DRAW );
	mVboIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER );
	mVboIndices->bufferData( &indices[ 0 ], (GLuint)( indices.size() * sizeof( GLuint ) ), GL_STATIC_DRAW );
	
	// VAO
	mVao = gl::Vao::create();
	
	size_t offset = 0;
	gl::Vao::Attribute attrColor( mShader->getAttribLocation( "aColor" ), 4 );
	attrColor.setStride( sizeof( Vertex ) );
	offset += sizeof( ColorAf );
	mVao->addAttribute( attrColor );

	gl::Vao::Attribute attrNorm( mShader->getAttribLocation( "aNormal" ), 3 );
	attrNorm.setOffset( (const GLvoid *)offset );
	attrNorm.setStride( sizeof( Vertex ) );
	mVao->addAttribute( attrNorm );
	offset += sizeof( Vec3f );
	
	gl::Vao::Attribute attrPos( mShader->getAttribLocation( "aPosition" ), 3 );
	attrPos.setOffset( (const GLvoid *)offset );
	attrPos.setStride( sizeof( Vertex ) );
	mVao->addAttribute( attrPos );
	offset += sizeof( Vec3f );
	
	gl::Vao::Attribute attrTexCoord( mShader->getAttribLocation( "aTexCoord" ), 4 );
	attrTexCoord.setOffset( (const GLvoid *)offset );
	attrTexCoord.setStride( sizeof( Vertex ) );
	mVao->addAttribute( attrTexCoord );
	
	////////////////////////////////////////////////////////////
	// VBO mesh test from TriMesh
	
	mVboMeshFromTriMesh = gl::VboMesh::create( TriMesh::createSphere( Vec2i( 8, 4 ) ) );
	mVboMeshFromTriMesh->getLayout().setColorAttribLocation( mShader->getAttribLocation( "aColor" ) );
	mVboMeshFromTriMesh->getLayout().setNormalAttribLocation( mShader->getAttribLocation( "aNormal" ) );
	mVboMeshFromTriMesh->getLayout().setPositionAttribLocation( mShader->getAttribLocation( "aPosition" ) );
	mVboMeshFromTriMesh->getLayout().setTexCoordAttribLocation( mShader->getAttribLocation( "aTexCoord" ) );
	
	////////////////////////////////////////////////////////////
	// VBO mesh test
	
/*	mVboMesh = gl::VboMesh::create();
	mVboMesh->getLayout().setColorAttribLocation( mShader->getAttribLocation( "aColor" ) );
	mVboMesh->getLayout().setNormalAttribLocation( mShader->getAttribLocation( "aNormal" ) );
	mVboMesh->getLayout().setPositionAttribLocation( mShader->getAttribLocation( "aPosition" ) );
	mVboMesh->getLayout().setTexCoordAttribLocation( mShader->getAttribLocation( "aTexCoord" ) );
	
	vector<Vec3f> positions;
	positions.push_back( Vec3f(  0.0f,  1.0f,  0.0f ) );
	positions.push_back( Vec3f( -1.0f, -1.0f,  1.0f ) );
	positions.push_back( Vec3f(  1.0f, -1.0f,  1.0f ) );
	positions.push_back( Vec3f(  0.0f,  1.0f,  0.0f ) );
	positions.push_back( Vec3f(  1.0f, -1.0f,  1.0f ) );
	positions.push_back( Vec3f(  1.0f, -1.0f, -1.0f ) );
	positions.push_back( Vec3f(  0.0f,  1.0f,  0.0f ) );
	positions.push_back( Vec3f(  1.0f, -1.0f, -1.0f ) );
	positions.push_back( Vec3f( -1.0f, -1.0f, -1.0f ) );
	positions.push_back( Vec3f(  0.0f,  1.0f,  0.0f ) );
	positions.push_back( Vec3f( -1.0f, -1.0f, -1.0f ) );
	positions.push_back( Vec3f( -1.0f, -1.0f,  1.0f ) );
	
	vector<ColorAf> colors;
	colors.push_back( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	colors.push_back( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	colors.push_back( ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	colors.push_back( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	
	vector<Vec3f> normals;
	for ( size_t i = 0; i < 12; ++i ) {
		normals.push_back( Vec3f::zero() );
	}
	
	vector<ci::Vec4f> texCoords;
	texCoords.push_back( Vec4f( 0.5f, 1.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 1.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.5f, 1.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 1.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.5f, 1.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 1.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.5f, 1.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 1.0f, 0.0f, 0.0f, 0.0f ) );
	texCoords.push_back( Vec4f( 0.0f, 0.0f, 0.0f, 0.0f ) );
	
	mVboMesh->bufferColors( colors );
	mVboMesh->bufferNormals( normals );
	mVboMesh->bufferTexCoords( texCoords );
	mVboMesh->bufferPositions( positions );
	mVboMesh->bufferIndices( indices );*/
}

void Es2App::update()
{
	float e = (float)getElapsedSeconds();
	
	float c = math<float>::cos( e );
	float s = math<float>::sin( e );
	
	Vec3f v( c, s, c * s );
	mLight.setPosition( v * 2.0f );
}

CINDER_APP_NATIVE( Es2App, RendererGl )
