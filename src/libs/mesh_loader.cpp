#include <assert.h>
#include <fbxsdk.h>

#include "math/common.h"
#include "mesh_loader.h"
#include "os/file.h"
#include "../sys/sys_local.h"
#include "../win32/win_time.h"

static const s32 UP_AXIS_SIGN = 1;
static const s32 TRIANGLE_POLYGON = 3;
static const s32 QUAD_POLYGON = 4;

static const char *FOUR_SPACES = "    ";

static FbxManager* fbx_manager = NULL;
static FbxAxisSystem::EUpVector fbx_scene_up_axis = FbxAxisSystem::EUpVector::eYAxis;


static inline Vector3 degrees_to_radians(Vector3 *vector)
{
	return Vector3(degrees_to_radians(vector->x), degrees_to_radians(vector->y), degrees_to_radians(vector->z));
}

static inline Vector3 to_vector3(FbxDouble3 *fbx_vector)
{
	return Vector3((float)fbx_vector->mData[0], (float)fbx_vector->mData[1], (float)fbx_vector->mData[2]);
}

static inline Vector3 to_vector3(FbxVector4 *fbx_vector)
{
	return Vector3((float)fbx_vector->mData[0], (float)fbx_vector->mData[1], (float)fbx_vector->mData[2]);
}

static FbxScene *load_scene_from_fbx_file(const char *full_path_to_file)
{
	FbxScene *fbx_scene = FbxScene::Create(fbx_manager, "");
	if (!fbx_scene) {
		print("load_scene_from_fbx_file: Unable to create Fbx Scene.");
		return NULL;
	}

	FbxImporter *fbx_importer = FbxImporter::Create(fbx_manager, "");
	if (!fbx_importer) {
		print("load_scene_from_fbx_file: Unable to create Fbx Importer.");
		return NULL;
	}

	if (!fbx_importer->Initialize(full_path_to_file)) {
		print("load_scene_from_fbx_file: Unable to initialize Fbx Importer. The path to file '{}'.", full_path_to_file);
		return NULL;
	}

	if (!fbx_importer->Import(fbx_scene)) {
		print("load_scene_from_fbx_file: Unable to import the currently opened file into a scene. The path to file '{}'.", full_path_to_file);
		return NULL;
	}
	fbx_importer->Destroy();
	return fbx_scene;
}

static String get_node_attribute_type_name(FbxNode *fbx_node)
{
	String type_name;
	if (!fbx_node->GetNodeAttribute()) {
		type_name = "No node attribute type";
		return type_name;
	}

	switch (fbx_node->GetNodeAttribute()->GetAttributeType())
	{
		case FbxNodeAttribute::eMarker:                
			type_name = "Marker";               
			break;
		case FbxNodeAttribute::eSkeleton:              
			type_name = "Skeleton";             
			break;
		case FbxNodeAttribute::eMesh:                  
			type_name = "Mesh";                 
			break;
		case FbxNodeAttribute::eCamera:                
			type_name = "Camera";               
			break;
		case FbxNodeAttribute::eLight:                 
			type_name = "Light";                
			break;
		case FbxNodeAttribute::eBoundary:              
			type_name = "Boundary";             
			break;
		case FbxNodeAttribute::eOpticalMarker:        
			type_name = "Optical marker";       
			break;
		case FbxNodeAttribute::eOpticalReference:     
			type_name = "Optical reference";    
			break;
		case FbxNodeAttribute::eCameraSwitcher:       
			type_name = "Camera switcher";      
			break;
		case FbxNodeAttribute::eNull:                  
			type_name = "Null";                 
			break;
		case FbxNodeAttribute::ePatch:                 
			type_name = "Patch";                
			break;
		case FbxNodeAttribute::eNurbs:                  
			type_name = "NURB";                 
			break;
		case FbxNodeAttribute::eNurbsSurface:         
			type_name = "Nurbs surface";        
			break;
		case FbxNodeAttribute::eNurbsCurve:           
			type_name = "NURBS curve";          
			break;
		case FbxNodeAttribute::eTrimNurbsSurface:    
			type_name = "Trim nurbs surface";   
			break;
		case FbxNodeAttribute::eUnknown:          
			type_name = "Unidentified";         
			break;
	}
	return type_name;
}

static void display_node(FbxNode *fbx_node, u32 node_level = 0)
{
	assert(fbx_node);
	String spaces = "";
	for (u32 i = 0; i < node_level; i++) {
		spaces.append(FOUR_SPACES);
	}

	Vector3 scaling = to_vector3(&fbx_node->LclScaling.Get());
	Vector3 rotation = to_vector3(&fbx_node->LclRotation.Get());
	Vector3 translation = to_vector3(&fbx_node->LclTranslation.Get());
	
	print("{}fbx_node:", spaces);
	print("{}    Name:", spaces, fbx_node->GetName());
	print("{}    Parent name:", spaces, fbx_node->GetParent()->GetName());
	print("{}    attribute count:", spaces, fbx_node->GetNodeAttributeCount());
	print("{}    attribute type:", spaces, get_node_attribute_type_name(fbx_node));
	print("{}    scaling:", spaces, &scaling);
	print("{}    rotation:", spaces, &rotation);
	print("{}    translation:", spaces, &translation);
	if (fbx_node->GetNodeAttribute()) {
		switch (fbx_node->GetNodeAttribute()->GetAttributeType()) {
			case FbxNodeAttribute::eMesh: {
				FbxMesh *fbx_mesh = fbx_node->GetMesh();
				if (fbx_mesh) {
					print("{}        Mesh info:", spaces);
					print("{}        Mesh name:", spaces, fbx_mesh->GetName());
					print("{}        Polygons count:", spaces, fbx_mesh->GetPolygonCount());
					print("{}        Vertex count:", spaces, fbx_mesh->GetPolygonVertexCount());
					fbx_mesh->GetPolygonCount();
				}
				break;
			}
		}
	}
	
	for (s32 i = 0; i < fbx_node->GetChildCount(); i++) {
		display_node(fbx_node->GetChild(i), node_level + 1);
	}
}

static void display_scene_nodes(FbxScene *fbx_scene)
{
	assert(fbx_scene);

	FbxNode *fbx_node = fbx_scene->GetRootNode();
	if (fbx_node) {
		for (s32 i = 0; i < fbx_node->GetChildCount(); i++) {
			display_node(fbx_node->GetChild(i));
		}
	}
}

static Vector3 get_fbx_mesh_normal(FbxMesh *fbx_mesh, s32 vertex_index, s32 vertex_count)
{
	assert(fbx_mesh);
	
	Vector3 normal = Vector3::zero;
	if (fbx_mesh->GetElementNormalCount() > 0) {
		FbxGeometryElementNormal *element_normal = fbx_mesh->GetElementNormal(0);
		switch (element_normal->GetMappingMode()) {
			case FbxGeometryElement::eByControlPoint: {
				switch (element_normal->GetReferenceMode()) {
					case FbxGeometryElement::eDirect: {
						FbxVector4 fbx_normal = element_normal->GetDirectArray().GetAt(vertex_index);
						normal.x = (float)fbx_normal.mData[0];
						normal.y = (float)fbx_normal.mData[1];
						normal.z = (float)fbx_normal.mData[2];
						break;
					}
					case FbxGeometryElement::eIndexToDirect: {
						s32 index = element_normal->GetIndexArray().GetAt(vertex_index);
						FbxVector4 fbx_normal = element_normal->GetDirectArray().GetAt(index);
						normal.x = (float)fbx_normal.mData[0];
						normal.y = (float)fbx_normal.mData[1];
						normal.z = (float)fbx_normal.mData[2];
						break;
					}
					default: {
						print("get_fbx_mesh_normal: Unable to get fbx mesh normal. Fbx element normal has unsupported reference mode.");
					}
				}
				break;
			}
			case FbxGeometryElement::eByPolygonVertex: {
				switch (element_normal->GetReferenceMode()) {
					case FbxGeometryElement::eDirect: {
						FbxVector4 fbx_normal = element_normal->GetDirectArray().GetAt(vertex_count);
						normal.x = (float)fbx_normal.mData[0];
						normal.y = (float)fbx_normal.mData[1];
						normal.z = (float)fbx_normal.mData[2];
						break;
					}
					case FbxGeometryElement::eIndexToDirect: {
						s32 index = element_normal->GetIndexArray().GetAt(vertex_count);
						FbxVector4 fbx_normal = element_normal->GetDirectArray().GetAt(index);
						normal.x = (float)fbx_normal.mData[0];
						normal.y = (float)fbx_normal.mData[1];
						normal.z = (float)fbx_normal.mData[2];
						break;
					}
					default: {
						print("get_fbx_mesh_normal: Unable to get fbx mesh normal. Fbx element normal has unsupported reference mode.");
					}
				}
				break;
			}
			default: {
				print("get_fbx_mesh_normal: Unable to get fbx mesh normal. Fbx element normal has unsupported mapping mode.");
			}
		}
	}
	return normal;
}

static Vector2 get_fbx_mesh_uv(FbxMesh *fbx_mesh, s32 vertex_index, s32 uv_index)
{
	assert(fbx_mesh);

	Vector2 uv = Vector2::zero;
	if (fbx_mesh->GetElementUVCount() > 0) {
		FbxGeometryElementUV *element_uv = fbx_mesh->GetElementUV(0);
		switch (element_uv->GetMappingMode()) {
			case FbxGeometryElement::eByControlPoint: {
				switch (element_uv->GetReferenceMode()) {
					case FbxLayerElementUV::eDirect: {
						FbxVector2 fbx_uv = element_uv->GetDirectArray().GetAt(vertex_index);
						uv.x = (float)fbx_uv.mData[0];
						uv.y = (float)fbx_uv.mData[1];
						break;
					}
					case FbxLayerElementUV::eIndexToDirect: {
						s32 index = element_uv->GetIndexArray().GetAt(vertex_index);
						FbxVector2 fbx_uv = element_uv->GetDirectArray().GetAt(index);
						uv.x = (float)fbx_uv.mData[0];
						uv.y = (float)fbx_uv.mData[1];
						break;
					}
					default: {
						print("get_uv_from_fbx_mesh: Unable to get fbx mesh uv. Fbx element uv has unsupported reference mode.");
					}
				}
				break;
			}
			case FbxGeometryElement::eByPolygonVertex: {
				switch (element_uv->GetReferenceMode()) {
					case FbxLayerElementUV::eDirect: {
						FbxVector2 fbx_uv = element_uv->GetDirectArray().GetAt(uv_index);
						uv.x = (float)fbx_uv.mData[0];
						uv.y = (float)fbx_uv.mData[1];
						break; 
					}
					case FbxLayerElementUV::eIndexToDirect: {
						s32 index = element_uv->GetIndexArray().GetAt(uv_index);
						FbxVector2 fbx_uv = element_uv->GetDirectArray().GetAt(index);
						uv.x = (float)fbx_uv.mData[0];
						uv.y = (float)fbx_uv.mData[1];
						break;
					}
					default: {
						print("get_uv_from_fbx_mesh: Unable to get fbx mesh uv. Fbx element uv has unsupported reference mode.");
					}
				}
				break;
			}
			default: {
				print("get_uv_from_fbx_mesh: Unable to get fbx mesh uv. Fbx element uv has unsupported mapping mode.");
			}
		}
	}
	return uv;
}

static bool process_mesh(FbxNode *fbx_node, Triangle_Mesh *mesh)
{
	assert(fbx_node);
	assert(mesh);

	FbxMesh *fbx_mesh = fbx_node->GetMesh();

	mesh->name = fbx_mesh->GetName();
	mesh->vertices.reserve(fbx_mesh->GetControlPointsCount());
	mesh->indices.resize(fbx_mesh->GetPolygonCount() * TRIANGLE_POLYGON);

	for (s32 i = 0; i < fbx_mesh->GetControlPointsCount(); i++) {
		FbxVector4 fbx_vector = fbx_mesh->GetControlPointAt(i);
		mesh->vertices[i].position.x = (float)fbx_vector.mData[0];
		mesh->vertices[i].position.y = (float)fbx_vector.mData[1];
		mesh->vertices[i].position.z = (float)fbx_vector.mData[2];
	}

	s32 vertex_count = 0;
	for (s32 polygon_index = 0; polygon_index < fbx_mesh->GetPolygonCount(); polygon_index++) {
		s32 index_buffer[255];
		s32 polygon_size = fbx_mesh->GetPolygonSize(polygon_index);
		for (s32 i = 0; i < polygon_size; i++) {
			s32 vertex_index = fbx_mesh->GetPolygonVertex(polygon_index, i);
			mesh->vertices[vertex_index].uv = get_fbx_mesh_uv(fbx_mesh, vertex_index, fbx_mesh->GetTextureUVIndex(polygon_index, i));
			mesh->vertices[vertex_index].normal = get_fbx_mesh_normal(fbx_mesh, vertex_index, vertex_count);
			index_buffer[i] = vertex_index;
			vertex_count++;
		}

		if (polygon_size == TRIANGLE_POLYGON) {
			mesh->indices.push(index_buffer[0]);
			mesh->indices.push(index_buffer[1]);
			mesh->indices.push(index_buffer[2]);
		} else if (polygon_size == QUAD_POLYGON) {
			mesh->indices.push(index_buffer[0]);
			mesh->indices.push(index_buffer[1]);
			mesh->indices.push(index_buffer[2]);

			mesh->indices.push(index_buffer[2]);
			mesh->indices.push(index_buffer[3]);
			mesh->indices.push(index_buffer[0]);
		} else {
			return false;
		}
	}
	return true;
}

static void process_node(FbxNode *fbx_node, Array<Import_Mesh> *imported_meshes, Hash_Table<String, s32> *mesh_cache)
{
	assert(fbx_node);
	assert(imported_meshes);
	assert(mesh_cache);

	for (s32 i = 0; i < fbx_node->GetNodeAttributeCount(); i++) {
		if ((fbx_node->GetNodeAttributeByIndex(i)->GetAttributeType() == FbxNodeAttribute::eMesh) && fbx_node->GetMesh()) {
			s32 mesh_index = 0;
			const char *mesh_name = fbx_node->GetMesh()->GetName();
			if (!mesh_cache->get(mesh_name, &mesh_index)) {
				Import_Mesh import_mesh;
				if (!process_mesh(fbx_node, &import_mesh.mesh)) {
					print("process_mesh: Mesh {} has polygon with unsupported size. The mesh will be triangulated.", mesh_name);

					FbxGeometryConverter geometry_converter = FbxGeometryConverter(fbx_manager);
					FbxNodeAttribute *current_node_attribute = fbx_node->GetNodeAttribute();
					FbxNodeAttribute *new_node_attribute = geometry_converter.Triangulate(current_node_attribute, true);

					if (!new_node_attribute || (new_node_attribute == current_node_attribute)) {
						print("process_mesh: Unable to triangulate the fbx mesh.");
						mesh_index = -1;
						continue;
					}
					fbx_node->AddNodeAttribute(new_node_attribute);
					process_mesh(fbx_node, &import_mesh.mesh);
				}
				FbxAMatrix transform_matrix = fbx_node->EvaluateGlobalTransform();
				mesh_index = (s32)imported_meshes->push(import_mesh);
				mesh_cache->set(mesh_name, mesh_index);
			} else {
				if (mesh_index > -1) {
					FbxAMatrix transform_matrix = fbx_node->EvaluateGlobalTransform();

					Import_Mesh::Transform_Info transform_info;
					transform_info.scaling = to_vector3(&transform_matrix.GetS());
					transform_info.rotation = degrees_to_radians(&to_vector3(&transform_matrix.GetR()));
					transform_info.translation = to_vector3(&transform_matrix.GetT());

					imported_meshes->get((u32)mesh_index).mesh_instances.push(transform_info);
				}
			}
		}
	}

	s32 count = fbx_node->GetChildCount();
	for (s32 i = 0; i < count; i++) {
		process_node(fbx_node->GetChild(i), imported_meshes, mesh_cache);
	}
}

static bool process_scene_nodes(FbxScene *fbx_scene, Array<Import_Mesh> *imported_meshes)
{
	assert(fbx_scene);
	assert(imported_meshes);

	FbxNode *fbx_node = fbx_scene->GetRootNode();
	if (fbx_node) {
		Hash_Table<String, s32> mesh_cache;
		for (s32 i = 0; i < fbx_node->GetChildCount(); i++) {
			process_node(fbx_node->GetChild(i), imported_meshes, &mesh_cache);
		}
		return true;
	}
	print("process_scene_nodes: Unable to process nodes. There is no a root node.");
	return false;
}

void init_fbx_lib()
{
	fbx_manager = FbxManager::Create();
	if (fbx_manager) {
		FbxIOSettings* ios = FbxIOSettings::Create(fbx_manager, IOSROOT);
		fbx_manager->SetIOSettings(ios);
	} else {
		print("init_fbx_lib: Unable to create FBX Manager.");
	}
}

bool load_fbx_mesh(const char *full_path_to_file, Array<Import_Mesh> *imported_meshes, bool display_info)
{
	assert(fbx_manager);
	assert(imported_meshes);
	assert(full_path_to_file);
	
	s64 start = milliseconds_counter();

	String file_name;
	extract_file_from_path(full_path_to_file, file_name);
	print("load_fbx_mesh: Started to load fbx mesh from {}.", file_name);
	
	if (!file_exists(full_path_to_file)) {
		print("load_fbx_mesh: Unable to load a fbx model. The path '{}' doesn't exist.", full_path_to_file);
		return false;
	}

	FbxScene *fbx_scene = load_scene_from_fbx_file(full_path_to_file);
	if (!fbx_scene) {
		print("load_fbx_mesh: Unable to load a scene from the fbx file by the path ''.", full_path_to_file);
		return false;
	}

	s32 axis_sign = 0;
	fbx_scene_up_axis = fbx_scene->GetGlobalSettings().GetAxisSystem().GetUpVector(axis_sign);
	if (axis_sign != UP_AXIS_SIGN) {
		print("load_fbx_mesh: Unable to load fbx meshes from {}. The function supports only up axis with sigh 1", file_name);
		fbx_scene->Destroy();
		return false;
	}

	if (fbx_scene->GetGlobalSettings().GetAxisSystem() != FbxAxisSystem::DirectX) {
		FbxAxisSystem::DirectX.DeepConvertScene(fbx_scene);
	}

	if (display_info) {
		display_scene_nodes(fbx_scene);
	}

	bool result = process_scene_nodes(fbx_scene, imported_meshes);
	if (result) {
		s64 delta_time = milliseconds_counter() - start;
		print("load_fbx_mesh: {} mesh was loaded succeeded. Loading time is {}ms.", file_name, delta_time);
	}
	
	fbx_scene->Destroy();
	return result;
}

void shutdown_fbx_lib()
{
	fbx_manager->Destroy();
}

Import_Mesh::Import_Mesh(const Import_Mesh &other)
{
	*this = other;
}

Import_Mesh & Import_Mesh::operator=(const Import_Mesh &other)
{
	if (this != &other) {
		mesh_instances = other.mesh_instances;
		mesh.name = other.mesh.name;
		mesh.vertices = other.mesh.vertices;
		mesh.indices = other.mesh.indices;
	}
	return *this;
}
