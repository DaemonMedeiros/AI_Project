/* ============================================================
 * terrain.h - World terrain generation and rendering
 *
 * The world is divided into four zones (one per quarter of the
 * map), each with its own biome palette and decoration:
 *   - Forest   (west)
 *   - Desert   (center-south)
 *   - Cliffside (center-north)
 *   - Snowy    (east)
 * A single winding path threads through all four.
 * ============================================================ */
#ifndef TERRAIN_H
#define TERRAIN_H

#include "raylib.h"
#include <stdbool.h>

/* World dimensions (in pixels) */
#define WORLD_SCALE    4
#define WORLD_WIDTH    (800 * WORLD_SCALE)   /* 3200 */
#define WORLD_HEIGHT   (600 * WORLD_SCALE)   /* 2400 */

/* Path generation tuning */
#define PATH_CONTROL_POINTS       17
#define PATH_SAMPLES_PER_SEGMENT  40
#define MAX_PATH_SAMPLES          ((PATH_CONTROL_POINTS) * (PATH_SAMPLES_PER_SEGMENT) + 8)
#define PATH_BASE_RADIUS          46.0f
#define PATH_HALF_WIDTH           48.0f

/* Zone definitions: quartered world layout
 *   NW quadrant: Forest
 *   NE quadrant: Snowy
 *   SW quadrant: Desert
 *   SE quadrant: Cliffside
 * Zone is determined by player position. */
typedef enum {
    ZONE_FOREST = 0,
    ZONE_SNOWY,
    ZONE_DESERT,
    ZONE_CLIFFSIDE,
    ZONE_COUNT
} ZoneType;

void  TerrainInit(void);
void  TerrainUnload(void);

int     TerrainGetPathSampleCount(void);
Vector2 TerrainGetPathSample(int index);
Vector2 TerrainGetPathMidpoint(void);
bool    TerrainIsOnPath(Vector2 pos);
void    TerrainDraw(void);
float   TerrainClampf(float value, float low, float high);

/* Zone helpers (also useful for encounter tables / HUD) */
ZoneType TerrainGetZoneAt(Vector2 pos);
const char *TerrainGetZoneName(ZoneType zone);

#endif /* TERRAIN_H */