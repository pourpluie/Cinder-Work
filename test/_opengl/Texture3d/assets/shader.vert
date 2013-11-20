#version 150

uniform mat4	uModelViewProjection;

in vec4		vPosition;

void main( void )
{
	gl_Position	= uModelViewProjection * vPosition;
}
