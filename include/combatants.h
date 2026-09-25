/* ============================================================
 * combatants.h - Combatant sprite rendering
 *
 * Loads the knight and wolf textures once and draws them with
 * idle-bob and attack-lunge transforms. A per-call `scale`
 * multiplier lets call sites (world vs combat) render at
 * different sizes.
 *
 * The overworld knight is drawn from a separate 4x4 walking
 * spritesheet via CombatantDrawKnightAnimated().
 * ============================================================ */
#ifndef COMBATANTS_H
#define COMBATANTS_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    float poseT;    /* 0 at rest, ramps to 1 at attack peak */
    float time;     /* total elapsed, for idle bob */
} CombatantPose;

/* Direction the overworld knight is facing. Matches the
 * spritesheet rows: 0 = UP, 1 = RIGHT, 2 = DOWN, 3 = LEFT. */
typedef enum {
    KNIGHT_DIR_UP = 0,
    KNIGHT_DIR_RIGHT,
    KNIGHT_DIR_DOWN,
    KNIGHT_DIR_LEFT
} KnightDirection;

/* Load / unload the knight and wolf textures. */
void CombatantsInit(void);
void CombatantsUnload(void);

/* Draw the knight centered at pos. facingRight = true means the
 * knight faces to the right (toward the enemy). `scale` multiplies
 * the base target height (1.0 = base size, 2.0 = double). */
void CombatantDrawKnight(Vector2 pos, bool facingRight,
                         CombatantPose pose, float scale);

/* Draw the wolf, facing its opponent. `scale` behaves as above. */
void CombatantDrawWolf(Vector2 pos, bool facingRight,
                       CombatantPose pose, float scale);

/* Draw the overworld knight using the 4x4 walking spritesheet.
 * `direction` selects the row, `frame` selects the column
 * (0..3), and `scale` behaves as above. The sprite is drawn
 * feet-anchored at `pos`, matching CombatantDrawKnight. */
void CombatantDrawKnightAnimated(Vector2 pos, KnightDirection direction,
                                 int frame, float scale);

/* Height the knight sprite is drawn at when scale = 1.0. Useful
 * for callers that need to position the sprite relative to its
 * collision center. */
float CombatantGetKnightBaseHeight(void);

/* Height the wolf sprite is drawn at when scale = 1.0. */
float CombatantGetWolfBaseHeight(void);

#endif /* COMBATANTS_H */