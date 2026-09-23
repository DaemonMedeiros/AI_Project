/* ============================================================
 * main.c - Simple Turn-Based Fantasy RPG
 *
 * A small top-down 2D RPG built with C and raylib, implementing
 * the accompanying game design document: grid exploration with
 * random forest encounters, potion collection, and turn-based
 * combat (Attack / Spell / Items / Flee) against a randomly
 * chosen enemy with simple attack/spell AI.
 *
 * Drop this file into src/ of the portable raylib project and
 * build with the existing Makefile (make / make run).
 * ============================================================ */

#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------
 *  Constants
 * ---------------------------------------------------------- */
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define TILE_SIZE      40
#define MAP_COLS       (SCREEN_WIDTH  / TILE_SIZE)   /* 20 */
#define MAP_ROWS       (SCREEN_HEIGHT / TILE_SIZE)   /* 15 */

#define MAX_POTIONS_WORLD   6
#define STARTING_POTIONS    3

#define PLAYER_MAX_HP        50
#define PLAYER_ATTACK_DMG    8
#define PLAYER_SPELL_DMG     15
#define ENEMY_ATTACK_DMG     5
#define ENEMY_SPELL_DMG      9
#define POTION_HEAL_AMOUNT   20
#define FLEE_SUCCESS_CHANCE  50   /* percent */
#define EXP_PER_KILL         15

#define ENCOUNTER_CHANCE     15   /* percent, rolled per step onto a forest tile */
#define MOVE_REPEAT_DELAY    0.15f /* seconds between steps while a direction is held */

#define ANIM_DURATION        0.6f
#define MESSAGE_DURATION     1.6f

#define NUM_ENEMY_TYPES 4

/* ------------------------------------------------------------
 *  Types
 * ---------------------------------------------------------- */
typedef enum { TILE_FOREST, TILE_PATH } TileType;

typedef enum { GS_WORLD, GS_COMBAT, GS_GAMEOVER } GameState;

typedef enum { CM_MAIN, CM_SPELL, CM_ITEM } CombatMenu;

typedef enum { ACTOR_PLAYER, ACTOR_ENEMY } Actor;
typedef enum { ANIM_ATTACK, ANIM_SPELL } AnimType;

typedef enum { PHASE_MENU, PHASE_ANIM, PHASE_MESSAGE } CombatPhase;

typedef enum {
    AFTER_NONE,
    AFTER_ENEMY_TURN,
    AFTER_BACK_TO_MENU,
    AFTER_VICTORY,
    AFTER_RETURN_WORLD,
    AFTER_GAMEOVER
} AfterMessage;

typedef struct {
    int gridX, gridY;
    int maxHealth, health;
    int exp;
    int score;
    int potions;
} Player;

typedef struct {
    const char *name;
    int maxHealth, health;
} Enemy;

typedef struct {
    int gridX, gridY;
    bool collected;
} Potion;

/* ------------------------------------------------------------
 *  Global game state
 * ---------------------------------------------------------- */
static TileType map[MAP_ROWS][MAP_COLS];
static Potion   potions[MAX_POTIONS_WORLD];

static Player player;
static Enemy  enemy;

static GameState   state;
static CombatPhase phase;
static CombatMenu  combatMenu;
static int         menuSelection;

static Actor    animActor;
static AnimType animType;
static float    animTimer;
static int      pendingDamage;

static char         message[160];
static float        messageTimer;
static AfterMessage afterMessage;

static float moveTimer;

static const char *enemyNames[NUM_ENEMY_TYPES]   = { "Slime", "Goblin", "Wolf", "Orc" };
static const int   enemyHealths[NUM_ENEMY_TYPES] = { 15, 22, 28, 36 };

/* Anchor points for the two combatants on the combat screen */
static const Vector2 playerCombatPos = { 180, 380 };
static const Vector2 enemyCombatPos  = { 600, 200 };

/* ------------------------------------------------------------
 *  Forward declarations
 * ---------------------------------------------------------- */
static void ResetGame(void);
static void GenerateMap(void);
static void SpawnPotions(void);
static int  Clamp(int v, int lo, int hi);

static void UpdateWorld(float dt);
static void DrawWorld(void);

static void StartEncounter(void);
static void StartEnemyTurn(void);
static void ResolveAnimation(void);
static void SetMessage(const char *text, AfterMessage after);
static void AdvanceMessage(void);
static void UpdateCombat(float dt);
static void DrawCombat(void);

static void UpdateGameOver(void);
static void DrawGameOver(void);

/* ------------------------------------------------------------
 *  main
 * ---------------------------------------------------------- */
int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple Turn-Based Fantasy RPG");
    SetTargetFPS(60);

    ResetGame();

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        switch (state)
        {
            case GS_WORLD:    UpdateWorld(dt);   break;
            case GS_COMBAT:   UpdateCombat(dt);  break;
            case GS_GAMEOVER: UpdateGameOver();  break;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        switch (state)
        {
            case GS_WORLD:    DrawWorld();    break;
            case GS_COMBAT:   DrawCombat();   break;
            case GS_GAMEOVER: DrawGameOver(); break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

/* ------------------------------------------------------------
 *  Setup / reset
 * ---------------------------------------------------------- */
static void ResetGame(void)
{
    GenerateMap();
    SpawnPotions();

    player.gridX     = MAP_COLS / 2;
    player.gridY     = MAP_ROWS / 2;
    player.maxHealth = PLAYER_MAX_HP;
    player.health    = PLAYER_MAX_HP;
    player.exp       = 0;
    player.score     = 0;
    player.potions   = STARTING_POTIONS;

    moveTimer = 0.0f;
    state = GS_WORLD;
}

/* Border + a crossroad of path tiles; everything else is forest. */
static void GenerateMap(void)
{
    for (int y = 0; y < MAP_ROWS; y++)
    {
        for (int x = 0; x < MAP_COLS; x++)
        {
            bool isBorder    = (x == 0 || y == 0 || x == MAP_COLS - 1 || y == MAP_ROWS - 1);
            bool isCrossroad = (x == MAP_COLS / 2 || y == MAP_ROWS / 2);
            map[y][x] = (isBorder || isCrossroad) ? TILE_PATH : TILE_FOREST;
        }
    }
}

static void SpawnPotions(void)
{
    int startX = MAP_COLS / 2;
    int startY = MAP_ROWS / 2;

    for (int i = 0; i < MAX_POTIONS_WORLD; i++)
    {
        int x = startX, y = startY, attempts = 0;
        do {
            x = GetRandomValue(1, MAP_COLS - 2);
            y = GetRandomValue(1, MAP_ROWS - 2);
            attempts++;
        } while ((map[y][x] != TILE_FOREST || (x == startX && y == startY)) && attempts < 200);

        potions[i].gridX = x;
        potions[i].gridY = y;
        potions[i].collected = false;
    }
}

static int Clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ------------------------------------------------------------
 *  World / exploration
 * ---------------------------------------------------------- */
static void UpdateWorld(float dt)
{
    moveTimer -= dt;

    int dx = 0, dy = 0;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dy = -1;
    else if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dy = 1;
    else if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx = -1;
    else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx = 1;

    if ((dx != 0 || dy != 0) && moveTimer <= 0.0f)
    {
        int newX = Clamp(player.gridX + dx, 0, MAP_COLS - 1);
        int newY = Clamp(player.gridY + dy, 0, MAP_ROWS - 1);

        if (newX != player.gridX || newY != player.gridY)
        {
            player.gridX = newX;
            player.gridY = newY;
            moveTimer = MOVE_REPEAT_DELAY;

            /* Collect a potion if standing on one */
            for (int i = 0; i < MAX_POTIONS_WORLD; i++)
            {
                if (!potions[i].collected && potions[i].gridX == newX && potions[i].gridY == newY)
                {
                    potions[i].collected = true;
                    player.potions++;
                }
            }

            /* Random encounter, forest tiles only */
            if (map[newY][newX] == TILE_FOREST)
            {
                if (GetRandomValue(1, 100) <= ENCOUNTER_CHANCE) StartEncounter();
            }
        }
    }
}

static void DrawWorld(void)
{
    for (int y = 0; y < MAP_ROWS; y++)
    {
        for (int x = 0; x < MAP_COLS; x++)
        {
            Color c = (map[y][x] == TILE_FOREST) ? (Color){ 34, 120, 40, 255 } : (Color){ 194, 165, 110, 255 };
            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
            DrawRectangleLines(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, (Color){ 0, 0, 0, 25 });
        }
    }

    for (int i = 0; i < MAX_POTIONS_WORLD; i++)
    {
        if (!potions[i].collected)
        {
            int cx = potions[i].gridX * TILE_SIZE + TILE_SIZE / 2;
            int cy = potions[i].gridY * TILE_SIZE + TILE_SIZE / 2;
            DrawCircle(cx, cy, 8, RED);
            DrawCircle(cx, cy - 3, 3, WHITE);
        }
    }

    int px = player.gridX * TILE_SIZE + TILE_SIZE / 2;
    int py = player.gridY * TILE_SIZE + TILE_SIZE / 2;
    DrawRectangle(px - 12, py - 12, 24, 24, (Color){ 60, 120, 230, 255 });
    DrawRectangleLines(px - 12, py - 12, 24, 24, BLACK);
}

/* ------------------------------------------------------------
 *  Combat
 * ---------------------------------------------------------- */
static void StartEncounter(void)
{
    int t = GetRandomValue(0, NUM_ENEMY_TYPES - 1);
    enemy.name      = enemyNames[t];
    enemy.maxHealth = enemyHealths[t];
    enemy.health    = enemyHealths[t];

    state = GS_COMBAT;
    phase = PHASE_MENU;
    combatMenu = CM_MAIN;
    menuSelection = 0;
}

static void SetMessage(const char *text, AfterMessage after)
{
    strncpy(message, text, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    messageTimer = MESSAGE_DURATION;
    afterMessage = after;
    phase = PHASE_MESSAGE;
}

static void StartEnemyTurn(void)
{
    bool useSpell = (GetRandomValue(0, 1) == 1);
    animType      = useSpell ? ANIM_SPELL : ANIM_ATTACK;
    animActor     = ACTOR_ENEMY;
    pendingDamage = useSpell ? ENEMY_SPELL_DMG : ENEMY_ATTACK_DMG;
    animTimer     = 0.0f;
    phase = PHASE_ANIM;
}

static void ResolveAnimation(void)
{
    char buf[160];

    if (animActor == ACTOR_PLAYER)
    {
        enemy.health = Clamp(enemy.health - pendingDamage, 0, enemy.maxHealth);

        if (enemy.health <= 0)
        {
            player.score++;
            player.exp += EXP_PER_KILL;
            snprintf(buf, sizeof(buf), "You defeated the %s! +%d EXP", enemy.name, EXP_PER_KILL);
            SetMessage(buf, AFTER_VICTORY);
        }
        else
        {
            const char *verb = (animType == ANIM_SPELL) ? "Fireball hits" : "You attack";
            snprintf(buf, sizeof(buf), "%s the %s for %d damage!", verb, enemy.name, pendingDamage);
            SetMessage(buf, AFTER_ENEMY_TURN);
        }
    }
    else /* ACTOR_ENEMY */
    {
        player.health = Clamp(player.health - pendingDamage, 0, player.maxHealth);

        if (player.health <= 0)
        {
            snprintf(buf, sizeof(buf), "The %s defeated you...", enemy.name);
            SetMessage(buf, AFTER_GAMEOVER);
        }
        else
        {
            const char *verb = (animType == ANIM_SPELL) ? "casts a spell on you" : "attacks you";
            snprintf(buf, sizeof(buf), "The %s %s for %d damage!", enemy.name, verb, pendingDamage);
            SetMessage(buf, AFTER_BACK_TO_MENU);
        }
    }
}

static void AdvanceMessage(void)
{
    switch (afterMessage)
    {
        case AFTER_ENEMY_TURN:   StartEnemyTurn(); break;
        case AFTER_BACK_TO_MENU: phase = PHASE_MENU; combatMenu = CM_MAIN; menuSelection = 0; break;
        case AFTER_VICTORY:      state = GS_WORLD; break;
        case AFTER_RETURN_WORLD: state = GS_WORLD; break;
        case AFTER_GAMEOVER:     state = GS_GAMEOVER; break;
        default: break;
    }
}

static void UpdateCombat(float dt)
{
    switch (phase)
    {
        case PHASE_MENU:
        {
            if (combatMenu == CM_MAIN)
            {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 4;
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menuSelection = (menuSelection + 3) % 4;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0) /* Attack */
                    {
                        animType = ANIM_ATTACK; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_ATTACK_DMG; animTimer = 0.0f;
                        phase = PHASE_ANIM;
                    }
                    else if (menuSelection == 1) /* Spell */
                    {
                        combatMenu = CM_SPELL; menuSelection = 0;
                    }
                    else if (menuSelection == 2) /* Items */
                    {
                        combatMenu = CM_ITEM; menuSelection = 0;
                    }
                    else /* Flee */
                    {
                        if (GetRandomValue(1, 100) <= FLEE_SUCCESS_CHANCE)
                            SetMessage("You fled from battle!", AFTER_RETURN_WORLD);
                        else
                            SetMessage("You failed to flee!", AFTER_ENEMY_TURN);
                    }
                }
            }
            else if (combatMenu == CM_SPELL)
            {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                    IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
                    menuSelection = (menuSelection + 1) % 2;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0) /* Fireball */
                    {
                        animType = ANIM_SPELL; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_SPELL_DMG; animTimer = 0.0f;
                        phase = PHASE_ANIM;
                    }
                    else /* Back */
                    {
                        combatMenu = CM_MAIN; menuSelection = 0;
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
                {
                    combatMenu = CM_MAIN; menuSelection = 0;
                }
            }
            else /* CM_ITEM */
            {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                    IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
                    menuSelection = (menuSelection + 1) % 2;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0) /* Health Potion */
                    {
                        if (player.potions > 0)
                        {
                            player.potions--;
                            player.health = Clamp(player.health + POTION_HEAL_AMOUNT, 0, player.maxHealth);
                            SetMessage("Used a Health Potion! Restored 20 HP.", AFTER_ENEMY_TURN);
                        }
                        else
                        {
                            SetMessage("You don't have any potions!", AFTER_BACK_TO_MENU);
                        }
                    }
                    else /* Back */
                    {
                        combatMenu = CM_MAIN; menuSelection = 0;
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
                {
                    combatMenu = CM_MAIN; menuSelection = 0;
                }
            }
            break;
        }

        case PHASE_ANIM:
        {
            animTimer += dt;
            if (animTimer >= ANIM_DURATION) ResolveAnimation();
            break;
        }

        case PHASE_MESSAGE:
        {
            messageTimer -= dt;
            if (messageTimer <= 0.0f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                AdvanceMessage();
            break;
        }
    }
}

static Vector2 Lerp2(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

static void DrawCombatAnimation(void)
{
    float t = animTimer / ANIM_DURATION;
    if (t > 1.0f) t = 1.0f;

    Vector2 origin = (animActor == ACTOR_PLAYER) ? playerCombatPos : enemyCombatPos;
    Vector2 target = (animActor == ACTOR_PLAYER) ? enemyCombatPos  : playerCombatPos;

    if (animType == ANIM_ATTACK)
    {
        /* Weapon lunges partway toward the target and back (triangle wave) */
        float wave = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
        Vector2 mid = Lerp2(origin, target, 0.35f);
        Vector2 pos = Lerp2(origin, mid, wave);
        DrawLineEx(origin, pos, 4, DARKGRAY);
        DrawCircleV(pos, 8, GRAY);
    }
    else /* ANIM_SPELL: Fireball / enemy bolt travels straight to the target */
    {
        Vector2 pos = Lerp2(origin, target, t);
        DrawCircleV(pos, 10, ORANGE);
        DrawCircleV(pos, 5, YELLOW);
    }
}

static void DrawHealthBar(int x, int y, int width, int health, int maxHealth, const char *label)
{
    float pct = (maxHealth > 0) ? ((float)health / (float)maxHealth) : 0.0f;
    if (pct < 0.0f) pct = 0.0f;

    DrawRectangle(x, y, width, 18, (Color){ 60, 60, 60, 255 });
    DrawRectangle(x, y, (int)(width * pct), 18, (Color){ 60, 200, 90, 255 });
    DrawRectangleLines(x, y, width, 18, BLACK);

    DrawText(TextFormat("%s HP: %d/%d", label, health, maxHealth), x, y - 20, 18, WHITE);
}

static void DrawCombat(void)
{
    ClearBackground((Color){ 25, 25, 40, 255 });

    DrawText(TextFormat("Score: %d", player.score), 20, 20, 20, YELLOW);

    /* Combatant markers */
    DrawRectangle((int)playerCombatPos.x - 20, (int)playerCombatPos.y - 20, 40, 40, (Color){ 60, 120, 230, 255 });
    DrawRectangleLines((int)playerCombatPos.x - 20, (int)playerCombatPos.y - 20, 40, 40, BLACK);

    DrawRectangle((int)enemyCombatPos.x - 20, (int)enemyCombatPos.y - 20, 40, 40, (Color){ 200, 60, 60, 255 });
    DrawRectangleLines((int)enemyCombatPos.x - 20, (int)enemyCombatPos.y - 20, 40, 40, BLACK);
    DrawText(enemy.name, (int)enemyCombatPos.x - 30, (int)enemyCombatPos.y + 30, 18, WHITE);

    DrawHealthBar(80, 460, 220, player.health, player.maxHealth, "Player");
    DrawHealthBar(500, 100, 220, enemy.health, enemy.maxHealth, "Enemy");

    if (phase == PHASE_ANIM) DrawCombatAnimation();

    if (phase == PHASE_MENU)
    {
        int mx = 60, my = 500, lineH = 26;
        const char *mainOptions[4]  = { "Attack", "Spell", "Items", "Flee" };
        const char *spellOptions[2] = { "Fireball", "Back" };
        const char *itemOptions[2]  = { TextFormat("Health Potion (x%d)", player.potions), "Back" };

        const char **options = mainOptions;
        int count = 4;
        if (combatMenu == CM_SPELL) { options = spellOptions; count = 2; }
        if (combatMenu == CM_ITEM)  { options = itemOptions;  count = 2; }

        DrawRectangle(mx - 10, my - 10, 300, lineH * count + 20, (Color){ 0, 0, 0, 160 });
        for (int i = 0; i < count; i++)
        {
            Color c = (i == menuSelection) ? YELLOW : WHITE;
            DrawText(TextFormat("%s%s", (i == menuSelection) ? "> " : "  ", options[i]), mx, my + i * lineH, 20, c);
        }
    }

    if (phase == PHASE_MESSAGE)
    {
        int boxW = 600, boxH = 60;
        int bx = (SCREEN_WIDTH - boxW) / 2, by = SCREEN_HEIGHT - 100;
        DrawRectangle(bx, by, boxW, boxH, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines(bx, by, boxW, boxH, WHITE);
        DrawText(message, bx + 20, by + 20, 20, WHITE);
    }
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
