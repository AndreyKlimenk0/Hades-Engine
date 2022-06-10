#ifndef HADES_FONT_H
#define HADES_FONT_H

#include <d3d11.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../sys/sys_local.h"
#include "../libs/math/vector.h"
#include "../libs/ds/hash_table.h"
#include "../win32/win_types.h"
#include "../libs/math/common.h"


struct Font_Char {
	Font_Char() {}
	Font_Char(const Font_Char &other);
	~Font_Char();
	
	u32 *bitmap = NULL;
	u32 advance_y;
	u32 advance;
	Size_u32 size;
	Size_u32 bearing;
	Size_u32 bitmap_size;
	
	void operator=(const Font_Char &other);
};

struct Font {
	Font() : characters(128) {}

	u32 max_height = 0;
	u32 max_width  = 0;
	u32 max_alphabet_height = 0;

	u32 bitmaps_width = 0;
	u32 bitmaps_height = 0;
	
	FT_Library lib;
	FT_Face face;
	Hash_Table<char, Font_Char> characters;
	
	void init(int font_size);
	u32 get_char_width(char c);
	u32 get_text_width(const char *text);
	Size_u32 get_text_size(const char *text);
};

extern Font font;

void draw_text(int x, int y, const char *text);
#endif