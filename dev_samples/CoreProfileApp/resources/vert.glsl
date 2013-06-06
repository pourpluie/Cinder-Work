#version 150

in vec4		vPosition;
in vec2		vTexCoord0;
in vec3		vColor;

uniform mat4	uModelViewProjection;

out highp vec2		TexCoord;
out mediump vec3	Color;

void main( void ) 
{
	TexCoord	= vTexCoord0;
	gl_Position	= uModelViewProjection * vPosition;
	Color = vColor;
}
