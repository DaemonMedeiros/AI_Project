/* ============================================================
 * combat.c - Turn-based combat system with spell VFX
 *
 * Includes:
 *   - Turn / menu / damage resolution logic
 *   - Fireball VFX: randomized particles, fire trail, explosion
 *   - Ice spell VFX: crystalline shards, frost trail, shatter burst
 * ============================================================ */
#include "../include/combat.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ---------- Layout ---------- */
static const Vector2 playerCombatPos = { 180, 380 };
static const Vector2 enemyCombatPos  = { 600, 200 };

/* ---------- Enemy roster ---------- */
static const char *enemyNames[NUM_ENEMY_TYPES]   = { "Slime", "Goblin", "Wolf", "Orc" };
static const int   enemyHealths[NUM_ENEMY_TYPES] = { 15, 22, 28, 36 };

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

/* ---------- Effect system ---------- */
#define MAX_PARTICLES 512

typedef enum {
    PT_FIRE_TRAIL, PT_FIRE_BURST, PT_FIRE_EMBER,
    PT_ICE_TRAIL, PT_ICE_SHARD, PT_ICE_MIST, PT_ICE_BURST
} ParticleType;

typedef struct {
    bool    active;
    ParticleType type;
    Vector2 pos;
    Vector2 vel;
    float   life;      /* seconds remaining */
    float   maxLife;
    float   size;
    float   rot;
    float   rotSpeed;
    Color   colStart;
    Color   colEnd;
} Particle;

static Particle particles[MAX_PARTICLES];

/* Projectile animation for the standalone spells */
typedef struct {
    bool    active;
    bool    isIce;      /* false = fire */
    Vector2 origin;
    Vector2 target;
    float   t;          /* 0..1 */
    float   duration;
    bool    exploded;
    float   explodeTimer;
    float   jitterSeed;
} SpellProjectile;

#define MAX_PROJECTILES 8
static SpellProjectile projectiles[MAX_PROJECTILES];

/* ---------- Utility ---------- */
static int   ClampInt(int v, int lo, int hi){ if (v<lo) return lo; if (v>hi) return hi; return v; }
static float RandRange(float lo, float hi){ return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo); }
static float RandSym(float range){ return RandRange(-range, range); }

static Vector2 Lerp2(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

/* Allocate a free particle slot; returns index or -1 */
static int AllocParticle(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (!particles[i].active) return i;
    return -1;
}

static void SpawnParticle(ParticleType type, Vector2 pos, Vector2 vel,
                          float life, float size,
                          Color colStart, Color colEnd)
{
    int idx = AllocParticle();
    if (idx < 0) return;
    Particle *p = &particles[idx];
    p->active   = true;
    p->type     = type;
    p->pos      = pos;
    p->vel      = vel;
    p->life     = life;
    p->maxLife  = life;
    p->size     = size;
    p->rot      = RandRange(0.0f, PI * 2.0f);
    p->rotSpeed = RandSym(6.0f);
    p->colStart = colStart;
    p->colEnd   = colEnd;
}

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Color FadeColor(Color c, float alphaScale)
{
    float a = c.a * alphaScale;
    if (a < 0) a = 0; if (a > 255) a = 255;
    c.a = (unsigned char)a;
    return c;
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
}

void CombatCleanup(void)
{
    CombatClearEffects();
}

void CombatClearEffects(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    for (int i = 0; i < MAX_PROJECTILES; i++) projectiles[i].active = false;
}

const char *CombatGetEnemyName(void) { return enemy.name ? enemy.name : "?"; }

/* ---------- Encounter setup ---------- */
void CombatStartEncounter(void)
{
    int t = GetRandomValue(0, NUM_ENEMY_TYPES - 1);
    enemy.name      = enemyNames[t];
    enemy.maxHealth = enemyHealths[t];
    enemy.health    = enemyHealths[t];

    phase = PHASE_MENU;
    combatMenu = CM_MAIN;
    menuSelection = 0;
    currentResult = COMBAT_RESULT_NONE;
    CombatClearEffects();
}

/* ---------- Message system ---------- */
static void SetMessage(const char *text, AfterMessage after)
{
    strncpy(message, text, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    messageTimer = MESSAGE_DURATION;
    afterMessage = after;
    phase = PHASE_MESSAGE;
}

/* ============================================================
 *  Fireball VFX
 * ============================================================ */
static void EmitFireTrail(Vector2 pos, float intensity)
{
    /* Core ember */
    int n = GetRandomValue(2, 4);
    for (int i = 0; i < n; i++)
    {
        Vector2 v = { RandSym(60.0f) - 40.0f, RandSym(60.0f) };
        Color start = (Color){ 255, (unsigned char)GetRandomValue(180, 240), 60, 255 };
        Color end   = (Color){ 200, 40, 10, 0 };
        SpawnParticle(PT_FIRE_TRAIL, pos, v,
                      RandRange(0.25f, 0.55f) * intensity,
                      RandRange(6.0f, 12.0f) * intensity,
                      start, end);
    }
    /* Occasional smoke puff */
    if (GetRandomValue(0, 3) == 0)
    {
        Vector2 v = { RandSym(30.0f), RandSym(30.0f) - 20.0f };
        SpawnParticle(PT_FIRE_TRAIL, pos, v,
                      RandRange(0.5f, 0.9f),
                      RandRange(10.0f, 16.0f),
                      (Color){ 120, 100, 90, 140 },
                      (Color){ 60, 50, 50, 0 });
    }
}

static void EmitFireExplosion(Vector2 pos)
{
    /* Bright core flash */
    for (int i = 0; i < 12; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(60.0f, 220.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_FIRE_BURST, pos, v,
                      RandRange(0.35f, 0.6f),
                      RandRange(14.0f, 26.0f),
                      (Color){ 255, 250, 200, 255 },
                      (Color){ 255, 80, 20, 0 });
    }
    /* Mid-ring orange */
    for (int i = 0; i < 26; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(120.0f, 420.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_FIRE_BURST, pos, v,
                      RandRange(0.5f, 0.9f),
                      RandRange(10.0f, 20.0f),
                      (Color){ 255, 170, 60, 255 },
                      (Color){ 180, 30, 10, 0 });
    }
    /* Embers - small, long-lived, drift upward */
    for (int i = 0; i < 30; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(40.0f, 200.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd - RandRange(0.0f, 60.0f) };
        SpawnParticle(PT_FIRE_EMBER, pos, v,
                      RandRange(0.7f, 1.4f),
                      RandRange(2.0f, 5.0f),
                      (Color){ 255, 230, 120, 255 },
                      (Color){ 200, 60, 0, 0 });
    }
    /* Shockwave ring: particles with zero velocity but large size, expanding */
    for (int i = 0; i < 20; i++)
    {
        float ang = (float)i / 20.0f * PI * 2.0f;
        float spd = 260.0f;
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_FIRE_BURST, pos, v,
                      0.45f, 8.0f,
                      (Color){ 255, 240, 180, 255 },
                      (Color){ 255, 100, 30, 0 });
    }
}

/* ============================================================
 *  Ice spell VFX
 * ============================================================ */
static void EmitIceTrail(Vector2 pos, float intensity)
{
    /* Cool mist / snowflake specks */
    int n = GetRandomValue(2, 4);
    for (int i = 0; i < n; i++)
    {
        Vector2 v = { RandSym(40.0f) - 20.0f, RandSym(50.0f) };
        Color start = (Color){ 200, 235, 255, 255 };
        Color end   = (Color){ 90, 150, 220, 0 };
        SpawnParticle(PT_ICE_TRAIL, pos, v,
                      RandRange(0.3f, 0.6f) * intensity,
                      RandRange(4.0f, 9.0f) * intensity,
                      start, end);
    }
    /* Occasional crystalline shard fragment */
    if (GetRandomValue(0, 2) == 0)
    {
        Vector2 v = { RandSym(70.0f), RandSym(70.0f) };
        SpawnParticle(PT_ICE_SHARD, pos, v,
                      RandRange(0.4f, 0.8f),
                      RandRange(6.0f, 11.0f),
                      (Color){ 230, 250, 255, 255 },
                      (Color){ 120, 180, 240, 0 });
    }
}

static void EmitIceExplosion(Vector2 pos)
{
    /* White-blue core flash */
    for (int i = 0; i < 10; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(50.0f, 180.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_ICE_BURST, pos, v,
                      RandRange(0.3f, 0.55f),
                      RandRange(16.0f, 28.0f),
                      (Color){ 255, 255, 255, 255 },
                      (Color){ 140, 200, 255, 0 });
    }
    /* Crystal shards bursting outward */
    for (int i = 0; i < 32; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(180.0f, 480.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_ICE_SHARD, pos, v,
                      RandRange(0.6f, 1.1f),
                      RandRange(7.0f, 15.0f),
                      (Color){ 240, 250, 255, 255 },
                      (Color){ 90, 150, 220, 0 });
    }
    /* Frost mist - slow, expanding, pale */
    for (int i = 0; i < 24; i++)
    {
        float ang = RandRange(0, PI * 2.0f);
        float spd = RandRange(20.0f, 90.0f);
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd - 15.0f };
        SpawnParticle(PT_ICE_MIST, pos, v,
                      RandRange(0.9f, 1.6f),
                      RandRange(18.0f, 34.0f),
                      (Color){ 200, 230, 255, 200 },
                      (Color){ 150, 200, 240, 0 });
    }
    /* Shockwave ring */
    for (int i = 0; i < 22; i++)
    {
        float ang = (float)i / 22.0f * PI * 2.0f;
        float spd = 240.0f;
        Vector2 v = { cosf(ang) * spd, sinf(ang) * spd };
        SpawnParticle(PT_ICE_BURST, pos, v,
                      0.5f, 8.0f,
                      (Color){ 220, 245, 255, 255 },
                      (Color){ 120, 180, 240, 0 });
    }
}

/* ============================================================
 *  Standalone projectile spawning (also used by combat flow)
 * ============================================================ */
static int AllocProjectile(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (!projectiles[i].active) return i;
    return -1;
}

static void SpawnProjectile(Vector2 origin, Vector2 target, bool isIce)
{
    int idx = AllocProjectile();
    if (idx < 0) return;
    SpellProjectile *p = &projectiles[idx];
    p->active       = true;
    p->isIce        = isIce;
    p->origin       = origin;
    p->target       = target;
    p->t            = 0.0f;
    p->duration     = ANIM_DURATION;
    p->exploded     = false;
    p->explodeTimer = 0.0f;
    p->jitterSeed   = RandRange(0.0f, 1000.0f);
}

void CombatSpawnFireball(Vector2 origin, Vector2 target) { SpawnProjectile(origin, target, false); }
void CombatSpawnIceSpell(Vector2 origin, Vector2 target) { SpawnProjectile(origin, target, true);  }

/* ============================================================
 *  Effect update
 * ============================================================ */
void CombatUpdateEffects(float dt)
{
    /* Particles */
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &particles[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) { p->active = false; continue; }

        /* Per-type motion */
        switch (p->type)
        {
            case PT_FIRE_TRAIL:
            case PT_FIRE_BURST:
                p->vel.x *= (1.0f - 2.5f * dt);
                p->vel.y *= (1.0f - 2.5f * dt);
                p->vel.y -= 30.0f * dt;   /* rise */
                break;
            case PT_FIRE_EMBER:
                p->vel.x *= (1.0f - 1.2f * dt);
                p->vel.y -= 40.0f * dt;
                break;
            case PT_ICE_TRAIL:
            case PT_ICE_BURST:
                p->vel.x *= (1.0f - 3.0f * dt);
                p->vel.y *= (1.0f - 3.0f * dt);
                break;
            case PT_ICE_SHARD:
                p->vel.x *= (1.0f - 1.5f * dt);
                p->vel.y *= (1.0f - 1.5f * dt);
                p->vel.y += 120.0f * dt;  /* gravity - shards fall */
                break;
            case PT_ICE_MIST:
                p->vel.x *= (1.0f - 1.8f * dt);
                p->vel.y *= (1.0f - 1.8f * dt);
                p->vel.y -= 8.0f * dt;
                break;
        }

        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->rot   += p->rotSpeed * dt;
    }

    /* Projectiles: emit trail particles while traveling, explode on impact */
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        SpellProjectile *p = &projectiles[i];
        if (!p->active) continue;

        if (!p->exploded)
        {
            p->t += dt / p->duration;

            /* Per-frame trail emission */
            Vector2 pos = Lerp2(p->origin, p->target, p->t);
            if (p->isIce) EmitIceTrail(pos, 1.0f);
            else          EmitFireTrail(pos, 1.0f);

            if (p->t >= 1.0f)
            {
                p->t = 1.0f;
                p->exploded = true;
                p->explodeTimer = 0.35f;
                if (p->isIce) EmitIceExplosion(p->target);
                else          EmitFireExplosion(p->target);
            }
        }
        else
        {
            p->explodeTimer -= dt;
            if (p->explodeTimer <= 0.0f) p->active = false;
        }
    }
}

/* ============================================================
 *  Effect drawing
 * ============================================================ */
static void DrawParticle(const Particle *p)
{
    float t = 1.0f - (p->life / p->maxLife);   /* 0 fresh -> 1 dead */
    Color c = LerpColor(p->colStart, p->colEnd, t);
    float size = p->size * (1.0f - t * 0.7f);

    switch (p->type)
    {
        case PT_FIRE_TRAIL:
        case PT_FIRE_BURST:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(p->pos, size, FadeColor(c, 1.0f - t));
            DrawCircleV(p->pos, size * 0.5f, FadeColor((Color){255,255,220,c.a}, 1.0f - t));
            EndBlendMode();
            break;

        case PT_FIRE_EMBER:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(p->pos, size, FadeColor(c, 1.0f - t));
            EndBlendMode();
            break;

        case PT_ICE_TRAIL:
        case PT_ICE_MIST:
            DrawCircleV(p->pos, size, FadeColor(c, (1.0f - t) * 0.85f));
            break;

        case PT_ICE_SHARD:
        {
            /* Diamond-shaped shard */
            float s = size;
            Vector2 pts[4] = {
                { p->pos.x,             p->pos.y - s        },
                { p->pos.x + s * 0.6f,  p->pos.y            },
                { p->pos.x,             p->pos.y + s        },
                { p->pos.x - s * 0.6f,  p->pos.y            }
            };
            /* Slight rotation via manual transform */
            float cs = cosf(p->rot), sn = sinf(p->rot);
            for (int k = 0; k < 4; k++)
            {
                float dx = pts[k].x - p->pos.x;
                float dy = pts[k].y - p->pos.y;
                pts[k].x = p->pos.x + dx * cs - dy * sn;
                pts[k].y = p->pos.y + dx * sn + dy * cs;
            }
            Color cc = FadeColor(c, 1.0f - t * 0.8f);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawTriangle(pts[0], pts[3], pts[1], cc);
            DrawTriangle(pts[1], pts[3], pts[2], cc);
            EndBlendMode();
            break;
        }

        case PT_ICE_BURST:
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(p->pos, size, FadeColor(c, 1.0f - t));
            EndBlendMode();
            break;
    }
}

void CombatDrawEffects(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (particles[i].active) DrawParticle(&particles[i]);

    /* Projectile cores drawn on top of their trails */
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        SpellProjectile *p = &projectiles[i];
        if (!p->active || p->exploded) continue;

        Vector2 pos = Lerp2(p->origin, p->target, p->t);

        if (p->isIce)
        {
            /* Cool layered glow */
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(pos, 18, (Color){ 120, 180, 255, 90 });
            DrawCircleV(pos, 12, (Color){ 180, 230, 255, 180 });
            DrawCircleV(pos,  7, (Color){ 255, 255, 255, 255 });
            EndBlendMode();
            /* Rotating crystalline shards around core */
            for (int k = 0; k < 4; k++)
            {
                float a = p->jitterSeed + (float)k * PI * 0.5f + p->t * 8.0f;
                Vector2 q = { pos.x + cosf(a) * 12.0f, pos.y + sinf(a) * 12.0f };
                DrawPoly(q, 3, 5.0f, (a * RAD2DEG) + 90.0f, (Color){ 220, 245, 255, 220 });
            }
        }
        else
        {
            /* Flickering fire core */
            float flicker = 1.0f + sinf((p->t * 40.0f) + p->jitterSeed) * 0.08f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(pos, 22 * flicker, (Color){ 255, 90, 30, 70 });
            DrawCircleV(pos, 14 * flicker, (Color){ 255, 160, 60, 180 });
            DrawCircleV(pos,  8 * flicker, (Color){ 255, 240, 180, 255 });
            EndBlendMode();
        }
    }
}

/* ============================================================
 *  Turn resolution
 * ============================================================ */
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
    else /* ACTOR_ENEMY */
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

/* ============================================================
 *  Main combat update
 * ============================================================ */
CombatResult CombatUpdate(float dt,
                          int *playerHealth, int *playerPotions,
                          int *playerScore, int *playerExp)
{
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
                /* Options: Fireball / Ice Shard / Back */
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 3;
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menuSelection = (menuSelection + 2) % 3;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    if (menuSelection == 0) /* Fireball */
                    {
                        animType = ANIM_SPELL_FIRE; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_SPELL_DMG; animTimer = 0.0f;
                        CombatSpawnFireball(playerCombatPos, enemyCombatPos);
                        phase = PHASE_ANIM;
                    }
                    else if (menuSelection == 1) /* Ice Shard */
                    {
                        animType = ANIM_SPELL_ICE; animActor = ACTOR_PLAYER;
                        pendingDamage = PLAYER_SPELL_DMG; animTimer = 0.0f;
                        CombatSpawnIceSpell(playerCombatPos, enemyCombatPos);
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
                    else
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

/* ============================================================
 *  Combat drawing
 * ============================================================ */
static void DrawHealthBar(int x, int y, int width, int health, int maxHealth, const char *label)
{
    float pct = (maxHealth > 0) ? ((float)health / (float)maxHealth) : 0.0f;
    if (pct < 0.0f) pct = 0.0f;

    DrawRectangle(x, y, width, 18, (Color){ 60, 60, 60, 255 });
    DrawRectangle(x, y, (int)(width * pct), 18, (Color){ 60, 200, 90, 255 });
    DrawRectangleLines(x, y, width, 18, BLACK);

    DrawText(TextFormat("%s HP: %d/%d", label, health, maxHealth), x, y - 20, 18, WHITE);
}

static void DrawCombatAnimation(void)
{
    float t = animTimer / ANIM_DURATION;
    if (t > 1.0f) t = 1.0f;

    Vector2 origin = (animActor == ACTOR_PLAYER) ? playerCombatPos : enemyCombatPos;
    Vector2 target = (animActor == ACTOR_PLAYER) ? enemyCombatPos  : playerCombatPos;

    if (animType == ANIM_ATTACK)
    {
        float wave = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
        Vector2 mid = Lerp2(origin, target, 0.35f);
        Vector2 pos = Lerp2(origin, mid, wave);
        DrawLineEx(origin, pos, 4, DARKGRAY);
        DrawCircleV(pos, 8, GRAY);
    }
    /* Spell animation is handled by CombatDrawEffects() */
}

void CombatDraw(int playerHealth, int maxPlayerHealth,
                int playerPotions, int playerScore)
{
    ClearBackground((Color){ 25, 25, 40, 255 });

    DrawText(TextFormat("Score: %d", playerScore), 20, 20, 20, YELLOW);

    /* Combatant markers */
    DrawRectangle((int)playerCombatPos.x - 20, (int)playerCombatPos.y - 20, 40, 40, (Color){ 60, 120, 230, 255 });
    DrawRectangleLines((int)playerCombatPos.x - 20, (int)playerCombatPos.y - 20, 40, 40, BLACK);

    DrawRectangle((int)enemyCombatPos.x - 20, (int)enemyCombatPos.y - 20, 40, 40, (Color){ 200, 60, 60, 255 });
    DrawRectangleLines((int)enemyCombatPos.x - 20, (int)enemyCombatPos.y - 20, 40, 40, BLACK);
    DrawText(enemy.name, (int)enemyCombatPos.x - 30, (int)enemyCombatPos.y + 30, 18, WHITE);

    DrawHealthBar(80, 460, 220, playerHealth, maxPlayerHealth, "Player");
    DrawHealthBar(500, 100, 220, enemy.health, enemy.maxHealth, "Enemy");

    /* Basic attack animation (spells handled by effects layer) */
    if (phase == PHASE_ANIM && animType == ANIM_ATTACK)
        DrawCombatAnimation();

    /* Spell / particle effects (drawn above combatants) */
    CombatDrawEffects();

    if (phase == PHASE_MENU)
    {
        int mx = 60, my = 500, lineH = 26;
        const char *mainOptions[4]  = { "Attack", "Spell", "Items", "Flee" };
        const char *spellOptions[3] = { "Fireball", "Ice Shard", "Back" };
        const char *itemOptions[2]  = { TextFormat("Health Potion (x%d)", playerPotions), "Back" };

        const char **options = mainOptions;
        int count = 4;
        if (combatMenu == CM_SPELL) { options = spellOptions; count = 3; }
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
        int bx = (800 - boxW) / 2, by = 600 - 100;
        DrawRectangle(bx, by, boxW, boxH, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines(bx, by, boxW, boxH, WHITE);
        DrawText(message, bx + 20, by + 20, 20, WHITE);
    }
}