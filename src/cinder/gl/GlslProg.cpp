/*
 Copyright (c) 2010, The Barbarian Group
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

#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Context.h"

using namespace std;

namespace cinder { namespace gl {

GlslProg::UniformSemanticMap	GlslProg::sDefaultUniformNameToSemanticMap;
GlslProg::AttribSemanticMap		GlslProg::sDefaultAttribNameToSemanticMap;

//////////////////////////////////////////////////////////////////////////
// GlslProg::Format
GlslProg::Format::Format()
{
	mAttribSemanticLocMap[geom::Attrib::POSITION] = 0;
}

GlslProg::Format& GlslProg::Format::vertex( const DataSourceRef &dataSource )
{
	if( dataSource ) {
		Buffer buffer( dataSource );
		mVertexShader = std::unique_ptr<char>( new char[buffer.getDataSize() + 1] );
		memcpy( mVertexShader.get(), buffer.getData(), buffer.getDataSize() );
		mVertexShader.get()[buffer.getDataSize()] = 0;
	}
	else
		mVertexShader.reset();

	return *this;
}

GlslProg::Format& GlslProg::Format::vertex( const char *vertexShader )
{
	if( vertexShader ) {
		const size_t stringSize = strlen( vertexShader );
		mVertexShader = std::unique_ptr<char>( new char[stringSize + 1] );
		strcpy( mVertexShader.get(), vertexShader );
	}
	else
		mVertexShader.reset();

	return *this;
}

GlslProg::Format& GlslProg::Format::fragment( const DataSourceRef &dataSource )
{
	if( dataSource ) {
		Buffer buffer( dataSource );
		mFragmentShader = std::unique_ptr<char>( new char[buffer.getDataSize() + 1] );
		memcpy( mFragmentShader.get(), buffer.getData(), buffer.getDataSize() );
		mFragmentShader.get()[buffer.getDataSize()] = 0;
	}
	else
		mFragmentShader.reset();
		
	return *this;
}

GlslProg::Format& GlslProg::Format::fragment( const char *fragmentShader )
{
	if( fragmentShader ) {
		const size_t stringSize = strlen( fragmentShader );
		mFragmentShader = std::unique_ptr<char>( new char[stringSize + 1] );
		strcpy( mFragmentShader.get(), fragmentShader );
	}
	else
		mFragmentShader.reset();

	return *this;
}

#if ! defined( CINDER_GLES )
GlslProg::Format& GlslProg::Format::geometry( const DataSourceRef &dataSource )
{
	if( dataSource ) {
		Buffer buffer( dataSource );
		mGeometryShader = std::unique_ptr<char>( new char[buffer.getDataSize() + 1] );
		memcpy( mGeometryShader.get(), buffer.getData(), buffer.getDataSize() );
		mGeometryShader.get()[buffer.getDataSize()] = 0;
	}
	else
		mGeometryShader.reset();
		
	return *this;
}

GlslProg::Format& GlslProg::Format::geometry( const char *geometryShader )
{
	if( geometryShader ) {
		const size_t stringSize = strlen( geometryShader );
		mGeometryShader = std::unique_ptr<char>( new char[stringSize + 1] );
		strcpy( mGeometryShader.get(), geometryShader );
	}
	else
		mGeometryShader.reset();

	return *this;
}
#endif // ! defined( CINDER_GLES )

GlslProg::Format& GlslProg::Format::attrib( geom::Attrib semantic, const std::string &attribName )
{
	mAttribSemanticMap[attribName] = semantic;
	return *this;
}

GlslProg::Format& GlslProg::Format::uniform( UniformSemantic semantic, const std::string &attribName )
{
	mUniformSemanticMap[attribName] = semantic;
	return *this;
}

GlslProg::Format& GlslProg::Format::attribLocation( const std::string &attribName, GLint location )
{
	mAttribNameLocMap[attribName] = location;
	return *this;
}

GlslProg::Format& GlslProg::Format::attribLocation( geom::Attrib attrib, GLint location )
{
	mAttribSemanticLocMap[attrib] = location;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
// GlslProg statics

GlslProgRef GlslProg::create( const Format &format )
{
	return GlslProgRef( new GlslProg( format ) );
}

GlslProgRef GlslProg::create( DataSourceRef vertexShader, DataSourceRef fragmentShader )
{
	return GlslProgRef( new GlslProg( GlslProg::Format().vertex( vertexShader ).fragment( fragmentShader ) ) );
}

GlslProgRef GlslProg::create( const char *vertexShader, const char *fragmentShader )
{
	return GlslProgRef( new GlslProg( GlslProg::Format().vertex( vertexShader ).fragment( fragmentShader ) ) );
}
	
GlslProg::~GlslProg()
{
	if ( mHandle ) {
		glDeleteProgram( (GLuint)mHandle );
	}
}

//////////////////////////////////////////////////////////////////////////
// GlslProg

GlslProg::GlslProg( const Format &format )
	: mActiveUniformTypesCached( false ), mActiveAttribTypesCached( false ),
	mUniformSemanticsCached( false ), mUniformNameToSemanticMap( getDefaultUniformNameToSemanticMap() ),
	mAttribSemanticsCached( false ), mAttribNameToSemanticMap( getDefaultAttribNameToSemanticMap() )
{
	mHandle = glCreateProgram();
	
	if( format.getVertex() )
		loadShader( format.getVertex(), GL_VERTEX_SHADER );
	if( format.getFragment() )
		loadShader( format.getFragment(), GL_FRAGMENT_SHADER );
#if ! defined( CINDER_GLES )
	if( format.getGeometry() )
		loadShader( format.getGeometry(), GL_GEOMETRY_SHADER );
#endif

	// copy the Format's attribute-semantic map
	for( auto &attribSemantic : format.getAttribSemantics() )
		mAttribNameToSemanticMap.insert( attribSemantic );

	// copy the Format's uniform-semantic map
	for( auto &uniformSemantic : format.getUniformSemantics() )
		mUniformNameToSemanticMap.insert( uniformSemantic );

	// THESE sections take all attribute locations which have been specified (either by their semantic or their names)
	// and ultimately maps them via glBindAttribLocation, which must be done ahead of linking
	auto attribLocations = format.getAttribNameLocations();
	
	// map the locations-specified semantics to their respective attribute name locations
	for( auto &semanticLoc : format.getAttribSemanticLocations() ) {
		string attribName;
		// first find if we have an attribute associated with a given semantic
		for( auto &attribSemantic : mAttribNameToSemanticMap ) {
			if( attribSemantic.second == semanticLoc.first ) {
				attribName = attribSemantic.first;
				break;
			}
		}
		
		// if we found an appropriate attribute-semantic pair, set attribLocations[attrib name] to be the semantic location
		if( ! attribName.empty() )
			attribLocations[attribName] = semanticLoc.second;
	}
	
	// finally, bind all location-specified attributes to their respective locations
	for( auto &attribLoc : attribLocations )
		glBindAttribLocation( mHandle, attribLoc.second, attribLoc.first.c_str() );
	
	link();
}

GlslProg::UniformSemanticMap& GlslProg::getDefaultUniformNameToSemanticMap()
{
	static bool initialized = false;
	if( ! initialized ) {
		sDefaultUniformNameToSemanticMap["ciModelView"] = UNIFORM_MODELVIEW;
		sDefaultUniformNameToSemanticMap["ciModelViewProjection"] = UNIFORM_MODELVIEWPROJECTION;
		sDefaultUniformNameToSemanticMap["ciProjection"] = UNIFORM_PROJECTION;
		sDefaultUniformNameToSemanticMap["ciNormalMatrix"] = UNIFORM_NORMAL_MATRIX;
		initialized = true;
	}
	
	return sDefaultUniformNameToSemanticMap;
}

GlslProg::AttribSemanticMap& GlslProg::getDefaultAttribNameToSemanticMap()
{
	static bool initialized = false;
	if( ! initialized ) {
		sDefaultAttribNameToSemanticMap["ciPosition"] = geom::Attrib::POSITION;
		sDefaultAttribNameToSemanticMap["ciNormal"] = geom::Attrib::NORMAL;
		sDefaultAttribNameToSemanticMap["ciTexCoord0"] = geom::Attrib::TEX_COORD_0;
		sDefaultAttribNameToSemanticMap["ciTexCoord1"] = geom::Attrib::TEX_COORD_1;
		sDefaultAttribNameToSemanticMap["ciTexCoord2"] = geom::Attrib::TEX_COORD_2;
		sDefaultAttribNameToSemanticMap["ciTexCoord3"] = geom::Attrib::TEX_COORD_3;						
		sDefaultAttribNameToSemanticMap["ciColor"] = geom::Attrib::COLOR;
		sDefaultAttribNameToSemanticMap["ciBoneIndex"] = geom::Attrib::BONE_INDEX;
		sDefaultAttribNameToSemanticMap["ciBoneWeight"] = geom::Attrib::BONE_WEIGHT;
		initialized = true;
	}
	
	return sDefaultAttribNameToSemanticMap;
}

void GlslProg::loadShader( const char *shaderSource, GLint shaderType )
{
	GLuint handle = glCreateShader( shaderType );
	glShaderSource( handle, 1, reinterpret_cast<const GLchar**>( &shaderSource ), NULL );
	glCompileShader( handle );
	
	GLint status;
	glGetShaderiv( (GLuint) handle, GL_COMPILE_STATUS, &status );
	if( status != GL_TRUE ) {
		std::string log = getShaderLog( (GLuint)handle );
		throw GlslProgCompileExc( log, shaderType );
	}
	glAttachShader( mHandle, handle );
}

void GlslProg::link()
{
	glLinkProgram( mHandle );
}

void GlslProg::bind() const
{
	// this will in turn call bindImpl; this is so that the context can update its reference to the active shader
	gl::context()->bindGlslProg( std::const_pointer_cast<GlslProg>( shared_from_this() ) );
}

// This is called by the Context whenever a GlslProg is bound. The indirection is so that the Context can update its reference to the active shader
void GlslProg::bindImpl()
{
	glUseProgram( mHandle );
}

std::string GlslProg::getShaderLog( GLuint handle ) const
{
	string log;
	
	GLchar* debugLog;
	GLint charsWritten	= 0;
	GLint debugLength	= 0;
	glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &debugLength );
	
	if ( debugLength > 0 ) {
		debugLog = new GLchar[ debugLength ];
		glGetShaderInfoLog( handle, debugLength, &charsWritten, debugLog );
		log.append( debugLog, 0, debugLength );
		delete [] debugLog;
	}
	
	return log;
}

// int
void GlslProg::uniform( int location, int data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform1i( location, data );
}

void GlslProg::uniform( const std::string &name, int data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );
	glUniform1i( loc, data );
}

// Vec2i
void GlslProg::uniform( int location, const Vec2i &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform2i( location, data.x, data.y );
}

void GlslProg::uniform( const std::string &name, const Vec2i &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform2i( loc, data.x, data.y );
}

// int *, count
void GlslProg::uniform( int location, const int *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform1iv( location, count, data );
}

void GlslProg::uniform( const std::string &name, const int *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform1iv( loc, count, data );
}

// Vec2i *, count
void GlslProg::uniform( int location, const Vec2i *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform2iv( location, count, &data[0].x );
}

void GlslProg::uniform( const std::string &name, const Vec2i *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform2iv( loc, count, &data[0].x );
}

// float
void GlslProg::uniform( int location, float data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform1f( location, data );
}

void GlslProg::uniform( const std::string &name, float data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform1f( loc, data );
}

// Vec2f
void GlslProg::uniform( int location, const Vec2f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform2f( location, data.x, data.y );
}

void GlslProg::uniform( const std::string &name, const Vec2f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform2f( loc, data.x, data.y );
}

// Vec3f
void GlslProg::uniform( int location, const Vec3f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform3f( location, data.x, data.y, data.z );
}

void GlslProg::uniform( const std::string &name, const Vec3f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform3f( loc, data.x, data.y, data.z );
}

// Vec4f
void GlslProg::uniform( int location, const Vec4f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform4f( location, data.x, data.y, data.z, data.w );
}

void GlslProg::uniform( const std::string &name, const Vec4f &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform4f( loc, data.x, data.y, data.z, data.w );
}

// Matrix33f
void GlslProg::uniform( int location, const Matrix33f &data, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    glUniformMatrix3fv( location, 1, ( transpose ) ? GL_TRUE : GL_FALSE, data.m );
}

void GlslProg::uniform( const std::string &name, const Matrix33f &data, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
    glUniformMatrix3fv( loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE, data.m );
}

// Matrix44f
void GlslProg::uniform( int location, const Matrix44f &data, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    glUniformMatrix4fv( location, 1, ( transpose ) ? GL_TRUE : GL_FALSE, data.m );
}

void GlslProg::uniform( const std::string &name, const Matrix44f &data, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
    glUniformMatrix4fv( loc, 1, ( transpose ) ? GL_TRUE : GL_FALSE, data.m );
}

// Color
void GlslProg::uniform( int location, const Color &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform3f( location, data.r, data.g, data.b );
}

void GlslProg::uniform( const std::string &name, const Color &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform3f( loc, data.r, data.g, data.b );
}

// ColorA
void GlslProg::uniform( int location, const ColorA &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform4f( location, data.r, data.g, data.b, data.a );
}

void GlslProg::uniform( const std::string &name, const ColorA &data ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform4f( loc, data.r, data.g, data.b, data.a );
}

// float*, count
void GlslProg::uniform( int location, const float *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform1fv( location, count, data );
}

void GlslProg::uniform( const std::string &name, const float *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform1fv( loc, count, data );
}

// Vec2f*, count
void GlslProg::uniform( int location, const Vec2f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform2fv( location, count, &data[0].x );
}

void GlslProg::uniform( const std::string &name, const Vec2f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform2fv( loc, count, &data[0].x );
}

// Vec3f*, count
void GlslProg::uniform( int location, const Vec3f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform3fv( location, count, &data[0].x );
}

void GlslProg::uniform( const std::string &name, const Vec3f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform3fv( loc, count, &data[0].x );
}

// Vec4f*, count
void GlslProg::uniform( int location, const Vec4f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	glUniform4fv( location, count, &data[0].x );
}

void GlslProg::uniform( const std::string &name, const Vec4f *data, int count ) const
{
	GlslProgScope shaderBind( shared_from_this() );
	GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
	glUniform4fv( loc, count, &data[0].x );
}

// Matrix33f*, count
void GlslProg::uniform( int location, const Matrix33f *data, int count, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    glUniformMatrix3fv( location, count, ( transpose ) ? GL_TRUE : GL_FALSE, &data->m[0] );
}

void GlslProg::uniform( const std::string &name, const Matrix33f *data, int count, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
    glUniformMatrix3fv( loc, count, ( transpose ) ? GL_TRUE : GL_FALSE, &data->m[0] );
}

// Matrix44f*, count
void GlslProg::uniform( int location, const Matrix44f *data, int count, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    glUniformMatrix4fv( location, count, ( transpose ) ? GL_TRUE : GL_FALSE, &data->m[0] );
}

void GlslProg::uniform( const std::string &name, const Matrix44f *data, int count, bool transpose ) const
{
    GlslProgScope shaderBind( shared_from_this() );
    GLint loc = getUniformLocation( name );
	if( loc == -1 )
		throw GlslUnknownUniform( name );	
    glUniformMatrix4fv( loc, count, ( transpose ) ? GL_TRUE : GL_FALSE, &data->m[0] );
}

GLint GlslProg::getUniformLocation( const std::string &name ) const
{
	map<string,int>::const_iterator uniformIt = mUniformLocs.find( name );
	if( uniformIt == mUniformLocs.end() ) {
		GLint loc = glGetUniformLocation( mHandle, name.c_str() );
		mUniformLocs[name] = loc;
		return loc;
	}
	else {
		return uniformIt->second;
	}
}

const std::map<std::string,GLenum>& GlslProg::getActiveUniformTypes() const
{
	if( ! mActiveUniformTypesCached ) {
		GLint numActiveUniforms = 0;
		glGetProgramiv( mHandle, GL_ACTIVE_UNIFORMS, &numActiveUniforms );
		for( GLint i = 0; i < numActiveUniforms; ++i ) {
			char name[512];
			GLsizei nameLength;
			GLint size;
			GLenum type;
			glGetActiveUniform( mHandle, (GLuint)i, 511, &nameLength, &size, &type, name );
			name[nameLength] = 0;
			mActiveUniformTypes[name] = type;
		}
		mActiveUniformTypesCached = true;
	}
	return mActiveUniformTypes;
}

const std::map<std::string,GLenum>& GlslProg::getActiveAttribTypes() const
{
	if( ! mActiveAttribTypesCached ) {
		GLint numActiveAttrs = 0;
		glGetProgramiv( mHandle, GL_ACTIVE_ATTRIBUTES, &numActiveAttrs );
		for( GLint i = 0; i < numActiveAttrs; ++i ) {
			char name[512];
			GLsizei nameLength;
			GLint size;
			GLenum type;
			glGetActiveAttrib( mHandle, (GLuint)i, 511, &nameLength, &size, &type, name );
			name[nameLength] = 0;
			mActiveAttribTypes[name] = type;
		}
		mActiveAttribTypesCached = true;
	}
	return mActiveAttribTypes;
}

const GlslProg::UniformSemanticMap& GlslProg::getUniformSemantics() const
{
	if( ! mUniformSemanticsCached ) {
		auto activeUniformTypes = getActiveUniformTypes();
	
		for( auto activeUnifIt = activeUniformTypes.begin(); activeUnifIt != activeUniformTypes.end(); ++activeUnifIt ) {
			// first find this active uniform by name in the mUniformNameToSemanticMap
			auto semantic = mUniformNameToSemanticMap.find( activeUnifIt->first );
			if( semantic != mUniformNameToSemanticMap.end() ) {
				// found this semantic, add it mUniformSemantics
				mUniformSemantics[semantic->first] = semantic->second;
			}
		}
	
		mUniformSemanticsCached = true;
	}
	
	return mUniformSemantics;
}

const GlslProg::AttribSemanticMap& GlslProg::getAttribSemantics() const
{
	if( ! mAttribSemanticsCached ) {
		auto activeAttrTypes = getActiveAttribTypes();
	
		for( auto activeAttrIt = activeAttrTypes.begin(); activeAttrIt != activeAttrTypes.end(); ++activeAttrIt ) {
			// first find this active attribute by name in the mAttrNameToSemanticMap
			auto semantic = mAttribNameToSemanticMap.find( activeAttrIt->first );
			if( semantic != mAttribNameToSemanticMap.end() ) {
				// found this semantic, add it mAttrSemantics
				mAttribSemantics[semantic->first] = semantic->second;
			}
		}
	
		mAttribSemanticsCached = true;
	}
	
	return mAttribSemantics;
}

bool GlslProg::hasAttribSemantic( geom::Attrib semantic ) const
{
	auto semantics = getAttribSemantics();
	for( auto semIt = semantics.begin(); semIt != semantics.end(); ++semIt ) {
		if( semIt->second == semantic )
			return true;
	}
	
	return false;
}

GLint GlslProg::getAttribSemanticLocation( geom::Attrib semantic ) const
{
	auto semantics = getAttribSemantics();
	for( auto semIt = semantics.begin(); semIt != semantics.end(); ++semIt ) {
		if( semIt->second == semantic )
			return getAttribLocation( semIt->first );
	}
	
	return -1;
}

GLint GlslProg::getAttribLocation( const std::string &name ) const
{
	auto existing = mAttribLocs.find( name );
	if( existing == mAttribLocs.end() ) {
		const GLint loc = glGetAttribLocation( mHandle, name.c_str() );
		if( loc != -1 )
			mAttribLocs[name] = loc;
		return loc;
	}
	else
		return existing->second;
}

std::ostream& operator<<( std::ostream &lhs, const GlslProgRef &rhs )
{
	lhs << *rhs;
	return lhs;
}

std::ostream& operator<<( std::ostream &lhs, const GlslProg &rhs )
{
	lhs << "ID: " << rhs.mHandle << std::endl;
	lhs << " Uniforms: " << std::endl;
	auto uniformTypes = rhs.getActiveUniformTypes();
	for( auto &uni : uniformTypes ) {
		lhs << "  \"" << uni.first << "\":" << std::endl;
		lhs << "    Loc: " << rhs.getUniformLocation( uni.first ) << std::endl;
		lhs << "    Type: " << gl::typeToString( uni.second ) << std::endl;
		auto semIt = rhs.getUniformSemantics().find( uni.first );
		if( semIt != rhs.getUniformSemantics().end() ) {
			lhs << "    Semantic: <" << gl::uniformSemanticToString( semIt->second ) << ">" << std::endl;
		}
	}

	auto attribTypes = rhs.getActiveAttribTypes();
	lhs << " Attributes: " << std::endl;
	for( auto &attrib : attribTypes ) {
		lhs << "  \"" << attrib.first << "\":" << std::endl;
		lhs << "    Loc: " << rhs.getAttribLocation( attrib.first ) << std::endl;
		lhs << "    Type: " << gl::typeToString( attrib.second ) << std::endl;
		auto semIt = rhs.getAttribSemantics().find( attrib.first );
		if( semIt != rhs.getAttribSemantics().end() ) {
			lhs << "    Semantic: <" << geom::attribToString( semIt->second ) << ">" << std::endl;
		}
	}	

	return lhs;
}

//////////////////////////////////////////////////////////////////////////
// GlslProgCompileExc
GlslProgCompileExc::GlslProgCompileExc( const std::string &log, GLint aShaderType ) throw()
: mShaderType( aShaderType )
{
	if( mShaderType == GL_VERTEX_SHADER )
		strncpy( mMessage, "VERTEX: ", 1000 );
	else if( mShaderType == GL_FRAGMENT_SHADER )
		strncpy( mMessage, "FRAGMENT: ", 1000 );
	else
		strncpy( mMessage, "UNKNOWN: ", 1000 );
	strncat( mMessage, log.c_str(), 15000 );
}
	
} } // namespace cinder::gl
