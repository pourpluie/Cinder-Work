attribute vec4		vPosition;
attribute vec2		vTexCoord0;

uniform mat4		uModelViewProjection;

varying highp vec2	TexCoord;

void main( void )
{
	TexCoord	= vTexCoord0;
	gl_Position	= uModelViewProjection * vPosition;
}
