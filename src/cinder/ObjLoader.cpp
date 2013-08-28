/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Copyright (c) Microsoft Open Technologies, Inc. All rights reserved.

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

#include "cinder/ObjLoader.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
#include <sstream>
using std::ostringstream;

#include <sstream>
using namespace std;
using boost::make_tuple;

namespace cinder {

geom::SourceRef	loadGeom( const fs::path &path )
{
}

ObjLoader::ObjLoader( shared_ptr<IStreamCinder> stream, bool includeUVs )
	: mStream( stream )
{
	parse( includeUVs );
	load();
}

ObjLoader::ObjLoader( DataSourceRef dataSource, bool includeUVs )
	: mStream( dataSource->createStream() )
{
	parse( includeUVs );
	load();
}

ObjLoader::ObjLoader( DataSourceRef dataSource, DataSourceRef materialSource, bool includeUVs )
    : mStream( dataSource->createStream() )
{
    parseMaterial( materialSource->createStream() );
    parse( includeUVs );
	load();	
}
    
ObjLoader::~ObjLoader()
{
}

bool ObjLoader::hasAttrib( geom::Attrib attr ) const
{
	switch( attr ) {
		case geom::Attrib::POSITION: return ! mVertices.empty(); break;
		case geom::Attrib::COLOR: return ! mColors.empty(); break;
		case geom::Attrib::TEX_COORD_0: return ! mTexCoords.empty(); break;
		case geom::Attrib::NORMAL: return ! mNormals.empty();
		default:
			return false;
	}
}

bool ObjLoader::canProvideAttrib( geom::Attrib attr ) const
{
	switch( attr ) {
		case geom::Attrib::POSITION: return ! mVertices.empty(); break;
		case geom::Attrib::COLOR: return ! mColors.empty(); break;
		case geom::Attrib::TEX_COORD_0: return ! mTexCoords.empty(); break;
		case geom::Attrib::NORMAL: return (! mVertices.empty() ) || ( ! mNormals.empty() ); // we can derive normals if we have only positions
		default:
			return false;
	}
}

void ObjLoader::copyAttrib( geom::Attrib attr, uint8_t dimensions, size_t stride, float *dest ) const
{
	switch( attr ) {
		case geom::Attrib::POSITION:
			copyData( 3, (const float*)&mVertices[0], mVertices.size(), dimensions, stride, dest );
		break;
		case geom::Attrib::COLOR:
			copyData( 3, (const float*)&mColors[0], mColors.size(), dimensions, stride, dest );
		break;
		case geom::Attrib::TEX_COORD_0:
			copyData( 2, (const float*)&mTexCoords[0], mTexCoords.size(), dimensions, stride, dest );
		break;
		case geom::Attrib::NORMAL:
			copyData( 3, (const float*)&mNormals[0], mNormals.size(), dimensions, stride, dest );
		break;
		default:
			throw geom::ExcMissingAttrib();
	}
}

uint8_t	ObjLoader::getAttribDims( geom::Attrib attr ) const
{
	if( ! canProvideAttrib( attr ) )
		return 0;

	switch( attr ) {
		case geom::Attrib::POSITION: return 3;
		case geom::Attrib::COLOR: return 3;
		case geom::Attrib::TEX_COORD_0: return 2;
		case geom::Attrib::NORMAL: return 3;
		default:
			return 0;
	}
}

void ObjLoader::copyIndices( uint16_t *dest ) const
{
	size_t ct = mIndices.size();
	for( size_t i = 0; i < ct; ++i )
		dest[i] = mIndices[i];	
}

void ObjLoader::copyIndices( uint32_t *dest ) const
{
	memcpy( dest, &mIndices[0], sizeof(uint32_t) * mIndices.size() );
}
   
void ObjLoader::parseMaterial( std::shared_ptr<IStreamCinder> material )
{
    Material m;
    m.Ka[0] = m.Ka[1] = m.Ka[2] = 1.0f;
    m.Kd[0] = m.Kd[1] = m.Kd[2] = 1.0f;

    while( ! material->isEof() ) {
        string line = material->readLine();
        if( line.empty() || line[0] == '#' )
            continue;

        string tag;
        stringstream ss( line );
        ss >> tag;
        if( tag == "newmtl" ) {
            if( m.mName.length() > 0 )
                mMaterials[m.mName] = m;
            
            ss >> m.mName;
            m.Ka[0] = m.Ka[1] = m.Ka[2] = 1.0f;
            m.Kd[0] = m.Kd[1] = m.Kd[2] = 1.0f;
        }
        else if( tag == "Ka" ) {
            ss >> m.Ka[0] >> m.Ka[1] >> m.Ka[2];
        }
        else if( tag == "Kd" ) {
            ss >> m.Kd[0] >> m.Kd[1] >> m.Kd[2];
        }
    }
    if( m.mName.length() > 0 )
        mMaterials[m.mName] = m;
}

void ObjLoader::parse( bool includeUVs )
{
	Group *currentGroup;
	mGroups.push_back( Group() );
	currentGroup = &mGroups[mGroups.size()-1];
	currentGroup->mBaseVertexOffset = currentGroup->mBaseTexCoordOffset = currentGroup->mBaseNormalOffset = 0;

    const Material *currentMaterial = 0;
    
	size_t lineNumber = 0;
	while( ! mStream->isEof() ) {
		lineNumber++;
		string line = mStream->readLine(), tag;
        if( line.empty() || line[0] == '#' )
            continue;
        
		stringstream ss( line );
		ss >> tag;
		if( tag == "v" ) { // vertex
			Vec3f v;
			ss >> v.x >> v.y >> v.z;
			mVertices.push_back( v );
		}
		else if( tag == "vt" ) { // vertex texture coordinates
			if( includeUVs ) {
				Vec2f tex;
				ss >> tex.x >> tex.y;
				mTexCoords.push_back( tex );
			}
		}
		else if( tag == "vn" ) { // vertex normals
			Vec3f v;
			ss >> v.x >> v.y >> v.z;
			mNormals.push_back( v.normalized() );
		}
		else if( tag == "f" ) { // face
			parseFace( currentGroup, currentMaterial, line, includeUVs );
		}
		else if( tag == "g" ) { // group
			if( ! currentGroup->mFaces.empty() )
				mGroups.push_back( Group() );
			currentGroup = &mGroups[mGroups.size()-1];
			currentGroup->mBaseVertexOffset = mVertices.size();
			currentGroup->mBaseTexCoordOffset = mTexCoords.size();
			currentGroup->mBaseNormalOffset = mNormals.size();
			currentGroup->mName = line.substr( line.find( ' ' ) + 1 );
		}
        else if( tag == "usemtl") { // material
            string tag;
            ss >> tag;
            std::map<std::string, Material>::const_iterator m = mMaterials.find(tag);
            if( m != mMaterials.end() ) {
                currentMaterial = &m->second;
            }
        }
	}
}

void ObjLoader::parseFace( Group *group, const Material *material, const std::string &s, bool includeUVs )
{
	ObjLoader::Face result;
	result.mNumVertices = 0;
    result.mMaterial = material;

	size_t offset = 2; // account for "f "
	size_t length = s.length();
	while( offset < length ) {
		size_t endOfTriple, firstSlashOffset, secondSlashOffset;
	
		while( s[offset] == ' ' )
			++offset;
	
		// find the end of this triple "v/vt/vn"
		endOfTriple = s.find( ' ', offset );
		if( endOfTriple == string::npos ) endOfTriple = length;
		firstSlashOffset = s.find( '/', offset );
		if( firstSlashOffset != string::npos ) {
			secondSlashOffset = s.find( '/', firstSlashOffset + 1 );
			if( secondSlashOffset > endOfTriple ) secondSlashOffset = string::npos;
		}
		else
			secondSlashOffset = string::npos;
		
		// process the vertex index
		int vertexIndex = (firstSlashOffset != string::npos) ? 
            lexical_cast<int>( s.substr( offset, firstSlashOffset - offset ) ) : 
            lexical_cast<int>( s.substr( offset, endOfTriple - offset));
        
		if( vertexIndex < 0 )
			result.mVertexIndices.push_back( group->mBaseVertexOffset + vertexIndex );
		else
			result.mVertexIndices.push_back( vertexIndex - 1 );
			
		// process the tex coord index
		if( includeUVs && ( firstSlashOffset != string::npos ) ) {
			size_t numSize = ( secondSlashOffset == string::npos ) ? ( endOfTriple - firstSlashOffset - 1 ) : secondSlashOffset - firstSlashOffset - 1;
			if( numSize > 0 ) {
				int texCoordIndex = lexical_cast<int>( s.substr( firstSlashOffset + 1, numSize ) );
				if( texCoordIndex < 0 )
					result.mTexCoordIndices.push_back( group->mBaseTexCoordOffset + texCoordIndex );
				else
					result.mTexCoordIndices.push_back( texCoordIndex - 1 );
				if( group->mFaces.empty() )
					group->mHasTexCoords = true;
			}
			else
				group->mHasTexCoords = false;
		}
		else if( group->mFaces.empty() ) // if this is the first face, let's note that this group has no tex coords
			group->mHasTexCoords = false;
			
		// process the normal index
		if( secondSlashOffset != string::npos ) {
			int normalIndex = lexical_cast<int>( s.substr( secondSlashOffset + 1, endOfTriple - secondSlashOffset - 1 ) );
			if( normalIndex < 0 )
				result.mNormalIndices.push_back( group->mBaseNormalOffset + normalIndex );
			else
				result.mNormalIndices.push_back( normalIndex - 1 );
			group->mHasNormals = true;
		}
		else if( group->mFaces.empty() ) // if this is the first face, let's note that this group has no normals
			group->mHasNormals = false;
		
		offset = endOfTriple + 1;
		result.mNumVertices++;
	}
	
	group->mFaces.push_back( result );
}

void ObjLoader::load( size_t groupIndex, boost::tribool loadNormals, boost::tribool loadTexCoords )
{
	bool texCoords;
	if( loadTexCoords ) texCoords = true;
	else if( ! loadTexCoords ) texCoords = false;
	else texCoords = mGroups[groupIndex].mHasTexCoords;

	bool normals;
	if( loadNormals ) normals = true;
	else if( ! loadNormals ) normals = false;
	else normals = mGroups[groupIndex].mHasNormals;

	if( normals && texCoords ) {
		map<VertexTriple,int> uniqueVerts;
		loadGroupNormalsTextures( mGroups[groupIndex], uniqueVerts );
	}
	else if( normals ) {
		map<VertexPair,int> uniqueVerts;
		loadGroupNormals( mGroups[groupIndex], uniqueVerts );
	}
	else if( texCoords ) {
		map<VertexPair,int> uniqueVerts;
		loadGroupTextures( mGroups[groupIndex], uniqueVerts );
	}
	else {
		map<int,int> uniqueVerts;
		loadGroup( mGroups[groupIndex], uniqueVerts );
	}

}

void ObjLoader::load( boost::tribool loadNormals, boost::tribool loadTexCoords )
{
	// sort out if we're loading texCoords
	bool texCoords, normals;
	if( loadTexCoords ) texCoords = true;
	else if( ! loadTexCoords ) texCoords = false;
	else { // determine if any groups have texCoords
		texCoords = false;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt ) {
			if( groupIt->mHasTexCoords )
				texCoords = true;
		}
	
	}

	// sort out if we're loading normals
	if( loadNormals ) normals = true;
	else if( ! loadNormals ) normals = false;
	else { // determine if any groups have normals
		normals = false;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt ) {
			if( groupIt->mHasNormals )
				normals = true;
		}
	}

	if( normals && texCoords ) {
		map<VertexTriple,int> uniqueVerts;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt )
			loadGroupNormalsTextures( *groupIt, uniqueVerts );
	}
	else if( normals ) {
		map<VertexPair,int> uniqueVerts;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt )
			loadGroupNormals( *groupIt, uniqueVerts );
	}
	else if( texCoords ) {
		map<VertexPair,int> uniqueVerts;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt )
			loadGroupTextures( *groupIt, uniqueVerts );
	}
	else {
		map<int,int> uniqueVerts;
		for( vector<Group>::const_iterator groupIt = mGroups.begin(); groupIt != mGroups.end(); ++groupIt )
			loadGroup( *groupIt, uniqueVerts );
	}
}

void ObjLoader::loadGroupNormalsTextures( const Group &group, map<VertexTriple,int> &uniqueVerts )
{
    bool hasColors = mMaterials.size() > 0;
	for( size_t f = 0; f < group.mFaces.size(); ++f ) {
		Vec3f inferredNormal;
		bool forceUnique = false;
		Color rgb;
		if( hasColors ) {
			const Material *m = group.mFaces[f].mMaterial;
			if( m ) {
				rgb.r = m->Kd[0];
				rgb.g = m->Kd[1];
				rgb.b = m->Kd[2];
			}
			else {
				rgb.r = 1;
				rgb.g = 1;
				rgb.b = 1;
			}
		}
		if( group.mFaces[f].mNormalIndices.empty() ) { // we'll have to derive it from two edges
			Vec3f edge1 = mVertices[group.mFaces[f].mVertexIndices[1]] - mVertices[group.mFaces[f].mVertexIndices[0]];
			Vec3f edge2 = mVertices[group.mFaces[f].mVertexIndices[2]] - mVertices[group.mFaces[f].mVertexIndices[0]];
			inferredNormal = edge1.cross( edge2 ).normalized();
			forceUnique = true;
		}
		
		if( group.mFaces[f].mTexCoordIndices.empty() )
			forceUnique = true;
		
		vector<int> faceIndices;
		faceIndices.reserve( group.mFaces[f].mNumVertices );
		for( int v = 0; v < group.mFaces[f].mNumVertices; ++v ) {
			if( ! forceUnique ) {
				VertexTriple triple = make_tuple( group.mFaces[f].mVertexIndices[v], group.mFaces[f].mTexCoordIndices[v], group.mFaces[f].mNormalIndices[v] );
				pair<map<VertexTriple,int>::iterator,bool> result = uniqueVerts.insert( make_pair( triple, mVertices.size() ) );
				if( result.second ) { // we've got a new, unique vertex here, so let's append it
					mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
					mNormals.push_back( mNormals[group.mFaces[f].mNormalIndices[v]] );
					mTexCoords.push_back( mTexCoords[group.mFaces[f].mTexCoordIndices[v]] );
					if( hasColors )
						mColors.push_back( rgb );
				}
				// the unique ID of the vertex is appended for this vert
				faceIndices.push_back( result.first->second );
			}
			else { // have to force unique because this group lacks either normals or texCoords
				faceIndices.push_back( mVertices.size() );

				mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
				if( ! group.mHasNormals )
					mNormals.push_back( inferredNormal );
				else
					mNormals.push_back( mNormals[group.mFaces[f].mNormalIndices[v]] );
				if( ! group.mHasTexCoords )
					mTexCoords.push_back( Vec2f::zero() );
				else
					mTexCoords.push_back( mTexCoords[group.mFaces[f].mTexCoordIndices[v]] );
                if( hasColors )
                    mColors.push_back( rgb );
			}
		}

		int triangles = faceIndices.size() - 2;
		for( int t = 0; t < triangles; ++t ) {
			mIndices.push_back( faceIndices[0] ); mIndices.push_back( faceIndices[t + 1] ); mIndices.push_back( faceIndices[t + 2] );
		}
	}	
}

void ObjLoader::loadGroupNormals( const Group &group, map<VertexPair,int> &uniqueVerts )
{
    bool hasColors = mMaterials.size() > 0;
	for( size_t f = 0; f < group.mFaces.size(); ++f ) {
        Color rgb;
        if( hasColors ) {
			const Material *m = group.mFaces[f].mMaterial;
			if( m ) {
				rgb.r = m->Kd[0];
				rgb.g = m->Kd[1];
				rgb.b = m->Kd[2];
			}
			else {
				rgb.r = 1;
				rgb.g = 1;
				rgb.b = 1;
			}
        }
		Vec3f inferredNormal;
		bool forceUnique = false;
		if( group.mFaces[f].mNormalIndices.empty() ) { // we'll have to derive it from two edges
			Vec3f edge1 = mVertices[group.mFaces[f].mVertexIndices[1]] - mVertices[group.mFaces[f].mVertexIndices[0]];
			Vec3f edge2 = mVertices[group.mFaces[f].mVertexIndices[2]] - mVertices[group.mFaces[f].mVertexIndices[0]];
			inferredNormal = edge1.cross( edge2 ).normalized();
			forceUnique = true;
		}
		
		vector<int> faceIndices;
		faceIndices.reserve( group.mFaces[f].mNumVertices );
		for( int v = 0; v < group.mFaces[f].mNumVertices; ++v ) {
			if( ! forceUnique ) {
				VertexPair triple = make_tuple( group.mFaces[f].mVertexIndices[v], group.mFaces[f].mNormalIndices[v] );
				pair<map<VertexPair,int>::iterator,bool> result = uniqueVerts.insert( make_pair( triple, mVertices.size() ) );
				if( result.second ) { // we've got a new, unique vertex here, so let's append it
					mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
					mNormals.push_back( mNormals[group.mFaces[f].mNormalIndices[v]] );
                    if( hasColors )
                        mColors.push_back( rgb );
				}
				// the unique ID of the vertex is appended for this vert
				faceIndices.push_back( result.first->second );
			}
			else { // have to force unique because this group lacks normals
				faceIndices.push_back( mVertices.size() );

				mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
				if( ! group.mHasNormals )
					mNormals.push_back( inferredNormal );
				else
					mNormals.push_back( mNormals[group.mFaces[f].mNormalIndices[v]] );
                if( hasColors )
                    mColors.push_back( rgb );
			}
		}

		int triangles = faceIndices.size() - 2;
		for( int t = 0; t < triangles; ++t ) {
			mIndices.push_back( faceIndices[0] ); mIndices.push_back( faceIndices[t + 1] ); mIndices.push_back( faceIndices[t + 2] );
		}
	}	
}

void ObjLoader::loadGroupTextures( const Group &group, map<VertexPair,int> &uniqueVerts )
{
    bool hasColors = mMaterials.size() > 0;
	for( size_t f = 0; f < group.mFaces.size(); ++f ) {
        Color rgb;
        if( hasColors ) {
			const Material *m = group.mFaces[f].mMaterial;
			if( m ) {
				rgb.r = m->Kd[0];
				rgb.g = m->Kd[1];
				rgb.b = m->Kd[2];
			}
			else {
				rgb.r = 1;
				rgb.g = 1;
				rgb.b = 1;
			}
		}
		bool forceUnique = false;
		if( group.mFaces[f].mTexCoordIndices.empty() )
			forceUnique = true;
		
		vector<int> faceIndices;
		faceIndices.reserve( group.mFaces[f].mNumVertices );
		for( int v = 0; v < group.mFaces[f].mNumVertices; ++v ) {
			if( ! forceUnique ) {
				VertexPair triple = make_tuple( group.mFaces[f].mVertexIndices[v], group.mFaces[f].mTexCoordIndices[v] );
				pair<map<VertexPair,int>::iterator,bool> result = uniqueVerts.insert( make_pair( triple, mVertices.size() ) );
				if( result.second ) { // we've got a new, unique vertex here, so let's append it
					mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
					mTexCoords.push_back( mTexCoords[group.mFaces[f].mTexCoordIndices[v]] );
                    if( hasColors )
                        mColors.push_back( rgb );
				}
				// the unique ID of the vertex is appended for this vert
				faceIndices.push_back( result.first->second );
			}
			else { // have to force unique because this group lacks texCoords
				faceIndices.push_back( mVertices.size() );

				mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
				if( ! group.mHasTexCoords )
					mTexCoords.push_back( Vec2f::zero() );
				else
					mTexCoords.push_back( mTexCoords[group.mFaces[f].mTexCoordIndices[v]] );
                if( hasColors )
                    mColors.push_back( rgb );
			}
		}

		int triangles = faceIndices.size() - 2;
		for( int t = 0; t < triangles; ++t ) {
			mIndices.push_back( faceIndices[0] ); mIndices.push_back( faceIndices[t + 1] ); mIndices.push_back( faceIndices[t + 2] );
		}
	}	
}

void ObjLoader::loadGroup( const Group &group, map<int,int> &uniqueVerts )
{
    bool hasColors = mMaterials.size() > 0;
	for( size_t f = 0; f < group.mFaces.size(); ++f ) {
        Color rgb;
        if( hasColors ) {
			const Material *m = group.mFaces[f].mMaterial;
            if( m ) {
                rgb.r = m->Kd[0];
                rgb.g = m->Kd[1];
                rgb.b = m->Kd[2];
            }
            else {
                rgb.r = 1;
                rgb.g = 1;
                rgb.b = 1;
            }
        }
		vector<int> faceIndices;
		faceIndices.reserve( group.mFaces[f].mNumVertices );
		for( int v = 0; v < group.mFaces[f].mNumVertices; ++v ) {
			pair<map<int,int>::iterator,bool> result = uniqueVerts.insert( make_pair( group.mFaces[f].mVertexIndices[v], mVertices.size() ) );
			if( result.second ) { // we've got a new, unique vertex here, so let's append it
				mVertices.push_back( mVertices[group.mFaces[f].mVertexIndices[v]] );
                if( hasColors )
                    mColors.push_back( rgb );
			}
			// the unique ID of the vertex is appended for this vert
			faceIndices.push_back( result.first->second );
		}

		int triangles = faceIndices.size() - 2;
		for( int t = 0; t < triangles; ++t ) {
			mIndices.push_back( faceIndices[0] ); mIndices.push_back( faceIndices[t + 1] ); mIndices.push_back( faceIndices[t + 2] );
		}
	}	
}

/*
void ObjLoader::write( DataTargetRef dataTarget, const TriMesh &mesh, bool writeNormals, bool includeUVs )
{
	OStreamRef stream = dataTarget->getStream();
	const size_t numVerts = mesh.getNumVertices();
	for( size_t p = 0; p < numVerts; ++p ) {
		ostringstream os;
		os << "v " << mesh.getVertices()[p].x << " " << mesh.getVertices()[p].y << " " << mesh.getVertices()[p].z << std::endl;
		stream->writeData( os.str().c_str(), os.str().length() );
	}

	const bool processTexCoords = mesh.hasTexCoords() && includeUVs;
	if( processTexCoords ) {
		for( size_t p = 0; p < numVerts; ++p ) {
			ostringstream os;
			os << "vt " << mesh.getTexCoords()[p].x << " " << mesh.getTexCoords()[p].y << std::endl;
			stream->writeData( os.str().c_str(), os.str().length() );
		}
	}
	
	const bool processNormals = mesh.hasNormals() && writeNormals;
	if( processNormals ) {
		for( size_t p = 0; p < numVerts; ++p ) {
			ostringstream os;
			os << "vn " << mesh.getNormals()[p].x << " " << mesh.getNormals()[p].y << " " << mesh.getNormals()[p].z << std::endl;
			stream->writeData( os.str().c_str(), os.str().length() );
		}
	}
	
	const size_t numTriangles = mesh.getNumTriangles();
	const std::vector<uint32_t>& indices( mesh.getIndices() );
	for( size_t t = 0; t < numTriangles; ++t ) {
		ostringstream os;
		os << "f ";
		if( processNormals && processTexCoords ) {
			os << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << " ";
			os << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << " ";
			os << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << " ";
		}
		else if ( processNormals ) {
			os << indices[t*3+0]+1 << "//" << indices[t*3+0]+1 << " ";
			os << indices[t*3+1]+1 << "//" << indices[t*3+1]+1 << " ";
			os << indices[t*3+2]+1 << "//" << indices[t*3+2]+1 << " ";
		}
		else if( processTexCoords ) {
			os << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << " ";
			os << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << " ";
			os << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << " ";
		}
		else { // just verts
			os << indices[t*3+0]+1 << " ";
			os << indices[t*3+1]+1 << " ";
			os << indices[t*3+2]+1 << " ";			
		}
		os << std::endl;
		stream->writeData( os.str().c_str(), os.str().length() );
	}
}*/

} // namespace cinder
