#ifndef EFFECT_H
#define EFFECT_H

#include <d3dx11effect.h>
#include "../render/base.h"
#include "../libs/ds/hash_table.h"

Hash_Table<const char *, ID3DX11Effect *> *get_fx_shaders(const Direct3D *direct3d);

#endif