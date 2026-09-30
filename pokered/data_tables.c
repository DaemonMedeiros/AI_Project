#include "data_tables.h"
#include <string.h>

const CollisionLabel collision_labels[] = {
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

const TilesetAlias tileset_aliases[] = {
    {"reds_house_1", "reds_house"},
    {"reds_house_2", "reds_house"},
    {"dojo", "gym"},
    {"mart", "pokecenter"},
    {"forest_gate", "gate"},
    {NULL, NULL}
};

const WarpTileList warp_tile_lists[] = {
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

const GrassTileEntry grass_tiles[] = {
    {"overworld", 0x52},
    {"forest",    0x20},
    {"plateau",   0x45},
    {NULL, 0},
};

const uint8_t carpet_down[] = { 0x01, 0x12, 0x17, 0x3D, 0x04, 0x18, 0x33, 0xFF };
const uint8_t carpet_up[] = { 0x01, 0x5C, 0xFF };
const uint8_t carpet_left[] = { 0x1A, 0x4B, 0xFF };
const uint8_t carpet_right[] = { 0x0F, 0x4E, 0xFF };

const LedgeEntry ledge_tiles[NUM_LEDGE_TILES] = {
    {DIR_DOWN,  0x2C, 0x37},
    {DIR_DOWN,  0x39, 0x36},
    {DIR_DOWN,  0x39, 0x37},
    {DIR_LEFT,  0x2C, 0x27},
    {DIR_LEFT,  0x39, 0x27},
    {DIR_RIGHT, 0x2C, 0x0D},
    {DIR_RIGHT, 0x2C, 0x1D},
    {DIR_RIGHT, 0x39, 0x0D},
};

const uint8_t sprite_frames[6][4] = {
    {0x00, 0x01, 0x02, 0x03},
    {0x80, 0x81, 0x82, 0x83},
    {0x04, 0x05, 0x06, 0x07},
    {0x84, 0x85, 0x86, 0x87},
    {0x08, 0x09, 0x0a, 0x0b},
    {0x88, 0x89, 0x8a, 0x8b},
};

const int anim_table[4][4] = {
    {0, 1, 0, 1},
    {2, 3, 2, 3},
    {4, 5, 4, 5},
    {4, 5, 4, 5},
};

const int anim_flip[4][4] = {
    {0, 0, 0, 1},
    {0, 0, 0, 1},
    {0, 0, 0, 0},
    {1, 1, 1, 1},
};

const char* npc_sprite_files[NUM_NPC_SPRITES] = {
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

const char* lookup_collision_label(const char* stem) {
    for (int i = 0; collision_labels[i].stem; i++)
        if (strcmp(collision_labels[i].stem, stem) == 0)
            return collision_labels[i].label;
    return "Overworld_Coll";
}

uint8_t grass_tile_for(const char* stem) {
    for (int i = 0; grass_tiles[i].stem; i++)
        if (strcmp(grass_tiles[i].stem, stem) == 0)
            return grass_tiles[i].grass_tile;
    return 0xFF;
}

int list_has8(const uint8_t* ids, uint8_t tid) {
    for (int i = 0; ids[i] != 0 && i < 8; i++)
        if (ids[i] == tid) return 1;
    return 0;
}

int list_has_term(const uint8_t* ids, uint8_t tid) {
    for (int i = 0; ids[i] != 0xFF; i++)
        if (ids[i] == tid) return 1;
    return 0;
}

int is_warp_tile(const char* stem, uint8_t tid) {
    for (int i = 0; warp_tile_lists[i].stem; i++)
        if (strcmp(warp_tile_lists[i].stem, stem) == 0)
            return list_has8(warp_tile_lists[i].warp_ids, tid);
    return 0;
}

int is_door_tile(const char* stem, uint8_t tid) {
    for (int i = 0; warp_tile_lists[i].stem; i++)
        if (strcmp(warp_tile_lists[i].stem, stem) == 0)
            return list_has8(warp_tile_lists[i].door_ids, tid);
    return 0;
}

int sprite_id_from_name(const char* name) {
    static const char* names[] = {
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

int dir_dx(Direction d) { return (d == DIR_LEFT) ? -1 : (d == DIR_RIGHT) ? 1 : 0; }
int dir_dy(Direction d) { return (d == DIR_UP) ? -1 : (d == DIR_DOWN) ? 1 : 0; }
