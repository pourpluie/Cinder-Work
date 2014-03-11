#version 150

in vec4 Position;
in vec3 Color;

uniform mat4 ciModelView;

out vec3 vColor;

void main()
{
    gl_Position = ciModelView * Position;
	vColor = Color;
}