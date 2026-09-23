/* ============================================================
 * combatants.h - Procedural combatant sprites
 *
 * Draws each enemy type + the player knight from primitive shapes.
 * All draw calls take a center position and a "flip" hint so the
 * sprite faces its opponent. A simple pose parameter drives the
 * attack-lunge animation.
 * ============================================================ */
#ifndef COMBATANTS_H
#define COMBATANTS_H

#include "raylib.h"
#include <stdbool.h>

/* Pose drives subtle animation.
 *   poseT: 0 at rest, ramps to 1 at peak of attack lunge
 *   time : total elapsed, for idle bob */
typedef struct {
    float poseT;
    float time;
} CombatantPose;

/* Draw the player knight centered at pos. facingRight = true means
 * the knight faces to the right (toward the enemy). */
void CombatantDrawKnight(Vector2 pos, bool facingRight, CombatantPose pose);

/* Draw the enemy, dispatched by name ("Slime","Goblin","Wolf","Orc"). */
void CombatantDrawEnemy(const char *name, Vector2 pos, bool facingRight, CombatantPose pose);

/* Individual sprite drawing (exposed for reuse) */
void CombatantDrawSlime (Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawGoblin(Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawWolf  (Vector2 pos, bool facingRight, CombatantPose pose);
void CombatantDrawOrc   (Vector2 pos, bool facingRight, CombatantPose pose);

#endif /* COMBATANTS_H */