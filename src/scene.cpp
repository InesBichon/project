#include "scene.hpp"
#include "terrain.hpp"

using namespace cgp;

cgp::vec3 get_random_color()
{
	return {rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0), rand_uniform(0.3, 1.0)};
}

cgp::vec3 get_random_normalized()
{
	return cgp::normalize(cgp::vec3{cgp::rand_uniform(-1., 1.), cgp::rand_uniform(-1., 1.), cgp::rand_uniform(-1., 1.)});
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

	// change the random seed
	srand(time(NULL));
	cgp::rand_initialize_generator();

	global_frame.initialize_data_on_gpu(mesh_primitive_frame());

	// load shaders

	shader_custom.load(
		project::path + "shaders/shading_custom/shading_custom.vert.glsl",
		project::path + "shaders/shading_custom/shading_custom.frag.glsl");

	shader_parabola.load(
		project::path + "shaders/shading_parabola/shading_parabola.vert.glsl",
		project::path + "shaders/shading_parabola/shading_parabola.frag.glsl"
	);

	// intialize terrain

	terrain.create_terrain_mesh(N_terrain_samples, terrain_length, n_bumps);

	terrain_mesh.initialize_data_on_gpu(terrain.mesh);
	terrain_mesh.shader = shader_custom;
	terrain_mesh.material.color = {1, 1, 1};

	// initialize the camera

	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.translation_speed *= 10;
	camera_control.camera_model.position_camera = {-10, -10, terrain.evaluate_terrain_height(-10,-10) + 10};

	// initialize the position & speed of the ball (this also moves the camera to look at the ball)
	
	reset_position();

	// initialize light meshes, positions, speeds and colors
	// (n_lights moving lights, one inside the ball, one above the target)

	spheres.resize(n_lights+2);
	light_colors.resize(n_lights+2);
	light_pos.resize(n_lights);
	light_speed.resize(n_lights);

	for (int i = 0; i < n_lights; i++)
	{
		light_colors[i] = get_random_color();

		// avoid spawning lights too close to the walls and spawn them slightly above ground
		light_pos[i] = {cgp::rand_uniform(-terrain_length / 2.2, terrain_length / 2.2), cgp::rand_uniform(-terrain_length / 2.2, terrain_length / 2.2), 0};
		light_pos[i].z = terrain.evaluate_terrain_height(light_pos[i].x, light_pos[i].y) + 3.0f;

		light_speed[i] = get_random_normalized();
		
		mesh sphere_mesh = mesh_primitive_sphere();
		spheres[i].initialize_data_on_gpu(sphere_mesh);
		spheres[i].model.scaling = 0.5f;
		spheres[i].material.color = light_colors[i];
		// spheres[i].shader = shader_custom;
	}

	// initialize the position of the light inside the ball, and the light above the target
	// (they're special lights so they aren't included in n_lights)

	// red light of the ball
	light_colors[n_lights] = {1.0f, 0, 0};
	mesh sphere_mesh1 = mesh_primitive_sphere();
	spheres[n_lights].initialize_data_on_gpu(sphere_mesh1);
	spheres[n_lights].model.scaling = 0.2f;
	spheres[n_lights].material.color = light_colors[n_lights];

	// blue light above the target	
	light_colors[n_lights+1] = {0, 0, 1.0f};
	mesh sphere_mesh2 = mesh_primitive_sphere();
	spheres[n_lights+1].initialize_data_on_gpu(sphere_mesh2);
	spheres[n_lights+1].model.scaling = 0.2f; // coordinates are multiplied by 0.2 in the shader
	spheres[n_lights+1].material.color = light_colors[n_lights+1];

	// initialize the ball mesh

	mesh ball_mesh = mesh_primitive_sphere();
	ball.initialize_data_on_gpu(ball_mesh);
	ball.model.scaling = ball_radius;
	// ball.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/tex.jpeg");
	// since the ball doesn't roll, the texture was fixed and didn't look nice
	ball.material.color = {1,0,0};

	// initialize the target mesh

	mesh torus_mesh = mesh_primitive_torus(torus_max_radius, torus_min_radius);
	target.initialize_data_on_gpu(torus_mesh);
	target.material.color = {0., 0., 9.};
	target.shader = shader_custom;
	
	// the torus is facing the y axis (ie. a (0,1,0) vector goes through the target hole)
	rotation_transform R = rotation_transform::from_axis_angle({ 1,0,0 }, Pi / 2);
	target.model.rotation = R;

	reset_target_position();

	// initialize the force arrow mesh
	// force arrow initially from (0,0,0) to (1,0,0)

	mesh force_arrow_mesh = mesh_primitive_arrow();
	force_arrow.initialize_data_on_gpu(force_arrow_mesh);
	force_arrow.material.color = {0.8, 0.8, 0.8};
	force_arrow.material.phong.ambient = 1;
	force_arrow.material.phong.diffuse = 0;
	force_arrow.material.phong.specular = 0;

	reset_force();

	// initialize the skybox (code from the cgp examples)

	image_structure image_skybox_template = image_load_file(project::path+"assets/skybox.jpg");
	std::vector<image_structure> image_grid = image_split_grid(image_skybox_template, 4, 3);

	skybox.initialize_data_on_gpu();
	skybox.texture.initialize_cubemap_on_gpu(image_grid[1], image_grid[7], image_grid[5], image_grid[3], image_grid[10], image_grid[4]);
	skybox.model.rotation = cgp::rotation_axis_angle({1,0,0}, Pi/2);

	// initialize the curve for the parabola: it's actually a chain of N segments going from (0,0,0) to (1,0,0) in a straight line
	// then, the actual parabola will be computed in the vertex shader to avoid copying data to the GPU each frame
	// we'll just need to pass the position, force & gravity as uniforms

	std::vector<vec3> positions(N_parabola, {0., 0., 0.});
	for (int i = 0; i < N_parabola; i++)
		positions[i].x = (float)(i) / (N_parabola - 1);

	segments.display_type = curve_drawable_display_type::Curve;
	segments.shader = shader_parabola;
	segments.initialize_data_on_gpu(positions, shader_parabola);

	// the ball starts in the air, so it's moving (phase 0)
	phase = 0;

	// remove the uncaught uniforms warning
	cgp_warning::max_warning = 0;
	
	// helper message
	std::cout << general_message;
}

void scene_structure::simulation_step(float dt)
{
	if (phase > 0)
		return;

	float m = 0.01f;       		// ball mass
	vec3 g = { 0,0,-gravity}; 	// gravity


	ball_weight = m * g;
	ball_force = ball_weight;

	ball_velocity = ball_velocity + dt * ball_force / m;
	check_target_hit(ball_position, ball_position + dt * ball_velocity);
	ball_position = ball_position + dt * ball_velocity;

	vec3 normal = terrain.get_normal_from_position(terrain.N, terrain.terrain_length, ball_position.x, ball_position.y);

	if (ball_position.z - ball_radius <= terrain.evaluate_terrain_height(ball_position.x, ball_position.y) && dot(ball_velocity, normal) < 0)
	{
		// we went under the ground: reflect towards the normal (and reduce the speed norm to lose energy)
		ball_velocity = 0.8 * reflect(ball_velocity, normal);
		// stay above the ground
		ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + ball_radius;

		// we want the ball to slide down slopes reasonably fast, but not gain too much speed (otherwise, it falls with a constant & low speed)
		// but also not enter infinite loops so we stop it after 5 seconds
		if (normal.z < 0.995 && cgp::norm(ball_velocity) < 3 && timer.t - last_action_time < 5)
			ball_velocity = 1.3 * ball_velocity;

		// if 10 seconds have passed, we stop once it's slow enough (otherwise, it can get boring)
		if (timer.t - last_action_time > 10 && norm(ball_velocity) < 0.5)
		{
			ball_velocity = {0,0,0};
			ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + ball_radius;
		}
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
	
	// move the camera (no longer necessary with the first person camera structure)
	// move_cam(interval);

	update_light_pos(interval);
	simulation_step(timer.scale * 0.1f);

	// draw the skybox before everything else
	glDepthMask(GL_FALSE);
	draw(skybox, environment);
	glDepthMask(GL_TRUE);

	// if (gui.display_frame)
	// 	draw(global_frame, environment);

	draw(terrain_mesh, environment);

	ball.model.translation = ball_position;
	draw(ball, environment);

	draw(target, environment);

	// the first n_lights are regular lights, the last 2 follow the ball and the target
	glUseProgram(shader_custom.id);

	// if the ball went through the target in the last 5 seconds, we want to display a pretty win animation
	bool is_win_animation = last_win_time != -1.0f && timer.t - last_win_time <= 5;

	environment.uniform_generic.uniform_float["ambiant"] = 1.0f / n_lights;
	environment.uniform_generic.uniform_float["diffuse"] = 5.f / n_lights;
	environment.uniform_generic.uniform_float["specular"] = 35.f / n_lights;
	environment.uniform_generic.uniform_float["specular_exp"] = 100;
	environment.uniform_generic.uniform_float["dl_max"] = is_win_animation ? 100 : 30;

	environment.uniform_generic.uniform_int["light_n"] = n_lights + 2;

	// we need to use a bit of raw OpenGL to access uniform arrays in the shaders
	GLint pos_loc = shader_custom.query_uniform_location("light_positions");
	GLint col_loc = shader_custom.query_uniform_location("light_colors");

	for (int i = 0; i < n_lights; i++)
	{		
		cgp::vec3 color = light_colors[i];

		// if we just won, the lights will be red/green/blue and switch color every 0.5 second; otherwise, use the default colors
		if (is_win_animation)
		{
			int nb = (int)((timer.t - last_win_time) * 2);
			color = {(i + nb) % 3 == 0, (i + nb) % 3 == 1, (i + nb) % 3 == 2};
		}

		cgp::vec3 pos = light_pos[i];

		glUniform3f(pos_loc + i, pos.x, pos.y, pos.z);
		glUniform3f(col_loc + i, color.x, color.y, color.z);

		spheres[i].model.translation = pos;
	}

	// Ball light (inside the ball)
	cgp::vec3 color1 = light_colors[n_lights];
	cgp::vec3 pos1 = ball_position;

	glUniform3f(pos_loc + n_lights, pos1.x, pos1.y, pos1.z);
	glUniform3f(col_loc + n_lights, color1.x, color1.y, color1.z);

	spheres[n_lights].model.translation = pos1;

	// Target light
	cgp::vec3 color2 = light_colors[n_lights+1];
	cgp::vec3 pos2 = target.model.translation;
	pos2.z = pos2.z + 5.0f;

	glUniform3f(pos_loc + n_lights+1, pos2.x, pos2.y, pos2.z);
	glUniform3f(col_loc + n_lights+1, color2.x, color2.y, color2.z);

	spheres[n_lights+1].model.translation = pos2;

	for (mesh_drawable& sphere: spheres)
		draw(sphere, environment);

	// if (gui.display_wireframe)
	// 	draw_wireframe(terrain_mesh, environment);

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

	// third phase: choose the force strength
	else if (phase == 3)
	{
		const float force_freq = 0.5;

		force_strength = 1.0 + 0.7 * sin(2 * Pi * (timer.t - last_action_time) * force_freq);			// smooth force between 0.3 and 1.7
	}

	// draw the force arrow and the parabola if the ball isn't currently in its movement phase
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
		// apparently, glLineWidth isn't supported anymore on modern devices... shame

		// give the vertex shader the necessary information to compute the shape of the parabola
		environment.uniform_generic.uniform_vec3["ball_position"] = ball_position;
		environment.uniform_generic.uniform_vec3["kickforce"] = kick_direction * force_strength * force_coef;
		environment.uniform_generic.uniform_float["gravity"] = gravity;

		environment.uniform_generic.uniform_vec3["segment_color"] = {1., 0., 0.};

		draw(segments, environment);
	}

	// stop the ball if it's going slow & near the ground (and in the movement phase)
	if (phase == 0 && cgp::norm(ball_velocity) < stop_threshold && ball_position.z <= terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + 1.5 * ball_radius)
	{
		phase++;
		ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + ball_radius;
		ball_velocity = {0, 0, 0};
		last_action_time = timer.t;
		reset_force();
	}

	// we want the camera to stay inside the arena (x & y between -boundary and boundary), above the ground (z >= height of ground + 1) and with a correct "up" vector

	vec3 campos = camera_control.camera_model.position_camera;
	float boundary = terrain_length * 0.45;

	campos.x = std::max(std::min(campos.x, boundary), -boundary);
	campos.y = std::max(std::min(campos.y, boundary), -boundary);
	campos.z = std::max(campos.z, terrain.evaluate_terrain_height(campos.x, campos.y) + 1.f);

	camera_control.camera_model.position_camera = campos;

	camera_control.camera_model.look_at(camera_control.camera_model.position_camera,
		camera_control.camera_model.position_camera + camera_control.camera_model.front(),
		{0, 0, 1});
}


void scene_structure::display_gui()
{
	// we do not need gui parameters
	return;
}

void scene_structure::reset_force()
{
	// reset the force to default
	angle_phi = 0.0f;
	angle_theta = Pi / 4;
	force_strength = 1.0f;
}

void scene_structure::space_pressed()
{
	// increase the phase (do nothing if phase = 0)
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
	// reset the ball position to a random point (above the ground)

	float boundary = terrain_length * 0.4;
	ball_position = {cgp::rand_uniform(-boundary, boundary), cgp::rand_uniform(-boundary, boundary), 0};
	ball_position.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + 15 * ball_radius;
	ball_velocity = {0.f, 0.f, 0.f};

	cgp::vec3 look_at_pos = ball_position;
	look_at_pos.z = terrain.evaluate_terrain_height(ball_position.x, ball_position.y) + 3 * ball_radius;

	camera_control.look_at(camera_control.camera_model.position_camera, ball_position, {0,0,1});
	phase = 0;
}

void scene_structure::reset_target_position()
{
	// reset the target position to a random point
	float boundary = terrain_length * 0.4;

	vec3 pos = {cgp::rand_uniform(-boundary, boundary), cgp::rand_uniform(-boundary, boundary), 0};
	pos.z = terrain.evaluate_terrain_height(pos.x, pos.y) + torus_max_radius;

	target.model.translation = pos;
}


void scene_structure::launch()
{
	// launch the ball after the force has been chosen

	phase = 0;
	ball_velocity = kick_direction * force_strength * force_coef;
	timer.update();
	last_action_time = timer.t;
}

void scene_structure::update_light_pos(float time_passed)
{
	// update the lights positions & speeds
	// every frame, we randomly change every speed by a small vector to make them move smoothly but also randomly
	// we always assume the speeds to be normalized

	const float speed = 3.;			// actual speed of the ball

	// coefficients of actual speed, direction towards the ball, c3 = 1 - c1 - c2 random direction
	// actually, it seems better to use a random direction without considering the ball
	// (at first, we wanted the lights to go towards the ball in a menacing way)

	const float c1 = 0.95, c2 = 0.0;

	for (int i = 0; i < n_lights; i++)
	{
		light_speed[i] = cgp::normalize(c1 * light_speed[i] + c2 * cgp::normalize(ball_position - light_pos[i]) + (1-c1-c2) * get_random_normalized());

		light_pos[i] += time_passed * speed * light_speed[i];
		
		// if balls are getting to close to the walls, we reverse the corresponding speed coordinate
		// no need to re-normalize the speed

		if (light_pos[i].x > terrain.terrain_length / 2.2 && light_speed[i].x > 0 || light_pos[i].x < -terrain.terrain_length / 2.2 && light_speed[i].x < 0)
			light_speed[i].x = -light_speed[i].x;

		if (light_pos[i].y > terrain.terrain_length / 2.2 && light_speed[i].y > 0 || light_pos[i].y < -terrain.terrain_length / 2.2 && light_speed[i].y < 0)
			light_speed[i].y = -light_speed[i].y;

		light_pos[i].z = terrain.evaluate_terrain_height(light_pos[i].x, light_pos[i].y) + 3.;
	}
}

void scene_structure::check_target_hit(vec3 old_pos, vec3 new_pos)
{
	// check whether the target was hit (if it's the case, update last_win_time and display a message)

	// recall that the target is always facing the y axis (that is, a {0,1,0} vector is going through the hole)
	
	vec3 target_pos = target.model.translation;

	// first condition: we need to go from one side of the y plane to another, ie. the sign of (pos.y - target.y) has changed
	if ((new_pos.y - target_pos.y) * (old_pos.y - target_pos.y) >= 0)
		return;

	// then, we compute the position of the intersection point (pos.y == target.y)
	vec3 intersection = old_pos + (new_pos - old_pos) * (target_pos.y - old_pos.y) / (new_pos.y - old_pos.y);

	// and we just have to check if the distance from the intersection point to the center of the target is <= the target radius
	if (cgp::norm(intersection - target_pos) <= torus_max_radius)
	{
		std::cout << "\nCongratulations!\n\n";
		
		reset_target_position();
		
		timer.update();
		last_win_time = timer.t;
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
