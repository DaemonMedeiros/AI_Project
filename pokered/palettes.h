#ifndef PALETTES_H
#define PALETTES_H

#include "raylib.h"
#include <stdint.h>

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

extern const SGBPalette sgb_super_palettes[NUM_SGB_PALS];
extern const uint8_t dmg_shade[4];

Color sgb_color_to_rgba(uint16_t c);
int reg_shade(uint8_t reg, int index);
int lookup_sgb_palette_ids(const char* map_name, int out[4]);

#endif
