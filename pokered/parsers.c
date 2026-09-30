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
        /* Text ID is a symbol like TEXT_PALLETTOWN_OAK. Strip the "TEXT_" prefix
           so what's left is a stable string, since the actual numeric ID depends
           on the map's text pointer table. */
        n->text_id = 0;
        if (strncmp(text_id, "TEXT_", 5) == 0) {
            /* We can't resolve the symbol to a number without the map's
               text_pointers table, but we can hash it into a stable int so
               different NPCs get different IDs. */
            unsigned hash = 0;
            for (const char* s = text_id; *s; s++)
                hash = hash * 31u + (unsigned char)*s;
            n->text_id = (int)(hash & 0x7FFFFFFF);
        }

        strncpy(n->text_symbol, text_id, sizeof(n->text_symbol) - 1);
        n->text_symbol[sizeof(n->text_symbol) - 1] = 0;
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

void parse_fly_warps(const char* path, FlyWarpTable* out) {
    out->count = 0;
    FILE* f = fopen(path, "r");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); return; }
    char line[256];
    while (fgets(line, sizeof(line), f) && out->count < MAX_FLY_WARPS) {
        char* p = strstr(line, "fly_warp");
        if (!p) continue;
        p += strlen("fly_warp");

        int x = 0, y = 0;
        char name[64] = { 0 };
        if (sscanf(p, " %63[^,], %d, %d", name, &x, &y) != 3) continue;

        char* s = name;
        while (*s == ' ' || *s == '\t') s++;
        char* e = s + strlen(s) - 1;
        while (e > s && (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n')) *e-- = 0;
        strip_underscores(s);

        strncpy(out->entries[out->count].map_name, s,
            sizeof(out->entries[0].map_name) - 1);
        out->entries[out->count].map_name[sizeof(out->entries[0].map_name) - 1] = 0;
        out->entries[out->count].x = x;
        out->entries[out->count].y = y;
        out->count++;
    }
    fclose(f);
}

int lookup_fly_warp(const FlyWarpTable* table, const char* map_name,
    int* out_x, int* out_y) {
    char target[64];
    strncpy(target, map_name, sizeof(target) - 1);
    target[sizeof(target) - 1] = 0;
    strip_underscores(target);

    for (int i = 0; i < table->count; i++) {
        char candidate[64];
        strncpy(candidate, table->entries[i].map_name, sizeof(candidate) - 1);
        candidate[sizeof(candidate) - 1] = 0;
        strip_underscores(candidate);
        if (strcmp(candidate, target) == 0) {
            *out_x = table->entries[i].x;
            *out_y = table->entries[i].y;
            return 1;
        }
    }
    return 0;
}

void build_map_pascal(const char* map_const, char* out, size_t out_size) {
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

/* Strip the surrounding quotes, convert # to newline, stop at @, skip
   <...> substitutions. */
   /* Emit a single-byte sentinel for each control token we care about.
      Anything else is dropped. */
static void emit_token(const char* tok, size_t toklen,
    char* out, size_t out_size, size_t* used) {
#define EMIT(b) do { if (*used + 1 < out_size) out[(*used)++] = (char)(b); } while (0)
    if (toklen == 2 && tok[0] == 'P' && tok[1] == 'K') { EMIT(0x01); return; } /* <PK> */
    if (toklen == 2 && tok[0] == 'M' && tok[1] == 'N') { EMIT(0x02); return; } /* <MN> */
    if (toklen == 6 && memcmp(tok, "PLAYER", 6) == 0) { EMIT(0x03); return; }
    if (toklen == 5 && memcmp(tok, "RIVAL", 5) == 0) { EMIT(0x04); return; }
    if (toklen == 4 && memcmp(tok, "LINE", 4) == 0) { EMIT('\n'); return; }
    if (toklen == 4 && memcmp(tok, "PARA", 4) == 0) { EMIT(0x08); return; }
    if (toklen == 4 && memcmp(tok, "CONT", 4) == 0) { EMIT(0x06); return; }
    if (toklen == 4 && memcmp(tok, "NEXT", 4) == 0) { EMIT('\n'); return; }
    if (toklen == 4 && memcmp(tok, "PAGE", 4) == 0) { EMIT(0x05); return; } /* page break */
    if (toklen == 4 && memcmp(tok, "PKMN", 4) == 0) { EMIT(0x01); EMIT(0x02); return; }
    if (toklen == 2 && tok[0] == 'P' && tok[1] == 'K') { EMIT(0x01); return; }
    if (toklen == 2 && tok[0] == 'M' && tok[1] == 'N') { EMIT(0x02); return; }
    /* Everything else (<DONE>, <TARGET>, <USER>, <PC>, <TM>, <TRAINER>,
       <ROCKET>, <DEXEND>, <SCROLL>, <_CONT>, <BOLD_*>, <LV>, <ID>, ...)
       is silently dropped. */
#undef EMIT
}

static void extract_text_line(const char* line, char* out, size_t out_size,
    size_t* used) {
    const char* p = strchr(line, '"');
    if (!p) return;
    p++;
    while (*p && *p != '"' && *used + 1 < out_size) {
        if (*p == '@') return;
        if (*p == '#') { out[(*used)++] = 0x07; p++; continue; }
        if (*p == '<') {
            const char* start = ++p;
            while (*p && *p != '>') p++;
            size_t toklen = (size_t)(p - start);
            if (*p == '>') p++;
            emit_token(start, toklen, out, out_size, used);
            continue;
        }
        out[(*used)++] = *p++;
    }
}

void parse_scripts(const char* scripts_path, const char* text_path,
    TextTable* out) {
    out->count = 0;
    FILE* f = fopen(scripts_path, "r");
    if (!f) {
        TraceLog(LOG_WARNING, "Could not open script file %s", scripts_path);
        return;
    }

    char line[1024];
    int in_text_pointers = 0;

    while (fgets(line, sizeof(line), f) && out->count < MAX_TEXT_ENTRIES) {
        if (!in_text_pointers) {
            /* Match either `Foo_TextPointers:` or `Foo_TextPointers::`. */
            if (strstr(line, "_TextPointers:")) in_text_pointers = 1;
            continue;
        }

        /* Skip the def_text_pointers macro line and blank lines. */
        if (strstr(line, "def_text_pointers")) continue;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line[0] != ' ' && line[0] != '\t') break;   /* next top-level label */

        char* p = strstr(line, "dw_const");
        if (!p) continue;
        p += strlen("dw_const");
        while (*p == ' ' || *p == '\t') p++;

        char label[64] = { 0 };
        char sym[64] = { 0 };
        if (sscanf(p, " %63[^,], %63[^,\r\n]", label, sym) != 2) continue;

        char* s = label;
        while (*s == ' ' || *s == '\t') s++;
        char* e = s + strlen(s) - 1;
        while (e > s && (*e == ' ' || *e == '\t')) *e-- = 0;

        s = sym;
        while (*s == ' ' || *s == '\t') s++;
        e = s + strlen(s) - 1;
        while (e > s && (*e == ' ' || *e == '\t')) *e-- = 0;

        TextEntry* te = &out->entries[out->count];
        memset(te, 0, sizeof(*te));
        strncpy(te->label, label, sizeof(te->label) - 1);
        strncpy(te->symbol, sym, sizeof(te->symbol) - 1);
        out->count++;
    }
    fclose(f);

    /* For each entry, find the label body in the script file, extract the
       text_far symbol, then read the body from the text file. */
    char far_symbol[64] = { 0 };
    for (int i = 0; i < out->count; i++) {
        TextEntry* te = &out->entries[i];
        far_symbol[0] = 0;

        FILE* sf = fopen(scripts_path, "r");
        if (!sf) continue;
        char buf[1024];
        int found_label = 0;
        while (fgets(buf, sizeof(buf), sf)) {
            if (!found_label) {
                char needle[80];
                snprintf(needle, sizeof(needle), "%s:", te->label);
                char* p = strstr(buf, needle);
                if (p && (p == buf || p[-1] == '\n' || p[-1] == '\r' ||
                    p[-1] == ' ' || p[-1] == '\t')) {
                    found_label = 1;
                }
                continue;
            }
            char* p = strstr(buf, "text_far");
            if (p) {
                p += strlen("text_far");
                while (*p == ' ' || *p == '\t') p++;
                int j = 0;
                while (*p && *p != ' ' && *p != '\t' && *p != '\r' &&
                    *p != '\n' && j < (int)sizeof(far_symbol) - 1) {
                    far_symbol[j++] = *p++;
                }
                far_symbol[j] = 0;
                break;
            }
            if (buf[0] != ' ' && buf[0] != '\t' && buf[0] != '\n' &&
                buf[0] != '\r' && strstr(buf, "::")) {
                break;
            }
        }
        fclose(sf);

        if (far_symbol[0] == 0) continue;

        /* Now read the text body from the text file. The symbol in the text
           file is the far_symbol with a trailing colon. */
        FILE* tf = fopen(text_path, "r");
        if (!tf) continue;
        char tbuf[1024];
        int in_body = 0;
        size_t used = 0;
        while (fgets(tbuf, sizeof(tbuf), tf)) {
            if (!in_body) {
                char needle[80];
                snprintf(needle, sizeof(needle), "%s:", far_symbol);
                char* p = strstr(tbuf, needle);
                if (p && (p == tbuf || p[-1] == '\n' || p[-1] == '\r')) {
                    in_body = 1;
                }
                continue;
            }
            const char* t = tbuf;
            while (*t == ' ' || *t == '\t') t++;
            int is_text = (strncmp(t, "text ", 5) == 0);
            int is_line = (strncmp(t, "line ", 5) == 0);
            int is_para = (strncmp(t, "para ", 5) == 0);
            int is_cont = (strncmp(t, "cont ", 5) == 0);

            if (is_text || is_line || is_para || is_cont) {
                if (used > 0) {
                    if (is_line || is_cont) {
                        if (used + 1 < sizeof(te->text)) te->text[used++] = '\n';
                    }
                    else if (is_para) {
                        if (used + 1 < sizeof(te->text)) te->text[used++] = 0x08;
                    }
                }
                extract_text_line(tbuf, te->text, sizeof(te->text), &used);
            }
            else if (strstr(tbuf, "done") || strstr(tbuf, "text_end")) {
                break;
            }
            else if (tbuf[0] != ' ' && tbuf[0] != '\t' &&
                tbuf[0] != '\n' && tbuf[0] != '\r') {
                break;
            }
        }
        fclose(tf);
    }
}

/* Returns the number of pages needed to display `text` in a 2-line box.
   A page ends at a \n that would push us past 2 rows, at a <PAGE> (0x05),
   or at <CONT> (0x06), whichever comes first. The trailing partial page
   always counts. */
int count_text_pages(const char* text) {
    int pages = 0;
    int lines_this_page = 0;
    int any_content_this_page = 0;

    for (const char* s = text; *s; s++) {
        unsigned char ch = (unsigned char)*s;

        if (ch == 0x05 || ch == 0x06 || ch == 0x08) {
            if (any_content_this_page) { pages++; any_content_this_page = 0; }
            lines_this_page = 0;
            continue;
        }
        if (ch == '\n') {
            lines_this_page++;
            if (lines_this_page >= 2) {
                pages++;
                lines_this_page = 0;
                any_content_this_page = 0;
            }
            continue;
        }
        any_content_this_page = 1;
    }
    if (any_content_this_page) pages++;
    if (pages == 0) pages = 1;
    return pages;
}

const char* lookup_text(const TextTable* table, const char* symbol) {
    for (int i = 0; i < table->count; i++)
        if (strcmp(table->entries[i].symbol, symbol) == 0)
            return table->entries[i].text;
    return NULL;
}
