/* ============================================================
 * combat.c - Turn-based combat system with spell VFX
 * ============================================================ */
#include "../include/combat.h"
#include "../include/backgrounds.h"
#include "../include/combatants.h"
#include "raymath.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ---------- Layout ----------
 * Positions are FEET anchors: the sprite's soles sit at this y.
 * All six battle backgrounds have their ground around y ≈ 440-460,
 * so feet sit a bit above that for visual balance. */
static const Vector2 PLAYER_COMBAT_POS = { 220, 470 };
static const Vector2 ENEMY_COMBAT_POS  = { 600, 470 };

/* Combat sprites render at 2x their base texture size. */
#define COMBAT_SPRITE_SCALE  2.0f

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define HEALTH_BAR_W   300
#define HEALTH_BAR_H   18
#define HEALTH_BAR_Y   (SCREEN_HEIGHT - 40)
#define HEALTH_BAR_GAP 40

#define MAX_PARTICLES   512
#define MAX_PROJECTILES 8

/* ---------- Wolf (only enemy) ---------- */
#define WOLF_NAME       "Wolf"
#define WOLF_MAX_HEALTH 28

/* ---------- Internal state ---------- */
static Enemy        enemy;
static CombatPhase  phase;
static CombatMenu   combatMenu;
static int          menuSelection;

static Actor    animActor;
static AnimType animType;
static float    animTimer;
static int      pendingDamage;

static char         message[160];
static float        messageTimer;
static AfterMessage afterMessage;

static CombatResult currentResult;
static BackgroundType currentBackground = BG_FOREST_DAY;
static float combatTime = 0.0f;

/* ---------- Particle system ---------- */
typedef enum {
    PT_FIRE_TRAIL, PT_FIRE_BURST, PT_FIRE_EMBER,
    PT_ICE_TRAIL, PT_ICE_SHARD, PT_ICE_MIST, PT_ICE_BURST
} ParticleType;

typedef struct {
    bool    active;
    ParticleType type;
    Vector2 pos;
    Vector2 vel;
    float   life;
    float   maxLife;
    float   size;
    float   rot;
    float   rotSpeed;
    Color   colStart;
    Color   colEnd;
} Particle;

static Particle particles[MAX_PARTICLES];

typedef struct {
    bool    active;
    bool    isIce;
    Vector2 origin;
    Vector2 target;
    float   progress;
    float   duration;
    bool    exploded;
    float   explodeTimer;
    float   jitterSeed;
} SpellProjectile;

static SpellProjectile projectiles[MAX_PROJECTILES];

/* ---------- Math helpers ---------- */
static int ClampInt(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float RandRange(float low, float high)
{
    return low + (float)GetRandomValue(0, 10000) / 10000.0f * (high - low);
}

static float RandSym(float range) { return RandRange(-range, range); }

static Vector2 LerpVector(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Color FadeColor(Color color, float alphaScale)
{
    float alpha = color.a * alphaScale;
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    color.a = (unsigned char)alpha;
    return color;
}

/* ---------- Particle helpers ---------- */
static int AllocParticle(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (!particles[i].active) return i;
    return -1;
}

static void SpawnParticle(ParticleType type, Vector2 pos, Vector2 vel,
                          float life, float size, Color colStart, Color colEnd)
{
    int index = AllocParticle();
    if (index < 0) return;
    Particle *particle = &particles[index];
    particle->active   = true;
    particle->type     = type;
    particle->pos      = pos;
    particle->vel      = vel;
    particle->life     = life;
    particle->maxLife  = life;
    particle->size     = size;
    particle->rot      = RandRange(0.0f, PI * 2.0f);
    particle->rotSpeed = RandSym(6.0f);
    particle->colStart = colStart;
    particle->colEnd   = colEnd;
}

/* ---------- Lifecycle ---------- */
void CombatInit(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    for (int i = 0; i < MAX_PROJECTILES; i++) projectiles[i].active = false;
    currentResult = COMBAT_RESULT_NONE;
    phase = PHASE_MENU;
    combatMenu = CM_MAIN;
    menuSelection = 0;
    message[0] = '\0';
    combatTime = 0.0f;
}

void CombatCleanup(void) { CombatClearEffects(); }

void CombatClearEffects(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    for (int i = 0; i < MAX_PROJECTILES; i++) projectiles[i].active = false;
}

const char *CombatGetEnemyName(void) { return enemy.name ? enemy.name : "?"; }
BackgroundType CombatGetBackground(void) { return currentBackground; }

/* ---------- Encounter setup ---------- */
void CombatStartEncounter(void)
{
    enemy.name      = WOLF_NAME;
    enemy.maxHealth = WOLF_MAX_HEALTH;
    enemy.health    = WOLF_MAX_HEALTH;

    currentBackground = BackgroundsPickRandom();
    phase = PHASE_MENU;
    combatMenu = CM_MAIN;
    menuSelection = 0;
    currentResult = COMBAT_RESULT_NONE;
    CombatClearEffects();
    combatTime = 0.0f;
}

static void SetMessage(const char *text, AfterMessage after)
{
    strncpy(message, text, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    messageTimer = MESSAGE_DURATION;
    afterMessage = after;
    phase = PHASE_MESSAGE;
}

/* ---------- Fireball VFX ---------- */
static void EmitFireTrail(Vector2 pos, float intensity)
{
    int count = GetRandomValue(2, 4);
    for (int i = 0; i < count; i++)
    {
        Vector2 vel = { RandSym(60.0f) - 40.0f, RandSym(60.0f) };
        Color start = { 255, (unsigned char)GetRandomValue(180, 240), 60, 255 };
        Color end   = { 200, 40, 10, 0 };
        SpawnParticle(PT_FIRE_TRAIL, pos, vel,
                      RandRange(0.25f, 0.55f) * intensity,
                      RandRange(6.0f, 12.0f) * intensity,
                      start, end);
    }
    if (GetRandomValue(0, 3) == 0)
    {
        Vector2 vel = { RandSym(30.0f), RandSym(30.0f) - 20.0f };
        SpawnParticle(PT_FIRE_TRAIL, pos, vel,
                      RandRange(0.5f, 0.9f), RandRange(10.0f, 16.0f),
                      (Color){ 120, 100, 90, 140 }, (Color){ 60, 50, 50, 0 });
    }
}

static void EmitFireExplosion(Vector2 pos)
{
    for (int i = 0; i < 12; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(60.0f, 220.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        SpawnParticle(PT_FIRE_BURST, pos, vel,
                      RandRange(0.35f, 0.6f), RandRange(14.0f, 26.0f),
                      (Color){ 255, 250, 200, 255 }, (Color){ 255, 80, 20, 0 });
    }
    for (int i = 0; i < 26; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(120.0f, 420.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        SpawnParticle(PT_FIRE_BURST, pos, vel,
                      RandRange(0.5f, 0.9f), RandRange(10.0f, 20.0f),
                      (Color){ 255, 170, 60, 255 }, (Color){ 180, 30, 10, 0 });
    }
    for (int i = 0; i < 30; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(40.0f, 200.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed - RandRange(0.0f, 60.0f) };
        SpawnParticle(PT_FIRE_EMBER, pos, vel,
                      RandRange(0.7f, 1.4f), RandRange(2.0f, 5.0f),
                      (Color){ 255, 230, 120, 255 }, (Color){ 200, 60, 0, 0 });
    }
    for (int i = 0; i < 20; i++)
    {
        float angle = (float)i / 20.0f * PI * 2.0f;
        Vector2 vel = { cosf(angle) * 260.0f, sinf(angle) * 260.0f };
        SpawnParticle(PT_FIRE_BURST, pos, vel, 0.45f, 8.0f,
                      (Color){ 255, 240, 180, 255 }, (Color){ 255, 100, 30, 0 });
    }
}

/* ---------- Ice spell VFX ---------- */
static void EmitIceTrail(Vector2 pos, float intensity)
{
    int count = GetRandomValue(2, 4);
    for (int i = 0; i < count; i++)
    {
        Vector2 vel = { RandSym(40.0f) - 20.0f, RandSym(50.0f) };
        SpawnParticle(PT_ICE_TRAIL, pos, vel,
                      RandRange(0.3f, 0.6f) * intensity,
                      RandRange(4.0f, 9.0f) * intensity,
                      (Color){ 200, 235, 255, 255 }, (Color){ 90, 150, 220, 0 });
    }
    if (GetRandomValue(0, 2) == 0)
    {
        Vector2 vel = { RandSym(70.0f), RandSym(70.0f) };
        SpawnParticle(PT_ICE_SHARD, pos, vel,
                      RandRange(0.4f, 0.8f), RandRange(6.0f, 11.0f),
                      (Color){ 230, 250, 255, 255 }, (Color){ 120, 180, 240, 0 });
    }
}

static void EmitIceExplosion(Vector2 pos)
{
    for (int i = 0; i < 10; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(50.0f, 180.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        SpawnParticle(PT_ICE_BURST, pos, vel,
                      RandRange(0.3f, 0.55f), RandRange(16.0f, 28.0f),
                      (Color){ 255, 255, 255, 255 }, (Color){ 140, 200, 255, 0 });
    }
    for (int i = 0; i < 32; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(180.0f, 480.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        SpawnParticle(PT_ICE_SHARD, pos, vel,
                      RandRange(0.6f, 1.1f), RandRange(7.0f, 15.0f),
                      (Color){ 240, 250, 255, 255 }, (Color){ 90, 150, 220, 0 });
    }
    for (int i = 0; i < 24; i++)
    {
        float angle = RandRange(0, PI * 2.0f);
        float speed = RandRange(20.0f, 90.0f);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed - 15.0f };
        SpawnParticle(PT_ICE_MIST, pos, vel,
                      RandRange(0.9f, 1.6f), RandRange(18.0f, 34.0f),
                      (Color){ 200, 230, 255, 200 }, (Color){ 150, 200, 240, 0 });
    }
    for (int i = 0; i < 22; i++)
    {
        float angle = (float)i / 22.0f * PI * 2.0f;
        Vector2 vel = { cosf(angle) * 240.0f, sinf(angle) * 240.0f };
        SpawnParticle(PT_ICE_BURST, pos, vel, 0.5f, 8.0f,
                      (Color){ 220, 245, 255, 255 }, (Color){ 120, 180, 240, 0 });
    }
}

/* ---------- Projectile spawning ---------- */
static int AllocProjectile(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (!projectiles[i].active) return i;
    return -1;
}

static void SpawnProjectile(Vector2 origin, Vector2 target, bool isIce)
{
    int index = AllocProjectile();
    if (index < 0) return;
    SpellProjectile *projectile = &projectiles[index];
    projectile->active       = true;
    projectile->isIce        = isIce;
    projectile->origin       = origin;
    projectile->target       = target;
    projectile->progress     = 0.0f;
    projectile->duration     = ANIM_DURATION;
    projectile->exploded     = false;
    projectile->explodeTimer = 0.0f;
    projectile->jitterSeed   = RandRange(0.0f, 1000.0f);
}

void CombatSpawnFireball(Vector2 origin, Vector2 target) { SpawnProjectile(origin, target, false); }
void CombatSpawnIceSpell(Vector2 origin, Vector2 target) { SpawnProjectile(origin, target, true);  }

/* ---------- Effect update ---------- */
void CombatUpdateEffects(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *particle = &particles[i];
        if (!particle->active) continue;

        particle->life -= dt;
        if (particle->life <= 0.0f) { particle->active = false; continue; }

        switch (particle->type)
        {
            case PT_FIRE_TRAIL:
            case PT_FIRE_BURST:
                particle->vel.x *= (1.0f - 2.5f * dt);
                particle->vel.y *= (1.0f - 2.5f * dt);
                particle->vel.y -= 30.0f * dt;
                break;
            case PT_FIRE_EMBER:
                particle->vel.x *= (1.0f - 1.2f * dt);
                particle->vel.y -= 40.0f * dt;
                break;
            case PT_ICE_TRAIL:
            case PT_ICE_BURST:
                particle->vel.x *= (1.0f - 3.0f * dt);
                particle->vel.y *= (1.0f - 3.0f * dt);
                break;
            case PT_ICE_SHARD:
                particle->vel.x *= (1.0f - 1.5f * dt);
                particle->vel.y *= (1.0f - 1.5f * dt);
                particle->vel.y += 120.0f * dt;
                break;
            case PT_ICE_MIST:
                particle->vel.x *= (1.0f - 1.8f * dt);
                particle->vel.y *= (1.0f - 1.8f * dt);
                particle->vel.y -= 8.0f * dt;
                break;
        }

        particle->pos.x += particle->vel.x * dt;
        particle->pos.y += particle->vel.y * dt;
        particle->rot   += particle->rotSpeed * dt;
    }

    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        SpellProjectile *projectile = &projectiles[i];
        if (!projectile->active) continue;

        if (!projectile->exploded)
        {
            projectile->progress += dt / projectile->duration;
            Vector2 pos = LerpVector(projectile->origin, projectile->target, projectile->progress);
            if (projectile->isIce) EmitIceTrail(pos, 1.0f);
            else                    EmitFireTrail(pos, 1.0f);

            if (projectile->progress >= 1.0f)
            {
                projectile->progress = 1.0f;
                projectile->exploded = true;
                projectile->explodeTimer = 0.35f;
                if (projectile->isIce) EmitIceExplosion(projectile->target);
                else                    EmitFireExplosion(projectile->target);
            }
        }
        else
        {
            projectile->explodeTimer -= dt;
            if (projectile->explodeTimer <= 0.0f) projectile->active = false;
        }
    }
}

/* ---------- Effect drawing ---------- */
static void DrawParticle(const Particle *particle)
{
    float t = 1.0f - (particle->life / particle->maxLife);
    Color color = LerpColor(particle->colStart, particle->colEnd, t);
    float size = particle->size * (1.0f - t * 0.7f);

    switch (particle->type)
    {
        case PT_FIRE_TRAIL:
        case PT_FIRE_BURST:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(particle->pos, size, FadeColor(color, 1.0f - t));
            DrawCircleV(particle->pos, size * 0.5f, FadeColor((Color){255,255,220,color.a}, 1.0f - t));
            EndBlendMode();
            break;

        case PT_FIRE_EMBER:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(particle->pos, size, FadeColor(color, 1.0f - t));
            EndBlendMode();
            break;

        case PT_ICE_TRAIL:
        case PT_ICE_MIST:
            DrawCircleV(particle->pos, size, FadeColor(color, (1.0f - t) * 0.85f));
            break;

        case PT_ICE_SHARD:
        {
            float s = size;
            Vector2 points[4] = {
                { particle->pos.x,            particle->pos.y - s        },
                { particle->pos.x + s * 0.6f, particle->pos.y            },
                { particle->pos.x,            particle->pos.y + s        },
                { particle->pos.x - s * 0.6f, particle->pos.y            }
            };
            float cosRot = cosf(particle->rot);
            float sinRot = sinf(particle->rot);
            for (int k = 0; k < 4; k++)
            {
                float dx = points[k].x - particle->pos.x;
                float dy = points[k].y - particle->pos.y;
                points[k].x = particle->pos.x + dx * cosRot - dy * sinRot;
                points[k].y = particle->pos.y + dx * sinRot + dy * cosRot;
            }
            Color cc = FadeColor(color, 1.0f - t * 0.8f);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawTriangle(points[0], points[3], points[1], cc);
            DrawTriangle(points[1], points[3], points[2], cc);
            EndBlendMode();
            break;
        }

        case PT_ICE_BURST:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(particle->pos, size, FadeColor(color, 1.0f - t));
            EndBlendMode();
            break;
    }
}

void CombatDrawEffects(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (particles[i].active) DrawParticle(&particles[i]);

    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        SpellProjectile *projectile = &projectiles[i];
        if (!projectile->active || projectile->exploded) continue;

        Vector2 pos = LerpVector(projectile->origin, projectile->target, projectile->progress);

        if (projectile->isIce)
        {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(pos, 18, (Color){ 120, 180, 255, 90 });
            DrawCircleV(pos, 12, (Color){ 180, 230, 255, 180 });
            DrawCircleV(pos,  7, (Color){ 255, 255, 255, 255 });
            EndBlendMode();
            for (int k = 0; k < 4; k++)
            {
                float angle = projectile->jitterSeed + (float)k * PI * 0.5f + projectile->progress * 8.0f;
                Vector2 shardPos = { pos.x + cosf(angle) * 12.0f, pos.y + sinf(angle) * 12.0f };
                DrawPoly(shardPos, 3, 5.0f, (angle * RAD2DEG) + 90.0f, (Color){ 220, 245, 255, 220 });
            }
        }
        else
        {
            float flicker = 1.0f + sinf((projectile->progress * 40.0f) + projectile->jitterSeed) * 0.08f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(pos, 22 * flicker, (Color){ 255, 90, 30, 70 });
            DrawCircleV(pos, 14 * flicker, (Color){ 255, 160, 60, 180 });
            DrawCircleV(pos,  8 * flicker, (Color){ 255, 240, 180, 255 });
            EndBlendMode();
        }
    }
}

/* ---------- Turn resolution ---------- */
static void StartEnemyTurn(void)
{
    bool useSpell = (GetRandomValue(0, 1) == 1);
    animType      = useSpell ? ANIM_SPELL_FIRE : ANIM_ATTACK;
    animActor     = ACTOR_ENEMY;
    pendingDamage = useSpell ? ENEMY_SPELL_DMG : ENEMY_ATTACK_DMG;
    animTimer     = 0.0f;
    phase = PHASE_ANIM;
}

static void ResolveAnimation(int *playerHealth, int *playerPotions,
                             int *playerScore, int *playerExp)
{
    char buf[160];

    if (animActor == ACTOR_PLAYER)
    {
        enemy.health = ClampInt(enemy.health - pendingDamage, 0, enemy.maxHealth);

        if (enemy.health <= 0)
        {
            (*playerScore)++;
            (*playerExp) += EXP_PER_KILL;
            snprintf(buf, sizeof(buf), "You defeated the %s! +%d EXP", enemy.name, EXP_PER_KILL);
            SetMessage(buf, AFTER_VICTORY);
        }
        else
        {
            const char *verb = "You attack";
            if (animType == ANIM_SPELL_FIRE) verb = "Fireball hits";
            else if (animType == ANIM_SPELL_ICE) verb = "Ice shards hit";
            snprintf(buf, sizeof(buf), "%s the %s for %d damage!", verb, enemy.name, pendingDamage);
            SetMessage(buf, AFTER_ENEMY_TURN);
        }
    }
    else
    {
        *playerHealth = ClampInt(*playerHealth - pendingDamage, 0, PLAYER_MAX_HP);

        if (*playerHealth <= 0)
        {
            snprintf(buf, sizeof(buf), "The %s defeated you...", enemy.name);
            SetMessage(buf, AFTER_GAMEOVER);
        }
        else
        {
            const char *verb = (animType == ANIM_SPELL_FIRE) ? "casts a spell on you" : "attacks you";
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
        case AFTER_VICTORY:      currentResult = COMBAT_RESULT_VICTORY; break;
        case AFTER_RETURN_WORLD: currentResult = COMBAT_RESULT_FLED; break;
        case AFTER_GAMEOVER:     currentResult = COMBAT_RESULT_DEFEAT; break;
        default: break;
    }
}

/* ---------- Main combat update ---------- */
CombatResult CombatUpdate(float dt,
                          int *playerHealth, int *playerPotions,
                          int *playerScore, int *playerExp)
{
    combatTime += dt;
    CombatUpdateEffects(dt);

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
                    if (menuSelection == 0)
                    {
                        animType = ANIM_ATTACK; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_ATTACK_DMG; animTimer = 0.0f;
                        phase = PHASE_ANIM;
                    }
                    else if (menuSelection == 1) { combatMenu = CM_SPELL; menuSelection = 0; }
                    else if (menuSelection == 2) { combatMenu = CM_ITEM;  menuSelection = 0; }
                    else
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
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 3;
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menuSelection = (menuSelection + 2) % 3;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0)
                    {
                        animType = ANIM_SPELL_FIRE; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_SPELL_DMG; animTimer = 0.0f;
                        CombatSpawnFireball(PLAYER_COMBAT_POS, ENEMY_COMBAT_POS);
                        phase = PHASE_ANIM;
                    }
                    else if (menuSelection == 1)
                    {
                        animType = ANIM_SPELL_ICE; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_SPELL_DMG; animTimer = 0.0f;
                        CombatSpawnIceSpell(PLAYER_COMBAT_POS, ENEMY_COMBAT_POS);
                        phase = PHASE_ANIM;
                    }
                    else { combatMenu = CM_MAIN; menuSelection = 0; }
                }
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
                {
                    combatMenu = CM_MAIN; menuSelection = 0;
                }
            }
            else
            {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
                    IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
                    menuSelection = (menuSelection + 1) % 2;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0)
                    {
                        if (*playerPotions > 0)
                        {
                            (*playerPotions)--;
                            *playerHealth = ClampInt(*playerHealth + POTION_HEAL_AMOUNT, 0, PLAYER_MAX_HP);
                            SetMessage("Used a Health Potion! Restored 20 HP.", AFTER_ENEMY_TURN);
                        }
                        else
                        {
                            SetMessage("You don't have any potions!", AFTER_BACK_TO_MENU);
                        }
                    }
                    else { combatMenu = CM_MAIN; menuSelection = 0; }
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
            if (animTimer >= ANIM_DURATION)
                ResolveAnimation(playerHealth, playerPotions, playerScore, playerExp);
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

    return currentResult;
}

/* ---------- Combat drawing ---------- */
static void DrawHealthBar(int x, int y, int width, int health, int maxHealth, const char *label)
{
    float percent = (maxHealth > 0) ? ((float)health / (float)maxHealth) : 0.0f;
    if (percent < 0.0f) percent = 0.0f;

    DrawRectangle(x, y, width, HEALTH_BAR_H, (Color){ 60, 60, 60, 255 });
    DrawRectangle(x, y, (int)(width * percent), HEALTH_BAR_H, (Color){ 60, 200, 90, 255 });
    DrawRectangleLines(x, y, width, HEALTH_BAR_H, BLACK);
    DrawText(TextFormat("%s HP: %d/%d", label, health, maxHealth), x, y - 20, 18, WHITE);
}

static void DrawCombatAnimation(void)
{
    float t = animTimer / ANIM_DURATION;
    if (t > 1.0f) t = 1.0f;

    Vector2 origin = (animActor == ACTOR_PLAYER) ? PLAYER_COMBAT_POS : ENEMY_COMBAT_POS;
    Vector2 target = (animActor == ACTOR_PLAYER) ? ENEMY_COMBAT_POS  : PLAYER_COMBAT_POS;

    if (animType == ANIM_ATTACK)
    {
        float wave = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
        Vector2 mid = LerpVector(origin, target, 0.35f);
        Vector2 pos = LerpVector(origin, mid, wave);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV(pos, 8.0f, (Color){ 255, 240, 200, 180 });
        DrawCircleV(pos, 4.0f, (Color){ 255, 255, 255, 255 });
        EndBlendMode();
    }
}

void CombatDraw(int playerHealth, int maxPlayerHealth,
                int playerPotions, int playerScore)
{
    BackgroundsDraw(currentBackground);

    DrawText(TextFormat("Score: %d", playerScore), 20, 20, 20, YELLOW);
    DrawText(BackgroundsGetName(currentBackground), 20, 44, 14, (Color){ 220, 220, 220, 220 });

    {
        float lungeT = 0.0f;
        if (phase == PHASE_ANIM)
        {
            float t = animTimer / ANIM_DURATION;
            if (t > 1.0f) t = 1.0f;
            lungeT = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
        }

        CombatantPose playerPose = {
            (animActor == ACTOR_PLAYER && phase == PHASE_ANIM) ? lungeT : 0.0f,
            combatTime
        };
        CombatantPose enemyPose = {
            (animActor == ACTOR_ENEMY && phase == PHASE_ANIM) ? lungeT : 0.0f,
            combatTime + 0.7f
        };

        CombatantDrawKnight(PLAYER_COMBAT_POS, true,  playerPose, COMBAT_SPRITE_SCALE);
        CombatantDrawWolf  (ENEMY_COMBAT_POS,  true,  enemyPose,  COMBAT_SPRITE_SCALE);
    }

    {
        int totalWidth = HEALTH_BAR_W * 2 + HEALTH_BAR_GAP;
        int startX = (SCREEN_WIDTH - totalWidth) / 2;

        DrawHealthBar(startX, HEALTH_BAR_Y, HEALTH_BAR_W,
                      playerHealth, maxPlayerHealth, "Player");
        DrawHealthBar(startX + HEALTH_BAR_W + HEALTH_BAR_GAP, HEALTH_BAR_Y, HEALTH_BAR_W,
                      enemy.health, enemy.maxHealth, "Wolf");
    }

    if (phase == PHASE_ANIM && animType == ANIM_ATTACK)
        DrawCombatAnimation();

    CombatDrawEffects();

    if (phase == PHASE_MENU)
    {
        const char *mainOptions[4]  = { "Attack", "Spell", "Items", "Flee" };
        const char *spellOptions[3] = { "Fireball", "Ice Shard", "Back" };
        const char *itemOptions[2]  = { TextFormat("Health Potion (x%d)", playerPotions), "Back" };

        const char **options = mainOptions;
        int count = 4;
        if (combatMenu == CM_SPELL) { options = spellOptions; count = 3; }
        if (combatMenu == CM_ITEM)  { options = itemOptions;  count = 2; }

        const int fontSize = 20;
        const int lineHeight = 28;
        const int paddingX = 18;
        const int paddingY = 14;
        const int prefixWidth = MeasureText("> ", fontSize);

        int maxTextWidth = 0;
        for (int i = 0; i < count; i++)
        {
            int width = MeasureText(options[i], fontSize);
            if (width > maxTextWidth) maxTextWidth = width;
        }

        int boxWidth = prefixWidth + maxTextWidth + paddingX * 2;
        int boxHeight = lineHeight * count + paddingY * 2;
        int boxX = (SCREEN_WIDTH - boxWidth) / 2;
        int boxY = (SCREEN_HEIGHT - boxHeight) / 2;

        DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 0, 0, 0, 190 });
        DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, (Color){ 220, 220, 220, 200 });

        for (int i = 0; i < count; i++)
        {
            Color color = (i == menuSelection) ? YELLOW : WHITE;
            const char *prefix = (i == menuSelection) ? "> " : "  ";
            DrawText(TextFormat("%s%s", prefix, options[i]),
                     boxX + paddingX, boxY + paddingY + i * lineHeight, fontSize, color);
        }
    }

    if (phase == PHASE_MESSAGE)
    {
        const int fontSize = 20;
        int textWidth = MeasureText(message, fontSize);
        int boxWidth = textWidth + 60;
        if (boxWidth < 320) boxWidth = 320;
        if (boxWidth > 760) boxWidth = 760;

        int boxHeight = 52;
        int boxX = (SCREEN_WIDTH - boxWidth) / 2;
        int boxY = 76;

        DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 0, 0, 0, 210 });
        DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
        DrawText(message, boxX + (boxWidth - textWidth) / 2,
                 boxY + (boxHeight - fontSize) / 2, fontSize, WHITE);
    }
}