#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/ds/hash_table.h"
#include "../libs/math/common.h"
#include "../win32/win_types.h"


enum Texture_Type {
	TEXTURE1D,
	TEXTURE2D,
	TEXTURE3D,
};

u32 *r8_to_rgba32(u8 *data, u32 width, u32 height);

struct Texture {
	Texture() {}
	~Texture();

	u32 width;
	u32 height;
	Texture_Type type;

	ID3D11Resource *texture = NULL;
	ID3D11ShaderResourceView *shader_resource = NULL;

	String name;

	void init(u32 width, u32 height);
	void set_color(const Color &color);
	void update(Rect_u32 *rect, void *data, u32 row_pitch = 0);

	operator ID3D11ShaderResourceView*();
};

struct Texture_Manager {
	Texture_Manager() {}
	~Texture_Manager();

	Texture default_texture;
	
	Hash_Table<String, Texture *> textures;
	Array<Texture *> rest_textures;

	void init();
	Texture *get_texture(const char *texture_name);
	Texture *create_texture(u32 width, u32 height, DXGI_FORMAT format, u32 mips_level = 1);
};

extern Texture_Manager texture_manager;
#endif