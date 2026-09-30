#ifndef FADE_H
#define FADE_H

#include "types.h"

void begin_fade_out(PaletteFade* fade, int to_black, int warp_index);
void fade_tick(PaletteFade* fade, ActiveMap* am, Player* player,
    Tileset* player_ts, Tileset* npc_ts,
    const char* player_sprite_path, const MapPaths* paths);

#endif
