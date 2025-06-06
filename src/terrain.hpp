#pragma once

#include "cgp/cgp.hpp"

using cgp::vec2;

struct Terrain
{
	int N, n_bumps;
	float terrain_length;

	std::vector<vec2> p_i;			// positions of the bumps
	std::vector<float> h_i;			// heights of the bumps
	std::vector<float> s_i;			// width of the bumps

	cgp::mesh mesh;

	float evaluate_terrain_height(float x, float y);

	/** Compute a terrain mesh 
	The (x,y) coordinates of the terrain are set in [-length/2, length/2].
	The z coordinates of the vertices are computed using evaluate_terrain_height(x,y).
	The vertices are sampled along a regular grid structure in (x,y) directions. 
	The total number of vertices is N*N (N along each direction x/y) 	*/

	void update_positions();
	void create_terrain_mesh(int N, float length, int n_bumps);
	cgp::vec3 get_normal_from_position(int N, float length, float x, float y);
};