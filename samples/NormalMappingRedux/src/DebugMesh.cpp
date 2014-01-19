#include "DebugMesh.h"

using namespace ci;
using namespace ci::geom;
using namespace std;

DebugMesh::DebugMesh(void)
{
	enable( Attrib::POSITION );
	enable( Attrib::COLOR );

	clear();
}

DebugMesh::DebugMesh(const TriMesh& mesh)
{
	enable( Attrib::POSITION );
	enable( Attrib::COLOR );

	setMesh(mesh);
}

DebugMesh::~DebugMesh(void)
{
}

void DebugMesh::clear()
{
	mVertices.clear();
	mColors.clear();
	mIndices.clear();
}

void DebugMesh::setMesh(const TriMesh& mesh)
{
	clear();

	// create a debug mesh, showing normals, tangents and bitangents
	size_t numVertices = mesh.getNumVertices();

	mVertices.reserve( numVertices * 4 );
	mColors.reserve( numVertices * 4 );
	mIndices.reserve( numVertices * 6 );

	for(size_t i=0;i<numVertices;++i) {
		uint32_t idx = mVertices.size();

		mVertices.push_back( mesh.getVertices<3>()[i] );
		mVertices.push_back( mesh.getVertices<3>()[i] + mesh.getTangents()[i] );
		mVertices.push_back( mesh.getVertices<3>()[i] + mesh.getNormals()[i].cross(mesh.getTangents()[i]) );
		mVertices.push_back( mesh.getVertices<3>()[i] + mesh.getNormals()[i] );

		mColors.push_back( Color(0, 0, 0) );	// base vertices black
		mColors.push_back( Color(1, 0, 0) );	// tangents (along u-coordinate) red
		mColors.push_back( Color(0, 1, 0) ); // bitangents (along v-coordinate) green
		mColors.push_back( Color(0, 0, 1) ); // normals blue

		mIndices.push_back( idx );
		mIndices.push_back( idx + 1 );
		mIndices.push_back( idx );
		mIndices.push_back( idx + 2 );
		mIndices.push_back( idx );
		mIndices.push_back( idx + 3 );
	}
}

uint8_t DebugMesh::getAttribDims( Attrib attr ) const
{
	switch( attr ) {
		case Attrib::POSITION: return 3;
		case Attrib::COLOR: return 3;
		default:
			return 0;
	}
}

void DebugMesh::loadInto( Target *target ) const
{
	target->copyAttrib( Attrib::POSITION, 3, 0, reinterpret_cast<const float*>(&mVertices.front()), mVertices.size() );
	target->copyAttrib( Attrib::COLOR, 3, 0, reinterpret_cast<const float*>(&mColors.front()), mColors.size() );
	target->copyIndices( Primitive::LINES, &mIndices.front(), mIndices.size(), 4 );
}
