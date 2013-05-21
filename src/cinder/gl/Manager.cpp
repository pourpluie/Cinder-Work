#include "cinder/gl/gl.h"
#include "cinder/gl/Manager.h"
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
	
ManagerRef Manager::get()
{
	static ManagerRef manager;
	if ( !manager ) {
		manager = ManagerRef( new Manager() );
	}
	return manager;
}

Manager::Manager()
: mColor( ColorAf::white() ), mFogEnabled( false ), mLighting( false ), mMaterialEnabled( false ),
mMode( GL_TRIANGLES ), mNormal( Vec3f( 0.0f, 0.0f, 1.0f ) ), mTexCoord( Vec4f::zero() ),
mTextureUnit( -1 ), mWireframe( false )
{
	clear();
	mModelView.push_back( Matrix44f() );
	mModelView.back().setToIdentity();
	mProjection.push_back( Matrix44f() );
	mProjection.back().setToIdentity();
}

Manager::~Manager()
{
	clear();
}

void Manager::clear()
{
	mVertices.clear();
}

void Manager::draw()
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
		GLsizei stride = (GLsizei)sizeof( Manager::Vertex );
		if ( ! mVbo ) {
			mVbo = Vbo::create( GL_ARRAY_BUFFER );
			mVbo->setUsage( GL_DYNAMIC_DRAW );
		}
		mVbo->bind();		
		mVbo->bufferData( &mVertices[ 0 ], ( GLuint )( mVertices.size() * stride ), GL_DYNAMIC_DRAW );
	
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
		mVao->bind();
		mVbo->bind();
		glDrawArrays( mode, 0, mVertices.size() );
		mVbo->unbind();
		mVao->unbind();
		shader->unbind();
	}
	clear();
}
	
void Manager::pushBack( const ci::Vec4f &v )
{
	Vertex vertex;		
	vertex.mColor		= mColor;
	vertex.mNormal		= mNormal;
	vertex.mPosition	= v.xyz();
	vertex.mTexCoord	= mTexCoord;
	
	mVertices.push_back( vertex );
}

} } // namespace cinder::gl