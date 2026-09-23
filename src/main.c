/* ============================================================
 * main.c - Simple Turn-Based Fantasy RPG
 *
 * World terrain lives in terrain.c. Combat (turn logic + spell
 * VFX) lives in combat.c. Battle backgrounds and combatant
 * sprites live in backgrounds.c / combatants.c.
 * ============================================================ */
#include "raylib.h"
#include "raymath.h"
#include "../include/terrain.h"
#include "../include/combat.h"
#include "../include/backgrounds.h"
#include "../include/combatants.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------
 *  Constants
 * ---------------------------------------------------------- */
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#define MAX_POTIONS_WORLD   6
#define STARTING_POTIONS    3

#define ENCOUNTER_CHANCE           15
#define ENCOUNTER_CHECK_INTERVAL   0.35f

#define PLAYER_RADIUS       14.0f
#define PLAYER_MAX_SPEED    220.0f
#define PLAYER_ACCEL        900.0f
#define PLAYER_DECEL        1400.0f

#define CAMERA_FOLLOW_SPEED 8.0f

#define POTION_RADIUS         8.0f
#define POTION_COLLECT_RADIUS 20.0f

/* ------------------------------------------------------------
 *  Types
 * ---------------------------------------------------------- */
typedef enum { GS_WORLD, GS_COMBAT, GS_GAMEOVER } GameState;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float   facingAngle;
    int maxHealth, health;
    int exp;
    int score;
    int potions;
} Player;

typedef struct {
    Vector2 position;
    bool    collected;
} Potion;

/* ------------------------------------------------------------
 *  Global state
 * ---------------------------------------------------------- */
static Potion potions[MAX_POTIONS_WORLD];
static Player player;

static GameState state;
static float     encounterCooldown;
static Camera2D  camera;

static float worldTime = 0.0f;

/* ------------------------------------------------------------
 *  Forward declarations
 * ---------------------------------------------------------- */
static void ResetGame(void);
static void SpawnPotions(void);
static void UpdateWorld(float dt);
static void DrawWorld(void);
static void UpdateGameOver(void);
static void DrawGameOver(void);

static float Clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ------------------------------------------------------------
 *  main
 * ---------------------------------------------------------- */
int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple Turn-Based Fantasy RPG");
    SetTargetFPS(60);

    TerrainInit();
    BackgroundsInit();
    CombatInit();
    ResetGame();

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        worldTime += dt;

        switch (state)
        {
            case GS_WORLD:
                UpdateWorld(dt);
                break;

            case GS_COMBAT:
            {
                CombatResult r = CombatUpdate(dt,
                                              &player.health,
                                              &player.potions,
                                              &player.score,
                                              &player.exp);
                if (r == COMBAT_RESULT_VICTORY || r == COMBAT_RESULT_FLED)
                {
                    CombatClearEffects();
                    state = GS_WORLD;
                    encounterCooldown = ENCOUNTER_CHECK_INTERVAL;
                }
                else if (r == COMBAT_RESULT_DEFEAT)
                {
                    CombatClearEffects();
                    state = GS_GAMEOVER;
                }
                break;
            }

            case GS_GAMEOVER:
                UpdateGameOver();
                break;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        switch (state)
        {
            case GS_WORLD:
                DrawWorld();
                break;
            case GS_COMBAT:
                CombatDraw(player.health, player.maxHealth,
                           player.potions, player.score);
                break;
            case GS_GAMEOVER:
                DrawGameOver();
                break;
        }

        EndDrawing();
    }

    CombatCleanup();
    BackgroundsUnload();
    TerrainUnload();
    CloseWindow();
    return 0;
}

/* ------------------------------------------------------------
 *  Setup / reset
 * ---------------------------------------------------------- */
static void ResetGame(void)
{
    SpawnPotions();

    player.position    = TerrainGetPathMidpoint();
    player.velocity    = (Vector2){ 0.0f, 0.0f };
    player.facingAngle = -PI / 2.0f;
    player.maxHealth   = PLAYER_MAX_HP;
    player.health      = PLAYER_MAX_HP;
    player.exp         = 0;
    player.score       = 0;
    player.potions     = STARTING_POTIONS;

    encounterCooldown = ENCOUNTER_CHECK_INTERVAL;

    camera.target   = player.position;
    camera.offset   = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    state = GS_WORLD;
}

static void SpawnPotions(void)
{
    for (int i = 0; i < MAX_POTIONS_WORLD; i++)
    {
        Vector2 pos = { 0 };
        int attempts = 0;
        do {
            pos.x = (float)GetRandomValue(80, WORLD_WIDTH - 80);
            pos.y = (float)GetRandomValue(80, WORLD_HEIGHT - 80);
            attempts++;
        } while (TerrainIsOnPath(pos) && attempts < 300);

        potions[i].position  = pos;
        potions[i].collected = false;
    }
}

/* ------------------------------------------------------------
 *  World update
 * ---------------------------------------------------------- */
static void UpdateWorld(float dt)
{
    Vector2 inputDir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) inputDir.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) inputDir.y += 1.0f;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputDir.x += 1.0f;

    bool hasInput = (inputDir.x != 0.0f || inputDir.y != 0.0f);

    if (hasInput)
    {
        inputDir = Vector2Normalize(inputDir);
        player.facingAngle = atan2f(inputDir.y, inputDir.x);

        player.velocity = Vector2Add(player.velocity, Vector2Scale(inputDir, PLAYER_ACCEL * dt));
        float speed = Vector2Length(player.velocity);
        if (speed > PLAYER_MAX_SPEED)
            player.velocity = Vector2Scale(Vector2Normalize(player.velocity), PLAYER_MAX_SPEED);
    }
    else
    {
        float speed = Vector2Length(player.velocity);
        if (speed > 0.0001f)
        {
            float newSpeed = speed - PLAYER_DECEL * dt;
            if (newSpeed < 0.0f) newSpeed = 0.0f;
            player.velocity = Vector2Scale(player.velocity, newSpeed / speed);
        }
        else player.velocity = (Vector2){ 0.0f, 0.0f };
    }

    player.position = Vector2Add(player.position, Vector2Scale(player.velocity, dt));
    player.position.x = Clampf(player.position.x, PLAYER_RADIUS, WORLD_WIDTH  - PLAYER_RADIUS);
    player.position.y = Clampf(player.position.y, PLAYER_RADIUS, WORLD_HEIGHT - PLAYER_RADIUS);

    for (int i = 0; i < MAX_POTIONS_WORLD; i++)
    {
        if (!potions[i].collected &&
            Vector2Distance(player.position, potions[i].position) < POTION_COLLECT_RADIUS)
        {
            potions[i].collected = true;
            player.potions++;
        }
    }

    encounterCooldown -= dt;
    if (encounterCooldown <= 0.0f)
    {
        encounterCooldown = ENCOUNTER_CHECK_INTERVAL;
        if (Vector2Length(player.velocity) > 5.0f && !TerrainIsOnPath(player.position))
        {
            if (GetRandomValue(1, 100) <= ENCOUNTER_CHANCE)
            {
                CombatStartEncounter();
                state = GS_COMBAT;
            }
        }
    }

    float followT = 1.0f - expf(-CAMERA_FOLLOW_SPEED * dt);
    camera.target = Vector2Lerp(camera.target, player.position, followT);

    float halfW = SCREEN_WIDTH  / 2.0f;
    float halfH = SCREEN_HEIGHT / 2.0f;
    camera.target.x = Clampf(camera.target.x, halfW, WORLD_WIDTH  - halfW);
    camera.target.y = Clampf(camera.target.y, halfH, WORLD_HEIGHT - halfH);
}

/* ------------------------------------------------------------
 *  World drawing
 * ---------------------------------------------------------- */
static void DrawPotion(Vector2 pos)
{
    DrawCircleV(pos, POTION_RADIUS, RED);
    DrawCircle((int)pos.x, (int)pos.y - 3, 3, WHITE);

    Rectangle stopper = { pos.x - 3.0f, pos.y - POTION_RADIUS - 5.0f, 6.0f, 5.0f };
    DrawRectangleRec(stopper, BROWN);
    DrawRectangleLinesEx(stopper, 1.0f, (Color){ 60, 40, 20, 255 });
}

static void DrawWorld(void)
{
    BeginMode2D(camera);

        TerrainDraw();

        for (int i = 0; i < MAX_POTIONS_WORLD; i++)
            if (!potions[i].collected) DrawPotion(potions[i].position);

        /* Player knight in the exploration view */
        {
            bool facingRight = (cosf(player.facingAngle) >= 0.0f);

            DrawEllipse((int)player.position.x,
                        (int)(player.position.y + 18),
                        14.0f, 5.0f, (Color){ 0, 0, 0, 100 });

            CombatantPose pose = { 0.0f, worldTime };
            CombatantDrawKnight(player.position, facingRight, pose);
        }

    EndMode2D();
}

/* ------------------------------------------------------------
 *  Game over
 * ---------------------------------------------------------- */
static void UpdateGameOver(void)
{
    if (IsKeyPressed(KEY_ENTER)) ResetGame();
}

static void DrawGameOver(void)
{
    ClearBackground((Color){ 20, 10, 10, 255 });

    const char *title = "GAME OVER";
    int titleSize = 48;
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, titleSize)) / 2, 220, titleSize, RED);

    const char *scoreLine = TextFormat("Enemies Defeated: %d", player.score);
    DrawText(scoreLine, (SCREEN_WIDTH - MeasureText(scoreLine, 24)) / 2, 300, 24, WHITE);

    const char *prompt = "Press ENTER to Restart";
    DrawText(prompt, (SCREEN_WIDTH - MeasureText(prompt, 20)) / 2, 360, 20, LIGHTGRAY);
}