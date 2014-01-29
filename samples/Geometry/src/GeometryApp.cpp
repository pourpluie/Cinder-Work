#include "cinder/Camera.h"
#include "cinder/GeomIo.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"

#include "DebugMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryApp : public AppNative
{
public:
	typedef enum Primitive { SPHERE, CAPSULE, ICOSPHERE,/* CONE, WEDGE,*/ CUBE, TEAPOT };

	void setup();
	void update();
	void draw();

	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );

	void keyDown( KeyEvent event );

	void resize();
private:
	void createShader();
	void createPrimitive();

	Primitive			mSelected;
	uint8_t				mSubdivision;
	bool				mWireframe;

	CameraPersp			mCamera;
	MayaCamUI			mMayaCam;

	gl::VertBatchRef	mGrid;

	gl::VboMeshRef		mPrimitive;
	gl::VboMeshRef		mOriginalNormals;
	gl::VboMeshRef		mCalculatedNormals;

	gl::GlslProgRef		mWireframeShader;

	gl::TextureRef		mTexture;
};

void GeometryApp::setup()
{
	mSelected = ICOSPHERE;
	mSubdivision = 0;
	mWireframe = true;

	//
	gl::Texture::Format fmt;
	fmt.setWrap( GL_REPEAT, GL_CLAMP_TO_BORDER );
	mTexture = gl::Texture::create( loadImage( loadAsset("stripes.jpg") ), fmt );

	//
	gl::enableDepthRead();
	gl::enableDepthWrite();

	//
	mGrid = gl::VertBatch::create( GL_LINES );
	mGrid->begin( GL_LINES );
	mGrid->color( Color(0.5f, 0.5f, 0.5f) ); mGrid->vertex( -10.0f, 0.0f, 0.0f );
	mGrid->color( Color(0.5f, 0.5f, 0.5f) ); mGrid->vertex( 0.0f, 0.0f, 0.0f );
	mGrid->color( Color(1, 0, 0) ); mGrid->vertex( 0.0f, 0.0f, 0.0f );
	mGrid->color( Color(1, 0, 0) ); mGrid->vertex( 20.0f, 0.0f, 0.0f );
	mGrid->color( Color(0, 1, 0) ); mGrid->vertex( 0.0f, 0.0f, 0.0f );
	mGrid->color( Color(0, 1, 0) ); mGrid->vertex( 0.0f, 20.0f, 0.0f );
	mGrid->color( Color(0.5f, 0.5f, 0.5f) ); mGrid->vertex( 0.0f, 0.0f, -10.0f );
	mGrid->color( Color(0.5f, 0.5f, 0.5f) ); mGrid->vertex( 0.0f, 0.0f, 0.0f );
	mGrid->color( Color(0, 0, 1) ); mGrid->vertex( 0.0f, 0.0f, 0.0f );
	mGrid->color( Color(0, 0, 1) ); mGrid->vertex( 0.0f, 0.0f, 20.0f );
	for( int i = -10; i <= 10; ++i ) {
		if( i == 0 )
			continue;

		mGrid->color( Color(0.5f, 0.5f, 0.5f) );
		mGrid->color( Color(0.5f, 0.5f, 0.5f) );
		mGrid->color( Color(0.5f, 0.5f, 0.5f) );
		mGrid->color( Color(0.5f, 0.5f, 0.5f) );
		
		mGrid->vertex( float(i), 0.0f, -10.0f );
		mGrid->vertex( float(i), 0.0f, +10.0f );
		mGrid->vertex( -10.0f, 0.0f, float(i) );
		mGrid->vertex( +10.0f, 0.0f, float(i) );
	}
	mGrid->end();

	//
	createShader();
	createPrimitive();

	mCamera.setEyePoint( Vec3f(0, 2, 4) );
	mCamera.setCenterOfInterestPoint( Vec3f(0, 0, 0) );

	mWireframeShader->uniform( "uTexture", 0 );
}

void GeometryApp::update()
{
	float t = static_cast<float>( getElapsedSeconds() );

	Vec3f p( 5.0f * sinf(t), 5.0f * cos(t), 5.0f * sin(t * 0.7f) );
}

void GeometryApp::draw()
{
	gl::clear( Color::black() );
	gl::setMatrices( mCamera );

	{
		gl::GlslProgScope glslProgScope( gl::context()->getStockShader( gl::ShaderDef().color() ) );
		
		if(mGrid)
			mGrid->draw();

		if(mOriginalNormals)
			gl::draw( mOriginalNormals );

		if(mCalculatedNormals)
			gl::draw( mCalculatedNormals );
	}

	if(mPrimitive)
	{
		try {
			gl::TextureBindScope textureBindScope( mTexture );

			gl::GlslProgScope glslProgScope( mWireframe ? mWireframeShader :
				gl::context()->getStockShader( gl::ShaderDef().texture( mTexture ) ) );

			gl::enableAlphaBlending();
			gl::enable( GL_CULL_FACE );

			glCullFace( GL_FRONT );
			gl::draw( mPrimitive );

			glCullFace( GL_BACK );
			gl::draw( mPrimitive );
		
			gl::disable( GL_CULL_FACE );
			gl::disableAlphaBlending();
		}
		catch( const std::exception &e ) {
			console() << e.what() << std::endl;
		}
	}
}

void GeometryApp::mouseDown( MouseEvent event )
{
	mMayaCam.setCurrentCam( mCamera );
	mMayaCam.mouseDown( event.getPos() );
}

void GeometryApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCam.getCamera();
}

void GeometryApp::resize(void)
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
	
	if(mWireframeShader)
		mWireframeShader->uniform( "uViewportSize", Vec2f( getWindowSize() ) );
}

void GeometryApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_SPACE:
		mSelected = static_cast<Primitive>( static_cast<int>(mSelected) + 1 );
		createPrimitive();
		break;
	case KeyEvent::KEY_UP:
		mSubdivision++;
		createPrimitive();
		break;
	case KeyEvent::KEY_DOWN:
		if( mSubdivision > 0 )
			mSubdivision--;
		createPrimitive();
		break;
	case KeyEvent::KEY_RETURN:
		mWireframe = !mWireframe;
		break;
	}
}

void GeometryApp::createPrimitive(void)
{
	geom::SourceRef primitive;

	try {
		switch( mSelected )
		{
		default:
			mSelected = SPHERE;
		case SPHERE:
			primitive = geom::SourceRef( new geom::Sphere( geom::Sphere().segments(40) ) );
			break;
		case CAPSULE:
			primitive = geom::SourceRef( new geom::Capsule( geom::Capsule().segments(40).length(4.0f) ) );
			break;
		case ICOSPHERE:
			primitive = geom::SourceRef( new geom::IcoSphere( geom::IcoSphere().subdivision( mSubdivision ) ) );
			break;
		/*case CONE:
			primitive = geom::SourceRef( new geom::Cone( geom::Cone() ) );
			break;*/
		case CUBE:
			primitive = geom::SourceRef( new geom::Cube( geom::Cube().subdivision( mSubdivision ) ) );
			break;
		case TEAPOT:
			primitive = geom::SourceRef( new geom::Teapot( geom::Teapot().subdivision( mSubdivision ) ) );
			break;
		}
	
		mPrimitive = gl::VboMesh::create( *primitive );
		/*
		TriMesh mesh( *primitive );
		mOriginalNormals = gl::VboMesh::create( DebugMesh( mesh, Color(1,1,0) ) );

		mesh.recalculateNormalsHighQuality();
		mCalculatedNormals = gl::VboMesh::create( DebugMesh( mesh, Color(0,1,1) ) );
		*/
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}
}

void GeometryApp::createShader(void)
{
	try {
	mWireframeShader = gl::GlslProg::create( gl::GlslProg::Format()
		.vertex(
			"#version 150\n"
			"\n"
			"uniform mat4	ciModelViewProjection;\n"
			"in vec4		ciPosition;\n"
			"in vec4		ciColor;\n"
			"in vec2		ciTexCoord0;\n"
			"\n"
			"out VertexData {\n"
			"	vec4 color;\n"
			"	vec2 texcoord;\n"
			"} vVertexOut;\n"
			"\n"
			"void main(void) {\n"
			"	vVertexOut.color = ciColor;\n"
			"	vVertexOut.texcoord = ciTexCoord0;\n"
			"	gl_Position = ciModelViewProjection * ciPosition;\n"
			"}\n"
		) 
		.geometry(
			"#version 150\n"
			"\n"
			"layout (triangles) in;\n"
			"layout (triangle_strip, max_vertices = 3) out;\n"
			"\n"
			"uniform vec2 			uViewportSize;\n"
			"\n"
			"in VertexData	{\n"
			"	vec4 color;\n"
			"	vec2 texcoord;\n"
			"} vVertexIn[];\n"
			"\n"
			"out VertexData	{\n"
			"	noperspective vec3 distance;\n"
			"	vec4 color;\n"
			"	vec2 texcoord;\n"
			"} vVertexOut;\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	// taken from 'Single-Pass Wireframe Rendering'\n"
			"	vec2 p0 = uViewportSize * gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;\n"
			"	vec2 p1 = uViewportSize * gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;\n"
			"	vec2 p2 = uViewportSize * gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w;\n"
			"\n"
			"	vec2 v0 = p2-p1;\n"
			"	vec2 v1 = p2-p0;\n"
			"	vec2 v2 = p1-p0;\n"
			"	float fArea = abs(v1.x*v2.y - v1.y * v2.x);\n"
			"\n"
			"	vVertexOut.distance = vec3(fArea/length(v0),0,0);\n"
			"	vVertexOut.color = vVertexIn[0].color;\n"
			"	vVertexOut.texcoord = vVertexIn[0].texcoord;\n"
			"	gl_Position = gl_in[0].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	vVertexOut.distance = vec3(0,fArea/length(v1),0);\n"
			"	vVertexOut.color = vVertexIn[1].color;\n"
			"	vVertexOut.texcoord = vVertexIn[1].texcoord;\n"
			"	gl_Position = gl_in[1].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	vVertexOut.distance = vec3(0,0,fArea/length(v2));\n"
			"	vVertexOut.color = vVertexIn[2].color;\n"
			"	vVertexOut.texcoord = vVertexIn[2].texcoord;\n"
			"	gl_Position = gl_in[2].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	EndPrimitive();\n"
			"}\n"
		) 
		.fragment(
			"#version 150\n"
			"\n"
			"uniform sampler2D uTexture;\n"
			"\n"
			"in VertexData	{\n"
			"	noperspective vec3 distance;\n"
			"	vec4 color;\n"
			"	vec2 texcoord;\n"
			"} vVertexIn;\n"
			"\n"
			"out vec4				oColor;\n"
			"\n"
			"void main(void) {\n"
			"	// determine frag distance to closest edge\n"
			"	float fNearest = min(min(vVertexIn.distance[0],vVertexIn.distance[1]),vVertexIn.distance[2]);\n"
			"	float fEdgeIntensity = exp2(-1.0*fNearest*fNearest);\n"
			"\n"
			"	// blend between edge color and face color\n"
			"	vec4 vFaceColor = texture2D( uTexture, vVertexIn.texcoord ); vFaceColor.a = 0.9;\n"
			//"	vec4 vFaceColor = vec4( vVertexIn.texcoord.x, vVertexIn.texcoord.y, 0.0, 1.0 );\n"
			//"	vec4 vEdgeColor; vEdgeColor.rgb = vVertexIn.color.rgb; vEdgeColor.a = 1.0;\n"
			"	vec4 vEdgeColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
			"	oColor = mix(vFaceColor, vEdgeColor, fEdgeIntensity);\n"
			"}\n"
		)
	);
	}
	catch( const std::exception& e ) {
		console() << e.what() << std::endl;
	}
}

CINDER_APP_NATIVE( GeometryApp, RendererGl )
