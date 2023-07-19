#ifndef MODEL_H
#define MODEL_H

#include "../libs/ds/array.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "vertex.h"

template <typename T>
struct Mesh {
	Array<T> vertices;
	Array<u32> indices;
};

typedef Mesh<Vertex_XNUV> Triangle_Mesh;
typedef Mesh<Vector3> Line_Mesh;

//template <typename T>
//struct Unified_Mesh_Storate {
//	struct Mesh_Instance {
//		u32 vertex_count = 0;
//		u32 index_count = 0;
//		u32 vertex_offset = 0;
//		u32 index_offset = 0;
//	};
//
//	Array<T> unified_vertices;
//	Array<u32> unified_indices;
//	Array<Mesh_Instance> mesh_instances;
//	Hash_Table<String_Id, Mesh_Idx> mesh_table;
//
//	Gpu_Struct_Buffer vertex_struct_buffer;
//	Gpu_Struct_Buffer index_struct_buffer;
//	Gpu_Struct_Buffer mesh_struct_buffer;
//
//	void allocate_gpu_memory();
//	bool add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Idx *_mesh_idx);
//};
//
//typedef u32 Render_Model_Idx;
//
//struct Temp {
//	Matrix4 transform_matrix;
//	Mesh_Idx mesh_idx;
//};
//
//struct Render_Model {
//	Array<Temp> temps;
//};
//
//struct Render_Entity {
//	Entity_Id entity_id;
//	Render_Model_Idx render_model_idx;
//};
//
//
//
//struct Render_Model_Manager {
//	
//	Array<Render_Model> render_models;
//
//	Unified_Mesh_Storate<Vertex_XNUV> triangle_meshes;
//	Unified_Mesh_Storate<Vector3> line_meshes;
//
//	bool load_model(const char *file_name, Render_Model_Idx *render_model_idx);
//	
//	Render_Model *get_render_model(Render_Model_Idx render_model_idx);
//};
//
//inline bool Render_Model_Manager::load_model(const char *file_name, Render_Model_Idx *render_model_idx)
//{
//
//}
//
//void draw()
//{
//
//}
#endif 
