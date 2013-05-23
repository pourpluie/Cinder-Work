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
	mVaoStack.push_back( id );
	if( mActiveVao != id ) {
		mActiveVao = id;
#if defined( CINDER_GLES )
		glBindVertexArrayOES( mActiveVao );
#else
		glBindVertexArray( mActiveVao );
#endif
	}
	return VaoScope( this );
}

VaoScope Context::vaoPush( const Vao *vao )
{
	return vaoPush( vao->getId() );
}

VaoScope Context::vaoPush( const VaoRef &vao )
{
	return vaoPush( vao->getId() );
}

void Context::vaoPop()
{
	mVaoStack.pop_back();
}

void Context::vaoPrepareUse()
{
	if( mVaoStack.back() != mActiveVao ) {
		mActiveVao = mVaoStack.back();
#if defined( CINDER_GLES )
		glBindVertexArrayOES( mActiveVao );
#else
		glBindVertexArray( mActiveVao );
#endif
	}
}

//////////////////////////////////////////////////////////////////
// Buffer
BufferScope Context::bufferPush( GLenum target, GLuint id )
{
	mBufferStack[target].push_back( id );
	if( mActiveBuffer[target] != id ) {
		mActiveBuffer[target] = id;
		glBindBuffer( target, id );
	}
	return BufferScope( this, id );
}

BufferScope Context::bufferPush( const BufferObj *buffer )
{
	return bufferPush( buffer->getTarget(), buffer->getId() );
}

BufferScope Context::bufferPush( const BufferObjRef &buffer )
{
	return bufferPush( buffer->getTarget(), buffer->getId() );
}

void Context::bufferPop( GLenum target )
{
	mBufferStack[target].pop_back();
}

void Context::bufferPrepareUse( GLenum target )
{
	if( mActiveBuffer.find( target ) == mActiveBuffer.end() )
		mActiveBuffer[target] = 0;
	if( mBufferStack.find( target ) == mBufferStack.end() ) {
		mBufferStack[target] = vector<GLuint>();
		mBufferStack[target].push_back( 0 );
	}

	if( mBufferStack[target].back() != mActiveBuffer[target] ) {
		mActiveBuffer[target] = mBufferStack[target].back();
#if defined( CINDER_GLES )
		glBindVertexArrayOES( mActiveVao );
#else
		glBindVertexArray( mActiveVao );
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////////////
void Context::prepareDraw()
{
	vaoPrepareUse();
	bufferPrepareUse( GL_ARRAY_BUFFER );
	bufferPrepareUse( GL_ELEMENT_ARRAY_BUFFER );
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
		if ( ! mVbo ) {
			mVbo = Vbo::create( GL_ARRAY_BUFFER );
			mVbo->setUsage( GL_DYNAMIC_DRAW );
		}
		mVbo->bufferData( &mVertices[ 0 ], ( GLuint )( mVertices.size() * stride ), GL_DYNAMIC_DRAW );
		mVbo->bind();
	
		// Create VAO. All shader variations have the same attribute
		// layout, so we only need to this once
		if ( !mVao ) {
			mVao = Vao::create();
			const GlslProgRef& glslProg = shader->getGlslProg();
			
			GLint offset	= 0;
			GLint size		= 4;
			Vao::Attribute attrColor( glslProg->getAttribLocation( "aColor" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mVao->addAttribute( attrColor );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrNormal( glslProg->getAttribLocation( "aNormal" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mVao->addAttribute( attrNormal );
			offset			+= size * sizeof( float );
			
			size			= 3;
			Vao::Attribute attrPosition( glslProg->getAttribLocation( "aPosition" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mVao->addAttribute( attrPosition );
			offset			+= size * sizeof( float );
			
			size			= 4;
			Vao::Attribute attrTexCoord( glslProg->getAttribLocation( "aTexCoord" ), size, GL_FLOAT, false, stride, ( const GLvoid* )offset );
			mVao->addAttribute( attrTexCoord );
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
		auto vaoBind( context()->vaoPush( mVao ) );
		auto vboBind( context()->bufferPush( mVbo ) );
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