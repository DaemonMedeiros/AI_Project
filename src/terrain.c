/* ============================================================
 * terrain.c - Zone-based world terrain generation
 *
 * The world is baked once into a single RenderTexture: four
 * biome quadrants are painted first, then a Catmull-Rom dirt
 * path is drawn on top so it visually connects every zone.
 * ============================================================ */
#include "../include/terrain.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

/* ---------- Palette ---------- */
static const Color FOREST_GRASS_BASE   = {  56,  96,  46, 255 };
static const Color FOREST_GRASS_DARK   = {  44,  82,  38, 255 };
static const Color FOREST_GRASS_LIGHT  = {  70, 116,  54, 255 };
static const Color FOREST_CANOPY_A     = {  24,  90,  32, 235 };
static const Color FOREST_CANOPY_B     = {  30, 110,  40, 220 };
static const Color FOREST_CANOPY_C     = {  20,  75,  28, 245 };
static const Color FOREST_CANOPY_D     = {  40, 125,  48, 210 };

static const Color SNOW_BASE           = { 226, 232, 238, 255 };
static const Color SNOW_DRIFT_DARK     = { 208, 218, 228, 255 };
static const Color SNOW_DRIFT_LIGHT    = { 240, 245, 250, 255 };
static const Color SNOW_TREE_BODY      = {  46,  86,  62, 255 };
static const Color SNOW_TREE_CAP       = { 245, 250, 255, 240 };
static const Color SNOW_ROCK_BASE      = { 130, 135, 145, 255 };
static const Color SNOW_ROCK_HI        = { 170, 176, 186, 255 };
static const Color SNOW_BRUSH          = { 120,  96,  62, 220 };

static const Color DESERT_SAND_BASE    = { 226, 196, 140, 255 };
static const Color DESERT_SAND_DARK    = { 212, 178, 120, 255 };
static const Color DESERT_SAND_LIGHT   = { 238, 212, 160, 255 };
static const Color DESERT_CRACK        = { 120,  90,  56, 220 };
static const Color DESERT_ROCK         = { 138, 138, 140, 255 };
static const Color DESERT_CACTUS       = {  74, 118,  74, 255 };
static const Color DESERT_CACTUS_DARK  = {  46,  82,  46, 255 };
static const Color DESERT_CACTUS_HI    = { 116, 158, 108, 255 };
static const Color DESERT_PEBBLE       = { 140, 110,  70, 200 };

static const Color CLIFF_STONE_BASE    = {  96, 100, 108, 255 };
static const Color CLIFF_STONE_DARK    = {  78,  82,  90, 255 };
static const Color CLIFF_STONE_LIGHT   = { 118, 122, 130, 255 };
static const Color CLIFF_STONE_HI      = { 140, 144, 152, 255 };
static const Color CLIFF_ROCK          = { 120, 124, 132, 255 };
static const Color CLIFF_WATER_BANK    = {  40,  42,  48, 200 };
static const Color CLIFF_WATER_BED     = {  74, 118, 156, 255 };
static const Color CLIFF_WATER_SHINE   = { 148, 200, 232, 220 };
static const Color CLIFF_PLANK_TOP     = { 138,  96,  58, 255 };
static const Color CLIFF_PLANK_SIDE    = {  88,  60,  34, 255 };
static const Color CLIFF_POST          = {  66,  44,  26, 255 };
static const Color CLIFF_POST_HI       = { 102,  68,  40, 255 };
static const Color CLIFF_RAIL          = {  60,  40,  22, 220 };
static const Color CLIFF_GRASS         = {  90, 120,  66, 210 };

static const Color PATH_EDGE           = { 150, 120,  70, 255 };
static const Color PATH_FILL           = { 198, 170, 112, 255 };
static const Color PATH_SPECKLE        = { 160, 130,  80, 200 };

/* ---------- Module state ---------- */
static Vector2         pathSamples[MAX_PATH_SAMPLES];
static int             pathSampleCount;
static RenderTexture2D worldTexture;
static bool            initialized;

/* ============================================================
 *  Small helpers
 * ============================================================ */
static float RandFloat(float low, float high)
{
    return low + (float)GetRandomValue(0, 10000) / 10000.0f * (high - low);
}

static int RandInt(int low, int high)
{
    return GetRandomValue(low, high);
}

static Color ShadeColor(Color color, float factor)
{
    int red   = (int)(color.r * factor); if (red   < 0) red   = 0; if (red   > 255) red   = 255;
    int green = (int)(color.g * factor); if (green < 0) green = 0; if (green > 255) green = 255;
    int blue  = (int)(color.b * factor); if (blue  < 0) blue  = 0; if (blue  > 255) blue  = 255;
    return (Color){ (unsigned char)red, (unsigned char)green, (unsigned char)blue, color.a };
}

static Rectangle ZoneRect(ZoneType zone)
{
    float halfWidth  = (float)WORLD_WIDTH  * 0.5f;
    float halfHeight = (float)WORLD_HEIGHT * 0.5f;
    switch (zone)
    {
        case ZONE_FOREST:    return (Rectangle){ 0,      0,      halfWidth, halfHeight };
        case ZONE_SNOWY:     return (Rectangle){ halfWidth, 0,    halfWidth, halfHeight };
        case ZONE_DESERT:    return (Rectangle){ 0,      halfHeight, halfWidth, halfHeight };
        case ZONE_CLIFFSIDE: return (Rectangle){ halfWidth, halfHeight, halfWidth, halfHeight };
        default:             return (Rectangle){ 0, 0, 0, 0 };
    }
}

/* Scatter circular patches of a color inside a rectangle. */
static void ScatterPatches(Rectangle area, Color color, int count,
                           float minRadius, float maxRadius, float margin)
{
    for (int i = 0; i < count; i++)
    {
        Vector2 center = {
            RandFloat(area.x + margin, area.x + area.width  - margin),
            RandFloat(area.y + margin, area.y + area.height - margin)
        };
        DrawCircleV(center, RandFloat(minRadius, maxRadius), color);
    }
}

/* ============================================================
 *  Biome painters
 * ============================================================ */

static void PaintForest(Rectangle area)
{
    DrawRectangleRec(area, FOREST_GRASS_BASE);
    ScatterPatches(area, FOREST_GRASS_DARK,  120, 40.0f, 120.0f, 40.0f);
    ScatterPatches(area, FOREST_GRASS_LIGHT,  90, 30.0f,  90.0f, 40.0f);

    const Color canopyPalette[] = { FOREST_CANOPY_A, FOREST_CANOPY_B,
                                    FOREST_CANOPY_C, FOREST_CANOPY_D };

    for (int cluster = 0; cluster < 42; cluster++)
    {
        Vector2 center = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        int   treeCount     = RandInt(10, 22);
        float clusterRadius = RandFloat(70.0f, 170.0f);

        for (int tree = 0; tree < treeCount; tree++)
        {
            float angle    = RandFloat(0.0f, 360.0f) * DEG2RAD;
            float distance = RandFloat(0.0f, clusterRadius);
            Vector2 position = {
                center.x + cosf(angle) * distance,
                center.y + sinf(angle) * distance
            };

            DrawCircleV((Vector2){ position.x + 2, position.y + 3 },
                        RandFloat(10.0f, 18.0f), (Color){ 0, 0, 0, 60 });

            float radius = RandFloat(14.0f, 34.0f);
            Color canopy = canopyPalette[RandInt(0, 3)];
            DrawCircleV(position, radius, canopy);
            DrawCircleV((Vector2){ position.x - radius * 0.3f, position.y - radius * 0.3f },
                        radius * 0.5f, ShadeColor(canopy, 1.25f));
        }
    }

    for (int i = 0; i < 120; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        DrawCircleV((Vector2){ position.x + 1, position.y + 2 }, 12.0f,
                    (Color){ 0, 0, 0, 50 });
        DrawCircleV(position, RandFloat(10.0f, 22.0f), FOREST_CANOPY_C);
    }
}

static void PaintSnowy(Rectangle area)
{
    DrawRectangleRec(area, SNOW_BASE);
    ScatterPatches(area, SNOW_DRIFT_DARK,  140, 60.0f, 160.0f, 40.0f);
    ScatterPatches(area, SNOW_DRIFT_LIGHT, 110, 40.0f, 110.0f, 40.0f);

    for (int cluster = 0; cluster < 32; cluster++)
    {
        Vector2 center = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        int   treeCount     = RandInt(6, 14);
        float clusterRadius = RandFloat(60.0f, 150.0f);

        for (int tree = 0; tree < treeCount; tree++)
        {
            float angle    = RandFloat(0.0f, 360.0f) * DEG2RAD;
            float distance = RandFloat(0.0f, clusterRadius);
            Vector2 position = {
                center.x + cosf(angle) * distance,
                center.y + sinf(angle) * distance
            };
            float radius = RandFloat(12.0f, 26.0f);

            DrawCircleV((Vector2){ position.x + 2, position.y + 3 },
                        radius * 0.9f, (Color){ 60, 70, 90, 60 });
            DrawCircleV(position, radius, SNOW_TREE_BODY);
            DrawCircleV((Vector2){ position.x - radius * 0.25f, position.y - radius * 0.35f },
                        radius * 0.55f, SNOW_TREE_CAP);
        }
    }

    for (int i = 0; i < 90; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        float radius = RandFloat(8.0f, 22.0f);
        DrawCircleV((Vector2){ position.x, position.y + 2 }, radius,
                    (Color){ 0, 0, 0, 50 });
        DrawCircleV(position, radius, SNOW_ROCK_BASE);
        DrawCircleV((Vector2){ position.x - radius * 0.25f, position.y - radius * 0.3f },
                    radius * 0.55f, SNOW_ROCK_HI);
        DrawCircleV((Vector2){ position.x - radius * 0.15f, position.y - radius * 0.55f },
                    radius * 0.35f, SNOW_TREE_CAP);
    }

    for (int i = 0; i < 180; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        for (int frond = 0; frond < 4; frond++)
        {
            float angle = -PI / 2.0f + (frond - 1.5f) * 0.35f;
            DrawLineEx(position,
                       (Vector2){ position.x + cosf(angle) * 9.0f,
                                  position.y + sinf(angle) * 9.0f },
                       1.6f, SNOW_BRUSH);
        }
    }
}

static void PaintDesertCactus(Vector2 base, float height, float width)
{
    DrawEllipse((int)(base.x + 3), (int)(base.y + 4),
                width * 0.9f, width * 0.5f, (Color){ 0, 0, 0, 60 });

    DrawRectangle((int)(base.x - width * 0.5f), (int)(base.y - height),
                  (int)width, (int)height, DESERT_CACTUS);
    DrawRectangle((int)(base.x - width * 0.5f), (int)(base.y - height),
                  (int)(width * 0.35f), (int)height, DESERT_CACTUS_HI);
    DrawRectangle((int)(base.x + width * 0.15f), (int)(base.y - height),
                  (int)(width * 0.35f), (int)height, DESERT_CACTUS_DARK);

    float armY = base.y - height * 0.55f;
    DrawRectangle((int)(base.x - width * 1.6f), (int)armY,
                  (int)(width * 1.1f), (int)(width * 0.55f), DESERT_CACTUS);
    DrawRectangle((int)(base.x - width * 1.6f), (int)(armY - height * 0.35f),
                  (int)(width * 0.55f), (int)(height * 0.35f), DESERT_CACTUS);

    DrawRectangle((int)(base.x + width * 0.5f), (int)(armY + 6.0f),
                  (int)(width * 1.1f), (int)(width * 0.55f), DESERT_CACTUS);
    DrawRectangle((int)(base.x + width * 1.05f), (int)(armY + 6.0f - height * 0.30f),
                  (int)(width * 0.55f), (int)(height * 0.30f), DESERT_CACTUS);
}

static void PaintDesert(Rectangle area)
{
    DrawRectangleRec(area, DESERT_SAND_BASE);
    ScatterPatches(area, DESERT_SAND_DARK,  130, 60.0f, 180.0f, 40.0f);
    ScatterPatches(area, DESERT_SAND_LIGHT, 100, 50.0f, 130.0f, 40.0f);

    /* Ground cracks: branching random walks */
    for (int i = 0; i < 70; i++)
    {
        Vector2 position = {
            RandFloat(area.x + 40.0f, area.x + area.width  - 40.0f),
            RandFloat(area.y + 40.0f, area.y + area.height - 40.0f)
        };
        float angle  = RandFloat(0.0f, PI * 2.0f);
        float length = RandFloat(20.0f, 70.0f);
        int   segments = RandInt(3, 6);

        Vector2 previous = position;
        for (int s = 0; s < segments; s++)
        {
            angle += RandFloat(-0.5f, 0.5f);
            Vector2 next = {
                previous.x + cosf(angle) * (length / (float)segments),
                previous.y + sinf(angle) * (length / (float)segments)
            };
            DrawLineEx(previous, next, RandFloat(1.2f, 2.4f), DESERT_CRACK);
            previous = next;
        }
    }

    /* Grey rocks */
    for (int i = 0; i < 80; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        float radius = RandFloat(8.0f, 22.0f);
        DrawCircleV((Vector2){ position.x + 2, position.y + 3 }, radius,
                    (Color){ 0, 0, 0, 55 });
        DrawCircleV(position, radius, DESERT_ROCK);
        DrawCircleV((Vector2){ position.x - radius * 0.3f, position.y - radius * 0.3f },
                    radius * 0.55f, ShadeColor(DESERT_ROCK, 1.20f));
        DrawCircleV((Vector2){ position.x + radius * 0.4f, position.y + radius * 0.3f },
                    radius * 0.35f, ShadeColor(DESERT_ROCK, 0.75f));
    }

    /* Cacti */
    for (int i = 0; i < 26; i++)
    {
        Vector2 base = {
            RandFloat(area.x + 30.0f, area.x + area.width  - 30.0f),
            RandFloat(area.y + 30.0f, area.y + area.height - 30.0f)
        };
        float height = RandFloat(24.0f, 46.0f);
        PaintDesertCactus(base, height, height * 0.28f);
    }

    for (int i = 0; i < 220; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        DrawCircleV(position, RandFloat(1.0f, 2.5f), DESERT_PEBBLE);
    }
}

static void PaintCliffStream(Vector2 origin)
{
    float angle  = RandFloat(0.0f, PI * 2.0f);
    float length = RandFloat(160.0f, 320.0f);
    int   steps  = 24;

    Vector2 previous = origin;
    for (int i = 0; i < steps; i++)
    {
        angle += RandFloat(-0.35f, 0.35f);
        Vector2 next = {
            previous.x + cosf(angle) * (length / (float)steps),
            previous.y + sinf(angle) * (length / (float)steps)
        };
        DrawLineEx(previous, next, 18.0f, CLIFF_WATER_BANK);
        DrawLineEx(previous, next, 12.0f, CLIFF_WATER_BED);
        DrawLineEx(previous, next,  5.0f, CLIFF_WATER_SHINE);
        previous = next;
    }
}

static void PaintCliffBridge(Vector2 center)
{
    float angle  = RandFloat(-PI * 0.4f, PI * 0.4f);
    float length = RandFloat(48.0f, 74.0f);
    float width  = 18.0f;

    Vector2 direction     = { cosf(angle), sinf(angle) };
    Vector2 perpendicular = { -direction.y, direction.x };

    /* Support posts */
    for (int side = -1; side <= 1; side += 2)
    {
        Vector2 postBase = {
            center.x + perpendicular.x * width * 0.6f * side,
            center.y + perpendicular.y * width * 0.6f * side
        };
        DrawCircleV(postBase, 4.0f, CLIFF_POST);
        DrawCircleV((Vector2){ postBase.x - 1.0f, postBase.y - 1.0f }, 2.2f, CLIFF_POST_HI);
    }

    /* Planks */
    int plankCount = (int)(length / 7.0f);
    for (int p = 0; p < plankCount; p++)
    {
        float t = (float)p / (float)(plankCount - 1);
        float offset = (t - 0.5f) * length;
        Vector2 plankCenter = {
            center.x + direction.x * offset,
            center.y + direction.y * offset
        };

        Color plankTop = CLIFF_PLANK_TOP;
        if ((p % 3) == 0) plankTop = ShadeColor(plankTop, 0.9f);
        if ((p % 5) == 0) plankTop = ShadeColor(plankTop, 1.1f);

        Vector2 a = {
            plankCenter.x - perpendicular.x * width * 0.5f,
            plankCenter.y - perpendicular.y * width * 0.5f
        };
        Vector2 b = {
            plankCenter.x + perpendicular.x * width * 0.5f,
            plankCenter.y + perpendicular.y * width * 0.5f
        };
        DrawLineEx(a, b, 5.5f, CLIFF_PLANK_SIDE);
        DrawLineEx(a, b, 4.0f, plankTop);
    }

    /* Rope rails */
    Vector2 railA = { center.x - direction.x * length * 0.5f,
                      center.y - direction.y * length * 0.5f };
    Vector2 railB = { center.x + direction.x * length * 0.5f,
                      center.y + direction.y * length * 0.5f };
    for (int side = -1; side <= 1; side += 2)
    {
        Vector2 a = { railA.x + perpendicular.x * width * 0.6f * side,
                      railA.y + perpendicular.y * width * 0.6f * side };
        Vector2 b = { railB.x + perpendicular.x * width * 0.6f * side,
                      railB.y + perpendicular.y * width * 0.6f * side };
        DrawLineEx(a, b, 1.5f, CLIFF_RAIL);
    }
}

static void PaintCliffside(Rectangle area)
{
    DrawRectangleRec(area, CLIFF_STONE_BASE);
    ScatterPatches(area, CLIFF_STONE_LIGHT, 130, 40.0f, 140.0f, 40.0f);
    ScatterPatches(area, CLIFF_STONE_DARK,  110, 30.0f, 110.0f, 40.0f);
    ScatterPatches(area, CLIFF_STONE_HI,     80, 20.0f,  70.0f, 40.0f);

    /* Rock clusters */
    for (int cluster = 0; cluster < 40; cluster++)
    {
        Vector2 center = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        int rockCount = RandInt(3, 9);
        for (int r = 0; r < rockCount; r++)
        {
            Vector2 position = {
                center.x + RandFloat(-60.0f, 60.0f),
                center.y + RandFloat(-60.0f, 60.0f)
            };
            float radius = RandFloat(10.0f, 26.0f);

            DrawCircleV((Vector2){ position.x + 3, position.y + 4 }, radius,
                        (Color){ 0, 0, 0, 70 });
            DrawCircleV(position, radius, CLIFF_ROCK);
            DrawCircleV((Vector2){ position.x - radius * 0.30f, position.y - radius * 0.30f },
                        radius * 0.55f, ShadeColor(CLIFF_ROCK, 1.30f));
            DrawCircleV((Vector2){ position.x + radius * 0.35f, position.y + radius * 0.35f },
                        radius * 0.40f, ShadeColor(CLIFF_ROCK, 0.65f));
            DrawCircleLines((int)position.x, (int)position.y, radius,
                            (Color){ 50, 54, 60, 180 });
        }
    }

    /* Streams */
    for (int i = 0; i < 6; i++)
    {
        Vector2 origin = {
            RandFloat(area.x + 80.0f, area.x + area.width  - 80.0f),
            RandFloat(area.y + 80.0f, area.y + area.height - 80.0f)
        };
        PaintCliffStream(origin);
    }

    /* Bridges */
    for (int i = 0; i < 5; i++)
    {
        Vector2 center = {
            RandFloat(area.x + 100.0f, area.x + area.width  - 100.0f),
            RandFloat(area.y + 100.0f, area.y + area.height - 100.0f)
        };
        PaintCliffBridge(center);
    }

    /* Grass tufts */
    for (int i = 0; i < 200; i++)
    {
        Vector2 position = {
            RandFloat(area.x, area.x + area.width),
            RandFloat(area.y, area.y + area.height)
        };
        for (int blade = 0; blade < 3; blade++)
        {
            float angle = -PI / 2.0f + (blade - 1) * 0.4f;
            DrawLineEx(position,
                       (Vector2){ position.x + cosf(angle) * 7.0f,
                                  position.y + sinf(angle) * 7.0f },
                       1.4f, CLIFF_GRASS);
        }
    }
}

/* ============================================================
 *  Path construction
 * ============================================================ */
static void BuildPathControlPoints(Vector2 *control)
{
    /* Normalized (x, y) control points visiting every zone:
     * Forest NW -> Snowy NE -> Cliffside SE -> Desert SW -> back. */
    static const float points[PATH_CONTROL_POINTS][2] = {
        { 0.10f, 0.20f }, { 0.28f, 0.30f }, { 0.45f, 0.18f },
        { 0.62f, 0.28f }, { 0.82f, 0.22f }, { 0.92f, 0.42f },
        { 0.80f, 0.62f }, { 0.66f, 0.78f }, { 0.44f, 0.86f },
        { 0.28f, 0.78f }, { 0.14f, 0.60f }, { 0.22f, 0.44f },
        { 0.10f, 0.30f }, { 0.18f, 0.18f }, { 0.34f, 0.12f },
        { 0.50f, 0.06f }, { 0.68f, 0.10f }
    };

    for (int i = 0; i < PATH_CONTROL_POINTS; i++)
    {
        control[i].x = points[i][0] * (float)WORLD_WIDTH;
        control[i].y = points[i][1] * (float)WORLD_HEIGHT;
    }
}

static void SamplePath(Vector2 *control)
{
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
}

static void PaintPath(void)
{
    for (int i = 0; i < pathSampleCount; i++)
    {
        float wobble = sinf((float)i * 0.18f) * 7.0f;
        float radius = PATH_BASE_RADIUS + wobble;
        DrawCircleV(pathSamples[i], radius + 14, PATH_EDGE);
    }
    for (int i = 0; i < pathSampleCount; i++)
    {
        float wobble = sinf((float)i * 0.18f) * 7.0f;
        float radius = PATH_BASE_RADIUS + wobble;
        DrawCircleV(pathSamples[i], radius, PATH_FILL);
    }
    for (int i = 0; i < pathSampleCount; i += 3)
    {
        Vector2 center = pathSamples[i];
        for (int speckle = 0; speckle < 3; speckle++)
        {
            Vector2 offset = { RandFloat(-PATH_BASE_RADIUS, PATH_BASE_RADIUS),
                               RandFloat(-PATH_BASE_RADIUS, PATH_BASE_RADIUS) };
            if (Vector2Length(offset) > PATH_BASE_RADIUS - 4.0f) continue;
            DrawCircleV((Vector2){ center.x + offset.x, center.y + offset.y },
                        RandFloat(1.0f, 2.4f), PATH_SPECKLE);
        }
    }
}

/* ============================================================
 *  Lifecycle
 * ============================================================ */
void TerrainInit(void)
{
    if (initialized) TerrainUnload();

    Vector2 control[PATH_CONTROL_POINTS];
    BuildPathControlPoints(control);
    SamplePath(control);

    worldTexture = LoadRenderTexture(WORLD_WIDTH, WORLD_HEIGHT);

    BeginTextureMode(worldTexture);
        ClearBackground(FOREST_GRASS_BASE);

        PaintForest   (ZoneRect(ZONE_FOREST));
        PaintSnowy    (ZoneRect(ZONE_SNOWY));
        PaintDesert   (ZoneRect(ZONE_DESERT));
        PaintCliffside(ZoneRect(ZONE_CLIFFSIDE));

        PaintPath();
    EndTextureMode();

    initialized = true;
}

void TerrainUnload(void)
{
    if (!initialized) return;
    UnloadRenderTexture(worldTexture);
    initialized = false;
}

/* ============================================================
 *  Queries
 * ============================================================ */
int TerrainGetPathSampleCount(void)
{
    return pathSampleCount;
}

Vector2 TerrainGetPathSample(int index)
{
    if (pathSampleCount <= 0) return (Vector2){ 0, 0 };
    if (index < 0) index = 0;
    if (index >= pathSampleCount) index = pathSampleCount - 1;
    return pathSamples[index];
}

Vector2 TerrainGetPathMidpoint(void)
{
    return TerrainGetPathSample(pathSampleCount / 2);
}

bool TerrainIsOnPath(Vector2 position)
{
    for (int i = 0; i < pathSampleCount; i++)
        if (Vector2Distance(position, pathSamples[i]) < PATH_HALF_WIDTH)
            return true;
    return false;
}

ZoneType TerrainGetZoneAt(Vector2 position)
{
    bool east  = position.x >= (float)WORLD_WIDTH  * 0.5f;
    bool south = position.y >= (float)WORLD_HEIGHT * 0.5f;

    if (!east && !south) return ZONE_FOREST;
    if ( east && !south) return ZONE_SNOWY;
    if (!east &&  south) return ZONE_DESERT;
    return ZONE_CLIFFSIDE;
}

const char *TerrainGetZoneName(ZoneType zone)
{
    switch (zone)
    {
        case ZONE_FOREST:    return "Forest";
        case ZONE_SNOWY:     return "Snowy Woods";
        case ZONE_DESERT:    return "Desert";
        case ZONE_CLIFFSIDE: return "Cliffside";
        default:             return "Unknown";
    }
}

/* ============================================================
 *  Rendering
 * ============================================================ */
void TerrainDraw(void)
{
    Rectangle source = {
        0, 0,
        (float)worldTexture.texture.width,
        -(float)worldTexture.texture.height
    };
    DrawTextureRec(worldTexture.texture, source, (Vector2){ 0, 0 }, WHITE);
}