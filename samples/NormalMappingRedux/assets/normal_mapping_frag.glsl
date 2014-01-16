#version 150

struct LightSource
{
	vec4 position;
	vec4 diffuse;
	vec4 specular;
};

in vec4				vVertex;
in vec3				vNormal;
in vec3				vTangent;
in vec3				vBiTangent;
in vec2				vTexCoord0;

uniform	sampler2D	uDiffuseMap;
uniform	sampler2D	uSpecularMap;
uniform	sampler2D	uNormalMap;
uniform	sampler2D	uEmmisiveMap;

uniform bool		bShowNormals;

uniform bool		bUseDiffuseMap;
uniform bool		bUseSpecularMap;
uniform bool		bUseNormalMap;
uniform bool		bUseEmmisiveMap;

uniform LightSource	uLights[2];

out vec4			oColor;

void main()
{
	// fetch the normal from the normal map and modify it using the normal (and tangents) from the 3D mesh
	vec3	vMappedNormal = texture2D(uNormalMap, vTexCoord0.st).rgb * 2.0 - 1.0;
	vec3	vSurfaceNormal = bUseNormalMap ? normalize((vTangent * vMappedNormal.x) + (vBiTangent * vMappedNormal.y) + (vNormal * vMappedNormal.z)) : vNormal;
 
	vec3	vToCamera = normalize(-vVertex.xyz); 

	// apply each of our light sources
	vec4	vDiffuseColor	= bUseEmmisiveMap ? texture2D(uEmmisiveMap, vTexCoord0.st) : vec4(0, 0, 0, 1);
	vec4	vSpecularColor	= vec4(0, 0, 0, 1);

	for(int i=0; i<2; ++i)
	{
		// calculate view space light vectors (for directional light source)
		vec3	vToLight = normalize(-uLights[i].position.xyz); 
		vec3	vReflect = normalize(-reflect(vToLight,vSurfaceNormal));

		// calculate diffuse term
		float	fDiffuse = max(dot(vSurfaceNormal,vToLight), 0.0);
		fDiffuse = clamp(fDiffuse, 0.1, 1.0);  

		// calculate specular term
		float	fSpecularPower = 100.0;
		float	fSpecular = pow( max( dot(vReflect, vToCamera), 0.0), fSpecularPower );
		fSpecular = clamp(fSpecular, 0.0, 1.0);

		// calculate final colors
		if(bUseDiffuseMap)
			vDiffuseColor += texture2D(uDiffuseMap, vTexCoord0.st) * uLights[i].diffuse * fDiffuse;
		else
			vDiffuseColor += uLights[i].diffuse * fDiffuse; 

		if(bUseSpecularMap)
			vSpecularColor += texture2D(uSpecularMap, vTexCoord0.st) * uLights[i].specular * fSpecular;
		else
			vSpecularColor += uLights[i].specular * fSpecular; 
	}

	// output colors to buffer
	oColor.rgb = bShowNormals ? vSurfaceNormal : (vDiffuseColor + vSpecularColor).rgb;
	oColor.a = 1.0;
}