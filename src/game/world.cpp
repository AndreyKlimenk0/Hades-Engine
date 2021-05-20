#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_generator.h"

#include "../render/directx.h"


void World::init(Free_Camera *camera)
{

	free_camera = camera;
	free_camera->position = Vector3(0.0f, 200.0f, 100.0f);

	Entity * floor = new Floor();
	floor->id = 4;
	floor->position = Vector3(0, 0, 0);
	floor->model = generate_floor_model(5000.0f, 5000.0f, 50, 50);
	floor->model->mesh.allocate_static_buffer();
	entity_manager.add_entity(floor);

	Entity * mutant = new Mutant();
	mutant->id = 1;
	mutant->position = Vector3(400, 0, 0);

	Render_Model * m = new Render_Model();
	m->init_from_file("mutant.fbx");
	//m->set_model_color(Blue);
	m->mesh.allocate_static_buffer();
	mutant->model = m;

	entity_manager.add_entity(mutant);


	Entity * soldier = new Soldier();
	soldier->id = 2;
	soldier->position = Vector3(0, 0, 500);

	Render_Model *s = new Render_Model();
	s->init_from_file("vampire.fbx");
	//s->set_model_color(Blue);
	s->mesh.allocate_static_buffer();
	soldier->model = s;

	entity_manager.add_entity(soldier);

	Entity * mutant2 = new Mutant();
	mutant2->id = 3;
	mutant2->position = Vector3(-400, 0, 0);
	mutant2->model = m;

	entity_manager.add_entity(mutant2);


	Entity *sphere = new Entity();
	sphere->position = Vector3(0, 100, 300);
	sphere->model = new Render_Model();
	sphere->model->set_model_color(Color::Blue);
	sphere->model->material = make_default_material();
	
	generate_sphere(50.0f, 5, 5, &sphere->model->mesh);
	sphere->model->mesh.allocate_static_buffer();
	entity_manager.add_entity(sphere);

	Entity *box = new Entity();
	box->type = ENTITY_TYPE_SOLDIER;
	box->position = Vector3(300, 100, 300);
	box->model = new Render_Model();
	box->model->set_model_color(Color::Blue);
	box->model->material = make_default_material();
	//generate_sphere(50.0f, 20, 20, &box->model->mesh);
	generate_box(100, 100, 100, &box->model->mesh);
	box->model->mesh.allocate_static_buffer();

	entity_manager.add_entity(box);

	Entity *sphere3 = new Entity();
	sphere3->type = ENTITY_TYPE_SOLDIER;
	sphere3->position = Vector3(500, 100, 300);
	sphere3->model = new Render_Model();
	sphere3->model->set_model_color(Color::Blue);
	sphere3->model->material = make_default_material();
	//generate_sphere(50.0f, 20, 20, &box->model->mesh);
	generate_sphere(50.0f, 20, 20, &sphere3->model->mesh);
	sphere3->model->mesh.allocate_static_buffer();

	entity_manager.add_entity(sphere3);

	Light *l = entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(0.5f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	//Light *l = entity_manager.make_point_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(1.0f, 1.0f, 1.0f), 1.0f, 0.08f, 0.032f);
	//Light *l = entity_manager.make_point_light(Vector3(0.0f, 200.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), 1000.0f);
	//entity_manager.make_point_light(Vector3(0.0f, 200.0f, 500.0f), Vector3(2.0f, 1.0f, 1.0f), 1000.0f);
	//entity_manager.make_point_light(Vector3(000.0f, 200.0f, -700.0f), Vector3(1.0f, 2.0f, 1.0f), 1000.0f);
	//Light *l = entity_manager.make_point_light(Vector3(0.0f, 400.0f, 100.0f), Vector3(1.0f, 1.0f, 1.0f), 1.0f, 0.014f, 0.0007f);
	l->model = new Render_Model();
	l->model->set_model_color(Color(1.0f, 1.0f, 1.0f));
	generate_sphere(10.0f, 30, 30, &l->model->mesh);
	l->model->mesh.allocate_static_buffer();

	//entity_manager.make_spot_light(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), 10.0f);
}