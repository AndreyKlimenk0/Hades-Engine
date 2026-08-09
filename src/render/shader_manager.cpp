#include <stdlib.h>
#include <string.h>

#include "shader_manager.h"
#include "../sys/sys.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"

#pragma comment(lib, "dxcompiler.lib")

static const Shader_Entry SHADER_LIST[] = {
	{ "render_2d.hlsl",              "render_2d",               "-DDEBUG_SHADOWS" },
	{ "forward_light.hlsl",          "forward_light",           "-DDEBUG_SHADOWS" },
	{ "depth_map.hlsl",              "depth_map",               "-DDEBUG_SHADOWS" },
	{ "debug_cascaded_shadows.hlsl", "debug_cascaded_shadows",  "-DDEBUG_SHADOWS" },
	{ "draw_vertices.hlsl",          "draw_vertices",           "-DDEBUG_SHADOWS" },
	{ "silhouette.hlsl",             "silhouette",              "-DDEBUG_SHADOWS" },
	{ "outlining.hlsl",              "outlining",               "-DDEBUG_SHADOWS" },
	{ "downsample_hzb.hlsl",         "downsample_hzb",          "-DDEBUG_SHADOWS" },
	{ "ui_rendering.hlsl",           "ui_rendering",            "-DDEBUG_SHADOWS" },
	{ "culling.hlsl",                "culling",                 "-DDEBUG_SHADOWS" },
	{ "shadows_culling.hlsl",        "shadows_culling",         "-DDEBUG_SHADOWS" },
	{ "tile_frustum.hlsl",           "tile_frustum",            "-DDEBUG_SHADOWS" },
};

static const Shader_Type SHADER_TYPE_LIST[] = {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	COMPUTE_SHADER,
	HULL_SHADER,
	DOMAIN_SHADER,
	PIXEL_SHADER,
};

String to_string(Shader_Type shader_type)
{
	String result;
	switch (shader_type) {
		case VERTEX_SHADER: {
			result = "Vertex Shader";
			break;
		}
		case GEOMETRY_SHADER: {
			result = "Geometry Shader";
			break;
		}
		case COMPUTE_SHADER: {
			result = "Compute Shader";
			break;
		}
		case HULL_SHADER: {
			result = "Hull Shader";
			break;
		}
		case DOMAIN_SHADER: {
			result = "Domain Shader";
			break;
		}
		case PIXEL_SHADER: {
			result = "Pixel Shader";
			break;
		}
		default: {
			assert(false);
		}
	}
	return result;
}

String get_shader_prefix(Shader_Type shader_type)
{
	String result;
	switch (shader_type) {
		case VERTEX_SHADER: {
			result = "vs";
			break;
		}
		case GEOMETRY_SHADER: {
			result = "gs";
			break;
		}
		case COMPUTE_SHADER: {
			result = "cs";
			break;
		}
		case HULL_SHADER: {
			result = "hs";
			break;
		}
		case DOMAIN_SHADER: {
			result = "ds";
			break;
		}
		case PIXEL_SHADER: {
			result = "ps";
			break;
		}
		default: {
			assert(false);
		}
	}
	return result;
}

String get_shader_entry_point(Shader_Type shader_type)
{
	String result;
	switch (shader_type) {
		case VERTEX_SHADER: {
			result = "vs_main";
			break;
		}
		case GEOMETRY_SHADER: {
			result = "gs_main";
			break;
		}
		case COMPUTE_SHADER: {
			result = "cs_main";
			break;
		}
		case HULL_SHADER: {
			result = "hs_main";
			break;
		}
		case DOMAIN_SHADER: {
			result = "ds_main";
			break;
		}
		case PIXEL_SHADER: {
			result = "ps_main";
			break;
		}
		default: {
			assert(false);
		}
	}
	return result;
}

String get_shader_profile(Shader_Type shader_type)
{
	String result;
	String profile = "6_6";

	switch (shader_type) {
		case VERTEX_SHADER:
		{
			result = "vs_" + profile;
			break;
		}
		case GEOMETRY_SHADER:
		{
			result = "gs_" + profile;
			break;
		}
		case COMPUTE_SHADER:
		{
			result = "cs_" + profile;
			break;
		}
		case HULL_SHADER:
		{
			result = "hs_" + profile;
			break;
		}
		case DOMAIN_SHADER:
		{
			result = "ds_" + profile;
			break;
		}
		case PIXEL_SHADER:
		{
			result = "ps_" + profile;
			break;
		}
		default:
		{
			assert(false);
		}
	}
	return result;
}

Shader_Compiler::Shader_Compiler()
{
	HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
	HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));
	HR(utils->CreateDefaultIncludeHandler(&include_handler));
}

Shader_Compiler::~Shader_Compiler()
{
}

String build_pdb_shader_file_name(const char *shader_file_name, Shader_Type shader_type)
{
	String base_file_name;
	extract_base_file_name(shader_file_name, base_file_name);
	return base_file_name + "_" + get_shader_prefix(shader_type) + "." + "pdb";
}

String build_cso_shader_file_name(const char *shader_file_name, Shader_Type shader_type)
{
	String base_file_name;
	extract_base_file_name(shader_file_name, base_file_name);
	return base_file_name + "_" + get_shader_prefix(shader_type) + "." + "cso";
}

Shader_Compilation_Result Shader_Compiler::compile(void *shader_code, u32 code_size, const char *path_to_shader_file, const char *file_name, Shader_Type shader_type)
{
	shader_source_directory = join_paths(get_base_path(), "hlsl");
	shader_pdb_directory = join_paths(join_paths(get_full_path_to_data_directory(), "shaders"), "pdb");
	cso_directory = join_paths(get_full_path_to_data_directory(), "shaders");

	print("Shader_Compiler::compile: Compiling {} as {}.", file_name, to_string(shader_type));

	ComPtr<IDxcBlobEncoding> shader_blob;
	HR(utils->CreateBlob(shader_code, code_size, 0, shader_blob.ReleaseAndGetAddressOf()));

	DxcBuffer buffer = { shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), DXC_CP_ACP };

	Array<String> arguments;
	arguments.push(path_to_shader_file);
	arguments.push("-Zi");  // Enable debug information
	arguments.push("-Od");  // Disable optimization;
	arguments.push("-Zpr"); // Row major matrix order
	arguments.push("-E");   // Entry point
	arguments.push(get_shader_entry_point(shader_type));
	arguments.push("-T");   // Profile
	arguments.push(get_shader_profile(shader_type));
	arguments.push("-HV");  // HLSL verison (2016, 2017, 2018, 2021). Default is 2018
	arguments.push("2021");
	arguments.push("-I");
	arguments.push(shader_source_directory);

	Array<LPCWSTR> wstrings;
	for (u32 i = 0; i < arguments.count; i++) {
		wstrings.push(static_cast<LPCWSTR>(to_wstring(arguments[i])));
	}

	ComPtr<IDxcResult> dxc_result;
	HR(compiler->Compile(&buffer, wstrings.items, wstrings.count, include_handler.Get(), IID_PPV_ARGS(dxc_result.ReleaseAndGetAddressOf())));

	HRESULT compilation_status = E_FAIL;
	HR(dxc_result->GetStatus(&compilation_status));

	Shader_Compilation_Result compilation_result;
	if (FAILED(compilation_status)) {
		ComPtr<IDxcBlobEncoding> error_buffer;
		HR(dxc_result->GetErrorBuffer(error_buffer.ReleaseAndGetAddressOf()));

		ComPtr<IDxcBlobUtf8> error_message;
		HR(utils->GetBlobAsUtf8(error_buffer.Get(), error_message.ReleaseAndGetAddressOf()));

		print("Shader_Compiler::compile: {} {} compilation failed.", file_name, to_string(shader_type));
		print((const char *)error_message->GetBufferPointer());
	} else {
		ComPtr<IDxcBlob> pdb;
		ComPtr<IDxcBlob> intermediate_code;

		HR(dxc_result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdb.ReleaseAndGetAddressOf()), NULL));
		HR(dxc_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(intermediate_code.ReleaseAndGetAddressOf()), NULL));

		File pdb_file;
		if (pdb_file.open(join_paths(shader_pdb_directory, build_pdb_shader_file_name(file_name, shader_type)), FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
			pdb_file.write(pdb->GetBufferPointer(), pdb->GetBufferSize());
		}

		File cso_file;
		if (cso_file.open(join_paths(cso_directory, build_cso_shader_file_name(file_name, shader_type)), FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
			cso_file.write(intermediate_code->GetBufferPointer(), intermediate_code->GetBufferSize());
		}

		compilation_result.compiled = true;
		compilation_result.last_write_time = cso_file.get_last_write_time();
		compilation_result.bytecode.size = intermediate_code->GetBufferSize();
		compilation_result.bytecode.data = new u8[intermediate_code->GetBufferSize()];
		memcpy(compilation_result.bytecode.data, intermediate_code->GetBufferPointer(), intermediate_code->GetBufferSize());

		print("Shader_Compiler::compile: {} as {} was successfully compiled.", file_name, to_string(shader_type));
	}

	return compilation_result;
}

inline bool compare(const Shader_Compilation_Info &x, const String &y)
{
	return x.cso_file_name == y;
}

void Shader_Manager::init()
{
	load_shader_compilation_info();

	for (u32 i = 0; i < ARRAY_SIZE(SHADER_LIST); i++) {
		Shader_Entry shader_entry = SHADER_LIST[i];

		String full_path_to_shader_file = join_paths(join_paths(get_base_path(), "hlsl"), shader_entry.shader_file_name);

		if (!file_exists(full_path_to_shader_file)) {
			print("Shader_System::init: Shader compilcation failed, {} was found.", shader_entry.shader_file_name);
			continue;
		}

		String shader_code = read_entire_file(full_path_to_shader_file, "rb");
		if (shader_code.is_empty()) {
			print("Shader_System::init: Faield to read {}", shader_entry.shader_file_name);
			continue;
		}

		for (u32 j = 0; j < ARRAY_SIZE(SHADER_TYPE_LIST); j++) {
			Shader_Type shader_type = SHADER_TYPE_LIST[j];

			if (shader_code.find(get_shader_entry_point(shader_type)) > -1) {

				Find_Result<Shader_Compilation_Info> result = find_in_array(shader_info_list, build_cso_shader_file_name(shader_entry.shader_file_name, shader_type), compare);
				if (result.found) {
					String path_to_cso_file = join_paths(join_paths(get_full_path_to_data_directory(), "shaders"), result.data.cso_file_name);
					String path_to_shader_file = join_paths(join_paths(get_base_path(), "hlsl"), result.data.shader_file_name);
					File cso_file;
					File shader_file;
					if (cso_file.open(path_to_cso_file, FILE_MODE_READ, FILE_OPEN_EXISTING) && shader_file.open(path_to_shader_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
						u64 cso_file_last_write_time = cso_file.get_last_write_time();
						u64 shader_file_last_write_time = shader_file.get_last_write_time();
						if (cso_file_last_write_time >= shader_file_last_write_time) {
							print("Shader_Compiler::init: {} as {} has already beed compiled. Loading the shader.", result.data.shader_file_name, to_string(shader_type));

							Shader_Data *shader_data = NULL;
							if (!shader_table.get(shader_entry.shader_alias, &shader_data)) {
								shader_data = new Shader_Data();
								shader_data->source_file = shader_entry.shader_file_name;
								shader_table.set(shader_entry.shader_alias, shader_data);
							}
							u8 *buffer = new u8[GetFileSize(cso_file.file_handle, NULL)];
							cso_file.read((void *)buffer, GetFileSize(cso_file.file_handle, NULL));
							shader_data->set_bytecode(shader_type, { buffer, GetFileSize(cso_file.file_handle, NULL) });
							continue;
						}
					}
				}

				Shader_Compilation_Result compilation_result = shader_compiler.compile((void *)shader_code.data, shader_code.len, full_path_to_shader_file, shader_entry.shader_file_name, shader_type);

				if (compilation_result.compiled) {
					Shader_Data *shader_data = NULL;
					if (!shader_table.get(shader_entry.shader_alias, &shader_data)) {
						shader_data = new Shader_Data();
						shader_data->source_file = shader_entry.shader_file_name;
						shader_table.set(shader_entry.shader_alias, shader_data);
					}
					shader_data->set_bytecode(shader_type, compilation_result.bytecode);


					Shader_Compilation_Info shader_info = { shader_entry.shader_file_name, shader_entry.shader_alias,
						build_cso_shader_file_name(shader_entry.shader_file_name, shader_type),  compilation_result.last_write_time, shader_entry.arguments };
					shader_info_list.push(shader_info);

					//update_shader_compilation_info(compilation_result.last_write_time, &shader_entry, shader_type);
				}
			}
		}
	}

	String buffer;
	for (u32 i = 0; i < shader_info_list.count; i++) {
		Shader_Compilation_Info *shader_info = &shader_info_list[i];
		char *str = format("{} {} {} {} {}\n", shader_info->shader_file_name, shader_info->shader_alias, shader_info->cso_file_name, shader_info->cso_file_last_write_time, shader_info->shader_compilation_args);
		buffer.append(str);
		free_string(str);
	}

	String path = join_paths(get_full_path_to_data_directory(), "shader_compilation_info.txt");
	File shader_compilation_info_file;
	if (shader_compilation_info_file.open(path, FILE_MODE_WRITE, FILE_OPEN_ALWAYS)) {
		shader_compilation_info_file.write((void *)buffer.data, buffer.len);
	}
}

void free_bytecode(Shader_Bytecode *shader_bytecode)
{
	shader_bytecode->size = 0;
	DELETE_ARRAY(shader_bytecode->data);
}

void Shader_Manager::shutdown()
{
	for (u32 i = 0; i < shader_table.count; i++) {
		Shader_Data *shader = shader_table.get_value(i);
		free_bytecode(&shader->vs_bytecode);
		free_bytecode(&shader->gs_bytecode);
		free_bytecode(&shader->cs_bytecode);
		free_bytecode(&shader->hs_bytecode);
		free_bytecode(&shader->ds_bytecode);
		free_bytecode(&shader->ps_bytecode);
		DELETE_PTR(shader);
	}
}

void Shader_Manager::load_shader_compilation_info()
{
	String path = join_paths(get_full_path_to_data_directory(), "shader_compilation_info.txt");

	String buffer = read_entire_file(path, "rb");
	char *text = buffer.data;
	//buffer.reset();
	while (true) {
		char *line = get_next_line(&text);
		if (!line) {
			break;
		}
		Array<String> result;
		String temp = line;
		split(&temp, " ", &result);
		int x = 0;
		Shader_Compilation_Info shader_info = { result[0], result[1], result[2], static_cast<u64>(atoll(result[3])), result[4] };
		shader_info_list.push(shader_info);
	}
	//free_string(text);
	int xi = 0;
	//File shader_compilation_info_file;
	//if (shader_compilation_info_file.open(path, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
	//	u8 *buffer = NULL;
	//	u32 data_size = 0;
	//	shader_compilation_info_file.read((void *)buffer, data_size);
	//}
}

void Shader_Manager::update_shader_compilation_info(u64 cso_file_last_write_time, Shader_Entry *shader_entry, Shader_Type shader_type)
{
	String path = join_paths(get_full_path_to_data_directory(), "shader_compilation_info.txt");

	File shader_compilation_info_file;
	if (shader_compilation_info_file.open(path, FILE_MODE_WRITE, FILE_OPEN_ALWAYS)) {
		char *str = format("{} {}\n", build_cso_shader_file_name(shader_entry->shader_file_name, shader_type), cso_file_last_write_time);

		shader_compilation_info_file.write((void *)str, strlen(str));
		shader_compilation_info_file.write((void *)str, strlen(str));

		free_string(str);
	}
}

Shader_Bytecode Shader_Manager::get_shader_bytecode(const char *shader_alias, Shader_Type shader_type)
{

	return shader_table[shader_alias]->get_bytecode(shader_type);
}

void Shader_Data::set_bytecode(Shader_Type shader_type, Shader_Bytecode bytecode)
{
	switch (shader_type) {
		case VERTEX_SHADER: {
			vs_bytecode = bytecode;
			break;
		}
		case GEOMETRY_SHADER: {
			gs_bytecode = bytecode;
			break;
		}
		case COMPUTE_SHADER: {
			cs_bytecode = bytecode;
			break;
		}
		case HULL_SHADER: {
			hs_bytecode = bytecode;
			break;
		}
		case DOMAIN_SHADER: {
			ds_bytecode = bytecode;
			break;
		}
		case PIXEL_SHADER: {
			ps_bytecode = bytecode;
			break;
		}
		default: {
			assert(false);
		}
	}
}

Shader_Bytecode Shader_Data::get_bytecode(Shader_Type shader_type)
{
	switch (shader_type) {
		case VERTEX_SHADER:
			return vs_bytecode;
		case GEOMETRY_SHADER:
			return gs_bytecode;
		case COMPUTE_SHADER:
			return cs_bytecode;
		case HULL_SHADER:
			return hs_bytecode;
		case DOMAIN_SHADER:
			return ds_bytecode;
		case PIXEL_SHADER:
			return ps_bytecode;
		default:
			assert(false);
	}
	return Shader_Bytecode{};
}
