#version 150

in vec4		vPosition;
in vec2		vTexCoord0;

uniform mat4	uModelViewProjection;

out highp vec2	TexCoord;

void main( void ) 
{
	TexCoord	= vTexCoord0;
	gl_Position	= uModelViewProjection * vPosition;
}
