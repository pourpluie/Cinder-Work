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
	typedef enum { CAPSULE, CONE, CUBE, CYLINDER, HELIX, ICOSAHEDRON, ICOSPHERE, SPHERE, TEAPOT, TORUS } Primitive;

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
	bool				mColored;
	bool				mShowNormals;
	unsigned			mTwist;

	CameraPersp			mCamera;
	MayaCamUI			mMayaCam;

	gl::VertBatchRef	mGrid;

	gl::BatchRef		mPrimitive;
	gl::BatchRef		mPrimitiveWireframe;
	gl::BatchRef		mNormals;

	gl::GlslProgRef		mWireframeShader;

	gl::TextureRef		mTexture;
};

void GeometryApp::setup()
{
	mSelected = SPHERE;
	mSubdivision = 1;
	mWireframe = true;
	mColored = true;
	mShowNormals = true;
	mTwist = 0;

	//
	gl::Texture::Format fmt;
	fmt.setWrap( GL_REPEAT, GL_CLAMP_TO_EDGE );
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

	mCamera.setEyePoint( Vec3f(3, 4, 6) );
	mCamera.setCenterOfInterestPoint( Vec3f(0, 0, 0) );

	mWireframeShader->uniform( "uTexture", 0 );
}

void GeometryApp::update()
{
}

void GeometryApp::draw()
{
	gl::clear( Color::black() );
	gl::setMatrices( mCamera );

	{
		gl::ScopedGlslProg scopedGlslProg( gl::context()->getStockShader( gl::ShaderDef().color() ) );
		
		if(mGrid)
			mGrid->draw();
	}

	if(mNormals && mShowNormals)
		mNormals->draw();

	if(mPrimitive)
	{
		gl::ScopedTextureBind scopedTextureBind( mTexture );
			
		gl::enableAlphaBlending();
		gl::enable( GL_CULL_FACE );

		glCullFace( GL_FRONT );
		if(mWireframe)
			mPrimitiveWireframe->draw();
		else
			mPrimitive->draw();

		glCullFace( GL_BACK );
		if(mWireframe)
			mPrimitiveWireframe->draw();
		else
			mPrimitive->draw();
		
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
	
	if(mWireframeShader)
		mWireframeShader->uniform( "uViewportSize", Vec2f( getWindowSize() ) );
}

void GeometryApp::keyDown( KeyEvent event )
{
	char str[1024];

	switch( event.getCode() )
	{
	case KeyEvent::KEY_SPACE:
		mSelected = static_cast<Primitive>( static_cast<int>(mSelected) + 1 );
		mSubdivision = 1;
		createPrimitive();
		break;
	case KeyEvent::KEY_UP:
		mSubdivision++;
		createPrimitive();
		break;
	case KeyEvent::KEY_DOWN:
		if( mSubdivision > 1 )
			mSubdivision--;
		createPrimitive();
		break;
	case KeyEvent::KEY_c:
		mColored = !mColored;
		createPrimitive();
		break;
	case KeyEvent::KEY_n:
		mShowNormals = !mShowNormals;
		break;
	case KeyEvent::KEY_w:
		mWireframe = !mWireframe;
		break;
	case KeyEvent::KEY_RIGHT:
		mTwist++;
		sprintf_s(str, 1024, "Geometry - Twist: %d", mTwist);
		getWindow()->setTitle( std::string(str) );
		createPrimitive();
		break;
	case KeyEvent::KEY_LEFT:
		if(mTwist > 0) mTwist--;
		sprintf_s(str, 1024, "Geometry - Twist: %d", mTwist);
		getWindow()->setTitle( std::string(str) );
		createPrimitive();
		break;
	}
}

void GeometryApp::createPrimitive(void)
{
	geom::SourceRef primitive;

	switch( mSelected )
	{
	default:
		mSelected = CAPSULE;
	case CAPSULE:
		primitive = geom::SourceRef( new geom::Capsule( geom::Capsule() ) );
		break;
	case CONE:
		primitive = geom::SourceRef( new geom::Cone( geom::Cone() ) );
		break;
	case CUBE:
		primitive = geom::SourceRef( new geom::Cube( geom::Cube() ) );
		break;
	case CYLINDER:
		primitive = geom::SourceRef( new geom::Cylinder( geom::Cylinder() ) );
		break;
	case HELIX:
		primitive = geom::SourceRef( new geom::Helix( geom::Helix().coils(1.5f).height(2) ) );
		break;
	case ICOSAHEDRON:
		primitive = geom::SourceRef( new geom::Icosahedron( geom::Icosahedron() ) );
		break;
	case ICOSPHERE:
		primitive = geom::SourceRef( new geom::Icosphere( geom::Icosphere().subdivision(3) ) );
		break;
	case SPHERE:
		primitive = geom::SourceRef( new geom::Sphere( geom::Sphere() ) );
		break;
	case TEAPOT:
		primitive = geom::SourceRef( new geom::Teapot( geom::Teapot() ) );
		break;
	case TORUS:
		primitive = geom::SourceRef( new geom::Torus( geom::Torus().segmentsRing(12).twist(mTwist) ) );
		break;
	}

	if( mColored )
		primitive->enable( geom::Attrib::COLOR );
	
	TriMesh mesh( *primitive );

	if( mSubdivision > 0 && mSelected != ICOSPHERE ) {
		if( mSelected == SPHERE )
			mesh.subdivide( mSubdivision, true );
		else
			mesh.subdivide( mSubdivision, false );
	}

	mPrimitive = gl::Batch::create( mesh, gl::context()->getStockShader( gl::ShaderDef().color() ) );
	mPrimitiveWireframe = gl::Batch::create( mesh, mWireframeShader );
	mNormals = gl::Batch::create( DebugMesh( mesh, Color(1,1,0) ), gl::context()->getStockShader( gl::ShaderDef().color() ) );
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
				"	vec4 vFaceColor = texture( uTexture, vVertexIn.texcoord ) * vVertexIn.color; vFaceColor.a = 0.85;\n"
				"	vec4 vEdgeColor = vec4(1.0, 1.0, 1.0, 0.85);\n"
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
