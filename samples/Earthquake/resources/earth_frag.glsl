#version 150

uniform sampler2D	uTexDiffuse;
uniform sampler2D	uTexNormal;
uniform sampler2D	uTexMask;
uniform vec3		uLightDir;

in vec3 Normal;
in vec2 TexCoord0;

out vec4 oColor;

void main()
{
	vec3 diffuseSample		= texture( uTexDiffuse, TexCoord0 ).rgb;
	vec3 normalSample		= ( texture( uTexNormal, TexCoord0 ).rgb - vec3( 0.5, 0.5, 0.5 ) ) * 2.0;
	vec3 texSample			= texture( uTexMask, TexCoord0 ).rgb;
	
	
	// use green channel for land elevation data
	float landValue			= texSample.g;

	// use blue channel for ocean elevation data
	float oceanValue		= texSample.b;

	
	vec3 ppNormal			= normalize( Normal + normalSample );
	float ppDiffuse			= abs( dot( ppNormal, uLightDir ) );
	float ppFresnel			= pow( ( 1.0 - ppDiffuse ), 3.0 );
	float ppSpecular		= pow( ppDiffuse, 10.0 );
	float ppSpecularBright	= pow( ppDiffuse, 120.0 );
	
	
	// use red channel for nighttime city lights
	float electrictyValue	= ( texSample.r ) * ( 1.0 - ppDiffuse );

	vec3 landFinal		= diffuseSample * landValue + ppSpecularBright * landValue;
	vec3 oceanFinal		= diffuseSample * ppSpecular * oceanValue + oceanValue * ppSpecularBright + oceanValue * ppFresnel * 15.0;
	
	float r		= ( 1.0 - ppNormal.r ) * oceanValue * 0.5;
	oColor.rgb	= landFinal + oceanFinal;// + vec3( r*r, r * 0.25, 0 ) * oceanValue;
	oColor.a	= 1.0;
}
