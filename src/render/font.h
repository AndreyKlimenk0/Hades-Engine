#ifndef HADES_FONT_H
#define HADES_FONT_H

#include <d3d11.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"

const u32 MAX_CHARACTERS = 128;
const u32 CONTORL_CHARACTERS = 32;

struct Font_Char {
	Font_Char() {}
	Font_Char(const Font_Char &other);
	~Font_Char();

	u8 character;
	u32 *bitmap = NULL;
	u32 advance;
	Size_u32 bearing;
	Size_u32 size;


	Font_Char &operator=(const Font_Char &other);
	u32 get_index();
};

enum Text_Alignment {
	ALIGN_TEXT_BY_MAX_NUMBER,
	ALIGN_TEXT_BY_MAX_ALPHABET,
	ALIGN_TEXT_BY_MAX_SYMBOL,
	ALIGN_TEXT_BY_MAX_SYMBOL_IN_TEXT
};

struct Font {
	Font();
	String name;
	u32 font_size;

	u32 max_symbol_height = 0;
	u32 max_alphabet_height = 0;
	u32 max_number_height = 0;

	u32 bitmaps_width = 0;
	u32 bitmaps_height = 0;

	Array<Font_Char> characters;

	u32 get_char_advance(char c);
	u32 get_text_width(const char *text);
	Size_u32 get_text_size(const char *text, Text_Alignment text_alignment = ALIGN_TEXT_BY_MAX_SYMBOL_IN_TEXT);
	Font_Char *get_font_char(u8 character);
};

struct Font_Manager {
	String path_to_font_dir;
	Hash_Table<String, Font> font_table;

	void init();
	bool load_font(const char *name, u32 font_size);
	Font *get_font(const char *name, u32 font_size);
};
#endif
