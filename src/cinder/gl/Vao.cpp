#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

using namespace std;

Vao::Attribute::Attribute( GLuint index, GLint size, GLenum type, GLboolean normalized,
						  GLsizei stride, const GLvoid* offset )
: mIndex( index ), mNormalized( normalized ), mOffset( offset ), mSize( size ),
mStride( stride ), mType( type )
{
}
	
Vao::Attribute::~Attribute()
{
}

void Vao::Attribute::buffer()
{
	glVertexAttribPointer( mIndex, mSize, mType, mNormalized, mStride, mOffset );
}
	
void Vao::Attribute::enable( bool enabled )
{
	if( enabled ) {
		glEnableVertexAttribArray( mIndex );
	}
	else {
		glDisableVertexAttribArray( mIndex );
	}
}

GLuint Vao::Attribute::getIndex() const
{
	return mIndex;
}

GLboolean Vao::Attribute::getNormalized() const
{
	return mNormalized;
}

const GLvoid* Vao::Attribute::getOffset() const
{
	return mOffset;
}

GLint Vao::Attribute::getSize() const
{
	return mSize;
}

GLsizei Vao::Attribute::getStride() const
{
	return mStride;
}

GLenum Vao::Attribute::getType() const
{
	return mType;
}

void Vao::Attribute::setIndex( GLuint value )
{
	mIndex = value;
	buffer();
}

void Vao::Attribute::setNormalized( GLboolean value )
{
	mNormalized = value;
	buffer();
}

void Vao::Attribute::setOffset( const GLvoid* value )
{
	mOffset = value;
	buffer();
}

void Vao::Attribute::setSize( GLint value )
{
	mSize = value;
	buffer();
}

void Vao::Attribute::setStride( GLsizei value )
{
	mStride = value;
	buffer();
}
	
void Vao::Attribute::setType( GLenum value )
{
	mType = value;
	buffer();
}

/////////////////////////////////////////////////////////////////
	
VaoRef Vao::create()
{
	return VaoRef( new Vao() );
}

Vao::Vao()
{
	mId	= 0;
#if defined( CINDER_GLES )	
	glGenVertexArraysOES( 1, &mId );
#else
	glGenVertexArrays( 1, &mId );
#endif
}

Vao::~Vao()
{
#if defined( CINDER_GLES )	
	glDeleteVertexArraysOES( 1, &mId );
#else
	glDeleteVertexArrays( 1, &mId );
#endif
}

void Vao::vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	auto ctx = gl::context();
	VaoScope vaoBind( mId );

	ctx->vertexAttribPointer( index, size, type, normalized, stride, pointer );
	ctx->enableVertexAttribArray( index );
}

void Vao::enableVertexAttribArray( GLuint index )
{
	auto ctx = gl::context();
	VaoScope vaoBind( mId );
	ctx->enableVertexAttribArray( index );
}

void Vao::bindBuffer( const VboRef &vbo )
{
	auto ctx = gl::context();
	
	VaoScope vaoBind( mId );
	ctx->bufferBind( vbo->getTarget(), vbo->getId() );
	if( vbo->getTarget() == GL_ELEMENT_ARRAY_BUFFER )
		mBoundElementArrayBuffer = vbo->getId();
	else if( vbo->getTarget() == GL_ARRAY_BUFFER )
		mBoundArrayBuffer = vbo->getId();
}

void Vao::addAttribute( const Vao::Attribute &attr )
{
//	bind();
	VaoScope vaoBind( mId );
	mAttributes.push_back( attr );
	mAttributes.back().buffer();
	mAttributes.back().enable();
//	unbind();
}
	
void Vao::bind() const
{
	context()->vaoBind( mId );
}
	
vector<Vao::Attribute>& Vao::getAttributes()
{
	return mAttributes;
}

const vector<Vao::Attribute>& Vao::getAttributes() const
{
	return mAttributes;
}

void Vao::removeAttribute( GLuint index )
{
	for ( vector<Attribute>::iterator iter = mAttributes.begin(); iter != mAttributes.end(); ) {
		if ( iter->getIndex() == index ) {
			iter = mAttributes.erase( iter );
			return;
		} else {
			++iter;
		}
	}
}

void Vao::unbind() const
{
	context()->vaoBind( 0 );
}
	
} }
