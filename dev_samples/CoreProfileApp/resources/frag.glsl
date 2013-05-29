#version 150

uniform sampler2D tex0;
in vec2	TexCoord;

out vec4 oColor;

void main( void )
{
//	oColor = vec4( 1.0, 0.5, 0.25, 1.0 );
	oColor = texture( tex0, TexCoord.st );
}
