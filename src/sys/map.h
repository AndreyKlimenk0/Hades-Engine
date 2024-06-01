#ifndef MAP_SAVER_H
#define MAP_SAVER_H

#include "../game/world.h"
#include "../render/render_world.h"

void init_game_and_render_world_from_map(const char *map_name, Game_World *game_world, Render_World *render_world);
void save_game_and_render_world_in_map(const char *map_name, Game_World *game_world, Render_World *render_world);

#endif