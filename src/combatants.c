/* ============================================================
 * combatants.c - Combatant sprite rendering from textures
 * ============================================================ */
#include "../include/combatants.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

#define KNIGHT_TEXTURE_PATH  "./resources/textures/knight.png"
#define WOLF_TEXTURE_PATH    "./resources/textures/wolf.png"

/* Idle / animation tunables */
#define IDLE_BOB_AMPLITUDE    1.5f
#define IDLE_BOB_SPEED        3.0f
#define WOLF_BOB_AMPLITUDE    1.0f
#define WOLF_BOB_SPEED        6.0f
#define LUNGE_DISTANCE        22.0f
#define WOLF_LUNGE_DISTANCE   24.0f

/* On-screen sprite heights at scale = 1.0. These are the values
 * you tune; the actual texture's native resolution is ignored. */
#define KNIGHT_BASE_HEIGHT    96.0f
#define WOLF_BASE_HEIGHT      72.0f

static Texture2D knightTexture;
static Texture2D wolfTexture;
static bool      texturesLoaded;

/* ------------------------------------------------------------
 *  Lifecycle
 * ---------------------------------------------------------- */
void CombatantsInit(void)
{
    if (texturesLoaded) return;

    knightTexture = LoadTexture(KNIGHT_TEXTURE_PATH);
    wolfTexture   = LoadTexture(WOLF_TEXTURE_PATH);

    if (knightTexture.id == 0)
        printf("WARNING: failed to load knight texture: %s\n", KNIGHT_TEXTURE_PATH);
    else
        printf("Loaded knight: %dx%d\n", knightTexture.width, knightTexture.height);

    if (wolfTexture.id == 0)
        printf("WARNING: failed to load wolf texture: %s\n", WOLF_TEXTURE_PATH);
    else
        printf("Loaded wolf: %dx%d\n", wolfTexture.width, wolfTexture.height);

    texturesLoaded = true;
}

void CombatantsUnload(void)
{
    if (!texturesLoaded) return;

    UnloadTexture(knightTexture);
    UnloadTexture(wolfTexture);
    texturesLoaded = false;
}

float CombatantGetKnightBaseHeight(void)
{
    return KNIGHT_BASE_HEIGHT;
}

/* ------------------------------------------------------------
 *  Drawing helpers
 * ---------------------------------------------------------- */
/* Draw a texture anchored by its feet at `feetPos`, scaled so its
 * height equals `targetHeight`, optionally mirrored horizontally.
 *
 * Feet-anchoring means `feetPos` is where the sprite's soles sit,
 * which is what you want for a character standing on the ground. */
static void DrawFeetAnchored(Texture2D texture, Vector2 feetPos,
                             float targetHeight, bool faceLeft)
{
    if (texture.id == 0) return;

    float scale  = targetHeight / (float)texture.height;
    float width  = (float)texture.width  * scale;
    float height = targetHeight;

    Rectangle source = {
        0, 0,
        faceLeft ? -(float)texture.width : (float)texture.width,
        (float)texture.height
    };
    Rectangle dest = {
        feetPos.x - width * 0.5f,
        feetPos.y - height,
        width, height
    };
    DrawTexturePro(texture, source, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

/* ------------------------------------------------------------
 *  Knight
 * ---------------------------------------------------------- */
void CombatantDrawKnight(Vector2 pos, bool facingRight,
                         CombatantPose pose, float scale)
{
    float bob   = sinf(pose.time * IDLE_BOB_SPEED) * IDLE_BOB_AMPLITUDE;
    float lunge = pose.poseT * LUNGE_DISTANCE * scale;

    Vector2 feetPos = {
        pos.x + (facingRight ? lunge : -lunge),
        pos.y + bob
    };

    DrawFeetAnchored(knightTexture, feetPos,
                     KNIGHT_BASE_HEIGHT * scale, !facingRight);
}

/* ------------------------------------------------------------
 *  Wolf
 * ---------------------------------------------------------- */
void CombatantDrawWolf(Vector2 pos, bool facingRight,
                       CombatantPose pose, float scale)
{
    float bob   = sinf(pose.time * WOLF_BOB_SPEED) * WOLF_BOB_AMPLITUDE;
    float lunge = pose.poseT * WOLF_LUNGE_DISTANCE * scale;

    Vector2 feetPos = {
        pos.x + (facingRight ? lunge : -lunge),
        pos.y + bob
    };

    DrawFeetAnchored(wolfTexture, feetPos,
                     WOLF_BASE_HEIGHT * scale, !facingRight);
}