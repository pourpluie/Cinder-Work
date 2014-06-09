/*
 Copyright (c) 2014, The Cinder Project: http://libcinder.org
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


#include "cinder/gl/ConstantStrings.h"
#include "cinder/gl/gl.h"

#include <unordered_map>

namespace cinder { namespace gl {

//! Returns a string representation for a subset of the GL constants. Returns empty string if unknown.
std::string	constantToString( GLenum constant )
{
	static bool initialized = false;
	static std::unordered_map<GLenum,std::string> sSymbols;
	if( ! initialized ) {
		// Types
		sSymbols[GL_BYTE] = "BYTE";
		sSymbols[GL_UNSIGNED_BYTE] = "UNSIGNED_BYTE";
		sSymbols[GL_SHORT] = "SHORT";
		sSymbols[GL_UNSIGNED_SHORT] = "UNSIGNED_SHORT";
		sSymbols[GL_INT] = "INT";
		sSymbols[GL_UNSIGNED_INT] = "UNSIGNED_INT";
		sSymbols[GL_FIXED] = "FIXED";
		sSymbols[GL_FLOAT] = "FLOAT";
		sSymbols[GL_FLOAT_VEC2] = "FLOAT_VEC2";
		sSymbols[GL_FLOAT_VEC3] = "FLOAT_VEC3";
		sSymbols[GL_FLOAT_VEC4] = "FLOAT_VEC4";
		sSymbols[GL_INT_VEC2] = "INT_VEC2";
		sSymbols[GL_INT_VEC3] = "INT_VEC3";
		sSymbols[GL_INT_VEC4] = "INT_VEC4";
		sSymbols[GL_BOOL] = "BOOL";
		sSymbols[GL_BOOL_VEC2] = "BOOL_VEC2";
		sSymbols[GL_BOOL_VEC3] = "BOOL_VEC3";
		sSymbols[GL_BOOL_VEC4] = "BOOL_VEC4";
		sSymbols[GL_FLOAT_MAT2] = "FLOAT_MAT2";
		sSymbols[GL_FLOAT_MAT3] = "FLOAT_MAT3";
		sSymbols[GL_FLOAT_MAT4] = "FLOAT_MAT4";
		sSymbols[GL_SAMPLER_2D] = "SAMPLER_2D";
		sSymbols[GL_SAMPLER_CUBE] = "SAMPLER_CUBE";
#if ! defined( CINDER_GL_ES )
		sSymbols[GL_SAMPLER_1D] = "SAMPLER_1D";
		sSymbols[GL_SAMPLER_3D] = "SAMPLER_3D";
		sSymbols[GL_SAMPLER_1D_SHADOW] = "SAMPLER_1D_SHADOW";
		sSymbols[GL_SAMPLER_2D_SHADOW] = "SAMPLER_2D_SHADOW";
		sSymbols[GL_HALF_FLOAT] = "HALF_FLOAT";
		sSymbols[GL_DOUBLE] = "DOUBLE";
		sSymbols[GL_INT_2_10_10_10_REV] = "INT_2_10_10_10_REV";
		sSymbols[GL_UNSIGNED_INT_2_10_10_10_REV] = "UNSIGNED_INT_2_10_10_10_REV";
#endif
		// Buffer bindings
		sSymbols[GL_ARRAY_BUFFER] = "GL_ARRAY_BUFFER";
		sSymbols[GL_ELEMENT_ARRAY_BUFFER] = "GL_ELEMENT_ARRAY_BUFFER";
#if ! defined( CINDER_GL_ES )
		sSymbols[GL_ATOMIC_COUNTER_BUFFER] = "GL_ATOMIC_COUNTER_BUFFER";
		sSymbols[GL_COPY_READ_BUFFER] = "GL_COPY_READ_BUFFER";
		sSymbols[GL_COPY_WRITE_BUFFER] = "GL_COPY_WRITE_BUFFER";
		sSymbols[GL_DRAW_INDIRECT_BUFFER] = "GL_DRAW_INDIRECT_BUFFER";
		sSymbols[GL_DISPATCH_INDIRECT_BUFFER] = "GL_DISPATCH_INDIRECT_BUFFER";
		sSymbols[GL_PIXEL_PACK_BUFFER] = "GL_PIXEL_PACK_BUFFER";
		sSymbols[GL_PIXEL_UNPACK_BUFFER] = "GL_PIXEL_UNPACK_BUFFER";
		sSymbols[GL_QUERY_BUFFER] = "GL_QUERY_BUFFER";
		sSymbols[GL_SHADER_STORAGE_BUFFER] = "GL_SHADER_STORAGE_BUFFER";
		sSymbols[GL_TEXTURE_BUFFER] = "GL_TEXTURE_BUFFER";
		sSymbols[GL_TRANSFORM_FEEDBACK_BUFFER] = "GL_TRANSFORM_FEEDBACK_BUFFER";
		sSymbols[GL_UNIFORM_BUFFER] = "GL_UNIFORM_BUFFER";
#endif
		// Buffer usage
		sSymbols[GL_STREAM_DRAW] = "GL_STREAM_DRAW";
		sSymbols[GL_STATIC_DRAW] = "GL_STATIC_DRAW";
		sSymbols[GL_DYNAMIC_DRAW] = "GL_DYNAMIC_DRAW";
#if ! defined( CINDER_GL_ES )
		sSymbols[GL_STREAM_READ] = "GL_STREAM_READ";
		sSymbols[GL_STREAM_COPY] = "GL_STREAM_COPY";
		sSymbols[GL_STATIC_READ] = "GL_STATIC_READ";
		sSymbols[GL_STATIC_COPY] = "GL_STATIC_COPY";
		sSymbols[GL_DYNAMIC_READ] = "GL_DYNAMIC_READ";
		sSymbols[GL_DYNAMIC_COPY] = "GL_DYNAMIC_COPY";
#endif
		// Texture targets
		sSymbols[GL_TEXTURE_2D] = "GL_TEXTURE_2D";
		sSymbols[GL_TEXTURE_CUBE_MAP] = "GL_TEXTURE_CUBE_MAP";
#if ! defined( CINDER_GL_ES )
		sSymbols[GL_TEXTURE_1D] = "GL_TEXTURE_1D";
		sSymbols[GL_TEXTURE_3D] = "GL_TEXTURE_3D";
#endif

		initialized = true;
	}
	
	auto it = sSymbols.find( constant );
	if( it != sSymbols.end() )
		return it->second;
	else
		return std::string();
}

} } // namespace cinder::gl