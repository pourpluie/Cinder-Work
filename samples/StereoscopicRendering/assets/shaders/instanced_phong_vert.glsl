#version 150

in vec4 ciPosition;
in vec3 ciNormal;
in vec2 ciTexCoord0;
in vec4 ciColor;

in vec4 vInstancePosition;
in vec4 vInstanceColor;

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

out highp vec3 Position;
out highp vec3 Normal;
out highp vec2 TexCoord;
out lowp  vec4 Color;

void main()
{
	// rotate around the y-axis
	float sinr = sin( vInstancePosition.w );
	float cosr = cos( vInstancePosition.w );

	vec4 p = ciPosition;
	p.x = ciPosition.z * sinr + ciPosition.x * cosr;
	p.z = ciPosition.z * cosr - ciPosition.x * sinr;
	p += vec4( vInstancePosition.xyz, 1 );

	vec3 n = ciNormal;
	n.x = ciNormal.z * sinr + ciNormal.x * cosr;
	n.z = ciNormal.z * cosr - ciNormal.x * sinr;

	Position = vec3(ciModelView * p);
	Normal   = vec3(ciNormalMatrix * n);

	TexCoord = ciTexCoord0;
	Color    = vInstanceColor;

	gl_Position = ciModelViewProjection * p;
}
