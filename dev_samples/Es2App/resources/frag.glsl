uniform sampler2D	uTex0;
varying highp vec2	TexCoord;

void main( void )
{
	gl_FragColor = vec4( 1.0, 0.5, 0.25, 1.0 );
//	gl_FragColor = texture2D( uTex0, TexCoord.st );
}
