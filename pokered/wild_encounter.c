#include "wild_encounter.h"
#include "raylib.h"
#include "map.h"
#include "data_tables.h"
#include <string.h>

/* Cooldown after a battle (a la wNumberOfNoRandomBattleStepsLeft) */
static int s_cooldown_steps = 0;

/* WildMonEncounterSlotChances from data/wild/probabilities.asm. */
static const uint8_t encounter_slot_chances[MAX_WILD_SLOTS] = {
    51, 102, 140, 165, 191, 216, 229, 242, 252, 255
};

/* ---- Route 1 (data/maps/objects/Route1.asm) ---- */
static const WildEncounterData route1_data = {
    .map_name = "ROUTE_1",
    .grass_rate = 25,
    .grass_mons = {
        { 3, SPECIES_PIDGEY  },
        { 3, SPECIES_RATTATA },
        { 3, SPECIES_RATTATA },
        { 2, SPECIES_RATTATA },
        { 2, SPECIES_PIDGEY  },
        { 3, SPECIES_PIDGEY  },
        { 3, SPECIES_PIDGEY  },
        { 4, SPECIES_RATTATA },
        { 4, SPECIES_PIDGEY  },
        { 5, SPECIES_PIDGEY  },
    },
    .water_rate = 0,
    .water_mons = {{0, 0}},
};

/* Add more maps here as needed. */
static const WildEncounterData* wild_data_table[] = {
    &route1_data,
    NULL
};

void wild_encounter_init(void) {
    s_cooldown_steps = 0;
}

void grant_battle_cooldown(int steps) {
    if (steps > 0) s_cooldown_steps = steps;
}

/* ---- FIX: strip underscores so "ROUTE_1" and "ROUTE1" compare equal ---- */
static void normalize_name(const char* in, char* out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < out_size; i++) {
        if (in[i] != '_') out[j++] = in[i];
    }
    out[j] = 0;
}

static const WildEncounterData* get_data_for_map(const char* map_name) {
    char want[64];
    normalize_name(map_name, want, sizeof(want));
    for (int i = 0; wild_data_table[i]; i++) {
        char got[64];
        normalize_name(wild_data_table[i]->map_name, got, sizeof(got));
        if (strcmp(got, want) == 0)
            return wild_data_table[i];
    }
    return NULL;
}
/* ---------------------------------------------------------------------- */

int try_wild_encounter(ActiveMap* am, Player* player,
    uint8_t* out_species, uint8_t* out_level) {
    /* Consume cooldown first. */
    if (s_cooldown_steps > 0) {
        s_cooldown_steps--;
        return 0;
    }

    const WildEncounterData* data = get_data_for_map(am->name);
    if (!data) return 0;
    if (data->grass_rate == 0) return 0;

    /* Determine whether the tile the player is standing on matches the
     * grass tile for the current tileset. */
    uint8_t grass_tile = grass_tile_for(am->tileset_stem);
    if (grass_tile == 0xFF) return 0;

    uint8_t standing = tile_in_front_of_cell(am, player->tile_x, player->tile_y);
    if (standing != grass_tile) return 0;

    /* hRandomAdd < grass_rate ?  (25/256 ≈ 10% on Route 1) */
    int r = GetRandomValue(0, 255);
    if (r >= data->grass_rate) return 0;

    /* Pick an encounter slot based on hRandomSub. */
    int r2 = GetRandomValue(0, 255);
    int slot = 0;
    for (int i = 0; i < MAX_WILD_SLOTS; i++) {
        if (encounter_slot_chances[i] >= r2) { slot = i; break; }
    }

    *out_species = data->grass_mons[slot].species;
    *out_level = data->grass_mons[slot].level;
    return 1;
}