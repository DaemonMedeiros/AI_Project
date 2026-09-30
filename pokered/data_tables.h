#ifndef DATA_TABLES_H
#define DATA_TABLES_H

#include <stdint.h>
#include "types.h"

typedef struct { Direction dir; uint8_t standing_tile; uint8_t ledge_tile; } LedgeEntry;

typedef struct { const char *stem; const char *label; } CollisionLabel;
typedef struct { const char *from; const char *to; } TilesetAlias;
typedef struct {
    const char *stem;
    uint8_t warp_ids[8];
    uint8_t door_ids[8];
} WarpTileList;
typedef struct { const char *stem; uint8_t grass_tile; } GrassTileEntry;

extern const CollisionLabel collision_labels[];
extern const TilesetAlias tileset_aliases[];
extern const WarpTileList warp_tile_lists[];
extern const GrassTileEntry grass_tiles[];
extern const uint8_t carpet_down[];
extern const uint8_t carpet_up[];
extern const uint8_t carpet_left[];
extern const uint8_t carpet_right[];
#define NUM_LEDGE_TILES 8
extern const LedgeEntry ledge_tiles[NUM_LEDGE_TILES];
extern const char* npc_sprite_files[];
extern const uint8_t sprite_frames[6][4];
extern const int anim_table[4][4];
extern const int anim_flip[4][4];

#define NUM_NPC_SPRITES 73

const char* lookup_collision_label(const char* stem);
uint8_t grass_tile_for(const char* stem);
int list_has8(const uint8_t* ids, uint8_t tid);
int list_has_term(const uint8_t* ids, uint8_t tid);
int is_warp_tile(const char* stem, uint8_t tid);
int is_door_tile(const char* stem, uint8_t tid);
int sprite_id_from_name(const char* name);
int dir_dx(Direction d);
int dir_dy(Direction d);

#endif
