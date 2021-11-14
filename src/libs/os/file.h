#ifndef FILE_H
#define FILE_H

#include <stdio.h>

#include "../../win32/win_types.h"
#include "../str.h"
#include "../ds/array.h"


void extract_file_name(const char *file_name, String &result);
void extract_file_extension(const char *file_name, String &result);

bool get_file_names_from_dir(const char *full_path, Array<String> *file_names);

u8  read_u8(FILE *file);
u16 read_u16(FILE *file);
u32 read_u32(FILE *file);
u64 read_u64(FILE *file);

s8  read_s8(FILE *file);
s16 read_s16(FILE *file);
s32 read_s32(FILE *file);
s64 read_s64(FILE *file);

float read_real32(FILE *file);
double read_real64(FILE *file);

char *read_string(FILE *file, int len);
char *read_entire_file(const char *name, const char *mode = "r", int *file_size = NULL);
#endif