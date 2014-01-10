uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

attribute vec4	ciPosition;
attribute vec3	ciNormal;
attribute vec2 ciTexCoord0;

varying highp vec3 Normal;
varying highp vec3 PositionObjSpace;
varying highp vec2 TexCoord0;

void main()
{
	Normal = ciNormalMatrix * ciNormal;
	PositionObjSpace = ciPosition.xyz;
	TexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}
