#version 150

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;

out vec4 oColor;

void main()
{
	const vec4	ambient = vec4(0.1, 0.1, 0.1, 1);
	const vec4	specular = vec4(0.7, 0.7, 0.4, 1);
	const float shinyness = 50.0;

	vec3 N = normalize(Normal);
	vec3 Light   = normalize(-Position);
	vec3 Eye     = normalize(-Position);
	vec3 Reflect = normalize(-reflect(Light,N));

	// ambient term
	vec4 Ambient = ambient;

	// diffuse term
	vec4 Diffuse = Color;
	Diffuse *= max(dot(N,Light), 0.0);

	// specular term
	vec4 Specular = specular;
	Specular *= pow(max(dot(Reflect,Eye),0.0), shinyness);

	// reflection term (fake ground reflection)
	float r = sqrt(max(dot(N,vec3(0,-1,0)), 0.0)) * 0.5;
	vec4  Reflection = vec4(r, r, r, 1.0) * specular;

	// final color
	oColor = Ambient + Diffuse + Reflection + Specular;
}