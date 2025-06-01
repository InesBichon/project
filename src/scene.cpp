#include "scene.hpp"
#include "terrain.hpp"

using namespace cgp;

cgp::vec3 get_random_color()
{
	return {rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0)};
}

void scene_structure::initialize()
{
	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.set_rotation_axis_z();
	camera_control.look_at({ 15.0f,6.0f,6.0f }, {0,0,0});
	

	// General information
	display_info();

	global_frame.initialize_data_on_gpu(mesh_primitive_frame());
	
	shader_custom.load(
		project::path + "shaders/shading_custom/shading_custom.vert.glsl",
		project::path + "shaders/shading_custom/shading_custom.frag.glsl");

	int N_terrain_samples = 500, n_col = 50;
	float terrain_length = 50;

	terrain.create_terrain_mesh(N_terrain_samples, terrain_length, n_col);

	terrain_mesh.initialize_data_on_gpu(terrain.mesh);
	// terrain_mesh.material.color = {0.1f,0.1f,0.1f};
	// terrain_mesh.material.phong.specular = 0.0f; // non-specular terrain material
	terrain_mesh.shader = shader_custom;
	terrain_mesh.material.color = {1, 1, 1};

	spheres.resize(n_lights);
	light_colors.resize(n_lights);

	for (int i = 0; i < n_lights; i++)
	{
		light_colors[i] = {i % 3 == 0, i % 3 == 1, i % 3 == 2};
		// std::cout << light_colors[i] << '\n';
		mesh sphere_mesh = mesh_primitive_sphere();
		spheres[i].initialize_data_on_gpu(sphere_mesh);
		spheres[i].model.scaling = 0.2f; // coordinates are multiplied by 0.2 in the shader
		// spheres[i].texture = 
		spheres[i].material.color = light_colors[i];
		// spheres[i].shader = shader_custom;
	}
	
	// Initial position and speed of ball
	// ******************************************* //


	mesh ball_mesh = mesh_primitive_sphere();
	ball.initialize_data_on_gpu(ball_mesh);
	ball.model.scaling = 0.2;
	ball.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/marbre.jpg");

	


	cgp_warning::max_warning = 0;
	
	// skybox code from the cgp examples github

	image_structure image_skybox_template = image_load_file(project::path+"assets/skybox2.png");
	std::vector<image_structure> image_grid = image_split_grid(image_skybox_template, 4, 3);

	skybox.initialize_data_on_gpu();
	skybox.texture.initialize_cubemap_on_gpu(image_grid[1], image_grid[7], image_grid[5], image_grid[3], image_grid[10], image_grid[4]);
}



void scene_structure::simulation_step(float dt)
{

	float m = 0.01f;       // ball mass
	vec3 g = { 0,0,-9.81f }; // gravity


	ball_weight = g;

	ball_velocity = ball_velocity + dt * ball_acceleration / m;
	ball_position = ball_position + dt * ball_velocity;
	


}

void scene_structure::display_frame()
{
	// Update time
	timer.update();

	float freq = 0.5f;

	// draw the skybox before everything else
	glDepthMask(GL_FALSE);
	draw(skybox, environment);
	glDepthMask(GL_TRUE);

	// Set the light to the current position of the camera
	// environment.light = camera_control.camera_model.position();
	glUseProgram(shader_custom.id);

	environment.uniform_generic.uniform_float["ambiant"] = 0.0f / n_lights;
	environment.uniform_generic.uniform_float["diffuse"] = 5.f / n_lights;
	environment.uniform_generic.uniform_float["specular"] = 35.f / n_lights;
	environment.uniform_generic.uniform_float["specular_exp"] = 100;
	environment.uniform_generic.uniform_float["dl_max"] = 100;

	environment.uniform_generic.uniform_int["light_n"] = n_lights;

	GLint pos_loc = shader_custom.query_uniform_location("light_positions");
	GLint col_loc = shader_custom.query_uniform_location("light_colors");
	
	for (int i = 0; i < n_lights; i++)
	{
		cgp::vec3 color = light_colors[i];
		cgp::vec3 pos = {10 * cos(10. * i), 10 * sin(10. * i), 0};
		pos.z = terrain.evaluate_terrain_height(pos.x, pos.y) + 3;
		pos.x += 2 * cos(freq * timer.t * (i % 2 + 1));
		pos.y += 2 * sin(freq * timer.t * (i % 2 + 1));

		glUniform3f(pos_loc + i, pos.x, pos.y, pos.z);
		glUniform3f(col_loc + i, color.x, color.y, color.z);

		spheres[i].model.translation = pos;
		// spheres[i].material.color = color * 0.8f;

		// spheres[i].material.phong.ambient = 1;
		// spheres[i].material.phong.diffuse = 0;
		// spheres[i].material.phong.specular = 0;
	}

	// environment.background_color = gui;

	// if (gui.display_frame)
	// 	draw(global_frame, environment);

	draw(terrain_mesh, environment);
		
	for (mesh_drawable& sphere: spheres)
		draw(sphere, environment);
	
	// draw(tree, environment, N_trees);
	draw(ball, environment);

	if (gui.display_wireframe)
	{
		draw_wireframe(terrain_mesh, environment);
		
		// for (vec3 pos: tree_position)
		// {
		// 	tree.model.translation = pos;
		// 	tree.model.translation.z -= 0.1f;
		// 	draw_wireframe(tree, environment);
		// }
	}

}


void scene_structure::display_gui()
{
	ImGui::Checkbox("Frame", &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.display_wireframe);

	bool update = false;
	update |= ImGui::SliderFloat("Persistance", &terrain.persistency, 0.1f, 0.6f);
	update |= ImGui::SliderFloat("Frequency gain", &terrain.frequency_gain, 1.5f, 2.5f);
	update |= ImGui::SliderInt("Octave", &terrain.octave, 1, 8);
	update |= ImGui::SliderFloat("Height", &terrain.height, 0.f, 10.f);

	if (update)// if any slider has been changed - then update the terrain
	{
		terrain.update_positions();	// Update step: Allows to update a mesh_drawable without creating a new one
		terrain_mesh.vbo_position.update(terrain.mesh.position);
		terrain_mesh.vbo_normal.update(terrain.mesh.normal);
		terrain_mesh.vbo_color.update(terrain.mesh.color);
		
		// for (vec3& pos: tree_position)
		// 	pos.z = terrain.evaluate_terrain_height(pos.x, pos.y);
		// tree.supplementary_vbo[0].clear();
		// tree.initialize_supplementary_data_on_gpu(cgp::numarray<vec3>(tree_position), 4, 1);
	}
}

void scene_structure::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move(environment.camera_view);
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}
void scene_structure::keyboard_event()
{
	camera_control.action_keyboard(environment.camera_view);
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}

void scene_structure::display_info()
{
	std::cout << "\nCAMERA CONTROL:" << std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
	std::cout << camera_control.doc_usage() << std::endl;
	std::cout << "-----------------------------------------------\n" << std::endl;
}
