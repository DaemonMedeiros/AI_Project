#include "parsers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "data_tables.h"
#include "palettes.h"
#include "map.h"

void strip_underscores(char* s) {
    char* d = s;
    while (*s) { if (*s != '_') *d++ = *s; s++; }
    *d = 0;
}

void build_blk_filename(const char* map_const, char* out, size_t out_size) {
    size_t j = 0;
    int at_word_start = 1;
    for (size_t i = 0; map_const[i] && j + 1 < out_size; i++) {
        char ch = map_const[i];
        if (ch == '_') { at_word_start = 1; continue; }
        if (at_word_start) {
            if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
            at_word_start = 0;
        }
        else {
            if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        }
        out[j++] = ch;
    }
    out[j] = 0;
}

void parse_map_const(const char* path, const char* map_name, int* w, int* h) {
    *w = 0; *h = 0;
    char target[64];
    strncpy(target, map_name, sizeof(target) - 1);
    target[sizeof(target) - 1] = 0;
    strip_underscores(target);

    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char name[64]; int width, height;
        if (sscanf(line, " map_const %63[^,], %d, %d", name, &width, &height) == 3) {
            strip_underscores(name);
            if (strcmp(name, target) == 0) { *w = width; *h = height; break; }
        }
    }
    fclose(f);
}

void apply_tileset_alias(char* stem) {
    for (int i = 0; tileset_aliases[i].from; i++) {
        if (strcmp(stem, tileset_aliases[i].from) == 0) {
            strncpy(stem, tileset_aliases[i].to, 63);
            stem[63] = 0;
            return;
        }
    }
}

int parse_map_tileset(const char* header_path, char* out_stem, size_t stem_size) {
    FILE* f = fopen(header_path, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* p = strstr(line, "map_header");
        if (!p) continue;
        p += strlen("map_header");
        char* c1 = strchr(p, ',');
        if (!c1) continue;
        char* c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char* c3 = strchr(c2 + 1, ',');
        if (!c3) c3 = c2 + strlen(c2);

        char* s = c2 + 1;
        while (*s == ' ' || *s == '\t') s++;
        char* e = s;
        while (e < c3 && *e && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n') e++;
        int len = (int)(e - s);
        if (len <= 0 || (size_t)len >= stem_size) continue;
        for (int i = 0; i < len; i++) {
            char ch = s[i];
            if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
            out_stem[i] = ch;
        }
        out_stem[len] = 0;
        while (len > 0 && (out_stem[len - 1] == '\r' || out_stem[len - 1] == '\n' ||
            out_stem[len - 1] == ' ' || out_stem[len - 1] == '\t'))
            out_stem[--len] = 0;
        apply_tileset_alias(out_stem);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

int parse_border_block(const char* objects_path, uint8_t* out_border) {
    FILE* f = fopen(objects_path, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* p = strstr(line, "border block");
        if (!p) continue;
        unsigned int v = 0;
        char* dollar = strchr(line, '$');
        if (dollar && sscanf(dollar, "$%x", &v) == 1) {
            *out_border = (uint8_t)v;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int parse_connections(const char* header_path, MapConnection* out, int max) {
    FILE* f = fopen(header_path, "r");
    if (!f) return 0;
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && count < max) {
        char* p = strstr(line, "connection");
        if (!p) continue;
        p += strlen("connection");
        while (*p == ' ' || *p == '\t') p++;

        ConnDir dir;
        if (strncmp(p, "north", 5) == 0)      dir = CONN_NORTH;
        else if (strncmp(p, "south", 5) == 0) dir = CONN_SOUTH;
        else if (strncmp(p, "west", 4) == 0)  dir = CONN_WEST;
        else if (strncmp(p, "east", 4) == 0)  dir = CONN_EAST;
        else continue;

        char* c1 = strchr(p, ',');
        if (!c1) continue;
        char* c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char* c3 = strchr(c2 + 1, ',');
        if (!c3) continue;

        char* s = c2 + 1;
        while (*s == ' ' || *s == '\t') s++;
        char* e = s;
        while (*e && *e != ',' && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n') e++;
        int len = (int)(e - s);
        if (len <= 0 || len >= 64) continue;
        char map_const[64] = { 0 };
        memcpy(map_const, s, len);
        strip_underscores(map_const);

        s = c3 + 1;
        while (*s == ' ' || *s == '\t') s++;
        int offset = atoi(s);

        out[count].dir = dir;
        strncpy(out[count].map_name, map_const, sizeof(out[count].map_name) - 1);
        out[count].map_name[sizeof(out[count].map_name) - 1] = 0;
        out[count].offset = offset;
        out[count].loaded = 0;
        count++;
    }
    fclose(f);
    return count;
}

int parse_warps(const char* objects_path, WarpEvent* out, int max) {
    FILE* f = fopen(objects_path, "r");
    if (!f) return 0;
    int count = 0;
    int in_section = 0;
    char line[512];
    while (fgets(line, sizeof(line), f) && count < max) {
        if (!in_section) {
            if (strstr(line, "def_warp_events")) in_section = 1;
            continue;
        }
        if (strstr(line, "def_bg_events") ||
            strstr(line, "def_object_events") ||
            strstr(line, "def_warps_to")) break;

        char* p = strstr(line, "warp_event");
        if (!p) continue;
        p += strlen("warp_event");

        int x = 0, y = 0, dest_id = 0;
        char dest_map[64] = { 0 };
        if (sscanf(p, " %d, %d, %63[^,], %d", &x, &y, dest_map, &dest_id) != 4) continue;

        char* s = dest_map;
        while (*s == ' ' || *s == '\t') s++;
        char* e = s + strlen(s) - 1;
        while (e > s && (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n')) *e-- = 0;
        strip_underscores(s);

        out[count].cell_x = x;
        out[count].cell_y = y;
        strncpy(out[count].dest_map, s, sizeof(out[count].dest_map) - 1);
        out[count].dest_map[sizeof(out[count].dest_map) - 1] = 0;
        out[count].dest_warp_id = dest_id - 1;
        out[count].warp_type = WARP_TYPE_NORMAL;
        out[count].warp_dir = -1;
        count++;
    }
    fclose(f);
    return count;
}

int parse_objects(const char* objects_path, NPC* out, int max) {
    FILE* f = fopen(objects_path, "r");
    if (!f) return 0;
    int count = 0;
    int in_section = 0;
    char line[512];
    while (fgets(line, sizeof(line), f) && count < max) {
        if (!in_section) {
            if (strstr(line, "def_object_events")) in_section = 1;
            continue;
        }
        if (strstr(line, "def_warps_to")) break;

        char* p = strstr(line, "object_event");
        if (!p) continue;
        p += strlen("object_event");

        int x = 0, y = 0;
        char sprite_name[32] = { 0 };
        char mv1[16] = { 0 }, mv2[16] = { 0 }, text_id[64] = { 0 };
        if (sscanf(p, " %d, %d, %31[^,], %15[^,], %15[^,], %63[^,\n]",
            &x, &y, sprite_name, mv1, mv2, text_id) != 6) continue;

        for (char* s = sprite_name; *s; s++) if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char* s = mv1; *s; s++)         if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char* s = mv2; *s; s++)         if (*s == ' ' || *s == '\t') { *s = 0; break; }
        for (char* s = text_id; *s; s++)
            if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') { *s = 0; break; }

        NPC* n = &out[count];
        memset(n, 0, sizeof(*n));
        n->sprite_id = sprite_id_from_name(sprite_name);
        n->movement_byte1 = parse_movement_byte1(mv1);
        n->movement_byte2 = parse_movement_byte2(mv2);
        n->active = (n->sprite_id != 0);
        n->tile_x = x;
        n->tile_y = y;
        n->target_x = n->tile_x;
        n->target_y = n->tile_y;
        n->movement_status = MSTAT_READY;
        switch (n->movement_byte2) {
        case MOVE_DIR_DOWN:  n->facing = DIR_DOWN;  break;
        case MOVE_DIR_UP:    n->facing = DIR_UP;    break;
        case MOVE_DIR_LEFT:  n->facing = DIR_LEFT;  break;
        case MOVE_DIR_RIGHT: n->facing = DIR_RIGHT; break;
        default:             n->facing = DIR_DOWN;  break;
        }
        count++;
    }
    fclose(f);
    return count;
}

uint8_t parse_movement_byte1(const char* s) {
    if (strncmp(s, "WALK", 4) == 0) return MOVE_WALK;
    return MOVE_STAY;
}

uint8_t parse_movement_byte2(const char* s) {
    if (strncmp(s, "ANY_DIR", 7) == 0)     return MOVE_ANY_DIR;
    if (strncmp(s, "UP_DOWN", 7) == 0)     return MOVE_UP_DOWN;
    if (strncmp(s, "LEFT_RIGHT", 10) == 0) return MOVE_LEFT_RIGHT;
    if (strncmp(s, "DOWN", 4) == 0)        return MOVE_DIR_DOWN;
    if (strncmp(s, "UP", 2) == 0)          return MOVE_DIR_UP;
    if (strncmp(s, "LEFT", 4) == 0)        return MOVE_DIR_LEFT;
    if (strncmp(s, "RIGHT", 5) == 0)       return MOVE_DIR_RIGHT;
    if (strncmp(s, "NONE", 4) == 0)        return MOVE_DIR_NONE;
    return MOVE_DIR_NONE;
}
