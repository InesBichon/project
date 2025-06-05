#include "scene.hpp"
#include "terrain.hpp"

using namespace cgp;

cgp::vec3 get_random_color()
{
	return {rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0)};
}

cgp::vec3 scene_structure::reflect(cgp::vec3 v, cgp::vec3 n)
{
	cgp::vec3 new_v;
	new_v = 2 * n * dot(n, v) - v;
	return -new_v;

}

void scene_structure::initialize()
{
	// General information
	display_info();

	global_frame.initialize_data_on_gpu(mesh_primitive_frame());
	
	shader_custom.load(
		project::path + "shaders/shading_custom/shading_custom.vert.glsl",
		project::path + "shaders/shading_custom/shading_custom.frag.glsl");

	shader_parabola.load(
		project::path + "shaders/shading_parabola/shading_parabola.vert.glsl",
		project::path + "shaders/shading_parabola/shading_parabola.frag.glsl"
	);

	int N_terrain_samples = 100, n_col = 50;
	float terrain_length = 200;

	terrain.create_terrain_mesh(N_terrain_samples, terrain_length, n_col);

	terrain_mesh.initialize_data_on_gpu(terrain.mesh);
	// terrain_mesh.material.color = {0.1f,0.1f,0.1f};
	// terrain_mesh.material.phong.specular = 0.0f; // non-specular terrain material
	terrain_mesh.shader = shader_custom;
	terrain_mesh.material.color = {1, 1, 1};
	
	ball_position = {10.f, 10.f, 10.f};
	ball_velocity = {0.f, 0.f, 0.f};

	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.translation_speed *= 10;
	vec3 cam_pos = {-10.0f, 0.0f, 0.0f};
	cam_pos.z = terrain.evaluate_terrain_height(cam_pos.x, cam_pos.y) + 2.;
	camera_control.look_at(cam_pos, ball_position, {0,0,1});
	// camera_control.set_rotation_axis_z();
	// camera_control.look_at({ 15.0f,6.0f,6.0f }, {0,0,0});

	spheres.resize(n_lights+2);
	light_colors.resize(n_lights+2);

	for (int i = 0; i < n_lights; i++)
	{
		// light_colors[i] = {i % 3 == 0, i % 3 == 1, i % 3 == 2};			// alternating red/green/blue lights
		light_colors[i] = get_random_color();
		// std::cout << light_colors[i] << '\n';
		mesh sphere_mesh = mesh_primitive_sphere();
		spheres[i].initialize_data_on_gpu(sphere_mesh);
		spheres[i].model.scaling = 0.2f; // coordinates are multiplied by 0.2 in the shader
		// spheres[i].texture = 
		spheres[i].material.color = light_colors[i];
		// spheres[i].shader = shader_custom;
	}

	// Initial position of the ball light and of the target light
	// Light of the ball
	light_colors[n_lights] = {1.0f, 0, 0};
	mesh sphere_mesh1 = mesh_primitive_sphere();
	spheres[n_lights].initialize_data_on_gpu(sphere_mesh1);
	spheres[n_lights].model.scaling = 0.2f; // coordinates are multiplied by 0.2 in the shader
	spheres[n_lights].material.color = light_colors[n_lights];

	
	light_colors[n_lights+1] = {1.0f, 1.0f, 1.0f};
	mesh sphere_mesh2 = mesh_primitive_sphere();
	spheres[n_lights+1].initialize_data_on_gpu(sphere_mesh2);
	spheres[n_lights+1].model.scaling = 0.2f; // coordinates are multiplied by 0.2 in the shader
	spheres[n_lights+1].material.color = light_colors[n_lights+1];

	// Initial position and speed of ball and 
	// ******************************************* //

	mesh ball_mesh = mesh_primitive_sphere();
	ball.initialize_data_on_gpu(ball_mesh);
	ball.model.scaling = ball_radius;
	// ball.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/tex.jpeg");
	ball.material.color = {1,0,0};

	mesh torus_mesh = mesh_primitive_torus(torus_max_radius, torus_min_radius);
	target.initialize_data_on_gpu(torus_mesh);
	target.material.color = {9., 0., 0.};
	
	// target.model.rotation = 

	mesh force_arrow_mesh = mesh_primitive_arrow();
	force_arrow.initialize_data_on_gpu(force_arrow_mesh);
	force_arrow.material.color = {0.8, 0.8, 0.8};
	force_arrow.material.phong.ambient = 1;
	force_arrow.material.phong.diffuse = 0;
	force_arrow.material.phong.specular = 0;

	ball_position = {0, 0, 10};
	reset_force();

	// skybox code from the cgp examples github

	image_structure image_skybox_template = image_load_file(project::path+"assets/skybox4.jpg");
	std::vector<image_structure> image_grid = image_split_grid(image_skybox_template, 4, 3);

	skybox.initialize_data_on_gpu();
	skybox.texture.initialize_cubemap_on_gpu(image_grid[1], image_grid[7], image_grid[5], image_grid[3], image_grid[10], image_grid[4]);

	// initialize the curve for the parabola: it's actually a chain of N segments going from (0,0,0) to (1,0,0) in a straight line
	// then, the actual parabola will be computed in the vertex shader to avoid copying data to the GPU each frame

	std::vector<vec3> positions(N_parabola, {0., 0., 0.});
	for (int i = 0; i < N_parabola; i++)
		positions[i].x = (float)(i) / (N_parabola - 1);

	segments.display_type = curve_drawable_display_type::Curve;
	segments.shader = shader_parabola;
	segments.initialize_data_on_gpu(positions, shader_parabola);

	phase = 0;

	cgp_warning::max_warning = 0;
}

void scene_structure::simulation_step(float dt)
{
	if (phase > 0)
		return;

	float m = 0.01f;       // ball mass
	vec3 g = { 0,0,-gravity}; // gravity


	ball_weight = m * g;
	ball_force = ball_weight;

	ball_velocity = ball_velocity + dt * ball_force / m;
	ball_position = ball_position + dt * ball_velocity;
	
	vec3 normal = terrain.get_normal_from_position(terrain.N, terrain.terrain_length, ball_position.x, ball_position.y);

	if (ball_position.z - ball_radius <= terrain.evaluate_terrain_height(ball_position.x, ball_position.y) && dot(ball_velocity, normal) < 0)
	{
		// std::cout << "rebond\n\tinitial velocity " << ball_velocity << "\n\tnormal " << normal << "\n\t";
		ball_velocity = 0.8 * reflect(ball_velocity, normal);
		ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + ball_radius;
		// std::cout << "velocity " << ball_velocity << "\n\tnorm " << cgp::norm(ball_velocity) << "\n";

		if (normal.z < 0.995 && cgp::norm(ball_velocity) < 3)		// we want the ball to slide down slopes reasonably fast, but not gain too much speed
			ball_velocity = 1.3 * ball_velocity;
	}

}

void scene_structure::display_frame()
{
	// Update time
	timer.update();

	if (last_frame_time == -1.0f)			// avoid a massive interval during the first frame
		last_frame_time = timer.t;

	float interval = timer.t - last_frame_time;
	last_frame_time = timer.t;
	
	// move the camera
	// move_cam(interval);

	float freq = 0.5f;

	// draw the skybox before everything else
	glDepthMask(GL_FALSE);
	draw(skybox, environment);
	glDepthMask(GL_TRUE);



	ball.model.translation = ball_position;
	// environment.background_color = gui;

	// if (gui.display_frame)
	// 	draw(global_frame, environment);

	draw(terrain_mesh, environment);

	
	simulation_step(timer.scale * 0.1f);
	// draw(tree, environment, N_trees);

	ball.model.translation = ball_position;
	draw(ball, environment);
	float torus_x = -40.0f;
	float torus_y = 40.0f;
	vec3 target_position = {torus_x, torus_y, torus_max_radius + terrain.evaluate_terrain_height(torus_x, torus_y)};
	rotation_transform R = rotation_transform::from_axis_angle({ 1,0,0 }, Pi / 2);
	target.model.rotation = R;
	target.model.translation = target_position;
	

	draw(target, environment);


		// Set the light to the current position of the camera
	// environment.light = camera_control.camera_model.position();
	// the first n_lights are regular lights, the last 2 follow the ball and the target
	glUseProgram(shader_custom.id);

	environment.uniform_generic.uniform_float["ambiant"] = 1.0f / n_lights;
	environment.uniform_generic.uniform_float["diffuse"] = 5.f / n_lights;
	environment.uniform_generic.uniform_float["specular"] = 35.f / n_lights;
	environment.uniform_generic.uniform_float["specular_exp"] = 100;
	environment.uniform_generic.uniform_float["dl_max"] = 50;

	environment.uniform_generic.uniform_int["light_n"] = n_lights + 2;

	GLint pos_loc = shader_custom.query_uniform_location("light_positions");
	GLint col_loc = shader_custom.query_uniform_location("light_colors");
	
	for (int i = 0; i < n_lights; i++)
	{
		cgp::vec3 color = light_colors[i];
		cgp::vec3 pos = {50 * cos(10. * i), 50 * sin(10. * i), 0};
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


	// Ball light (inside the ball)
	cgp::vec3 color1 = light_colors[n_lights];
	cgp::vec3 pos1 = ball_position;

	glUniform3f(pos_loc + n_lights, pos1.x, pos1.y, pos1.z);
	glUniform3f(col_loc + n_lights, color1.x, color1.y, color1.z);

	spheres[n_lights].model.translation = pos1;

	// Target light
	cgp::vec3 color2 = light_colors[n_lights+1];
	cgp::vec3 pos2 = target_position;
	pos2.z = target_position.z + 5.0f;

	glUniform3f(pos_loc + n_lights+1, pos2.x, pos2.y, pos2.z);
	glUniform3f(col_loc + n_lights+1, color2.x, color2.y, color2.z);

	spheres[n_lights+1].model.translation = pos2;


	for (mesh_drawable& sphere: spheres)
		draw(sphere, environment);



	if (gui.display_wireframe)
		draw_wireframe(terrain_mesh, environment);

	// first phase: theta = pi/4, choose phi
	if (phase == 1)
	{
		const float phi_freq = 0.5;

		angle_phi = 2 * Pi * (timer.t - last_action_time) * phi_freq;
	}

	// second phase: choose theta
	else if (phase == 2)
	{
		const float theta_freq = 0.5;

		angle_theta = Pi / 4 + Pi / 12 * sin(2 * Pi * (timer.t - last_action_time) * theta_freq);		// smooth angle between pi/6 and pi/3
	}

	else if (phase == 3)
	{
		const float force_freq = 0.5;

		force_strength = 1.0 + 0.7 * sin(2 * Pi * (timer.t - last_action_time) * force_freq);			// smooth force between 0.3 and 1.7
	}

	// draw the force arrow and the parabola if necessary
	if (phase > 0)
	{
		cgp::rotation_transform rot = cgp::rotation_axis_angle({0, 0, 1}, angle_phi) * cgp::rotation_axis_angle({0, 1, 0}, -angle_theta);
		kick_direction = rot * vec3{1, 0, 0};

		force_arrow.model.rotation = rot;
		force_arrow.model.scaling = 2 * force_strength;
		force_arrow.model.translation = ball_position + kick_direction * 2;
		draw(force_arrow, environment);

		glUseProgram(shader_parabola.id);
		// glLineWidth((GLfloat) 2.);

		// draw the parabola
		environment.uniform_generic.uniform_vec3["ball_position"] = ball_position;
		environment.uniform_generic.uniform_vec3["kickforce"] = kick_direction * force_strength * force_coef;
		environment.uniform_generic.uniform_float["gravity"] = gravity;

		environment.uniform_generic.uniform_vec3["segment_color"] = {1., 0., 0.};

		draw(segments, environment);
	}

	if (phase == 0 && cgp::norm(ball_velocity) < stop_threshold && ball_position.z <= terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + 1.5 * ball_radius)
	{
		std::cout << "stop!!\n";
		phase++;
		ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + ball_radius;
		ball_velocity = {0, 0, 0};
		last_action_time = timer.t;
		reset_force();
	}

	camera_control.camera_model.look_at(camera_control.camera_model.position_camera,
		camera_control.camera_model.position_camera + camera_control.camera_model.front(),
		{0, 0, 1});
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

void scene_structure::reset_force()
{
	angle_phi = 0.0f;
	angle_theta = Pi / 4;
	force_strength = 1.0f;
}

void scene_structure::space_pressed()
{
	if (phase == 1 || phase == 2)
	{
		timer.update();
		last_action_time = timer.t;
		phase++;
	}
	else if (phase == 3)
		launch();
}

void scene_structure::reset_position()
{
	timer.update();
	ball_position = {0., 0., terrain.evaluate_terrain_height(0., 0.) + 15 * ball_radius};
	ball_velocity = {0.f, 0.f, 0.f};
	phase = 0;
}

void scene_structure::launch()
{
	std::cout << "launch!!\n";
	phase = 0;
	ball_velocity = kick_direction * force_strength * force_coef;
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
