uniform mat3		ciNormalMatrix;
uniform mat4		ciModelViewProjection;

attribute vec4		ciPosition;
attribute vec3		ciNormal;

varying highp vec3	Normal;

void main()
{
	Normal		= normalize( ciNormalMatrix * ciNormal );
	gl_Position = ciModelViewProjection * ciPosition;
}