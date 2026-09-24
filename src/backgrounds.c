/* ============================================================
 * backgrounds.c - Procedurally generated battle backgrounds
 *
 * Each background is built from primitive shapes and baked
 * into a RenderTexture once at startup.
 * ============================================================ */
#include "../include/backgrounds.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

#define BG_WIDTH   800
#define BG_HEIGHT  600

static RenderTexture2D bgTextures[BG_COUNT];
static bool initialized;

/* ---------- Helpers ---------- */
static float RandFloat(float low, float high)
{
    return low + (float)GetRandomValue(0, 10000) / 10000.0f * (high - low);
}

static int RandInt(int low, int high) { return GetRandomValue(low, high); }

static Color ShadeColor(Color color, float factor)
{
    int red   = (int)(color.r * factor);
    int green = (int)(color.g * factor);
    int blue  = (int)(color.b * factor);
    if (red < 0) red = 0; if (red > 255) red = 255;
    if (green < 0) green = 0; if (green > 255) green = 255;
    if (blue < 0) blue = 0; if (blue > 255) blue = 255;
    return (Color){ (unsigned char)red, (unsigned char)green, (unsigned char)blue, color.a };
}

static void DrawVerticalGradient(int x, int y, int width, int height, Color top, Color bottom)
{
    for (int row = 0; row < height; row++)
    {
        float t = (float)row / (float)(height > 1 ? height - 1 : 1);
        Color color = {
            (unsigned char)(top.r + (bottom.r - top.r) * t),
            (unsigned char)(top.g + (bottom.g - top.g) * t),
            (unsigned char)(top.b + (bottom.b - top.b) * t),
            255
        };
        DrawRectangle(x, y + row, width, 1, color);
    }
}

static void DrawStars(int count, int maxY, Color tint)
{
    for (int i = 0; i < count; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(0, maxY);
        int radius = RandInt(1, 2);
        unsigned char alpha = (unsigned char)RandInt(140, 255);
        DrawCircle(x, y, (float)radius, (Color){ tint.r, tint.g, tint.b, alpha });
    }
}

/* ---------- Deep forest ---------- */
static void BuildForest(bool night)
{
    if (night)
    {
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
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
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
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
        int x = RandInt(-20, BG_WIDTH + 20);
        int y = RandInt(180, 320);
        int width = RandInt(40, 90);
        int height = RandInt(60, 130);
        Color color = night ? (Color){ 12, 30, 20, 255 } : (Color){ 40, 90, 55, 255 };
        DrawEllipse(x, y, (float)width * 0.5f, (float)height * 0.5f, color);
    }

    for (int i = 0; i < 16; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int trunkWidth = RandInt(10, 22);
        int trunkTop = RandInt(140, 240);
        Color trunkColor = night ? (Color){ 30, 22, 18, 255 } : (Color){ 70, 45, 28, 255 };
        DrawRectangle(x, trunkTop, trunkWidth, BG_HEIGHT - trunkTop, trunkColor);

        int canopyRadius = RandInt(45, 80);
        Color canopyColor = night ? (Color){ 18, 45, 26, 255 } : (Color){ 55, 120, 60, 255 };
        DrawCircle(x + trunkWidth / 2, trunkTop - 10, (float)canopyRadius, canopyColor);
        DrawCircle(x + trunkWidth / 2 - 25, trunkTop + 5, (float)canopyRadius * 0.75f, ShadeColor(canopyColor, 0.85f));
        DrawCircle(x + trunkWidth / 2 + 25, trunkTop + 5, (float)canopyRadius * 0.75f, ShadeColor(canopyColor, 0.9f));
    }

    Color floorNear = night ? (Color){ 20, 40, 24, 255 } : (Color){ 55, 110, 55, 255 };
    Color floorFar  = night ? (Color){ 10, 22, 14, 255 } : (Color){ 30, 70, 35, 255 };
    DrawVerticalGradient(0, 430, BG_WIDTH, BG_HEIGHT - 430, floorFar, floorNear);

    for (int i = 0; i < 180; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(440, BG_HEIGHT - 5);
        float length = RandFloat(6.0f, 16.0f);
        Color grass = night ? (Color){ 20, 55, 30, 200 } : (Color){ 40, 110, 50, 220 };
        DrawLineEx((Vector2){ (float)x, (float)y },
                   (Vector2){ x + RandFloat(-2.0f, 2.0f), y - length }, 1.5f, grass);
    }

    for (int i = 0; i < 10; i++)
    {
        int x = RandInt(20, BG_WIDTH - 20);
        int y = RandInt(470, BG_HEIGHT - 20);
        Color fern = night ? (Color){ 15, 45, 25, 220 } : (Color){ 35, 95, 45, 230 };
        for (int frond = 0; frond < 5; frond++)
        {
            float angle = -PI / 2.0f + (frond - 2) * 0.35f;
            DrawLineEx((Vector2){ (float)x, (float)y },
                       (Vector2){ x + cosf(angle) * 26.0f, y + sinf(angle) * 26.0f }, 3.0f, fern);
        }
    }
}

/* ---------- Forest path with mountains ---------- */
static void DrawMountain(float centerX, float baseY, float width, float height,
                         Color color, Color shadow)
{
    Vector2 peak = { centerX, baseY - height };
    Vector2 leftBase  = { centerX - width * 0.5f, baseY };
    Vector2 rightBase = { centerX + width * 0.5f, baseY };
    DrawTriangle(leftBase, peak, rightBase, color);

    Vector2 shadowLeft  = { centerX - width * 0.5f, baseY };
    Vector2 shadowRight = { centerX - width * 0.15f, baseY };
    DrawTriangle(shadowLeft, peak, shadowRight, shadow);

    float capHeight = height * 0.22f;
    Vector2 capLeft  = { centerX - (width * 0.5f) * (capHeight / height), baseY - height + capHeight };
    Vector2 capRight = { centerX + (width * 0.5f) * (capHeight / height), baseY - height + capHeight };
    DrawTriangle(capLeft, peak, capRight, (Color){ 240, 245, 255, 255 });

    Vector2 capShadow = { centerX - 4.0f, baseY - height + capHeight };
    DrawTriangle(capLeft, peak, capShadow, (Color){ 200, 210, 230, 255 });
}

static void BuildPathMountains(bool night)
{
    if (night)
    {
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                             (Color){ 12, 18, 40, 255 },
                             (Color){ 45, 45, 70, 255 });
        DrawStars(110, 240, (Color){ 220, 230, 255, 255 });
        DrawCircle(690, 90, 36, (Color){ 220, 225, 200, 255 });
        DrawCircle(690, 90, 30, (Color){ 245, 245, 225, 255 });
    }
    else
    {
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                             (Color){ 255, 200, 140, 255 },
                             (Color){ 180, 210, 230, 255 });
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(660, 130, 100, (Color){ 255, 200, 120, 70 });
        DrawCircle(660, 130, 60,  (Color){ 255, 230, 160, 150 });
        EndBlendMode();
        DrawCircle(660, 130, 32, (Color){ 255, 245, 210, 255 });
    }

    Color farMountain  = night ? (Color){ 35, 40, 65, 255 }  : (Color){ 130, 150, 175, 255 };
    Color farShadow    = night ? (Color){ 25, 30, 55, 255 }  : (Color){ 100, 120, 150, 255 };
    DrawMountain(120, 320, 260, 150, farMountain, farShadow);
    DrawMountain(360, 320, 320, 190, farMountain, farShadow);
    DrawMountain(620, 320, 280, 160, farMountain, farShadow);
    DrawMountain(760, 320, 220, 130, farMountain, farShadow);

    Color midMountain = night ? (Color){ 22, 28, 48, 255 } : (Color){ 80, 100, 125, 255 };
    Color midShadow   = night ? (Color){ 15, 20, 38, 255 } : (Color){ 55, 75, 100, 255 };
    DrawMountain(60,  360, 220, 110, midMountain, midShadow);
    DrawMountain(280, 360, 240, 130, midMountain, midShadow);
    DrawMountain(520, 360, 260, 120, midMountain, midShadow);
    DrawMountain(760, 360, 240, 140, midMountain, midShadow);

    Color farTree = night ? (Color){ 12, 25, 22, 255 } : (Color){ 40, 75, 55, 255 };
    for (int i = 0; i < 90; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(355, 400);
        int radius = RandInt(6, 14);
        DrawCircle(x, y, (float)radius, farTree);
    }

    Color pathFar  = night ? (Color){ 45, 38, 30, 255 } : (Color){ 140, 110, 75, 255 };
    Color pathNear = night ? (Color){ 30, 25, 20, 255 } : (Color){ 100, 75, 50, 255 };
    DrawVerticalGradient(0, 400, BG_WIDTH, BG_HEIGHT - 400, pathFar, pathNear);

    for (int i = 0; i < 40; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(405, BG_HEIGHT);
        float width = RandFloat(30.0f, 80.0f);
        Color grass = night ? (Color){ 18, 40, 24, 220 } : (Color){ 55, 100, 55, 220 };
        DrawEllipse(x, y, width * 0.5f, 8.0f, grass);
    }

    for (int i = 0; i < 14; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(430, BG_HEIGHT - 10);
        int radius = RandInt(4, 10);
        Color rock = night ? (Color){ 55, 55, 65, 255 } : (Color){ 120, 110, 100, 255 };
        DrawCircle(x, y, (float)radius, rock);
        DrawCircle(x + radius / 2, y + 2, (float)radius * 0.6f, ShadeColor(rock, 0.8f));
    }

    for (int i = 0; i < 60; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(420, BG_HEIGHT);
        Color pebble = night ? (Color){ 20, 18, 15, 200 } : (Color){ 80, 65, 50, 200 };
        DrawCircle(x, y, RandFloat(1.0f, 2.5f), pebble);
    }
}

/* ---------- Rocky desert ---------- */
static void DrawMesa(float centerX, float baseY, float width, float height,
                     Color top, Color side, Color shadow)
{
    float topWidth = width * 0.72f;
    DrawRectangle((int)(centerX - topWidth * 0.5f), (int)(baseY - height),
                  (int)topWidth, (int)height, side);
    DrawRectangle((int)(centerX - topWidth * 0.5f), (int)(baseY - height),
                  (int)topWidth, 4, top);
    DrawRectangle((int)(centerX + topWidth * 0.5f - topWidth * 0.18f), (int)(baseY - height),
                  (int)(topWidth * 0.18f), (int)height, shadow);
    DrawRectangle((int)(centerX - topWidth * 0.5f), (int)(baseY - height),
                  (int)(topWidth * 0.10f), (int)height, top);
}

static void BuildDesert(bool night)
{
    if (night)
    {
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                             (Color){ 10, 15, 35, 255 },
                             (Color){ 60, 40, 55, 255 });
        DrawStars(120, 220, (Color){ 220, 230, 255, 255 });
        DrawCircle(120, 90, 34, (Color){ 220, 225, 200, 255 });
        DrawCircle(120, 90, 28, (Color){ 245, 245, 225, 255 });
        DrawCircle(112, 82, 6,  (Color){ 210, 215, 190, 255 });
    }
    else
    {
        DrawVerticalGradient(0, 0, BG_WIDTH, BG_HEIGHT,
                             (Color){ 250, 180, 100, 255 },
                             (Color){ 255, 225, 180, 255 });
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(660, 110, 100, (Color){ 255, 220, 140, 80 });
        DrawCircle(660, 110, 55,  (Color){ 255, 245, 190, 160 });
        EndBlendMode();
        DrawCircle(660, 110, 30, (Color){ 255, 250, 220, 255 });
    }

    Color farMesa   = night ? (Color){ 45, 30, 55, 255 } : (Color){ 190, 120, 90, 255 };
    Color farTop    = night ? (Color){ 55, 38, 65, 255 } : (Color){ 215, 145, 100, 255 };
    Color farShadow = night ? (Color){ 30, 20, 40, 255 } : (Color){ 150, 90, 65, 255 };
    DrawMesa(150, 380, 200, 90,  farTop, farMesa,  farShadow);
    DrawMesa(400, 380, 260, 120, farTop, farMesa,  farShadow);
    DrawMesa(680, 380, 230, 100, farTop, farMesa,  farShadow);

    Color midMesa   = night ? (Color){ 55, 38, 60, 255 } : (Color){ 200, 130, 95, 255 };
    Color midTop    = night ? (Color){ 68, 48, 75, 255 } : (Color){ 225, 155, 110, 255 };
    Color midShadow = night ? (Color){ 38, 25, 45, 255 } : (Color){ 160, 100, 70, 255 };
    DrawMesa(60,  420, 180, 80,  midTop, midMesa, midShadow);
    DrawMesa(300, 420, 220, 100, midTop, midMesa, midShadow);
    DrawMesa(560, 420, 200, 90,  midTop, midMesa, midShadow);
    DrawMesa(780, 420, 180, 70,  midTop, midMesa, midShadow);

    Color groundFar  = night ? (Color){ 60, 40, 55, 255 } : (Color){ 230, 170, 120, 255 };
    Color groundNear = night ? (Color){ 40, 28, 40, 255 } : (Color){ 190, 130, 85, 255 };
    DrawVerticalGradient(0, 410, BG_WIDTH, BG_HEIGHT - 410, groundFar, groundNear);

    for (int i = 0; i < 40; i++)
    {
        int x = RandInt(0, BG_WIDTH);
        int y = RandInt(430, BG_HEIGHT);
        float width = RandFloat(30.0f, 90.0f);
        Color ripple = night ? (Color){ 55, 38, 50, 180 } : (Color){ 210, 150, 100, 200 };
        DrawEllipse(x, y, width * 0.5f, 3.0f, ripple);
    }

    for (int i = 0; i < 20; i++)
    {
        int x = RandInt(20, BG_WIDTH - 20);
        int y = RandInt(460, BG_HEIGHT - 15);
        int radius = RandInt(4, 12);
        Color rock = night ? (Color){ 45, 35, 45, 255 } : (Color){ 120, 80, 60, 255 };
        DrawCircle(x, y, (float)radius, rock);
        DrawCircle(x + radius / 2, y + 2, (float)radius * 0.6f, ShadeColor(rock, 0.75f));
    }

    for (int i = 0; i < 4; i++)
    {
        int x = RandInt(60, BG_WIDTH - 60);
        int baseY = RandInt(470, BG_HEIGHT - 20);
        int height = RandInt(40, 70);
        Color cactus = night ? (Color){ 30, 55, 40, 255 } : (Color){ 70, 110, 70, 255 };
        DrawRectangle(x - 5, baseY - height, 10, height, cactus);
        DrawRectangle(x - 18, baseY - height + 15, 12, 6, cactus);
        DrawRectangle(x - 18, baseY - height + 15, 6, 20, cactus);
        DrawRectangle(x + 6, baseY - height + 25, 12, 6, cactus);
        DrawRectangle(x + 12, baseY - height + 25, 6, 20, cactus);
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

/* ---------- Baking ---------- */
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
    Rectangle source = { 0, 0, (float)BG_WIDTH, -(float)BG_HEIGHT };
    DrawTextureRec(bgTextures[type].texture, source, (Vector2){ 0, 0 }, WHITE);
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