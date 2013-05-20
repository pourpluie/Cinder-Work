varying highp vec4	vColor;
varying highp vec4	vTexCoord;

uniform bool		uTexEnabled;
uniform sampler2D	uTexture;

void main( void )
{
	highp vec4 color = vColor;
	if ( uTexEnabled ) {
		color *= texture2D( uTexture, vTexCoord.st );
	}
	gl_FragColor = color;
}
