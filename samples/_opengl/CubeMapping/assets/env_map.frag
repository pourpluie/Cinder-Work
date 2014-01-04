#version 150

uniform samplerCube uCubeMapTex;

in vec3	ReflectDir;

out vec4 	oColor;

void main( void )
{
	oColor = texture( uCubeMapTex, normalize(ReflectDir) );
}