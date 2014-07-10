/*
 Copyright (c) 2012-2014, Paul Houx
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

/*
	This sample will show how to use the CameraStereo class to setup and render stereoscopic images.
	The camera contains different matrices for the left and right eye of the viewer. By rendering the scene
	twice, once for each eye, we can view the scene in 3D on monitors or televisions that support 3D.

	Here, we divide the window into a left and right half and render the scene to each half. This is called
	side-by-side stereoscopic and is supported by most 3D televisions. Simply connect your computer to
	such a television, run the sample in full screen and enable the TV's 3D mode.

	When creating your own stereoscopic application, be careful how you choose your convergence.
	An excellent article can be found here:
	http://paulbourke.net/miscellaneous/stereographics/stereorender/

	The CameraStereo class is based on the Off-Axis method described in this article. 
*/

#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"

#include "cinder/Camera.h"
#include "cinder/Font.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"

#include "AutoFocuser.h"

#include <boost/format.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

class StereoscopicRenderingApp : public AppBasic {
  public:
	typedef enum { SET_CONVERGENCE, SET_FOCUS, AUTO_FOCUS } FocusMethod;
	typedef enum { MONO, ANAGLYPH_RED_CYAN, SIDE_BY_SIDE, OVER_UNDER, INTERLACED_HORIZONTAL } RenderMethod;
public:
	void prepareSettings( Settings *settings );

	void setup();	
	void update();
	void draw();

	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );

	void keyDown( KeyEvent event );

	void resize();
private:
	void createFbo();

	void renderAnaglyph( const Vec2i &size, const ColorA &left, const ColorA &right );
	void renderSideBySide( const Vec2i &size );
	void renderOverUnder( const Vec2i &size );
	void renderInterlacedHorizontal( const Vec2i &size );

	void render();
	void renderUI();
private:
	static const int NUM_NOTES = 201;

	bool					mDrawUI;
	bool					mDrawAutoFocus;

	FocusMethod				mFocusMethod;
	RenderMethod			mRenderMethod;

	MayaCamUI				mMayaCam;
	CameraStereo			mCamera;

	AutoFocuser				mAF;

	gl::GlslProgRef			mShaderPhong;
	gl::GlslProgRef			mShaderInstancedPhong;
	gl::GlslProgRef			mShaderAnaglyph;
	gl::GlslProgRef			mShaderInterlaced;

	gl::BatchRef			mTrombone;
	gl::BatchRef			mNote;
	gl::BatchRef			mFloor;
	gl::BatchRef			mGrid;
	
	gl::VboRef				mInstanceDataVbo;

	gl::FboRef				mFbo;

	Color					mColorBackground;

	Font					mFont;

	// Will we draw all notes with a single draw call. This
	// structure contains the data unique for each note.
	// Do not change the order of the data! The Vec3f position
	// and float rotation will be packed into a Vec4f when
	// sent to the shader.
	struct InstanceData
	{
		Vec3f   position; // 3 floats
		float   rotation; // 1 float

		ColorAf color;    // 4 floats
	};
};

void StereoscopicRenderingApp::prepareSettings( Settings *settings )
{
	// Create a 16:9 window
	settings->setWindowSize(960, 540);
	settings->setTitle("Stereoscopic Rendering");

	// Allow high frame rates to test performance
	settings->setFrameRate( 300.0f );
}

void StereoscopicRenderingApp::setup()
{
	// Enable stereoscopic rendering
	mRenderMethod = SIDE_BY_SIDE;

	// Enable auto-focussing
	mFocusMethod = AUTO_FOCUS;
	mDrawAutoFocus = false;

	// Setup the camera
	mCamera.setEyePoint( Vec3f(0.2f, 1.3f, -11.5f) );
	mCamera.setCenterOfInterestPoint( Vec3f(0.5f, 1.5f, -0.1f) );
	mCamera.setFov( 60.0f );

	mMayaCam.setCurrentCam( mCamera );

	// Load shader(s)
	try {
		mShaderPhong = gl::GlslProg::create( loadAsset("shaders/phong_vert.glsl"), loadAsset("shaders/phong_frag.glsl") );
		mShaderInstancedPhong = gl::GlslProg::create( loadAsset("shaders/instanced_phong_vert.glsl"), loadAsset("shaders/phong_frag.glsl") );
		mShaderAnaglyph = gl::GlslProg::create( loadAsset("shaders/anaglyph_vert.glsl"), loadAsset("shaders/anaglyph_frag.glsl") );
		mShaderInterlaced = gl::GlslProg::create( loadAsset("shaders/interlaced_vert.glsl"), loadAsset("shaders/interlaced_frag.glsl") );
	}
	catch( const std::exception &e ) {
		// Something went wrong, display error and quit
		console() << e.what() << std::endl;
		quit();
	}

	// Create models
	try {
		// Create floor from basic cube
		mFloor = gl::Batch::create( geom::Cube(), gl::getStockShader( gl::ShaderDef().color() ) );

		// Load and create trombone
		TriMeshRef	triMesh = TriMesh::create();
		triMesh->read( loadAsset("models/trombone.msh") );
		mTrombone = gl::Batch::create( triMesh, mShaderPhong );
		
		// Load music note
		{
			triMesh->read( loadAsset("models/note.msh") );

			// Create instanced data for the music notes
			std::vector<InstanceData> instanceData(NUM_NOTES);
			mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW );

			geom::BufferLayout instanceDataLayout;
			instanceDataLayout.append( geom::Attrib::CUSTOM_0, 4, sizeof(InstanceData), 0, 1 ); // position and rotation packed together as Vec4f
			instanceDataLayout.append( geom::Attrib::CUSTOM_1, 4, sizeof(InstanceData), sizeof(Vec4f), 1 ); // color as Vec4f, with correct offset

			gl::Batch::AttributeMapping mapping;
			mapping[geom::Attrib::CUSTOM_0] = "vInstancePosition";
			mapping[geom::Attrib::CUSTOM_1] = "vInstanceColor";

			// Append per instance data
			gl::VboMeshRef vboMesh = gl::VboMesh::create( *triMesh.get() );
			vboMesh->appendVbo( instanceDataLayout, mInstanceDataVbo );

			// Create music note
			mNote = gl::Batch::create( vboMesh, mShaderInstancedPhong, mapping );
		}

		// Create grid of lines (yup, it's painful to no longer have a gl::drawLine() convenience method)
		{
			std::vector<Vec3f> vertices;
			for(int i=-100; i<=100; ++i) {
				vertices.push_back( Vec3f((float) i, 0, -100) );
				vertices.push_back( Vec3f((float) i, 0,  100) );
				vertices.push_back( Vec3f(-100, 0, (float) i) );
				vertices.push_back( Vec3f( 100, 0, (float) i) );
			}

			geom::BufferLayout bufferLayout;
			bufferLayout.append( geom::Attrib::POSITION, 3, 0, 0 ); // we only provide vertices (positions)

			gl::VboRef vertexDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, 3 * sizeof(float) * vertices.size(), vertices.data() );

			std::vector<pair<geom::BufferLayout, gl::VboRef>> buffers;
			buffers.push_back( make_pair( bufferLayout, vertexDataVbo ) );

			gl::VboMeshRef grid = gl::VboMesh::create( vertices.size(), GL_LINES, buffers );
			mGrid = gl::Batch::create( grid, gl::getStockShader( gl::ShaderDef().color() ) );
		}
	}
	catch( const std::exception &e ) {
		// Something went wrong, display error and quit
		console() << e.what() << std::endl;
		quit();
	}

	mColorBackground = Color( 0.8f, 0.8f, 0.8f );

	mFont = Font("Verdana", 24.0f);
	mDrawUI = true;
}

void StereoscopicRenderingApp::update()
{
	float	d, f;

	// note: viewport is defined with the lower left corner as the origin, whereas area is
	//  defined with the upper left corner as the origin. It's safe to ignore that difference for now.
	const auto viewport = gl::getViewport();
	Area       area = Area( viewport.first, viewport.first + viewport.second );

	switch( mFocusMethod )
	{
	case SET_CONVERGENCE:
		// Auto-focus by calculating distance to center of interest
		d = (mCamera.getCenterOfInterestPoint() - mCamera.getEyePoint()).length();
		f = math<float>::min( 5.0f, d * 0.5f );

		// The setConvergence() method will not change the eye separation distance, 
		// which may cause the parallax effect to become uncomfortably big. 
		mCamera.setConvergence( f );
		mCamera.setEyeSeparation( 0.05f );
		break;
	case SET_FOCUS:
		// Auto-focus by calculating distance to center of interest
		d = (mCamera.getCenterOfInterestPoint() - mCamera.getEyePoint()).length();
		f = math<float>::min( 5.0f, d * 0.5f );

		// The setConvergence( value, true ) method will automatically calculate a fitting value for the eye separation distance.
		// There is still no guarantee that the parallax effect stays within comfortable levels,
		// because there may be objects very near to the camera compared to the point we are looking at.
		mCamera.setConvergence( f, true );
		break;
	case AUTO_FOCUS:
		// Here, we use the gl::StereoAutoFocuser class to determine the best focal length,
		// based on the contents of the current depth buffer. This is by far the best method of
		// the three, because it guarantees the parallax effect will never be out of bounds.
		// Depending on the rendering method, we can sample different area's of the screen
		// to optimally detect details. This is not required, however.
		// Use the UP and DOWN keys to adjust the intensity of the parallax effect.
		switch( mRenderMethod ) 
		{
		case MONO:
			break;
		case SIDE_BY_SIDE:
			// Sample half the left eye, half the right eye
			area.expand( -area.getWidth()/4, 0 );
			mAF.autoFocus( &mCamera, area );
			break;
		case OVER_UNDER:
			// Sample half the left eye, half the right eye
			area.expand( 0, -area.getHeight()/4 );
			mAF.autoFocus( &mCamera, area );
			break;
		case ANAGLYPH_RED_CYAN:
		case INTERLACED_HORIZONTAL:
			// Sample the depth buffer of one of the FBO's
			mAF.autoFocus( &mCamera, mFbo );
			break;
		}
		break;
	}

	// Update our music notes
	InstanceData *data = reinterpret_cast<InstanceData*>( mInstanceDataVbo->map( GL_WRITE_ONLY ) );
	size_t numInstances = mInstanceDataVbo->getSize() / sizeof(InstanceData);

	Rand rnd;
	float seconds = (float) getElapsedSeconds();
	for( size_t i=0; i<numInstances; ++i ) {
		rnd.seed(i + 1);

		int x = i - NUM_NOTES/2;
		float t = rnd.nextFloat() * 200.0f + 2.0f * seconds;
		float r = rnd.nextFloat() * 360.0f + 120.0f * seconds;
		float z = fmodf( 5.0f * t, 200.0f ) - 100.0f;

		data->position = Vec3f( x * 0.5f, 0.15f + 1.0f * math<float>::abs( sinf(3.0f * t) ), -z );
		data->rotation = toRadians(r);
		data->color = Color( CM_HSV, rnd.nextFloat(), 1.0f, 1.0f );

		data++;
	}
	mInstanceDataVbo->unmap();
}

void StereoscopicRenderingApp::draw()
{
	// Clear color and depth buffers
	gl::clear( mColorBackground ); 
	
	// Stereoscopic rendering
	switch( mRenderMethod ) 
	{
	case MONO:
		// Render mono camera
		mCamera.disableStereo();
		render();
		break;
	case ANAGLYPH_RED_CYAN:
		renderAnaglyph( getWindowSize(), Color(1, 0, 0), Color(0, 1, 1) );
		break;
	case SIDE_BY_SIDE:
		renderSideBySide( getWindowSize() );
		break;
	case OVER_UNDER:
		renderOverUnder( getWindowSize() );
		break;
	case INTERLACED_HORIZONTAL:
		renderInterlacedHorizontal( getWindowSize() );
		break;
	}
}

void StereoscopicRenderingApp::mouseDown( MouseEvent event )
{
	// Handle camera
	mMayaCam.setCurrentCam( mCamera );
	mMayaCam.mouseDown( event.getPos() );
}

void StereoscopicRenderingApp::mouseDrag( MouseEvent event )
{
	// Handle camera
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	
	// Update stereoscopic camera
	Vec3f eye = mMayaCam.getCamera().getEyePoint();
	Vec3f coi = mMayaCam.getCamera().getCenterOfInterestPoint();
	Vec3f dir = coi - eye;

	eye.y = math<float>::max(eye.y, 1.0f);
	coi = eye + dir;

	mCamera.setEyePoint( eye );
	mCamera.setCenterOfInterestPoint( coi );
}

void StereoscopicRenderingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		// Toggle full screen
		setFullScreen( ! isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		// Toggle vertical sync
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_d:
		// Toggle visualizer
		mDrawAutoFocus = !mDrawAutoFocus;
		break;
	case KeyEvent::KEY_u:
		// Toggle interface
		mDrawUI = !mDrawUI;
		break;
	case KeyEvent::KEY_UP:
		// Increase the parallax effect (towards negative parallax) 
		if(mFocusMethod == AUTO_FOCUS)
			mAF.setDepth( mAF.getDepth() + 0.01f );
		break;
	case KeyEvent::KEY_DOWN:
		// Decrease the parallax effect (towards positive parallax) 
		if(mFocusMethod == AUTO_FOCUS)
			mAF.setDepth( mAF.getDepth() - 0.01f );
		break;
	case KeyEvent::KEY_SPACE:
		// Reset the parallax effect to 'no parallax for the nearest object'
		mAF.setDepth( 1.0f );
		break;
	case KeyEvent::KEY_LEFT:
		// Reduce the auto focus speed
		mAF.setSpeed( mAF.getSpeed() - 0.01f );
		break;
	case KeyEvent::KEY_RIGHT:
		// Increase the auto focus speed
		mAF.setSpeed( mAF.getSpeed() + 0.01f );
		break;
	case KeyEvent::KEY_1:
		mFocusMethod = SET_CONVERGENCE;
		break;
	case KeyEvent::KEY_2:
		mFocusMethod = SET_FOCUS;
		break;
	case KeyEvent::KEY_3:
		mFocusMethod = AUTO_FOCUS;
		break;
	case KeyEvent::KEY_F1:
		mRenderMethod = MONO;
		createFbo();
		break;
	case KeyEvent::KEY_F2:
		mRenderMethod = ANAGLYPH_RED_CYAN;
		createFbo();
		break;
	case KeyEvent::KEY_F3:
		mRenderMethod = SIDE_BY_SIDE;
		createFbo();
		break;
	case KeyEvent::KEY_F4:
		mRenderMethod = OVER_UNDER;
		createFbo();
		break;
	case KeyEvent::KEY_F5:
		mRenderMethod = INTERLACED_HORIZONTAL;
		createFbo();
		break;
	}
}

void StereoscopicRenderingApp::resize()
{
	// Make sure the camera's aspect ratio remains correct
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( mCamera );

	// Create/resize the Frame Buffer Object required for some of the render methods
	createFbo();
}

void StereoscopicRenderingApp::createFbo()
{
	Vec2i size = getWindowSize();

	// Set texture format
	gl::Texture::Format tfmt;
	tfmt.setMagFilter( GL_LINEAR );

	gl::Fbo::Format fmt;
	fmt.setColorTextureFormat( tfmt );
	fmt.setSamples(16);
	fmt.setCoverageSamples(16);

	switch( mRenderMethod )
	{
	case ANAGLYPH_RED_CYAN: 
		// By doubling the horizontal resolution, we can effectively render
		// both the left and right eye views side by side at full resolution
		mFbo = gl::Fbo::create( size.x * 2, size.y, fmt );
		mFbo->getColorTexture()->setFlipped(true);
		break;
	default:
		mFbo = gl::Fbo::create( size.x, size.y, fmt );
		mFbo->getColorTexture()->setFlipped(true);
		break;
	}
}

void StereoscopicRenderingApp::renderAnaglyph(  const Vec2i &size, const ColorA &left, const ColorA &right )
{
	// Bind the FBO and clear its buffer
	mFbo->bindFramebuffer();
	gl::clear( mColorBackground );

	// Render the scene using the side-by-side technique
	renderSideBySide( mFbo->getSize() );

	// Unbind the FBO
	mFbo->unbindFramebuffer();

	// Enable the anaglyph shader
	mShaderAnaglyph->bind();
	mShaderAnaglyph->uniform( "uColorTex", 0 );
	mShaderAnaglyph->uniform( "uColorLeftEye", left );
	mShaderAnaglyph->uniform( "uColorRightEye", right );

	// Bind the FBO texture and draw a flipped full screen rectangle
	mFbo->getColorTexture()->bind(0);
	gl::drawSolidRect( Rectf(0, float(size.y), float(size.x), 0) );
}

void StereoscopicRenderingApp::renderSideBySide( const Vec2i &size )
{
	// Store current viewport
	gl::pushViewport( gl::getViewport() );

	// Draw to left half of window only
	gl::viewport( Vec2i(0, 0), Vec2i(size.x / 2, size.y) );

	// Render left camera
	mCamera.enableStereoLeft();
	render();

	// Draw to right half of window only
	gl::viewport( Vec2i(size.x / 2, 0), Vec2i(size.x / 2, size.y) );

	// Render right camera
	mCamera.enableStereoRight();
	render();

	// Restore viewport
	gl::popViewport();
}

void StereoscopicRenderingApp::renderOverUnder( const Vec2i &size )
{
	// Store current viewport
	gl::pushViewport( gl::getViewport() );

	// Draw to top half of window only
	gl::viewport( Vec2i(0, 0), Vec2i(size.x, size.y / 2) );

	// Render left camera
	mCamera.enableStereoLeft();
	render();

	// Draw to bottom half of window only
	gl::viewport( Vec2i(0, size.y / 2), Vec2i(size.x, size.y / 2) );

	// Render right camera
	mCamera.enableStereoRight();
	render();

	// Restore viewport
	gl::popViewport();
}

void StereoscopicRenderingApp::renderInterlacedHorizontal( const Vec2i &size )
{
	// Bind the FBO and clear its buffer
	mFbo->bindFramebuffer();
	gl::clear( mColorBackground );

	// Render the scene using the over-under technique
	renderOverUnder( mFbo->getSize() );

	// Unbind the FBO
	mFbo->unbindFramebuffer();

	// Enable the interlace shader
	mShaderInterlaced->bind();
	mShaderInterlaced->uniform( "uColorTex", 0 );
	mShaderInterlaced->uniform( "uWindowMetrics", Vec4f( getWindowPosX(), getWindowPosY(), getWindowWidth(), getWindowHeight() ) );

	// Bind the FBO texture and draw a flipped full screen rectangle
	mFbo->getColorTexture()->bind(0);
	gl::drawSolidRect( Rectf(0, float(size.y), float(size.x), 0) );
}

void StereoscopicRenderingApp::render()
{	
	float seconds = (float) getElapsedSeconds();

	// Enable 3D rendering
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// Set 3D camera matrices
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	if( mShaderPhong && mTrombone && mNote ) {
		// Draw trombone
		gl::pushModelViewMatrices();
		{
			gl::color( Color(0.7f, 0.6f, 0.0f) );
			gl::rotate( 10.0f * seconds, Vec3f::yAxis() );
			mTrombone->draw();

			// reflection
			gl::scale( 1.0f, -1.0f, 1.0f );
			mTrombone->draw();
		}
		gl::popModelViewMatrices();

		// Draw animated notes
		gl::color( Color(1.0f, 0.0f, 1.0f) );
		gl::pushModelViewMatrices();
		{
			mNote->drawInstanced(NUM_NOTES);
				
			// reflection
			gl::scale( 1.0f, -1.0f, 1.0f );
			mNote->drawInstanced(NUM_NOTES);
		}
		gl::popModelViewMatrices();
	}

	// Draw grid
	gl::color( Color(0.8f, 0.8f, 0.8f) );
	mGrid->draw();

	// Draw floor
	gl::pushModelViewMatrices();
	gl::enableAlphaBlending();
	gl::color( ColorA(1,1,1,0.75f) );
	gl::translate(0.0f, -0.5f, 0.0f);
	gl::scale(100.0f, 0.5f, 100.0f);
	mFloor->draw();
	gl::disableAlphaBlending();
	gl::popModelViewMatrices();

	// Restore 2D rendering
	gl::popMatrices();
	gl::disableDepthWrite();
	gl::disableDepthRead();

	// Render UI
	if( mDrawUI ) renderUI();
}

void StereoscopicRenderingApp::renderUI()
{
	float w = (float) getWindowWidth() * 0.5f;
	float h = (float) getWindowHeight();

	std::string renderMode, focusMode;
	switch(mRenderMethod) {
		case MONO: renderMode = "Mono"; break;
		case SIDE_BY_SIDE: renderMode = "Side By Side"; break;
		case OVER_UNDER: renderMode = "Over Under"; break;
		case ANAGLYPH_RED_CYAN: renderMode = "Anaglyph Red Cyan"; break;
		case INTERLACED_HORIZONTAL: renderMode = "Interlaced Horizontal"; break;
	}
	switch(mFocusMethod) {
		case SET_CONVERGENCE: focusMode = "setConvergence(d, false)"; break;
		case SET_FOCUS: focusMode = "setConvergence(d, true)"; break;
		case AUTO_FOCUS: focusMode = "autoFocus(cam)"; break;
	}

	std::string labels( "Render mode (F1-F5):\nFocus mode (1-3):\nFocal Length:\nEye Distance:\nAuto Focus Depth (Up/Down):\nAuto Focus Speed (Left/Right):" );
	std::string values( (boost::format( "%s\n%s\n%.2f\n%.2f\n%.2f\n%.2f" ) % renderMode % focusMode % mCamera.getConvergence() % mCamera.getEyeSeparation() % mAF.getDepth() % mAF.getSpeed()).str() );

	TextBox columnLeft = TextBox().alignment( TextBox::Alignment::LEFT ).font( mFont ).color(Color::black()).text(labels);
	TextBox columnRight = TextBox().alignment( TextBox::Alignment::RIGHT ).font( mFont ).color(Color::black()).text(values);

	Surface left = columnLeft.render();
	Surface right = columnRight.render();

	gl::enableAlphaBlending();
	int margin = 10;
	float width = columnLeft.measure().x + columnRight.measure().x;
	float height = math<float>::max(columnLeft.measure().y, columnRight.measure().y);
	float x = getWindowWidth() * 0.5f - width * 0.5f - margin;
	float y = getWindowHeight() - height - margin;
	gl::draw( gl::Texture::create( left ), Vec2f( x, y ) );
	x += columnLeft.measure().x + 2 * margin;
	gl::draw( gl::Texture::create( right ), Vec2f( x, y ) );
	gl::disableAlphaBlending();
}

CINDER_APP_BASIC( StereoscopicRenderingApp, RendererGl )