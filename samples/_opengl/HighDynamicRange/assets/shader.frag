#version 150
 
out vec4 oColor;
 
uniform sampler2D 	uTex0;
uniform float 		uExposure;
 
in vec2 			TexCoord0;
 
void main(void)
{ 
	vec4 color = texture( uTex0, TexCoord0 );
 
	oColor.rgb = pow( color.rgb * uExposure, vec3(1.0 / 2.2) );//1.0 - exp2( -color * uExposure );
	oColor.a = 1.0;
}