#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include <stdint.h>
#include "gb.h"

typedef enum { DIR_DOWN = 0, DIR_UP = 1, DIR_LEFT = 2, DIR_RIGHT = 3 } Direction;
typedef enum { PSTATE_NOT_MOVING = 0, PSTATE_MOVING } PlayerState;
typedef enum { CONN_NORTH = 0, CONN_SOUTH, CONN_WEST, CONN_EAST } ConnDir;
typedef enum { ANIM_NONE, ANIM_FRAME_SWAP, ANIM_TILE_ROTATE } AnimType;

typedef struct { uint8_t* block_data; int width; int height; } MapBlk;
typedef struct { uint8_t tile_ids[TILES_PER_BLOCK]; } BlockDef;

typedef struct {
    Color* pixels;
    int tiles_wide, tiles_high, tex_w, tex_h;
    Texture2D texture;
} Tileset;

typedef struct {
    AnimType type;
    int target_tile;
    uint8_t* frame_data;
    int frame_size;
    int current_frame;
    int frame_count;
    int tick_counter;
    int direction;
    uint8_t* working_tile;
} TileAnim;

typedef struct {
    int tile_x, tile_y;
    int target_x, target_y;
    int pixel_offset;
    int walk_counter;
    int intra_frame;
    PlayerState state;
    Direction facing;
    int anim_frame;
    int forced_move_ticks;
    int hop_active;
    int hop_frames_remaining;
    int warp_cooldown;
    int stepped_from_warp;
    int grass_priority;
} Player;

typedef struct {
    int sprite_id;
    int global_object_id;  /* index into missable_objects[]; -1 if not missable */
    uint8_t move_type;
    uint8_t move_param;
    int text_id;
    char text_symbol[64];
    int active;
    int frozen;

    int tile_x, tile_y;
    int target_x, target_y;
    int pixel_offset;
    int walk_counter;
    int intra_frame;
    int anim_frame;
    int movement_status;
    Direction facing;
    int movement_delay;
    int grass_priority;
} NPC;

typedef enum { FADE_NONE = 0, FADE_OUT, FADE_IN } FadePhase;
typedef enum { FADE_PENDING_NONE = 0, FADE_PENDING_WARP } FadePending;

typedef struct {
    FadePhase phase;
    FadePending pending;
    int step;
    int ticks_in_step;
    int to_black;
    int warp_index;
    uint8_t current_bgp;
    int hide_world;
} PaletteFade;

typedef struct {
    ConnDir dir;
    char map_name[64];
    int offset;
    MapBlk map;
    int loaded;
} MapConnection;

typedef struct {
    int cell_x, cell_y;
    char dest_map[64];
    int dest_warp_id;
    int warp_type;
    int warp_dir;
} WarpEvent;

#define MAX_CONNECTIONS 4
#define MAX_WARPS       32
#define MAX_OBJECTS     16

typedef struct {
    char symbol[64];
    char label[64];
    char text[512];
} TextEntry;

#define MAX_TEXT_ENTRIES 64

typedef struct {
    TextEntry entries[MAX_TEXT_ENTRIES];
    int count;
} TextTable;

#define MAX_BG_EVENTS 8

typedef struct {
    int cell_x, cell_y;
    char text_symbol[64];
} BgEvent;

typedef struct {
    char name[64];
    char prev_map[64];
    char tileset_stem[64];
    MapBlk map;
    uint8_t border_block;

    Tileset tileset;
    BlockDef* blocks;
    int num_blocks;
    uint8_t* collision_list;
    int num_collision;

    MapConnection connections[MAX_CONNECTIONS];
    int num_connections;

    WarpEvent warps[MAX_WARPS];
    int num_warps;

    NPC npcs[MAX_OBJECTS];
    int num_npcs;

    int sgb_pals[4];

    TextTable texts;
    
    BgEvent bg_events[MAX_BG_EVENTS];
    int num_bg_events;
} ActiveMap;

typedef struct {
    const char* map_const_path;
    const char* headers_dir;
    const char* objects_dir;
    const char* maps_dir;
    const char* tilesets_dir;
    const char* blocksets_dir;
    const char* collision_path;
} MapPaths;

#define MAX_FLY_WARPS 32

typedef struct {
    char map_name[64];
    int x, y;
} FlyWarp;

typedef struct {
    FlyWarp entries[MAX_FLY_WARPS];
    int count;
} FlyWarpTable;

#endif
