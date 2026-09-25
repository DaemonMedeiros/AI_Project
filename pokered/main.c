#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------------
   Constants
   ------------------------------------------------------------------------- */

#define TILE_SIZE           8
#define TILE_PIXEL_SIZE     16
#define BLOCK_SIZE          4
#define BLOCK_PIXEL_SIZE    (TILE_SIZE * BLOCK_SIZE)
#define TILES_PER_BLOCK     16
#define BLOCK_ENTRY_SIZE    16
#define TILESET_TILES_WIDE  16

#define GB_WIDTH            160
#define GB_HEIGHT           144
#define SCALE               2
#define BORDER_MARGIN       8

#define MAX_CONNECTIONS     4
#define MAX_WARPS           32
#define MAX_OBJECTS         16
#define NUM_NPC_SPRITES     73

#define WALK_FRAME_TICKS    4
#define WALK_STEP_FRAMES    8
#define NPC_STEP_FRAMES     16
#define HOP_STEP_FRAMES     16
#define HOP_PEAK_PIXELS     8

#define FADE_STEP_TICKS     4

#define REG_BGP             0xE4
#define REG_OBP0            0xD0
#define REG_OBP1            0xC0

#define USE_SGB_PALETTES    1

#define REPO_ROOT           "pokered"

#define MOVE_WALK           0xFE
#define MOVE_STAY           0xFF
#define MOVE_ANY_DIR        0x00
#define MOVE_UP_DOWN        0x01
#define MOVE_LEFT_RIGHT     0x02
#define MOVE_DIR_DOWN       0xD0
#define MOVE_DIR_UP         0xD1
#define MOVE_DIR_LEFT       0xD2
#define MOVE_DIR_RIGHT      0xD3
#define MOVE_DIR_NONE       0xFF

#define MSTAT_READY         1
#define MSTAT_DELAYED       2
#define MSTAT_WALKING       3

#define WARP_TYPE_NORMAL    0
#define WARP_TYPE_DOOR      1
#define WARP_TYPE_CARPET    2

/* -------------------------------------------------------------------------
   Basic types
   ------------------------------------------------------------------------- */

typedef enum { DIR_DOWN = 0, DIR_UP = 1, DIR_LEFT = 2, DIR_RIGHT = 3 } Direction;
typedef enum { PSTATE_NOT_MOVING = 0, PSTATE_MOVING } PlayerState;
typedef enum { CONN_NORTH = 0, CONN_SOUTH, CONN_WEST, CONN_EAST } ConnDir;
typedef enum { ANIM_NONE, ANIM_FRAME_SWAP, ANIM_TILE_ROTATE } AnimType;

typedef struct { uint8_t *block_data; int width; int height; } MapBlk;
typedef struct { uint8_t tile_ids[TILES_PER_BLOCK]; } BlockDef;

typedef struct {
    Color *pixels;
    int tiles_wide, tiles_high, tex_w, tex_h;
    Texture2D texture;
} Tileset;

typedef struct {
    AnimType type;
    int target_tile;
    uint8_t *frame_data;
    int frame_size;
    int current_frame;
    int frame_count;
    int tick_counter;
    int direction;
    uint8_t *working_tile;
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
    uint8_t movement_byte1;
    uint8_t movement_byte2;
    int active;

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

typedef enum {
    FADE_NONE = 0,
    FADE_OUT,
    FADE_IN,
} FadePhase;

typedef enum {
    FADE_PENDING_NONE = 0,
    FADE_PENDING_WARP,
} FadePending;

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

static const uint8_t fade_pal[8][3] = {
    {0xFF, 0xFF, 0xFF},
    {0xFE, 0xFE, 0xF8},
    {0xF9, 0xE4, 0xE4},
    {0xE4, 0xD0, 0xC0},
    {0xE4, 0xD0, 0xC0},
    {0x90, 0x80, 0x90},
    {0x40, 0x40, 0x40},
    {0x00, 0x00, 0x00},
};

static const int fade_out_black_seq[4] = {3, 2, 1, 0};
static const int fade_in_black_seq[4]  = {0, 1, 2, 3};
static const int fade_out_white_seq[3] = {5, 6, 7};
static const int fade_in_white_seq[3]  = {6, 5, 3};

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

typedef struct {
    char name[64];
    char prev_map[64];
    char tileset_stem[64];
    MapBlk map;
    uint8_t border_block;

    Tileset tileset;
    BlockDef *blocks;
    int num_blocks;
    uint8_t *collision_list;
    int num_collision;

    MapConnection connections[MAX_CONNECTIONS];
    int num_connections;

    WarpEvent warps[MAX_WARPS];
    int num_warps;

    NPC npcs[MAX_OBJECTS];
    int num_npcs;

    int sgb_pals[4];
} ActiveMap;

/* -------------------------------------------------------------------------
   Sprite frame tables
   ------------------------------------------------------------------------- */

static const uint8_t sprite_frames[6][4] = {
    {0x00, 0x01, 0x02, 0x03},
    {0x80, 0x81, 0x82, 0x83},
    {0x04, 0x05, 0x06, 0x07},
    {0x84, 0x85, 0x86, 0x87},
    {0x08, 0x09, 0x0a, 0x0b},
    {0x88, 0x89, 0x8a, 0x8b},
};

static const int anim_table[4][4] = {
    {0, 1, 0, 1},
    {2, 3, 2, 3},
    {4, 5, 4, 5},
    {4, 5, 4, 5},
};

static const int anim_flip[4][4] = {
    {0, 0, 0, 1},
    {0, 0, 0, 1},
    {0, 0, 0, 0},
    {1, 1, 1, 1},
};

/* -------------------------------------------------------------------------
   SGB palettes
   ------------------------------------------------------------------------- */

static const uint8_t dmg_shade[4] = {255, 170, 85, 0};

typedef struct { uint16_t colors[4]; } SGBPalette;

typedef enum {
    PAL_ROUTE, PAL_PALLET, PAL_VIRIDIAN, PAL_PEWTER, PAL_CERULEAN,
    PAL_LAVENDER, PAL_VERMILION, PAL_CELADON, PAL_FUCHSIA, PAL_CINNABAR,
    PAL_INDIGO, PAL_SAFFRON, PAL_TOWNMAP, PAL_LOGO1, PAL_LOGO2, PAL_0F,
    PAL_MEWMON, PAL_BLUEMON, PAL_REDMON, PAL_CYANMON, PAL_PURPLEMON,
    PAL_BROWNMON, PAL_GREENMON, PAL_PINKMON, PAL_YELLOWMON, PAL_GRAYMON,
    PAL_SLOTS1, PAL_SLOTS2, PAL_SLOTS3, PAL_SLOTS4, PAL_BLACK,
    PAL_GREENBAR, PAL_YELLOWBAR, PAL_REDBAR, PAL_BADGE, PAL_CAVE,
    PAL_GAMEFREAK,
    NUM_SGB_PALS
} SGBPaletteID;

static const SGBPalette sgb_super_palettes[NUM_SGB_PALS] = {
    {{0x7FBF, 0x2F95, 0x7E94, 0x0843}},
    {{0x7FBF, 0x6F99, 0x7F54, 0x0843}},
    {{0x7FBF, 0x0F11, 0x7F54, 0x0843}},
    {{0x7FBF, 0x43D7, 0x7F54, 0x0843}},
    {{0x7FBF, 0x7A91, 0x7F54, 0x0843}},
    {{0x7FBF, 0x6E1B, 0x7F54, 0x0843}},
    {{0x7FBF, 0x025E, 0x7F54, 0x0843}},
    {{0x7FBF, 0x5BF0, 0x7F54, 0x0843}},
    {{0x7FBF, 0x59FF, 0x7F54, 0x0843}},
    {{0x7FBF, 0x195A, 0x7F54, 0x0843}},
    {{0x7FBF, 0x6356, 0x7F54, 0x0843}},
    {{0x7FBF, 0x0F3B, 0x7F54, 0x0843}},
    {{0x7FBF, 0x7F54, 0x2FEA, 0x0843}},
    {{0x7FBF, 0x7FD1, 0x2FEA, 0x1015}},
    {{0x7FBF, 0x7FD1, 0x4B32, 0x3B03}},
    {{0x7FBF, 0x673B, 0x5A6B, 0x0843}},
    {{0x7FBF, 0x6337, 0x3B77, 0x0843}},
    {{0x7FBF, 0x5333, 0x4B4E, 0x0843}},
    {{0x7FBF, 0x7F4A, 0x6A46, 0x0843}},
    {{0x7FBF, 0x5B3E, 0x4F37, 0x0843}},
    {{0x7FBF, 0x6BB8, 0x5B5C, 0x0843}},
    {{0x7FBF, 0x6758, 0x4F49, 0x0843}},
    {{0x7FBF, 0x56F4, 0x4F4A, 0x0843}},
    {{0x7FBF, 0x6BB8, 0x6BD6, 0x0843}},
    {{0x7FBF, 0x7F5E, 0x6A80, 0x0843}},
    {{0x7FBF, 0x6AD7, 0x3F3B, 0x0843}},
    {{0x7FBF, 0x6AD7, 0x6B8A, 0x0843}},
    {{0x7FBF, 0x7F51, 0x6758, 0x0843}},
    {{0x7FBF, 0x5FB1, 0x6758, 0x0843}},
    {{0x7FBF, 0x4F5D, 0x6758, 0x0843}},
    {{0x7FBF, 0x3F39, 0x1843, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x4F4A, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x6A80, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x6A46, 0x0843}},
    {{0x7FBF, 0x6337, 0x4B4E, 0x0843}},
    {{0x7FBF, 0x5B49, 0x5F36, 0x0843}},
    {{0x7FBF, 0x7F5E, 0x5F49, 0x0843}},
};

static Color sgb_color_to_rgba(uint16_t c) {
    Color out;
    out.r = (uint8_t)(((c & 0x1F) * 255) / 31);
    out.g = (uint8_t)((((c >> 5) & 0x1F) * 255) / 31);
    out.b = (uint8_t)((((c >> 10) & 0x1F) * 255) / 31);
    out.a = 255;
    return out;
}

typedef struct { const char *map_name; SGBPaletteID pals[4]; } MapSGBPalettes;

static const MapSGBPalettes map_sgb_palettes[] = {
    {"PALLETTOWN",     {PAL_PALLET,    PAL_PALLET,    PAL_PALLET,    PAL_PALLET}},
    {"PALLET_TOWN",    {PAL_PALLET,    PAL_PALLET,    PAL_PALLET,    PAL_PALLET}},
    {"VIRIDIANCITY",   {PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN}},
    {"VIRIDIAN_CITY",  {PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN}},
    {"PEWTERCITY",     {PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER}},
    {"PEWTER_CITY",    {PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER}},
    {"CERULEANCITY",   {PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN}},
    {"CERULEAN_CITY",  {PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN}},
    {"LAVENDERTOWN",   {PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER}},
    {"LAVENDER_TOWN",  {PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER}},
    {"VERMILIONCITY",  {PAL_VERMILION, PAL_VERMILION, PAL_VERMILION, PAL_VERMILION}},
    {"VERMILION_CITY", {PAL_VERMILION, PAL_VERMILION, PAL_VERMILION, PAL_VERMILION}},
    {"CELADONCITY",    {PAL_CELADON,   PAL_CELADON,   PAL_CELADON,   PAL_CELADON}},
    {"CELADON_CITY",   {PAL_CELADON,   PAL_CELADON,   PAL_CELADON,   PAL_CELADON}},
    {"FUCHSIACITY",    {PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA}},
    {"FUCHSIA_CITY",   {PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA}},
    {"CINNABARISLAND", {PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR}},
    {"CINNABAR_ISLAND",{PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR}},
    {"INDIGOPLATEAU",  {PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO}},
    {"INDIGO_PLATEAU", {PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO}},
    {"SAFFRONCITY",    {PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON}},
    {"SAFFRON_CITY",   {PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON}},
    {"ROUTE1",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_1",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE2",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_2",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE3",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_3",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {NULL, {0, 0, 0, 0}}
};

static int lookup_sgb_palette_ids(const char *map_name, int out[4]) {
    for (int i = 0; map_sgb_palettes[i].map_name; i++) {
        if (strcmp(map_sgb_palettes[i].map_name, map_name) == 0) {
            for (int j = 0; j < 4; j++) out[j] = (int)map_sgb_palettes[i].pals[j];
            return 1;
        }
    }
    for (int j = 0; j < 4; j++) out[j] = PAL_ROUTE;
    return 0;
}

/* -------------------------------------------------------------------------
   Data tables
   ------------------------------------------------------------------------- */

static const struct { const char *stem; const char *label; } collision_labels[] = {
    {"cavern",     "Cavern_Coll"},
    {"cemetery",   "Cemetery_Coll"},
    {"club",       "Club_Coll"},
    {"facility",   "Facility_Coll"},
    {"forest",     "Forest_Coll"},
    {"gate",       "Gate_Coll"},
    {"gym",        "Gym_Coll"},
    {"house",      "House_Coll"},
    {"interior",   "Interior_Coll"},
    {"lab",        "Lab_Coll"},
    {"lobby",      "Lobby_Coll"},
    {"mansion",    "Mansion_Coll"},
    {"overworld",  "Overworld_Coll"},
    {"plateau",    "Plateau_Coll"},
    {"pokecenter", "Pokecenter_Coll"},
    {"reds_house", "RedsHouse1_Coll"},
    {"ship",       "Ship_Coll"},
    {"ship_port",  "ShipPort_Coll"},
    {"underground","Underground_Coll"},
    {NULL, NULL}
};

static const struct { const char *from; const char *to; } tileset_aliases[] = {
    {"reds_house_1", "reds_house"},
    {"reds_house_2", "reds_house"},
    {"dojo", "gym"},
    {"mart", "pokecenter"},
    {"forest_gate", "gate"},
    {NULL, NULL}
};

static const struct {
    const char *stem;
    uint8_t warp_ids[8];
    uint8_t door_ids[8];
} warp_tile_lists[] = {
    {"overworld",  {0x1B, 0x58, 0},            {0x1B, 0x58, 0}},
    {"reds_house", {0x1A, 0x1C, 0},            {0}},
    {"mart",       {0x5E, 0},                  {0x5E, 0}},
    {"pokecenter", {0x5E, 0},                  {0x5E, 0}},
    {"forest",     {0x5A, 0x5C, 0x3A, 0},      {0x3A, 0}},
    {"gym",        {0x4A, 0},                  {0}},
    {"house",      {0x54, 0x5C, 0x32, 0},      {0x54, 0}},
    {"gate",       {0x3B, 0x1A, 0x1C, 0},      {0x3B, 0}},
    {"ship",       {0x37, 0x39, 0x1E, 0x4A, 0}, {0x1E, 0}},
    {"interior",   {0x15, 0x55, 0x04, 0},      {0}},
    {"cavern",     {0x18, 0x1A, 0x22, 0},      {0}},
    {"lobby",      {0x1A, 0x1C, 0x38, 0},      {0x1C, 0x38, 0x1A, 0}},
    {"mansion",    {0x1A, 0x1C, 0x53, 0},      {0x1A, 0x1C, 0x53, 0}},
    {"lab",        {0x34, 0},                  {0x34, 0}},
    {"facility",   {0x43, 0x58, 0x20, 0},      {0x43, 0x58, 0x1B, 0}},
    {"cemetery",   {0x1B, 0},                  {0}},
    {"underground",{0x13, 0},                  {0}},
    {"plateau",    {0x1B, 0x3B, 0},            {0x3B, 0x1B, 0}},
    {"club",       {0},                        {0}},
    {"ship_port",  {0},                        {0}},
    {NULL, {0}, {0}}
};

static const struct { const char *stem; uint8_t grass_tile; } grass_tiles[] = {
    {"overworld", 0x52},
    {"forest",    0x20},
    {"plateau",   0x45},
    {NULL, 0},
};

static const uint8_t carpet_down[]  = {0x01, 0x12, 0x17, 0x3D, 0x04, 0x18, 0x33, 0xFF};
static const uint8_t carpet_up[]    = {0x01, 0x5C, 0xFF};
static const uint8_t carpet_left[]  = {0x1A, 0x4B, 0xFF};
static const uint8_t carpet_right[] = {0x0F, 0x4E, 0xFF};

typedef struct {
    Direction dir;
    uint8_t standing_tile;
    uint8_t ledge_tile;
} LedgeEntry;

static const LedgeEntry ledge_tiles[] = {
    {DIR_DOWN,  0x2C, 0x37},
    {DIR_DOWN,  0x39, 0x36},
    {DIR_DOWN,  0x39, 0x37},
    {DIR_LEFT,  0x2C, 0x27},
    {DIR_LEFT,  0x39, 0x27},
    {DIR_RIGHT, 0x2C, 0x0D},
    {DIR_RIGHT, 0x2C, 0x1D},
    {DIR_RIGHT, 0x39, 0x0D},
};

static const char *npc_sprite_files[NUM_NPC_SPRITES] = {
    NULL, "red", "blue", "oak", "youngster", "monster", "cooltrainer_f",
    "cooltrainer_m", "little_girl", "bird", "middle_aged_man", "gambler",
    "super_nerd", "girl", "hiker", "beauty", "gentleman", "daisy", "biker",
    "sailor", "cook", "bike_shop_clerk", "mr_fuji", "giovanni", "rocket",
    "channeler", "waiter", "silph_worker_f", "middle_aged_woman",
    "brunette_girl", "lance", "scientist", "scientist", "rocker", "swimmer",
    "safari_zone_worker", "gym_guide", "gramps", "clerk", "fishing_guru",
    "granny", "nurse", "link_receptionist", "silph_president",
    "silph_worker_m", "warden", "captain", "fisher", "koga", "guard",
    "guard", "mom", "balding_guy", "little_boy", "gameboy_kid",
    "gameboy_kid", "fairy", "agatha", "bruno", "lorelei", "seel",
    "poke_ball", "fossil", "boulder", "paper", "pokedex", "clipboard",
    "snorlax", "old_amber", "old_amber", "gambler_asleep", "gambler_asleep",
    "gambler_asleep",
};

/* -------------------------------------------------------------------------
   Small helpers
   ------------------------------------------------------------------------- */

static void strip_underscores(char *s) {
    char *d = s;
    while (*s) { if (*s != '_') *d++ = *s; s++; }
    *d = 0;
}

static int dir_dx(Direction d) { return (d == DIR_LEFT) ? -1 : (d == DIR_RIGHT) ? 1 : 0; }
static int dir_dy(Direction d) { return (d == DIR_UP) ? -1 : (d == DIR_DOWN) ? 1 : 0; }

static int reg_shade(uint8_t reg, int index) { return (reg >> (index * 2)) & 3; }

static const char *lookup_collision_label(const char *stem) {
    for (int i = 0; collision_labels[i].stem; i++)
        if (strcmp(collision_labels[i].stem, stem) == 0)
            return collision_labels[i].label;
    return "Overworld_Coll";
}

static uint8_t grass_tile_for(const char *stem) {
    for (int i = 0; grass_tiles[i].stem; i++)
        if (strcmp(grass_tiles[i].stem, stem) == 0)
            return grass_tiles[i].grass_tile;
    return 0xFF;
}

static int list_has8(const uint8_t *ids, uint8_t tid) {
    for (int i = 0; ids[i] != 0 && i < 8; i++)
        if (ids[i] == tid) return 1;
    return 0;
}

static int list_has_term(const uint8_t *ids, uint8_t tid) {
    for (int i = 0; ids[i] != 0xFF; i++)
        if (ids[i] == tid) return 1;
    return 0;
}

static int is_warp_tile(const char *stem, uint8_t tid) {
    for (int i = 0; warp_tile_lists[i].stem; i++)
        if (strcmp(warp_tile_lists[i].stem, stem) == 0)
            return list_has8(warp_tile_lists[i].warp_ids, tid);
    return 0;
}

static int is_door_tile(const char *stem, uint8_t tid) {
    for (int i = 0; warp_tile_lists[i].stem; i++)
        if (strcmp(warp_tile_lists[i].stem, stem) == 0)
            return list_has8(warp_tile_lists[i].door_ids, tid);
    return 0;
}

static int sprite_id_from_name(const char *name) {
    static const char *names[] = {
        "SPRITE_NONE", "SPRITE_RED", "SPRITE_BLUE", "SPRITE_OAK",
        "SPRITE_YOUNGSTER", "SPRITE_MONSTER", "SPRITE_COOLTRAINER_F",
        "SPRITE_COOLTRAINER_M", "SPRITE_LITTLE_GIRL", "SPRITE_BIRD",
        "SPRITE_MIDDLE_AGED_MAN", "SPRITE_GAMBLER", "SPRITE_SUPER_NERD",
        "SPRITE_GIRL", "SPRITE_HIKER", "SPRITE_BEAUTY", "SPRITE_GENTLEMAN",
        "SPRITE_DAISY", "SPRITE_BIKER", "SPRITE_SAILOR", "SPRITE_COOK",
        "SPRITE_BIKE_SHOP_CLERK", "SPRITE_MR_FUJI", "SPRITE_GIOVANNI",
        "SPRITE_ROCKET", "SPRITE_CHANNELER", "SPRITE_WAITER",
        "SPRITE_SILPH_WORKER_F", "SPRITE_MIDDLE_AGED_WOMAN",
        "SPRITE_BRUNETTE_GIRL", "SPRITE_LANCE", "SPRITE_UNUSED_SCIENTIST",
        "SPRITE_SCIENTIST", "SPRITE_ROCKER", "SPRITE_SWIMMER",
        "SPRITE_SAFARI_ZONE_WORKER", "SPRITE_GYM_GUIDE", "SPRITE_GRAMPS",
        "SPRITE_CLERK", "SPRITE_FISHING_GURU", "SPRITE_GRANNY",
        "SPRITE_NURSE", "SPRITE_LINK_RECEPTIONIST", "SPRITE_SILPH_PRESIDENT",
        "SPRITE_SILPH_WORKER_M", "SPRITE_WARDEN", "SPRITE_CAPTAIN",
        "SPRITE_FISHER", "SPRITE_KOGA", "SPRITE_GUARD",
        "SPRITE_UNUSED_GUARD", "SPRITE_MOM", "SPRITE_BALDING_GUY",
        "SPRITE_LITTLE_BOY", "SPRITE_UNUSED_GAMEBOY_KID", "SPRITE_GAMEBOY_KID",
        "SPRITE_FAIRY", "SPRITE_AGATHA", "SPRITE_BRUNO", "SPRITE_LORELEI",
        "SPRITE_SEEL", "SPRITE_POKE_BALL", "SPRITE_FOSSIL", "SPRITE_BOULDER",
        "SPRITE_PAPER", "SPRITE_POKEDEX", "SPRITE_CLIPBOARD", "SPRITE_SNORLAX",
        "SPRITE_UNUSED_OLD_AMBER", "SPRITE_OLD_AMBER",
        "SPRITE_UNUSED_GAMBLER_ASLEEP_1", "SPRITE_UNUSED_GAMBLER_ASLEEP_2",
        "SPRITE_GAMBLER_ASLEEP"
    };
    for (int i = 0; i < (int)(sizeof(names) / sizeof(names[0])); i++)
        if (strcmp(names[i], name) == 0) return i;
    return 0;
}

static uint8_t parse_movement_byte1(const char *s) {
    if (strncmp(s, "WALK", 4) == 0) return MOVE_WALK;
    return MOVE_STAY;
}

static uint8_t parse_movement_byte2(const char *s) {
    if (strncmp(s, "ANY_DIR", 7) == 0)     return MOVE_ANY_DIR;
    if (strncmp(s, "UP_DOWN", 7) == 0)     return MOVE_UP_DOWN;
    if (strncmp(s, "LEFT_RIGHT", 10) == 0) return MOVE_LEFT_RIGHT;
    if (strncmp(s, "DOWN", 4) == 0)        return MOVE_DIR_DOWN;
    if (strncmp(s, "UP", 2) == 0)          return MOVE_DIR_UP;
    if (strncmp(s, "LEFT", 4) == 0)        return MOVE_DIR_LEFT;
    if (strncmp(s, "RIGHT", 5) == 0)       return MOVE_DIR_RIGHT;
    if (strncmp(s, "NONE", 4) == 0)        return MOVE_DIR_NONE;
    return MOVE_DIR_NONE;
}

static void build_blk_filename(const char *map_const, char *out, size_t out_size) {
    size_t j = 0;
    int at_word_start = 1;
    for (size_t i = 0; map_const[i] && j + 1 < out_size; i++) {
        char ch = map_const[i];
        if (ch == '_') { at_word_start = 1; continue; }
        if (at_word_start) {
            if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
            at_word_start = 0;
        } else {
            if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        }
        out[j++] = ch;
    }
    out[j] = 0;
}

/* -------------------------------------------------------------------------
   Asset decoders
   ------------------------------------------------------------------------- */

static Color decode_2bpp_bg_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
                                  const SGBPalette *sgb) {
    uint8_t idx = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
    int shade = reg_shade(reg, idx);
    Color c;
    if (sgb) c = sgb_color_to_rgba(sgb->colors[shade]);
    else {
        uint8_t v = dmg_shade[shade];
        c.r = v; c.g = v; c.b = v;
    }
    c.b = (uint8_t)((c.b & 0xFC) | (idx & 0x03));
    c.a = 255;
    return c;
}

static Color decode_2bpp_sprite_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
                                      const SGBPalette *sgb) {
    uint8_t idx = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
    int shade = reg_shade(reg, idx);
    Color c;
    if (sgb) c = sgb_color_to_rgba(sgb->colors[shade]);
    else {
        uint8_t v = dmg_shade[shade];
        c.r = v; c.g = v; c.b = v;
    }
    c.b = (uint8_t)((c.b & 0xFC) | (idx & 0x03));
    c.a = (idx == 0) ? 0 : 255;
    return c;
}

static void decode_tileset(const char *path, Tileset *ts, uint8_t reg,
                           int is_sprite, int pal_id) {
    ts->texture.id = 0;
    const SGBPalette *sgb = (pal_id >= 0 && pal_id < NUM_SGB_PALS)
                            ? &sgb_super_palettes[pal_id] : NULL;
    FILE *f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    int num_tiles = (int)(size / 16);
    if (num_tiles < 1) num_tiles = 1;

    ts->tiles_wide = is_sprite ? TILESET_TILES_WIDE * 2 : TILESET_TILES_WIDE;
    ts->tiles_high = (num_tiles + TILESET_TILES_WIDE - 1) / TILESET_TILES_WIDE;
    if (ts->tiles_high < 1) ts->tiles_high = 1;
    ts->tex_w = ts->tiles_wide * TILE_SIZE;
    ts->tex_h = ts->tiles_high * TILE_SIZE;
    ts->pixels = (Color *)malloc(ts->tex_w * ts->tex_h * sizeof(Color));
    for (int i = 0; i < ts->tex_w * ts->tex_h; i++)
        ts->pixels[i] = (Color){0, 0, 0, 0};

    uint8_t tile_data[16];
    for (int t = 0; t < num_tiles; t++) {
        if (fread(tile_data, 1, 16, f) != 16) break;
        int bank_col = t % TILESET_TILES_WIDE;
        int bank_row = t / TILESET_TILES_WIDE;
        for (int y = 0; y < 8; y++) {
            uint8_t lo = tile_data[y * 2], hi = tile_data[y * 2 + 1];
            for (int x = 0; x < 8; x++) {
                Color c = is_sprite
                    ? decode_2bpp_sprite_pixel(lo, hi, 7 - x, reg, sgb)
                    : decode_2bpp_bg_pixel(lo, hi, 7 - x, reg, sgb);
                int ax = bank_col * TILE_SIZE + x;
                int ay = bank_row * TILE_SIZE + y;
                ts->pixels[ay * ts->tex_w + ax] = c;
                if (is_sprite) {
                    int mx = (bank_col + TILESET_TILES_WIDE) * TILE_SIZE + (7 - x);
                    int my = bank_row * TILE_SIZE + y;
                    ts->pixels[my * ts->tex_w + mx] = c;
                }
            }
        }
    }
    fclose(f);

    Image img = {
        .data = ts->pixels,
        .width = ts->tex_w,
        .height = ts->tex_h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    ts->texture = LoadTextureFromImage(img);
    if (ts->texture.id) SetTextureFilter(ts->texture, TEXTURE_FILTER_POINT);
}

static Texture2D decode_1bpp(const char *path) {
    Texture2D tex = {0};
    FILE *f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); return tex; }
    uint8_t data[8];
    if (fread(data, 1, 8, f) != 8) { fclose(f); return tex; }
    fclose(f);

    Color pixels[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int bit = (data[y] >> (7 - x)) & 1;
            Color c = {0, 0, 0, bit ? 255 : 0};
            if (bit) c.b = (uint8_t)((c.b & 0xFC) | 3);
            pixels[y * 8 + x] = c;
        }
    }
    Image img = {
        .data = pixels,
        .width = 8, .height = 8, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    tex = LoadTextureFromImage(img);
    if (tex.id) SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

/* -------------------------------------------------------------------------
   Map data loaders
   ------------------------------------------------------------------------- */

static MapBlk load_blk(const char *path, int w_blocks, int h_blocks) {
    MapBlk m = {0};
    m.width = w_blocks;
    m.height = h_blocks;
    m.block_data = (uint8_t *)malloc(w_blocks * h_blocks);
    FILE *f = fopen(path, "rb");
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to open %s", path);
        free(m.block_data);
        m.block_data = NULL;
        return m;
    }
    fread(m.block_data, 1, w_blocks * h_blocks, f);
    fclose(f);
    return m;
}

static uint8_t *load_file(const char *path, int *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_size = 0; return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc(size);
    if (fread(data, 1, size, f) != (size_t)size)
        TraceLog(LOG_WARNING, "Short read on %s", path);
    fclose(f);
    *out_size = (int)size;
    return data;
}

static BlockDef *load_blockset(const char *path, int *out_count) {
    FILE *f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_count = 0; return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    int count = (int)(size / BLOCK_ENTRY_SIZE);
    BlockDef *blocks = (BlockDef *)malloc(count * sizeof(BlockDef));
    for (int i = 0; i < count; i++) {
        fread(blocks[i].tile_ids, 1, TILES_PER_BLOCK, f);
        fseek(f, BLOCK_ENTRY_SIZE - TILES_PER_BLOCK, SEEK_CUR);
    }
    fclose(f);
    *out_count = count;
    return blocks;
}

static uint8_t *load_collision_list(const char *path, const char *label, int *out_count) {
    FILE *f = fopen(path, "r");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_count = 0; return NULL; }
    uint8_t *list = (uint8_t *)malloc(256);
    int count = 0;
    int in_section = 0;
    int saw_coll_tiles = 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (!in_section) {
            char *p = strstr(line, label);
            if (p && strstr(p, "::")) in_section = 1;
            continue;
        }
        char *p = strstr(line, "coll_tiles");
        if (!p) {
            if (saw_coll_tiles && strstr(line, "::")) break;
            continue;
        }
        saw_coll_tiles = 1;
        p += strlen("coll_tiles");
        int found_any = 0;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == ',') p++;
            if (*p != '$') break;
            p++;
            unsigned int v = 0;
            int digits = 0;
            while (isxdigit((unsigned char)*p)) {
                char c = *p++;
                int d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else d = c - 'A' + 10;
                v = (v << 4) | (unsigned)d;
                digits++;
            }
            if (digits > 0 && count < 256) {
                list[count++] = (uint8_t)v;
                found_any = 1;
            }
        }
        if (!found_any) break;
    }
    fclose(f);
    *out_count = count;
    return list;
}

/* -------------------------------------------------------------------------
   Assembly source parsers
   ------------------------------------------------------------------------- */

static void parse_map_const(const char *path, const char *map_name, int *w, int *h) {
    *w = 0; *h = 0;
    char target[64];
    strncpy(target, map_name, sizeof(target) - 1);
    target[sizeof(target) - 1] = 0;
    strip_underscores(target);

    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char name[64]; int width, height;
        if (sscanf(line, " map_const %63[^,], %d, %d", name, &width, &height) == 3) {
            strip_underscores(name);
            if (strcmp(name, target) == 0) { *w = width; *h = height; break; }
        }
    }
    fclose(f);
}

static void apply_tileset_alias(char *stem) {
    for (int i = 0; tileset_aliases[i].from; i++) {
        if (strcmp(stem, tileset_aliases[i].from) == 0) {
            strncpy(stem, tileset_aliases[i].to, 63);
            stem[63] = 0;
            return;
        }
    }
}

static int parse_map_tileset(const char *header_path, char *out_stem, size_t stem_size) {
    FILE *f = fopen(header_path, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "map_header");
        if (!p) continue;
        p += strlen("map_header");
        char *c1 = strchr(p, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char *c3 = strchr(c2 + 1, ',');
        if (!c3) c3 = c2 + strlen(c2);

        char *s = c2 + 1;
        while (*s == ' ' || *s == '\t') s++;
        char *e = s;
        while (e < c3 && *e && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n') e++;
        int len = (int)(e - s);
        if (len <= 0 || (size_t)len >= stem_size) continue;
        for (int i = 0; i < len; i++) {
            char ch = s[i];
            if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
            out_stem[i] = ch;
        }
        out_stem[len] = 0;
        while (len > 0 && (out_stem[len - 1] == '\r' || out_stem[len - 1] == '\n' ||
                           out_stem[len - 1] == ' '  || out_stem[len - 1] == '\t'))
            out_stem[--len] = 0;
        apply_tileset_alias(out_stem);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

static int parse_border_block(const char *objects_path, uint8_t *out_border) {
    FILE *f = fopen(objects_path, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "border block");
        if (!p) continue;
        unsigned int v = 0;
        char *dollar = strchr(line, '$');
        if (dollar && sscanf(dollar, "$%x", &v) == 1) {
            *out_border = (uint8_t)v;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

static int parse_connections(const char *header_path, MapConnection *out, int max) {
    FILE *f = fopen(header_path, "r");
    if (!f) return 0;
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && count < max) {
        char *p = strstr(line, "connection");
        if (!p) continue;
        p += strlen("connection");
        while (*p == ' ' || *p == '\t') p++;

        ConnDir dir;
        if (strncmp(p, "north", 5) == 0)      dir = CONN_NORTH;
        else if (strncmp(p, "south", 5) == 0) dir = CONN_SOUTH;
        else if (strncmp(p, "west", 4) == 0)  dir = CONN_WEST;
        else if (strncmp(p, "east", 4) == 0)  dir = CONN_EAST;
        else continue;

        char *c1 = strchr(p, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char *c3 = strchr(c2 + 1, ',');
        if (!c3) continue;

        char *s = c2 + 1;
        while (*s == ' ' || *s == '\t') s++;
        char *e = s;
        while (*e && *e != ',' && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n') e++;
        int len = (int)(e - s);
        if (len <= 0 || len >= 64) continue;
        char map_const[64] = {0};
        memcpy(map_const, s, len);
        strip_underscores(map_const);

        s = c3 + 1;
        while (*s == ' ' || *s == '\t') s++;
        int offset = atoi(s);

        out[count].dir = dir;
        strncpy(out[count].map_name, map_const, sizeof(out[count].map_name) - 1);
        out[count].map_name[sizeof(out[count].map_name) - 1] = 0;
        out[count].offset = offset;
        out[count].loaded = 0;
        count++;
    }
    fclose(f);
    return count;
}

static int parse_warps(const char *objects_path, WarpEvent *out, int max) {
    FILE *f = fopen(objects_path, "r");
    if (!f) return 0;
    int count = 0;
    int in_section = 0;
    char line[512];
    while (fgets(line, sizeof(line), f) && count < max) {
        if (!in_section) {
            if (strstr(line, "def_warp_events")) in_section = 1;
            continue;
        }
        if (strstr(line, "def_bg_events") ||
            strstr(line, "def_object_events") ||
            strstr(line, "def_warps_to")) break;

        char *p = strstr(line, "warp_event");
        if (!p) continue;
        p += strlen("warp_event");

        int x = 0, y = 0, dest_id = 0;
        char dest_map[64] = {0};
        if (sscanf(p, " %d, %d, %63[^,], %d", &x, &y, dest_map, &dest_id) != 4) continue;

        char *s = dest_map;
        while (*s == ' ' || *s == '\t') s++;
        char *e = s + strlen(s) - 1;
        while (e > s && (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n')) *e-- = 0;
        strip_underscores(s);

        out[count].cell_x = x;
        out[count].cell_y = y;
        strncpy(out[count].dest_map, s, sizeof(out[count].dest_map) - 1);
        out[count].dest_map[sizeof(out[count].dest_map) - 1] = 0;
        out[count].dest_warp_id = dest_id - 1;
        out[count].warp_type = WARP_TYPE_NORMAL;
        out[count].warp_dir = -1;
        count++;
    }
    fclose(f);
    return count;
}

static int parse_objects(const char *objects_path, NPC *out, int max) {
    FILE *f = fopen(objects_path, "r");
    if (!f) return 0;
    int count = 0;
    int in_section = 0;
    char line[512];
    while (fgets(line, sizeof(line), f) && count < max) {
        if (!in_section) {
            if (strstr(line, "def_object_events")) in_section = 1;
            continue;
        }
        if (strstr(line, "def_warps_to")) break;

        char *p = strstr(line, "object_event");
        if (!p) continue;
        p += strlen("object_event");

        int x = 0, y = 0;
        char sprite_name[32] = {0};
        char mv1[16] = {0}, mv2[16] = {0}, text_id[64] = {0};
        if (sscanf(p, " %d, %d, %31[^,], %15[^,], %15[^,], %63[^,\n]",
                   &x, &y, sprite_name, mv1, mv2, text_id) != 6) continue;

        for (char *s = sprite_name; *s; s++) if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char *s = mv1; *s; s++)         if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char *s = mv2; *s; s++)         if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char *s = text_id; *s; s++)
            if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') { *s = 0; break; }

        NPC *n = &out[count];
        memset(n, 0, sizeof(*n));
        n->sprite_id = sprite_id_from_name(sprite_name);
        n->movement_byte1 = parse_movement_byte1(mv1);
        n->movement_byte2 = parse_movement_byte2(mv2);
        n->active = (n->sprite_id != 0);
        n->tile_x = x;
        n->tile_y = y;
        n->target_x = n->tile_x;
        n->target_y = n->tile_y;
        n->movement_status = MSTAT_READY;
        switch (n->movement_byte2) {
            case MOVE_DIR_DOWN:  n->facing = DIR_DOWN;  break;
            case MOVE_DIR_UP:    n->facing = DIR_UP;    break;
            case MOVE_DIR_LEFT:  n->facing = DIR_LEFT;  break;
            case MOVE_DIR_RIGHT: n->facing = DIR_RIGHT; break;
            default:             n->facing = DIR_DOWN;  break;
        }
        count++;
    }
    fclose(f);
    return count;
}

/* -------------------------------------------------------------------------
   Map loading
   ------------------------------------------------------------------------- */

static void free_map_data(ActiveMap *am) {
    if (am->map.block_data) { free(am->map.block_data); am->map.block_data = NULL; }
    for (int i = 0; i < am->num_connections; i++) {
        if (am->connections[i].loaded) {
            free(am->connections[i].map.block_data);
            am->connections[i].map.block_data = NULL;
            am->connections[i].loaded = 0;
        }
    }
    am->num_connections = 0;
    am->num_warps = 0;
    am->num_npcs = 0;
    if (am->blocks) { free(am->blocks); am->blocks = NULL; am->num_blocks = 0; }
    if (am->collision_list) { free(am->collision_list); am->collision_list = NULL; am->num_collision = 0; }
}

static void free_map_gpu(ActiveMap *am) {
    if (am->tileset.texture.id) {
        UnloadTexture(am->tileset.texture);
        am->tileset.texture.id = 0;
    }
    if (am->tileset.pixels) {
        free(am->tileset.pixels);
        am->tileset.pixels = NULL;
    }
}

static void load_connection_map(MapConnection *c, const char *map_const_path,
                                const char *maps_dir) {
    int w = 0, h = 0;
    parse_map_const(map_const_path, c->map_name, &w, &h);
    if (w == 0 || h == 0) {
        TraceLog(LOG_WARNING, "Could not parse dimensions for %s", c->map_name);
        return;
    }
    char filename[128];
    build_blk_filename(c->map_name, filename, sizeof(filename));
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.blk", maps_dir, filename);
    c->map = load_blk(path, w, h);
    if (c->map.block_data) c->loaded = 1;
}

static int cell_has_door_tile(ActiveMap *am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef *def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
            if (is_door_tile(am->tileset_stem, def->tile_ids[(qy + dy) * 4 + (qx + dx)]))
                return 1;
    return 0;
}

static int cell_has_warp_tile(ActiveMap *am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef *def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
            if (is_warp_tile(am->tileset_stem, def->tile_ids[(qy + dy) * 4 + (qx + dx)]))
                return 1;
    return 0;
}

static int cell_has_carpet_tile(ActiveMap *am, int cx, int cy, int *out_dir) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef *def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            uint8_t tid = def->tile_ids[(qy + dy) * 4 + (qx + dx)];
            if (list_has_term(carpet_down, tid))  { if (out_dir) *out_dir = DIR_DOWN;  return 1; }
            if (list_has_term(carpet_up, tid))    { if (out_dir) *out_dir = DIR_UP;    return 1; }
            if (list_has_term(carpet_left, tid))  { if (out_dir) *out_dir = DIR_LEFT;  return 1; }
            if (list_has_term(carpet_right, tid)) { if (out_dir) *out_dir = DIR_RIGHT; return 1; }
        }
    }
    return 0;
}

static int load_map(ActiveMap *am, const char *map_name,
                    const char *map_const_path,
                    const char *headers_dir,
                    const char *objects_dir,
                    const char *maps_dir,
                    const char *tilesets_dir,
                    const char *blocksets_dir,
                    const char *collision_path) {
    int w = 0, h = 0;
    parse_map_const(map_const_path, map_name, &w, &h);
    if (w == 0 || h == 0) {
        TraceLog(LOG_ERROR, "Could not parse dimensions for %s", map_name);
        return 0;
    }

    char saved_prev[64];
    saved_prev[0] = 0;
    if (am->name[0] && am->num_connections > 0) {
        strncpy(saved_prev, am->name, sizeof(saved_prev) - 1);
        saved_prev[sizeof(saved_prev) - 1] = 0;
    } else if (am->prev_map[0]) {
        strncpy(saved_prev, am->prev_map, sizeof(saved_prev) - 1);
        saved_prev[sizeof(saved_prev) - 1] = 0;
    }

    free_map_gpu(am);
    free_map_data(am);
    memset(am, 0, sizeof(*am));

    if (saved_prev[0]) {
        strncpy(am->prev_map, saved_prev, sizeof(am->prev_map) - 1);
        am->prev_map[sizeof(am->prev_map) - 1] = 0;
    }
    strncpy(am->name, map_name, sizeof(am->name) - 1);
    am->name[sizeof(am->name) - 1] = 0;

    char filename[128];
    build_blk_filename(map_name, filename, sizeof(filename));

    char blk_path[256];
    snprintf(blk_path, sizeof(blk_path), "%s/%s.blk", maps_dir, filename);
    am->map = load_blk(blk_path, w, h);
    if (!am->map.block_data) {
        TraceLog(LOG_ERROR, "Failed to load map %s from %s", map_name, blk_path);
        return 0;
    }

    char obj_path[256];
    snprintf(obj_path, sizeof(obj_path), "%s/%s.asm", objects_dir, filename);
    uint8_t border = 0x00;
    parse_border_block(obj_path, &border);
    am->border_block = border;

    char hdr_path[256];
    snprintf(hdr_path, sizeof(hdr_path), "%s/%s.asm", headers_dir, filename);
    am->tileset_stem[0] = 0;
    if (!parse_map_tileset(hdr_path, am->tileset_stem, sizeof(am->tileset_stem)))
        strncpy(am->tileset_stem, "overworld", sizeof(am->tileset_stem) - 1);

    am->num_connections = parse_connections(hdr_path, am->connections, MAX_CONNECTIONS);
    for (int i = 0; i < am->num_connections; i++)
        load_connection_map(&am->connections[i], map_const_path, maps_dir);

    if (am->num_connections == 0 && am->prev_map[0])
        lookup_sgb_palette_ids(am->prev_map, am->sgb_pals);
    else
        lookup_sgb_palette_ids(am->name, am->sgb_pals);

    char ts_path[256];
    snprintf(ts_path, sizeof(ts_path), "%s/%s.2bpp", tilesets_dir, am->tileset_stem);
    decode_tileset(ts_path, &am->tileset, REG_BGP, 0, am->sgb_pals[0]);

    char bs_path[256];
    snprintf(bs_path, sizeof(bs_path), "%s/%s.bst", blocksets_dir, am->tileset_stem);
    am->num_blocks = 0;
    am->blocks = load_blockset(bs_path, &am->num_blocks);

    const char *coll_label = lookup_collision_label(am->tileset_stem);
    am->num_collision = 0;
    am->collision_list = load_collision_list(collision_path, coll_label, &am->num_collision);

    am->num_warps = parse_warps(obj_path, am->warps, MAX_WARPS);
    for (int i = 0; i < am->num_warps; i++) {
        am->warps[i].warp_type = WARP_TYPE_NORMAL;
        am->warps[i].warp_dir = -1;
        if (cell_has_door_tile(am, am->warps[i].cell_x, am->warps[i].cell_y)) {
            am->warps[i].warp_type = WARP_TYPE_DOOR;
        } else if (cell_has_warp_tile(am, am->warps[i].cell_x, am->warps[i].cell_y)) {
            am->warps[i].warp_type = WARP_TYPE_NORMAL;
        } else {
            int dir = -1;
            if (cell_has_carpet_tile(am, am->warps[i].cell_x, am->warps[i].cell_y, &dir)) {
                am->warps[i].warp_type = WARP_TYPE_CARPET;
                am->warps[i].warp_dir = dir;
            }
        }
    }

    am->num_npcs = parse_objects(obj_path, am->npcs, MAX_OBJECTS);
    for (int i = 0; i < am->num_npcs; i++) {
        NPC *n = &am->npcs[i];
        if (!n->active) continue;
        if (n->tile_x < 0 || n->tile_y < 0 ||
            n->tile_x >= am->map.width * 2 ||
            n->tile_y >= am->map.height * 2)
            n->active = 0;
    }

    TraceLog(LOG_INFO, "Loaded map %s (%dx%d) tileset=%s blocks=%d collision=%d conns=%d warps=%d npcs=%d prev=%s",
             map_name, w, h, am->tileset_stem, am->num_blocks, am->num_collision,
             am->num_connections, am->num_warps, am->num_npcs, am->prev_map);
    return 1;
}

/* -------------------------------------------------------------------------
   Tile sampling and collision
   ------------------------------------------------------------------------- */

static int is_tile_passable(uint8_t tile_id, uint8_t *collision_list, int num_collision) {
    if (tile_id == 0x03) return 1;
    for (int i = 0; i < num_collision; i++)
        if (collision_list[i] == tile_id) return 1;
    return 0;
}

static uint8_t tile_in_front_of_cell(ActiveMap *am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0xFF;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0xFF;
    BlockDef *def = &am->blocks[block_id];
    return def->tile_ids[((cy & 1) * 2 + 1) * 4 + (cx & 1) * 2];
}

static int can_walk_tile(ActiveMap *am, int nx, int ny) {
    if (nx < 0 || ny < 0) return 0;
    if (nx >= am->map.width * 2 || ny >= am->map.height * 2) return 0;
    uint8_t tile_id = tile_in_front_of_cell(am, nx, ny);
    if (tile_id == 0xFF) return 0;
    return is_tile_passable(tile_id, am->collision_list, am->num_collision);
}

static int npc_occupies(ActiveMap *am, int skip, int nx, int ny) {
    for (int i = 0; i < am->num_npcs; i++) {
        if (i == skip) continue;
        NPC *n = &am->npcs[i];
        if (!n->active) continue;
        if (n->tile_x == nx && n->tile_y == ny) return 1;
        if (n->movement_status == MSTAT_WALKING &&
            n->target_x == nx && n->target_y == ny) return 1;
    }
    return 0;
}

static int player_occupies(Player *p, int nx, int ny) {
    if (p->tile_x == nx && p->tile_y == ny) return 1;
    if (p->state == PSTATE_MOVING &&
        p->target_x == nx && p->target_y == ny) return 1;
    return 0;
}

static int is_ledge_tile(ActiveMap *am, int standing_x, int standing_y,
                         Direction d, int nx, int ny) {
    uint8_t standing_tile = tile_in_front_of_cell(am, standing_x, standing_y);
    uint8_t target_tile   = tile_in_front_of_cell(am, nx, ny);
    for (int i = 0; i < (int)(sizeof(ledge_tiles) / sizeof(ledge_tiles[0])); i++) {
        if (ledge_tiles[i].dir == d &&
            ledge_tiles[i].standing_tile == standing_tile &&
            ledge_tiles[i].ledge_tile == target_tile)
            return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static int read_input_direction(void) {
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) return DIR_UP;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) return DIR_DOWN;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) return DIR_LEFT;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) return DIR_RIGHT;
    return -1;
}

/* -------------------------------------------------------------------------
   Walk animation
   ------------------------------------------------------------------------- */

static void advance_walk_anim(int *intra_frame, int *anim_frame) {
    (*intra_frame)++;
    if (*intra_frame >= WALK_FRAME_TICKS) {
        *intra_frame = 0;
        *anim_frame = (*anim_frame + 1) & 3;
    }
}

/* -------------------------------------------------------------------------
   Player step management
   ------------------------------------------------------------------------- */

static void begin_step(Player *p, Direction d, int nx, int ny, int frames) {
    p->target_x = nx;
    p->target_y = ny;
    p->pixel_offset = 0;
    p->walk_counter = frames;
    p->intra_frame = 0;
    p->state = PSTATE_MOVING;
    p->facing = d;
}

static MapConnection *check_connection(ActiveMap *am, int nx, int ny) {
    if (nx >= 0 && ny >= 0 &&
        nx < am->map.width * 2 && ny < am->map.height * 2) return NULL;

    ConnDir needed;
    if (ny < 0)                        needed = CONN_NORTH;
    else if (ny >= am->map.height * 2) needed = CONN_SOUTH;
    else if (nx < 0)                   needed = CONN_WEST;
    else                               needed = CONN_EAST;

    for (int i = 0; i < am->num_connections; i++)
        if (am->connections[i].dir == needed && am->connections[i].loaded)
            return &am->connections[i];
    return NULL;
}

static int find_warp_index(ActiveMap *am, int cx, int cy) {
    for (int i = 0; i < am->num_warps; i++)
        if (am->warps[i].cell_x == cx && am->warps[i].cell_y == cy)
            return i;
    return -1;
}

static WarpEvent *find_warp(ActiveMap *am, int cx, int cy) {
    int i = find_warp_index(am, cx, cy);
    return (i >= 0) ? &am->warps[i] : NULL;
}

static void place_player_at_warp(ActiveMap *am, Player *p, WarpEvent *dst) {
    int ex = dst->cell_x;
    int ey = dst->cell_y;
    if (ex < 0) ex = 0;
    if (ey < 0) ey = 0;
    if (ex >= am->map.width * 2)  ex = am->map.width * 2 - 1;
    if (ey >= am->map.height * 2) ey = am->map.height * 2 - 1;
    p->tile_x = ex;
    p->tile_y = ey;
    p->target_x = ex;
    p->target_y = ey;
    p->pixel_offset = 0;
    p->walk_counter = 0;
    p->state = PSTATE_NOT_MOVING;
    p->hop_active = 0;
    p->hop_frames_remaining = 0;
}

static Direction choose_forced_dir(ActiveMap *am, int tx, int ty) {
    if (can_walk_tile(am, tx, ty + 1)) return DIR_DOWN;
    if (can_walk_tile(am, tx, ty - 1)) return DIR_UP;
    if (can_walk_tile(am, tx + 1, ty)) return DIR_RIGHT;
    if (can_walk_tile(am, tx - 1, ty)) return DIR_LEFT;
    return DIR_DOWN;
}

/* -------------------------------------------------------------------------
   Map transitions and warps
   ------------------------------------------------------------------------- */

typedef struct {
    const char *map_const_path;
    const char *headers_dir;
    const char *objects_dir;
    const char *maps_dir;
    const char *tilesets_dir;
    const char *blocksets_dir;
    const char *collision_path;
} MapPaths;

static void do_map_transition(ActiveMap *am, Player *p, const MapPaths *paths) {
    MapConnection *conn = check_connection(am, p->tile_x, p->tile_y);
    if (!conn) return;

    char new_map[64];
    strncpy(new_map, conn->map_name, sizeof(new_map) - 1);
    new_map[sizeof(new_map) - 1] = 0;

    int old_tx = p->tile_x;
    int old_ty = p->tile_y;
    Direction d = p->facing;
    int offset = conn->offset;

    if (!load_map(am, new_map, paths->map_const_path, paths->headers_dir,
                  paths->objects_dir, paths->maps_dir, paths->tilesets_dir,
                  paths->blocksets_dir, paths->collision_path))
        return;

    switch (d) {
        case DIR_UP:    p->tile_y = am->map.height * 2 - 1; p->tile_x = old_tx - offset * 2; break;
        case DIR_DOWN:  p->tile_y = 0;                       p->tile_x = old_tx - offset * 2; break;
        case DIR_LEFT:  p->tile_x = am->map.width * 2 - 1;   p->tile_y = old_ty - offset * 2; break;
        case DIR_RIGHT: p->tile_x = 0;                       p->tile_y = old_ty - offset * 2; break;
    }
    if (p->tile_x < 0) p->tile_x = 0;
    if (p->tile_y < 0) p->tile_y = 0;
    if (p->tile_x >= am->map.width * 2)  p->tile_x = am->map.width * 2 - 1;
    if (p->tile_y >= am->map.height * 2) p->tile_y = am->map.height * 2 - 1;

    p->target_x = p->tile_x;
    p->target_y = p->tile_y;
    p->pixel_offset = 0;
    p->walk_counter = 0;
    p->state = PSTATE_NOT_MOVING;
    p->hop_active = 0;
    p->hop_frames_remaining = 0;
}

static void do_warp(ActiveMap *am, Player *p, WarpEvent *warp, const MapPaths *paths) {
    char dest_map[64];
    strncpy(dest_map, warp->dest_map, sizeof(dest_map) - 1);
    dest_map[sizeof(dest_map) - 1] = 0;

    if (strcmp(dest_map, "LASTMAP") == 0) {
        if (am->prev_map[0] == 0) {
            TraceLog(LOG_WARNING, "LASTMAP warp but no previous outdoor map");
            return;
        }
        strncpy(dest_map, am->prev_map, sizeof(dest_map) - 1);
        dest_map[sizeof(dest_map) - 1] = 0;
    }

    int dest_id = warp->dest_warp_id;
    if (!load_map(am, dest_map, paths->map_const_path, paths->headers_dir,
                  paths->objects_dir, paths->maps_dir, paths->tilesets_dir,
                  paths->blocksets_dir, paths->collision_path))
        return;

    if (dest_id < 0 || dest_id >= am->num_warps) return;
    WarpEvent *dst = &am->warps[dest_id];
    place_player_at_warp(am, p, dst);

    if (dst->warp_type == WARP_TYPE_DOOR) {
        Direction forced = choose_forced_dir(am, p->tile_x, p->tile_y);
        p->forced_move_ticks = WALK_STEP_FRAMES;
        p->target_x = p->tile_x + dir_dx(forced);
        p->target_y = p->tile_y + dir_dy(forced);
        p->pixel_offset = 0;
        p->walk_counter = WALK_STEP_FRAMES;
        p->facing = forced;
        p->state = PSTATE_MOVING;
    }
    p->warp_cooldown = 8;
}

/* -------------------------------------------------------------------------
   Sprite loading
   ------------------------------------------------------------------------- */

static void reload_player_sprite(Tileset *player_ts, const char *sprite_path,
                                 uint8_t reg, int pal_id) {
    if (player_ts->texture.id) {
        UnloadTexture(player_ts->texture);
        player_ts->texture.id = 0;
    }
    if (player_ts->pixels) {
        free(player_ts->pixels);
        player_ts->pixels = NULL;
    }
    memset(player_ts, 0, sizeof(*player_ts));
    decode_tileset(sprite_path, player_ts, reg, 1, pal_id);
}

static void load_npc_sprites(ActiveMap *am, Tileset *npc_ts, uint8_t reg, int pal_id) {
    for (int i = 0; i < NUM_NPC_SPRITES; i++) {
        if (npc_ts[i].texture.id) {
            UnloadTexture(npc_ts[i].texture);
            npc_ts[i].texture.id = 0;
        }
        if (npc_ts[i].pixels) {
            free(npc_ts[i].pixels);
            npc_ts[i].pixels = NULL;
        }
        memset(&npc_ts[i], 0, sizeof(npc_ts[i]));
    }
    for (int i = 0; i < am->num_npcs; i++) {
        NPC *n = &am->npcs[i];
        if (!n->active) continue;
        if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
        if (npc_ts[n->sprite_id].texture.id) continue;
        const char *file = npc_sprite_files[n->sprite_id];
        if (!file) continue;
        char path[256];
        snprintf(path, sizeof(path), "%s/%s.2bpp", REPO_ROOT "/gfx/sprites", file);
        decode_tileset(path, &npc_ts[n->sprite_id], reg, 1, pal_id);
    }
}

static void apply_palette(ActiveMap *am, Tileset *player_ts, Tileset *npc_ts,
                          const char *player_sprite_path,
                          uint8_t bgp, uint8_t obp0) {
    char ts_path[256];
    snprintf(ts_path, sizeof(ts_path), "%s/%s.2bpp", REPO_ROOT "/gfx/tilesets",
             am->tileset_stem);
    if (am->tileset.texture.id) {
        UnloadTexture(am->tileset.texture);
        am->tileset.texture.id = 0;
    }
    if (am->tileset.pixels) {
        free(am->tileset.pixels);
        am->tileset.pixels = NULL;
    }
    memset(&am->tileset, 0, sizeof(am->tileset));
    decode_tileset(ts_path, &am->tileset, bgp, 0, am->sgb_pals[0]);

    reload_player_sprite(player_ts, player_sprite_path, obp0, am->sgb_pals[0]);
    load_npc_sprites(am, npc_ts, obp0, am->sgb_pals[0]);
}

/* -------------------------------------------------------------------------
   NPC update
   ------------------------------------------------------------------------- */

static void update_npc(ActiveMap *am, NPC *n, int idx, Player *player) {
    if (!n->active) return;
    int player_walk_counter = player->walk_counter;

    if (n->movement_status == MSTAT_READY) {
        if (player_walk_counter != 0) return;

        Direction chosen;
        int should_step = 1;

        if (n->movement_byte1 == MOVE_STAY) {
            should_step = 0;
            switch (n->movement_byte2) {
                case MOVE_DIR_DOWN:  chosen = DIR_DOWN;  break;
                case MOVE_DIR_UP:    chosen = DIR_UP;    break;
                case MOVE_DIR_LEFT:  chosen = DIR_LEFT;  break;
                case MOVE_DIR_RIGHT: chosen = DIR_RIGHT; break;
                default:             chosen = DIR_DOWN;  break;
            }
        } else {
            switch (n->movement_byte2) {
                case MOVE_UP_DOWN:
                    chosen = (GetRandomValue(0, 1) == 0) ? DIR_UP : DIR_DOWN; break;
                case MOVE_LEFT_RIGHT:
                    chosen = (GetRandomValue(0, 1) == 0) ? DIR_LEFT : DIR_RIGHT; break;
                case MOVE_DIR_DOWN:  chosen = DIR_DOWN;  break;
                case MOVE_DIR_UP:    chosen = DIR_UP;    break;
                case MOVE_DIR_LEFT:  chosen = DIR_LEFT;  break;
                case MOVE_DIR_RIGHT: chosen = DIR_RIGHT; break;
                default:             chosen = (Direction)GetRandomValue(0, 3); break;
            }
        }

        n->facing = chosen;

        if (should_step) {
            int nx = n->tile_x + dir_dx(chosen);
            int ny = n->tile_y + dir_dy(chosen);
            if (can_walk_tile(am, nx, ny) &&
                !npc_occupies(am, idx, nx, ny) &&
                !player_occupies(player, nx, ny)) {
                n->target_x = nx;
                n->target_y = ny;
                n->pixel_offset = 0;
                n->walk_counter = NPC_STEP_FRAMES;
                n->intra_frame = 0;
                n->anim_frame = 0;
                n->movement_status = MSTAT_WALKING;
            } else {
                n->movement_delay = GetRandomValue(1, 0x40);
                n->movement_status = MSTAT_DELAYED;
            }
        } else {
            n->movement_delay = GetRandomValue(1, 0x40);
            n->movement_status = MSTAT_DELAYED;
        }
    } else if (n->movement_status == MSTAT_WALKING) {
        advance_walk_anim(&n->intra_frame, &n->anim_frame);
        n->pixel_offset += 1;
        n->walk_counter--;
        if (n->walk_counter == 0) {
            n->tile_x = n->target_x;
            n->tile_y = n->target_y;
            n->pixel_offset = 0;
            if (n->movement_byte1 == MOVE_WALK) {
                n->movement_delay = GetRandomValue(1, 0x40);
                n->movement_status = MSTAT_DELAYED;
            } else {
                n->movement_status = MSTAT_READY;
            }
        }
    } else if (n->movement_status == MSTAT_DELAYED) {
        n->movement_delay--;
        if (n->movement_delay <= 0) n->movement_status = MSTAT_READY;
    }
}

/* -------------------------------------------------------------------------
   Fade state machine
   ------------------------------------------------------------------------- */

static void begin_fade_out(PaletteFade *fade, int to_black, int warp_index) {
    fade->phase = FADE_OUT;
    fade->pending = FADE_PENDING_WARP;
    fade->step = 0;
    fade->ticks_in_step = 0;
    fade->to_black = to_black;
    fade->warp_index = warp_index;
    fade->current_bgp = REG_BGP;
}

static void fade_tick(PaletteFade *fade, ActiveMap *am, Player *player,
                      Tileset *player_ts, Tileset *npc_ts,
                      const char *player_sprite_path, const MapPaths *paths) {
    if (fade->phase == FADE_NONE) return;

    if (fade->phase == FADE_OUT) {
        if (fade->ticks_in_step == 0) {
            int idx = fade->to_black ? fade_out_black_seq[fade->step]
                                     : fade_out_white_seq[fade->step];
            fade->current_bgp = fade_pal[idx][0];
            apply_palette(am, player_ts, npc_ts, player_sprite_path,
                          fade_pal[idx][0], fade_pal[idx][1]);
        }

        fade->ticks_in_step++;
        if (fade->ticks_in_step < FADE_STEP_TICKS) return;
        fade->ticks_in_step = 0;
        fade->step++;

        int num_steps = fade->to_black ? 4 : 3;
        if (fade->step >= num_steps) {
            if (fade->pending == FADE_PENDING_WARP) {
                WarpEvent *warp = &am->warps[fade->warp_index];
                do_warp(am, player, warp, paths);
                fade->pending = FADE_PENDING_NONE;
                fade->hide_world = 1;
            }
            fade->phase = FADE_IN;
            fade->step = 0;
            fade->ticks_in_step = 0;
        }
    } else {
        /* FADE_IN */
        fade->hide_world = 0;

        if (fade->ticks_in_step == 0) {
            int idx = fade->to_black ? fade_in_black_seq[fade->step]
                                     : fade_in_white_seq[fade->step];
            fade->current_bgp = fade_pal[idx][0];
            apply_palette(am, player_ts, npc_ts, player_sprite_path,
                          fade_pal[idx][0], fade_pal[idx][1]);
        }

        fade->ticks_in_step++;
        if (fade->ticks_in_step < FADE_STEP_TICKS) return;
        fade->ticks_in_step = 0;
        fade->step++;

        int num_steps = fade->to_black ? 4 : 3;
        if (fade->step >= num_steps) {
            fade->phase = FADE_NONE;
            fade->current_bgp = REG_BGP;
            apply_palette(am, player_ts, npc_ts, player_sprite_path,
                          REG_BGP, REG_OBP0);
        }
    }
}

/* -------------------------------------------------------------------------
   Shaders
   ------------------------------------------------------------------------- */

static const char *priority_shader_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D bgTex;\n"
    "uniform sampler2D sprTex;\n"
    "uniform sampler2D prioTex;\n"
    "uniform vec3 bgColor;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 bg  = texture(bgTex,  fragTexCoord);\n"
    "    vec4 spr = texture(sprTex, fragTexCoord);\n"
    "    float bg_idx  = floor(mod(bg.b  * 255.0 + 0.5, 4.0));\n"
    "    float spr_idx = floor(mod(spr.b * 255.0 + 0.5, 4.0));\n"
    "    float prio    = texture(prioTex, fragTexCoord).r;\n"
    "    vec3 out_rgb;\n"
    "    if (spr_idx < 0.5) {\n"
    "        out_rgb = (bg_idx < 0.5) ? bgColor : bg.rgb;\n"
    "    } else if (prio < 0.5 || bg_idx < 0.5) {\n"
    "        out_rgb = spr.rgb;\n"
    "    } else {\n"
    "        out_rgb = bg.rgb;\n"
    "    }\n"
    "    finalColor = vec4(out_rgb, 1.0);\n"
    "}\n";

static const char *prio_write_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float isPrio;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 t = texture(texture0, fragTexCoord) * fragColor;\n"
    "    if (t.a < 0.5) discard;\n"
    "    finalColor = vec4(isPrio, isPrio, isPrio, 1.0);\n"
    "}\n";

/* -------------------------------------------------------------------------
   Sprite rendering helpers
   ------------------------------------------------------------------------- */

static Rectangle sprite_quadrant_src(const Tileset *ts, uint8_t tid, int flip) {
    int physical = (tid >= 0x80) ? (tid - 0x80 + 12) : tid;
    int bank_col = physical % TILESET_TILES_WIDE;
    int bank_row = physical / TILESET_TILES_WIDE;
    int col = flip ? (bank_col + TILESET_TILES_WIDE) : bank_col;
    int row = bank_row;
    (void)ts;
    return (Rectangle){ col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE };
}

static void draw_sprite(Tileset *ts, const uint8_t *tiles, int flip,
                        int px, int py) {
    if (!ts->texture.id) return;
    for (int i = 0; i < 4; i++) {
        int src_i = flip ? (i ^ 1) : i;
        uint8_t tid = tiles[src_i];
        Rectangle src = sprite_quadrant_src(ts, tid, flip);
        int ox = (i & 1) ? 8 : 0;
        int oy = (i & 2) ? 8 : 0;
        Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
        DrawTexturePro(ts->texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

static void draw_shadow(Texture2D shadow_tex, int px, int py) {
    if (!shadow_tex.id) return;
    for (int q = 0; q < 4; q++) {
        int ox = (q & 1) ? 8 : 0;
        int oy = (q & 2) ? 8 : 0;
        int flipx = (q & 1) ? 1 : 0;
        int flipy = (q & 2) ? 1 : 0;
        Rectangle src = {
            (float)(flipx ? 8 : 0),
            (float)(flipy ? 8 : 0),
            (float)(flipx ? -8 : 8),
            (float)(flipy ? -8 : 8)
        };
        Rectangle dst = { (float)(px + ox), (float)(py + oy), 8.0f, 8.0f };
        DrawTexturePro(shadow_tex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

/* -------------------------------------------------------------------------
   Main
   ------------------------------------------------------------------------- */

int main(void) {
    const MapPaths paths = {
        .map_const_path = REPO_ROOT "/constants/map_constants.asm",
        .headers_dir    = REPO_ROOT "/data/maps/headers",
        .objects_dir    = REPO_ROOT "/data/maps/objects",
        .maps_dir       = REPO_ROOT "/maps",
        .tilesets_dir   = REPO_ROOT "/gfx/tilesets",
        .blocksets_dir  = REPO_ROOT "/gfx/blocksets",
        .collision_path = REPO_ROOT "/data/tilesets/collision_tile_ids.asm",
    };
    const char *flower_frame_1     = REPO_ROOT "/gfx/tilesets/flower/flower1.2bpp";
    const char *flower_frame_2     = REPO_ROOT "/gfx/tilesets/flower/flower2.2bpp";
    const char *flower_frame_3     = REPO_ROOT "/gfx/tilesets/flower/flower3.2bpp";
    const char *player_sprite_path = REPO_ROOT "/gfx/sprites/red.2bpp";
    const char *shadow_path        = REPO_ROOT "/gfx/overworld/shadow.1bpp";

    InitWindow(GB_WIDTH * SCALE, GB_HEIGHT * SCALE, "Pokered");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D bg_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(bg_layer.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D sprite_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(sprite_layer.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D priority_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(priority_layer.texture, TEXTURE_FILTER_POINT);

    Image white_img = GenImageColor(1, 1, WHITE);
    Texture2D white_tex = LoadTextureFromImage(white_img);
    UnloadImage(white_img);

    Shader priority_shader   = LoadShaderFromMemory(NULL, priority_shader_fs);
    Shader prio_write_shader = LoadShaderFromMemory(NULL, prio_write_fs);
    int loc_bg_tex    = GetShaderLocation(priority_shader, "bgTex");
    int loc_spr_tex   = GetShaderLocation(priority_shader, "sprTex");
    int loc_prio_tex  = GetShaderLocation(priority_shader, "prioTex");
    int loc_bg_color  = GetShaderLocation(priority_shader, "bgColor");
    int loc_pw_isprio = GetShaderLocation(prio_write_shader, "isPrio");

    ActiveMap current = {0};
    if (!load_map(&current, "PALLET_TOWN", paths.map_const_path,
                  paths.headers_dir, paths.objects_dir, paths.maps_dir,
                  paths.tilesets_dir, paths.blocksets_dir, paths.collision_path)) {
        UnloadShader(priority_shader);
        UnloadShader(prio_write_shader);
        UnloadTexture(white_tex);
        UnloadRenderTexture(priority_layer);
        UnloadRenderTexture(sprite_layer);
        UnloadRenderTexture(bg_layer);
        UnloadRenderTexture(target);
        CloseWindow();
        return 1;
    }

    Tileset player_ts = {0};
    decode_tileset(player_sprite_path, &player_ts, REG_OBP0, 1, current.sgb_pals[0]);

    Tileset npc_ts[NUM_NPC_SPRITES] = {0};
    load_npc_sprites(&current, npc_ts, REG_OBP0, current.sgb_pals[0]);

    Texture2D shadow_tex = decode_1bpp(shadow_path);

    int overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);

    TileAnim flower_anim = {
        .type = ANIM_FRAME_SWAP, .target_tile = 0x03,
        .current_frame = 0, .frame_count = 3,
    };
    TileAnim water_anim = {
        .type = ANIM_TILE_ROTATE, .target_tile = 0x14,
    };

    int size1 = 0, size2 = 0, size3 = 0;
    uint8_t *frame1 = load_file(flower_frame_1, &size1);
    uint8_t *frame2 = load_file(flower_frame_2, &size2);
    uint8_t *frame3 = load_file(flower_frame_3, &size3);
    if (frame1 && frame2 && frame3 && size1 >= 16 && size2 >= 16 && size3 >= 16) {
        flower_anim.frame_data = (uint8_t *)malloc(48);
        memcpy(flower_anim.frame_data,      frame1, 16);
        memcpy(flower_anim.frame_data + 16, frame2, 16);
        memcpy(flower_anim.frame_data + 32, frame3, 16);
        flower_anim.frame_size = 48;
    } else {
        flower_anim.type = ANIM_NONE;
    }
    free(frame1); free(frame2); free(frame3);

    water_anim.working_tile = (uint8_t *)malloc(16);
    {
        char water_path[256];
        snprintf(water_path, sizeof(water_path), "%s/overworld.2bpp", paths.tilesets_dir);
        FILE *tf = fopen(water_path, "rb");
        if (tf) {
            fseek(tf, 0x14 * 16, SEEK_SET);
            fread(water_anim.working_tile, 1, 16, tf);
            fclose(tf);
        }
    }

    Player player = {0};
    player.tile_x = current.map.width;
    player.tile_y = current.map.height;
    player.target_x = player.tile_x;
    player.target_y = player.tile_y;
    player.state = PSTATE_NOT_MOVING;
    player.facing = DIR_DOWN;

    PaletteFade fade = {0};
    fade.current_bgp = REG_BGP;

    int counter1 = 0, counter2 = 0;
    float logic_accumulator = 0.0f;
    float anim_accumulator  = 0.0f;
    const float LOGIC_DT  = 1.0f / 30.0f;
    const float VBLANK_DT = 1.0f / 60.0f;

    while (!WindowShouldClose()) {
        float frame_time = GetFrameTime();
        if (frame_time > 0.25f) frame_time = 0.25f;
        logic_accumulator += frame_time;
        anim_accumulator  += frame_time;

        /* -------------------- Logic tick -------------------- */
        while (logic_accumulator >= LOGIC_DT) {
            if (player.warp_cooldown > 0) player.warp_cooldown--;

            if (fade.phase != FADE_NONE) {
                fade_tick(&fade, &current, &player, &player_ts, npc_ts,
                          player_sprite_path, &paths);
                overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);
                logic_accumulator -= LOGIC_DT;
                continue;
            }

            for (int i = 0; i < current.num_npcs; i++)
                update_npc(&current, &current.npcs[i], i, &player);

            if (player.forced_move_ticks > 0) {
                advance_walk_anim(&player.intra_frame, &player.anim_frame);
                player.pixel_offset += 2;
                player.walk_counter--;
                player.forced_move_ticks--;
                if (player.walk_counter == 0) {
                    player.tile_x = player.target_x;
                    player.tile_y = player.target_y;
                    player.pixel_offset = 0;
                    player.forced_move_ticks = 0;
                    player.state = PSTATE_NOT_MOVING;
                }
                logic_accumulator -= LOGIC_DT;
                continue;
            }

            if (player.state == PSTATE_MOVING) {
                advance_walk_anim(&player.intra_frame, &player.anim_frame);
                player.pixel_offset += 2;
                if (player.hop_active) {
                    if (player.hop_frames_remaining > 0) player.hop_frames_remaining--;
                    if (player.hop_frames_remaining == 0) player.hop_active = 0;
                }
                player.walk_counter--;
                if (player.walk_counter == 0) {
                    player.tile_x = player.target_x;
                    player.tile_y = player.target_y;
                    player.pixel_offset = 0;
                    player.hop_active = 0;
                    player.hop_frames_remaining = 0;
                    player.state = PSTATE_NOT_MOVING;

                    if (player.warp_cooldown == 0) {
                        int warp_idx = find_warp_index(&current,
                                                       player.tile_x,
                                                       player.tile_y);
                        if (warp_idx >= 0 &&
                            current.warps[warp_idx].warp_type != WARP_TYPE_CARPET &&
                            !player.stepped_from_warp) {
                            begin_fade_out(&fade, 1, warp_idx);
                        } else if (check_connection(&current,
                                                    player.tile_x,
                                                    player.tile_y)) {
                            do_map_transition(&current, &player, &paths);
                            overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);
                            reload_player_sprite(&player_ts, player_sprite_path,
                                                 REG_OBP0, current.sgb_pals[0]);
                            load_npc_sprites(&current, npc_ts, REG_OBP0,
                                             current.sgb_pals[0]);
                        }
                    }
                }
            } else {
                int input = read_input_direction();
                if (input < 0) {
                    player.anim_frame = 0;
                    player.intra_frame = 0;
                    player.state = PSTATE_NOT_MOVING;
                } else {
                    Direction d = (Direction)input;
                    int standing_idx = find_warp_index(&current,
                                                       player.tile_x,
                                                       player.tile_y);

                    if (standing_idx >= 0 &&
                        current.warps[standing_idx].warp_type == WARP_TYPE_CARPET &&
                        player.warp_cooldown == 0 &&
                        (current.warps[standing_idx].warp_dir == -1 ||
                         current.warps[standing_idx].warp_dir == (int)d)) {
                        begin_fade_out(&fade, 1, standing_idx);
                        logic_accumulator -= LOGIC_DT;
                        continue;
                    }

                    if (d != player.facing) {
                        player.facing = d;
                        player.state = PSTATE_NOT_MOVING;
                    } else {
                        int nx = player.tile_x + dir_dx(d);
                        int ny = player.tile_y + dir_dy(d);
                        WarpEvent *warp = find_warp(&current, nx, ny);
                        MapConnection *conn = check_connection(&current, nx, ny);
                        int lx = nx + dir_dx(d);
                        int ly = ny + dir_dy(d);

                        if (is_ledge_tile(&current, player.tile_x, player.tile_y,
                                          d, nx, ny) &&
                            can_walk_tile(&current, lx, ly) &&
                            !npc_occupies(&current, -1, lx, ly)) {
                            player.stepped_from_warp = 0;
                            begin_step(&player, d, lx, ly, HOP_STEP_FRAMES);
                            player.hop_active = 1;
                            player.hop_frames_remaining = HOP_STEP_FRAMES;
                        } else if (warp || conn ||
                                   (can_walk_tile(&current, nx, ny) &&
                                    !npc_occupies(&current, -1, nx, ny))) {
                            player.stepped_from_warp = (standing_idx >= 0);
                            begin_step(&player, d, nx, ny, WALK_STEP_FRAMES);
                        }
                    }
                }
            }
            logic_accumulator -= LOGIC_DT;
        }

        /* -------------------- Tile animations -------------------- */
        if (overworld_anims_active && flower_anim.type != ANIM_NONE) {
            while (anim_accumulator >= VBLANK_DT) {
                Tileset *ts = &current.tileset;
                if (!ts->texture.id) break;
                counter1++;
                if (counter1 >= 20) {
                    if (counter1 == 21) {
                        counter1 = 0;
                        int frame_index;
                        int sel = counter2 & 3;
                        if (sel < 2)       frame_index = 0;
                        else if (sel == 2) frame_index = 1;
                        else               frame_index = 2;
                        int tx = (flower_anim.target_tile % ts->tiles_wide) * TILE_SIZE;
                        int ty = (flower_anim.target_tile / ts->tiles_wide) * TILE_SIZE;
                        const uint8_t *fd = flower_anim.frame_data + frame_index * 16;
                        const SGBPalette *sgb = &sgb_super_palettes[current.sgb_pals[0]];
                        for (int y = 0; y < 8; y++) {
                            uint8_t lo = fd[y * 2], hi = fd[y * 2 + 1];
                            for (int x = 0; x < 8; x++)
                                ts->pixels[(ty + y) * ts->tex_w + (tx + x)] =
                                    decode_2bpp_bg_pixel(lo, hi, 7 - x, fade.current_bgp, sgb);
                        }
                        UpdateTexture(ts->texture, ts->pixels);
                    } else {
                        counter2 = (counter2 + 1) & 7;
                        int left = (counter2 & 4) != 0;
                        for (int i = 0; i < 16; i++) {
                            uint8_t b = water_anim.working_tile[i];
                            water_anim.working_tile[i] = left
                                ? (uint8_t)((b << 1) | (b >> 7))
                                : (uint8_t)((b >> 1) | (b << 7));
                        }
                        int tx = (water_anim.target_tile % ts->tiles_wide) * TILE_SIZE;
                        int ty = (water_anim.target_tile / ts->tiles_wide) * TILE_SIZE;
                        const uint8_t *fd = water_anim.working_tile;
                        const SGBPalette *sgb = &sgb_super_palettes[current.sgb_pals[0]];
                        for (int y = 0; y < 8; y++) {
                            uint8_t lo = fd[y * 2], hi = fd[y * 2 + 1];
                            for (int x = 0; x < 8; x++)
                                ts->pixels[(ty + y) * ts->tex_w + (tx + x)] =
                                    decode_2bpp_bg_pixel(lo, hi, 7 - x, fade.current_bgp, sgb);
                        }
                        UpdateTexture(ts->texture, ts->pixels);
                    }
                }
                anim_accumulator -= VBLANK_DT;
            }
        } else {
            anim_accumulator = 0.0f;
        }

        /* -------------------- Per-frame positions -------------------- */
        int base_x = player.tile_x * TILE_PIXEL_SIZE;
        int base_y = player.tile_y * TILE_PIXEL_SIZE;
        int interp_x = 0, interp_y = 0;
        if (player.state == PSTATE_MOVING) {
            interp_x = dir_dx(player.facing) * player.pixel_offset;
            interp_y = dir_dy(player.facing) * player.pixel_offset;
        }
        int player_px = base_x + interp_x;
        int player_py = base_y + interp_y;

        int hop_y_offset = 0;
        if (player.hop_active) {
            int t = HOP_STEP_FRAMES - player.hop_frames_remaining;
            hop_y_offset = -((t * (HOP_STEP_FRAMES - t)) * HOP_PEAK_PIXELS) /
                           ((HOP_STEP_FRAMES / 2) * (HOP_STEP_FRAMES / 2));
        }

        int cam_x = player_px + 8 - GB_WIDTH  / 2;
        int cam_y = player_py + 8 - GB_HEIGHT / 2;

        uint8_t standing_tile = tile_in_front_of_cell(&current, player.tile_x, player.tile_y);
        uint8_t grass_id = grass_tile_for(current.tileset_stem);
        player.grass_priority = (grass_id != 0xFF && standing_tile == grass_id) ? 1 : 0;

        for (int i = 0; i < current.num_npcs; i++) {
            NPC *n = &current.npcs[i];
            if (!n->active) { n->grass_priority = 0; continue; }
            uint8_t st = tile_in_front_of_cell(&current, n->tile_x, n->tile_y);
            n->grass_priority = (grass_id != 0xFF && st == grass_id) ? 1 : 0;
        }

        Tileset *ts = &current.tileset;
        const SGBPalette *sgb = &sgb_super_palettes[current.sgb_pals[0]];
        int sprite_top_y = player_py - cam_y + hop_y_offset;

        /* -------------------- Background layer -------------------- */
        BeginTextureMode(bg_layer);
        ClearBackground((Color){0, 0, 0, 0});
        if (ts->texture.id) {
            for (int by = -BORDER_MARGIN; by < current.map.height + BORDER_MARGIN; by++) {
                for (int bx = -BORDER_MARGIN; bx < current.map.width + BORDER_MARGIN; bx++) {
                    uint8_t block_id;
                    if (bx >= 0 && bx < current.map.width && by >= 0 && by < current.map.height) {
                        block_id = current.map.block_data[by * current.map.width + bx];
                    } else {
                        block_id = current.border_block;
                        for (int ci = 0; ci < current.num_connections; ci++) {
                            MapConnection *c = &current.connections[ci];
                            if (!c->loaded) continue;
                            if (c->dir == CONN_NORTH && by < 0) {
                                int cx = bx - c->offset, cy = c->map.height + by;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                    { block_id = c->map.block_data[cy * c->map.width + cx]; break; }
                            } else if (c->dir == CONN_SOUTH && by >= current.map.height) {
                                int cx = bx - c->offset, cy = by - current.map.height;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                    { block_id = c->map.block_data[cy * c->map.width + cx]; break; }
                            } else if (c->dir == CONN_WEST && bx < 0) {
                                int cx = c->map.width + bx, cy = by - c->offset;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                    { block_id = c->map.block_data[cy * c->map.width + cx]; break; }
                            } else if (c->dir == CONN_EAST && bx >= current.map.width) {
                                int cx = bx - current.map.width, cy = by - c->offset;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                    { block_id = c->map.block_data[cy * c->map.width + cx]; break; }
                            }
                        }
                    }
                    if (block_id >= current.num_blocks) continue;

                    BlockDef *def = &current.blocks[block_id];
                    for (int ty = 0; ty < 4; ty++) {
                        for (int tx = 0; tx < 4; tx++) {
                            uint8_t tile_id = def->tile_ids[ty * 4 + tx];
                            Rectangle src = {
                                (tile_id % ts->tiles_wide) * TILE_SIZE,
                                (tile_id / ts->tiles_wide) * TILE_SIZE,
                                TILE_SIZE, TILE_SIZE
                            };
                            Vector2 dest = {
                                (bx * BLOCK_PIXEL_SIZE) + (tx * TILE_SIZE) - cam_x,
                                (by * BLOCK_PIXEL_SIZE) + (ty * TILE_SIZE) - cam_y
                            };
                            DrawTextureRec(ts->texture, src, dest, WHITE);
                        }
                    }
                }
            }
        }
        EndTextureMode();

        /* -------------------- Sprite layer -------------------- */
        BeginTextureMode(sprite_layer);
        ClearBackground((Color){0, 0, 0, 0});

        if (player.hop_active)
            draw_shadow(shadow_tex, player_px - cam_x, player_py - cam_y + 8);

        for (int i = 0; i < current.num_npcs; i++) {
            NPC *n = &current.npcs[i];
            if (!n->active) continue;
            if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
            Tileset *nts = &npc_ts[n->sprite_id];
            if (!nts->texture.id) continue;

            int anim_idx = anim_table[n->facing][n->anim_frame];
            const uint8_t *tiles = sprite_frames[anim_idx];
            int flip = anim_flip[n->facing][n->anim_frame];
            int nx_px = n->tile_x * TILE_PIXEL_SIZE + dir_dx(n->facing) * n->pixel_offset;
            int ny_px = n->tile_y * TILE_PIXEL_SIZE + dir_dy(n->facing) * n->pixel_offset;
            draw_sprite(nts, tiles, flip, nx_px - cam_x, ny_px - cam_y);
        }

        if (player_ts.texture.id) {
            int anim_idx = anim_table[player.facing][player.anim_frame];
            const uint8_t *tiles = sprite_frames[anim_idx];
            int flip = anim_flip[player.facing][player.anim_frame];
            draw_sprite(&player_ts, tiles, flip, player_px - cam_x, sprite_top_y);
        }
        EndTextureMode();

        /* -------------------- Priority mask -------------------- */
        BeginTextureMode(priority_layer);
        ClearBackground((Color){0, 0, 0, 255});

        for (int i = 0; i < current.num_npcs; i++) {
            NPC *n = &current.npcs[i];
            if (!n->active) continue;
            if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
            Tileset *nts = &npc_ts[n->sprite_id];
            if (!nts->texture.id) continue;

            int anim_idx = anim_table[n->facing][n->anim_frame];
            const uint8_t *tiles = sprite_frames[anim_idx];
            int flip = anim_flip[n->facing][n->anim_frame];
            int nx_px = n->tile_x * TILE_PIXEL_SIZE + dir_dx(n->facing) * n->pixel_offset;
            int ny_px = n->tile_y * TILE_PIXEL_SIZE + dir_dy(n->facing) * n->pixel_offset;
            int px = nx_px - cam_x, py = ny_px - cam_y;

            for (int q = 0; q < 4; q++) {
                float isprio = (n->grass_priority && (q & 2)) ? 1.0f : 0.0f;
                BeginShaderMode(prio_write_shader);
                SetShaderValue(prio_write_shader, loc_pw_isprio, &isprio, SHADER_UNIFORM_FLOAT);

                int src_i = flip ? (q ^ 1) : q;
                uint8_t tid = tiles[src_i];
                Rectangle src = sprite_quadrant_src(nts, tid, flip);
                int ox = (q & 1) ? 8 : 0;
                int oy = (q & 2) ? 8 : 0;
                Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
                DrawTexturePro(nts->texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
                EndShaderMode();
            }
        }

        if (player_ts.texture.id) {
            int anim_idx = anim_table[player.facing][player.anim_frame];
            const uint8_t *tiles = sprite_frames[anim_idx];
            int flip = anim_flip[player.facing][player.anim_frame];
            int px = player_px - cam_x, py = sprite_top_y;

            for (int q = 0; q < 4; q++) {
                float isprio = (player.grass_priority && (q & 2)) ? 1.0f : 0.0f;
                BeginShaderMode(prio_write_shader);
                SetShaderValue(prio_write_shader, loc_pw_isprio, &isprio, SHADER_UNIFORM_FLOAT);

                int src_i = flip ? (q ^ 1) : q;
                uint8_t tid = tiles[src_i];
                Rectangle src = sprite_quadrant_src(&player_ts, tid, flip);
                int ox = (q & 1) ? 8 : 0;
                int oy = (q & 2) ? 8 : 0;
                Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
                DrawTexturePro(player_ts.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
                EndShaderMode();
            }
        }
        EndTextureMode();

        /* -------------------- Composite -------------------- */
        int shade_idx = reg_shade(fade.current_bgp, 0);
        Color bg_col = sgb_color_to_rgba(sgb->colors[shade_idx]);
        Vector3 bg_col_f = { bg_col.r / 255.0f, bg_col.g / 255.0f, bg_col.b / 255.0f };
        SetShaderValue(priority_shader, loc_bg_color, &bg_col_f, SHADER_UNIFORM_VEC3);

        BeginTextureMode(target);
        if (fade.hide_world) {
            //ClearBackground(WHITE);
        } else {
            ClearBackground((Color){0, 0, 0, 0});
            BeginShaderMode(priority_shader);
            SetShaderValueTexture(priority_shader, loc_bg_tex,   bg_layer.texture);
            SetShaderValueTexture(priority_shader, loc_spr_tex,  sprite_layer.texture);
            SetShaderValueTexture(priority_shader, loc_prio_tex, priority_layer.texture);
            DrawTexturePro(white_tex,
                           (Rectangle){0, 0, 1, 1},
                           (Rectangle){0, 0, (float)GB_WIDTH, (float)GB_HEIGHT},
                           (Vector2){0, 0}, 0.0f, WHITE);
            EndShaderMode();
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src_rect = { 0, 0, (float)GB_WIDTH, (float)GB_HEIGHT };
        Rectangle dst_rect = { 0, 0, (float)(GB_WIDTH * SCALE), (float)(GB_HEIGHT * SCALE) };
        DrawTexturePro(target.texture, src_rect, dst_rect, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    /* -------------------- Cleanup -------------------- */
    if (player_ts.texture.id) UnloadTexture(player_ts.texture);
    if (player_ts.pixels) free(player_ts.pixels);
    for (int i = 0; i < NUM_NPC_SPRITES; i++) {
        if (npc_ts[i].texture.id) UnloadTexture(npc_ts[i].texture);
        if (npc_ts[i].pixels) free(npc_ts[i].pixels);
    }
    if (shadow_tex.id) UnloadTexture(shadow_tex);
    if (flower_anim.frame_data) free(flower_anim.frame_data);
    if (water_anim.working_tile) free(water_anim.working_tile);
    free_map_gpu(&current);
    free_map_data(&current);

    UnloadShader(priority_shader);
    UnloadShader(prio_write_shader);
    UnloadTexture(white_tex);
    UnloadRenderTexture(priority_layer);
    UnloadRenderTexture(sprite_layer);
    UnloadRenderTexture(bg_layer);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}