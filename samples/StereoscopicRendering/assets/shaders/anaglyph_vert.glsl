#version 150

in vec4 ciPosition;
in vec2 ciTexCoord0;

uniform mat4 ciModelViewProjection;

out highp vec2 TexCoord;

void main()
{
	TexCoord = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}

