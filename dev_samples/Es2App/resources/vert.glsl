attribute vec4		vPosition;
attribute vec2		vTexCoord0;
attribute vec3		vColor;

uniform mat4		uModelViewProjection;

varying highp vec2		TexCoord;
varying mediump vec4	Color;

void main( void )
{
	TexCoord	= vTexCoord0;
	Color		= vec4( vColor, 1 );
	gl_Position	= uModelViewProjection * vPosition;
}
