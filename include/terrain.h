/* ============================================================
 * terrain.h - World terrain generation and rendering
 * ============================================================ */
#ifndef TERRAIN_H
#define TERRAIN_H

#include "raylib.h"
#include <stdbool.h>

#define WORLD_SCALE    4
#define WORLD_WIDTH    (800 * WORLD_SCALE)
#define WORLD_HEIGHT   (600 * WORLD_SCALE)

#define PATH_CONTROL_POINTS       9
#define PATH_SAMPLES_PER_SEGMENT  40
#define MAX_PATH_SAMPLES          ((PATH_CONTROL_POINTS) * (PATH_SAMPLES_PER_SEGMENT) + 8)
#define PATH_BASE_RADIUS          46.0f
#define PATH_HALF_WIDTH           48.0f
#define NUM_TREE_CLUSTERS         55
#define NUM_SPARSE_TREES          260

void  TerrainInit(void);
void  TerrainUnload(void);

int     TerrainGetPathSampleCount(void);
Vector2 TerrainGetPathSample(int index);
Vector2 TerrainGetPathMidpoint(void);
bool    TerrainIsOnPath(Vector2 pos);
void    TerrainDraw(void);
float   TerrainClampf(float value, float low, float high);

#endif /* TERRAIN_H */