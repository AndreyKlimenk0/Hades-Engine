#include <ctype.h>
#include <d3dx11.h>
#include <windows.h>

#include "font.h"
#include "vertex.h"
#include "render_system.h"
#include "../win32/win_types.h"
#include <string.h>

void Font::init(int font_size)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		print("Font::init: Could not init FreeType Library");
	}

	FT_Face face;
	if (FT_New_Face(ft, "C:/Windows/Fonts/consola.ttf", 0, &face)) {
		print("Font::init: Failed to load font");
	}

	FT_Set_Pixel_Sizes(face, 0, font_size);

	for (unsigned char c = 0; c < 128; c++) {
		
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			print("Font::init: Failed to load Char [{}] index [{}]", (char)c, (int)c);
			continue;
		}

		Font_Char font_char;
		font_char.advance_y = face->glyph->advance.y;
		font_char.advance = face->glyph->advance.x;
		font_char.size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bearing = Size_u32(face->glyph->bitmap_left, face->glyph->bitmap_top);

		u32 *data = r8_to_rgba32((u8 *)face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bitmap = data;
		font_char.bitmap_size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);

		if (font_char.size.width > max_width) {
			max_width = font_char.size.width;
		}

		if (font_char.size.height > max_height) {
			max_height = font_char.size.height;
		}

		if (isalpha(c) && (font_char.size.height > max_alphabet_height)) {
			max_alphabet_height = font_char.size.height;
		}
		
		characters.set(c, font_char);
	}
}

u32 Font::get_char_width(char c)
{
	return characters[c].size.width;
}

u32 Font::get_char_advance(char c)
{
	return characters[c].advance >> 6;
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
		char c = text[index];
		Font_Char font_char;
		if (!characters.get(c, &font_char)) {
			print("Font::get_text_size: Can't get font char '{}' from text '{}'.", c, text);
			return Size_u32(0, 0);
		}

		if (font_char.size.height > max_height) {
			max_height = font_char.size.height;
			result.height = font_char.size.height;
		}
		
		if ((index == 0) && (index == (len - 1))) {
			result.width += (font_char.advance >> 6) - (font_char.bearing.width * 2);
			break;
		}

		if ((index == 0) || (index == (len - 1))) {
			result.width += (font_char.advance >> 6) - font_char.bearing.width;
		} else {
			result.width += font_char.advance >> 6;
		}
	}
	
	return result;
}

Font_Char::~Font_Char()
{
	DELETE_PTR(bitmap);
}

Font_Char::Font_Char(const Font_Char &other)
{
	bitmap_size = other.bitmap_size;
	advance = other.advance;
	size = other.size;
	bearing = other.bearing;

	bitmap = new u32[size.width * size.height];
	memcpy((void *)bitmap, (void *)other.bitmap, (other.bitmap_size.width * other.bitmap_size.height) * sizeof(u32));
}

void Font_Char::operator=(const Font_Char &other)
{
	bitmap_size = other.bitmap_size;
	advance = other.advance;
	size = other.size;
	bearing = other.bearing;

	bitmap = new u32[size.width * size.height];
	memcpy((void *)bitmap, (void *)other.bitmap, (other.bitmap_size.width * other.bitmap_size.height) * sizeof(u32));
}
