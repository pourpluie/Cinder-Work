#version 150

in vec4 ciPosition;
in vec3 ciNormal;
in vec2 ciTexCoord0;
in vec4 ciColor;

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

out highp vec3 Position;
out highp vec3 Normal;
out highp vec2 TexCoord;
out lowp  vec4 Color;

void main()
{
	Position = vec3(ciModelView * ciPosition);
	Normal   = vec3(ciNormalMatrix * ciNormal);

	TexCoord = ciTexCoord0;
	Color    = ciColor;

	gl_Position = ciModelViewProjection * ciPosition;
}
