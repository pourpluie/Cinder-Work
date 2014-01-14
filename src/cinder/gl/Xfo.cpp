
#include "Xfo.h"
#include "cinder/gl/Vbo.h"
#include "Context.h"
#include "cinder/gl/Environment.h"

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
extern XfoRef createXfoImplHardware();
extern XfoRef createXfoImplSoftware();

XfoRef Xfo::create()
{
	if( ! glGenTransformFeedbacks ) {
		return createXfoImplSoftware();
	}
	else {
		return createXfoImplHardware();
	}
}

void Xfo::bind()
{
	gl::context()->xfoBind( shared_from_this() );
}

void Xfo::unbind()
{
	gl::context()->xfoBind( nullptr );
}

void Xfo::begin( GLenum primitiveMode )
{
	glBeginTransformFeedback( primitiveMode );
}

void Xfo::pause()
{
	glPauseTransformFeedback();
}

void Xfo::resume()
{
	glResumeTransformFeedback();
}

void Xfo::end()
{
	glEndTransformFeedback();
}

#endif
	
} } // gl // cinder
