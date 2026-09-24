/* ============================================================
 * backgrounds.h - Procedurally generated battle backgrounds
 *
 * Six distinct battle backdrops are baked into RenderTextures
 * once at startup and blitted during combat.
 * ============================================================ */
#ifndef BACKGROUNDS_H
#define BACKGROUNDS_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    BG_FOREST_DAY = 0,
    BG_FOREST_NIGHT,
    BG_PATH_DAY,
    BG_PATH_NIGHT,
    BG_DESERT_DAY,
    BG_DESERT_NIGHT,
    BG_COUNT
} BackgroundType;

void BackgroundsInit(void);
void BackgroundsUnload(void);
BackgroundType BackgroundsPickRandom(void);
void BackgroundsDraw(BackgroundType type);
const char *BackgroundsGetName(BackgroundType type);

#endif /* BACKGROUNDS_H */