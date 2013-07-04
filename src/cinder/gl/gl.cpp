#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Texture.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include <vector>

using namespace std;

namespace cinder { namespace gl {

Context* context()
{
	static Context *sContext = NULL;
	if( ! sContext ) {
		sContext = new Context();
	}
	return sContext;
}

bool isExtensionAvailable( const std::string& extName )
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
		sExtMap[ extName ] = found;
		return found;
	} else {
		return extIt->second;
	}
}

void setDefaultShaderUniforms()
{
	auto ctx = gl::context();
	auto glslProg = ctx->shaderGet();
	if( glslProg ) {
		auto uniforms = glslProg->getUniformSemantics();
		for( auto unifIt = uniforms.cbegin(); unifIt != uniforms.end(); ++unifIt ) {
			switch( unifIt->second ) {
				case UNIFORM_MODELVIEWPROJECTION:
					glslProg->uniform( unifIt->first, gl::getProjection() * gl::getModelView() );
				break;
			}
		}
	}
}

void clear( const ColorA& color, bool clearDepthBuffer )
{
	glClearColor( color.r, color.g, color.b, color.a );
	if ( clearDepthBuffer ) {
		gl::context()->depthMask( GL_TRUE );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	} else {
		glClear( GL_COLOR_BUFFER_BIT );
	}
}

Area getViewport()
{
	GLint params[ 4 ];
	glGetIntegerv( GL_VIEWPORT, params );
	Area result;
	return Area( params[ 0 ], params[ 1 ], params[ 0 ] + params[ 2 ], params[ 1 ] + params[ 3 ] );
}

void setViewport( const Area& area )
{
	glViewport( area.x1, area.y1, ( area.x2 - area.x1 ), ( area.y2 - area.y1 ) );
}

void enable( GLenum state, bool enable )
{
	context()->enable( state, enable );
}

void enableAlphaBlending( bool premultiplied )
{
	auto ctx = gl::context();
	ctx->enable( GL_BLEND );
	if( ! premultiplied ) {
		ctx->blendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		ctx->blendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	}
}

void disableAlphaBlending()
{
	gl::disable( GL_BLEND );
}

void enableAdditiveBlending()
{
	auto ctx = gl::context();
	ctx->enable( GL_BLEND );
	ctx->blendFunc( GL_SRC_ALPHA, GL_ONE );
}

void disableDepthRead()
{
	gl::disable( GL_DEPTH_TEST );
}

void enableDepthRead( bool enable )
{
	gl::enable( GL_DEPTH_TEST, enable );
}

void enableDepthWrite( bool enable )
{
	gl::context()->depthMask( enable ? GL_TRUE : GL_FALSE );
}

void disableDepthWrite()
{
	gl::context()->depthMask( GL_FALSE );
}

void enableLighting( bool enable )
{
	auto ctx	= gl::context();
	ctx->mLighting	= enable;
}

void disableLighting()
{
	auto ctx	= gl::context();
	ctx->mLighting	= false;
}
	
void enableWireframe( bool enable )
{
	auto ctx	= gl::context();
	ctx->mWireframe = enable;
}

void disableWireframe()
{
	auto ctx	= gl::context();
	ctx->mWireframe = false;
}
	
void setMatrices( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() = cam.getModelViewMatrix();
	ctx->mProjection.back() = cam.getProjectionMatrix();
}

void setModelView( const ci::Matrix44f &m )
{
	gl::context()->mModelView.back() = m;
}

void setModelView( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() = cam.getModelViewMatrix();
}

void setProjection( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mProjection.back() = cam.getProjectionMatrix();
}

void setProjection( const ci::Matrix44f &m )
{
	gl::context()->mProjection.back() = m;
}

void pushModelView()
{
	auto ctx	= gl::context();
	ctx->mModelView.push_back( ctx->mModelView.back() );
}

void popModelView()
{
	auto ctx	= gl::context();
	ctx->mModelView.pop_back();
}

void pushProjection( const ci::Camera& cam )
{
	auto ctx	= gl::context();
	ctx->mProjection.push_back( cam.getProjectionMatrix().m );
}

void pushMatrices()
{
	auto ctx		= gl::context();
	Matrix44f modelView		= ctx->mModelView.back();
	Matrix44f projection	= ctx->mProjection.back();
	ctx->mModelView.push_back( modelView );
	ctx->mProjection.push_back( projection );
}

void popMatrices()
{
	auto ctx	= gl::context();
	if ( ctx->mModelView.size() > 1 && ctx->mProjection.size() > 1 ) {
		ctx->mModelView.pop_back();
		ctx->mProjection.pop_back();
	}
}

void multModelView( const ci::Matrix44f& mtx )
{
	auto ctx	= gl::context();
	ctx->mModelView.back() *= mtx;
}

void multProjection( const ci::Matrix44f& mtx )
{
	auto ctx	= gl::context();
	ctx->mProjection.back() *= mtx;
}

Matrix44f getModelView()
{
	auto ctx	= gl::context();
	return ctx->mModelView.back();
}

Matrix44f getProjection()
{
	auto ctx	= gl::context();
	return ctx->mProjection.back();
}

Matrix33f calcNormalMatrix()
{
	Matrix33f mv = getModelView().subMatrix33( 0, 0 );
	mv.invert( FLT_MIN );
	mv.transpose();
	return mv;
}

void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	// TODO add perspective
	// TODO enable origin
	auto ctx	= gl::context();
	ctx->mModelView.back().setToIdentity();
	ctx->mProjection.back().setRows( Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
										Vec4f( 0.0f, 2.0f / -(float)screenHeight, 0.0f, 1.0f ),
										Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
										Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
}

void setMatricesWindowPersp( const ci::Vec2i& screenSize, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	setMatricesWindowPersp( screenSize.x, screenSize.y, fovDegrees, nearPlane, farPlane, originUpperLeft );
}

void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft )
{
	// TODO enable origin
	auto ctx	= gl::context();
	ctx->mModelView.back().setToIdentity();
	ctx->mProjection.back().setRows( Vec4f( 2.0f / (float)screenWidth, 0.0f, 0.0f, -1.0f ),
										Vec4f( 0.0f, 2.0f / -(float)screenHeight, 0.0f, 1.0f ),
										Vec4f( 0.0f, 0.0f, -1.0f, 0.0f ),
										Vec4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
}

void setMatricesWindow( const ci::Vec2i& screenSize, bool originUpperLeft )
{
	setMatricesWindow( screenSize.x, screenSize.y, originUpperLeft );
}

void rotate( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().rotate( Vec3f( toRadians( v.x ), toRadians( v.y ), toRadians( v.z ) ) );
}
	
void scale( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().scale( v );
}

void translate( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mModelView.back().translate( v );
}
	
void begin( GLenum mode )
{
	auto ctx	= gl::context();
	ctx->mMode		= mode;
	ctx->clear();
}

void end()
{
	auto ctx = gl::context();
//	ctx->draw();
}

void color( float r, float g, float b )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( r, g, b, 1.0f );
}

void color( float r, float g, float b, float a )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( r, g, b, a );
}

void color( const ci::Color& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::ColorA& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::Color8u& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void color( const ci::ColorA8u& c )
{
	auto ctx	= gl::context();
	ctx->mColor		= ColorAf( c );
}

void normal( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mNormal	= v;
}

void texCoord( float s )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, 0.0f, 0.0f, 0.0f );
}

void texCoord( float s, float t )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, 0.0f, 0.0f );
}

void texCoord( float s, float t, float r )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, r, 0.0f );
}

void texCoord( float s, float t, float r, float q )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( s, t, r, q );
}

void texCoord( const ci::Vec2f& v )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( v.x, v.y, 0.0f, 0.0f );
}

void texCoord( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= Vec4f( v.x, v.y, v.z, 0.0f );
}

void texCoord( const ci::Vec4f& v )	
{
	auto ctx	= gl::context();
	ctx->mTexCoord	= v;
}

void vertex( float x, float y )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, 0.0f, 0.0f ) );
}

void vertex( float x, float y, float z )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, z, 0.0f ) );
}

void vertex( float x, float y, float z, float w )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( x, y, z, w ) );
}

void vertex( const ci::Vec2f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( v.x, v.y, 0.0f, 0.0f ) );
}

void vertex( const ci::Vec3f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( Vec4f( v.x, v.y, v.z, 0.0f ) );
}
	
void vertex( const ci::Vec4f& v )
{
	auto ctx	= gl::context();
	ctx->pushBack( v );
}

#if ! defined( CINDER_GLES )
void polygonMode( GLenum face, GLenum mode )
{
	auto ctx = gl::context();
	ctx->polygonMode( face, mode );
}
#endif

void draw( const VboRef& vbo )
{
	drawRange( vbo );
}

void drawRange( const VboRef& vbo, GLint start, GLsizei count )
{
	auto ctx	= gl::context();
	GLenum mode			= ctx->mMode;
	if ( ctx->mWireframe && mode != GL_POINTS ) {
		mode = GL_LINE_STRIP;
	}
	
	vbo->bind();
	if ( vbo->getUsage() == GL_STATIC_DRAW ) {
		if ( vbo->getTarget() == GL_ELEMENT_ARRAY_BUFFER && vbo->getSize() >= sizeof( GLuint ) ) {
			if ( count == 0 ) {
				count = vbo->getSize() / sizeof( GLuint ) - start;
			}
			glDrawElements( mode, count, GL_UNSIGNED_INT, (const GLvoid*)start );
		}
	} else {
		glDrawArrays( mode, start, count );
	}
	vbo->unbind();
}

void draw( const VboMeshRef& mesh )
{
	drawRange( mesh );
}

void drawRange( const VboMeshRef& mesh, GLint start, GLsizei count )
{
	auto vaoBind( mesh->mVao );
	
	if ( mesh->mVboIndices ) {
		if ( mesh->mVboVerticesDynamic ) {
			mesh->mVboVerticesDynamic->bind();
		}
		if ( mesh->mVboVerticesStatic ) {
			mesh->mVboVerticesStatic->bind();
		}
		drawRange( mesh->mVboIndices, start, count );
		if ( mesh->mVboVerticesDynamic ) {
			mesh->mVboVerticesDynamic->unbind();
		}
		if ( mesh->mVboVerticesStatic ) {
			mesh->mVboVerticesStatic->unbind();
		}
	} else {
		if ( mesh->mVboVerticesDynamic ) {
			drawRange( mesh->mVboVerticesDynamic, start, count );
		}
		if ( mesh->mVboVerticesStatic ) {
			drawRange( mesh->mVboVerticesStatic, start, count );
		}
	}
	
	mesh->mVao->unbind();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Attributes
void vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	context()->vertexAttribPointer( index, size, type, normalized, stride, pointer );
}

void enableVertexAttribArray( GLuint index )
{
	context()->enableVertexAttribArray( index );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffers
void vertexAttrib1f( GLuint index, float v0 )
{
	context()->vertexAttrib1f( index, v0 );
}

void vertexAttrib2f( GLuint index, float v0, float v1 )
{
	context()->vertexAttrib2f( index, v0, v1 );
}

void vertexAttrib3f( GLuint index, float v0, float v1, float v2 )
{
	context()->vertexAttrib3f( index, v0, v1, v2 );
}

void vertexAttrib4f( GLuint index, float v0, float v1, float v2, float v3 )
{
	context()->vertexAttrib4f( index, v0, v1, v2, v3 );
}

void bindBuffer( const BufferObjRef &buffer )
{
	context()->bindBuffer( buffer->getTarget(), buffer->getId() );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw*
void drawArrays( GLenum mode, GLint first, GLsizei count )
{
	context()->prepareDraw();
	glDrawArrays( mode, first, count );
}

void drawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	context()->prepareDraw();
	glDrawElements( mode, count, type, indices );
}

void drawCube( const Vec3f &c, const Vec3f &size )
{
	GLfloat sx = size.x * 0.5f;
	GLfloat sy = size.y * 0.5f;
	GLfloat sz = size.z * 0.5f;
	GLfloat vertices[24*3]={c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,		// +X
							c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,		// +Y
							c.x+1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,		// +Z
							c.x+-1.0f*sx,c.y+1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	// -X
							c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+1.0f*sz,	// -Y
							c.x+1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+-1.0f*sy,c.z+-1.0f*sz,	c.x+-1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz,	c.x+1.0f*sx,c.y+1.0f*sy,c.z+-1.0f*sz};	// -Z


	static GLfloat normals[24*3]={ 1,0,0,	1,0,0,	1,0,0,	1,0,0,
								  0,1,0,	0,1,0,	0,1,0,	0,1,0,
									0,0,1,	0,0,1,	0,0,1,	0,0,1,
								  -1,0,0,	-1,0,0,	-1,0,0,	-1,0,0,
								  0,-1,0,	0,-1,0,  0,-1,0,0,-1,0,
								  0,0,-1,	0,0,-1,	0,0,-1,	0,0,-1};

	static GLubyte colors[24*4]={	255,0,0,255,	255,0,0,255,	255,0,0,255,	255,0,0,255,	// +X = red
									0,255,0,255,	0,255,0,255,	0,255,0,255,	0,255,0,255,	// +Y = green
									0,0,255,255,	0,0,255,255,	0,0,255,255,	0,0,255,255,	// +Z = blue
									0,255,255,255,	0,255,255,255,	0,255,255,255,	0,255,255,255,	// -X = cyan
									255,0,255,255,	255,0,255,255,	255,0,255,255,	255,0,255,255,	// -Y = purple
									255,255,0,255,	255,255,0,255,	255,255,0,255,	255,255,0,255 };// -Z = yellow

	static GLfloat texs[24*2]={	0,1,	1,1,	1,0,	0,0,
								1,1,	1,0,	0,0,	0,1,
								0,1,	1,1,	1,0,	0,0,							
								1,1,	1,0,	0,0,	0,1,
								1,0,	0,0,	0,1,	1,1,
								1,0,	0,0,	0,1,	1,1 };

	static GLubyte elements[6*6] ={	0, 1, 2, 0, 2, 3,
									4, 5, 6, 4, 6, 7,
									8, 9,10, 8, 10,11,
									12,13,14,12,14,15,
									16,17,18,16,18,19,
									20,21,22,20,22,23 };
	
	Context *ctx = gl::context();
	GlslProgRef curShader = ctx->shaderGet();
	bool hasPositions = curShader->hasAttribSemantic( ATTRIB_POSITION );
	bool hasNormals = curShader->hasAttribSemantic( ATTRIB_NORMAL );
	bool hasTextureCoords = curShader->hasAttribSemantic( ATTRIB_TEX_COORD_0 );
	bool hasColors = curShader->hasAttribSemantic( ATTRIB_COLOR );
	
	size_t totalArrayBufferSize = 0;
	if( hasPositions )
		totalArrayBufferSize += sizeof(vertices);
	if( hasNormals )
		totalArrayBufferSize += sizeof(normals);
	if( hasTextureCoords )
		totalArrayBufferSize += sizeof(texs);
	if( hasColors )
		totalArrayBufferSize += sizeof(colors);
	
	VaoRef vao = Vao::create();
	VboRef arrayVbo = Vbo::create( GL_ARRAY_BUFFER, totalArrayBufferSize );
	VboRef elementVbo = Vbo::create( GL_ELEMENT_ARRAY_BUFFER, sizeof(elements) );

	VaoScope vaoScope( vao );
	elementVbo->bind();
	size_t curBufferOffset = 0;
	if( hasPositions ) {
		int loc = curShader->getAttribSemanticLocation( ATTRIB_POSITION );
		gl::bindBuffer( arrayVbo );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)curBufferOffset );
		arrayVbo->bufferSubData( curBufferOffset, sizeof(vertices), vertices );
		curBufferOffset += sizeof(vertices);
	}

	if( hasTextureCoords ) {
		int loc = curShader->getAttribSemanticLocation( ATTRIB_TEX_COORD_0 );
		gl::bindBuffer( arrayVbo );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)curBufferOffset );
		arrayVbo->bufferSubData( curBufferOffset, sizeof(texs), texs );
		curBufferOffset += sizeof(texs);
	}

	if( hasColors ) {
		int loc = curShader->getAttribSemanticLocation( ATTRIB_COLOR );
		gl::bindBuffer( arrayVbo );
		enableVertexAttribArray( loc );
		vertexAttribPointer( loc, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)curBufferOffset );
		arrayVbo->bufferSubData( curBufferOffset, sizeof(colors), colors );
		curBufferOffset += sizeof(colors);
	}
	
	elementVbo->bufferData( sizeof(elements), elements, GL_DYNAMIC_DRAW );

	//BufferScope arrayScope( arrayVbo );
	//BufferScope elementScope( elementVbo );
	arrayVbo->bind();
	elementVbo->bind();
	ctx->prepareDraw();
	gl::setDefaultShaderUniforms();
	gl::drawElements( GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0 );

	arrayVbo->unbind();
	elementVbo->unbind();
}

void draw( const TextureRef &texture, const Vec2f &offset )
{
	Context *ctx = context();
//	GlslProgRef shader = ctx->getStockShader( Shader().texture() );
}

void draw( const TextureRef &texture, const Rectf &rect )
{
	Context *ctx = context();
	GlslProgRef shader = ctx->getStockShader( ShaderDef().texture() );
	ScopeShader shaderScope( shader );
	
	texture->bind();
	shader->uniform( "uTex0", 0 );
	
	
	GLfloat data[8+8]; // both verts and texCoords
	GLfloat *verts = data, *texCoords = data + 8;
	
	verts[0*2+0] = rect.getX2(); texCoords[0*2+0] = texture->getRight();
	verts[0*2+1] = rect.getY1(); texCoords[0*2+1] = texture->getTop();
	verts[1*2+0] = rect.getX1(); texCoords[1*2+0] = texture->getLeft();
	verts[1*2+1] = rect.getY1(); texCoords[1*2+1] = texture->getTop();
	verts[2*2+0] = rect.getX2(); texCoords[2*2+0] = texture->getRight();
	verts[2*2+1] = rect.getY2(); texCoords[2*2+1] = texture->getBottom();
	verts[3*2+0] = rect.getX1(); texCoords[3*2+0] = texture->getLeft();
	verts[3*2+1] = rect.getY2(); texCoords[3*2+1] = texture->getBottom();
	
	VaoRef vao = Vao::create();
	VaoScope vaoScope( vao );
	VboRef arrayVbo = Vbo::create( GL_ARRAY_BUFFER, sizeof(data), data );
	arrayVbo->bind();

	int posLoc = shader->getAttribSemanticLocation( ATTRIB_POSITION );
	enableVertexAttribArray( posLoc );
	vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	int texLoc = shader->getAttribSemanticLocation( ATTRIB_TEX_COORD_0 );
	enableVertexAttribArray( texLoc );	
	vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*8) );
	
	gl::setDefaultShaderUniforms();
	gl::drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

GLenum getError()
{
	return glGetError();
}

std::string getErrorString( GLenum err )
{
	switch( err ) {
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		default:
			return "";
	}
}
	
SaveTextureBindState::SaveTextureBindState( GLint target )
: mTarget( target ), mOldID( -1 )
{
	switch( target ) {
		case GL_TEXTURE_2D:
			glGetIntegerv( GL_TEXTURE_BINDING_2D, &mOldID );
			break;
		default:
			throw gl::ExceptionUnknownTarget();
	}
}

SaveTextureBindState::~SaveTextureBindState()
{
	glBindTexture( mTarget, mOldID );
}

BoolState::BoolState( GLint target )
: mTarget( target )
{
	glGetBooleanv( target, &mOldValue );
}

BoolState::~BoolState()
{
	if ( mOldValue ) {
		gl::enable( mTarget );
	} else {
		gl::disable( mTarget );
	}
}

ClientBoolState::ClientBoolState( GLint target )
: mTarget( target )
{
	mOldValue = glIsEnabled( target );
}

SaveFramebufferBinding::SaveFramebufferBinding()
{
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &mOldValue );
}

SaveFramebufferBinding::~SaveFramebufferBinding()
{
	glBindFramebuffer( GL_FRAMEBUFFER, mOldValue );
}

} }
