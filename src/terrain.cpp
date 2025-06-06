
#include "terrain.hpp"


using namespace cgp;

float Terrain::evaluate_terrain_height(float x, float y)
{
	// Evaluate z position of the terrain for any (x,y)
	
	float u = x / terrain_length + 0.5f, v = y / terrain_length + 0.5f;

	float z = 0.0f;

	for (int i = 0; i < n_bumps; i++)
	{
		float s = norm(vec2(x, y) - p_i[i]) / s_i[i];
		z += h_i[i] * std::exp(-s*s);
	}

	// add walls on the side

	float min_car = min(cgp::numarray<float>{u, v, 1-u, 1-v});		// minimal distance to a side

	z += 1 / (min_car + 0.01);			// very high near the side, low in the middle

	return z;
}

void Terrain::update_positions()
{
	// compute the positions and connectivity

	mesh.position.resize(N*N);

	// Fill terrain geometry
	for(int ku=0; ku<N; ++ku)
	{
		for(int kv=0; kv<N; ++kv)
		{
			// Compute local parametric coordinates (u,v) \in [0,1]
			float u = ku/(N-1.0f);
			float v = kv/(N-1.0f);

			// Compute the real coordinates (x,y) of the terrain in [-terrain_length/2, +terrain_length/2]
			float x = (u - 0.5f) * terrain_length;
			float y = (v - 0.5f) * terrain_length;

			// Compute the surface height function at the given sampled coordinate
			float z = evaluate_terrain_height(x,y);

			// Store vertex coordinates
			mesh.position[kv+N*ku] = {x,y,z};
			// mesh.uv[kv+N*ku] = {u * N_tex, v * N_tex};
		}
	}

	// Generate triangle organization
	//  Parametric surface with uniform grid sampling: generate 2 triangles for each grid cell
	for(int ku=0; ku<N-1; ++ku)
	{
		for(int kv=0; kv<N-1; ++kv)
		{
			unsigned int idx = kv + N*ku; // current vertex offset

			uint3 triangle_1 = {idx, idx+1+N, idx+1};
			uint3 triangle_2 = {idx, idx+N, idx+1+N};

			mesh.connectivity.push_back(triangle_1);
			mesh.connectivity.push_back(triangle_2);
		}
	}

	// need to call this function to fill the other buffer with default values (normal, color, etc)
	mesh.fill_empty_field(); 
}

void Terrain::create_terrain_mesh(int N, float terrain_length, int n_bumps)
{
	this->N = N;
	this->n_bumps = n_bumps;
	this->terrain_length = terrain_length;

	p_i.resize(n_bumps);
	h_i.resize(n_bumps);
	s_i.resize(n_bumps);

	for (int i = 0; i < n_bumps; i++)
	{
		p_i[i] = {(rand_uniform() - 0.5f) * terrain_length * 0.9, (rand_uniform() - 0.5f) * terrain_length * 0.9};
		h_i[i] = rand_uniform(3.0f, 10.f);
		s_i[i] = rand_uniform(3.0f, 15.0f);
	}
	
	update_positions();
}

vec3 Terrain::get_normal_from_position(int N, float length, float x, float y)
{
	// compute the normal vector
	int triangle_position;
	float u0 = (x / length + 0.5) * (N-1);
	float v0 = (y / length + 0.5) * (N-1);
	int ku0 = round(u0);
	int kv0 = round(v0);
	float ru0 = u0 - ku0;
	float rv0 = v0 - kv0;

	triangle_position = kv0+N*ku0;
	if (ru0 + rv0 >= 1)
	{
		triangle_position += 1;
	}

	// if we're outside the terrain, return a vertical normal
	if (triangle_position < 0 || triangle_position >= mesh.normal.size())
	{
		// std::cout << "outside\n";
		return {0, 0, 1};
	}
	
	return mesh.normal[triangle_position];
}