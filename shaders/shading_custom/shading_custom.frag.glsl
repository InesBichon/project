#version 330 core

#define MAX_LIGHTS 15

// Inputs coming from the vertex shader
in struct fragment_data
{
    vec3 position; // position in the world space
    vec3 normal;   // normal in the world space
    vec3 color;    // current color on the fragment
    vec2 uv;       // current uv-texture on the fragment

} fragment;

// Output of the fragment shader - output color
layout(location=0) out vec4 FragColor;

// View matrix
uniform mat4 view;

struct material_structure
{
	vec3 color;  // Uniform color of the object
};
uniform material_structure material;

// Ambiant uniform controled from the GUI
uniform float ambiant;
uniform float diffuse;
uniform float specular;
uniform float specular_exp;
uniform float dl_max;

uniform int light_n;

uniform vec3 light_colors[MAX_LIGHTS];
uniform vec3 light_positions[MAX_LIGHTS];


void main()
{
	mat3 O = transpose(mat3(view)); // get the orientation matrix
	vec3 last_col = vec3(view * vec4(0.0, 0.0, 0.0, 1.0)); // get the last column
	vec3 camera_position = -O * last_col;

	vec3 final_color = vec3(0.0f, 0.0f, 0.0f);

	vec3 n = normalize(fragment.normal);
	vec3 u_v = normalize(camera_position - fragment.position);
	
	for (int i = 0; i < light_n; i++)
	{
		vec3 u_l = normalize(light_positions[i] - fragment.position);
		vec3 u_r = reflect(-u_l, n);

		vec3 real_light_color = (1. - min(1., length(light_positions[i] - fragment.position) / dl_max)) * light_colors[i];
		// vec3 real_light_color = light_colors[i];
		
		vec3 ambiant_color = ambiant * material.color * real_light_color;
		vec3 diffuse_color = diffuse * max(0., dot(n, u_l)) * material.color * real_light_color;
		vec3 specular_color = specular * pow(max(0., dot(u_r, u_v)), specular_exp) * real_light_color;

		final_color += ambiant_color + diffuse_color + specular_color;
	}

	FragColor = vec4(final_color, 1.0);
	//FragColor = vec4(fog_coef * fog_color + (1-fog_coef) * final_color, 1.0); 	// Note: the last alpha component is not used here
}