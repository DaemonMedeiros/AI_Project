#ifndef BATTLE_H
#define BATTLE_H

#include "types.h"
#include "raylib.h"

#define TYPE_NORMAL   0
#define TYPE_FIGHTING 1
#define TYPE_FLYING   2
#define TYPE_POISON   3
#define TYPE_GROUND   4
#define TYPE_ROCK     5
#define TYPE_BUG      6
#define TYPE_GHOST    7
#define TYPE_FIRE     20
#define TYPE_WATER    21
#define TYPE_GRASS    22
#define TYPE_ELECTRIC 23
#define TYPE_PSYCHIC  24
#define TYPE_ICE      25
#define TYPE_DRAGON   26

/* Move IDs */
#define MOVE_NONE        0
#define MOVE_SCRATCH    10
#define MOVE_GUST       16
#define MOVE_SAND_ATTACK 28
#define MOVE_TACKLE     33
#define MOVE_TAIL_WHIP  39
#define MOVE_GROWL      45
#define MOVE_QUICK_ATTACK 98

#define BATTLE_NUM_MOVES 4

typedef enum {
    BATTLE_PHASE_NONE = 0,
    BATTLE_PHASE_INTRO,
    BATTLE_PHASE_MENU,
    BATTLE_PHASE_MOVE_SELECT,
    BATTLE_PHASE_PLAYER_MOVE,
    BATTLE_PHASE_ENEMY_MOVE,
    BATTLE_PHASE_MESSAGE,
    BATTLE_PHASE_END,
} BattlePhase;

/* 0=Fight(top-left), 1=Item(top-right), 2=PKMN(bottom-left), 3=Run(bottom-right) */
typedef enum {
    BATTLE_MENU_FIGHT = 0,
    BATTLE_MENU_ITEM,
    BATTLE_MENU_POKEMON,
    BATTLE_MENU_RUN,
    BATTLE_MENU_COUNT,
} BattleMenuItem;

typedef struct {
    uint8_t  species;
    uint8_t  level;
    uint16_t hp;
    uint16_t max_hp;
    uint8_t  attack;
    uint8_t  defense;
    uint8_t  speed;
    uint8_t  special;
    uint8_t  type1;
    uint8_t  type2;
    uint8_t  moves[BATTLE_NUM_MOVES];
    uint8_t  pp[BATTLE_NUM_MOVES];
    char     name[16];    /* widened from 12 to hold every Gen 1 name */
    int      fainted;
    int      is_player;
} BattleMon;

typedef struct {
    BattlePhase phase;
    int active;
    BattleMon player_mon;
    BattleMon enemy_mon;
    int menu_cursor;
    int move_cursor;
    char message[80];
    int message_timer;
    int phase_timer;
    int player_went_first;
    int turn_executed;
    int escape_attempts;
    int battle_result;    /* 0 = ongoing, 1 = win, 2 = lose, 3 = ran */
    int return_to_overworld;

    /* Edge-detection state for menu input (30 Hz logic vs 60 Hz render) */
    int prev_up;
    int prev_down;
    int prev_left;
    int prev_right;
    int prev_select;
    int prev_cancel;
} BattleState;

/* One-time init */
void battle_init(BattleState* bs);

/* Kick off a wild battle */
void battle_start_wild(BattleState* bs, uint8_t enemy_species, uint8_t enemy_level);

/* Logic tick (call once per LOGIC_DT while active) */
void battle_update(BattleState* bs, Player* player);

/* Draw inside an active BeginTextureMode(target) block at 160x144.
 * Only composites a pre-rendered layer, so it is safe to call from
 * inside another BeginTextureMode. */
void battle_render(const BattleState* bs);

/* Helpers */
int  battle_is_active(const BattleState* bs);

/* Cleanup (call before CloseWindow) */
void battle_unload_sprites(void);

#endif