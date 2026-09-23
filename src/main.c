/* ============================================================
 * main.c - Simple Turn-Based Fantasy RPG
 *
 * A small 2D RPG built with C and raylib: physics-based
 * exploration over a large, organically-painted forest with a
 * winding dirt path, plus turn-based combat (Attack / Spell /
 * Items / Flee) against a randomly chosen enemy.
 *
 * The world terrain (forest + path) is generated once at
 * startup and baked into an off-screen texture, since no
 * external art assets are available in this build - all
 * "sprites" are procedurally drawn, anti-aliased raylib shapes
 * rather than bitmap images.
 *
 * Drop this file into src/ of the portable raylib project and
 * build with the existing Makefile (make / make run).
 * ============================================================ */

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------
 *  Constants
 * ---------------------------------------------------------- */
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#define WORLD_SCALE    4
#define WORLD_WIDTH    (SCREEN_WIDTH  * WORLD_SCALE)   /* 3200 */
#define WORLD_HEIGHT   (SCREEN_HEIGHT * WORLD_SCALE)   /* 2400 */

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

#define ENCOUNTER_CHANCE           15    /* percent, rolled periodically while walking in forest */
#define ENCOUNTER_CHECK_INTERVAL   0.35f /* seconds between encounter rolls */

#define ANIM_DURATION        0.6f
#define MESSAGE_DURATION     1.6f

#define NUM_ENEMY_TYPES 4

/* Player physics */
#define PLAYER_RADIUS       14.0f
#define PLAYER_MAX_SPEED    220.0f  /* px/sec */
#define PLAYER_ACCEL        900.0f  /* px/sec^2, ramps up to top speed */
#define PLAYER_DECEL        1400.0f /* px/sec^2, brakes to a stop when input releases */

/* Camera */
#define CAMERA_FOLLOW_SPEED 8.0f    /* higher = camera catches up to the player faster */

/* Terrain generation */
#define PATH_CONTROL_POINTS       9
#define PATH_SAMPLES_PER_SEGMENT  40
#define MAX_PATH_SAMPLES          ((PATH_CONTROL_POINTS) * (PATH_SAMPLES_PER_SEGMENT) + 8)
#define PATH_BASE_RADIUS          46.0f  /* average half-width of the walkable dirt path */
#define PATH_HALF_WIDTH           48.0f  /* used for forest-vs-path terrain checks */
#define NUM_TREE_CLUSTERS         55
#define NUM_SPARSE_TREES          260

/* Potions */
#define POTION_RADIUS         8.0f
#define POTION_COLLECT_RADIUS 20.0f

/* ------------------------------------------------------------
 *  Types
 * ---------------------------------------------------------- */
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
    Vector2 position;
    Vector2 velocity;
    float   facingAngle;
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
    Vector2 position;
    bool    collected;
} Potion;

/* ------------------------------------------------------------
 *  Global game state
 * ---------------------------------------------------------- */
static Potion potions[MAX_POTIONS_WORLD];

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

static float encounterCooldown;

static const char *enemyNames[NUM_ENEMY_TYPES]   = { "Slime", "Goblin", "Wolf", "Orc" };
static const int   enemyHealths[NUM_ENEMY_TYPES] = { 15, 22, 28, 36 };

/* Anchor points for the two combatants on the combat screen */
static const Vector2 playerCombatPos = { 180, 380 };
static const Vector2 enemyCombatPos  = { 600, 200 };

/* Terrain */
static Vector2 pathSamples[MAX_PATH_SAMPLES];
static int     pathSampleCount;
static RenderTexture2D worldTexture;
static Camera2D camera;

/* ------------------------------------------------------------
 *  Forward declarations
 * ---------------------------------------------------------- */
static void ResetGame(void);
static void GenerateWorld(void);
static void SpawnPotions(void);
static bool IsOnPath(Vector2 pos);
static int  ClampInt(int v, int lo, int hi);
static float Clampf(float v, float lo, float hi);

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

    GenerateWorld();
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

    UnloadRenderTexture(worldTexture);
    CloseWindow();
    return 0;
}

/* ------------------------------------------------------------
 *  Setup / reset
 * ---------------------------------------------------------- */
static void ResetGame(void)
{
    SpawnPotions();

    /* Spawn on the path itself so the player starts in a safe spot */
    player.position    = pathSamples[pathSampleCount / 2];
    player.velocity     = (Vector2){ 0.0f, 0.0f };
    player.facingAngle  = -PI / 2.0f;
    player.maxHealth = PLAYER_MAX_HP;
    player.health    = PLAYER_MAX_HP;
    player.exp       = 0;
    player.score     = 0;
    player.potions   = STARTING_POTIONS;

    encounterCooldown = ENCOUNTER_CHECK_INTERVAL;

    camera.target   = player.position;
    camera.offset   = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    state = GS_WORLD;
}

/* Builds a winding Catmull-Rom path across the world, then bakes the whole
 * terrain (irregular forest clumps + the path cut through them) into a
 * single off-screen texture so it only has to be drawn once, ever. */
static void GenerateWorld(void)
{
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
            y = Clampf(y, margin, WORLD_HEIGHT - margin);
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
    if (pathSampleCount < MAX_PATH_SAMPLES) pathSamples[pathSampleCount++] = control[PATH_CONTROL_POINTS - 1];

    worldTexture = LoadRenderTexture(WORLD_WIDTH, WORLD_HEIGHT);

    BeginTextureMode(worldTexture);
        ClearBackground((Color){ 40, 70, 35, 255 }); /* base undergrowth */

        /* Irregular forest clumps: overlapping circles of varied size/shade,
         * randomly scattered so density and clump shape are never uniform. */
        for (int c = 0; c < NUM_TREE_CLUSTERS; c++)
        {
            Vector2 center = { (float)GetRandomValue(0, WORLD_WIDTH), (float)GetRandomValue(0, WORLD_HEIGHT) };
            int   treeCount     = GetRandomValue(10, 22);
            float clusterRadius = (float)GetRandomValue(70, 170);

            for (int t = 0; t < treeCount; t++)
            {
                float ang  = (float)GetRandomValue(0, 360) * DEG2RAD;
                float dist = (float)GetRandomValue(0, (int)clusterRadius);
                Vector2 pos = { center.x + cosf(ang) * dist, center.y + sinf(ang) * dist };
                float r = (float)GetRandomValue(14, 34);

                Color col;
                switch (GetRandomValue(0, 3))
                {
                    case 0:  col = (Color){ 24, 90, 32, 235 };  break;
                    case 1:  col = (Color){ 30, 110, 40, 220 }; break;
                    case 2:  col = (Color){ 20, 75, 28, 245 };  break;
                    default: col = (Color){ 40, 125, 48, 210 }; break;
                }
                DrawCircleV(pos, r, col);
            }
        }

        /* Sparse solitary trees so gaps between clusters don't look empty/flat */
        for (int i = 0; i < NUM_SPARSE_TREES; i++)
        {
            Vector2 pos = { (float)GetRandomValue(0, WORLD_WIDTH), (float)GetRandomValue(0, WORLD_HEIGHT) };
            float r = (float)GetRandomValue(10, 22);
            DrawCircleV(pos, r, (Color){ 28, 100, 36, 200 });
        }

        /* Winding dirt path, drawn last so it cuts a clean curved swath
         * through the forest instead of looking grid-aligned. */
        for (int i = 0; i < pathSampleCount; i++)
        {
            float wobble = sinf((float)i * 0.18f) * 7.0f;
            float radius = PATH_BASE_RADIUS + wobble;
            DrawCircleV(pathSamples[i], radius + 14, (Color){ 150, 120, 70, 255 }); /* dirt edge/shadow */
        }
        for (int i = 0; i < pathSampleCount; i++)
        {
            float wobble = sinf((float)i * 0.18f) * 7.0f;
            float radius = PATH_BASE_RADIUS + wobble;
            DrawCircleV(pathSamples[i], radius, (Color){ 198, 170, 112, 255 }); /* path fill */
        }
    EndTextureMode();
}

static bool IsOnPath(Vector2 pos)
{
    for (int i = 0; i < pathSampleCount; i++)
        if (Vector2Distance(pos, pathSamples[i]) < PATH_HALF_WIDTH) return true;
    return false;
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
        } while (IsOnPath(pos) && attempts < 300);

        potions[i].position  = pos;
        potions[i].collected = false;
    }
}

static int ClampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float Clampf(float v, float lo, float hi)
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

        /* Accelerate toward top speed in the input direction */
        player.velocity = Vector2Add(player.velocity, Vector2Scale(inputDir, PLAYER_ACCEL * dt));
        float speed = Vector2Length(player.velocity);
        if (speed > PLAYER_MAX_SPEED)
            player.velocity = Vector2Scale(Vector2Normalize(player.velocity), PLAYER_MAX_SPEED);
    }
    else
    {
        /* No input: decelerate to a stop rather than halting instantly */
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

    /* Collect any potion within reach */
    for (int i = 0; i < MAX_POTIONS_WORLD; i++)
    {
        if (!potions[i].collected && Vector2Distance(player.position, potions[i].position) < POTION_COLLECT_RADIUS)
        {
            potions[i].collected = true;
            player.potions++;
        }
    }

    /* Periodic random encounter roll while actively walking through forest */
    encounterCooldown -= dt;
    if (encounterCooldown <= 0.0f)
    {
        encounterCooldown = ENCOUNTER_CHECK_INTERVAL;
        if (Vector2Length(player.velocity) > 5.0f && !IsOnPath(player.position))
        {
            if (GetRandomValue(1, 100) <= ENCOUNTER_CHANCE) StartEncounter();
        }
    }

    /* Smooth, frame-rate independent camera follow */
    float followT = 1.0f - expf(-CAMERA_FOLLOW_SPEED * dt);
    camera.target = Vector2Lerp(camera.target, player.position, followT);

    float halfW = SCREEN_WIDTH  / 2.0f;
    float halfH = SCREEN_HEIGHT / 2.0f;
    camera.target.x = Clampf(camera.target.x, halfW, WORLD_WIDTH  - halfW);
    camera.target.y = Clampf(camera.target.y, halfH, WORLD_HEIGHT - halfH);
}

static void DrawPotion(Vector2 pos)
{
    DrawCircleV(pos, POTION_RADIUS, RED);
    DrawCircle((int)pos.x, (int)pos.y - 3, 3, WHITE);

    /* Small brown stopper, centered horizontally on top of the potion */
    Rectangle stopper = { pos.x - 3.0f, pos.y - POTION_RADIUS - 5.0f, 6.0f, 5.0f };
    DrawRectangleRec(stopper, BROWN);
    DrawRectangleLinesEx(stopper, 1.0f, (Color){ 60, 40, 20, 255 });
}

static void DrawWorld(void)
{
    BeginMode2D(camera);

        /* Render texture rows are stored bottom-up, so flip on draw */
        Rectangle src = { 0, 0, (float)worldTexture.texture.width, -(float)worldTexture.texture.height };
        DrawTextureRec(worldTexture.texture, src, (Vector2){ 0, 0 }, WHITE);

        for (int i = 0; i < MAX_POTIONS_WORLD; i++)
            if (!potions[i].collected) DrawPotion(potions[i].position);

        /* Player: soft gradient circle with a small facing indicator */
        DrawCircleGradient(player.position, PLAYER_RADIUS,
                            (Color){ 120, 175, 255, 255 }, (Color){ 40, 80, 200, 255 });
        DrawCircleLines((int)player.position.x, (int)player.position.y, PLAYER_RADIUS, (Color){ 20, 40, 120, 255 });

        Vector2 tip = {
            player.position.x + cosf(player.facingAngle) * (PLAYER_RADIUS + 8.0f),
            player.position.y + sinf(player.facingAngle) * (PLAYER_RADIUS + 8.0f)
        };
        DrawLineEx(player.position, tip, 3.0f, (Color){ 20, 40, 120, 255 });

    EndMode2D();
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
        enemy.health = ClampInt(enemy.health - pendingDamage, 0, enemy.maxHealth);

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
        player.health = ClampInt(player.health - pendingDamage, 0, player.maxHealth);

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
                            player.health = ClampInt(player.health + POTION_HEAL_AMOUNT, 0, player.maxHealth);
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
