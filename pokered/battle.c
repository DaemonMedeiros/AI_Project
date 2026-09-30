#include "battle.h"
#include "wild_encounter.h"
#include "gb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Move data                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t type;
    uint8_t power;
    uint8_t accuracy;
    uint8_t pp;
    uint8_t priority;
    uint8_t is_status;
} MoveInfo;

static const MoveInfo move_table[] = {
    [MOVE_NONE] = { TYPE_NORMAL,   0,   0,  0, 0, 1 },
    [MOVE_SCRATCH] = { TYPE_NORMAL,  40, 100, 35, 0, 0 },
    [MOVE_GUST] = { TYPE_NORMAL,  40, 100, 35, 0, 0 },
    [MOVE_SAND_ATTACK] = { TYPE_NORMAL,   0, 100, 15, 0, 1 },
    [MOVE_TACKLE] = { TYPE_NORMAL,  35,  95, 35, 0, 0 },
    [MOVE_TAIL_WHIP] = { TYPE_NORMAL,   0, 100, 30, 0, 1 },
    [MOVE_GROWL] = { TYPE_NORMAL,   0, 100, 40, 0, 1 },
    [MOVE_QUICK_ATTACK] = { TYPE_NORMAL,  40, 100, 30, 1, 0 },
};

static const MoveInfo* get_move(uint8_t id) {
    if (id >= sizeof(move_table) / sizeof(move_table[0])) return &move_table[MOVE_NONE];
    return &move_table[id];
}

/* ------------------------------------------------------------------ */
/* Base stats                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t hp, atk, def, spd, spc;
    uint8_t type1, type2;
    const char* name;
    uint8_t default_moves[BATTLE_NUM_MOVES];
} SpeciesInfo;

static const SpeciesInfo species_table[] = {
    [SPECIES_NONE] = {  0,  0,  0,  0,  0, TYPE_NORMAL, TYPE_NORMAL, "NONE",     {0,0,0,0} },
    [SPECIES_BULBASAUR] = { 45, 49, 49, 45, 65, TYPE_GRASS,  TYPE_POISON, "BULBASAUR",{MOVE_TACKLE, MOVE_GROWL, 0, 0} },
    [SPECIES_CHARMANDER] = { 39, 52, 43, 65, 50, TYPE_FIRE,   TYPE_FIRE,   "CHARMANDER",{MOVE_SCRATCH, MOVE_GROWL, 0, 0} },
    [SPECIES_SQUIRTLE] = { 44, 48, 65, 43, 50, TYPE_WATER,  TYPE_WATER,  "SQUIRTLE", {MOVE_TACKLE, MOVE_TAIL_WHIP, 0, 0} },
    [SPECIES_PIDGEY] = { 40, 45, 40, 56, 35, TYPE_NORMAL, TYPE_FLYING, "PIDGEY",   {MOVE_GUST, MOVE_SAND_ATTACK, 0, 0} },
    [SPECIES_RATTATA] = { 30, 56, 35, 72, 25, TYPE_NORMAL, TYPE_NORMAL, "RATTATA",  {MOVE_TACKLE, MOVE_TAIL_WHIP, MOVE_QUICK_ATTACK, 0} },
};

static const SpeciesInfo* get_species(uint8_t id) {
    if (id >= sizeof(species_table) / sizeof(species_table[0]))
        return &species_table[SPECIES_NONE];
    return &species_table[id];
}

/* ------------------------------------------------------------------ */
/* Stat calculation                                                    */
/* ------------------------------------------------------------------ */

static uint16_t calc_hp_stat(uint8_t base, uint8_t level) {
    int dv = 8;
    int v = ((base + dv) * 2 + 63) * level / 100 + level + 10;
    return (uint16_t)v;
}

static uint8_t calc_other_stat(uint8_t base, uint8_t level) {
    int dv = 8;
    int v = ((base + dv) * 2 + 63) * level / 100 + 5;
    if (v > 255) v = 255;
    return (uint8_t)v;
}

/* ------------------------------------------------------------------ */
/* Type effectiveness (stub — all 1x for now)                          */
/* ------------------------------------------------------------------ */

static float type_effectiveness(uint8_t atk_type, uint8_t def_type1, uint8_t def_type2) {
    (void)atk_type; (void)def_type1; (void)def_type2;
    return 1.0f;
}

/* ------------------------------------------------------------------ */
/* BattleMon construction                                              */
/* ------------------------------------------------------------------ */

static void build_mon(BattleMon* m, uint8_t species, uint8_t level, int is_player) {
    const SpeciesInfo* s = get_species(species);
    memset(m, 0, sizeof(*m));
    m->species = species;
    m->level = level;
    m->max_hp = calc_hp_stat(s->hp, level);
    m->hp = m->max_hp;
    m->attack = calc_other_stat(s->atk, level);
    m->defense = calc_other_stat(s->def, level);
    m->speed = calc_other_stat(s->spd, level);
    m->special = calc_other_stat(s->spc, level);
    m->type1 = s->type1;
    m->type2 = s->type2;
    strncpy(m->name, s->name, sizeof(m->name) - 1);
    for (int i = 0; i < BATTLE_NUM_MOVES; i++) {
        m->moves[i] = s->default_moves[i];
        if (m->moves[i]) m->pp[i] = get_move(m->moves[i])->pp;
    }
    m->is_player = is_player;
    m->fainted = 0;
}

/* ------------------------------------------------------------------ */
/* Damage formula (Gen 1)                                              */
/* ------------------------------------------------------------------ */

static int calc_damage(const BattleMon* atk, const BattleMon* def,
    uint8_t move_id, int* out_crit) {
    const MoveInfo* mv = get_move(move_id);
    if (mv->is_status || mv->power == 0) { *out_crit = 0; return 0; }

    int crit = (GetRandomValue(0, 255) < (atk->speed / 2)) ? 1 : 0;
    *out_crit = crit;

    int level = atk->level;
    int power = mv->power;

    int is_special = (mv->type == TYPE_FIRE || mv->type == TYPE_WATER ||
        mv->type == TYPE_GRASS || mv->type == TYPE_ELECTRIC ||
        mv->type == TYPE_PSYCHIC || mv->type == TYPE_ICE ||
        mv->type == TYPE_DRAGON);
    int A = is_special ? atk->special : atk->attack;
    int D = is_special ? def->special : def->defense;
    if (D == 0) D = 1;

    int base = (((2 * level / 5) + 2) * power * A) / D;
    base = base / 50 + 2;

    if (crit) base *= 2;
    if (mv->type == atk->type1 || mv->type == atk->type2) base = base * 3 / 2;

    float eff = type_effectiveness(mv->type, def->type1, def->type2);
    base = (int)(base * eff);

    int r = GetRandomValue(217, 255);
    base = base * r / 255;

    if (base < 1) base = 1;
    return base;
}

/* ------------------------------------------------------------------ */
/* Battle lifecycle                                                    */
/* ------------------------------------------------------------------ */

void battle_init(BattleState* bs) {
    memset(bs, 0, sizeof(*bs));
}

int battle_is_active(const BattleState* bs) {
    return bs->active;
}

static void set_message(BattleState* bs, const char* msg) {
    strncpy(bs->message, msg, sizeof(bs->message) - 1);
    bs->message[sizeof(bs->message) - 1] = 0;
    bs->message_timer = 0;
}

/* Forward declaration — defined below */
static void render_battle_to_layer(const BattleState* bs);

void battle_start_wild(BattleState* bs, uint8_t enemy_species, uint8_t enemy_level) {
    memset(bs, 0, sizeof(*bs));
    bs->active = 1;
    bs->phase = BATTLE_PHASE_INTRO;
    bs->phase_timer = 0;
    bs->menu_cursor = 0;
    bs->move_cursor = 0;
    bs->escape_attempts = 0;
    bs->battle_result = 0;

    build_mon(&bs->player_mon, SPECIES_CHARMANDER, 5, 1);
    build_mon(&bs->enemy_mon, enemy_species, enemy_level, 0);

    char buf[80];
    snprintf(buf, sizeof(buf), "Wild %s appeared!", bs->enemy_mon.name);
    set_message(bs, buf);

    /* Render once immediately so the very first battle frame is valid. */
    render_battle_to_layer(bs);
}

/* ------------------------------------------------------------------ */
/* Turn resolution                                                     */
/* ------------------------------------------------------------------ */

static void do_player_attack(BattleState* bs, uint8_t move_id) {
    if (bs->enemy_mon.fainted || bs->player_mon.fainted) return;
    const MoveInfo* mv = get_move(move_id);
    if (mv->is_status) {
        char buf[80];
        snprintf(buf, sizeof(buf), "%s used %s!",
            bs->player_mon.name,
            move_id == MOVE_GROWL ? "GROWL" :
            move_id == MOVE_TAIL_WHIP ? "TAIL WHIP" :
            move_id == MOVE_SAND_ATTACK ? "SAND-ATTACK" : "MOVE");
        set_message(bs, buf);
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }

    int crit = 0;
    int dmg = calc_damage(&bs->player_mon, &bs->enemy_mon, move_id, &crit);

    int roll = GetRandomValue(0, 99);
    if (roll >= mv->accuracy) {
        char buf[80];
        snprintf(buf, sizeof(buf), "%s's attack missed!", bs->player_mon.name);
        set_message(bs, buf);
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }

    if (dmg >= bs->enemy_mon.hp) dmg = bs->enemy_mon.hp;
    bs->enemy_mon.hp -= dmg;

    char buf[80];
    if (crit)
        snprintf(buf, sizeof(buf), "Critical hit! %s took %d damage!",
            bs->enemy_mon.name, dmg);
    else
        snprintf(buf, sizeof(buf), "%s took %d damage!",
            bs->enemy_mon.name, dmg);
    set_message(bs, buf);
    bs->phase = BATTLE_PHASE_MESSAGE;
}

static void do_enemy_attack(BattleState* bs) {
    if (bs->enemy_mon.fainted || bs->player_mon.fainted) return;

    uint8_t chosen = MOVE_NONE;
    for (int i = 0; i < BATTLE_NUM_MOVES; i++) {
        uint8_t m = bs->enemy_mon.moves[i];
        if (m && !get_move(m)->is_status) { chosen = m; break; }
    }
    if (!chosen) {
        for (int i = 0; i < BATTLE_NUM_MOVES; i++) {
            if (bs->enemy_mon.moves[i]) { chosen = bs->enemy_mon.moves[i]; break; }
        }
    }
    if (!chosen) chosen = MOVE_TACKLE;

    const MoveInfo* mv = get_move(chosen);

    if (mv->is_status) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Enemy %s used a move!", bs->enemy_mon.name);
        set_message(bs, buf);
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }

    int roll = GetRandomValue(0, 99);
    if (roll >= mv->accuracy) {
        set_message(bs, "Enemy's attack missed!");
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }

    int crit = 0;
    int dmg = calc_damage(&bs->enemy_mon, &bs->player_mon, chosen, &crit);
    if (dmg >= bs->player_mon.hp) dmg = bs->player_mon.hp;
    bs->player_mon.hp -= dmg;

    char buf[80];
    if (crit)
        snprintf(buf, sizeof(buf), "Critical hit! %s took %d damage!",
            bs->player_mon.name, dmg);
    else
        snprintf(buf, sizeof(buf), "%s took %d damage!",
            bs->player_mon.name, dmg);
    set_message(bs, buf);
    bs->phase = BATTLE_PHASE_MESSAGE;
}

static void finish_turn(BattleState* bs) {
    if (bs->enemy_mon.hp == 0) {
        bs->enemy_mon.fainted = 1;
        char buf[80];
        snprintf(buf, sizeof(buf), "Wild %s fainted!", bs->enemy_mon.name);
        set_message(bs, buf);
        bs->battle_result = 1;
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }
    if (bs->player_mon.hp == 0) {
        bs->player_mon.fainted = 1;
        set_message(bs, "Your Pokemon fainted!");
        bs->battle_result = 2;
        bs->phase = BATTLE_PHASE_MESSAGE;
        return;
    }
    bs->phase = BATTLE_PHASE_MENU;
    bs->menu_cursor = 0;
}

/* ------------------------------------------------------------------ */
/* Input helper                                                        */
/* ------------------------------------------------------------------ */

static int read_menu_nav(int count, int* cursor) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        *cursor = (*cursor + 1) % count;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        *cursor = (*cursor - 1 + count) % count;
    }
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE);
}

/* ------------------------------------------------------------------ */
/* Main update                                                         */
/* ------------------------------------------------------------------ */

void battle_update(BattleState* bs, Player* player) {
    (void)player;
    if (!bs->active) return;
    bs->phase_timer++;

    switch (bs->phase) {
    case BATTLE_PHASE_INTRO:
        if (bs->phase_timer >= 60) {
            bs->phase = BATTLE_PHASE_MENU;
            bs->phase_timer = 0;
            bs->menu_cursor = 0;
        }
        break;

    case BATTLE_PHASE_MENU: {
        if (read_menu_nav(BATTLE_MENU_COUNT, &bs->menu_cursor)) {
            BattleMenuItem sel = (BattleMenuItem)bs->menu_cursor;
            if (sel == BATTLE_MENU_FIGHT) {
                bs->phase = BATTLE_PHASE_MOVE_SELECT;
                bs->move_cursor = 0;
                bs->phase_timer = 0;
            }
            else if (sel == BATTLE_MENU_RUN) {
                set_message(bs, "Got away safely!");
                bs->battle_result = 3;
                bs->phase = BATTLE_PHASE_MESSAGE;
                bs->phase_timer = 0;
            }
            else {
                set_message(bs, "Not available yet!");
                bs->phase = BATTLE_PHASE_MESSAGE;
                bs->phase_timer = 0;
            }
        }
        break;
    }

    case BATTLE_PHASE_MOVE_SELECT: {
        int num_moves = 0;
        for (int i = 0; i < BATTLE_NUM_MOVES; i++)
            if (bs->player_mon.moves[i]) num_moves++;
        if (num_moves == 0) num_moves = 1;

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X)) {
            bs->phase = BATTLE_PHASE_MENU;
            break;
        }
        if (read_menu_nav(num_moves, &bs->move_cursor)) {
            uint8_t chosen = bs->player_mon.moves[bs->move_cursor];
            if (!chosen) chosen = MOVE_TACKLE;

            const MoveInfo* pm = get_move(chosen);
            const MoveInfo* em = get_move(bs->enemy_mon.moves[0]);
            int player_first;
            if (pm->priority != em->priority)
                player_first = pm->priority > em->priority;
            else if (bs->player_mon.speed != bs->enemy_mon.speed)
                player_first = bs->player_mon.speed > bs->enemy_mon.speed;
            else
                player_first = (GetRandomValue(0, 1) == 0);

            bs->player_went_first = player_first;
            bs->turn_executed = 0;

            if (player_first) {
                do_player_attack(bs, chosen);
            }
            else {
                do_enemy_attack(bs);
            }
            bs->move_cursor = (chosen & 0xFF);
        }
        break;
    }

    case BATTLE_PHASE_MESSAGE: {
        if (bs->phase_timer < 30) break;
        bs->phase_timer = 0;

        if (bs->battle_result != 0) {
            bs->phase = BATTLE_PHASE_END;
            break;
        }

        if (!bs->turn_executed) {
            bs->turn_executed = 1;
            uint8_t chosen = (uint8_t)bs->move_cursor;
            if (bs->player_went_first) {
                if (bs->enemy_mon.hp == 0) { finish_turn(bs); break; }
                do_enemy_attack(bs);
            }
            else {
                if (bs->player_mon.hp == 0) { finish_turn(bs); break; }
                do_player_attack(bs, chosen);
            }
            break;
        }

        finish_turn(bs);
        break;
    }

    case BATTLE_PHASE_END:
        bs->return_to_overworld = 1;
        bs->active = 0;
        break;

    default:
        break;
    }

    if (bs->enemy_mon.hp == 0 && bs->battle_result == 0) {
        bs->enemy_mon.fainted = 1;
        char buf[80];
        snprintf(buf, sizeof(buf), "Wild %s fainted!", bs->enemy_mon.name);
        set_message(bs, buf);
        bs->battle_result = 1;
        bs->phase = BATTLE_PHASE_MESSAGE;
        bs->phase_timer = 0;
    }
    else if (bs->player_mon.hp == 0 && bs->battle_result == 0) {
        bs->player_mon.fainted = 1;
        set_message(bs, "Your Pokemon fainted!");
        bs->battle_result = 2;
        bs->phase = BATTLE_PHASE_MESSAGE;
        bs->phase_timer = 0;
    }

    /* Re-render the battle layer. This happens inside the logic loop,
     * outside any BeginTextureMode, so nesting is not an issue. */
    if (bs->active) {
        render_battle_to_layer(bs);
    }
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/*                                                                     */
/*  Two-phase rendering to avoid nesting BeginTextureMode:             */
/*                                                                     */
/*   Phase 1 — render_battle_to_layer()                                */
/*     Called from battle_start_wild() and battle_update(), which run  */
/*     in main's logic loop OUTSIDE any BeginTextureMode. Draws all    */
/*     primitives into a dedicated 160x144 render texture.             */
/*                                                                     */
/*   Phase 2 — battle_render()                                         */
/*     Called from main.c INSIDE BeginTextureMode(target). Does a      */
/*     single DrawTexturePro to composite the pre-rendered layer.      */
/* ------------------------------------------------------------------ */

static RenderTexture2D s_battle_layer = { 0 };

static void ensure_battle_layer(void) {
    if (s_battle_layer.id != 0) return;
    s_battle_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(s_battle_layer.texture, TEXTURE_FILTER_POINT);
}

static void draw_hp_bar(int x, int y, int w, int h, uint16_t cur, uint16_t max) {
    DrawRectangle(x, y, w, h, BLACK);
    DrawRectangle(x + 1, y + 1, w - 2, h - 2, (Color) { 80, 80, 80, 255 });
    if (max == 0) return;
    float pct = (float)cur / (float)max;
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    Color c = GREEN;
    if (pct <= 0.2f) c = RED;
    else if (pct <= 0.5f) c = YELLOW;
    int fill = (int)((w - 2) * pct);
    if (fill > 0)
        DrawRectangle(x + 1, y + 1, fill, h - 2, c);
}

static void draw_info_box(const BattleMon* m, int x, int y, int is_player) {
    int w = 78, h = 28;
    DrawRectangle(x, y, w, h, (Color) { 248, 248, 248, 255 });
    DrawRectangleLines(x, y, w, h, BLACK);

    char line[24];
    snprintf(line, sizeof(line), "%s", m->name);
    DrawText(line, x + 4, y + 2, 8, BLACK);

    snprintf(line, sizeof(line), "Lv%d", m->level);
    DrawText(line, x + w - 26, y + 2, 8, BLACK);

    draw_hp_bar(x + 22, y + 14, w - 28, 5, m->hp, m->max_hp);

    snprintf(line, sizeof(line), "%d/%d", m->hp, m->max_hp);
    DrawText(line, x + 4, y + 18, 8, BLACK);

    (void)is_player;
}

static void render_battle_to_layer(const BattleState* bs) {
    if (!bs->active && bs->phase != BATTLE_PHASE_END) return;
    ensure_battle_layer();

    BeginTextureMode(s_battle_layer);
    ClearBackground((Color) { 248, 248, 216, 255 });

    /* Enemy mon (top-right placeholder) */
    DrawRectangle(104, 18, 44, 44, (Color) { 180, 60, 60, 255 });
    DrawRectangleLines(104, 18, 44, 44, BLACK);
    DrawText("ENEMY", 106, 36, 8, WHITE);

    /* Player mon (bottom-left placeholder) */
    DrawRectangle(20, 68, 44, 44, (Color) { 60, 120, 200, 255 });
    DrawRectangleLines(20, 68, 44, 44, BLACK);
    DrawText("YOU", 28, 86, 8, WHITE);

    /* Info boxes */
    draw_info_box(&bs->enemy_mon, 4, 4, 0);
    draw_info_box(&bs->player_mon, 78, 78, 1);

    /* Message box at the bottom */
    DrawRectangle(0, 110, GB_WIDTH, GB_HEIGHT - 110, WHITE);
    DrawRectangleLines(0, 110, GB_WIDTH, GB_HEIGHT - 110, BLACK);

    if (bs->phase == BATTLE_PHASE_MENU) {
        int mx = 96, my = 114;
        DrawText("FIGHT", mx, my, 8, bs->menu_cursor == 0 ? RED : BLACK);
        DrawText("ITEM", mx, my + 8, 8, bs->menu_cursor == 1 ? RED : BLACK);
        DrawText("PKMN", mx, my + 16, 8, bs->menu_cursor == 2 ? RED : BLACK);
        DrawText("RUN", mx, my + 24, 8, bs->menu_cursor == 3 ? RED : BLACK);
        DrawText("What will", 4, 114, 8, BLACK);
        DrawText("you do?", 4, 122, 8, BLACK);
    }
    else if (bs->phase == BATTLE_PHASE_MOVE_SELECT) {
        int mx = 4, my = 114;
        for (int i = 0; i < BATTLE_NUM_MOVES; i++) {
            uint8_t mid = bs->player_mon.moves[i];
            if (!mid) continue;
            const char* name = "MOVE";
            switch (mid) {
            case MOVE_SCRATCH:      name = "SCRATCH";      break;
            case MOVE_GUST:         name = "GUST";         break;
            case MOVE_SAND_ATTACK:  name = "SAND-ATTACK";  break;
            case MOVE_TACKLE:       name = "TACKLE";       break;
            case MOVE_TAIL_WHIP:    name = "TAIL WHIP";    break;
            case MOVE_GROWL:        name = "GROWL";        break;
            case MOVE_QUICK_ATTACK: name = "QUICK ATTACK"; break;
            default: break;
            }
            DrawText(name, mx, my + i * 8, 8,
                (i == bs->move_cursor) ? RED : BLACK);
        }
        DrawText("X: Back", 110, 114, 8, DARKGRAY);
    }
    else {
        DrawText(bs->message, 4, 116, 8, BLACK);
        DrawText(bs->message, 4, 122, 8, BLACK);
        DrawText(bs->message, 4, 130, 8, BLACK);
    }
    EndTextureMode();
}

/* Composite the pre-rendered battle layer into the currently-bound
 * render target. Called from main.c inside BeginTextureMode(target). */
void battle_render(const BattleState* bs) {
    if (!bs->active && bs->phase != BATTLE_PHASE_END) return;
    ensure_battle_layer();
    Rectangle src = { 0, 0, (float)GB_WIDTH, (float)GB_HEIGHT };
    Rectangle dst = { 0, 0, (float)GB_WIDTH, (float)GB_HEIGHT };
    DrawTexturePro(s_battle_layer.texture, src, dst,
        (Vector2) {
        0, 0
    }, 0.0f, WHITE);
}