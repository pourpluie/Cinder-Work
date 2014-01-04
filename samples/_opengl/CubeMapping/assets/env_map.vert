#version 150

uniform mat4	ciModelViewProjection;
uniform mat3	ciNormalMatrix;
uniform mat4	ciProjection;

uniform mat4	uViewMatrix;
uniform mat4	uInverseViewMatrix;

in vec4		ciPosition;
in vec3		ciNormal;

out highp vec3	ReflectDir;

void main( void )
{
	ReflectDir = normalize( ciPosition.xyz );
	gl_Position = ciModelViewProjection * ciPosition;
}
