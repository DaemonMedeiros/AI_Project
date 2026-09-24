/* ============================================================
 * combat.h - Turn-based combat system with spell VFX
 * ============================================================ */
#ifndef COMBAT_H
#define COMBAT_H

#include "raylib.h"
#include "backgrounds.h"
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
#define ANIM_DURATION        0.6f
#define MESSAGE_DURATION     1.6f

/* ---------- Enums ---------- */
typedef enum { CM_MAIN, CM_SPELL, CM_ITEM } CombatMenu;
typedef enum { ACTOR_PLAYER, ACTOR_ENEMY } Actor;
typedef enum { ANIM_ATTACK, ANIM_SPELL_FIRE, ANIM_SPELL_ICE } AnimType;
typedef enum { PHASE_MENU, PHASE_ANIM, PHASE_MESSAGE } CombatPhase;

typedef enum {
    AFTER_NONE,
    AFTER_ENEMY_TURN,
    AFTER_BACK_TO_MENU,
    AFTER_VICTORY,
    AFTER_RETURN_WORLD,
    AFTER_GAMEOVER
} AfterMessage;

typedef enum {
    COMBAT_RESULT_NONE,
    COMBAT_RESULT_VICTORY,
    COMBAT_RESULT_FLED,
    COMBAT_RESULT_DEFEAT
} CombatResult;

typedef struct {
    const char *name;
    int maxHealth, health;
} Enemy;

/* ---------- Public API ---------- */
void CombatInit(void);
void CombatCleanup(void);
void CombatStartEncounter(void);

CombatResult CombatUpdate(float dt,
                          int *playerHealth,
                          int *playerPotions,
                          int *playerScore,
                          int *playerExp);

void CombatDraw(int playerHealth, int maxPlayerHealth,
                int playerPotions, int playerScore);

const char *CombatGetEnemyName(void);
BackgroundType CombatGetBackground(void);

void CombatSpawnFireball(Vector2 origin, Vector2 target);
void CombatSpawnIceSpell(Vector2 origin, Vector2 target);
void CombatUpdateEffects(float dt);
void CombatDrawEffects(void);
void CombatClearEffects(void);

#endif /* COMBAT_H */