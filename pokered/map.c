#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gb.h"
#include "palettes.h"
#include "data_tables.h"
#include "parsers.h"
#include "render.h"

static MapBlk load_blk(const char* path, int w_blocks, int h_blocks) {
    MapBlk m = { 0 };
    m.width = w_blocks;
    m.height = h_blocks;
    m.block_data = (uint8_t*)malloc(w_blocks * h_blocks);
    FILE* f = fopen(path, "rb");
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to open %s", path);
        free(m.block_data);
        m.block_data = NULL;
        return m;
    }
    fread(m.block_data, 1, w_blocks * h_blocks, f);
    fclose(f);
    return m;
}

static BlockDef* load_blockset(const char* path, int* out_count) {
    FILE* f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_count = 0; return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    int count = (int)(size / BLOCK_ENTRY_SIZE);
    BlockDef* blocks = (BlockDef*)malloc(count * sizeof(BlockDef));
    for (int i = 0; i < count; i++) {
        fread(blocks[i].tile_ids, 1, TILES_PER_BLOCK, f);
        fseek(f, BLOCK_ENTRY_SIZE - TILES_PER_BLOCK, SEEK_CUR);
    }
    fclose(f);
    *out_count = count;
    return blocks;
}

static uint8_t* load_collision_list(const char* path, const char* label, int* out_count) {
    FILE* f = fopen(path, "r");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_count = 0; return NULL; }
    uint8_t* list = (uint8_t*)malloc(256);
    int count = 0;
    int in_section = 0;
    int saw_coll_tiles = 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (!in_section) {
            char* p = strstr(line, label);
            if (p && strstr(p, "::")) in_section = 1;
            continue;
        }
        char* p = strstr(line, "coll_tiles");
        if (!p) {
            if (saw_coll_tiles && strstr(line, "::")) break;
            continue;
        }
        saw_coll_tiles = 1;
        p += strlen("coll_tiles");
        int found_any = 0;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == ',') p++;
            if (*p != '$') break;
            p++;
            unsigned int v = 0;
            int digits = 0;
            while (isxdigit((unsigned char)*p)) {
                char c = *p++;
                int d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else d = c - 'A' + 10;
                v = (v << 4) | (unsigned)d;
                digits++;
            }
            if (digits > 0 && count < 256) {
                list[count++] = (uint8_t)v;
                found_any = 1;
            }
        }
        if (!found_any) break;
    }
    fclose(f);
    *out_count = count;
    return list;
}

static void load_connection_map(MapConnection* c, const char* map_const_path,
    const char* maps_dir) {
    int w = 0, h = 0;
    parse_map_const(map_const_path, c->map_name, &w, &h);
    if (w == 0 || h == 0) {
        TraceLog(LOG_WARNING, "Could not parse dimensions for %s", c->map_name);
        return;
    }
    char filename[128];
    build_blk_filename(c->map_name, filename, sizeof(filename));
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.blk", maps_dir, filename);
    c->map = load_blk(path, w, h);
    if (c->map.block_data) c->loaded = 1;
}

static int cell_has_door_tile(ActiveMap* am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef* def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
            if (is_door_tile(am->tileset_stem, def->tile_ids[(qy + dy) * 4 + (qx + dx)]))
                return 1;
    return 0;
}

static int cell_has_warp_tile(ActiveMap* am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef* def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
            if (is_warp_tile(am->tileset_stem, def->tile_ids[(qy + dy) * 4 + (qx + dx)]))
                return 1;
    return 0;
}

static int cell_has_carpet_tile(ActiveMap* am, int cx, int cy, int* out_dir) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0;
    BlockDef* def = &am->blocks[block_id];
    int qx = (cx & 1) * 2, qy = (cy & 1) * 2;
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            uint8_t tid = def->tile_ids[(qy + dy) * 4 + (qx + dx)];
            if (list_has_term(carpet_down, tid)) { if (out_dir) *out_dir = DIR_DOWN;  return 1; }
            if (list_has_term(carpet_up, tid)) { if (out_dir) *out_dir = DIR_UP;    return 1; }
            if (list_has_term(carpet_left, tid)) { if (out_dir) *out_dir = DIR_LEFT;  return 1; }
            if (list_has_term(carpet_right, tid)) { if (out_dir) *out_dir = DIR_RIGHT; return 1; }
        }
    }
    return 0;
}

void free_map_data(ActiveMap* am) {
    if (am->map.block_data) { free(am->map.block_data); am->map.block_data = NULL; }
    for (int i = 0; i < am->num_connections; i++) {
        if (am->connections[i].loaded) {
            free(am->connections[i].map.block_data);
            am->connections[i].map.block_data = NULL;
            am->connections[i].loaded = 0;
        }
    }
    am->num_connections = 0;
    am->num_warps = 0;
    am->num_npcs = 0;
    if (am->blocks) { free(am->blocks); am->blocks = NULL; am->num_blocks = 0; }
    if (am->collision_list) { free(am->collision_list); am->collision_list = NULL; am->num_collision = 0; }
}

void free_map_gpu(ActiveMap* am) {
    if (am->tileset.texture.id) {
        UnloadTexture(am->tileset.texture);
        am->tileset.texture.id = 0;
    }
    if (am->tileset.pixels) {
        free(am->tileset.pixels);
        am->tileset.pixels = NULL;
    }
}

int load_map(ActiveMap* am, const char* map_name,
    const char* map_const_path,
    const char* headers_dir,
    const char* objects_dir,
    const char* maps_dir,
    const char* tilesets_dir,
    const char* blocksets_dir,
    const char* collision_path) {
    int w = 0, h = 0;
    parse_map_const(map_const_path, map_name, &w, &h);
    if (w == 0 || h == 0) {
        TraceLog(LOG_ERROR, "Could not parse dimensions for %s", map_name);
        return 0;
    }

    char saved_prev[64];
    saved_prev[0] = 0;
    if (am->name[0] && am->num_connections > 0) {
        strncpy(saved_prev, am->name, sizeof(saved_prev) - 1);
        saved_prev[sizeof(saved_prev) - 1] = 0;
    }
    else if (am->prev_map[0]) {
        strncpy(saved_prev, am->prev_map, sizeof(saved_prev) - 1);
        saved_prev[sizeof(saved_prev) - 1] = 0;
    }

    free_map_gpu(am);
    free_map_data(am);
    memset(am, 0, sizeof(*am));

    if (saved_prev[0]) {
        strncpy(am->prev_map, saved_prev, sizeof(am->prev_map) - 1);
        am->prev_map[sizeof(am->prev_map) - 1] = 0;
    }
    strncpy(am->name, map_name, sizeof(am->name) - 1);
    am->name[sizeof(am->name) - 1] = 0;

    char filename[128];
    build_blk_filename(map_name, filename, sizeof(filename));

    char blk_path[256];
    snprintf(blk_path, sizeof(blk_path), "%s/%s.blk", maps_dir, filename);
    am->map = load_blk(blk_path, w, h);
    if (!am->map.block_data) {
        TraceLog(LOG_ERROR, "Failed to load map %s from %s", map_name, blk_path);
        return 0;
    }

    char obj_path[256];
    snprintf(obj_path, sizeof(obj_path), "%s/%s.asm", objects_dir, filename);
    uint8_t border = 0x00;
    parse_border_block(obj_path, &border);
    am->border_block = border;

    char hdr_path[256];
    snprintf(hdr_path, sizeof(hdr_path), "%s/%s.asm", headers_dir, filename);
    am->tileset_stem[0] = 0;
    if (!parse_map_tileset(hdr_path, am->tileset_stem, sizeof(am->tileset_stem)))
        strncpy(am->tileset_stem, "overworld", sizeof(am->tileset_stem) - 1);

    am->num_connections = parse_connections(hdr_path, am->connections, MAX_CONNECTIONS);
    for (int i = 0; i < am->num_connections; i++)
        load_connection_map(&am->connections[i], map_const_path, maps_dir);

    if (am->num_connections == 0 && am->prev_map[0])
        lookup_sgb_palette_ids(am->prev_map, am->sgb_pals);
    else
        lookup_sgb_palette_ids(am->name, am->sgb_pals);

    char ts_path[256];
    snprintf(ts_path, sizeof(ts_path), "%s/%s.2bpp", tilesets_dir, am->tileset_stem);
    decode_tileset(ts_path, &am->tileset, REG_BGP, 0, am->sgb_pals[0]);

    char bs_path[256];
    snprintf(bs_path, sizeof(bs_path), "%s/%s.bst", blocksets_dir, am->tileset_stem);
    am->num_blocks = 0;
    am->blocks = load_blockset(bs_path, &am->num_blocks);

    const char* coll_label = lookup_collision_label(am->tileset_stem);
    am->num_collision = 0;
    am->collision_list = load_collision_list(collision_path, coll_label, &am->num_collision);

    am->num_warps = parse_warps(obj_path, am->warps, MAX_WARPS);
    for (int i = 0; i < am->num_warps; i++) {
        am->warps[i].warp_type = WARP_TYPE_NORMAL;
        am->warps[i].warp_dir = -1;
        if (cell_has_door_tile(am, am->warps[i].cell_x, am->warps[i].cell_y)) {
            am->warps[i].warp_type = WARP_TYPE_DOOR;
        }
        else if (cell_has_warp_tile(am, am->warps[i].cell_x, am->warps[i].cell_y)) {
            am->warps[i].warp_type = WARP_TYPE_NORMAL;
        }
        else {
            int dir = -1;
            if (cell_has_carpet_tile(am, am->warps[i].cell_x, am->warps[i].cell_y, &dir)) {
                am->warps[i].warp_type = WARP_TYPE_CARPET;
                am->warps[i].warp_dir = dir;
            }
        }
    }

    am->num_npcs = parse_objects(obj_path, map_name, am->npcs, MAX_OBJECTS);
    for (int i = 0; i < am->num_npcs; i++) {
        NPC* n = &am->npcs[i];
        if (!n->active) continue;
        if (n->tile_x < 0 || n->tile_y < 0 ||
            n->tile_x >= am->map.width * 2 ||
            n->tile_y >= am->map.height * 2)
            n->active = 0;
    }

    for (int i = 0; i < am->num_npcs; i++) {
        am->npcs[i].global_object_id = -1;
    }

    for (int i = 0; i < toggle_entries_count; i++) {
        if (strcmp(toggle_entries[i].map_name, am->name) != 0) continue;

        int idx = toggle_entries[i].object_index;
        if (idx < 0 || idx >= am->num_npcs) continue;

        am->npcs[idx].global_object_id = i;

        if (!toggle_entries[i].initially_on) {
            hide_object(i);
        }
    }

    am->num_bg_events = parse_bg_events(obj_path, am->bg_events, MAX_BG_EVENTS);
    
    char map_pascal[64];
    build_map_pascal(map_name, map_pascal, sizeof(map_pascal));

    char script_path[256];
    snprintf(script_path, sizeof(script_path), "%s/scripts/%s.asm",
        REPO_ROOT, map_pascal);

    char text_path[256];
    snprintf(text_path, sizeof(text_path), "%s/text/%s.asm",
        REPO_ROOT, map_pascal);

    parse_scripts(script_path, text_path, &am->texts);
    TraceLog(LOG_INFO, "  Loaded %d text entries", am->texts.count);

    TraceLog(LOG_INFO, "Loaded map %s (%dx%d) tileset=%s blocks=%d collision=%d conns=%d warps=%d npcs=%d prev=%s",
        map_name, w, h, am->tileset_stem, am->num_blocks, am->num_collision,
        am->num_connections, am->num_warps, am->num_npcs, am->prev_map);
    return 1;
}

int is_tile_passable(uint8_t tile_id, uint8_t* collision_list, int num_collision) {
    if (tile_id == 0x03) return 1;
    for (int i = 0; i < num_collision; i++)
        if (collision_list[i] == tile_id) return 1;
    return 0;
}

uint8_t tile_in_front_of_cell(ActiveMap* am, int cx, int cy) {
    int bx = cx / 2, by = cy / 2;
    if (bx < 0 || bx >= am->map.width || by < 0 || by >= am->map.height) return 0xFF;
    uint8_t block_id = am->map.block_data[by * am->map.width + bx];
    if (block_id >= am->num_blocks) return 0xFF;
    BlockDef* def = &am->blocks[block_id];
    return def->tile_ids[((cy & 1) * 2 + 1) * 4 + (cx & 1) * 2];
}

int can_walk_tile(ActiveMap* am, int nx, int ny) {
    if (IsKeyDown(KEY_C)) {
        if (nx < 0 || ny < 0) return 0;
        if (nx >= am->map.width * 2 || ny >= am->map.height * 2) return 0;
        return 1;
    }
    if (nx < 0 || ny < 0) return 0;
    if (nx >= am->map.width * 2 || ny >= am->map.height * 2) return 0;
    uint8_t tile_id = tile_in_front_of_cell(am, nx, ny);
    if (tile_id == 0xFF) return 0;
    return is_tile_passable(tile_id, am->collision_list, am->num_collision);
}

int npc_occupies(ActiveMap* am, int skip, int nx, int ny) {
    for (int i = 0; i < am->num_npcs; i++) {
        if (i == skip) continue;
        NPC* n = &am->npcs[i];
        if (!n->active) continue;
        if (n->global_object_id >= 0 && is_object_hidden(n->global_object_id))
            continue;
        if (n->tile_x == nx && n->tile_y == ny) return 1;
        if (n->movement_status == MSTAT_WALKING &&
            n->target_x == nx && n->target_y == ny) return 1;
    }
    return 0;
}

int player_occupies(Player* p, int nx, int ny) {
    if (p->tile_x == nx && p->tile_y == ny) return 1;
    if (p->state == PSTATE_MOVING &&
        p->target_x == nx && p->target_y == ny) return 1;
    return 0;
}

int is_ledge_tile(ActiveMap* am, int standing_x, int standing_y,
    Direction d, int nx, int ny) {
    uint8_t standing_tile = tile_in_front_of_cell(am, standing_x, standing_y);
    uint8_t target_tile = tile_in_front_of_cell(am, nx, ny);
    for (int i = 0; i < (int)(sizeof(ledge_tiles) / sizeof(ledge_tiles[0])); i++) {
        if (ledge_tiles[i].dir == d &&
            ledge_tiles[i].standing_tile == standing_tile &&
            ledge_tiles[i].ledge_tile == target_tile)
            return 1;
    }
    return 0;
}

int find_warp_index(ActiveMap* am, int cx, int cy) {
    for (int i = 0; i < am->num_warps; i++)
        if (am->warps[i].cell_x == cx && am->warps[i].cell_y == cy)
            return i;
    return -1;
}

WarpEvent* find_warp(ActiveMap* am, int cx, int cy) {
    int i = find_warp_index(am, cx, cy);
    return (i >= 0) ? &am->warps[i] : NULL;
}

MapConnection* check_connection(ActiveMap* am, int nx, int ny) {
    if (nx >= 0 && ny >= 0 &&
        nx < am->map.width * 2 && ny < am->map.height * 2) return NULL;

    ConnDir needed;
    if (ny < 0)                        needed = CONN_NORTH;
    else if (ny >= am->map.height * 2) needed = CONN_SOUTH;
    else if (nx < 0)                   needed = CONN_WEST;
    else                               needed = CONN_EAST;

    for (int i = 0; i < am->num_connections; i++)
        if (am->connections[i].dir == needed && am->connections[i].loaded)
            return &am->connections[i];
    return NULL;
}

void place_player_at_warp(ActiveMap* am, Player* p, WarpEvent* dst) {
    int ex = dst->cell_x;
    int ey = dst->cell_y;
    if (ex < 0) ex = 0;
    if (ey < 0) ey = 0;
    if (ex >= am->map.width * 2)  ex = am->map.width * 2 - 1;
    if (ey >= am->map.height * 2) ey = am->map.height * 2 - 1;
    p->tile_x = ex;
    p->tile_y = ey;
    p->target_x = ex;
    p->target_y = ey;
    p->pixel_offset = 0;
    p->walk_counter = 0;
    p->state = PSTATE_NOT_MOVING;
    p->hop_active = 0;
    p->hop_frames_remaining = 0;
}

Direction choose_forced_dir(ActiveMap* am, int tx, int ty) {
    if (can_walk_tile(am, tx, ty + 1)) return DIR_DOWN;
    if (can_walk_tile(am, tx, ty - 1)) return DIR_UP;
    if (can_walk_tile(am, tx + 1, ty)) return DIR_RIGHT;
    if (can_walk_tile(am, tx - 1, ty)) return DIR_LEFT;
    return DIR_DOWN;
}

NPC* npc_at(ActiveMap* am, int nx, int ny) {
    for (int i = 0; i < am->num_npcs; i++) {
        NPC* n = &am->npcs[i];
        if (!n->active) continue;
        if (n->global_object_id >= 0 && is_object_hidden(n->global_object_id))
            continue;
        if (n->tile_x == nx && n->tile_y == ny) return n;
    }
    return NULL;
}
