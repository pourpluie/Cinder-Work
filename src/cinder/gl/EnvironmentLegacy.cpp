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

#if ( ! defined( CINDER_GL_ES ) ) && ( ! defined( CINDER_GL_ANGLE ) )

#define CINDER_GL_LEGACY // to force appropriate GL headers
#include "glload/gl_all.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/gl.h"
#include "glload/gl_load.h"

#include "cinder/gl/Shader.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Vao.h"

namespace cinder { namespace gl {

class EnvironmentLegacy : public Environment {
  public:
	virtual void	initializeFunctionPointers() override;
	virtual bool	isExtensionAvailable( const std::string &extName ) override;
	virtual bool	supportsHardwareVao() override;
	virtual bool	isCoreProfile() const override { return false; }

	virtual std::string		generateVertexShader( const ShaderDef &shader ) override;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) override;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) override;
};

Environment* allocateEnvironmentLegacy()
{
	return new EnvironmentLegacy;
}

void EnvironmentLegacy::initializeFunctionPointers()
{
	static bool sInitialized = false;
	if( ! sInitialized ) {
		ogl_LoadFunctions();
		sInitialized = true;
	}
}

bool EnvironmentLegacy::isExtensionAvailable( const std::string &extName )
{
	static const char *sExtStr = reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) );
	static std::map<std::string, bool> sExtMap;
	
	std::map<std::string,bool>::const_iterator extIt = sExtMap.find( extName );
	if ( extIt == sExtMap.end() ) {
		bool found		= false;
		int extNameLen	= extName.size();
		const char *p	= sExtStr;
		const char *end = sExtStr + strlen( sExtStr );
		while ( p < end ) {
			int n = strcspn( p, " " );
			if ( (extNameLen == n ) && ( strncmp( extName.c_str(), p, n) == 0 ) ) {
				found = true;
				break;
			}
			p += (n + 1);
		}
		sExtMap[extName] = found;
		return found;
	}
	else {
		return extIt->second;
	}
}

bool EnvironmentLegacy::supportsHardwareVao()
{
	// TODO: extension string test
	return true;
}

std::string	EnvironmentLegacy::generateVertexShader( const ShaderDef &shader )
{
	std::string s;
	
	s +=		"#version 120\n"
				"\n"
				"uniform mat4	ciModelViewProjection;\n"
				"\n"
				"attribute vec4	ciPosition;\n"
				;
			
	if( shader.mTextureMapping ) {
		s +=	"attribute vec2	ciTexCoord0;\n"
				"varying vec2	TexCoord;\n"
				;
	}

	if( shader.mColor ) {
		s +=	"attribute vec4 ciColor;\n"
				"varying vec4 Color;\n"
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

std::string	EnvironmentLegacy::generateFragmentShader( const ShaderDef &shader )
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
	
	if( shader.mTextureMapping ) {
		std::string textureSampleStr = ( shader.mTextureMappingRectangleArb ) ? "texture2DRect( uTex0, TexCoord.st )" : "texture2D( uTex0, TexCoord.st )";
		if( ! Texture::supportsHardwareSwizzle() && ! shader.isTextureSwizzleDefault() )
			textureSampleStr += std::string(".") + shader.getTextureSwizzleString();
		if( shader.mColor ) {
			s +=	"	gl_FragColor = " + textureSampleStr + " * Color;\n"
					;
		}
		else {
			s +=	"	gl_FragColor = " + textureSampleStr + ";\n"
					;
		}
	}	
	else if( shader.mColor ) {
		s +=	"	gl_FragColor = Color;\n"
				;
	}
	
	s +=		"}\n"
				;
	
	return s;
}


GlslProgRef	EnvironmentLegacy::buildShader( const ShaderDef &shader )
{
//std::cout << "Compat shader vert:" << std::endl << generateVertexShader( shader ).c_str() << std::endl;
//std::cout << "Compat shader frag:" << std::endl << generateFragmentShader( shader ).c_str() << std::endl;
	return GlslProg::create( GlslProg::Format().vertex( generateVertexShader( shader ).c_str() )
												.fragment( generateFragmentShader( shader ).c_str() )
												.attribLocation( "ciPosition", 0 )
												);
}


} } // namespace cinder::gl

#endif // ( ! defined( CINDER_GL_ES ) ) && ( ! defined( CINDER_GL_ANGLE ) )