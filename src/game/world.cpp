#include "world.h"
#include "../render/render_frame.h"
#include "../render/base.h"

void World::init(Free_Camera *camera)
{

	free_camera = camera;
	free_camera->position = Vector3(0.0f, 200.0f, 100.0f);

	Entity * mutant = new Mutant();
	mutant->position = Vector3(400, 0, 0);

	Model * m = new Model();
	m->init_from_file("mutant.fbx");
	m->mesh.allocate_static_buffer();
	mutant->model = m;

	print(m->name);

	//entities.push(mutant);
	entity_manager.add_entity(mutant);


	Entity * soldier = new Soldier();
	soldier->position = Vector3(0, 0, 0);

	Model *s = new Model();
	s->init_from_file("soldier.fbx");
	s->mesh.allocate_static_buffer();
	soldier->model = s;

	entity_manager.add_entity(soldier);

	Entity * mutant2 = new Mutant();
	mutant2->position = Vector3(-400, 0, 0);
	mutant2->model = m;

	entity_manager.add_entity(mutant2);


	Entity * floor = new Floor();
	floor->position = Vector3(0, 0, 0);
	floor->model = generate_floor_model(5000.0f, 5000.0f, 50, 50);
	floor->model->mesh.allocate_static_buffer();
	entity_manager.add_entity(floor);
}

void World::draw()
{
	direct3d.begin_draw();

	Matrix4 view = free_camera->get_view_matrix();
	draw_entities(&entity_manager, view);

	direct3d.end_draw();
}