#include "shader.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include <D3DCompiler.inl>


const String HLSL_FILE_EXTENSION = "cso";

static void get_shader_name_from_file(const char *file_name, String &name)
{
	String f_name = file_name;
	
	Array<String> buffer;
	split(&f_name, "_", &buffer);

	for (int i = 0; i < (buffer.count - 1); i++) {
		if (i != 0) {
			name.append("_");
		}
		name.append(buffer[i]);
	}
}

static bool get_shader_type_from_file_name(const char *file_name, Shader_Type *shader_type)
{
	String name;
	String file_extension;
	
	extract_file_extension(file_name, file_extension);
	if (file_extension != HLSL_FILE_EXTENSION) {
		print("get_shader_type_from_file: {} has wrong file extension.", file_name);
		return false;
	}

	extract_file_name(file_name, name);

	Array<String> strings;
	bool result = split(&name, "_", &strings);
	if (!result) {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}

	String type = strings.last_item();

	if (type == "vs") {
		*shader_type = VERTEX_SHADER;
	} else if (type == "gs") {
		*shader_type = GEOMETRY_SHADER;
	} else if (type == "cs") {
		*shader_type = COMPUTE_SHADER;
	} else if (type == "hs") {
		*shader_type = HULL_SHADER;
	} else if (type == "ds") {
		*shader_type = DOMAIN_SHADER;
	} else if (type == "ps") {
		*shader_type = PIXEL_SHADER;
	} else {
		print("get_shader_type_from_file: can not extract shader type from {}.", file_name);
		return false;
	}
	return true;
}

FORCEINLINE HRESULT D3D11Reflect(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,_In_ SIZE_T SrcDataSize, _Out_ ID3D11ShaderReflection** ppReflector)
{
	return D3DReflect(pSrcData, SrcDataSize,IID_ID3D11ShaderReflection, (void**)ppReflector);
}

static void create_shader(u8 *byte_code, u32 byte_code_size, Shader_Type shader_type, Shader *shader)
{
	switch (shader_type) {
		case VERTEX_SHADER: {
			HR(directx11.device->CreateVertexShader((void *)byte_code, byte_code_size, NULL, &shader->vertex_shader));

			print("{}.hlsl: Vertex shader", shader->name);

			ID3D11ShaderReflection* pReflector = NULL;
			D3D11Reflect((void *)byte_code, byte_code_size, &pReflector);

			D3D11_SHADER_DESC shaderDesc;
			pReflector->GetDesc(&shaderDesc);
			u32 buffer_count = shaderDesc.ConstantBuffers;


			for (u32 i = 0; i < buffer_count; i++) {
				ID3D11ShaderReflectionConstantBuffer* cbuffer = pReflector->GetConstantBufferByIndex(i);
				D3D11_SHADER_BUFFER_DESC sbuffer;
				cbuffer->GetDesc(&sbuffer);

				String type;
				switch (sbuffer.Type)
				{
				case D3D11_CT_CBUFFER:
					type = "Consttanc buffer";
					break;
				case D3D11_CT_TBUFFER:
					type = "Texture buffer";
					break;
				case D3D11_CT_INTERFACE_POINTERS:
					type = "Interface buffer";
					break;
				case D3D11_CT_RESOURCE_BIND_INFO:
					type = "Bind buffer";
					break;
				default:
					break;
				}
				print("	Index [{}] buffer name [{}]", i, sbuffer.Name);

			}
			break;
		}
		case GEOMETRY_SHADER: {
			HR(directx11.device->CreateGeometryShader((void *)byte_code, byte_code_size, NULL, &shader->geometry_shader));
			break;
		}
		case COMPUTE_SHADER: {
			HR(directx11.device->CreateComputeShader((void *)byte_code, byte_code_size, NULL, &shader->compute_shader));
			break;
		}
		case HULL_SHADER: {
			HR(directx11.device->CreateHullShader((void *)byte_code, byte_code_size, NULL, &shader->hull_shader));
			break;
		}
		case DOMAIN_SHADER: {
			HR(directx11.device->CreateDomainShader((void *)byte_code, byte_code_size, NULL, &shader->domain_shader));
			break;
		}
		case PIXEL_SHADER: {
			HR(directx11.device->CreatePixelShader((void *)byte_code, byte_code_size, NULL, &shader->pixel_shader));

			print("{}.hlsl: Pixel Shader", shader->name);

			ID3D11ShaderReflection* pReflector = NULL;
			D3D11Reflect((void *)byte_code, byte_code_size, &pReflector);

			D3D11_SHADER_DESC shaderDesc;
			pReflector->GetDesc(&shaderDesc);
			u32 buffer_count = shaderDesc.ConstantBuffers;


			for (u32 i = 0; i < buffer_count; i++) {
				ID3D11ShaderReflectionConstantBuffer* cbuffer = pReflector->GetConstantBufferByIndex(i);
				D3D11_SHADER_BUFFER_DESC sbuffer;
				cbuffer->GetDesc(&sbuffer);
				
				String type;
				switch (sbuffer.Type)
				{
				case D3D11_CT_CBUFFER:
					type = "Consttanc buffer";
					break;
				case D3D11_CT_TBUFFER:
					type = "Texture buffer";
					break;
				case D3D11_CT_INTERFACE_POINTERS:
					type = "Interface buffer";
					break;
				case D3D11_CT_RESOURCE_BIND_INFO:
					type = "Bind buffer";
					break;
				default:
					break;
				}
				print("	Index [{}] buffer name [{}]", i, sbuffer.Name);
			}

			u32 bind_count = shaderDesc.BoundResources;
			for (u32 i = 0; i < bind_count; i++) {
				D3D11_SHADER_INPUT_BIND_DESC bind_desc;
				pReflector->GetResourceBindingDesc(i, &bind_desc);
				print("	Bind Resource: Name [{}]", bind_desc.Name);

			}
			break;
		}
	}
}

Shader::~Shader()
{
	free_com_object(vertex_shader);
	free_com_object(geometry_shader);
	free_com_object(compute_shader);
	free_com_object(hull_shader);
	free_com_object(domain_shader);
	free_com_object(pixel_shader);

	DELETE_PTR(byte_code);
}

void Shader_Manager::init()
{
	Array<String> file_names;
	
	String path_to_shader_dir;
	os_path.data_dir_paths.get("shader", path_to_shader_dir);

	bool success = get_file_names_from_dir(path_to_shader_dir + "\\", &file_names);
	if (file_names.is_empty() || !success) {
		error("Shader_Manamager::init: has not found compiled shader files.");
	}

	for (int i = 0; i < file_names.count; i++) {
		
		Shader_Type shader_type;
		if (!get_shader_type_from_file_name(file_names[i].c_str(), &shader_type)) {
			continue;
		}		
		
		String path_to_shader_file;
		os_path.build_full_path_to_shader_file(file_names[i], path_to_shader_file);

		
		int file_size;
		char *compiled_shader = read_entire_file(path_to_shader_file, "rb", &file_size);
		if (!compiled_shader) {
			continue;
		}
		
		String shader_name;		
		get_shader_name_from_file(file_names[i].c_str(), shader_name);

		
		Shader *existing_shader = NULL;
		if (shaders.get(shader_name, existing_shader)) {
			create_shader((u8 *)compiled_shader, file_size, shader_type, existing_shader);

			if (shader_type == VERTEX_SHADER) {
				existing_shader->byte_code = (u8 *)compiled_shader;
				existing_shader->byte_code_size = file_size;
			}
		} else {
			Shader *new_shader = new Shader();
			new_shader->name = shader_name;
			create_shader((u8 *)compiled_shader, file_size, shader_type, new_shader);
			
			if (shader_type == VERTEX_SHADER) {
				new_shader->byte_code = (u8 *)compiled_shader;
				new_shader->byte_code_size = file_size;
			}
			
			shaders.set(shader_name, new_shader);
		}

	}
}

