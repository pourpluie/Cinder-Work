#version 150

uniform sampler2D uColorTex;
uniform vec4      uColorLeftEye;
uniform vec4      uColorRightEye;

in vec2 TexCoord;

out vec4 oColor;

void main()
{
	vec4 uv = TexCoord.xyxy * vec4(0.5, 1.0, 0.5, 1.0) + vec4(0.0, 0.0, 0.5, 0.0);
	oColor = uColorLeftEye * texture( uColorTex, uv.xy ) + uColorRightEye * texture( uColorTex, uv.zw );
}