#include <ctype.h>
#include <windows.h>
#include <shlobj_core.h>

#include "font.h"
#include "../libs/os/file.h"
#include "../win32/win_types.h"
#include "render_helpers.h"
#include "../sys/engine.h"


const char *DEFAULT_PATH_TO_FONT_DIR = "C:/Windows/Fonts/";


Font::Font()
{
	characters.reserve(MAX_CHARACTERS);
}

u32 Font::get_char_bearing(char c) {
	return characters[c].bearing.width;
}

u32 Font::get_char_width(char c)
{
	return characters[c].advance_x >> 6;
}

u32 Font::get_char_advance(char c)
{
	return characters[c].advance_x >> 6;
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
	u32 max_height = 0;
	Size_u32 result = { 0, 0 };

	for (u32 index = 0; index < len; index++) {
		u8 c = text[index];
		Font_Char *font_char = get_font_char(c);

		if (index == (len - 1)) {
			result.width += font_char->bearing.width + font_char->size.width;
		} else {
			result.width += (font_char->advance_x >> 6);
		}
		
		result.height = math::max(result.height, font_char->size.height);
		
	}
	
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
	character = other.character;
	advance_y = other.advance_y;
	advance_x = other.advance_x;
	size = other.size;
	bearing = other.bearing;
	bitmap_size = other.bitmap_size;

	bitmap = new u32[size.width * size.height];
	memcpy((void *)bitmap, (void *)other.bitmap, (other.bitmap_size.width * other.bitmap_size.height) * sizeof(u32));
}

void Font_Char::operator=(const Font_Char &other)
{
	character = other.character;
	advance_y = other.advance_y;
	advance_x = other.advance_x;
	size = other.size;
	bearing = other.bearing;
	bitmap_size = other.bitmap_size;

	bitmap = new u32[size.width * size.height];
	memcpy((void *)bitmap, (void *)other.bitmap, (other.bitmap_size.width * other.bitmap_size.height) * sizeof(u32));
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
		font_char.advance_y = face->glyph->advance.y;
		font_char.advance_x = face->glyph->advance.x;
		font_char.size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bearing = Size_u32(face->glyph->bitmap_left, face->glyph->bitmap_top);

		u32 *data = r8_to_rgba32((u8 *)face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bitmap = data;
		font_char.bitmap_size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);

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
