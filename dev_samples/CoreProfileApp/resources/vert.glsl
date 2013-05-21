#version 150

in vec4		aPosition;
in vec4		aTexCoord;

uniform mat4	uModelViewProjection;

out highp vec4	vTexCoord;

void main( void ) 
{
	vTexCoord	= aTexCoord;
	gl_Position	= uModelViewProjection * aPosition;
}
