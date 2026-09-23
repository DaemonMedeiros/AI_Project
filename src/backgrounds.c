/* ============================================================
 * backgrounds.c - Procedurally generated battle backgrounds
 *
 * Each background is built from primitive shapes (rects, circles,
 * polygons, lines, triangles) and baked into a RenderTexture once
 * at startup. Combat then just blits the chosen texture.
 * ============================================================ */
#include "../include/backgrounds.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

#define BG_WIDTH   800
#define BG_HEIGHT  600

static RenderTexture2D bgTextures[BG_COUNT];
static bool            initialized;

/* ------------------------------------------------------------
 *  Small helpers
 * ---------------------------------------------------------- */
static float RR(float lo, float hi)
{
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

static int RRI(int lo, int hi) { return GetRandomValue(lo, hi); }

static Color Shade(Color c, float f)
{
    int r = (int)(c.r * f); if (r < 0) r = 0; if (r > 255) r = 255;
    int g = (int)(c.g * f); if (g < 0) g = 0; if (g > 255) g = 255;
    int b = (int)(c.b * f); if (b < 0) b = 0; if (b > 255) b = 255;
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}

static void DrawVGradient(int x, int y, int w, int h, Color top, Color bot)
{
    for (int i = 0; i < h; i++)
    {
        float t = (float)i / (float)(h > 1 ? h - 1 : 1);
        Color c = {
            (unsigned char)(top.r + (bot.r - top.r) * t),
            (unsigned char)(top.g + (bot.g - top.g) * t),
            (unsigned char)(top.b + (bot.b - top.b) * t),
            255
        };
        DrawRectangle(x, y + i, w, 1, c);
    }
}

static void DrawStars(int count, int yMax, Color tint)
{
    for (int i = 0; i < count; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(0, yMax);
        int r = RRI(1, 2);
        unsigned char a = (unsigned char)RRI(140, 255);
        DrawCircle(x, y, (float)r, (Color){ tint.r, tint.g, tint.b, a });
    }
}

/* ------------------------------------------------------------
 *  Deep forest
 * ---------------------------------------------------------- */
static void BuildForest(bool night)
{
    if (night)
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 8, 14, 28, 255 },
                      (Color){ 20, 28, 20, 255 });
        DrawStars(90, 260, (Color){ 220, 230, 255, 255 });

        DrawCircle(650, 110, 42, (Color){ 220, 225, 200, 255 });
        DrawCircle(650, 110, 34, (Color){ 245, 245, 220, 255 });
        DrawCircle(638, 100, 8,  (Color){ 210, 215, 190, 255 });
        DrawCircle(660, 122, 6,  (Color){ 210, 215, 190, 255 });
    }
    else
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 120, 170, 200, 255 },
                      (Color){ 200, 220, 180, 255 });

        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(120, 100, 90, (Color){ 255, 240, 180, 60 });
        DrawCircle(120, 100, 55, (Color){ 255, 245, 200, 130 });
        EndBlendMode();
        DrawCircle(120, 100, 34, (Color){ 255, 250, 220, 255 });
    }

    for (int i = 0; i < 26; i++)
    {
        int x = RRI(-20, BG_WIDTH + 20);
        int y = RRI(180, 320);
        int w = RRI(40, 90);
        int h = RRI(60, 130);
        Color c = night ? (Color){ 12, 30, 20, 255 } : (Color){ 40, 90, 55, 255 };
        DrawEllipse(x, y, (float)w * 0.5f, (float)h * 0.5f, c);
    }

    for (int i = 0; i < 16; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int trunkW = RRI(10, 22);
        int trunkTop = RRI(140, 240);
        Color trunkCol = night ? (Color){ 30, 22, 18, 255 } : (Color){ 70, 45, 28, 255 };
        DrawRectangle(x, trunkTop, trunkW, BG_HEIGHT - trunkTop, trunkCol);

        int canopyR = RRI(45, 80);
        Color canopyCol = night ? (Color){ 18, 45, 26, 255 } : (Color){ 55, 120, 60, 255 };
        DrawCircle(x + trunkW / 2, trunkTop - 10, (float)canopyR, canopyCol);
        DrawCircle(x + trunkW / 2 - 25, trunkTop + 5, (float)canopyR * 0.75f, Shade(canopyCol, 0.85f));
        DrawCircle(x + trunkW / 2 + 25, trunkTop + 5, (float)canopyR * 0.75f, Shade(canopyCol, 0.9f));
    }

    Color floorNear = night ? (Color){ 20, 40, 24, 255 } : (Color){ 55, 110, 55, 255 };
    Color floorFar  = night ? (Color){ 10, 22, 14, 255 } : (Color){ 30, 70, 35, 255 };
    DrawVGradient(0, 430, BG_WIDTH, BG_HEIGHT - 430, floorFar, floorNear);

    for (int i = 0; i < 180; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(440, BG_HEIGHT - 5);
        float len = RR(6.0f, 16.0f);
        Color g = night ? (Color){ 20, 55, 30, 200 } : (Color){ 40, 110, 50, 220 };
        DrawLineEx((Vector2){ (float)x, (float)y },
                   (Vector2){ x + RR(-2.0f, 2.0f), y - len }, 1.5f, g);
    }

    for (int i = 0; i < 10; i++)
    {
        int x = RRI(20, BG_WIDTH - 20);
        int y = RRI(470, BG_HEIGHT - 20);
        Color fr = night ? (Color){ 15, 45, 25, 220 } : (Color){ 35, 95, 45, 230 };
        for (int f = 0; f < 5; f++)
        {
            float a = -PI / 2.0f + (f - 2) * 0.35f;
            DrawLineEx((Vector2){ (float)x, (float)y },
                       (Vector2){ x + cosf(a) * 26.0f, y + sinf(a) * 26.0f }, 3.0f, fr);
        }
    }
}

/* ------------------------------------------------------------
 *  Forest path with mountains
 * ---------------------------------------------------------- */
static void DrawMountain(float cx, float baseY, float width, float height,
                         Color col, Color shadow)
{
    Vector2 p1 = { cx - width * 0.5f, baseY };
    Vector2 p2 = { cx, baseY - height };
    Vector2 p3 = { cx + width * 0.5f, baseY };
    DrawTriangle(p1, p2, p3, col);

    Vector2 s1 = { cx - width * 0.5f, baseY };
    Vector2 s2 = { cx, baseY - height };
    Vector2 s3 = { cx - width * 0.15f, baseY };
    DrawTriangle(s1, s2, s3, shadow);

    float capH = height * 0.22f;
    Vector2 c1 = { cx - (width * 0.5f) * (capH / height), baseY - height + capH };
    Vector2 c2 = { cx, baseY - height };
    Vector2 c3 = { cx + (width * 0.5f) * (capH / height), baseY - height + capH };
    DrawTriangle(c1, c2, c3, (Color){ 240, 245, 255, 255 });

    Vector2 cs = { cx, baseY - height };
    Vector2 c1s = { cx - (width * 0.5f) * (capH / height), baseY - height + capH };
    Vector2 c2s = { cx - 4.0f, baseY - height + capH };
    DrawTriangle(c1s, cs, c2s, (Color){ 200, 210, 230, 255 });
}

static void BuildPathMountains(bool night)
{
    if (night)
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 12, 18, 40, 255 },
                      (Color){ 45, 45, 70, 255 });
        DrawStars(110, 240, (Color){ 220, 230, 255, 255 });
        DrawCircle(690, 90, 36, (Color){ 220, 225, 200, 255 });
        DrawCircle(690, 90, 30, (Color){ 245, 245, 225, 255 });
    }
    else
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 255, 200, 140, 255 },
                      (Color){ 180, 210, 230, 255 });

        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(660, 130, 100, (Color){ 255, 200, 120, 70 });
        DrawCircle(660, 130, 60,  (Color){ 255, 230, 160, 150 });
        EndBlendMode();
        DrawCircle(660, 130, 32, (Color){ 255, 245, 210, 255 });
    }

    Color mFar  = night ? (Color){ 35, 40, 65, 255 }  : (Color){ 130, 150, 175, 255 };
    Color mFarS = night ? (Color){ 25, 30, 55, 255 }  : (Color){ 100, 120, 150, 255 };
    DrawMountain(120, 320, 260, 150, mFar, mFarS);
    DrawMountain(360, 320, 320, 190, mFar, mFarS);
    DrawMountain(620, 320, 280, 160, mFar, mFarS);
    DrawMountain(760, 320, 220, 130, mFar, mFarS);

    Color mMid  = night ? (Color){ 22, 28, 48, 255 } : (Color){ 80, 100, 125, 255 };
    Color mMidS = night ? (Color){ 15, 20, 38, 255 } : (Color){ 55, 75, 100, 255 };
    DrawMountain(60,  360, 220, 110, mMid, mMidS);
    DrawMountain(280, 360, 240, 130, mMid, mMidS);
    DrawMountain(520, 360, 260, 120, mMid, mMidS);
    DrawMountain(760, 360, 240, 140, mMid, mMidS);

    Color treeFar = night ? (Color){ 12, 25, 22, 255 } : (Color){ 40, 75, 55, 255 };
    for (int i = 0; i < 90; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(355, 400);
        int r = RRI(6, 14);
        DrawCircle(x, y, (float)r, treeFar);
    }

    Color pathFar  = night ? (Color){ 45, 38, 30, 255 } : (Color){ 140, 110, 75, 255 };
    Color pathNear = night ? (Color){ 30, 25, 20, 255 } : (Color){ 100, 75, 50, 255 };
    DrawVGradient(0, 400, BG_WIDTH, BG_HEIGHT - 400, pathFar, pathNear);

    for (int i = 0; i < 40; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(405, BG_HEIGHT);
        float w = RR(30.0f, 80.0f);
        Color g = night ? (Color){ 18, 40, 24, 220 } : (Color){ 55, 100, 55, 220 };
        DrawEllipse(x, y, w * 0.5f, 8.0f, g);
    }

    for (int i = 0; i < 14; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(430, BG_HEIGHT - 10);
        int r = RRI(4, 10);
        Color rock = night ? (Color){ 55, 55, 65, 255 } : (Color){ 120, 110, 100, 255 };
        DrawCircle(x, y, (float)r, rock);
        DrawCircle(x + r / 2, y + 2, (float)r * 0.6f, Shade(rock, 0.8f));
    }

    for (int i = 0; i < 60; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(420, BG_HEIGHT);
        Color peb = night ? (Color){ 20, 18, 15, 200 } : (Color){ 80, 65, 50, 200 };
        DrawCircle(x, y, RR(1.0f, 2.5f), peb);
    }
}

/* ------------------------------------------------------------
 *  Rocky desert
 * ---------------------------------------------------------- */
static void DrawMesa(float cx, float baseY, float width, float height,
                     Color top, Color side, Color shadow)
{
    float topW = width * 0.72f;
    DrawRectangle((int)(cx - topW * 0.5f), (int)(baseY - height), (int)topW, (int)height, side);
    DrawRectangle((int)(cx - topW * 0.5f), (int)(baseY - height), (int)topW, 4, top);
    DrawRectangle((int)(cx + topW * 0.5f - topW * 0.18f), (int)(baseY - height),
                  (int)(topW * 0.18f), (int)height, shadow);
    DrawRectangle((int)(cx - topW * 0.5f), (int)(baseY - height),
                  (int)(topW * 0.10f), (int)height, top);
}

static void BuildDesert(bool night)
{
    if (night)
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 10, 15, 35, 255 },
                      (Color){ 60, 40, 55, 255 });
        DrawStars(120, 220, (Color){ 220, 230, 255, 255 });
        DrawCircle(120, 90, 34, (Color){ 220, 225, 200, 255 });
        DrawCircle(120, 90, 28, (Color){ 245, 245, 225, 255 });
        DrawCircle(112, 82, 6,  (Color){ 210, 215, 190, 255 });
    }
    else
    {
        DrawVGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                      (Color){ 250, 180, 100, 255 },
                      (Color){ 255, 225, 180, 255 });

        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(660, 110, 100, (Color){ 255, 220, 140, 80 });
        DrawCircle(660, 110, 55,  (Color){ 255, 245, 190, 160 });
        EndBlendMode();
        DrawCircle(660, 110, 30, (Color){ 255, 250, 220, 255 });
    }

    Color mFar  = night ? (Color){ 45, 30, 55, 255 } : (Color){ 190, 120, 90, 255 };
    Color mFarT = night ? (Color){ 55, 38, 65, 255 } : (Color){ 215, 145, 100, 255 };
    Color mFarS = night ? (Color){ 30, 20, 40, 255 } : (Color){ 150, 90, 65, 255 };
    DrawMesa(150, 380, 200, 90,  mFarT, mFar,  mFarS);
    DrawMesa(400, 380, 260, 120, mFarT, mFar,  mFarS);
    DrawMesa(680, 380, 230, 100, mFarT, mFar,  mFarS);

    Color mMid  = night ? (Color){ 55, 38, 60, 255 } : (Color){ 200, 130, 95, 255 };
    Color mMidT = night ? (Color){ 68, 48, 75, 255 } : (Color){ 225, 155, 110, 255 };
    Color mMidS = night ? (Color){ 38, 25, 45, 255 } : (Color){ 160, 100, 70, 255 };
    DrawMesa(60,  420, 180, 80,  mMidT, mMid, mMidS);
    DrawMesa(300, 420, 220, 100, mMidT, mMid, mMidS);
    DrawMesa(560, 420, 200, 90,  mMidT, mMid, mMidS);
    DrawMesa(780, 420, 180, 70,  mMidT, mMid, mMidS);

    Color groundFar  = night ? (Color){ 60, 40, 55, 255 } : (Color){ 230, 170, 120, 255 };
    Color groundNear = night ? (Color){ 40, 28, 40, 255 } : (Color){ 190, 130, 85, 255 };
    DrawVGradient(0, 410, BG_WIDTH, BG_HEIGHT - 410, groundFar, groundNear);

    for (int i = 0; i < 40; i++)
    {
        int x = RRI(0, BG_WIDTH);
        int y = RRI(430, BG_HEIGHT);
        float w = RR(30.0f, 90.0f);
        Color ripple = night ? (Color){ 55, 38, 50, 180 } : (Color){ 210, 150, 100, 200 };
        DrawEllipse(x, y, w * 0.5f, 3.0f, ripple);
    }

    for (int i = 0; i < 20; i++)
    {
        int x = RRI(20, BG_WIDTH - 20);
        int y = RRI(460, BG_HEIGHT - 15);
        int r = RRI(4, 12);
        Color rock = night ? (Color){ 45, 35, 45, 255 } : (Color){ 120, 80, 60, 255 };
        DrawCircle(x, y, (float)r, rock);
        DrawCircle(x + r / 2, y + 2, (float)r * 0.6f, Shade(rock, 0.75f));
    }

    for (int i = 0; i < 4; i++)
    {
        int x = RRI(60, BG_WIDTH - 60);
        int baseY = RRI(470, BG_HEIGHT - 20);
        int h = RRI(40, 70);
        Color cactus = night ? (Color){ 30, 55, 40, 255 } : (Color){ 70, 110, 70, 255 };
        DrawRectangle(x - 5, baseY - h, 10, h, cactus);
        DrawRectangle(x - 18, baseY - h + 15, 12, 6, cactus);
        DrawRectangle(x - 18, baseY - h + 15, 6, 20, cactus);
        DrawRectangle(x + 6, baseY - h + 25, 12, 6, cactus);
        DrawRectangle(x + 12, baseY - h + 25, 6, 20, cactus);
    }

    if (!night)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 5; i++)
        {
            int y = 380 + i * 12;
            DrawRectangle(0, y, BG_WIDTH, 2, (Color){ 255, 240, 200, 20 });
        }
        EndBlendMode();
    }
}

/* ------------------------------------------------------------
 *  Baking
 * ---------------------------------------------------------- */
static void BakeOne(BackgroundType type)
{
    BeginTextureMode(bgTextures[type]);
        ClearBackground(BLACK);

        switch (type)
        {
            case BG_FOREST_DAY:   BuildForest(false);         break;
            case BG_FOREST_NIGHT: BuildForest(true);          break;
            case BG_PATH_DAY:     BuildPathMountains(false);  break;
            case BG_PATH_NIGHT:   BuildPathMountains(true);   break;
            case BG_DESERT_DAY:   BuildDesert(false);         break;
            case BG_DESERT_NIGHT: BuildDesert(true);          break;
            default: break;
        }
    EndTextureMode();
}

void BackgroundsInit(void)
{
    if (initialized) BackgroundsUnload();

    for (int i = 0; i < BG_COUNT; i++)
    {
        bgTextures[i] = LoadRenderTexture(BG_WIDTH, BG_HEIGHT);
        BakeOne((BackgroundType)i);
    }
    initialized = true;
}

void BackgroundsUnload(void)
{
    if (!initialized) return;
    for (int i = 0; i < BG_COUNT; i++)
        UnloadRenderTexture(bgTextures[i]);
    initialized = false;
}

BackgroundType BackgroundsPickRandom(void)
{
    return (BackgroundType)GetRandomValue(0, BG_COUNT - 1);
}

void BackgroundsDraw(BackgroundType type)
{
    if (type < 0 || type >= BG_COUNT) return;
    Rectangle src = { 0, 0, (float)BG_WIDTH, -(float)BG_HEIGHT };
    DrawTextureRec(bgTextures[type].texture, src, (Vector2){ 0, 0 }, WHITE);
}

const char *BackgroundsGetName(BackgroundType type)
{
    switch (type)
    {
        case BG_FOREST_DAY:   return "Deep Forest (Day)";
        case BG_FOREST_NIGHT: return "Deep Forest (Night)";
        case BG_PATH_DAY:     return "Forest Path (Day)";
        case BG_PATH_NIGHT:   return "Forest Path (Night)";
        case BG_DESERT_DAY:   return "Rocky Desert (Day)";
        case BG_DESERT_NIGHT: return "Rocky Desert (Night)";
        default:              return "Unknown";
    }
}