#version 150

uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

in vec4	ciPosition;
in vec3	ciNormal;
in vec2 ciTexCoord0;

out highp vec3 Normal;
out highp vec3 PositionObjSpace;
out highp vec2 TexCoord0;

void main()
{
	Normal = ciNormalMatrix * ciNormal;
	PositionObjSpace = ciPosition.xyz;
	TexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}
