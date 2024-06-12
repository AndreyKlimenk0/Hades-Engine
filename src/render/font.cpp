#include <ctype.h>
#include <windows.h>
#include <shlobj_core.h>

#include "font.h"
#include "../sys/sys.h"
#include "../sys/utils.h"
#include "../libs/os/file.h"

static const char *DEFAULT_PATH_TO_FONT_DIR = "C:/Windows/Fonts/";

inline u32 *r8_to_rgba32(u8 *data, u32 width, u32 height)
{
	u32 *new_data = new u32[width * height];

	u8 *pixels = (u8 *)new_data;
	for (u32 row = 0; row < height; row++) {
		u32 row_start = row * (width * sizeof(u32));
		u32 row_2 = row * (width * sizeof(u8));

		for (u32 col = 0; col < width; col++) {
			u32 col_start = col * 4;
			if (data[row_2 + col] > 0) {
				pixels[row_start + col_start + 0] = 255;
				pixels[row_start + col_start + 1] = 255;
				pixels[row_start + col_start + 2] = 255;
				pixels[row_start + col_start + 3] = data[row_2 + col];
			} else {
				pixels[row_start + col_start + 0] = 255;
				pixels[row_start + col_start + 1] = 0;
				pixels[row_start + col_start + 2] = 0;
				pixels[row_start + col_start + 3] = 0;
			}
		}
	}
	return new_data;
}

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

Size_u32 Font::get_text_size(const char *text, Text_Alignment text_alignment)
{
	assert(text);

	u32 len = (u32)strlen(text);
	u32 mines_one_len = len - 1;
	Size_u32 text_size = { 0, 0 };
	if (len == 0) {
		return text_size;
	}

	Font_Char *font_char = NULL;
	for (u32 index = 0; index < mines_one_len; index++) {
		font_char = get_font_char(text[index]);
		text_size.width += (font_char->advance);
		text_size.height = math::max(text_size.height, font_char->size.height);
	}
	font_char = get_font_char(text[mines_one_len]);
	text_size.width += font_char->bearing.width + font_char->size.width;
	text_size.height = math::max(text_size.height, font_char->size.height);

	switch (text_alignment) {
		case ALIGN_TEXT_BY_MAX_NUMBER: {
			text_size.height = max_number_height;
			break;
		}
		case ALIGN_TEXT_BY_MAX_ALPHABET: {
			text_size.height = max_alphabet_height;
			break;
		}
		case ALIGN_TEXT_BY_MAX_SYMBOL: {
			text_size.height = max_symbol_height;
			break;
		}
	}
	return text_size;
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

	if (FAILED(SHGetFolderPath(NULL, CSIDL_FONTS, NULL, 0, path))) {
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

		font.characters[font_char.get_index()] = font_char;

		font.max_symbol_height = math::max(font.max_symbol_height, font_char.size.height);

		if (isalpha(c)) {
			font.max_alphabet_height = math::max(font.max_alphabet_height, font_char.size.height);
		}
		if (isdigit(c)) {
			font.max_number_height = math::max(font.max_alphabet_height, font_char.size.height);
		}
	}
	font_table.set(font_name, font);
	free_string(font_name);

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
