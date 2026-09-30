#ifndef MAP_H
#define MAP_H

#include "types.h"

#define WARP_TYPE_NORMAL  0
#define WARP_TYPE_DOOR    1
#define WARP_TYPE_CARPET  2

#define MSTAT_READY       1
#define MSTAT_DELAYED     2
#define MSTAT_WALKING     3

int load_map(ActiveMap* am, const char* map_name,
    const char* map_const_path,
    const char* headers_dir,
    const char* objects_dir,
    const char* maps_dir,
    const char* tilesets_dir,
    const char* blocksets_dir,
    const char* collision_path);

void free_map_data(ActiveMap* am);
void free_map_gpu(ActiveMap* am);

int is_tile_passable(uint8_t tile_id, uint8_t* collision_list, int num_collision);
uint8_t tile_in_front_of_cell(ActiveMap* am, int cx, int cy);
int can_walk_tile(ActiveMap* am, int nx, int ny);

int npc_occupies(ActiveMap* am, int skip, int nx, int ny);
int player_occupies(Player* p, int nx, int ny);

int is_ledge_tile(ActiveMap* am, int standing_x, int standing_y,
    Direction d, int nx, int ny);

int find_warp_index(ActiveMap* am, int cx, int cy);
WarpEvent* find_warp(ActiveMap* am, int cx, int cy);
MapConnection* check_connection(ActiveMap* am, int nx, int ny);
void place_player_at_warp(ActiveMap* am, Player* p, WarpEvent* dst);
Direction choose_forced_dir(ActiveMap* am, int tx, int ty);

#endif
