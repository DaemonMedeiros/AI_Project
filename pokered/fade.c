#include "fade.h"
#include "gb.h"
#include "map.h"
#include "player.h"
#include "render.h"
#include "palettes.h"

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

static const int fade_out_black_seq[4] = { 3, 2, 1, 0 };
static const int fade_in_black_seq[4] = { 0, 1, 2, 3 };
static const int fade_out_white_seq[3] = { 5, 6, 7 };
static const int fade_in_white_seq[3] = { 6, 5, 3 };

void begin_fade_out(PaletteFade* fade, int to_black, int warp_index) {
    fade->phase = FADE_OUT;
    fade->pending = FADE_PENDING_WARP;
    fade->step = 0;
    fade->ticks_in_step = 0;
    fade->to_black = to_black;
    fade->warp_index = warp_index;
    fade->current_bgp = REG_BGP;
}

void fade_tick(PaletteFade* fade, ActiveMap* am, Player* player,
    Tileset* player_ts, Tileset* npc_ts,
    const char* player_sprite_path, const MapPaths* paths) {
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
                WarpEvent* warp = &am->warps[fade->warp_index];
                do_warp(am, player, warp, paths);
                fade->pending = FADE_PENDING_NONE;
                fade->hide_world = 1;
            }
            fade->phase = FADE_IN;
            fade->step = 0;
            fade->ticks_in_step = 0;
        }
    }
    else {
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
