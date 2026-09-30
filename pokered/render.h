#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "types.h"
#include "palettes.h"

Color decode_2bpp_bg_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
    const SGBPalette* sgb);
Color decode_2bpp_sprite_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
    const SGBPalette* sgb);

void decode_tileset(const char* path, Tileset* ts, uint8_t reg,
    int is_sprite, int pal_id);
Texture2D decode_1bpp(const char* path);

Rectangle sprite_quadrant_src(const Tileset* ts, uint8_t tid, int flip);
void draw_sprite(Tileset* ts, const uint8_t* tiles, int flip, int px, int py);
void draw_shadow(Texture2D shadow_tex, int px, int py);

#endif
