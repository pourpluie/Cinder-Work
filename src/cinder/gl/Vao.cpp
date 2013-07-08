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

void Vao::bind() const
{
	context()->vaoBind( mId );
}
	
void Vao::unbind() const
{
	context()->vaoBind( 0 );
}
	
} }
