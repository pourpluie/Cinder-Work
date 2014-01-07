#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"

#include "Xfo.h"
#include "TestContext.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TransformFeedbackSingleObjectApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	
	void setupShaders();
	void setupBuffers();
	
	gl::GlslProgRef mGlsl;
	gl::VaoRef		mVao;
	gl::VboRef		mVbo;
	gl::VboRef		mTransformVbo;
	gl::XfoRef		mXfo;
};

void TransformFeedbackSingleObjectApp::setup()
{
	setupShaders();
	setupBuffers();
}

void TransformFeedbackSingleObjectApp::setupShaders()
{
	std::vector<std::string> varyings( { "outValue" } );
	gl::GlslProg::Format mFormat;
	mFormat.vertex( loadAsset( "basic.vert" ) );
	//.geometry( loadAsset( "basic.geom" ) );
	mFormat.feedbackFormat( GL_SEPARATE_ATTRIBS ).feedbackVaryings( varyings );
	try {
		mGlsl = gl::GlslProg::create( mFormat );
		mGlsl->bind();
	} catch ( gl::GlslProgCompileExc ex) {
		cout << ex.what() << endl;
		shutdown();
	}
}

void TransformFeedbackSingleObjectApp::setupBuffers()
{
	mVao = gl::Vao::create();
	mVao->bind();
	
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	
	mVbo = gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW );
	mVbo->bind();
	
	gl::enableVertexAttribArray(0);
	gl::vertexAttribPointer( 0, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0 );
	
	mTransformVbo = gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(data), nullptr, GL_STATIC_READ );
	
	// So the create method taking a param won't be there in finished version,
	// just wanted to make an easy switch for testing. Just uncomment one of the
	// forms below
	
	// Test for Hardware Solution
//	mXfo = gl::Xfo::create( false );
//	mXfo->bind();
//	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTransformVbo );
	
	// Test for Software Solution
//	mXfo = gl::Xfo::create( true );
//	mXfo->bind();
//	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTransformVbo );
	
	// System Cached version
//	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTransformVbo );
	
	gl::enable( GL_RASTERIZER_DISCARD );
	
	gl::beginTransformFeedback( GL_POINTS );
	gl::drawArrays( GL_POINTS, 0, 5 );
	gl::endTransformFeedback();
	
	gl::disable( GL_RASTERIZER_DISCARD );
	
    glFlush();
	
    // Fetch and print results
    GLfloat feedback[5];
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(feedback), feedback);
	
    printf("%f %f %f %f %f\n", feedback[0], feedback[1], feedback[2], feedback[3], feedback[4]);
}

void TransformFeedbackSingleObjectApp::mouseDown( MouseEvent event )
{
}

void TransformFeedbackSingleObjectApp::update()
{
	
}

void TransformFeedbackSingleObjectApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( TransformFeedbackSingleObjectApp, RendererGl )
