#include <d3dx11.h>
#include <windows.h>

#include "mesh.h"
#include "font.h"
#include "vertex.h"
#include "directx.h"
#include "render_system.h"
#include "../win32/win_types.h"
#include <string.h>

Font font;

void Font::init(int font_size)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		print("Font::init: Could not init FreeType Library");
	}

	FT_Face face;
	if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) {
		print("Font::init: Failed to load font");
	}

	FT_Set_Pixel_Sizes(face, 0, font_size);


	for (unsigned char c = 0; c < 128; c++) {
		
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			print("Font::init: Failed to load Char [{}] index [{}]", (char)c, (int)c);
			continue;
		}

		//if (!face->glyph->bitmap.buffer) {
		//	print("Font::init: Failed to load the face bitmap of char {} index {}", (char)c, (int)c);
		//	continue;
		//}
		if (c == ' ') {
			print("K");
		}



		Font_Char font_char;
		font_char.advance_y = face->glyph->advance.y;
		font_char.advance = face->glyph->advance.x;
		font_char.size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bearing = Size_u32(face->glyph->bitmap_left, face->glyph->bitmap_top);

		u32 *data = r8_to_rgba32((u8 *)face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
		font_char.bitmap = data;
		font_char.bitmap_size = Size_u32(face->glyph->bitmap.width, face->glyph->bitmap.rows);

		
		characters.set(c, font_char);
	}
}

u32 Font::get_text_width(const char *text)
{
	return u32();
}

Size_u32 Font::get_text_size(const char *text)
{
	u32 len = strlen(text);
	u32 max_height = 0;
	Size_u32 result;

	for (u32 i = 0; i < len; i++) {
		char c = text[i];
		Font_Char &font_char = characters[c];

		//if (font_char.size.height > max_height) {
		//	max_height = font_char.size.height;
		//	result.height = font_char.size.height;
		//}

		if (font_char.advance_y > max_height) {
			max_height = font_char.advance_y;
			result.height = font_char.advance_y >> 5;
		}
		
		//if (c == ' ') {
		//	result.width += font_char.advance >> 6;
		//}
		result.width += font_char.advance >> 6;
		//result.width += font_char.size.width;
	}
	
	return result;
}

inline float x_to_screen_space(float x)
{
	 return 2 * x / (win32.window_width - 0) - (win32.window_width + 0) / (win32.window_width - 0); 
}

inline float y_to_screen_space(float y)
{
	return 2 * y / (0 - win32.window_height) - (0 + win32.window_height) / (0 - win32.window_height); 
}


void draw_text(int x, int y, const char *text)
{

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
