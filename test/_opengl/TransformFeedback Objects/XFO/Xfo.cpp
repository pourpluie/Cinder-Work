//
//  XFO.cpp
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/16/13.
//
//

#include "Xfo.h"
#include "cinder/gl/Vbo.h"
#include "TestContext.h"
#include "cinder/gl/Environment.h"

using std::cout;
using std::endl;

namespace cinder { namespace gl {
	
// defined in VaoImplEs
#if defined( CINDER_GLES )
extern XfoRef createXfoImplEs();
#else
extern XfoRef createXfoImplHardware();
#endif
extern XfoRef createXfoImplSoftware();

XfoRef Xfo::create( bool software )
{
	if( !glBindTransformFeedback && software ) {
		cout << "in baseXfo making software" << endl;
		return createXfoImplSoftware();
	}
	else {
		cout << "in baseXfo making hardware" << endl;
		return createXfoImplHardware();
	}
}
	
void Xfo::bind()
{
	gl::TestContext::testContext()->xfoBind( shared_from_this() );
}
	
void Xfo::unbind()
{
	gl::TestContext::testContext()->xfoBind( nullptr );
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

} } // gl // cinder