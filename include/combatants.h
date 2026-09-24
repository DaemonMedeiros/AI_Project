/* ============================================================
 * combatants.h - Procedural combatant sprites
 * ============================================================ */
#ifndef COMBATANTS_H
#define COMBATANTS_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    float poseT;    /* 0 at rest, ramps to 1 at attack peak */
    float time;     /* total elapsed, for idle bob */
} CombatantPose;

void CombatantDrawKnight(Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawEnemy(const char *name, Vector2 pos, bool facingRight, CombatantPose pose);

void CombatantDrawSlime (Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawGoblin(Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawWolf  (Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawOrc   (Vector2 pos, bool facingRight, CombatantPose pose);

#endif /* COMBATANTS_H */