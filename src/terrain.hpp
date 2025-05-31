#pragma once

#include "cgp/cgp.hpp"

using cgp::vec2;

struct Terrain
{
	int N, n_col;
	float terrain_length;

	std::vector<vec2> p_i;
	std::vector<float> h_i;
	std::vector<float> s_i;

	cgp::mesh mesh;
	
	int octave = 2;
	float height = 5;
	float persistency = 0.4f;
	float frequency_gain = 2.0f;

	float evaluate_terrain_height(float x, float y);

	/** Compute a terrain mesh 
	The (x,y) coordinates of the terrain are set in [-length/2, length/2].
	The z coordinates of the vertices are computed using evaluate_terrain_height(x,y).
	The vertices are sampled along a regular grid structure in (x,y) directions. 
	The total number of vertices is N*N (N along each direction x/y) 	*/
	void update_positions();
	void create_terrain_mesh(int N, float length, int n_col);

	std::vector<cgp::vec3> generate_positions_on_terrain(int);
};