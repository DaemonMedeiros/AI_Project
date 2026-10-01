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

int sprite_is_static(int sprite_id) {
    return sprite_id > 60;
}

/* toggle_entries.h
   Generated from constants/toggle_constants.asm.
   object_index is the TOGGLE_* constant value, which corresponds to the
   object's position in the map's def_object_events list. initially_on:
   1 = visible at game start, 0 = hidden until a script reveals it. */

const ToggleEntry toggle_entries[] = {
    /* PALLET_TOWN */
    { "PALLET_TOWN", 0x00, 0 },  /* TOGGLE_PALLET_TOWN_OAK */

    /* VIRIDIAN_CITY */
    { "VIRIDIAN_CITY", 0x01, 1 }, /* TOGGLE_LYING_OLD_MAN */
    { "VIRIDIAN_CITY", 0x02, 0 }, /* TOGGLE_OLD_MAN */

    /* PEWTER_CITY */
    { "PEWTER_CITY", 0x03, 1 },   /* TOGGLE_MUSEUM_GUY */
    { "PEWTER_CITY", 0x04, 1 },   /* TOGGLE_GYM_GUY */

    /* CERULEAN_CITY */
    { "CERULEAN_CITY", 0x05, 0 }, /* TOGGLE_CERULEAN_RIVAL */
    { "CERULEAN_CITY", 0x06, 1 }, /* TOGGLE_CERULEAN_ROCKET */
    { "CERULEAN_CITY", 0x07, 0 }, /* TOGGLE_CERULEAN_GUARD_1 */
    { "CERULEAN_CITY", 0x08, 1 }, /* TOGGLE_CERULEAN_CAVE_GUY */
    { "CERULEAN_CITY", 0x09, 1 }, /* TOGGLE_CERULEAN_GUARD_2 */

    /* SAFFRON_CITY */
    { "SAFFRON_CITY", 0x0A, 1 },  /* TOGGLE_SAFFRON_CITY_1 */
    { "SAFFRON_CITY", 0x0B, 1 },  /* TOGGLE_SAFFRON_CITY_2 */
    { "SAFFRON_CITY", 0x0C, 1 },  /* TOGGLE_SAFFRON_CITY_3 */
    { "SAFFRON_CITY", 0x0D, 1 },  /* TOGGLE_SAFFRON_CITY_4 */
    { "SAFFRON_CITY", 0x0E, 1 },  /* TOGGLE_SAFFRON_CITY_5 */
    { "SAFFRON_CITY", 0x0F, 1 },  /* TOGGLE_SAFFRON_CITY_6 */
    { "SAFFRON_CITY", 0x10, 1 },  /* TOGGLE_SAFFRON_CITY_7 */
    { "SAFFRON_CITY", 0x11, 0 },  /* TOGGLE_SAFFRON_CITY_8 */
    { "SAFFRON_CITY", 0x12, 0 },  /* TOGGLE_SAFFRON_CITY_9 */
    { "SAFFRON_CITY", 0x13, 0 },  /* TOGGLE_SAFFRON_CITY_A */
    { "SAFFRON_CITY", 0x14, 0 },  /* TOGGLE_SAFFRON_CITY_B */
    { "SAFFRON_CITY", 0x15, 0 },  /* TOGGLE_SAFFRON_CITY_C */
    { "SAFFRON_CITY", 0x16, 1 },  /* TOGGLE_SAFFRON_CITY_D */
    { "SAFFRON_CITY", 0x17, 0 },  /* TOGGLE_SAFFRON_CITY_E */
    { "SAFFRON_CITY", 0x18, 0 },  /* TOGGLE_SAFFRON_CITY_F */

    /* ROUTE_2 */
    { "ROUTE_2", 0x19, 1 },       /* TOGGLE_ROUTE_2_ITEM_1 */
    { "ROUTE_2", 0x1A, 1 },       /* TOGGLE_ROUTE_2_ITEM_2 */

    /* ROUTE_4 */
    { "ROUTE_4", 0x1B, 1 },       /* TOGGLE_ROUTE_4_ITEM */

    /* ROUTE_9 */
    { "ROUTE_9", 0x1C, 1 },       /* TOGGLE_ROUTE_9_ITEM */

    /* ROUTE_12 */
    { "ROUTE_12", 0x1D, 1 },      /* TOGGLE_ROUTE_12_SNORLAX */
    { "ROUTE_12", 0x1E, 1 },      /* TOGGLE_ROUTE_12_ITEM_1 */
    { "ROUTE_12", 0x1F, 1 },      /* TOGGLE_ROUTE_12_ITEM_2 */

    /* ROUTE_15 */
    { "ROUTE_15", 0x20, 1 },      /* TOGGLE_ROUTE_15_ITEM */

    /* ROUTE_16 */
    { "ROUTE_16", 0x21, 1 },      /* TOGGLE_ROUTE_16_SNORLAX */

    /* ROUTE_22 */
    { "ROUTE_22", 0x22, 0 },      /* TOGGLE_ROUTE_22_RIVAL_1 */
    { "ROUTE_22", 0x23, 0 },      /* TOGGLE_ROUTE_22_RIVAL_2 */

    /* ROUTE_24 */
    { "ROUTE_24", 0x24, 1 },      /* TOGGLE_NUGGET_BRIDGE_GUY */
    { "ROUTE_24", 0x25, 1 },      /* TOGGLE_ROUTE_24_ITEM */

    /* ROUTE_25 */
    { "ROUTE_25", 0x26, 1 },      /* TOGGLE_ROUTE_25_ITEM */

    /* BLUES_HOUSE */
    { "BLUES_HOUSE", 0x27, 1 },   /* TOGGLE_DAISY_SITTING */
    { "BLUES_HOUSE", 0x28, 0 },   /* TOGGLE_DAISY_WALKING */
    { "BLUES_HOUSE", 0x29, 1 },   /* TOGGLE_TOWN_MAP */

    /* OAKS_LAB */
    { "OAKS_LAB", 0x2A, 1 },      /* TOGGLE_OAKS_LAB_RIVAL */
    { "OAKS_LAB", 0x2B, 1 },      /* TOGGLE_STARTER_BALL_1 */
    { "OAKS_LAB", 0x2C, 1 },      /* TOGGLE_STARTER_BALL_2 */
    { "OAKS_LAB", 0x2D, 1 },      /* TOGGLE_STARTER_BALL_3 */
    { "OAKS_LAB", 0x2E, 0 },      /* TOGGLE_OAKS_LAB_OAK_1 */
    { "OAKS_LAB", 0x2F, 1 },      /* TOGGLE_POKEDEX_1 */
    { "OAKS_LAB", 0x30, 1 },      /* TOGGLE_POKEDEX_2 */
    { "OAKS_LAB", 0x31, 0 },      /* TOGGLE_OAKS_LAB_OAK_2 */

    /* VIRIDIAN_GYM */
    { "VIRIDIAN_GYM", 0x32, 1 },  /* TOGGLE_VIRIDIAN_GYM_GIOVANNI */
    { "VIRIDIAN_GYM", 0x33, 1 },  /* TOGGLE_VIRIDIAN_GYM_ITEM */

    /* MUSEUM_1F */
    { "MUSEUM_1F", 0x34, 1 },     /* TOGGLE_OLD_AMBER */

    /* CERULEAN_CAVE_1F */
    { "CERULEAN_CAVE_1F", 0x35, 1 }, /* TOGGLE_CERULEAN_CAVE_1F_ITEM_1 */
    { "CERULEAN_CAVE_1F", 0x36, 1 }, /* TOGGLE_CERULEAN_CAVE_1F_ITEM_2 */
    { "CERULEAN_CAVE_1F", 0x37, 1 }, /* TOGGLE_CERULEAN_CAVE_1F_ITEM_3 */

    /* POKEMON_TOWER_2F */
    { "POKEMON_TOWER_2F", 0x38, 1 }, /* TOGGLE_POKEMON_TOWER_2F_RIVAL */

    /* POKEMON_TOWER_3F */
    { "POKEMON_TOWER_3F", 0x39, 1 }, /* TOGGLE_POKEMON_TOWER_3F_ITEM */

    /* POKEMON_TOWER_4F */
    { "POKEMON_TOWER_4F", 0x3A, 1 }, /* TOGGLE_POKEMON_TOWER_4F_ITEM_1 */
    { "POKEMON_TOWER_4F", 0x3B, 1 }, /* TOGGLE_POKEMON_TOWER_4F_ITEM_2 */
    { "POKEMON_TOWER_4F", 0x3C, 1 }, /* TOGGLE_POKEMON_TOWER_4F_ITEM_3 */

    /* POKEMON_TOWER_5F */
    { "POKEMON_TOWER_5F", 0x3D, 1 }, /* TOGGLE_POKEMON_TOWER_5F_ITEM */

    /* POKEMON_TOWER_6F */
    { "POKEMON_TOWER_6F", 0x3E, 1 }, /* TOGGLE_POKEMON_TOWER_6F_ITEM_1 */
    { "POKEMON_TOWER_6F", 0x3F, 1 }, /* TOGGLE_POKEMON_TOWER_6F_ITEM_2 */

    /* POKEMON_TOWER_7F */
    { "POKEMON_TOWER_7F", 0x40, 1 }, /* TOGGLE_POKEMON_TOWER_7F_ROCKET_1 */
    { "POKEMON_TOWER_7F", 0x41, 1 }, /* TOGGLE_POKEMON_TOWER_7F_ROCKET_2 */
    { "POKEMON_TOWER_7F", 0x42, 1 }, /* TOGGLE_POKEMON_TOWER_7F_ROCKET_3 */
    { "POKEMON_TOWER_7F", 0x43, 1 }, /* TOGGLE_POKEMON_TOWER_7F_MR_FUJI */

    /* MR_FUJIS_HOUSE */
    { "MR_FUJIS_HOUSE", 0x44, 0 }, /* TOGGLE_MR_FUJIS_HOUSE_MR_FUJI */

    /* CELADON_MANSION_ROOF_HOUSE */
    { "CELADON_MANSION_ROOF_HOUSE", 0x45, 1 }, /* TOGGLE_CELADON_MANSION_EEVEE_GIFT */

    /* GAME_CORNER */
    { "GAME_CORNER", 0x46, 1 },   /* TOGGLE_GAME_CORNER_ROCKET */

    /* WARDENS_HOUSE */
    { "WARDENS_HOUSE", 0x47, 1 }, /* TOGGLE_WARDENS_HOUSE_ITEM */

    /* POKEMON_MANSION_1F */
    { "POKEMON_MANSION_1F", 0x48, 1 }, /* TOGGLE_POKEMON_MANSION_1F_ITEM_1 */
    { "POKEMON_MANSION_1F", 0x49, 1 }, /* TOGGLE_POKEMON_MANSION_1F_ITEM_2 */

    /* FIGHTING_DOJO */
    { "FIGHTING_DOJO", 0x4A, 1 }, /* TOGGLE_FIGHTING_DOJO_GIFT_1 */
    { "FIGHTING_DOJO", 0x4B, 1 }, /* TOGGLE_FIGHTING_DOJO_GIFT_2 */

    /* SILPH_CO_1F */
    { "SILPH_CO_1F", 0x4C, 0 },   /* TOGGLE_SILPH_CO_1F_RECEPTIONIST */

    /* POWER_PLANT */
    { "POWER_PLANT", 0x4D, 1 },   /* TOGGLE_VOLTORB_1 */
    { "POWER_PLANT", 0x4E, 1 },   /* TOGGLE_VOLTORB_2 */
    { "POWER_PLANT", 0x4F, 1 },   /* TOGGLE_VOLTORB_3 */
    { "POWER_PLANT", 0x50, 1 },   /* TOGGLE_ELECTRODE_1 */
    { "POWER_PLANT", 0x51, 1 },   /* TOGGLE_VOLTORB_4 */
    { "POWER_PLANT", 0x52, 1 },   /* TOGGLE_VOLTORB_5 */
    { "POWER_PLANT", 0x53, 1 },   /* TOGGLE_ELECTRODE_2 */
    { "POWER_PLANT", 0x54, 1 },   /* TOGGLE_VOLTORB_6 */
    { "POWER_PLANT", 0x55, 1 },   /* TOGGLE_ZAPDOS */
    { "POWER_PLANT", 0x56, 1 },   /* TOGGLE_POWER_PLANT_ITEM_1 */
    { "POWER_PLANT", 0x57, 1 },   /* TOGGLE_POWER_PLANT_ITEM_2 */
    { "POWER_PLANT", 0x58, 1 },   /* TOGGLE_POWER_PLANT_ITEM_3 */
    { "POWER_PLANT", 0x59, 1 },   /* TOGGLE_POWER_PLANT_ITEM_4 */
    { "POWER_PLANT", 0x5A, 1 },   /* TOGGLE_POWER_PLANT_ITEM_5 */

    /* VICTORY_ROAD_2F */
    { "VICTORY_ROAD_2F", 0x5B, 1 }, /* TOGGLE_MOLTRES */
    { "VICTORY_ROAD_2F", 0x5C, 1 }, /* TOGGLE_VICTORY_ROAD_2F_ITEM_1 */
    { "VICTORY_ROAD_2F", 0x5D, 1 }, /* TOGGLE_VICTORY_ROAD_2F_ITEM_2 */
    { "VICTORY_ROAD_2F", 0x5E, 1 }, /* TOGGLE_VICTORY_ROAD_2F_ITEM_3 */
    { "VICTORY_ROAD_2F", 0x5F, 1 }, /* TOGGLE_VICTORY_ROAD_2F_ITEM_4 */
    { "VICTORY_ROAD_2F", 0x60, 1 }, /* TOGGLE_VICTORY_ROAD_2F_BOULDER */

    /* BILLS_HOUSE */
    { "BILLS_HOUSE", 0x61, 1 },   /* TOGGLE_BILL_POKEMON */
    { "BILLS_HOUSE", 0x62, 0 },   /* TOGGLE_BILL_1 */
    { "BILLS_HOUSE", 0x63, 0 },   /* TOGGLE_BILL_2 */

    /* VIRIDIAN_FOREST */
    { "VIRIDIAN_FOREST", 0x64, 1 }, /* TOGGLE_VIRIDIAN_FOREST_ITEM_1 */
    { "VIRIDIAN_FOREST", 0x65, 1 }, /* TOGGLE_VIRIDIAN_FOREST_ITEM_2 */
    { "VIRIDIAN_FOREST", 0x66, 1 }, /* TOGGLE_VIRIDIAN_FOREST_ITEM_3 */

    /* MT_MOON_1F */
    { "MT_MOON_1F", 0x67, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_1 */
    { "MT_MOON_1F", 0x68, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_2 */
    { "MT_MOON_1F", 0x69, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_3 */
    { "MT_MOON_1F", 0x6A, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_4 */
    { "MT_MOON_1F", 0x6B, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_5 */
    { "MT_MOON_1F", 0x6C, 1 },    /* TOGGLE_MT_MOON_1F_ITEM_6 */

    /* MT_MOON_B2F */
    { "MT_MOON_B2F", 0x6D, 1 },   /* TOGGLE_MT_MOON_B2F_FOSSIL_1 */
    { "MT_MOON_B2F", 0x6E, 1 },   /* TOGGLE_MT_MOON_B2F_FOSSIL_2 */
    { "MT_MOON_B2F", 0x6F, 1 },   /* TOGGLE_MT_MOON_B2F_ITEM_1 */
    { "MT_MOON_B2F", 0x70, 1 },   /* TOGGLE_MT_MOON_B2F_ITEM_2 */

    /* SS_ANNE_2F */
    { "SS_ANNE_2F", 0x71, 0 },    /* TOGGLE_SS_ANNE_2F_RIVAL */

    /* SS_ANNE_1F_ROOMS */
    { "SS_ANNE_1F_ROOMS", 0x72, 1 }, /* TOGGLE_SS_ANNE_1F_ROOMS_ITEM */

    /* SS_ANNE_2F_ROOMS */
    { "SS_ANNE_2F_ROOMS", 0x73, 1 }, /* TOGGLE_SS_ANNE_2F_ROOMS_ITEM_1 */
    { "SS_ANNE_2F_ROOMS", 0x74, 1 }, /* TOGGLE_SS_ANNE_2F_ROOMS_ITEM_2 */

    /* SS_ANNE_B1F_ROOMS */
    { "SS_ANNE_B1F_ROOMS", 0x75, 1 }, /* TOGGLE_SS_ANNE_B1F_ROOMS_ITEM_1 */
    { "SS_ANNE_B1F_ROOMS", 0x76, 1 }, /* TOGGLE_SS_ANNE_B1F_ROOMS_ITEM_2 */
    { "SS_ANNE_B1F_ROOMS", 0x77, 1 }, /* TOGGLE_SS_ANNE_B1F_ROOMS_ITEM_3 */

    /* VICTORY_ROAD_3F */
    { "VICTORY_ROAD_3F", 0x78, 1 }, /* TOGGLE_VICTORY_ROAD_3F_ITEM_1 */
    { "VICTORY_ROAD_3F", 0x79, 1 }, /* TOGGLE_VICTORY_ROAD_3F_ITEM_2 */
    { "VICTORY_ROAD_3F", 0x7A, 1 }, /* TOGGLE_VICTORY_ROAD_3F_BOULDER */

    /* ROCKET_HIDEOUT_B1F */
    { "ROCKET_HIDEOUT_B1F", 0x7B, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B1F_ITEM_1 */
    { "ROCKET_HIDEOUT_B1F", 0x7C, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B1F_ITEM_2 */

    /* ROCKET_HIDEOUT_B2F */
    { "ROCKET_HIDEOUT_B2F", 0x7D, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B2F_ITEM_1 */
    { "ROCKET_HIDEOUT_B2F", 0x7E, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B2F_ITEM_2 */
    { "ROCKET_HIDEOUT_B2F", 0x7F, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B2F_ITEM_3 */
    { "ROCKET_HIDEOUT_B2F", 0x80, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B2F_ITEM_4 */

    /* ROCKET_HIDEOUT_B3F */
    { "ROCKET_HIDEOUT_B3F", 0x81, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B3F_ITEM_1 */
    { "ROCKET_HIDEOUT_B3F", 0x82, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B3F_ITEM_2 */

    /* ROCKET_HIDEOUT_B4F */
    { "ROCKET_HIDEOUT_B4F", 0x83, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_GIOVANNI */
    { "ROCKET_HIDEOUT_B4F", 0x84, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_ITEM_1 */
    { "ROCKET_HIDEOUT_B4F", 0x85, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_ITEM_2 */
    { "ROCKET_HIDEOUT_B4F", 0x86, 1 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_ITEM_3 */
    { "ROCKET_HIDEOUT_B4F", 0x87, 0 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_ITEM_4 */
    { "ROCKET_HIDEOUT_B4F", 0x88, 0 }, /* TOGGLE_ROCKET_HIDEOUT_B4F_ITEM_5 */

    /* SILPH_CO_2F */
    { "SILPH_CO_2F", 0x89, 1 },   /* TOGGLE_SILPH_CO_2F_1 */
    { "SILPH_CO_2F", 0x8A, 1 },   /* TOGGLE_SILPH_CO_2F_2 */
    { "SILPH_CO_2F", 0x8B, 1 },   /* TOGGLE_SILPH_CO_2F_3 */
    { "SILPH_CO_2F", 0x8C, 1 },   /* TOGGLE_SILPH_CO_2F_4 */
    { "SILPH_CO_2F", 0x8D, 1 },   /* TOGGLE_SILPH_CO_2F_5 */

    /* SILPH_CO_3F */
    { "SILPH_CO_3F", 0x8E, 1 },   /* TOGGLE_SILPH_CO_3F_1 */
    { "SILPH_CO_3F", 0x8F, 1 },   /* TOGGLE_SILPH_CO_3F_2 */
    { "SILPH_CO_3F", 0x90, 1 },   /* TOGGLE_SILPH_CO_3F_ITEM */

    /* SILPH_CO_4F */
    { "SILPH_CO_4F", 0x91, 1 },   /* TOGGLE_SILPH_CO_4F_1 */
    { "SILPH_CO_4F", 0x92, 1 },   /* TOGGLE_SILPH_CO_4F_2 */
    { "SILPH_CO_4F", 0x93, 1 },   /* TOGGLE_SILPH_CO_4F_3 */
    { "SILPH_CO_4F", 0x94, 1 },   /* TOGGLE_SILPH_CO_4F_ITEM_1 */
    { "SILPH_CO_4F", 0x95, 1 },   /* TOGGLE_SILPH_CO_4F_ITEM_2 */
    { "SILPH_CO_4F", 0x96, 1 },   /* TOGGLE_SILPH_CO_4F_ITEM_3 */

    /* SILPH_CO_5F */
    { "SILPH_CO_5F", 0x97, 1 },   /* TOGGLE_SILPH_CO_5F_1 */
    { "SILPH_CO_5F", 0x98, 1 },   /* TOGGLE_SILPH_CO_5F_2 */
    { "SILPH_CO_5F", 0x99, 1 },   /* TOGGLE_SILPH_CO_5F_3 */
    { "SILPH_CO_5F", 0x9A, 1 },   /* TOGGLE_SILPH_CO_5F_4 */
    { "SILPH_CO_5F", 0x9B, 1 },   /* TOGGLE_SILPH_CO_5F_ITEM_1 */
    { "SILPH_CO_5F", 0x9C, 1 },   /* TOGGLE_SILPH_CO_5F_ITEM_2 */
    { "SILPH_CO_5F", 0x9D, 1 },   /* TOGGLE_SILPH_CO_5F_ITEM_3 */

    /* SILPH_CO_6F */
    { "SILPH_CO_6F", 0x9E, 1 },   /* TOGGLE_SILPH_CO_6F_1 */
    { "SILPH_CO_6F", 0x9F, 1 },   /* TOGGLE_SILPH_CO_6F_2 */
    { "SILPH_CO_6F", 0xA0, 1 },   /* TOGGLE_SILPH_CO_6F_3 */
    { "SILPH_CO_6F", 0xA1, 1 },   /* TOGGLE_SILPH_CO_6F_ITEM_1 */
    { "SILPH_CO_6F", 0xA2, 1 },   /* TOGGLE_SILPH_CO_6F_ITEM_2 */

    /* SILPH_CO_7F */
    { "SILPH_CO_7F", 0xA3, 1 },   /* TOGGLE_SILPH_CO_7F_1 */
    { "SILPH_CO_7F", 0xA4, 1 },   /* TOGGLE_SILPH_CO_7F_2 */
    { "SILPH_CO_7F", 0xA5, 1 },   /* TOGGLE_SILPH_CO_7F_3 */
    { "SILPH_CO_7F", 0xA6, 1 },   /* TOGGLE_SILPH_CO_7F_4 */
    { "SILPH_CO_7F", 0xA7, 1 },   /* TOGGLE_SILPH_CO_7F_RIVAL */
    { "SILPH_CO_7F", 0xA8, 1 },   /* TOGGLE_SILPH_CO_7F_ITEM_1 */
    { "SILPH_CO_7F", 0xA9, 1 },   /* TOGGLE_SILPH_CO_7F_ITEM_2 */
    { "SILPH_CO_7F", 0xAA, 1 },   /* TOGGLE_SILPH_CO_7F_8 */

    /* SILPH_CO_8F */
    { "SILPH_CO_8F", 0xAB, 1 },   /* TOGGLE_SILPH_CO_8F_1 */
    { "SILPH_CO_8F", 0xAC, 1 },   /* TOGGLE_SILPH_CO_8F_2 */
    { "SILPH_CO_8F", 0xAD, 1 },   /* TOGGLE_SILPH_CO_8F_3 */

    /* SILPH_CO_9F */
    { "SILPH_CO_9F", 0xAE, 1 },   /* TOGGLE_SILPH_CO_9F_1 */
    { "SILPH_CO_9F", 0xAF, 1 },   /* TOGGLE_SILPH_CO_9F_2 */
    { "SILPH_CO_9F", 0xB0, 1 },   /* TOGGLE_SILPH_CO_9F_3 */

    /* SILPH_CO_10F */
    { "SILPH_CO_10F", 0xB1, 1 },  /* TOGGLE_SILPH_CO_10F_1 */
    { "SILPH_CO_10F", 0xB2, 1 },  /* TOGGLE_SILPH_CO_10F_2 */
    { "SILPH_CO_10F", 0xB3, 1 },  /* TOGGLE_SILPH_CO_10F_3 */
    { "SILPH_CO_10F", 0xB4, 1 },  /* TOGGLE_SILPH_CO_10F_ITEM_1 */
    { "SILPH_CO_10F", 0xB5, 1 },  /* TOGGLE_SILPH_CO_10F_ITEM_2 */
    { "SILPH_CO_10F", 0xB6, 1 },  /* TOGGLE_SILPH_CO_10F_ITEM_3 */

    /* SILPH_CO_11F */
    { "SILPH_CO_11F", 0xB7, 1 },  /* TOGGLE_SILPH_CO_11F_1 */
    { "SILPH_CO_11F", 0xB8, 1 },  /* TOGGLE_SILPH_CO_11F_2 */
    { "SILPH_CO_11F", 0xB9, 1 },  /* TOGGLE_SILPH_CO_11F_3 */

    /* UNUSED_MAP_F4 */
    { "UNUSED_MAP_F4", 0xBA, 1 }, /* TOGGLE_UNUSED_MAP_F4_1 */

    /* POKEMON_MANSION_2F */
    { "POKEMON_MANSION_2F", 0xBB, 1 }, /* TOGGLE_POKEMON_MANSION_2F_ITEM */

    /* POKEMON_MANSION_3F */
    { "POKEMON_MANSION_3F", 0xBC, 1 }, /* TOGGLE_POKEMON_MANSION_3F_ITEM_1 */
    { "POKEMON_MANSION_3F", 0xBD, 1 }, /* TOGGLE_POKEMON_MANSION_3F_ITEM_2 */

    /* POKEMON_MANSION_B1F */
    { "POKEMON_MANSION_B1F", 0xBE, 1 }, /* TOGGLE_POKEMON_MANSION_B1F_ITEM_1 */
    { "POKEMON_MANSION_B1F", 0xBF, 1 }, /* TOGGLE_POKEMON_MANSION_B1F_ITEM_2 */
    { "POKEMON_MANSION_B1F", 0xC0, 1 }, /* TOGGLE_POKEMON_MANSION_B1F_ITEM_3 */
    { "POKEMON_MANSION_B1F", 0xC1, 1 }, /* TOGGLE_POKEMON_MANSION_B1F_ITEM_4 */
    { "POKEMON_MANSION_B1F", 0xC2, 1 }, /* TOGGLE_POKEMON_MANSION_B1F_ITEM_5 */

    /* SAFARI_ZONE_EAST */
    { "SAFARI_ZONE_EAST", 0xC3, 1 }, /* TOGGLE_SAFARI_ZONE_EAST_ITEM_1 */
    { "SAFARI_ZONE_EAST", 0xC4, 1 }, /* TOGGLE_SAFARI_ZONE_EAST_ITEM_2 */
    { "SAFARI_ZONE_EAST", 0xC5, 1 }, /* TOGGLE_SAFARI_ZONE_EAST_ITEM_3 */
    { "SAFARI_ZONE_EAST", 0xC6, 1 }, /* TOGGLE_SAFARI_ZONE_EAST_ITEM_4 */

    /* SAFARI_ZONE_NORTH */
    { "SAFARI_ZONE_NORTH", 0xC7, 1 }, /* TOGGLE_SAFARI_ZONE_NORTH_ITEM_1 */
    { "SAFARI_ZONE_NORTH", 0xC8, 1 }, /* TOGGLE_SAFARI_ZONE_NORTH_ITEM_2 */

    /* SAFARI_ZONE_WEST */
    { "SAFARI_ZONE_WEST", 0xC9, 1 }, /* TOGGLE_SAFARI_ZONE_WEST_ITEM_1 */
    { "SAFARI_ZONE_WEST", 0xCA, 1 }, /* TOGGLE_SAFARI_ZONE_WEST_ITEM_2 */
    { "SAFARI_ZONE_WEST", 0xCB, 1 }, /* TOGGLE_SAFARI_ZONE_WEST_ITEM_3 */
    { "SAFARI_ZONE_WEST", 0xCC, 1 }, /* TOGGLE_SAFARI_ZONE_WEST_ITEM_4 */

    /* SAFARI_ZONE_CENTER */
    { "SAFARI_ZONE_CENTER", 0xCD, 1 }, /* TOGGLE_SAFARI_ZONE_CENTER_ITEM */

    /* CERULEAN_CAVE_2F */
    { "CERULEAN_CAVE_2F", 0xCE, 1 }, /* TOGGLE_CERULEAN_CAVE_2F_ITEM_1 */
    { "CERULEAN_CAVE_2F", 0xCF, 1 }, /* TOGGLE_CERULEAN_CAVE_2F_ITEM_2 */
    { "CERULEAN_CAVE_2F", 0xD0, 1 }, /* TOGGLE_CERULEAN_CAVE_2F_ITEM_3 */

    /* CERULEAN_CAVE_B1F */
    { "CERULEAN_CAVE_B1F", 0xD1, 1 }, /* TOGGLE_MEWTWO */
    { "CERULEAN_CAVE_B1F", 0xD2, 1 }, /* TOGGLE_CERULEAN_CAVE_B1F_ITEM_1 */
    { "CERULEAN_CAVE_B1F", 0xD3, 1 }, /* TOGGLE_CERULEAN_CAVE_B1F_ITEM_2 */

    /* VICTORY_ROAD_1F */
    { "VICTORY_ROAD_1F", 0xD4, 1 }, /* TOGGLE_VICTORY_ROAD_1F_ITEM_1 */
    { "VICTORY_ROAD_1F", 0xD5, 1 }, /* TOGGLE_VICTORY_ROAD_1F_ITEM_2 */

    /* CHAMPIONS_ROOM */
    { "CHAMPIONS_ROOM", 0xD6, 0 }, /* TOGGLE_CHAMPIONS_ROOM_OAK */

    /* SEAFOAM_ISLANDS_1F */
    { "SEAFOAM_ISLANDS_1F", 0xD7, 1 }, /* TOGGLE_SEAFOAM_ISLANDS_1F_BOULDER_1 */
    { "SEAFOAM_ISLANDS_1F", 0xD8, 1 }, /* TOGGLE_SEAFOAM_ISLANDS_1F_BOULDER_2 */

    /* SEAFOAM_ISLANDS_B1F */
    { "SEAFOAM_ISLANDS_B1F", 0xD9, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B1F_BOULDER_1 */
    { "SEAFOAM_ISLANDS_B1F", 0xDA, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B1F_BOULDER_2 */

    /* SEAFOAM_ISLANDS_B2F */
    { "SEAFOAM_ISLANDS_B2F", 0xDB, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B2F_BOULDER_1 */
    { "SEAFOAM_ISLANDS_B2F", 0xDC, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B2F_BOULDER_2 */

    /* SEAFOAM_ISLANDS_B3F */
    { "SEAFOAM_ISLANDS_B3F", 0xDD, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B3F_BOULDER_1 */
    { "SEAFOAM_ISLANDS_B3F", 0xDE, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B3F_BOULDER_2 */
    { "SEAFOAM_ISLANDS_B3F", 0xDF, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B3F_BOULDER_3 */
    { "SEAFOAM_ISLANDS_B3F", 0xE0, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B3F_BOULDER_4 */

    /* SEAFOAM_ISLANDS_B4F */
    { "SEAFOAM_ISLANDS_B4F", 0xE1, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B4F_BOULDER_1 */
    { "SEAFOAM_ISLANDS_B4F", 0xE2, 0 }, /* TOGGLE_SEAFOAM_ISLANDS_B4F_BOULDER_2 */
    { "SEAFOAM_ISLANDS_B4F", 0xE3, 1 }, /* TOGGLE_ARTICUNO */
};

#define TOGGLE_ENTRIES_COUNT 300

const int toggle_entries_count =
(int)(sizeof(toggle_entries) / sizeof(toggle_entries[0]));

/* The number of toggleable objects is the size of the master table.
   All flags are indexed by position in that table. */
static uint8_t toggle_flags[(TOGGLE_ENTRIES_COUNT + 7) / 8] = { 0 };

/* Note: toggle_entries_count must be a compile-time constant for the array
   size above to work. If it's declared `static const int`, the compiler
   will treat it as a constant expression in C (GCC/Clang/MSVC all allow
   this in practice). If your compiler complains, use a #define instead:

   #define TOGGLE_ENTRIES_COUNT 300

   and set toggle_entries_count = TOGGLE_ENTRIES_COUNT; in the .c file. */

int is_object_hidden(int global_id) {
    if (global_id < 0 || global_id >= toggle_entries_count) return 0;
    return (toggle_flags[global_id / 8] >> (global_id % 8)) & 1;
}

void hide_object(int global_id) {
    if (global_id < 0 || global_id >= toggle_entries_count) return;
    toggle_flags[global_id / 8] |= (uint8_t)(1u << (global_id % 8));
}

void show_object(int global_id) {
    if (global_id < 0 || global_id >= toggle_entries_count) return;
    toggle_flags[global_id / 8] &= (uint8_t)~(1u << (global_id % 8));
}

int sprite_is_collectable(int sprite_id) {
    switch (sprite_id) {
    case 61: /* SPRITE_POKE_BALL */
    case 62: /* SPRITE_FOSSIL */
    case 68: /* SPRITE_UNUSED_OLD_AMBER */
    case 69: /* SPRITE_OLD_AMBER */
        return 1;
    default:
        return 0;
    }
}

int dir_dx(Direction d) { return (d == DIR_LEFT) ? -1 : (d == DIR_RIGHT) ? 1 : 0; }
int dir_dy(Direction d) { return (d == DIR_UP) ? -1 : (d == DIR_DOWN) ? 1 : 0; }
