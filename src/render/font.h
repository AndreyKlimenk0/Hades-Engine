#ifndef HADES_FONT_H
#define HADES_FONT_H

#include <d3d11.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../sys/sys_local.h"
#include "../libs/math/vector.h"
#include "../libs/ds/hash_table.h"


struct Character {
	u32 advance;
	Vector2 size;
	Vector2 bearing;
	ID3D11ShaderResourceView *texture = NULL;
};

struct Font {
	Font() : characters(128) {}
	
	FT_Library lib;
	FT_Face face;
	Hash_Table<char, Character *> characters;
	
	void init(int font_size);
};

extern Font font;

void draw_text(int x, int y, const char *text);
#endif