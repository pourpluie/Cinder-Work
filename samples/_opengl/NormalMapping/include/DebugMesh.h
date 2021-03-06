/*
 Copyright (c) 2014, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

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

#include "cinder/GeomIo.h"
#include "cinder/TriMesh.h"

class DebugMesh : public ci::geom::Source
{
public:
	DebugMesh(void);
	DebugMesh(const ci::TriMesh& mesh);
	~DebugMesh(void);

	void						clear();
	void						setMesh(const ci::TriMesh& mesh);

	virtual size_t				getNumVertices() const override { return mVertices.size(); }
	virtual size_t				getNumIndices() const override { return mIndices.size(); }
	virtual ci::geom::Primitive	getPrimitive() const override { return ci::geom::Primitive::LINES; }
	virtual uint8_t				getAttribDims( ci::geom::Attrib attr ) const override;
	
	virtual void				loadInto( ci::geom::Target *target ) const override;

private:
	std::vector<ci::Vec3f>		mVertices;
	std::vector<ci::Color>		mColors;
	std::vector<ci::uint32_t>	mIndices;
};

