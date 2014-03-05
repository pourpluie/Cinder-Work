#version 150

layout(points) in;
layout(line_strip, max_vertices = 64) out;

in vec3 vColor[]; // Output from vertex shader for each vertex
out vec3 fColor; // Output to fragment shader

uniform int sides;

const float PI = 3.1415926;

void main() {
	fColor = vColor[0];
	
	for (int i = 0; i <= sides; i++) {
        // Angle between each side in radians
        float ang = PI * 2.0 / sides * i;
		
        // Offset from center of point (0.3 to accomodate for aspect ratio)
        vec4 offset = vec4(cos(ang) * 0.55, -sin(ang) * 0.7, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;
		
        EmitVertex();
    }
	
	EndPrimitive();
	
}