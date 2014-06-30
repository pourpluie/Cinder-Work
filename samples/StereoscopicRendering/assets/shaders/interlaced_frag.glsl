#version 150

uniform sampler2D	uColorTex;
uniform vec4		uWindowMetrics; // xy = origin, zw = size

// find the correct half of the texture to sample (over-under FBO)
const vec2			halfOffset = vec2(1.0, 0.5);
const vec2			offset = vec2(0.0, 0.5);

in vec2 TexCoord;

out vec4 oColor;

void main()
{
	// find the actual screen coordinate of this fragment
	float y = uWindowMetrics.w - gl_FragCoord.y + uWindowMetrics.y;

	// does the fragment sit on an odd or even line?
	float isOdd = mod( floor(y), 2.0 );

	// sample the correct half of the texture
	oColor = texture( uColorTex, TexCoord * halfOffset + ( isOdd * offset ) );
}