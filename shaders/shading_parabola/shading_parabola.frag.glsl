#version 330 core

// Output of the fragment shader - output color
layout(location=0) out vec4 FragColor;

// Color of the segments, controlled from the GUI
uniform vec3 segment_color;


void main()
{
	FragColor = vec4(segment_color, 1.0);
}