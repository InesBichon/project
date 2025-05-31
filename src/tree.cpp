#include "tree.hpp"

using namespace cgp;


mesh create_cylinder_mesh(float radius, float height)
{
	mesh m; 
	// To do: fill this mesh ...
	// ...
	// To add a position: 
	//   m.position.push_back(vec3{x,y,z})
	// Or in pre-allocating the buffer:
	//   m.position.resize(maximalSize);
	//   m.position[index] = vec3{x,y,z}; (with 0<= index < maximalSize)
	// 
	// Similar with the triangle connectivity:
	//  m.connectivity.push_back(uint3{index_1, index_2, index_3});

	const int n = 10;
	m.position.resize(2 * n);
	// 0 -> n-1: sommets du bas
	// n -> 2n-1 : sommets du haut

	for (int i = 0; i < n; i++)
	{
		m.position[i] = m.position[i + n] = {radius * cos(Pi * 2.0f * i / n), radius * sin(Pi * 2.0f * i / n), 0.0f};
		m.position[i + n].z = height;
	}

	for (int i = 0; i < n; i++)
	{
		m.connectivity.push_back(uint3({i, (i+1) % n, i + n}));
		m.connectivity.push_back(uint3({(i+1) % n, (i+1) % n + n, i + n}));
	}
	// Need to call fill_empty_field() before returning the mesh 
	//  this function fill all empty buffer with default values (ex. normals, colors, etc).
	m.fill_empty_field();

	return m;
}

mesh create_cone_mesh(float radius, float height, float z_offset)
{
	mesh m;

	const int n = 10;
	m.position.resize(n+2);
	// 0 -> n-1: sommets du bas
	// n: centre de la base
	// n+1: pointe

	for (int i = 0; i < n; i++)
		m.position[i] = {radius * cos(Pi * 2.0f * i / n), radius * sin(Pi * 2.0f * i / n), z_offset};
	m.position[n] = {0.0f, 0.0f, z_offset};
	m.position[n+1] = {0.0f, 0.0f, z_offset + height};

	// base & cône
	for (int i = 0; i < n; i++)
	{
		m.connectivity.push_back(uint3({i, (i+1) % n, n}));
		m.connectivity.push_back(uint3({i, (i+1) % n, n+1}));
	}

	m.fill_empty_field();
	return m;
}


mesh create_tree()
{
	float h = 0.7f; // trunk height
	float r = 0.1f; // trunk radius

	// Create a brown trunk
	mesh trunk = create_cylinder_mesh(r, h);
	trunk.color.fill({0.4f, 0.3f, 0.3f});


	// Create a green foliage from 3 cones
	mesh foliage = create_cone_mesh(4*r, 6*r, 0.0f);      // base-cone
	foliage.push_back(create_cone_mesh(4*r, 6*r, 2*r));   // middle-cone
	foliage.push_back(create_cone_mesh(4*r, 6*r, 4*r));   // top-cone
	foliage.translate({0,0,h});       // place foliage at the top of the trunk
	foliage.color.fill({0.4f, 0.6f, 0.3f});
	   
	// The tree is composed of the trunk and the foliage
	mesh tree = trunk;
	tree.push_back(foliage);

	return tree;
}