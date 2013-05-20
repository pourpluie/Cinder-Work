#include "cinder/gl/Vao.h"

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
	if ( enabled ) {
		glEnableVertexAttribArray( mIndex );
	} else {
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
	glGenVertexArraysOES( 1, &mId );
}

Vao::~Vao()
{
	glDeleteVertexArraysOES( 1, &mId );
}

void Vao::addAttribute( const Vao::Attribute &attr )
{
	bind();
	mAttributes.push_back( attr );
	mAttributes.back().buffer();
	mAttributes.back().enable();
	unbind();
}
	
void Vao::bind() const
{
	glBindVertexArrayOES( mId );
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
	glBindVertexArrayOES( 0 );
}
	
} }
