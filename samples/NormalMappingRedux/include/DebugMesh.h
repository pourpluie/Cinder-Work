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

