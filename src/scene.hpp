#pragma once


#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "terrain.hpp"

// This definitions allow to use the structures: mesh, mesh_drawable, etc. without mentionning explicitly cgp::
using cgp::mesh;
using cgp::mesh_drawable;
using cgp::vec3;
using cgp::numarray;
using cgp::timer_basic;

// Variables associated to the GUI
struct gui_parameters {
	bool display_frame = true;
	bool display_wireframe = false;
};

// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {
	
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //
	// camera_controller_orbit_euler camera_control;
	camera_controller_first_person camera_control;
	camera_projection_perspective camera_projection;
	window_structure window;
	opengl_shader_structure shader_custom;		// shader with Phong lighting
	opengl_shader_structure shader_parabola;	// shader allowing to dynamically compute a parabolic shape

	mesh_drawable global_frame;          // The standard global frame
	environment_structure environment;   // Standard environment controler
	input_devices inputs;                // Storage for inputs status (mouse, keyboard, window dimension)
	gui_parameters gui;                  // Standard GUI element storage

	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //

	Terrain terrain;
	cgp::mesh_drawable terrain_mesh;
	timer_basic timer;

	int n_lights = 10;

	std::vector<mesh_drawable> spheres;
	mesh_drawable sphere_light;

	std::vector<cgp::vec3> light_colors;

	cgp::skybox_drawable skybox;
	// cgp::mesh_drawable tree;

	// int N_trees = 100;
	// std::vector<cgp::vec3> tree_position;

	int N_parabola = 100;		// number of points in the parabola

	mesh_drawable ball;
	mesh_drawable target;
	mesh_drawable force_arrow;	// default position: from (0,0,0) to (1,0,0)
	curve_drawable segments;

	// Ball parameters
	vec3 ball_position;
	vec3 ball_velocity;
	vec3 ball_force;
	vec3 ball_weight;

	const float force_coef = 6;		// multiply the force strength by this value
	const float gravity = 9.81 * 0.2f;

	// 0 when the ball is moving, 1 when choosing a horizontal angle for the kick, 2 ... vertical angle, 3 force.
	int phase;
	float angle_phi = 0.f;			// when choosing the force, horizontal angle
	float angle_theta = Pi / 4;		// when choosing the force, vertical angle
	float force_strength = 1.0f;	// when choosing the force, strength of the force

	float last_action_time = 0.0f;	// when choosing the force, this will be updated to the time of the last action (to avoid angle discontinuities)
	float last_frame_time = -1.0f;

	vec3 kick_direction;

	// stop when the speed is lower than this quantity
	float stop_threshold = 0.2;

	float ball_radius = 1.0f;

	float torus_max_radius = 2.2f;
	float torus_min_radius = 0.2f;

	// ****************************** //
	// Functions
	// ****************************** //

	void simulation_step(float dt);
	cgp::vec3 reflect(cgp::vec3 v, cgp::vec3 n);


	void initialize();    // Standard initialization to be called before the animation loop
	void display_frame(); // The frame display to be called within the animation loop
	void display_gui();   // The display of the GUI, also called within the animation loop

	void reset_force();
	void space_pressed();	// to be called when the user presses space
	void reset_position();	// to be called when the user presses z, resets the position of the ball
	void launch();			// launch the ball

	// void move_cam(float time_passed);		// move the camera (with a given real time between the previous frame and the actual one)
											// no longer necessary with camera_controller_first_person (we manually re-implemnted WASD)
	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();

	void display_info();
};





