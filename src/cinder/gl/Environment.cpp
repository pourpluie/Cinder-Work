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

namespace cinder { namespace gl {

#if defined( CINDER_GLES )

class EnvironmentEs2 : public Environment {
  public:
	virtual void	initializeContextDefaults( Context *context ) override;
};


void EnvironmentEs2::initializeContextDefaults( Context *context )
{
}

#else
//////////////////////////////////////////////////////////////////////////////////
// GL Core Profile

class EnvironmentCoreProfile : public Environment {
  public:
	virtual void	initializeContextDefaults( Context *context ) override;
};


void EnvironmentCoreProfile::initializeContextDefaults( Context *context )
{
	
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
		sEnvironment = new EnvironmentCoreProfile();
#endif
	}
	
	return sEnvironment;
}

} } // namespace cinder::gl