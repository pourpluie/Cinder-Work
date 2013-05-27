#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/fog.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Utilities.h"

#include "cinder/app/App.h"

namespace cinder { namespace gl {

using namespace std;
	
Context::Context()
	: mActiveVao( 0 ), mColor( ColorAf::white() ), mFogEnabled( false ), mLighting( false ), mMaterialEnabled( false ),
	mMode( GL_TRIANGLES ), mNormal( Vec3f( 0.0f, 0.0f, 1.0f ) ), mTexCoord( Vec4f::zero() ),
	mTextureUnit( -1 ), mWireframe( false )
{
	env()->initializeContextDefaults( this );

	clear();
	mModelView.push_back( Matrix44f() );
	mModelView.back().setToIdentity();
	mProjection.push_back( Matrix44f() );
	mProjection.back().setToIdentity();
}

Context::~Context()
{
	clear();
}

//////////////////////////////////////////////////////////////////
// VAO
VaoScope Context::vaoPush( GLuint id )
{
	VaoScope result( this, mActiveVao );
	mActiveVao = id;
	vaoPrepareUse();
	return result;
}

VaoScope Context::vaoPush( const Vao *vao )
{
	return vaoPush( vao->getId() );
}

VaoScope Context::vaoPush( const VaoRef &vao )
{
	return vaoPush( vao->getId() );
}

void Context::vaoBind( GLuint id )
{
	mActiveVao = id;
	vaoPrepareUse();
}

void Context::vaoRestore( GLuint id )
{
	mActiveVao = id;
}

void Context::vaoPrepareUse()
{
	if( mTrueVao != mActiveVao ) {
		mTrueVao = mActiveVao;
#if defined( CINDER_GLES )
		glBindVertexArrayOES( mTrueVao );
#else
		glBindVertexArray( mTrueVao );
#endif
	}
}

//////////////////////////////////////////////////////////////////
// Buffer
void Context::bufferBind( GLenum target, GLuint id )
{
	mActiveBuffer[target] = id;

	if( mTrueBuffer.find( target ) == mTrueBuffer.end() )
		mTrueBuffer[target] = 0;
	
	if( mTrueBuffer[target] != mActiveBuffer[target] ) {
		mTrueBuffer[target] = mActiveBuffer[target];
		glBindBuffer( target, mTrueBuffer[target] );
	}
}

GLuint Context::bufferGet( GLenum target )
{
	auto active = mActiveBuffer.find( target );
	if( mActiveBuffer.find( target ) == mActiveBuffer.end() ) {
		mActiveBuffer[target] = 0;
		return 0;
	}
	else
		return active->second;
}

void Context::bufferRestore( GLenum target, GLuint id )
{
	mActiveBuffer[target] = id;
}

void Context::bufferPrepareUse( GLenum target )
{
	if( mActiveBuffer.find( target ) == mActiveBuffer.end() )
		mActiveBuffer[target] = 0;
	if( mTrueBuffer.find( target ) == mTrueBuffer.end() )
		mTrueBuffer[target] = 0;

	if( mTrueBuffer[target] != mActiveBuffer[target] ) {
		mTrueBuffer[target] = mActiveBuffer[target];
		glBindBuffer( target, mTrueBuffer[target] );
	}
}

//////////////////////////////////////////////////////////////////
// States
StateScope Context::enablePush( GLenum cap, bool enable )
{
	if( mActiveState.find( cap ) == mActiveState.end() )
		mTrueState[cap] = mActiveState[cap] = glIsEnabled( cap );

	StateScope result( this, cap, mActiveState[cap] );
	mActiveState[cap] = enable;
	statePrepareUse( cap );
	return result;
}

StateScope Context::disablePush( GLenum cap )
{
	return enablePush( cap, false );
}

void Context::enable( GLenum cap, bool enable )
{
	mActiveState[cap] = enable;
	
	if( mTrueState[cap] != mActiveState[cap] ) {
		mTrueState[cap] = mActiveState[cap];
		if( mTrueState[cap] )
			glEnable( cap );
		else
			glDisable( cap );
	}	
}

void Context::stateRestore( GLenum cap, bool enable )
{
	mActiveState[cap] = enable;
}

void Context::statePrepareUse( GLenum cap )
{
	if( (mActiveState.find( cap ) == mActiveState.end()) || (mTrueState.find( cap ) == mTrueState.end()) )
		mTrueState[cap] = mActiveState[cap] = glIsEnabled( cap );
		
	if( mTrueState[cap] != mActiveState[cap] ) {
		mTrueState[cap] = mActiveState[cap];
		if( mTrueState[cap] )
			glEnable( cap );
		else
			glDisable( cap );
	}
}

void Context::statesPrepareUse()
{
	for( auto stateIt = mActiveState.cbegin(); stateIt != mActiveState.cend(); ++stateIt )
		statePrepareUse( stateIt->first );
}

/////////////////////////////////////////////////////////////////////////////////////
void Context::prepareDraw()
{
	vaoPrepareUse();
	bufferPrepareUse( GL_ARRAY_BUFFER );
	bufferPrepareUse( GL_ELEMENT_ARRAY_BUFFER );
	statesPrepareUse();
}

void Context::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	vaoPrepareUse();
	bufferPrepareUse( GL_ARRAY_BUFFER );
	glVertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void Context::enableVertexAttribArray( GLuint index )
{
	vaoPrepareUse();
	glEnableVertexAttribArray( index );
}

void Context::clear()
{
	mVertices.clear();
}

void Context::draw()
{
	if( ! mVertices.empty() ) {
		// Choose shader
		Shader::UniformOptions options;
		options.enableColor();
		options.setLightingModel( mLighting ? Shader::LightingModel::LAMBERT : Shader::LightingModel::NONE );
		options.enableFog( mFogEnabled );
		options.enableMaterial( mMaterialEnabled );
		options.enableTexture( mTextureUnit >= 0 );
		options.setNumLights( mLights.size() );
		options.setPrecision( Shader::Precision::HIGH );
		
		bool found = false;
		for( ShaderMap::const_iterator iter = mShaders.begin(); iter != mShaders.end(); ++iter ) {
			if ( iter->first == options ) {
				found = true;
				break;
			}
		}
		if ( !found ) { // if ( mShader.find( options ) == mShaders.end() ) { // Isn't working here
			mShaders.insert( make_pair( options, Shader::create( options ) ) );
		}
		ShaderRef shader = mShaders[ options ];
		
		// Change mode to line strip for wireframes
		GLenum mode = mMode;
		if ( mWireframe && mode != GL_POINTS ) {
			mode = GL_LINE_STRIP;
		}
		
		// Buffer data
		GLsizei stride = (GLsizei)sizeof( Context::Vertex );
		if ( ! mImmVbo ) {
			mImmVbo = Vbo::create( GL_ARRAY_BUFFER );
			mImmVbo->setUsage( GL_DYNAMIC_DRAW );
		}
		mImmVbo->bufferData( &mVertices[ 0 ], ( GLuint )( mVertices.size() * stride ), GL_DYNAMIC_DRAW );
	
		// Create VAO. All shader variations have the same attribute
		// layout, so we only need to this once
		if( ! mImmVao ) {
			mImmVao = Vao::create();
			const GlslProgRef& glslProg = shader->getGlslProg();
			
			GLint offset	= 0;
			GLint size		= 4;
			Vao::Attribute attrColor( glslProg->getAttribLocation( "aColor" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrColor );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrNormal( glslProg->getAttribLocation( "aNormal" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrNormal );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrPosition( glslProg->getAttribLocation( "aPosition" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrPosition );
			offset			+= size * sizeof( float );
			
			size			= 4;
			Vao::Attribute attrTexCoord( glslProg->getAttribLocation( "aTexCoord" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mImmVao->addAttribute( attrTexCoord );
			offset			+= size * sizeof( float );
		}
		
		// Set uniforms
		shader->setModelViewProjectionMatrix( mProjection.back() * mModelView.back() );
		if ( mFogEnabled ) {
			shader->setFog( mFog );
		}
		if ( mMaterialEnabled ) {
			shader->setMaterial( mMaterial );
		}
		if ( mTextureUnit >= 0 ) {
			shader->setTextureId( mTextureUnit );
		}
		if ( mLights.size() > 0 ) {
			size_t i = 0;
			for ( vector<Light>::const_iterator iter = mLights.begin(); iter != mLights.end(); ++iter, ++i ) {
				shader->setLight( i, *iter );
			}
		}
		shader->update();
		
		// Draw
		shader->bind();
		auto vaoBind( context()->vaoPush( mImmVao ) );
		BufferScope bufferBind( mImmVbo->getTarget(), mImmVbo->getId() );
		drawArrays( mode, 0, mVertices.size() );
		shader->unbind();
	}
	clear();
}
	
void Context::pushBack( const ci::Vec4f &v )
{
	Vertex vertex;		
	vertex.mColor		= mColor;
	vertex.mNormal		= mNormal;
	vertex.mPosition	= v.xyz();
	vertex.mTexCoord	= mTexCoord;
	
	mVertices.push_back( vertex );
}

} } // namespace cinder::gl