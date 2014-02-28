#version 150

uniform mat4	ciModelViewProjectionMatrix;

in vec4			ciPosition;

out highp vec3	NormalWorldSpace;

void main( void )
{
	NormalWorldSpace = vec3( ciPosition );
	gl_Position = ciModelViewProjectionMatrix * ciPosition;
}
