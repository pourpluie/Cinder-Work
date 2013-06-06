#version 150

uniform sampler2D uTex0;
in vec2	TexCoord;
in vec3	Color;

out vec4 oColor;

void main( void )
{
//	oColor = vec4( 1.0, 0.5, 0.25, 1.0 );
	oColor.rgb = texture( uTex0, TexCoord.st ).rgb * Color;
	oColor.a = 1.0;
}
