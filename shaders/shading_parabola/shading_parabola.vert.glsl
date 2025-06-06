#version 330 core

// Inputs coming from VBOs
layout (location = 0) in vec3 vertex_position;

// Uniform variables expected to receive from the C++ program
uniform mat4 view; 
uniform mat4 projection;

uniform vec3 ball_position;	// initial position of the ball
uniform vec3 kickforce;		// initial speed of the ball
uniform float gravity;		// force of gravity (assumed >0)

#define maxt 7				// show the parabola corresponding to the first 7 seconds of the movement (including the part below the ground)

// assume the input vertex has coords (t, 0, 0) with 0 <= t <= 1
// return (x, y, z) corresponding to the position of the ball after a time t*maxt from the given starting point & speed

void main()
{
	float t = vertex_position.x * maxt;
	// z = p_0 + v_0 * t - 1/2 g t*t
	vec4 position = vec4(ball_position + kickforce * t + 0.5 * vec3(0, 0, -gravity) * t * t, 1.0);

	vec4 position_projected = projection * view * position;

	gl_Position = position_projected;
}
