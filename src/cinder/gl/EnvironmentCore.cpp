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
#include "cinder/gl/gl.h"
#include "glload/gl_load.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vao.h"

#if ! defined( CINDER_GLES )

namespace cinder { namespace gl {

class EnvironmentCore : public Environment {
  public:
	virtual void	initializeFunctionPointers() override;

	virtual bool	isCoreProfile() const override { return true; }
	virtual bool	supportsHardwareVao() override;

	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};

Environment* allocateEnvironmentCore()
{
	return new EnvironmentCore;
}

void EnvironmentCore::initializeFunctionPointers()
{
	static bool sInitialized = false;
	if( ! sInitialized ) {
		ogl_LoadFunctions();
		sInitialized = true;
	}
}

bool EnvironmentCore::supportsHardwareVao()
{
	return true;
}

std::string	EnvironmentCore::generateVertexShader( const ShaderDef &shader )
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

std::string	EnvironmentCore::generateFragmentShader( const ShaderDef &shader )
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


GlslProgRef	EnvironmentCore::buildShader( const ShaderDef &shader )
{
std::cout << "Core shader vert:" << std::endl << generateVertexShader( shader ).c_str() << std::endl;
std::cout << "Core shader frag:" << std::endl << generateFragmentShader( shader ).c_str() << std::endl;
	return GlslProg::create( GlslProg::Format().vertex( generateVertexShader( shader ).c_str() )
												.fragment( generateFragmentShader( shader ).c_str() )
												.attribLocation( "vPosition", 0 )
												);
}

} } // namespace cinder::gl

#endif // ! defined( CINDER_GLES )