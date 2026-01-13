#ifndef LEVEL_H
#define LEVEL_H

#include "../game/world.h"
#include "../render/render_world.h"

bool load_level(const char *level_name, Game_World *game_world, Render_World *render_world);
void save_level(const char *level_name, Game_World *game_world, Render_World *render_world);

#endif
