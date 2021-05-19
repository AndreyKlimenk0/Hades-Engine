#ifndef RENDER_H
#define RENDER_H

struct Triangle_Mesh;
struct Entity;
struct Fx_Shader;
struct Light;
struct Matrix4;

enum Render_API {
	DIRECTX11,
	OPENGL
};

struct Render {
	Render_API render_api;

	virtual void init() = 0;
	virtual void resize() = 0;
	virtual void shutdown() = 0;

	virtual void begin_draw() = 0;
	virtual void end_draw() = 0;

	virtual void draw_mesh(Triangle_Mesh *mesh) = 0;
	virtual void draw_shadow(Entity *entity, Fx_Shader *fx_shader_light, Light *light, Matrix4 &view, Matrix4 &perspective) = 0;
	
	virtual void create_default_buffer(Triangle_Mesh *mesh) = 0;
};
#endif