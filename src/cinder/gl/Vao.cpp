#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"

namespace cinder { namespace gl {

using namespace std;
	
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
	if( glGenVertexArrays )
		glGenVertexArrays( 1, &mId );
	else
		glGenVertexArraysAPPLE( 1, &mId );
#endif
}

Vao::~Vao()
{
#if defined( CINDER_GLES )	
	glDeleteVertexArraysOES( 1, &mId );
#else
	if( glDeleteVertexArrays )
		glDeleteVertexArrays( 1, &mId );
	else
		glDeleteVertexArraysAPPLE( 1, &mId );
#endif
}

void Vao::bind()
{
	// this will "come back" by calling bindImpl if it's necessary
	context()->vaoBind( shared_from_this() );
}

void Vao::unbind() const
{
	// this will "come back" by calling bindImpl if it's necessary
	context()->vaoBind( nullptr );
}

void Vao::bindImpl( GLuint id, Context *context )
{
#if defined( CINDER_GLES )
	glBindVertexArrayOES( id );
#else
	if( glBindVertexArray ) // not available on GL legacy
		glBindVertexArray( id );
	else
		glBindVertexArrayAPPLE( id );
#endif

	if( context )
		invalidateContext( context );
}

void Vao::unbindImpl( Context *context )
{
	bindImpl( 0, context );
}

void Vao::invalidateContext( Context *context )
{
	// binding a VAO invalidates other pieces of cached state
	context->invalidateBufferBinding( GL_ARRAY_BUFFER );
	context->invalidateBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
}

	
} }
