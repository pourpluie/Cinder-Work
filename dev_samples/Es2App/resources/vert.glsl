attribute vec4		aPosition;
attribute vec4		aNormal;
attribute vec4		aColor;
attribute vec4		aTexCoord;

uniform mat4		uModelViewProjection;

varying highp vec4	vColor;
varying highp vec4	vTexCoord;

void main( void ) 
{
	vColor		= aColor;
	vTexCoord	= aTexCoord;
	gl_Position	= uModelViewProjection * aPosition;
}
