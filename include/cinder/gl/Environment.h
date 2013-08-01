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

#pragma once

#include "cinder/gl/gl.h"

namespace cinder { namespace gl {

class ShaderDef;
class GlslProg;
typedef std::shared_ptr<GlslProg>		GlslProgRef;

class Environment {
  public:
	virtual void			initializeFunctionPointers() = 0;
	virtual void			initializeContextDefaults( Context *context ) = 0;
	
	virtual std::string		generateVertexShader( const ShaderDef &shader ) = 0;
	virtual std::string		generateFragmentShader( const ShaderDef &shader ) = 0;
	virtual GlslProgRef		buildShader( const ShaderDef &shader ) = 0;
	
  #if ! defined( CINDER_GLES )
  	static bool				isCoreProfile() { return sCoreProfile; }
	//! Must be called before a call to gl::env()
	static void				setCoreProfile( bool enable = true ) { sCoreProfile = enable; }
  
  protected:
	static bool				sCoreProfile;
  #endif
};

} } // namespace cinder::gl