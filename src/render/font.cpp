#include <ctype.h>
#include <windows.h>
#include <shlobj_core.h>

#include "font.h"
#include "render_helpers.h"
#include "../sys/engine.h"
#include "../win32/win_types.h"
#include "../libs/os/file.h"
#include "../libs/math/functions.h"

const char *DEFAULT_PATH_TO_FONT_DIR = "C:/Windows/Fonts/";

Font::Font()
{
	characters.reserve(MAX_CHARACTERS);
}

u32 Font::get_char_advance(char c)
{
	return characters[c].advance;
}

u32 Font::get_text_width(const char *text)
{
	Size_u32 size = get_text_size(text);
	return size.width;
}

Size_u32 Font::get_text_size(const char *text)
{
	assert(text);
	
	u32 len = (u32)strlen(text);
	u32 mines_one_len = len - 1;
	Size_u32 result = { 0, 0 };
	if (len == 0) {
		return result;
	}

	Font_Char *font_char = NULL;
	for (u32 index = 0; index < mines_one_len; index++) {
		font_char = get_font_char(text[index]);
		result.width += (font_char->advance);
		result.height = math::max(result.height, font_char->size.height);
	}
	font_char = get_font_char(text[mines_one_len]);
	result.width += font_char->bearing.width + font_char->size.width;
	result.height = math::max(result.height, font_char->size.height);
	
	return result;
}

Font_Char *Font::get_font_char(u8 character)
{
	return &characters[(u32)character];
}

Font_Char::~Font_Char()
{
	DELETE_PTR(bitmap);
}

Font_Char::Font_Char(const Font_Char &other)
{
	*this = other;
}

Font_Char &Font_Char::operator=(const Font_Char &other)
{
	if (this != &other) {
		character = other.character;
		advance = other.advance;
		size = other.size;
		bearing = other.bearing;
		
		if (other.bitmap) {
			DELETE_PTR(bitmap);
			bitmap = new u32[size.width * size.height];
			memcpy((void *)bitmap, (void *)other.bitmap, (size.width * size.height) * sizeof(u32));
		}
	}
	return *this;
}

u32 Font_Char::get_index()
{
	return (u32)character;
}

void Font_Manager::init()
{
	CHAR path[MAX_PATH];

	if (FAILED(SHGetFolderPath(NULL, CSIDL_FONTS, NULL, 0, path)))
	{
		if (!file_exists(DEFAULT_PATH_TO_FONT_DIR)) {
			print("Font_Manager::init: Faield to init Font Manager. The manager can not get path to a font directory");
		} else {
			path_to_font_dir = DEFAULT_PATH_TO_FONT_DIR;
		}
	} else {
		path_to_font_dir = path;
	}
}

bool Font_Manager::load_font(const char *name, u32 font_size)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		print("Font_Manager::load_font: Could not init FreeType Library.");
		return false;
	}

	FT_Face face;
	String full_path_to_font_file = path_to_font_dir + "\\" + name + ".ttf";
	if (FT_New_Face(ft, full_path_to_font_file, 0, &face)) {
		print("Font_Manager::load_font: Failed to load font {}.ttf.", name);
		return false;
	}

	face->num_fixed_sizes;
	if (FT_Set_Pixel_Sizes(face, 0, font_size)) {
		print("Font_Manager::load_font: Failed to load font {}.ttf with size {}.", name, font_size);
		return false;
	}

	char *font_name = format("{}_{}", name, font_size);
	Font font;
	font.name = font_name;
	font.font_size = font_size;

	for (u8 c = 0; c < MAX_CHARACTERS; c++) {

		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			print("Font::init: Failed to load Char [{}] index [{}]", (char)c, (int)c);
			continue;
		}

		Font_Char font_char;
		font_char.character = c;
		font_char.advance = face->glyph->advance.x >> 6;
		font_char.bearing = Size_u32(face->glyph->bitmap_left, face->glyph->bitmap_top);
		font_char.size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);

		u32 *data = r8_to_rgba32((u8 *)face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bitmap = data;

		if (font_char.size.width > font.max_width) {
			font.max_width = font_char.size.width;
		}

		if (font_char.size.height > font.max_height) {
			font.max_height = font_char.size.height;
		}

		if (isalpha(c) && (font_char.size.height > font.max_alphabet_height)) {
			font.max_alphabet_height = font_char.size.height;
		}

		font.characters[font_char.get_index()] = font_char;
	}
	font_table.set(font_name, font);
	free_string(font_name);
	
	Engine::get_render_system()->render_2d.get_render_font(&font);
	return true;
}

Font *Font_Manager::get_font(const char *name, u32 font_size)
{
	char *font_name = format("{}_{}", name, font_size);

	if (!font_table.key_in_table(font_name)) {
		if (!load_font(name, font_size)) {
			print("Font_Manager::get_font: Failed to get font '{}' wit size {}.", name, font_size);
			free_string(font_name);
			return NULL;
		}

	}
	Font *font = &font_table[font_name];
	free_string(font_name);
	return font;
}
