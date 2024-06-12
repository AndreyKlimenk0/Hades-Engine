#ifndef LEVEL_H
#define LEVEL_H

#include "../game/world.h"
#include "../render/render_world.h"

void init_game_and_render_world_from_level(const char *level_name, Game_World *game_world, Render_World *render_world);
void save_game_and_render_world_in_level(const char *level_name, Game_World *game_world, Render_World *render_world);

#endif
