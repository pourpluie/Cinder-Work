#version 150
 
// Attributes per vertex: position and texture coordinates
in vec4 vPosition;
in vec4 vTexCoord0;
 
uniform mat4   uModelViewProjection;
 
// Color to fragment program
out vec2 TexCoord0;
 
void main(void) 
{ 
    TexCoord0 = vTexCoord0.st;
    gl_Position = uModelViewProjection * vPosition;
}