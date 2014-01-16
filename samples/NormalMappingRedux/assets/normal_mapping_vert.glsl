#version 150

uniform mat4	ciModelView;
uniform mat4	ciModelViewProjection;
uniform mat3	ciNormalMatrix;

in vec4			ciPosition;
in vec3			ciNormal;
in vec3			ciTangent;
in vec2			ciTexCoord0;

out vec4		vVertex;
out vec3		vNormal;
out vec3		vTangent;
out vec3		vBiTangent;
out vec2		vTexCoord0;

void main()
{	
	// calculate view space position
	vVertex = ciModelView * ciPosition; 

	// calculate view space normal, tangent and bitangent
	vNormal = normalize(ciNormalMatrix * ciNormal);
	vTangent = normalize(ciNormalMatrix * ciTangent); 
	vBiTangent = normalize(cross(vNormal, vTangent));

	// pass texture coordinates and screen space position
	vTexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}
