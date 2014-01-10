precision highp float;

uniform sampler2D	uTexDiffuse;
uniform sampler2D	uTexNormal;
uniform sampler2D	uTexMask;
uniform vec3		uLightDir;

varying highp vec3 Normal;
varying highp vec2 TexCoord0;

void main()
{
	highp vec3 diffuseSample		= texture2D( uTexDiffuse, TexCoord0 ).rgb;
	highp vec3 normalSample		= ( texture2D( uTexNormal, TexCoord0 ).rgb - vec3( 0.5, 0.5, 0.5 ) ) * 2.0;
	highp vec3 texSample			= texture2D( uTexMask, TexCoord0 ).rgb;
	
	
	// use green channel for land elevation data
	highp float landValue			= texSample.g;

	// use blue channel for ocean elevation data
	highp float oceanValue		= texSample.b;

	
	highp vec3 ppNormal			= normalize( Normal + normalSample );
	highp float ppDiffuse			= abs( dot( ppNormal, uLightDir ) );
	highp float ppFresnel			= pow( ( 1.0 - ppDiffuse ), 3.0 );
	highp float ppSpecular		= pow( ppDiffuse, 10.0 );
	highp float ppSpecularBright	= pow( ppDiffuse, 120.0 );
	
	
	// use red channel for nighttime city lights
	highp float electrictyValue	= ( texSample.r ) * ( 1.0 - ppDiffuse );

	highp vec3 landFinal		= diffuseSample * landValue + ppSpecularBright * landValue;
	highp vec3 oceanFinal		= diffuseSample * ppSpecular * oceanValue + oceanValue * ppSpecularBright + oceanValue * ppFresnel * 15.0;
	
	float r		= ( 1.0 - ppNormal.r ) * oceanValue * 0.5;
	gl_FragColor.rgb	= landFinal + oceanFinal;// + vec3( r*r, r * 0.25, 0 ) * oceanValue;
	gl_FragColor.a		= 1.0;
}
