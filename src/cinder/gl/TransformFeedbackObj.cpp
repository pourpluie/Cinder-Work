
#include "cinder/gl/TransformFeedbackObj.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"

namespace cinder { namespace gl {

#if ! defined( CINDER_GLES )
	
extern TransformFeedbackObjRef createTransformFeedbackObjImplHardware();
extern TransformFeedbackObjRef createTransformFeedbackObjImplSoftware();

TransformFeedbackObjRef TransformFeedbackObj::create()
{
	if( ! glGenTransformFeedbacks ) {
		return createTransformFeedbackObjImplSoftware();
	}
	else {
		return createTransformFeedbackObjImplHardware();
	}
}

void TransformFeedbackObj::bind()
{
	gl::context()->bindTransformFeedbackObj( shared_from_this() );
}

void TransformFeedbackObj::unbind()
{
	gl::context()->bindTransformFeedbackObj( nullptr );
}

void TransformFeedbackObj::begin( GLenum primitiveMode )
{
	glBeginTransformFeedback( primitiveMode );
}

void TransformFeedbackObj::pause()
{
	glPauseTransformFeedback();
}

void TransformFeedbackObj::resume()
{
	glResumeTransformFeedback();
}

void TransformFeedbackObj::end()
{
	glEndTransformFeedback();
}

#endif
	
} } // gl // cinder
