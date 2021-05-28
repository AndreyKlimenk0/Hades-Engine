#include <D3DX11.h>
#include <stdlib.h>

#include "fbx_loader.h"

#include "../sys/sys_local.h"

#include "../render/model.h"
#include "../render/vertex.h" 

#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"


static void read_uv_from_fbx_mesh(FbxMesh *fbx_mesh, int vertex_index, int uv_index, Vector2 *uv) 
{
	FbxLayerElementUV *uv_element = fbx_mesh->GetLayer(0)->GetUVs();

	switch (uv_element->GetMappingMode()) {

		case FbxLayerElementUV::eByControlPoint: {
			switch (uv_element->GetReferenceMode()) {
				case FbxLayerElementUV::eDirect: {
					FbxVector2 fbx_uv = uv_element->GetDirectArray().GetAt(vertex_index);

					uv->x = static_cast<float>(fbx_uv.mData[0]);
					uv->y = static_cast<float>(fbx_uv.mData[1]);

					break;
				}
				case FbxLayerElementUV::eIndexToDirect: {
					int uv_index = uv_element->GetIndexArray().GetAt(vertex_index);
					FbxVector2 fbx_uv = uv_element->GetDirectArray().GetAt(uv_index);

					uv->x = static_cast<float>(fbx_uv.mData[0]);
					uv->y = static_cast<float>(fbx_uv.mData[1]);

					break;
				}
			}
			break;
		}
		case FbxLayerElementUV::eByPolygonVertex: {
			switch (uv_element->GetReferenceMode()) {
				case FbxLayerElementUV::eDirect:
				case FbxLayerElementUV::eIndexToDirect: {

					uv->x = static_cast<float>(uv_element->GetDirectArray().GetAt(uv_index).mData[0]);
					uv->y = static_cast<float>(uv_element->GetDirectArray().GetAt(uv_index).mData[1]);

					break;
				}
			}
			break;
		}
	}
}

static void read_normal_from_fbx_mesh(FbxMesh* fbx_mesh, int vertex_index, int vertex_count, Vector3 *normal)
{
	FbxGeometryElementNormal* fbx_normal = fbx_mesh->GetElementNormal(0);

	switch (fbx_normal->GetMappingMode()) {
		case FbxGeometryElement::eByControlPoint: {
			switch (fbx_normal->GetReferenceMode()) {
				case FbxGeometryElement::eDirect: {
					normal->x = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_index).mData[0]);
					normal->y = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_index).mData[1]);
					normal->z = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_index).mData[2]);
					break;
				}
				case FbxGeometryElement::eIndexToDirect: {
					int index = fbx_normal->GetIndexArray().GetAt(vertex_index);
					normal->x = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[0]);
					normal->y = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[1]);
					normal->z = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[2]);
					break;
				}
				default:
					print("In the case ByControlPoint there is not case for Fbx normal GetReferenceMode");
			}
			break;
		}
		case FbxGeometryElement::eByPolygonVertex: {
			switch (fbx_normal->GetReferenceMode()) {
				case FbxGeometryElement::eDirect: {
					normal->x = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_count).mData[0]);
					normal->y = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_count).mData[1]);
					normal->z = static_cast<float>(fbx_normal->GetDirectArray().GetAt(vertex_count).mData[2]);
					break;
				}
				case FbxGeometryElement::eIndexToDirect: {
					int index = fbx_normal->GetIndexArray().GetAt(vertex_count);
					normal->x = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[0]);
					normal->y = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[1]);
					normal->z = static_cast<float>(fbx_normal->GetDirectArray().GetAt(index).mData[2]);
					break;	
				}
				default:
					print("In the case ByPolygonVertex there is not case for Fbx normal GetReferenceMode");
			}
			break;
		}
	}
}

static FbxTexture *find_texture(FbxNode *mesh_node, const char *texture_type)
{
	int count = mesh_node->GetSrcObjectCount<FbxSurfaceMaterial>();

	for (int index = 0; index < count; index++) {
		FbxSurfaceMaterial *material = (FbxSurfaceMaterial *)mesh_node->GetSrcObject<FbxSurfaceMaterial>(index);
		if (material) {
			FbxProperty prop = material->FindProperty(texture_type);

			int layered_texture_count = prop.GetSrcObjectCount<FbxLayeredTexture>();
			if (layered_texture_count > 0) {
				for (int j = 0; j < layered_texture_count; j++) {
					FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
					int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();
					for (int k = 0; k < lcount; k++) {
						FbxTexture* texture = FbxCast<FbxTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
						return texture;
					}
				}
			} else {
				int texture_count = prop.GetSrcObjectCount<FbxTexture>();
				for (int j = 0; j < texture_count; j++) {
					FbxTexture* texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(j));
					return texture;
				}
			}
		}
	}
	return NULL;
}

static void find_and_copy_material(FbxNode *mesh_node, Render_Model *model)
{
	int count = mesh_node->GetMaterialCount();

	if (count <= 0) {
		print("There is no material in fbx file");
		return;
	}

	FbxSurfaceMaterial *material = mesh_node->GetMaterial(0);
	
	FbxProperty ambient = material->FindProperty(FbxSurfaceMaterial::sAmbient);
	FbxProperty diffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
	FbxProperty specular = material->FindProperty(FbxSurfaceMaterial::sSpecular);
	FbxProperty shininess = material->FindProperty(FbxSurfaceMaterial::sShininess);
	
	FbxColor ambient_color = ambient.Get<FbxColor>();
	FbxColor diffuse_color = diffuse.Get<FbxColor>();
	FbxColor specular_color = specular.Get<FbxColor>();
	FbxDouble shininess_color = shininess.Get<FbxDouble>();

	model->material.ambient = Vector4(ambient_color.mRed, ambient_color.mGreen, ambient_color.mBlue, ambient_color.mAlpha);
	model->material.diffuse = Vector4(diffuse_color.mRed, diffuse_color.mGreen, diffuse_color.mBlue, diffuse_color.mAlpha);
	model->material.specular = Vector4(specular_color.mRed, specular_color.mGreen, specular_color.mBlue, shininess_color);
}

static bool get_texture_file_name(FbxNode *mesh_node, const char *texture_type, String *file_name)
{
	FbxTexture *texture = find_texture(mesh_node, texture_type);
	if (!texture) {
		print("FbxTexture of type {} was not found in the file", texture_type, file_name);
		return false;
	}

	FbxFileTexture *file_texture = FbxCast<FbxFileTexture>(texture);
	const char *full_texture_path = file_texture->GetFileName();

	Array<char *> buffer;
	split(full_texture_path, "/", &buffer);
	*file_name = buffer.last_item();
	return true;
}

inline bool get_specular_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sSpecular, file_name);
}

inline bool get_diffuse_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sDiffuse, file_name);
}

inline bool get_normal_texture_file_name(FbxNode *mesh_node, String *file_name)
{
	return get_texture_file_name(mesh_node, FbxSurfaceMaterial::sNormalMap, file_name);
}

static FbxNode *find_fbx_mesh_node_in_scene(FbxScene *scene)
{
	FbxMesh *fbx_mesh = NULL;
	FbxNode *root_node = scene->GetRootNode();

	int child_count = root_node->GetChildCount();
	
	for (int i = 0; i < child_count; i++) {
		FbxNode *node = root_node->GetChild(i);
		if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
			return node;
		}
	}
	return NULL;
}

static void copy_fbx_mesh_to_triangle_mesh(FbxMesh *fbx_mesh, Triangle_Mesh *mesh)
{
	assert(mesh);

	if (!fbx_mesh) {
		print("Fbx mesh was not found in the scene");
		return;
	}

	int position_count = fbx_mesh->GetControlPointsCount();

	mesh->allocate_vertices(position_count);

	for (int i = 0; i < position_count; i++) {

		FbxVector4 position = fbx_mesh->GetControlPointAt(i);
		mesh->vertices[i].position.x = (float)position.mData[0];
		mesh->vertices[i].position.y = (float)position.mData[1];
		mesh->vertices[i].position.z = (float)position.mData[2];
	}

	Array<u32> indices;
	int vertex_count = 0;
	int polygon_count = fbx_mesh->GetPolygonCount();

	for (int i = 0; i < polygon_count; i++) {

		int index_buffer[4];
		int polygon_size = fbx_mesh->GetPolygonSize(i);
		for (int j = 0; j < polygon_size; j++) {

			int vertex_index = fbx_mesh->GetPolygonVertex(i, j);
			index_buffer[j] = vertex_index;

			Vector2 uv;
			read_uv_from_fbx_mesh(fbx_mesh, vertex_index, fbx_mesh->GetTextureUVIndex(i, j), &uv);

			mesh->vertices[vertex_index].uv.x = uv.x;
			mesh->vertices[vertex_index].uv.y = 1.0f  - uv.y;

			Vector3 normal;
			read_normal_from_fbx_mesh(fbx_mesh, vertex_index, vertex_count, &normal);
			mesh->vertices[vertex_index].normal = normal;

			vertex_count++;
		}

		if (polygon_size == 3) {
			indices.push(index_buffer[0]);
			indices.push(index_buffer[1]);
			indices.push(index_buffer[2]);
		} else {
			// covert quad indices to triangle indices
			indices.push(index_buffer[0]);
			indices.push(index_buffer[1]);
			indices.push(index_buffer[2]);

			indices.push(index_buffer[2]);
			indices.push(index_buffer[3]);
			indices.push(index_buffer[0]);
		}
	}

	mesh->copy_indices(indices.items, indices.count);

	fbx_mesh->Destroy();
}

static FbxScene *load_scene_from_fbx_file(String *file_path)
{
	assert(file_path);

	FbxManager* fbx_manager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(fbx_manager, IOSROOT);
	fbx_manager->SetIOSettings(ios);

	FbxScene* scene = FbxScene::Create(fbx_manager, "myScene");

	int file_format = -1;
	FbxImporter* lImporter = FbxImporter::Create(fbx_manager, "");
	if (!fbx_manager->GetIOPluginRegistry()->DetectReaderFileFormat(*file_path, file_format)) {
		file_format = fbx_manager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");
	}

	bool lImportStatus = lImporter->Initialize(*file_path, file_format);
	if (!lImportStatus) {
		print("Initializing Fbx Imported failed, file path {}", *file_path);
		return NULL;
	}

	lImporter->Import(scene);

	lImporter->Destroy();

	return scene;
}

void load_fbx_model(const char *file_name, Render_Model *model)
{
	String *path_to_model_file = os_path.build_full_path_to_model_file(&String(file_name));

	FbxScene *scene = load_scene_from_fbx_file(path_to_model_file);

	if (!scene)
		return;

	FbxNode *mesh_node = find_fbx_mesh_node_in_scene(scene);

	String normal_texture_name;
	String diffuse_texture_name;
	String specular_texture_name;

	get_normal_texture_file_name(mesh_node, &normal_texture_name);
	get_diffuse_texture_file_name(mesh_node, &diffuse_texture_name);
	get_specular_texture_file_name(mesh_node, &specular_texture_name);

	load_texture(&diffuse_texture_name, &model->diffuse_texture);

	find_and_copy_material(mesh_node, model);

	copy_fbx_mesh_to_triangle_mesh(mesh_node->GetMesh(), &model->mesh);
}