#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/ds/hash_table.h"
#include "../win32/win_types.h"


enum Texture_Type {
	TEXTURE1D,
	TEXTURE2D,
	TEXTURE3D,
};

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

	operator ID3D11ShaderResourceView*();
};

struct Texture_Manager {
	Texture_Manager() {}
	~Texture_Manager();

	Texture default_texture;
	
	Hash_Table<String, Texture *> textures;

	void init();
	Texture *get_texture(const char *texture_name);
};

extern Texture_Manager texture_manager;
#endif