#include "cinder/Camera.h"
#include "cinder/GeomIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"

#include "DebugMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryApp : public AppNative
{
public:
	typedef enum Primitive { SPHERE, CAPSULE, CONE, WEDGE, CUBE };

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

	CameraPersp			mCamera;
	MayaCamUI			mMayaCam;

	gl::VertBatchRef	mGrid;

	gl::VboMeshRef		mPrimitive;
	gl::VboMeshRef		mOriginalNormals;
	gl::VboMeshRef		mCalculatedNormals;

	gl::GlslProgRef		mWireframeShader;
};

void GeometryApp::setup()
{
	mSelected = Primitive::SPHERE;

	//
	gl::enableDepthRead();
	gl::enableDepthWrite();

	//
	mGrid = gl::VertBatch::create( GL_LINES );
	mGrid->begin( GL_LINES );
	mGrid->color( Color(1, 0, 0) ); mGrid->vertex( 0.0f, 0.0f, 0.0f ); 
	mGrid->color( Color(1, 0, 0) ); mGrid->vertex( 10.0f, 0.0f, 0.0f ); 
	mGrid->color( Color(0, 1, 0) ); mGrid->vertex( 0.0f, 0.0f, 0.0f ); 
	mGrid->color( Color(0, 1, 0) ); mGrid->vertex( 0.0f, 10.0f, 0.0f ); 
	mGrid->color( Color(0, 0, 1) ); mGrid->vertex( 0.0f, 0.0f, 0.0f ); 
	mGrid->color( Color(0, 0, 1) ); mGrid->vertex( 0.0f, 0.0f, 10.0f ); 
	for( int i = -10; i <= 10; ++i ) {
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

	if(mGrid)
	{
		gl::GlslProgScope glslProgScope( gl::context()->getStockShader( gl::ShaderDef().color() ) );		
		mGrid->draw();
	}

	if(mOriginalNormals && mCalculatedNormals)
	{
		gl::GlslProgScope glslProgScope( gl::context()->getStockShader( gl::ShaderDef().color() ) );
		gl::draw( mOriginalNormals );
		gl::draw( mCalculatedNormals );
	}

	if(mPrimitive)
	{
		gl::GlslProgScope glslProgScope( mWireframeShader );
		mWireframeShader->uniform( "uViewportSize", Vec2f( getWindowSize() ) );

		gl::enableAlphaBlending();
		gl::enable( GL_CULL_FACE );

		glCullFace( GL_FRONT );
		gl::draw( mPrimitive );

		glCullFace( GL_BACK );
		gl::draw( mPrimitive );
		
		gl::disable( GL_CULL_FACE );
		gl::disableAlphaBlending();
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
}

void GeometryApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_SPACE:
		mSelected = static_cast<Primitive>( static_cast<int>(mSelected) + 1 );
		createPrimitive();
		break;
	}
}

void GeometryApp::createPrimitive(void)
{
	switch( mSelected )
	{
	default:
	case SPHERE:
		mSelected = SPHERE;
		mPrimitive = gl::VboMesh::create( geom::Sphere() );
		break;
	case CAPSULE:
		mPrimitive = gl::VboMesh::create( geom::Capsule().length(2.0f) );
		break;
	}
/*
	TriMesh mesh(primitive);
	mOriginalNormals = gl::VboMesh::create( DebugMesh( mesh, Color(0,1,0) ) );

	mesh.recalculateNormals();
	mCalculatedNormals = gl::VboMesh::create( DebugMesh( mesh, Color(1,0,1) ) );
*/
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
			"\n"
			"void main(void) {\n"
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
			"noperspective out vec3	vDistance;\n"
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
			"	vDistance = vec3(fArea/length(v0),0,0);\n"
			"	gl_Position = gl_in[0].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	vDistance = vec3(0,fArea/length(v1),0);\n"
			"	gl_Position = gl_in[1].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	vDistance = vec3(0,0,fArea/length(v2));\n"
			"	gl_Position = gl_in[2].gl_Position;\n"
			"	EmitVertex();\n"
			"\n"
			"	EndPrimitive();\n"
			"}\n"
		) 
		.fragment(
			"#version 150\n"
			"\n"
			"noperspective in vec3	vDistance;\n"
			"out vec4				oColor;\n"
			"\n"
			"void main(void) {\n"
			"	// determine frag distance to closest edge\n"
			"	float fNearest = min(min(vDistance[0],vDistance[1]),vDistance[2]);\n"
			"	float fEdgeIntensity = exp2(-1.0*fNearest*fNearest);\n"
			"\n"
			"	// blend between edge color and face color\n"
			"	const vec4 vFaceColor = vec4(0.2, 0.2, 0.2, 0.7);\n"
			"	const vec4 vEdgeColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
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
