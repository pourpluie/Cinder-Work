uniform mat4	ciModelViewProjectionMatrix;

attribute vec4		ciPosition;

varying highp vec3	NormalWorldSpace;

void main( void )
{
	NormalWorldSpace = vec3( ciPosition );
	gl_Position = ciModelViewProjectionMatrix * ciPosition;
}
