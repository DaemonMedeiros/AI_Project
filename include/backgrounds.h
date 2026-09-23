/* ============================================================
 * backgrounds.h - Procedurally generated battle backgrounds
 *
 * Provides 6 distinct battle backdrops, generated once at
 * startup and baked into RenderTextures:
 *   - Deep forest (day / night)
 *   - Forest path with mountains (day / night)
 *   - Rocky desert (day / night)
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

/* Lifecycle: bake all backgrounds into GPU textures */
void BackgroundsInit(void);
void BackgroundsUnload(void);

/* Pick a random background and return its type */
BackgroundType BackgroundsPickRandom(void);

/* Draw the given background filling the screen */
void BackgroundsDraw(BackgroundType type);

/* Convenience: returns a name for debug / UI */
const char *BackgroundsGetName(BackgroundType type);

#endif /* BACKGROUNDS_H */