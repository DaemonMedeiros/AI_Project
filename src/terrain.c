/* ============================================================
 * terrain.c - Zone-based world terrain generation
 *
 * The world background is generated once into an offscreen
 * RenderTexture:
 *
 *   1. A low-resolution "zone weight" grid is built. At every
 *      grid cell we compute how much each of the four biomes
 *      (forest / snowy / desert / cliffside) influences that
 *      point, using a noise-warped bilinear blend across the
 *      four quadrants instead of a hard rectangle test. This
 *      is what makes the biome borders wavy and gradual rather
 *      than a straight seam.
 *
 *   2. A base color texture is painted at reduced resolution by
 *      blending each biome's ground color according to those
 *      weights (plus some per-pixel noise for grain/patchiness),
 *      then upscaled with bilinear filtering onto the world
 *      render texture. The blur from the upscale reinforces the
 *      soft transition.
 *
 *   3. Decorations (trees, rocks, cacti, snowdrifts, etc.) are
 *      scattered per biome as before, but now they are allowed
 *      to wander slightly past their home quadrant and are kept
 *      or discarded based on the local zone weight - so a few
 *      forest trees trail off into the snowy border instead of
 *      the tree line stopping on a ruler-straight edge.
 *
 *   4. A Catmull-Rom dirt path is drawn on top, looping through
 *      all four zones.
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

/* ---------- Zone blending tuning ----------
 * These knobs control how the four biome quadrants melt into
 * each other. Raise BLEND_SOFTNESS for a wider transition band;
 * raise WARP_AMOUNT for a wavier, more organic border instead of
 * a straight seam. AMBIENT_BLEED_MARGIN controls how far
 * decorations may scatter past their home quadrant before being
 * thinned out by ShouldPlaceInZone(). */
#define WARP_SCALE           0.0015f
#define WARP_AMOUNT          0.07f
#define BLEND_SOFTNESS       0.10f
#define PATCH_NOISE_SCALE    0.006f
#define GRAIN_NOISE_SCALE    0.05f
#define ZONE_GRID_STEP       8      /* world pixels per zone-weight grid cell */
#define BASE_TEX_DOWNSCALE   2      /* base texture is generated at 1/N resolution, then upscaled */
#define AMBIENT_BLEED_MARGIN 0.18f

/* Per-point influence of each biome, always summing to 1.0. */
typedef struct {
    float forest;
    float snowy;
    float desert;
    float cliff;
} ZoneWeights;

/* ---------- Module state ---------- */
static Vector2         pathSamples[MAX_PATH_SAMPLES];
static int             pathSampleCount;
static RenderTexture2D worldTexture;
static bool            initialized;

static ZoneWeights *zoneGrid;
static int          zoneGridWidth;
static int          zoneGridHeight;

/* ============================================================
 *  Small helpers
 * ============================================================ */
float TerrainClampf(float value, float low, float high)
{
    if (value < low)  return low;
    if (value > high) return high;
    return value;
}

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
    float red   = (float)color.r * factor;
    float green = (float)color.g * factor;
    float blue  = (float)color.b * factor;
    return (Color){
        (unsigned char)TerrainClampf(red,   0.0f, 255.0f),
        (unsigned char)TerrainClampf(green, 0.0f, 255.0f),
        (unsigned char)TerrainClampf(blue,  0.0f, 255.0f),
        color.a
    };
}

static Color LerpColor(Color a, Color b, float t)
{
    t = TerrainClampf(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)(a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)(a.b + ((float)b.b - (float)a.b) * t),
        (unsigned char)(a.a + ((float)b.a - (float)a.a) * t)
    };
}

/* ============================================================
 *  Value noise (hash-based, deterministic, no external deps)
 * ============================================================ */
static float Hash2D(int x, int y, int seed)
{
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + seed * 2147483647u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFFFu) / (float)0xFFFFFFu;
}

static float SmoothNoise(float x, float y, int seed)
{
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;

    float v00 = Hash2D(ix,     iy,     seed);
    float v10 = Hash2D(ix + 1, iy,     seed);
    float v01 = Hash2D(ix,     iy + 1, seed);
    float v11 = Hash2D(ix + 1, iy + 1, seed);

    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);

    float a = v00 + (v10 - v00) * sx;
    float b = v01 + (v11 - v01) * sx;
    return a + (b - a) * sy;
}

/* Fractal sum of a few octaves of SmoothNoise, result in [0, 1]. */
static float FBM(float x, float y, int seed, int octaves)
{
    float total     = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;

    for (int i = 0; i < octaves; i++)
    {
        total    += SmoothNoise(x * frequency, y * frequency, seed + i * 17) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return total / maxValue;
}

static float SmoothStepF(float edge0, float edge1, float x)
{
    float t = TerrainClampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* ============================================================
 *  Zone weight model
 *
 *  The world is conceptually a 2x2 grid (west/east, north/south).
 *  Instead of testing which hard quadrant a point falls in, we
 *  warp the point's normalized position with low-frequency noise
 *  and run it through a smoothstep, then combine the two axis
 *  weights bilinearly. This gives four weights that sum to 1.0
 *  everywhere, blend smoothly across every border (including the
 *  point where all four zones meet), and follow a wavy, organic
 *  seam instead of a straight line.
 * ============================================================ */
static ZoneWeights ComputeZoneWeights(float worldX, float worldY)
{
    float u = worldX / (float)WORLD_WIDTH;
    float v = worldY / (float)WORLD_HEIGHT;

    float warpX = (FBM(worldX * WARP_SCALE,          worldY * WARP_SCALE,          11, 4) - 0.5f) * WARP_AMOUNT;
    float warpY = (FBM(worldX * WARP_SCALE + 91.7f,   worldY * WARP_SCALE + 91.7f,  23, 4) - 0.5f) * WARP_AMOUNT;

    float wu = u + warpX;
    float wv = v + warpY;

    float wx = SmoothStepF(0.5f - BLEND_SOFTNESS, 0.5f + BLEND_SOFTNESS, wu); /* 0 = west,  1 = east  */
    float wy = SmoothStepF(0.5f - BLEND_SOFTNESS, 0.5f + BLEND_SOFTNESS, wv); /* 0 = north, 1 = south */

    ZoneWeights weights;
    weights.forest = (1.0f - wx) * (1.0f - wy); /* NW */
    weights.snowy  =         wx  * (1.0f - wy); /* NE */
    weights.desert = (1.0f - wx) *         wy;  /* SW */
    weights.cliff  =         wx  *         wy;  /* SE */
    return weights;
}

static float ZoneWeightValue(ZoneWeights w, ZoneType zone)
{
    switch (zone)
    {
        case ZONE_FOREST:    return w.forest;
        case ZONE_SNOWY:     return w.snowy;
        case ZONE_DESERT:    return w.desert;
        case ZONE_CLIFFSIDE: return w.cliff;
        default:              return 0.0f;
    }
}

/* The "home" rectangle a biome is anchored to (still a clean
 * quadrant - only the painted result and decoration placement
 * are soft, not the bookkeeping). */
static Rectangle ZoneRect(ZoneType zone)
{
    float halfWidth  = (float)WORLD_WIDTH  * 0.5f;
    float halfHeight = (float)WORLD_HEIGHT * 0.5f;
    switch (zone)
    {
        case ZONE_FOREST:    return (Rectangle){ 0,         0,          halfWidth, halfHeight };
        case ZONE_SNOWY:     return (Rectangle){ halfWidth, 0,          halfWidth, halfHeight };
        case ZONE_DESERT:    return (Rectangle){ 0,         halfHeight, halfWidth, halfHeight };
        case ZONE_CLIFFSIDE: return (Rectangle){ halfWidth, halfHeight, halfWidth, halfHeight };
        default:             return (Rectangle){ 0, 0, 0, 0 };
    }
}

/* Home rectangle padded outward (clamped to world bounds) so
 * decorations have room to bleed into neighbouring zones. */
static Rectangle ZoneRectExpanded(ZoneType zone, float marginFrac)
{
    Rectangle r = ZoneRect(zone);
    float mx = r.width  * marginFrac;
    float my = r.height * marginFrac;

    float x0 = r.x - mx;
    float y0 = r.y - my;
    float x1 = r.x + r.width  + mx;
    float y1 = r.y + r.height + my;

    if (x0 < 0.0f) x0 = 0.0f;
    if (y0 < 0.0f) y0 = 0.0f;
    if (x1 > (float)WORLD_WIDTH)  x1 = (float)WORLD_WIDTH;
    if (y1 > (float)WORLD_HEIGHT) y1 = (float)WORLD_HEIGHT;

    return (Rectangle){ x0, y0, x1 - x0, y1 - y0 };
}

static float AreaScale(Rectangle home, Rectangle expanded)
{
    return (expanded.width * expanded.height) / (home.width * home.height);
}

/* ============================================================
 *  Zone weight grid
 *
 *  Evaluating ComputeZoneWeights (several noise octaves) at
 *  every one of the world's ~7.7 million pixels would be slow,
 *  and the field is low-frequency anyway, so it's instead
 *  evaluated on a coarse grid once and bilinearly sampled
 *  everywhere else (base texture generation and decoration
 *  placement alike). The grid only needs to live for the
 *  duration of TerrainInit.
 * ============================================================ */
static void BuildZoneGrid(void)
{
    zoneGridWidth  = WORLD_WIDTH  / ZONE_GRID_STEP + 2;
    zoneGridHeight = WORLD_HEIGHT / ZONE_GRID_STEP + 2;

    zoneGrid = (ZoneWeights *)malloc(sizeof(ZoneWeights) * (size_t)zoneGridWidth * (size_t)zoneGridHeight);

    for (int gy = 0; gy < zoneGridHeight; gy++)
    {
        for (int gx = 0; gx < zoneGridWidth; gx++)
        {
            float worldX = (float)gx * (float)ZONE_GRID_STEP;
            float worldY = (float)gy * (float)ZONE_GRID_STEP;
            zoneGrid[gy * zoneGridWidth + gx] = ComputeZoneWeights(worldX, worldY);
        }
    }
}

static void FreeZoneGrid(void)
{
    free(zoneGrid);
    zoneGrid       = NULL;
    zoneGridWidth  = 0;
    zoneGridHeight = 0;
}

static ZoneWeights SampleZoneGrid(float worldX, float worldY)
{
    float gx = worldX / (float)ZONE_GRID_STEP;
    float gy = worldY / (float)ZONE_GRID_STEP;

    int gx0 = (int)floorf(gx);
    int gy0 = (int)floorf(gy);
    if (gx0 < 0) gx0 = 0;
    if (gy0 < 0) gy0 = 0;
    if (gx0 > zoneGridWidth  - 2) gx0 = zoneGridWidth  - 2;
    if (gy0 > zoneGridHeight - 2) gy0 = zoneGridHeight - 2;

    float fx = TerrainClampf(gx - (float)gx0, 0.0f, 1.0f);
    float fy = TerrainClampf(gy - (float)gy0, 0.0f, 1.0f);

    ZoneWeights w00 = zoneGrid[gy0       * zoneGridWidth + gx0];
    ZoneWeights w10 = zoneGrid[gy0       * zoneGridWidth + gx0 + 1];
    ZoneWeights w01 = zoneGrid[(gy0 + 1) * zoneGridWidth + gx0];
    ZoneWeights w11 = zoneGrid[(gy0 + 1) * zoneGridWidth + gx0 + 1];

    ZoneWeights top, bottom, result;
    top.forest = w00.forest + (w10.forest - w00.forest) * fx;
    top.snowy  = w00.snowy  + (w10.snowy  - w00.snowy)  * fx;
    top.desert = w00.desert + (w10.desert - w00.desert) * fx;
    top.cliff  = w00.cliff  + (w10.cliff  - w00.cliff)  * fx;

    bottom.forest = w01.forest + (w11.forest - w01.forest) * fx;
    bottom.snowy  = w01.snowy  + (w11.snowy  - w01.snowy)  * fx;
    bottom.desert = w01.desert + (w11.desert - w01.desert) * fx;
    bottom.cliff  = w01.cliff  + (w11.cliff  - w01.cliff)  * fx;

    result.forest = top.forest + (bottom.forest - top.forest) * fy;
    result.snowy  = top.snowy  + (bottom.snowy  - top.snowy)  * fy;
    result.desert = top.desert + (bottom.desert - top.desert) * fy;
    result.cliff  = top.cliff  + (bottom.cliff  - top.cliff)  * fy;
    return result;
}

/* Decides whether a decoration at pos should be drawn: it must
 * clear a minimum influence threshold, then survives with
 * probability equal to its zone weight - so density fades out
 * gradually near a border instead of stopping on a hard edge. */
static bool ShouldPlaceInZone(Vector2 pos, ZoneType zone, float minWeight)
{
    float weight = ZoneWeightValue(SampleZoneGrid(pos.x, pos.y), zone);
    if (weight < minWeight) return false;
    return RandFloat(0.0f, 1.0f) <= weight;
}

/* ============================================================
 *  Base texture (blended ground color)
 * ============================================================ */
static Color VariantColor(Color base, Color light, Color dark, float patch)
{
    if (patch >= 0.0f) return LerpColor(base, light,  patch);
    return LerpColor(base, dark, -patch);
}

static Color BlendZoneColors(Color forest, Color snowy, Color desert, Color cliff, ZoneWeights w)
{
    float r = (float)forest.r * w.forest + (float)snowy.r * w.snowy + (float)desert.r * w.desert + (float)cliff.r * w.cliff;
    float g = (float)forest.g * w.forest + (float)snowy.g * w.snowy + (float)desert.g * w.desert + (float)cliff.g * w.cliff;
    float b = (float)forest.b * w.forest + (float)snowy.b * w.snowy + (float)desert.b * w.desert + (float)cliff.b * w.cliff;
    float a = (float)forest.a * w.forest + (float)snowy.a * w.snowy + (float)desert.a * w.desert + (float)cliff.a * w.cliff;

    return (Color){
        (unsigned char)TerrainClampf(r, 0.0f, 255.0f),
        (unsigned char)TerrainClampf(g, 0.0f, 255.0f),
        (unsigned char)TerrainClampf(b, 0.0f, 255.0f),
        (unsigned char)TerrainClampf(a, 0.0f, 255.0f)
    };
}

/* Builds the blended ground-color texture at reduced resolution
 * (BASE_TEX_DOWNSCALE) and returns it ready to be upscaled with
 * bilinear filtering - the filtering softens the result further,
 * reinforcing the smooth transition between biomes. */
static Texture2D GenerateBaseTexture(void)
{
    int texW = WORLD_WIDTH  / BASE_TEX_DOWNSCALE;
    int texH = WORLD_HEIGHT / BASE_TEX_DOWNSCALE;

    Color *pixels = (Color *)malloc(sizeof(Color) * (size_t)texW * (size_t)texH);

    for (int ty = 0; ty < texH; ty++)
    {
        float worldY = ((float)ty + 0.5f) * (float)BASE_TEX_DOWNSCALE;
        for (int tx = 0; tx < texW; tx++)
        {
            float worldX = ((float)tx + 0.5f) * (float)BASE_TEX_DOWNSCALE;

            ZoneWeights w = SampleZoneGrid(worldX, worldY);

            float patch = FBM(worldX * PATCH_NOISE_SCALE, worldY * PATCH_NOISE_SCALE, 991,  3) * 2.0f - 1.0f;
            float grain = FBM(worldX * GRAIN_NOISE_SCALE, worldY * GRAIN_NOISE_SCALE, 4021, 2);
            float shade = 0.94f + 0.12f * grain;

            Color forestC = VariantColor(FOREST_GRASS_BASE, FOREST_GRASS_LIGHT, FOREST_GRASS_DARK, patch);
            Color snowyC  = VariantColor(SNOW_BASE,         SNOW_DRIFT_LIGHT,   SNOW_DRIFT_DARK,   patch);
            Color desertC = VariantColor(DESERT_SAND_BASE,  DESERT_SAND_LIGHT,  DESERT_SAND_DARK,  patch);
            Color cliffC  = VariantColor(CLIFF_STONE_BASE,  CLIFF_STONE_LIGHT,  CLIFF_STONE_DARK,  patch);

            Color blended = BlendZoneColors(forestC, snowyC, desertC, cliffC, w);
            pixels[ty * texW + tx] = ShadeColor(blended, shade);
        }
    }

    Image img = {
        .data    = pixels,
        .width   = texW,
        .height  = texH,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Texture2D texture = LoadTextureFromImage(img);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img); /* frees the pixel buffer */

    return texture;
}

/* ============================================================
 *  Decoration pieces (unchanged art, now placed via zone weight)
 * ============================================================ */
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

/* ============================================================
 *  Biome decoration painters
 *
 *  Each scatters within its home quadrant expanded by
 *  AMBIENT_BLEED_MARGIN, and keeps a candidate only if
 *  ShouldPlaceInZone agrees - so trees/rocks/etc. thin out
 *  naturally near a border instead of stopping on a hard line.
 *  Loop counts are scaled up by the expanded/home area ratio so
 *  the home quadrant keeps roughly its original decoration
 *  density despite the larger sampling area.
 * ============================================================ */
static void PaintForestDecor(void)
{
    ZoneType  zone     = ZONE_FOREST;
    Rectangle home     = ZoneRect(zone);
    Rectangle expanded = ZoneRectExpanded(zone, AMBIENT_BLEED_MARGIN);
    float     scale    = AreaScale(home, expanded);

    const Color canopyPalette[] = { FOREST_CANOPY_A, FOREST_CANOPY_B,
                                    FOREST_CANOPY_C, FOREST_CANOPY_D };

    int clusterCount = (int)(42 * scale + 0.5f);
    for (int cluster = 0; cluster < clusterCount; cluster++)
    {
        Vector2 center = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(center, zone, 0.10f)) continue;

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
            if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

            DrawCircleV((Vector2){ position.x + 2, position.y + 3 },
                        RandFloat(10.0f, 18.0f), (Color){ 0, 0, 0, 60 });

            float radius = RandFloat(14.0f, 34.0f);
            Color canopy = canopyPalette[RandInt(0, 3)];
            DrawCircleV(position, radius, canopy);
            DrawCircleV((Vector2){ position.x - radius * 0.3f, position.y - radius * 0.3f },
                        radius * 0.5f, ShadeColor(canopy, 1.25f));
        }
    }

    int soloCount = (int)(120 * scale + 0.5f);
    for (int i = 0; i < soloCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.10f)) continue;

        DrawCircleV((Vector2){ position.x + 1, position.y + 2 }, 12.0f, (Color){ 0, 0, 0, 50 });
        DrawCircleV(position, RandFloat(10.0f, 22.0f), FOREST_CANOPY_C);
    }
}

static void PaintSnowyDecor(void)
{
    ZoneType  zone     = ZONE_SNOWY;
    Rectangle home     = ZoneRect(zone);
    Rectangle expanded = ZoneRectExpanded(zone, AMBIENT_BLEED_MARGIN);
    float     scale    = AreaScale(home, expanded);

    int clusterCount = (int)(32 * scale + 0.5f);
    for (int cluster = 0; cluster < clusterCount; cluster++)
    {
        Vector2 center = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(center, zone, 0.10f)) continue;

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
            if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

            float radius = RandFloat(12.0f, 26.0f);

            DrawCircleV((Vector2){ position.x + 2, position.y + 3 }, radius * 0.9f, (Color){ 60, 70, 90, 60 });
            DrawCircleV(position, radius, SNOW_TREE_BODY);
            DrawCircleV((Vector2){ position.x - radius * 0.25f, position.y - radius * 0.35f },
                        radius * 0.55f, SNOW_TREE_CAP);
        }
    }

    int rockCount = (int)(90 * scale + 0.5f);
    for (int i = 0; i < rockCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.10f)) continue;

        float radius = RandFloat(8.0f, 22.0f);
        DrawCircleV((Vector2){ position.x, position.y + 2 }, radius, (Color){ 0, 0, 0, 50 });
        DrawCircleV(position, radius, SNOW_ROCK_BASE);
        DrawCircleV((Vector2){ position.x - radius * 0.25f, position.y - radius * 0.3f },
                    radius * 0.55f, SNOW_ROCK_HI);
        DrawCircleV((Vector2){ position.x - radius * 0.15f, position.y - radius * 0.55f },
                    radius * 0.35f, SNOW_TREE_CAP);
    }

    int brushCount = (int)(180 * scale + 0.5f);
    for (int i = 0; i < brushCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

        for (int frond = 0; frond < 4; frond++)
        {
            float angle = -PI / 2.0f + (frond - 1.5f) * 0.35f;
            DrawLineEx(position,
                       (Vector2){ position.x + cosf(angle) * 9.0f, position.y + sinf(angle) * 9.0f },
                       1.6f, SNOW_BRUSH);
        }
    }
}

static void PaintDesertDecor(void)
{
    ZoneType  zone     = ZONE_DESERT;
    Rectangle home     = ZoneRect(zone);
    Rectangle expanded = ZoneRectExpanded(zone, AMBIENT_BLEED_MARGIN);
    float     scale    = AreaScale(home, expanded);

    int crackCount = (int)(70 * scale + 0.5f);
    for (int i = 0; i < crackCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x + 40.0f, expanded.x + expanded.width  - 40.0f),
            RandFloat(expanded.y + 40.0f, expanded.y + expanded.height - 40.0f)
        };
        if (!ShouldPlaceInZone(position, zone, 0.12f)) continue;

        float angle    = RandFloat(0.0f, PI * 2.0f);
        float length   = RandFloat(20.0f, 70.0f);
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

    int rockCount = (int)(80 * scale + 0.5f);
    for (int i = 0; i < rockCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.10f)) continue;

        float radius = RandFloat(8.0f, 22.0f);
        DrawCircleV((Vector2){ position.x + 2, position.y + 3 }, radius, (Color){ 0, 0, 0, 55 });
        DrawCircleV(position, radius, DESERT_ROCK);
        DrawCircleV((Vector2){ position.x - radius * 0.3f, position.y - radius * 0.3f },
                    radius * 0.55f, ShadeColor(DESERT_ROCK, 1.20f));
        DrawCircleV((Vector2){ position.x + radius * 0.4f, position.y + radius * 0.3f },
                    radius * 0.35f, ShadeColor(DESERT_ROCK, 0.75f));
    }

    int cactusCount = (int)(26 * scale + 0.5f);
    for (int i = 0; i < cactusCount; i++)
    {
        Vector2 base = {
            RandFloat(expanded.x + 30.0f, expanded.x + expanded.width  - 30.0f),
            RandFloat(expanded.y + 30.0f, expanded.y + expanded.height - 30.0f)
        };
        if (!ShouldPlaceInZone(base, zone, 0.15f)) continue;

        float height = RandFloat(24.0f, 46.0f);
        PaintDesertCactus(base, height, height * 0.28f);
    }

    int pebbleCount = (int)(220 * scale + 0.5f);
    for (int i = 0; i < pebbleCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

        DrawCircleV(position, RandFloat(1.0f, 2.5f), DESERT_PEBBLE);
    }
}

static void PaintCliffsideDecor(void)
{
    ZoneType  zone     = ZONE_CLIFFSIDE;
    Rectangle home     = ZoneRect(zone);
    Rectangle expanded = ZoneRectExpanded(zone, AMBIENT_BLEED_MARGIN);
    float     scale    = AreaScale(home, expanded);

    int clusterCount = (int)(40 * scale + 0.5f);
    for (int cluster = 0; cluster < clusterCount; cluster++)
    {
        Vector2 center = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(center, zone, 0.10f)) continue;

        int rockCount = RandInt(3, 9);
        for (int r = 0; r < rockCount; r++)
        {
            Vector2 position = {
                center.x + RandFloat(-60.0f, 60.0f),
                center.y + RandFloat(-60.0f, 60.0f)
            };
            if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

            float radius = RandFloat(10.0f, 26.0f);

            DrawCircleV((Vector2){ position.x + 3, position.y + 4 }, radius, (Color){ 0, 0, 0, 70 });
            DrawCircleV(position, radius, CLIFF_ROCK);
            DrawCircleV((Vector2){ position.x - radius * 0.30f, position.y - radius * 0.30f },
                        radius * 0.55f, ShadeColor(CLIFF_ROCK, 1.30f));
            DrawCircleV((Vector2){ position.x + radius * 0.35f, position.y + radius * 0.35f },
                        radius * 0.40f, ShadeColor(CLIFF_ROCK, 0.65f));
            DrawCircleLines((int)position.x, (int)position.y, radius, (Color){ 50, 54, 60, 180 });
        }
    }

    /* Streams and bridges are structural set-pieces, so keep them
     * anchored in the home rect rather than letting them wander
     * into a neighbouring biome. */
    for (int i = 0; i < 6; i++)
    {
        Vector2 origin = {
            RandFloat(home.x + 80.0f, home.x + home.width  - 80.0f),
            RandFloat(home.y + 80.0f, home.y + home.height - 80.0f)
        };
        PaintCliffStream(origin);
    }

    for (int i = 0; i < 5; i++)
    {
        Vector2 center = {
            RandFloat(home.x + 100.0f, home.x + home.width  - 100.0f),
            RandFloat(home.y + 100.0f, home.y + home.height - 100.0f)
        };
        PaintCliffBridge(center);
    }

    int grassCount = (int)(200 * scale + 0.5f);
    for (int i = 0; i < grassCount; i++)
    {
        Vector2 position = {
            RandFloat(expanded.x, expanded.x + expanded.width),
            RandFloat(expanded.y, expanded.y + expanded.height)
        };
        if (!ShouldPlaceInZone(position, zone, 0.08f)) continue;

        for (int blade = 0; blade < 3; blade++)
        {
            float angle = -PI / 2.0f + (blade - 1) * 0.4f;
            DrawLineEx(position,
                       (Vector2){ position.x + cosf(angle) * 7.0f, position.y + sinf(angle) * 7.0f },
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

    BuildZoneGrid();
    Texture2D baseTexture = GenerateBaseTexture();

    worldTexture = LoadRenderTexture(WORLD_WIDTH, WORLD_HEIGHT);

    BeginTextureMode(worldTexture);
        Rectangle src = { 0, 0, (float)baseTexture.width, (float)baseTexture.height };
        Rectangle dst = { 0, 0, (float)WORLD_WIDTH, (float)WORLD_HEIGHT };
        DrawTexturePro(baseTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);

        PaintForestDecor();
        PaintSnowyDecor();
        PaintDesertDecor();
        PaintCliffsideDecor();

        PaintPath();
    EndTextureMode();

    UnloadTexture(baseTexture);
    FreeZoneGrid();

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
    /* Computed directly rather than via the (init-only) grid, so
     * this remains valid for the whole lifetime of the game, not
     * just during TerrainInit. */
    ZoneWeights w = ComputeZoneWeights(position.x, position.y);

    ZoneType best       = ZONE_FOREST;
    float    bestWeight = w.forest;

    if (w.snowy  > bestWeight) { best = ZONE_SNOWY;     bestWeight = w.snowy;  }
    if (w.desert > bestWeight) { best = ZONE_DESERT;    bestWeight = w.desert; }
    if (w.cliff  > bestWeight) { best = ZONE_CLIFFSIDE; bestWeight = w.cliff;  }

    return best;
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
