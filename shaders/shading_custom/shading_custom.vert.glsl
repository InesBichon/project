#version 330 core

// Inputs coming from VBOs
layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec3 vertex_color;
layout (location = 3) in vec2 vertex_uv;

// Output variables sent to the fragment shader
out struct fragment_data
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} fragment;

// Uniform variables expected to receive from the C++ program
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	// The position of the vertex in the world space
	vec4 position = model * vec4(vertex_position, 1.0);

	// The normal of the vertex in the world space
	mat4 modelNormal = transpose(inverse(model));
	vec4 normal = modelNormal * vec4(vertex_normal, 0.0);

	// The projected position of the vertex in the normalized device coordinates:
	vec4 position_projected = projection * view * position;

	// Fill the parameters sent to the fragment shader
	fragment.position = position.xyz;
	fragment.normal   = normal.xyz;
	fragment.color = vertex_color;
	fragment.uv = vertex_uv;

	gl_Position = position_projected; // gl_Position is the projected vertex position (in normalized device coordinates)
}
