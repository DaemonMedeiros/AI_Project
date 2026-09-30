#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

int read_input_direction(void);
void advance_walk_anim(int* intra_frame, int* anim_frame);
void begin_step(Player* p, Direction d, int nx, int ny, int frames);

void do_map_transition(ActiveMap* am, Player* p, const MapPaths* paths);
void do_warp(ActiveMap* am, Player* p, WarpEvent* warp, const MapPaths* paths);

#endif
