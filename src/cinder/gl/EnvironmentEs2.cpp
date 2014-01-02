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

#if defined( CINDER_GLES )

#include "cinder/gl/Shader.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vao.h"

namespace cinder { namespace gl {

class EnvironmentEs2 : public Environment {
  public:
	virtual void	initializeFunctionPointers() override;

	virtual bool	supportsHardwareVao() override;
	
	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};

Environment* allocateEnvironmentEs2()
{
	return new EnvironmentEs2;
}

void EnvironmentEs2::initializeFunctionPointers()
{
}

bool EnvironmentEs2::supportsHardwareVao()
{
#if defined( CINDER_COCOA_TOUCH )
	return true;
#else
	return false;
#endif
}

std::string	EnvironmentEs2::generateVertexShader( const ShaderDef &shader )
{
	std::string s;
	
	s +=		"uniform mat4	ciModelViewProjection;\n"
				"\n"
				"attribute vec4		ciPosition;\n"
				;
			
	if( shader.mTextureMapping ) {
		s +=	"attribute vec2		ciTexCoord0;\n"
				"varying highp vec2	TexCoord;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"attribute vec4		ciColor;\n"
				"varying vec4		Color;\n"
				;
	}

	s +=		"void main( void )\n"
				"{\n"
				"	gl_Position	= ciModelViewProjection * ciPosition;\n"
				;
				
	if( shader.mTextureMapping ) {	
		s +=	"	TexCoord = ciTexCoord0;\n"
				;
	}
	if( shader.mColor ) {
		s +=	"	Color = ciColor;\n"
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
//std::cout << "ES 2 Shader Vert: " << generateVertexShader( shader ) << std::endl;
//std::cout << "ES 2 Shader Frag: " << generateFragmentShader( shader ) << std::endl;	
	return GlslProg::create( generateVertexShader( shader ).c_str(), generateFragmentShader( shader ).c_str() );
}

} } // namespace cinder::gl

#endif // defined( CINDER_GLES )