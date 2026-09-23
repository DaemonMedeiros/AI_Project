/* ============================================================
 * terrain.h - World terrain generation and rendering
 * ============================================================ */
#ifndef TERRAIN_H
#define TERRAIN_H

#include "raylib.h"
#include <stdbool.h>

/* World dimensions (in pixels) */
#define WORLD_SCALE    4
#define WORLD_WIDTH    (800 * WORLD_SCALE)   /* 3200 */
#define WORLD_HEIGHT   (600 * WORLD_SCALE)   /* 2400 */

/* Terrain generation tuning */
#define PATH_CONTROL_POINTS       9
#define PATH_SAMPLES_PER_SEGMENT  40
#define MAX_PATH_SAMPLES          ((PATH_CONTROL_POINTS) * (PATH_SAMPLES_PER_SEGMENT) + 8)
#define PATH_BASE_RADIUS          46.0f
#define PATH_HALF_WIDTH           48.0f
#define NUM_TREE_CLUSTERS         55
#define NUM_SPARSE_TREES          260

/* Lifecycle */
void  TerrainInit(void);       /* generates path + bakes world texture */
void  TerrainUnload(void);     /* frees the baked render texture */

/* Accessors */
int        TerrainGetPathSampleCount(void);
Vector2    TerrainGetPathSample(int index);          /* index clamped internally */
Vector2    TerrainGetPathMidpoint(void);             /* safe spawn point */
bool       TerrainIsOnPath(Vector2 pos);

/* Drawing: draws the baked terrain inside the currently active 2D camera */
void       TerrainDraw(void);

/* Internal helpers exported for spawn logic */
float      TerrainClampf(float v, float lo, float hi);

#endif /* TERRAIN_H */