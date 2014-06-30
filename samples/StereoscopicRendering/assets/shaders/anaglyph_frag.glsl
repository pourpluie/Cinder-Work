#version 150

uniform sampler2D tex0;
uniform vec4      clr_left;
uniform vec4      clr_right;

in vec2 TexCoord;

out vec4 oColor;

void main()
{
	vec4 uv = TexCoord.xyxy * vec4(0.5, 1.0, 0.5, 1.0) + vec4(0.0, 0.0, 0.5, 0.0);
	oColor = clr_left * texture( tex0, uv.xy ) + clr_right * texture( tex0, uv.zw );
}