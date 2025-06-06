#version 330 core

layout(location=0) out vec4 FragColor;

// Color of the segments, controlled from the C++ code
uniform vec3 segment_color;

void main()
{
	// we do not wish to compute lighting, the parabola color should be plain (here, red)
	FragColor = vec4(segment_color, 1.0);
}