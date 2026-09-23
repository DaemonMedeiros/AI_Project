/* ============================================================
 * combat.h - Turn-based combat system with spell VFX
 * ============================================================ */
#ifndef COMBAT_H
#define COMBAT_H

#include "raylib.h"
#include <stdbool.h>

/* ---------- Tunables ---------- */
#define PLAYER_MAX_HP        50
#define PLAYER_ATTACK_DMG    8
#define PLAYER_SPELL_DMG     15
#define ENEMY_ATTACK_DMG     5
#define ENEMY_SPELL_DMG      9
#define POTION_HEAL_AMOUNT   20
#define FLEE_SUCCESS_CHANCE  50
#define EXP_PER_KILL         15

#define NUM_ENEMY_TYPES 4

#define ANIM_DURATION        0.6f
#define MESSAGE_DURATION     1.6f

/* ---------- Menu / phase enums ---------- */
typedef enum { CM_MAIN, CM_SPELL, CM_ITEM } CombatMenu;

typedef enum { ACTOR_PLAYER, ACTOR_ENEMY } Actor;
typedef enum {
    ANIM_ATTACK,
    ANIM_SPELL_FIRE,
    ANIM_SPELL_ICE
} AnimType;

typedef enum { PHASE_MENU, PHASE_ANIM, PHASE_MESSAGE } CombatPhase;

typedef enum {
    AFTER_NONE,
    AFTER_ENEMY_TURN,
    AFTER_BACK_TO_MENU,
    AFTER_VICTORY,
    AFTER_RETURN_WORLD,
    AFTER_GAMEOVER
} AfterMessage;

/* ---------- Combatant data ---------- */
typedef struct {
    const char *name;
    int maxHealth, health;
} Enemy;

/* ---------- Combat resolution result returned to main ---------- */
typedef enum {
    COMBAT_RESULT_NONE,      /* combat still running */
    COMBAT_RESULT_VICTORY,   /* enemy defeated */
    COMBAT_RESULT_FLED,      /* player escaped */
    COMBAT_RESULT_DEFEAT     /* player died */
} CombatResult;

/* ---------- Public API ---------- */
void  CombatInit(void);
void  CombatCleanup(void);

/* Begin a new encounter with a randomly chosen enemy */
void  CombatStartEncounter(void);

/* Update / draw. `playerHealth` etc. are passed by pointer so main owns the
 * persistent RPG stats. Returns the current CombatResult. */
CombatResult CombatUpdate(float dt,
                          int  *playerHealth,
                          int  *playerPotions,
                          int  *playerScore,
                          int  *playerExp);

void  CombatDraw(int playerHealth, int maxPlayerHealth,
                 int playerPotions, int playerScore);

/* Friendly name of the active enemy (for logging / UI) */
const char *CombatGetEnemyName(void);

/* ---------- Standalone spell VFX ----------
 * The combat module can also expose fireball / ice projectiles for other
 * systems to reuse. */
void  CombatSpawnFireball(Vector2 origin, Vector2 target);
void  CombatSpawnIceSpell(Vector2 origin, Vector2 target);
void  CombatUpdateEffects(float dt);
void  CombatDrawEffects(void);
void  CombatClearEffects(void);

#endif /* COMBAT_H */