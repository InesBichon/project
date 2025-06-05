
#include "terrain.hpp"


using namespace cgp;

// Evaluate 3D position of the terrain for any (x,y)
float Terrain::evaluate_terrain_height(float x, float y)
{
	float u = x / terrain_length + 0.5f, v = y / terrain_length + 0.5f;

	float z = 0.0f;

	for (int i = 0; i < n_col; i++)
	{
		float s = norm(vec2(x, y) - p_i[i]) / s_i[i];
		z += h_i[i] * std::exp(-s*s);
	}

	// Compute the Perlin noise
	// float noise = height * noise_perlin({u, v}, octave, persistency, frequency_gain);

	// add downwards walls
	double power = 4.;
	double height = 100;
	double maxz = 500;

	float min_car = min(cgp::numarray<float>{u, v, 1-u, 1-v});			// how close we are to a side

	z += 1 / (min_car + 0.01);

	// return z + noise;
	return z;
}

void Terrain::update_positions()
{
	int N_tex = 10;

	mesh.position.resize(N*N);
	// mesh.uv.resize(N*N);

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

void Terrain::create_terrain_mesh(int N, float terrain_length, int n_col)
{
	this->N = N;
	this->n_col = n_col;
	this->terrain_length = terrain_length;

	p_i.resize(n_col);
	h_i.resize(n_col);
	s_i.resize(n_col);

	for (int i = 0; i < n_col; i++)
	{
		p_i[i] = {(rand_uniform() - 0.5f) * terrain_length, (rand_uniform() - 0.5f) * terrain_length};
		h_i[i] = rand_uniform(-5.0f, 10.f);
		s_i[i] = rand_uniform(1.0f, 10.0f);
	}
	
	update_positions();
}

vec3 Terrain::get_normal_from_position(int N, float length, float x, float y)
{
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
		std::cout << "outside\n";
		return {0, 0, 1};
	}
	
	return mesh.normal[triangle_position];
}


std::vector<cgp::vec3> Terrain::generate_positions_on_terrain(int n_trees)
{
	std::vector<cgp::vec3> ans(n_trees);
	float min_dist = 1.5f;

	for (int i = 0; i < n_trees; i++)
	{
		float x, y, dist;
		do
		{
			dist = terrain_length;
			x = (rand_uniform() - 0.5f) * terrain_length;
			y = (rand_uniform() - 0.5f) * terrain_length;
			for (int j = 0; j < i; j++)
				dist = std::min(dist, norm(vec2(x, y) - vec2(ans[j].x, ans[j].y)));
		}
		while (dist < min_dist);
		
		ans[i] = {x, y, evaluate_terrain_height(x, y)};
	}

	return ans;
}