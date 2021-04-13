#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "file.h"
#include "../sys/sys_local.h"


const char DATA_DIR_NAME[] = "data";

Path os_path;


bool get_file_names_from_dir(const char *full_path, Array<char *> *file_names)
{
	full_path = concatenate_c_str(full_path, "*");

	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(full_path, &data);

	if (handle == INVALID_HANDLE_VALUE) {
		//@Note: may be here need to be warring message
		return false;
	}

	do {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		file_names->push(_strdup(data.cFileName));
	} while (FindNextFile(handle, &data));
	FindClose(handle);
	return true;
}

u8 read_u8(FILE *file)
{
	u8 byte;
	fread(&byte, sizeof(u8), 1, file);
	return byte;
}

u16 read_u16(FILE *file)
{
	u16 short_word;
	fread(&short_word, sizeof(u16), 1, file);
	return short_word;
}

u32 read_u32(FILE *file)
{
	u32 word;
	fread(&word, sizeof(u32), 1, file);
	return word;
}

u64 read_u64(FILE *file)
{
	u64 long_word;
	fread(&long_word, sizeof(u64), 1, file);
	return long_word;
}

s8 read_s8(FILE *file)
{
	s8 byte;
	fread(&byte, sizeof(s8), 1, file);
	return byte;
}

s16 read_s16(FILE *file)
{
	s16 short_word;
	fread(&short_word, sizeof(s16), 1, file);
	return short_word;
}

s32 read_s32(FILE *file)
{
	s32 word;
	fread(&word, sizeof(s32), 1, file);
	return word;
}

s64 read_s64(FILE *file)
{
	s64 long_word;
	fread(&long_word, sizeof(s64), 1, file);
	return long_word;
}

float read_real32(FILE *file)
{
	float real32;
	fread(&real32, sizeof(float), 1, file);
	return real32;
}

double read_real64(FILE *file)
{
	double real64;
	fread(&real64, sizeof(double), 1, file);
	return real64;
}

char *read_string(FILE *file, int len)
{
	char *buffer = new char[len + 1];
	fread(buffer, sizeof(u8), len, file);
	buffer[len] = '\0';
	return buffer;
}

char *read_entire_file(const char *name, const char *mode, int *file_size)
{
	FILE *f;
	if (fopen_s(&f, name, mode)) {
		print("Falied of reading file {}", name);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	if (file_size) *file_size = size;
	rewind(f);

	char *buffer = new char[size + 1];
	int result = fread(buffer, sizeof(char), size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

char *extract_file_extension(const char *file_path)
{
	Array<char *> buffer;
	split(const_cast<char *>(file_path), ".", &buffer);
	return _strdup(buffer[buffer.count - 1]);
}

char *extract_file_name(const char *file_path)
{
	Array<char *> buffer;
	split(const_cast<char *>(file_path), "\\", &buffer);
	return _strdup(buffer[buffer.count -1]);
}


void Path::init()
{
	init_base_path();

	char *texture_dir = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "textures");
	char *shader_dir  = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "shader");
	char *model_dir   = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "models");
	
	data_dir_paths.set("texture", texture_dir);
	data_dir_paths.set("shader", shader_dir);
	data_dir_paths.set("model", model_dir);

	free_string(texture_dir);
	free_string(shader_dir);
	free_string(model_dir);
}

void Path::init_base_path()
{
	char buffer[512];
	GetCurrentDirectory(512, buffer);
	String current_path = (const char *)&buffer;

	Array<String> dir_names;
	split(&current_path, "\\", &dir_names);

	dir_names.last_item().to_lower();
	if (dir_names.last_item() != "hades" || dir_names.last_item() != "hades-engine") {
		bool succeed = build_correct_base_path(&dir_names, &base_path);
		if (!succeed) {
			error("Base path can't be built correct");
		}
	}
}

bool Path::build_correct_base_path(Array<String> *splited_wrong_path, String *base_path)
{
	assert(base_path->data == NULL);

	String *dir = NULL;
	FOR((*splited_wrong_path), dir) {
		dir->to_lower();
		if (*dir == "hades" || *dir == "hades-engine") {
			base_path->append(dir);
			return true;
		} else {
			base_path->append(dir);
			base_path->append("\\");
		}
	}
	return false;
}
