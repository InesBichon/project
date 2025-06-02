#version 330 core

// Vertex shader - this code is executed for every vertex of the shape

// Inputs coming from VBOs
layout (location = 0) in vec3 vertex_position; // vertex position in local space (x,y,z)

// Uniform variables expected to receive from the C++ program
uniform mat4 model; // Model affine transform matrix associated to the current shape
uniform mat4 view;  // View matrix (rigid transform) of the camera
uniform mat4 projection; // Projection (perspective or orthogonal) matrix of the camera

uniform vec3 ball_position;	// initial position of the ball
uniform vec3 kickforce;		// initial speed of the ball
uniform float gravity;		// force of gravity

#define maxt 10

// assume the input is (t, 0, 0) with 0 <= t <= 1
// returns the corresponding (x, y, z) corresponding to the position of the ball after a time t*maxt

void main()
{
	// The position of the vertex in the world space
	float t = vertex_position.x * maxt;
	vec4 position = vec4(ball_position + 0.5 * vec3(0, 0, -gravity) * t * t + kickforce * t, 1.0);

	// The projected position of the vertex in the normalized device coordinates:
	vec4 position_projected = projection * view * position;

	// gl_Position is a built-in variable which is the expected output of the vertex shader
	gl_Position = position_projected; // gl_Position is the projected vertex position (in normalized device coordinates)
}
