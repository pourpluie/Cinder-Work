precision highp float;

uniform vec3	uLightDir;

varying highp vec3			Normal;

void main()
{
	highp vec3 ppNormal			= normalize( Normal );

	float ppDiffuse			= abs( dot( ppNormal, uLightDir ) );
	float ppFresnel			= pow( ( 1.0 - ppDiffuse ), 2.0 );
	float ppSpecular		= pow( ppDiffuse, 15.0 );
	float ppSpecularBright	= pow( ppDiffuse, 80.0 );

	gl_FragColor.rgb		= ( vec3( .25, 0, 0 ) * ppDiffuse + vec3( 1, 0, 0 ) * ppSpecular + ppSpecularBright );
	gl_FragColor.a		= 1.0;
}