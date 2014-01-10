#version 150

uniform mat3		ciNormalMatrix;
uniform mat4		ciModelViewProjection;

in vec4		ciPosition;
in vec3		ciNormal;

out vec3			Normal;

void main()
{
	Normal		= normalize( ciNormalMatrix * ciNormal );
	gl_Position = ciModelViewProjection * ciPosition;
}