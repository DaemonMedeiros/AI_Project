#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "data_tables.h"
#include "palettes.h"

Color decode_2bpp_bg_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
    const SGBPalette* sgb) {
    uint8_t idx = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
    int shade = reg_shade(reg, idx);
    Color c;
    if (sgb) c = sgb_color_to_rgba(sgb->colors[shade]);
    else {
        uint8_t v = dmg_shade[shade];
        c.r = v; c.g = v; c.b = v;
    }
    c.b = (uint8_t)((c.b & 0xFC) | (idx & 0x03));
    c.a = 255;
    return c;
}

Color decode_2bpp_sprite_pixel(uint8_t lo, uint8_t hi, int bit, uint8_t reg,
    const SGBPalette* sgb) {
    uint8_t idx = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
    int shade = reg_shade(reg, idx);
    Color c;
    if (sgb) c = sgb_color_to_rgba(sgb->colors[shade]);
    else {
        uint8_t v = dmg_shade[shade];
        c.r = v; c.g = v; c.b = v;
    }
    c.b = (uint8_t)((c.b & 0xFC) | (idx & 0x03));
    c.a = (idx == 0) ? 0 : 255;
    return c;
}

void decode_tileset(const char* path, Tileset* ts, uint8_t reg,
    int is_sprite, int pal_id) {
    ts->texture.id = 0;
    const SGBPalette* sgb = (pal_id >= 0 && pal_id < NUM_SGB_PALS)
        ? &sgb_super_palettes[pal_id] : NULL;
    FILE* f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    int num_tiles = (int)(size / 16);
    if (num_tiles < 1) num_tiles = 1;

    ts->tiles_wide = is_sprite ? TILESET_TILES_WIDE * 2 : TILESET_TILES_WIDE;
    ts->tiles_high = (num_tiles + TILESET_TILES_WIDE - 1) / TILESET_TILES_WIDE;
    if (ts->tiles_high < 1) ts->tiles_high = 1;
    ts->tex_w = ts->tiles_wide * TILE_SIZE;
    ts->tex_h = ts->tiles_high * TILE_SIZE;
    ts->pixels = (Color*)malloc(ts->tex_w * ts->tex_h * sizeof(Color));
    for (int i = 0; i < ts->tex_w * ts->tex_h; i++)
        ts->pixels[i] = (Color){ 0, 0, 0, 0 };

    uint8_t tile_data[16];
    for (int t = 0; t < num_tiles; t++) {
        if (fread(tile_data, 1, 16, f) != 16) break;
        int bank_col = t % TILESET_TILES_WIDE;
        int bank_row = t / TILESET_TILES_WIDE;
        for (int y = 0; y < 8; y++) {
            uint8_t lo = tile_data[y * 2], hi = tile_data[y * 2 + 1];
            for (int x = 0; x < 8; x++) {
                Color c = is_sprite
                    ? decode_2bpp_sprite_pixel(lo, hi, 7 - x, reg, sgb)
                    : decode_2bpp_bg_pixel(lo, hi, 7 - x, reg, sgb);
                int ax = bank_col * TILE_SIZE + x;
                int ay = bank_row * TILE_SIZE + y;
                ts->pixels[ay * ts->tex_w + ax] = c;
                if (is_sprite) {
                    int mx = (bank_col + TILESET_TILES_WIDE) * TILE_SIZE + (7 - x);
                    int my = bank_row * TILE_SIZE + y;
                    ts->pixels[my * ts->tex_w + mx] = c;
                }
            }
        }
    }
    fclose(f);

    Image img = {
        .data = ts->pixels,
        .width = ts->tex_w,
        .height = ts->tex_h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    ts->texture = LoadTextureFromImage(img);
    if (ts->texture.id) SetTextureFilter(ts->texture, TEXTURE_FILTER_POINT);
}

Texture2D decode_1bpp(const char* path) {
    Texture2D tex = { 0 };
    FILE* f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); return tex; }
    uint8_t data[8];
    if (fread(data, 1, 8, f) != 8) { fclose(f); return tex; }
    fclose(f);

    Color pixels[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int bit = (data[y] >> (7 - x)) & 1;
            Color c = { 0, 0, 0, bit ? 255 : 0 };
            if (bit) c.b = (uint8_t)((c.b & 0xFC) | 3);
            pixels[y * 8 + x] = c;
        }
    }
    Image img = {
        .data = pixels,
        .width = 8, .height = 8, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    tex = LoadTextureFromImage(img);
    if (tex.id) SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

Rectangle sprite_quadrant_src(const Tileset* ts, uint8_t tid, int flip) {
    int physical = (tid >= 0x80) ? (tid - 0x80 + 12) : tid;
    int bank_col = physical % TILESET_TILES_WIDE;
    int bank_row = physical / TILESET_TILES_WIDE;
    int col = flip ? (bank_col + TILESET_TILES_WIDE) : bank_col;
    int row = bank_row;
    (void)ts;
    return (Rectangle) { col* TILE_SIZE, row* TILE_SIZE, TILE_SIZE, TILE_SIZE };
}

void draw_sprite(Tileset* ts, const uint8_t* tiles, int flip, int px, int py) {
    if (!ts->texture.id) return;
    for (int i = 0; i < 4; i++) {
        int src_i = flip ? (i ^ 1) : i;
        uint8_t tid = tiles[src_i];
        Rectangle src = sprite_quadrant_src(ts, tid, flip);
        int ox = (i & 1) ? 8 : 0;
        int oy = (i & 2) ? 8 : 0;
        Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
        DrawTexturePro(ts->texture, src, dst, (Vector2) { 0, 0 }, 0.0f, WHITE);
    }
}

void draw_shadow(Texture2D shadow_tex, int px, int py) {
    if (!shadow_tex.id) return;
    for (int q = 0; q < 4; q++) {
        int ox = (q & 1) ? 8 : 0;
        int oy = (q & 2) ? 8 : 0;
        int flipx = (q & 1) ? 1 : 0;
        int flipy = (q & 2) ? 1 : 0;
        Rectangle src = {
            (float)(flipx ? 8 : 0),
            (float)(flipy ? 8 : 0),
            (float)(flipx ? -8 : 8),
            (float)(flipy ? -8 : 8)
        };
        Rectangle dst = { (float)(px + ox), (float)(py + oy), 8.0f, 8.0f };
        DrawTexturePro(shadow_tex, src, dst, (Vector2) { 0, 0 }, 0.0f, WHITE);
    }
}

void reload_player_sprite(Tileset* player_ts, const char* sprite_path,
    uint8_t reg, int pal_id) {
    if (player_ts->texture.id) {
        UnloadTexture(player_ts->texture);
        player_ts->texture.id = 0;
    }
    if (player_ts->pixels) {
        free(player_ts->pixels);
        player_ts->pixels = NULL;
    }
    memset(player_ts, 0, sizeof(*player_ts));
    decode_tileset(sprite_path, player_ts, reg, 1, pal_id);
}

void load_npc_sprites(ActiveMap* am, Tileset* npc_ts, uint8_t reg, int pal_id) {
    for (int i = 0; i < NUM_NPC_SPRITES; i++) {
        if (npc_ts[i].texture.id) {
            UnloadTexture(npc_ts[i].texture);
            npc_ts[i].texture.id = 0;
        }
        if (npc_ts[i].pixels) {
            free(npc_ts[i].pixels);
            npc_ts[i].pixels = NULL;
        }
        memset(&npc_ts[i], 0, sizeof(npc_ts[i]));
    }
    for (int i = 0; i < am->num_npcs; i++) {
        NPC* n = &am->npcs[i];
        if (!n->active) continue;
        if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
        if (npc_ts[n->sprite_id].texture.id) continue;
        const char* file = npc_sprite_files[n->sprite_id];
        if (!file) continue;
        char path[256];
        snprintf(path, sizeof(path), "%s/%s.2bpp", REPO_ROOT "/gfx/sprites", file);
        decode_tileset(path, &npc_ts[n->sprite_id], reg, 1, pal_id);
    }
}

void apply_palette(ActiveMap* am, Tileset* player_ts, Tileset* npc_ts,
    const char* player_sprite_path,
    uint8_t bgp, uint8_t obp0) {
    char ts_path[256];
    snprintf(ts_path, sizeof(ts_path), "%s/%s.2bpp", REPO_ROOT "/gfx/tilesets",
        am->tileset_stem);
    if (am->tileset.texture.id) {
        UnloadTexture(am->tileset.texture);
        am->tileset.texture.id = 0;
    }
    if (am->tileset.pixels) {
        free(am->tileset.pixels);
        am->tileset.pixels = NULL;
    }
    memset(&am->tileset, 0, sizeof(am->tileset));
    decode_tileset(ts_path, &am->tileset, bgp, 0, am->sgb_pals[0]);

    reload_player_sprite(player_ts, player_sprite_path, obp0, am->sgb_pals[0]);
    load_npc_sprites(am, npc_ts, obp0, am->sgb_pals[0]);
}
