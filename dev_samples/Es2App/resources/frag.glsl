uniform sampler2D		uTex0;
varying highp vec2		TexCoord;
varying mediump vec4	Color;

void main( void )
{
	gl_FragColor = texture2D( uTex0, TexCoord.st ) * Color;
}
