#ifndef WILD_ENCOUNTER_H
#define WILD_ENCOUNTER_H

#include <stdint.h>
#include "types.h"

/* Species IDs (Pokedex numbers, subset) */
#define SPECIES_NONE     0
#define SPECIES_BULBASAUR 1
#define SPECIES_CHARMANDER 4
#define SPECIES_SQUIRTLE 7
#define SPECIES_PIDGEY  16
#define SPECIES_RATTATA 19

#define MAX_WILD_SLOTS 10

typedef struct {
    uint8_t level;
    uint8_t species;
} WildMonSlot;

typedef struct {
    const char* map_name;
    uint8_t grass_rate;
    WildMonSlot grass_mons[MAX_WILD_SLOTS];
    uint8_t water_rate;
    WildMonSlot water_mons[MAX_WILD_SLOTS];
} WildEncounterData;

/* Init subsystem (call once in main) */
void wild_encounter_init(void);

/* Called when the player finishes a step.
 * Returns 1 if a wild encounter should start, 0 otherwise.
 * On 1, *out_species and *out_level are filled in. */
int try_wild_encounter(ActiveMap* am, Player* player,
    uint8_t* out_species, uint8_t* out_level);

/* Grant N safe steps after a battle */
void grant_battle_cooldown(int steps);

#endif