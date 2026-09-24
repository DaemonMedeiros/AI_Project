/* ============================================================
 * terrain.c - World terrain generation and rendering
 * ============================================================ */
#include "../include/terrain.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

static Vector2         pathSamples[MAX_PATH_SAMPLES];
static int             pathSampleCount;
static RenderTexture2D worldTexture;
static bool            initialized;

static float LocalClampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float TerrainClampf(float value, float low, float high)
{
    return LocalClampf(value, low, high);
}

/* ---------------------------------------------------------- */
void TerrainInit(void)
{
    if (initialized) TerrainUnload();

    Vector2 control[PATH_CONTROL_POINTS];
    float stepX = (float)WORLD_WIDTH / (float)(PATH_CONTROL_POINTS - 1);
    float y = WORLD_HEIGHT * 0.5f;

    for (int i = 0; i < PATH_CONTROL_POINTS; i++)
    {
        control[i].x = i * stepX;
        if (i > 0)
        {
            y += (float)GetRandomValue(-260, 260);
            float margin = WORLD_HEIGHT * 0.15f;
            y = LocalClampf(y, margin, WORLD_HEIGHT - margin);
        }
        control[i].y = y;
    }

    pathSampleCount = 0;
    for (int i = 0; i < PATH_CONTROL_POINTS - 1; i++)
    {
        Vector2 p0 = control[(i - 1 < 0) ? 0 : i - 1];
        Vector2 p1 = control[i];
        Vector2 p2 = control[i + 1];
        Vector2 p3 = control[(i + 2 >= PATH_CONTROL_POINTS) ? PATH_CONTROL_POINTS - 1 : i + 2];

        for (int s = 0; s < PATH_SAMPLES_PER_SEGMENT; s++)
        {
            float t = (float)s / (float)PATH_SAMPLES_PER_SEGMENT;
            if (pathSampleCount < MAX_PATH_SAMPLES)
                pathSamples[pathSampleCount++] = GetSplinePointCatmullRom(p0, p1, p2, p3, t);
        }
    }
    if (pathSampleCount < MAX_PATH_SAMPLES)
        pathSamples[pathSampleCount++] = control[PATH_CONTROL_POINTS - 1];

    worldTexture = LoadRenderTexture(WORLD_WIDTH, WORLD_HEIGHT);

    BeginTextureMode(worldTexture);
        ClearBackground((Color){ 40, 70, 35, 255 });

        for (int c = 0; c < NUM_TREE_CLUSTERS; c++)
        {
            Vector2 center = { (float)GetRandomValue(0, WORLD_WIDTH),
                               (float)GetRandomValue(0, WORLD_HEIGHT) };
            int   treeCount     = GetRandomValue(10, 22);
            float clusterRadius = (float)GetRandomValue(70, 170);

            for (int t = 0; t < treeCount; t++)
            {
                float angle    = (float)GetRandomValue(0, 360) * DEG2RAD;
                float distance = (float)GetRandomValue(0, (int)clusterRadius);
                Vector2 pos = { center.x + cosf(angle) * distance, center.y + sinf(angle) * distance };
                float radius = (float)GetRandomValue(14, 34);

                Color color;
                switch (GetRandomValue(0, 3))
                {
                    case 0:  color = (Color){ 24, 90, 32, 235 };  break;
                    case 1:  color = (Color){ 30, 110, 40, 220 }; break;
                    case 2:  color = (Color){ 20, 75, 28, 245 };  break;
                    default: color = (Color){ 40, 125, 48, 210 }; break;
                }
                DrawCircleV(pos, radius, color);
            }
        }

        for (int i = 0; i < NUM_SPARSE_TREES; i++)
        {
            Vector2 pos = { (float)GetRandomValue(0, WORLD_WIDTH),
                            (float)GetRandomValue(0, WORLD_HEIGHT) };
            float radius = (float)GetRandomValue(10, 22);
            DrawCircleV(pos, radius, (Color){ 28, 100, 36, 200 });
        }

        for (int i = 0; i < pathSampleCount; i++)
        {
            float wobble = sinf((float)i * 0.18f) * 7.0f;
            float radius = PATH_BASE_RADIUS + wobble;
            DrawCircleV(pathSamples[i], radius + 14, (Color){ 150, 120, 70, 255 });
        }
        for (int i = 0; i < pathSampleCount; i++)
        {
            float wobble = sinf((float)i * 0.18f) * 7.0f;
            float radius = PATH_BASE_RADIUS + wobble;
            DrawCircleV(pathSamples[i], radius, (Color){ 198, 170, 112, 255 });
        }
    EndTextureMode();

    initialized = true;
}

void TerrainUnload(void)
{
    if (initialized)
    {
        UnloadRenderTexture(worldTexture);
        initialized = false;
    }
}

int TerrainGetPathSampleCount(void) { return pathSampleCount; }

Vector2 TerrainGetPathSample(int index)
{
    if (pathSampleCount <= 0) return (Vector2){ 0, 0 };
    if (index < 0) index = 0;
    if (index >= pathSampleCount) index = pathSampleCount - 1;
    return pathSamples[index];
}

Vector2 TerrainGetPathMidpoint(void) { return TerrainGetPathSample(pathSampleCount / 2); }

bool TerrainIsOnPath(Vector2 pos)
{
    for (int i = 0; i < pathSampleCount; i++)
        if (Vector2Distance(pos, pathSamples[i]) < PATH_HALF_WIDTH) return true;
    return false;
}

void TerrainDraw(void)
{
    Rectangle source = { 0, 0, (float)worldTexture.texture.width, -(float)worldTexture.texture.height };
    DrawTextureRec(worldTexture.texture, source, (Vector2){ 0, 0 }, WHITE);
}