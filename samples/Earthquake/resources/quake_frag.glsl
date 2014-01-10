#version 150

uniform vec3	uLightDir;

in vec3			Normal;

out vec4		oColor;

void main()
{
	vec3 ppNormal			= normalize( Normal );

	float ppDiffuse			= abs( dot( ppNormal, uLightDir ) );
	float ppFresnel			= pow( ( 1.0 - ppDiffuse ), 2.0 );
	float ppSpecular		= pow( ppDiffuse, 15.0 );
	float ppSpecularBright	= pow( ppDiffuse, 80.0 );

	oColor.rgb		= ( vec3( .25, 0, 0 ) * ppDiffuse + vec3( 1, 0, 0 ) * ppSpecular + ppSpecularBright );
	oColor.a		= 1.0;
}