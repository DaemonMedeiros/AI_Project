#include "battle.h"
#include "wild_encounter.h"
#include "gb.h"
#include "render.h"      /* for decode_2bpp_sprite_pixel */
#include "palettes.h"    /* for SGBPalette, sgb_super_palettes,
                            NUM_SGB_PALS, PAL_REDMON, PAL_BLUEMON */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>        /* for sqrt */

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
/* Pokemon sprite loading                                              */
/* ------------------------------------------------------------------ */

static const char* species_sprite_names[] = {
    [SPECIES_NONE] = NULL,
    [SPECIES_BULBASAUR] = "bulbasaur",
    [SPECIES_CHARMANDER] = "charmander",
    [SPECIES_SQUIRTLE] = "squirtle",
    [SPECIES_PIDGEY] = "pidgey",
    [SPECIES_RATTATA] = "rattata",
    /* Add more as you expand species_table */
};

typedef struct {
    Texture2D front;
    Texture2D back;
    uint8_t species;
    int loaded;
} PokemonSprites;

static PokemonSprites s_player_sprites = { 0 };
static PokemonSprites s_enemy_sprites = { 0 };

/* Find the best (w, h) grid for a tile count. Pokered sprites are roughly
 * square, but not guaranteed to be exactly square. We pick the factor
 * pair that minimizes |w - h|. */
static void best_grid_for_tiles(int num_tiles, int* out_w, int* out_h) {
    int best_w = 1, best_h = num_tiles;
    int best_diff = num_tiles;
    for (int w = 1; w <= num_tiles; w++) {
        if (num_tiles % w != 0) continue;
        int h = num_tiles / w;
        int diff = (w > h) ? (w - h) : (h - w);
        if (diff < best_diff) {
            best_diff = diff;
            best_w = w;
            best_h = h;
        }
    }
    *out_w = best_w;
    *out_h = best_h;
}

static Texture2D load_pokemon_sprite(const char* path, int pal_id) {
    Texture2D tex = { 0 };
    FILE* f = fopen(path, "rb");
    if (!f) {
        TraceLog(LOG_WARNING, "Could not open Pokemon sprite: %s", path);
        return tex;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    int num_tiles = (int)(size / 16);
    if (num_tiles < 1) {
        fclose(f);
        TraceLog(LOG_WARNING, "Sprite %s is empty", path);
        return tex;
    }

    int tiles_wide, tiles_high;
    best_grid_for_tiles(num_tiles, &tiles_wide, &tiles_high);

    TraceLog(LOG_INFO, "Sprite %s: %d tiles -> %dx%d grid",
        path, num_tiles, tiles_wide, tiles_high);

    int tex_w = tiles_wide * TILE_SIZE;
    int tex_h = tiles_high * TILE_SIZE;
    Color* pixels = (Color*)calloc((size_t)tex_w * tex_h, sizeof(Color));

    const SGBPalette* sgb = (pal_id >= 0 && pal_id < NUM_SGB_PALS)
        ? &sgb_super_palettes[pal_id] : NULL;

    uint8_t tile_data[16];
    for (int t = 0; t < num_tiles; t++) {
        if (fread(tile_data, 1, 16, f) != 16) break;
        int col = t % tiles_wide;
        int row = t / tiles_wide;
        for (int y = 0; y < 8; y++) {
            uint8_t lo = tile_data[y * 2], hi = tile_data[y * 2 + 1];
            for (int x = 0; x < 8; x++) {
                Color c = decode_2bpp_sprite_pixel(lo, hi, 7 - x,
                    REG_OBP0, sgb);
                pixels[(row * 8 + y) * tex_w + (col * 8 + x)] = c;
            }
        }
    }
    fclose(f);

    Image img = {
        .data = pixels,
        .width = tex_w,
        .height = tex_h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    tex = LoadTextureFromImage(img);
    if (tex.id) SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    free(pixels);
    return tex;
}

static void load_pokemon_sprites(PokemonSprites* out, uint8_t species,
    int pal_id) {
    if (out->loaded) {
        if (out->front.id) UnloadTexture(out->front);
        if (out->back.id)  UnloadTexture(out->back);
        memset(out, 0, sizeof(*out));
    }

    const char* name = NULL;
    if (species < sizeof(species_sprite_names) / sizeof(species_sprite_names[0]))
        name = species_sprite_names[species];
    if (!name) {
        TraceLog(LOG_WARNING, "load_pokemon_sprites: no name for species %d",
            species);
        return;
    }

    char path[256];

    snprintf(path, sizeof(path), "%s/gfx/pokemon/front/%s.2bpp",
        REPO_ROOT, name);
    out->front = load_pokemon_sprite(path, pal_id);
    TraceLog(LOG_INFO, "FRONT %s: id=%u w=%d h=%d",
        path, out->front.id, out->front.width, out->front.height);

    /* Back sprites use the "<name>b.2bpp" convention (e.g. charmanderb.2bpp) */
    snprintf(path, sizeof(path), "%s/gfx/pokemon/back/%sb.2bpp",
        REPO_ROOT, name);
    out->back = load_pokemon_sprite(path, pal_id);
    TraceLog(LOG_INFO, "BACK  %s: id=%u w=%d h=%d",
        path, out->back.id, out->back.width, out->back.height);

    out->species = species;
    out->loaded = 1;
}

void battle_unload_sprites(void) {
    if (s_player_sprites.loaded) {
        if (s_player_sprites.front.id) UnloadTexture(s_player_sprites.front);
        if (s_player_sprites.back.id)  UnloadTexture(s_player_sprites.back);
        memset(&s_player_sprites, 0, sizeof(s_player_sprites));
    }
    if (s_enemy_sprites.loaded) {
        if (s_enemy_sprites.front.id) UnloadTexture(s_enemy_sprites.front);
        if (s_enemy_sprites.back.id)  UnloadTexture(s_enemy_sprites.back);
        memset(&s_enemy_sprites, 0, sizeof(s_enemy_sprites));
    }
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
    m->name[sizeof(m->name) - 1] = 0;
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

    /* Load sprites for both combatants */
    load_pokemon_sprites(&s_player_sprites, bs->player_mon.species, PAL_REDMON);
    load_pokemon_sprites(&s_enemy_sprites, bs->enemy_mon.species, PAL_BLUEMON);

    char buf[80];
    snprintf(buf, sizeof(buf), "Wild %s appeared!", bs->enemy_mon.name);
    set_message(bs, buf);

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
/* Input — edge detection via IsKeyDown so we don't miss presses on   */
/* frames where the 30 Hz logic loop doesn't tick.                     */
/*                                                                     */
/*  is_grid = 1  →  2x2 menu (Fight / Item / PKMN / Run).              */
/*                  Left/Right flip the low bit (0<->1, 2<->3).        */
/*                  Up/Down flip the high bit (0<->2, 1<->3).          */
/*                                                                     */
/*  is_grid = 0  →  vertical list. Up/Down move linearly with wrap.    */
/*                  Left/Right do nothing.                             */
/* ------------------------------------------------------------------ */

static int read_menu_input(BattleState* bs, int count, int is_grid,
    int* cursor, int* cancel_out) {
    int up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    int down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    int left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    int right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    int select = IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE);
    int cancel = IsKeyDown(KEY_ESCAPE) || IsKeyDown(KEY_X);

    int up_edge = up && !bs->prev_up;
    int down_edge = down && !bs->prev_down;
    int left_edge = left && !bs->prev_left;
    int right_edge = right && !bs->prev_right;
    int select_edge = select && !bs->prev_select;
    int cancel_edge = cancel && !bs->prev_cancel;

    if (is_grid) {
        if (up_edge || down_edge)   *cursor ^= 2;
        if (left_edge || right_edge) *cursor ^= 1;
    }
    else {
        if (up_edge)   *cursor = (*cursor - 1 + count) % count;
        if (down_edge) *cursor = (*cursor + 1) % count;
    }

    bs->prev_up = up;
    bs->prev_down = down;
    bs->prev_left = left;
    bs->prev_right = right;
    bs->prev_select = select;
    bs->prev_cancel = cancel;

    if (cancel_out) *cancel_out = cancel_edge;
    return select_edge;
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
        int cancel = 0;
        if (read_menu_input(bs, BATTLE_MENU_COUNT, 1,
            &bs->menu_cursor, &cancel)) {
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

        int cancel = 0;
        int confirm = read_menu_input(bs, num_moves, 0,
            &bs->move_cursor, &cancel);

        if (cancel) {
            bs->phase = BATTLE_PHASE_MENU;
            break;
        }
        if (confirm) {
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
/*     Runs OUTSIDE any BeginTextureMode (in the logic loop). Draws    */
/*     all primitives into a dedicated 160x144 render texture.         */
/*                                                                     */
/*   Phase 2 — battle_render()                                         */
/*     Runs INSIDE BeginTextureMode(target). Does a single             */
/*     DrawTexturePro to composite the pre-rendered layer.             */
/*                                                                     */
/*  Text style: every string is drawn twice — a 1px black drop shadow  */
/*  at (x+1, y+1) and the primary colour at (x, y). This bolds every   */
/*  glyph by one pixel and closely matches the chunky Game Boy font.   */
/*                                                                     */
/*  Font sizing: the menu and message box use BATTLE_FONT_SIZE (10).   */
/*  The info boxes (name / level / HP) use INFO_FONT_SIZE (9) so the   */
/*  longest Gen 1 names (CHARMANDER, CHARMELEON, BLASTOISE) fit inside */
/*  a 96-pixel-wide box without truncation. Names that still don't fit */
/*  shrink once more to size 8 before truncation kicks in.             */
/* ------------------------------------------------------------------ */

static RenderTexture2D s_battle_layer = { 0 };

#define BATTLE_FONT_SIZE 10
#define INFO_FONT_SIZE    9

static void ensure_battle_layer(void) {
    if (s_battle_layer.id == 0) {
        s_battle_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
        SetTextureFilter(s_battle_layer.texture, TEXTURE_FILTER_POINT);
    }
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_POINT);
}

/* Bold draw at an arbitrary font size. Skips the shadow for black text. */
static void draw_text_sized(const char* text, int x, int y, int size, Color color) {
    if (color.r != 0 || color.g != 0 || color.b != 0) {
        DrawText(text, x + 1, y + 1, size, BLACK);
    }
    DrawText(text, x, y, size, color);
}

static void draw_text10(const char* text, int x, int y, Color color) {
    draw_text_sized(text, x, y, BATTLE_FONT_SIZE, color);
}

/* Fits a name into max_width by first trying INFO_FONT_SIZE, then one
 * size smaller, and only truncating as a last resort. */
static void draw_name_fit(const char* name, int x, int y, int max_width, Color c) {
    int sizes[2] = { INFO_FONT_SIZE, INFO_FONT_SIZE - 1 };
    for (int s = 0; s < 2; s++) {
        int sz = sizes[s];
        if (MeasureText(name, sz) + 1 <= max_width) {
            draw_text_sized(name, x, y, sz, c);
            return;
        }
    }
    int sz = INFO_FONT_SIZE - 1;
    char buf[32];
    strncpy(buf, name, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    int len = (int)strlen(buf);
    while (len > 0 && MeasureText(buf, sz) + 1 > max_width) {
        len--;
        buf[len] = 0;
    }
    draw_text_sized(buf, x, y, sz, c);
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

static void draw_info_box(const BattleMon* m, int x, int y) {
    int w = 96, h = 36;
    DrawRectangle(x, y, w, h, (Color) { 248, 248, 248, 255 });
    DrawRectangleLines(x, y, w, h, BLACK);

    /* Level right-aligned using the smaller info font. */
    char lv[8];
    snprintf(lv, sizeof(lv), "Lv%d", m->level);
    int lv_w = MeasureText(lv, INFO_FONT_SIZE) + 1;
    int lv_x = x + w - 3 - lv_w;

    int name_max = lv_x - (x + 3) - 3;
    if (name_max < 20) name_max = 20;
    draw_name_fit(m->name, x + 3, y + 3, name_max, BLACK);
    draw_text_sized(lv, lv_x, y + 3, INFO_FONT_SIZE, BLACK);

    draw_hp_bar(x + 3, y + 17, w - 6, 6, m->hp, m->max_hp);

    char hp[16];
    snprintf(hp, sizeof(hp), "%d/%d", m->hp, m->max_hp);
    draw_text_sized(hp, x + 3, y + 24, INFO_FONT_SIZE, BLACK);
}

static void draw_message(const char* msg, int x, int y) {
    const int max_w = GB_WIDTH - x * 2 - 1;
    if (MeasureText(msg, BATTLE_FONT_SIZE) <= max_w) {
        draw_text10(msg, x, y, BLACK);
        return;
    }

    int len = (int)strlen(msg);
    int split = len;
    for (int i = len - 1; i > 0; i--) {
        if (msg[i] != ' ') continue;
        char buf[128];
        int n = i; if (n > 127) n = 127;
        memcpy(buf, msg, n);
        buf[n] = 0;
        if (MeasureText(buf, BATTLE_FONT_SIZE) <= max_w) { split = i; break; }
    }

    char line1[128] = { 0 };
    char line2[128] = { 0 };
    if (split > 0 && split < len) {
        int n1 = split; if (n1 > 127) n1 = 127;
        memcpy(line1, msg, n1);
        line1[n1] = 0;
        const char* rest = msg + split;
        while (*rest == ' ') rest++;
        strncpy(line2, rest, sizeof(line2) - 1);
    }
    else {
        strncpy(line1, msg, sizeof(line1) - 1);
    }

    draw_text10(line1, x, y, BLACK);
    if (line2[0]) draw_text10(line2, x, y + 12, BLACK);
}

static void render_battle_to_layer(const BattleState* bs) {
    if (!bs->active && bs->phase != BATTLE_PHASE_END) return;
    ensure_battle_layer();

    BeginTextureMode(s_battle_layer);
    ClearBackground((Color) { 248, 248, 216, 255 });

    /* Enemy sprite (front view, top-right) */
    if (s_enemy_sprites.loaded && s_enemy_sprites.front.id != 0) {
        float w = (float)s_enemy_sprites.front.width;
        float h = (float)s_enemy_sprites.front.height;
        float ex = (float)(GB_WIDTH - 8) - w;
        float ey = 8.0f;
        Rectangle src = { 0, 0, w, h };
        Rectangle dst = { ex, ey, w, h };
        DrawTexturePro(s_enemy_sprites.front, src, dst,
            (Vector2) {
            0, 0
        }, 0.0f, WHITE);
    }
    else {
        DrawRectangle(108, 20, 40, 40, (Color) { 180, 60, 60, 255 });
        DrawRectangleLines(108, 20, 40, 40, BLACK);
        draw_text10("ENEMY", 110, 34, WHITE);
    }

    /* Player sprite (back view, bottom-left)
     *
     * NOTE: the placeholder is MAGENTA so you can tell it apart from any
     * other blue draw call that might be covering the sprite. Change back
     * to (Color){ 60, 120, 200, 255 } once everything works. */
    if (s_player_sprites.loaded && s_player_sprites.back.id != 0) {
        float w = (float)s_player_sprites.back.width;
        float h = (float)s_player_sprites.back.height;
        float px = 8.0f;
        float py = 110.0f - h - 4.0f;
        Rectangle src = { 0, 0, w, h };
        Rectangle dst = { px, py, w, h };
        DrawTexturePro(s_player_sprites.back, src, dst,
            (Vector2) {
            0, 0
        }, 0.0f, WHITE);
    }
    else {
        DrawRectangle(20, 60, 40, 40, MAGENTA);
        DrawRectangleLines(20, 60, 40, 40, BLACK);
        draw_text10("YOU", 30, 74, WHITE);
    }

    /* Info boxes — 96 wide, positioned to avoid the sprites */
    draw_info_box(&bs->enemy_mon, 2, 4);
    draw_info_box(&bs->player_mon, 62, 68);

    /* Message box */
    DrawRectangle(0, 110, GB_WIDTH, GB_HEIGHT - 110, WHITE);
    DrawRectangleLines(0, 110, GB_WIDTH, GB_HEIGHT - 110, BLACK);

    if (bs->phase == BATTLE_PHASE_MENU) {
        /* Prompt on the left, 2x2 menu on the right with wide spacing. */
        draw_text10("What will", 4, 114, BLACK);
        draw_text10("you do?", 4, 126, BLACK);

        Color c_fight = (bs->menu_cursor == BATTLE_MENU_FIGHT) ? RED : BLACK;
        Color c_item = (bs->menu_cursor == BATTLE_MENU_ITEM) ? RED : BLACK;
        Color c_pkmn = (bs->menu_cursor == BATTLE_MENU_POKEMON) ? RED : BLACK;
        Color c_run = (bs->menu_cursor == BATTLE_MENU_RUN) ? RED : BLACK;

        draw_text10("FIGHT", 78, 114, c_fight);
        draw_text10("ITEM", 126, 114, c_item);
        draw_text10("PKMN", 78, 126, c_pkmn);
        draw_text10("RUN", 126, 126, c_run);
    }
    else if (bs->phase == BATTLE_PHASE_MOVE_SELECT) {
        int mx = 4, my = 114;
        int row = 0;
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
            draw_text10(name, mx, my + row * 11,
                (i == bs->move_cursor) ? RED : BLACK);
            row++;
        }
        draw_text10("X: Back", 108, 126, DARKGRAY);
    }
    else {
        draw_message(bs->message, 4, 118);
    }

    EndTextureMode();
}

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