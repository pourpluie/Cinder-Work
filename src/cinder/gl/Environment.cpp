/*
 Copyright (c) 2013, The Cinder Project
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org

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

#include "cinder/gl/Environment.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vao.h"

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
bool Environment::sCoreProfile = true;
#endif


//////////////////////////////////////////////////////////////////////////////////
// GL ES 2

#if defined( CINDER_GLES )

class EnvironmentEs2 : public Environment {
  public:
	virtual void	initializeContextDefaults( Context *context ) override;
	
	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};

void EnvironmentEs2::initializeContextDefaults( Context *context )
{
}

std::string	EnvironmentEs2::generateVertexShader( const ShaderDef &shader )
{
	std::string s;
	
	s +=		"uniform mat4	uModelViewProjection;\n"
				"\n"
				"attribute vec4		vPosition;\n"
				;
			
	if( shader.mTextureMapping ) {
		s +=	"attribute vec2		vTexCoord0;\n"
				"varying highp vec2	TexCoord;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"attribute vec4		vColor;\n"
				"varying vec4		Color;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				"	gl_Position	= uModelViewProjection * vPosition;\n"
				;
				
	if( shader.mTextureMapping ) {	
		s +=	"	TexCoord = vTexCoord0;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"	Color = vColor;\n"
				;
	}
	
	s +=		"}\n";
	
	return s;
}

std::string	EnvironmentEs2::generateFragmentShader( const ShaderDef &shader )
{
	std::string s;

	s +=		"precision highp float;\n"
				;

	if( shader.mTextureMapping ) {	
		s +=	"uniform sampler2D	uTex0;\n"
				"varying highp vec2	TexCoord;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"varying lowp vec4	Color;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				;
	
	if( shader.mTextureMapping && shader.mColor ) {
		s +=	"	gl_FragColor = texture2D( uTex0, TexCoord.st ) * Color;\n"
				;
	}
	else if( shader.mTextureMapping ) {
		s +=	"	gl_FragColor = texture2D( uTex0, TexCoord.st );\n"
				;
	}
	else if( shader.mColor ) {
		s +=	"	gl_FragColor = Color;\n"
				;
	}
	
	s +=		"}\n"
				;
	
	return s;
}


GlslProgRef	EnvironmentEs2::buildShader( const ShaderDef &shader )
{
	std::cout << "Vertex: " << generateVertexShader( shader ) << std::endl;
	std::cout << "Fragment: " << generateFragmentShader( shader ) << std::endl;	
	return GlslProg::create( generateVertexShader( shader ).c_str(), generateFragmentShader( shader ).c_str() );
}

#else
//////////////////////////////////////////////////////////////////////////////////
// GL Core Profile

class EnvironmentCoreProfile : public Environment {
  public:
	virtual void	initializeContextDefaults( Context *context ) override;

	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};


void EnvironmentCoreProfile::initializeContextDefaults( Context *context )
{
}

std::string	EnvironmentCoreProfile::generateVertexShader( const ShaderDef &shader )
{
	std::string s;
	
	s +=		"#version 150\n"
				"\n"
				"uniform mat4	uModelViewProjection;\n"
				"\n"
				"in vec4		vPosition;\n"
				;
	
	if( shader.mTextureMapping ) {
		s +=	"in vec2		vTexCoord0;\n"
				"out highp vec2	TexCoord;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"in vec4		vColor;\n"
				"out lowp vec4	Color;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				"	gl_Position	= uModelViewProjection * vPosition;\n"
				;
	if( shader.mColor ) {
		s +=	"	Color = vColor;\n"
				;
	}
	if( shader.mTextureMapping ) {	
		s +=	"	TexCoord	= vTexCoord0;\n"
				;
	}
	
	s +=		"}\n";
	
	return s;
}

std::string	EnvironmentCoreProfile::generateFragmentShader( const ShaderDef &shader )
{
	std::string s;
	
	s+=			"#version 150\n"
				"\n"
				"out vec4 oColor;\n"
				;

	if( shader.mColor ) {
		s +=	"in vec4		Color;\n";
	}

	if( shader.mTextureMapping ) {
		if( shader.mTextureMappingRectangleArb )
			s +="uniform sampler2DRect uTex0;\n";
		else
			s +="uniform sampler2D uTex0;\n";
		s	+=	"in vec2	TexCoord;\n";
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				;
	
	if( shader.mTextureMapping && shader.mColor ) {
		s +=	"	oColor = texture( uTex0, TexCoord.st ) * Color;\n"
				;
	}
	else if( shader.mTextureMapping ) {
		s +=	"	oColor = texture( uTex0, TexCoord.st );\n"
				;
	}
	else if( shader.mColor ) {
		s +=	"	oColor = Color;\n"
				;
	}
	
	s +=		"}\n"
				;
	
	return s;
}


GlslProgRef	EnvironmentCoreProfile::buildShader( const ShaderDef &shader )
{
	return GlslProg::create( generateVertexShader( shader ).c_str(), generateFragmentShader( shader ).c_str() );
}

//////////////////////////////////////////////////////////////////////////////////
// GL Compatibility Profile

class EnvironmentCompatibilityProfile : public Environment {
  public:
	virtual void	initializeContextDefaults( Context *context ) override;

	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};


void EnvironmentCompatibilityProfile::initializeContextDefaults( Context *context )
{
}

std::string	EnvironmentCompatibilityProfile::generateVertexShader( const ShaderDef &shader )
{
	std::string s;
	
	s +=		"#version 120\n"
				"\n"
				"uniform mat4	uModelViewProjection;\n"
				"\n"
				"attribute vec4	vPosition;\n"
				;
			
	if( shader.mTextureMapping ) {
		s +=	"attribute vec2	vTexCoord0;\n"
				"varying vec2	TexCoord;\n"
				;
	}

	if( shader.mColor ) {
		s +=	"attribute vec4 vColor;\n"
				"varying vec4 Color;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				"	gl_Position	= uModelViewProjection * vPosition;\n"
				;
				
	if( shader.mTextureMapping ) {	
		s +=	"	TexCoord = vTexCoord0;\n"
				;
	}
	
	if( shader.mColor ) {
		s +=	"	Color = vColor;\n"
				;
	}
	
	s +=		"}\n";
	
	return s;
}

std::string	EnvironmentCompatibilityProfile::generateFragmentShader( const ShaderDef &shader )
{
	std::string s;
	
	s+=			"#version 120\n"
				"\n"
				;

	if( shader.mColor ) {
		s +=	"varying vec4		Color;\n";
	}

	if( shader.mTextureMapping ) {	
		if( shader.mTextureMappingRectangleArb )
			s +="uniform sampler2DRect uTex0;\n";
		else
			s +="uniform sampler2D	uTex0;\n";
		s	+=	"varying vec2		TexCoord;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				;
	
	if( shader.mColor && shader.mTextureMapping ) {
		if( shader.mTextureMappingRectangleArb )
			s +="	gl_FragColor = texture2DRect( uTex0, TexCoord.st ) * Color;\n";
		else
			s +="	gl_FragColor = texture2D( uTex0, TexCoord.st ) * Color;\n";
	}
	else if( shader.mTextureMapping ) {
		if( shader.mTextureMappingRectangleArb )
			s +="	gl_FragColor = texture2DRect( uTex0, TexCoord.st );\n";
		else
			s +="	gl_FragColor = texture2D( uTex0, TexCoord.st );\n";
				;
	}
	else if( shader.mColor ) {
		s +=	"	gl_FragColor = Color;\n"
				;
	}
	
	s +=		"}\n"
				;
	
	return s;
}


GlslProgRef	EnvironmentCompatibilityProfile::buildShader( const ShaderDef &shader )
{
	return GlslProg::create( generateVertexShader( shader ).c_str(), generateFragmentShader( shader ).c_str() );
}

#endif // if defined( CINDER_GLES )
///////////////////////////////////////////////////////////////////////////////////

Environment* env()
{
	static Environment *sEnvironment = NULL;
	if( ! sEnvironment ) {
#if defined( CINDER_GLES )
		sEnvironment = new EnvironmentEs2();
#else
		if( Environment::isCoreProfile() )
			sEnvironment = new EnvironmentCoreProfile();
		else
			sEnvironment = new EnvironmentCompatibilityProfile();
#endif
	}
	
	return sEnvironment;
}

} } // namespace cinder::gl