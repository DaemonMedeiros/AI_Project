#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gb.h"
#include "types.h"
#include "palettes.h"
#include "data_tables.h"
#include "render.h"
#include "map.h"
#include "player.h"
#include "npc.h"
#include "fade.h"
#include "wild_encounter.h"   /* ADDED */
#include "battle.h"           /* ADDED */

static const char* priority_shader_fs =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D bgTex;\n"
"uniform sampler2D sprTex;\n"
"uniform sampler2D prioTex;\n"
"uniform vec3 bgColor;\n"
"out vec4 finalColor;\n"
"void main() {\n"
"    vec4 bg  = texture(bgTex,  fragTexCoord);\n"
"    vec4 spr = texture(sprTex, fragTexCoord);\n"
"    float bg_idx  = floor(mod(bg.b  * 255.0 + 0.5, 4.0));\n"
"    float spr_idx = floor(mod(spr.b * 255.0 + 0.5, 4.0));\n"
"    float prio    = texture(prioTex, fragTexCoord).r;\n"
"    vec3 out_rgb;\n"
"    if (spr_idx < 0.5) {\n"
"        out_rgb = (bg_idx < 0.5) ? bgColor : bg.rgb;\n"
"    } else if (prio < 0.5 || bg_idx < 0.5) {\n"
"        out_rgb = spr.rgb;\n"
"    } else {\n"
"        out_rgb = bg.rgb;\n"
"    }\n"
"    finalColor = vec4(out_rgb, 1.0);\n"
"}\n";

static const char* prio_write_fs =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform float isPrio;\n"
"out vec4 finalColor;\n"
"void main() {\n"
"    vec4 t = texture(texture0, fragTexCoord) * fragColor;\n"
"    if (t.a < 0.5) discard;\n"
"    finalColor = vec4(isPrio, isPrio, isPrio, 1.0);\n"
"}\n";

static uint8_t* load_file(const char* path, int* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) { TraceLog(LOG_ERROR, "Failed to open %s", path); *out_size = 0; return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* data = (uint8_t*)malloc(size);
    if (fread(data, 1, size, f) != (size_t)size)
        TraceLog(LOG_WARNING, "Short read on %s", path);
    fclose(f);
    *out_size = (int)size;
    return data;
}

int main(void) {
    const MapPaths paths = {
        .map_const_path = REPO_ROOT "/constants/map_constants.asm",
        .headers_dir = REPO_ROOT "/data/maps/headers",
        .objects_dir = REPO_ROOT "/data/maps/objects",
        .maps_dir = REPO_ROOT "/maps",
        .tilesets_dir = REPO_ROOT "/gfx/tilesets",
        .blocksets_dir = REPO_ROOT "/gfx/blocksets",
        .collision_path = REPO_ROOT "/data/tilesets/collision_tile_ids.asm",
    };
    const char* flower_frame_1 = REPO_ROOT "/gfx/tilesets/flower/flower1.2bpp";
    const char* flower_frame_2 = REPO_ROOT "/gfx/tilesets/flower/flower2.2bpp";
    const char* flower_frame_3 = REPO_ROOT "/gfx/tilesets/flower/flower3.2bpp";
    const char* player_sprite_path = REPO_ROOT "/gfx/sprites/red.2bpp";
    const char* shadow_path = REPO_ROOT "/gfx/overworld/shadow.1bpp";

    InitWindow(GB_WIDTH * SCALE, GB_HEIGHT * SCALE, "Pokered");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D bg_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(bg_layer.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D sprite_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(sprite_layer.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D priority_layer = LoadRenderTexture(GB_WIDTH, GB_HEIGHT);
    SetTextureFilter(priority_layer.texture, TEXTURE_FILTER_POINT);

    Image white_img = GenImageColor(1, 1, WHITE);
    Texture2D white_tex = LoadTextureFromImage(white_img);
    UnloadImage(white_img);

    Shader priority_shader = LoadShaderFromMemory(NULL, priority_shader_fs);
    Shader prio_write_shader = LoadShaderFromMemory(NULL, prio_write_fs);
    int loc_bg_tex = GetShaderLocation(priority_shader, "bgTex");
    int loc_spr_tex = GetShaderLocation(priority_shader, "sprTex");
    int loc_prio_tex = GetShaderLocation(priority_shader, "prioTex");
    int loc_bg_color = GetShaderLocation(priority_shader, "bgColor");
    int loc_pw_isprio = GetShaderLocation(prio_write_shader, "isPrio");

    ActiveMap current = { 0 };
    if (!load_map(&current, "PALLET_TOWN", paths.map_const_path,
        paths.headers_dir, paths.objects_dir, paths.maps_dir,
        paths.tilesets_dir, paths.blocksets_dir, paths.collision_path)) {
        UnloadShader(priority_shader);
        UnloadShader(prio_write_shader);
        UnloadTexture(white_tex);
        UnloadRenderTexture(priority_layer);
        UnloadRenderTexture(sprite_layer);
        UnloadRenderTexture(bg_layer);
        UnloadRenderTexture(target);
        CloseWindow();
        return 1;
    }

    Tileset player_ts = { 0 };
    decode_tileset(player_sprite_path, &player_ts, REG_OBP0, 1, current.sgb_pals[0]);

    Tileset npc_ts[NUM_NPC_SPRITES] = { 0 };
    load_npc_sprites(&current, npc_ts, REG_OBP0, current.sgb_pals[0]);

    Texture2D shadow_tex = decode_1bpp(shadow_path);

    int overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);

    TileAnim flower_anim = {
        .type = ANIM_FRAME_SWAP, .target_tile = 0x03,
        .current_frame = 0, .frame_count = 3,
    };
    TileAnim water_anim = {
        .type = ANIM_TILE_ROTATE, .target_tile = 0x14,
    };

    int size1 = 0, size2 = 0, size3 = 0;
    uint8_t* frame1 = load_file(flower_frame_1, &size1);
    uint8_t* frame2 = load_file(flower_frame_2, &size2);
    uint8_t* frame3 = load_file(flower_frame_3, &size3);
    if (frame1 && frame2 && frame3 && size1 >= 16 && size2 >= 16 && size3 >= 16) {
        flower_anim.frame_data = (uint8_t*)malloc(48);
        memcpy(flower_anim.frame_data, frame1, 16);
        memcpy(flower_anim.frame_data + 16, frame2, 16);
        memcpy(flower_anim.frame_data + 32, frame3, 16);
        flower_anim.frame_size = 48;
    }
    else {
        flower_anim.type = ANIM_NONE;
    }
    free(frame1); free(frame2); free(frame3);

    water_anim.working_tile = (uint8_t*)malloc(16);
    {
        char water_path[256];
        snprintf(water_path, sizeof(water_path), "%s/overworld.2bpp", paths.tilesets_dir);
        FILE* tf = fopen(water_path, "rb");
        if (tf) {
            fseek(tf, 0x14 * 16, SEEK_SET);
            fread(water_anim.working_tile, 1, 16, tf);
            fclose(tf);
        }
    }

    Player player = { 0 };
    player.tile_x = current.map.width;
    player.tile_y = current.map.height;
    player.target_x = player.tile_x;
    player.target_y = player.tile_y;
    player.state = PSTATE_NOT_MOVING;
    player.facing = DIR_DOWN;

    PaletteFade fade = { 0 };
    fade.current_bgp = REG_BGP;

    /* -------- ADDED: battle + wild encounter state -------- */
    BattleState battle;
    battle_init(&battle);
    wild_encounter_init();
    /* ------------------------------------------------------ */

    int counter1 = 0, counter2 = 0;
    float logic_accumulator = 0.0f;
    float anim_accumulator = 0.0f;
    const float LOGIC_DT = 1.0f / 30.0f;
    const float VBLANK_DT = 1.0f / 60.0f;

    while (!WindowShouldClose()) {
        float frame_time = GetFrameTime();
        if (frame_time > 0.25f) frame_time = 0.25f;
        logic_accumulator += frame_time;
        anim_accumulator += frame_time;

        while (logic_accumulator >= LOGIC_DT) {
            /* -------- ADDED: if a battle is active, tick it and
             *          skip the entire overworld logic -------- */
            if (battle.active) {
                battle_update(&battle, &player);
                if (battle.return_to_overworld) {
                    battle.return_to_overworld = 0;
                    /* 3 safe steps after a battle, like the ASM */
                    grant_battle_cooldown(3);
                }
                logic_accumulator -= LOGIC_DT;
                continue;
            }
            /* ------------------------------------------------- */

            if (player.warp_cooldown > 0) player.warp_cooldown--;

            if (fade.phase != FADE_NONE) {
                fade_tick(&fade, &current, &player, &player_ts, npc_ts,
                    player_sprite_path, &paths);
                overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);
                logic_accumulator -= LOGIC_DT;
                continue;
            }

            for (int i = 0; i < current.num_npcs; i++)
                update_npc(&current, &current.npcs[i], i, &player);

            if (player.forced_move_ticks > 0) {
                advance_walk_anim(&player.intra_frame, &player.anim_frame);
                player.pixel_offset += 2;
                player.walk_counter--;
                player.forced_move_ticks--;
                if (player.walk_counter == 0) {
                    player.tile_x = player.target_x;
                    player.tile_y = player.target_y;
                    player.pixel_offset = 0;
                    player.forced_move_ticks = 0;
                    player.state = PSTATE_NOT_MOVING;
                }
                logic_accumulator -= LOGIC_DT;
                continue;
            }

            if (player.state == PSTATE_MOVING) {
                advance_walk_anim(&player.intra_frame, &player.anim_frame);
                player.pixel_offset += 2;
                if (player.hop_active) {
                    if (player.hop_frames_remaining > 0) player.hop_frames_remaining--;
                    if (player.hop_frames_remaining == 0) player.hop_active = 0;
                }
                player.walk_counter--;
                if (player.walk_counter == 0) {
                    player.tile_x = player.target_x;
                    player.tile_y = player.target_y;
                    player.pixel_offset = 0;
                    player.hop_active = 0;
                    player.hop_frames_remaining = 0;
                    player.state = PSTATE_NOT_MOVING;

                    if (player.warp_cooldown == 0) {
                        int warp_idx = find_warp_index(&current,
                            player.tile_x,
                            player.tile_y);
                        if (warp_idx >= 0 &&
                            current.warps[warp_idx].warp_type != WARP_TYPE_CARPET &&
                            !player.stepped_from_warp) {
                            begin_fade_out(&fade, 1, warp_idx);
                        }
                        else if (check_connection(&current,
                            player.tile_x,
                            player.tile_y)) {
                            do_map_transition(&current, &player, &paths);
                            overworld_anims_active = (strcmp(current.tileset_stem, "overworld") == 0);
                            reload_player_sprite(&player_ts, player_sprite_path,
                                REG_OBP0, current.sgb_pals[0]);
                            load_npc_sprites(&current, npc_ts, REG_OBP0,
                                current.sgb_pals[0]);
                        }
                        else {
                            /* -------- ADDED: wild encounter check -------- */
                            uint8_t wild_species = 0, wild_level = 0;
                            if (try_wild_encounter(&current, &player,
                                &wild_species, &wild_level)) {
                                battle_start_wild(&battle,
                                    wild_species, wild_level);
                            }
                            /* --------------------------------------------- */
                        }
                    }
                }
            }
            else {
                int input = read_input_direction();
                if (input < 0) {
                    player.anim_frame = 0;
                    player.intra_frame = 0;
                    player.state = PSTATE_NOT_MOVING;
                }
                else {
                    Direction d = (Direction)input;
                    int standing_idx = find_warp_index(&current,
                        player.tile_x,
                        player.tile_y);

                    if (standing_idx >= 0 &&
                        current.warps[standing_idx].warp_type == WARP_TYPE_CARPET &&
                        player.warp_cooldown == 0 &&
                        (current.warps[standing_idx].warp_dir == -1 ||
                            current.warps[standing_idx].warp_dir == (int)d)) {
                        begin_fade_out(&fade, 1, standing_idx);
                        logic_accumulator -= LOGIC_DT;
                        continue;
                    }

                    if (d != player.facing) {
                        player.facing = d;
                        player.state = PSTATE_NOT_MOVING;
                    }
                    else {
                        int nx = player.tile_x + dir_dx(d);
                        int ny = player.tile_y + dir_dy(d);
                        WarpEvent* warp = find_warp(&current, nx, ny);
                        MapConnection* conn = check_connection(&current, nx, ny);
                        int lx = nx + dir_dx(d);
                        int ly = ny + dir_dy(d);

                        if (is_ledge_tile(&current, player.tile_x, player.tile_y,
                            d, nx, ny) &&
                            can_walk_tile(&current, lx, ly) &&
                            !npc_occupies(&current, -1, lx, ly)) {
                            player.stepped_from_warp = 0;
                            begin_step(&player, d, lx, ly, HOP_STEP_FRAMES);
                            player.hop_active = 1;
                            player.hop_frames_remaining = HOP_STEP_FRAMES;
                        }
                        else if (warp || conn ||
                            (can_walk_tile(&current, nx, ny) &&
                                !npc_occupies(&current, -1, nx, ny))) {
                            player.stepped_from_warp = (standing_idx >= 0);
                            begin_step(&player, d, nx, ny, WALK_STEP_FRAMES);
                        }
                    }
                }
            }
            logic_accumulator -= LOGIC_DT;
        }

        if (overworld_anims_active && flower_anim.type != ANIM_NONE) {
            while (anim_accumulator >= VBLANK_DT) {
                Tileset* ts = &current.tileset;
                if (!ts->texture.id) break;
                counter1++;
                if (counter1 >= 20) {
                    if (counter1 == 21) {
                        counter1 = 0;
                        int frame_index;
                        int sel = counter2 & 3;
                        if (sel < 2)       frame_index = 0;
                        else if (sel == 2) frame_index = 1;
                        else               frame_index = 2;
                        int tx = (flower_anim.target_tile % ts->tiles_wide) * TILE_SIZE;
                        int ty = (flower_anim.target_tile / ts->tiles_wide) * TILE_SIZE;
                        const uint8_t* fd = flower_anim.frame_data + frame_index * 16;
                        const SGBPalette* sgb = &sgb_super_palettes[current.sgb_pals[0]];
                        for (int y = 0; y < 8; y++) {
                            uint8_t lo = fd[y * 2], hi = fd[y * 2 + 1];
                            for (int x = 0; x < 8; x++)
                                ts->pixels[(ty + y) * ts->tex_w + (tx + x)] =
                                decode_2bpp_bg_pixel(lo, hi, 7 - x, fade.current_bgp, sgb);
                        }
                        UpdateTexture(ts->texture, ts->pixels);
                    }
                    else {
                        counter2 = (counter2 + 1) & 7;
                        int left = (counter2 & 4) != 0;
                        for (int i = 0; i < 16; i++) {
                            uint8_t b = water_anim.working_tile[i];
                            water_anim.working_tile[i] = left
                                ? (uint8_t)((b << 1) | (b >> 7))
                                : (uint8_t)((b >> 1) | (b << 7));
                        }
                        int tx = (water_anim.target_tile % ts->tiles_wide) * TILE_SIZE;
                        int ty = (water_anim.target_tile / ts->tiles_wide) * TILE_SIZE;
                        const uint8_t* fd = water_anim.working_tile;
                        const SGBPalette* sgb = &sgb_super_palettes[current.sgb_pals[0]];
                        for (int y = 0; y < 8; y++) {
                            uint8_t lo = fd[y * 2], hi = fd[y * 2 + 1];
                            for (int x = 0; x < 8; x++)
                                ts->pixels[(ty + y) * ts->tex_w + (tx + x)] =
                                decode_2bpp_bg_pixel(lo, hi, 7 - x, fade.current_bgp, sgb);
                        }
                        UpdateTexture(ts->texture, ts->pixels);
                    }
                }
                anim_accumulator -= VBLANK_DT;
            }
        }
        else {
            anim_accumulator = 0.0f;
        }

        int base_x = player.tile_x * TILE_PIXEL_SIZE;
        int base_y = player.tile_y * TILE_PIXEL_SIZE;
        int interp_x = 0, interp_y = 0;
        if (player.state == PSTATE_MOVING) {
            interp_x = dir_dx(player.facing) * player.pixel_offset;
            interp_y = dir_dy(player.facing) * player.pixel_offset;
        }
        int player_px = base_x + interp_x;
        int player_py = base_y + interp_y;

        int hop_y_offset = 0;
        if (player.hop_active) {
            int t = HOP_STEP_FRAMES - player.hop_frames_remaining;
            hop_y_offset = -((t * (HOP_STEP_FRAMES - t)) * HOP_PEAK_PIXELS) /
                ((HOP_STEP_FRAMES / 2) * (HOP_STEP_FRAMES / 2));
        }

        int cam_x = player_px + 8 - GB_WIDTH / 2;
        int cam_y = player_py + 8 - GB_HEIGHT / 2;

        uint8_t standing_tile = tile_in_front_of_cell(&current, player.tile_x, player.tile_y);
        uint8_t grass_id = grass_tile_for(current.tileset_stem);
        player.grass_priority = (grass_id != 0xFF && standing_tile == grass_id) ? 1 : 0;

        for (int i = 0; i < current.num_npcs; i++) {
            NPC* n = &current.npcs[i];
            if (!n->active) { n->grass_priority = 0; continue; }
            uint8_t st = tile_in_front_of_cell(&current, n->tile_x, n->tile_y);
            n->grass_priority = (grass_id != 0xFF && st == grass_id) ? 1 : 0;
        }

        Tileset* ts = &current.tileset;
        const SGBPalette* sgb = &sgb_super_palettes[current.sgb_pals[0]];
        int sprite_top_y = player_py - cam_y + hop_y_offset;

        BeginTextureMode(bg_layer);
        ClearBackground((Color) { 0, 0, 0, 0 });
        if (ts->texture.id) {
            for (int by = -BORDER_MARGIN; by < current.map.height + BORDER_MARGIN; by++) {
                for (int bx = -BORDER_MARGIN; bx < current.map.width + BORDER_MARGIN; bx++) {
                    uint8_t block_id;
                    if (bx >= 0 && bx < current.map.width && by >= 0 && by < current.map.height) {
                        block_id = current.map.block_data[by * current.map.width + bx];
                    }
                    else {
                        block_id = current.border_block;
                        for (int ci = 0; ci < current.num_connections; ci++) {
                            MapConnection* c = &current.connections[ci];
                            if (!c->loaded) continue;
                            if (c->dir == CONN_NORTH && by < 0) {
                                int cx = bx - c->offset, cy = c->map.height + by;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                {
                                    block_id = c->map.block_data[cy * c->map.width + cx]; break;
                                }
                            }
                            else if (c->dir == CONN_SOUTH && by >= current.map.height) {
                                int cx = bx - c->offset, cy = by - current.map.height;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                {
                                    block_id = c->map.block_data[cy * c->map.width + cx]; break;
                                }
                            }
                            else if (c->dir == CONN_WEST && bx < 0) {
                                int cx = c->map.width + bx, cy = by - c->offset;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                {
                                    block_id = c->map.block_data[cy * c->map.width + cx]; break;
                                }
                            }
                            else if (c->dir == CONN_EAST && bx >= current.map.width) {
                                int cx = bx - current.map.width, cy = by - c->offset;
                                if (cx >= 0 && cx < c->map.width && cy >= 0 && cy < c->map.height)
                                {
                                    block_id = c->map.block_data[cy * c->map.width + cx]; break;
                                }
                            }
                        }
                    }
                    if (block_id >= current.num_blocks) continue;

                    BlockDef* def = &current.blocks[block_id];
                    for (int ty = 0; ty < 4; ty++) {
                        for (int tx = 0; tx < 4; tx++) {
                            uint8_t tile_id = def->tile_ids[ty * 4 + tx];
                            Rectangle src = {
                                (tile_id % ts->tiles_wide) * TILE_SIZE,
                                (tile_id / ts->tiles_wide) * TILE_SIZE,
                                TILE_SIZE, TILE_SIZE
                            };
                            Vector2 dest = {
                                (bx * BLOCK_PIXEL_SIZE) + (tx * TILE_SIZE) - cam_x,
                                (by * BLOCK_PIXEL_SIZE) + (ty * TILE_SIZE) - cam_y
                            };
                            DrawTextureRec(ts->texture, src, dest, WHITE);
                        }
                    }
                }
            }
        }
        EndTextureMode();

        BeginTextureMode(sprite_layer);
        ClearBackground((Color) { 0, 0, 0, 0 });

        if (player.hop_active)
            draw_shadow(shadow_tex, player_px - cam_x, player_py - cam_y + 8);

        for (int i = 0; i < current.num_npcs; i++) {
            NPC* n = &current.npcs[i];
            if (!n->active) continue;
            if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
            Tileset* nts = &npc_ts[n->sprite_id];
            if (!nts->texture.id) continue;

            int anim_idx = anim_table[n->facing][n->anim_frame];
            const uint8_t* tiles = sprite_frames[anim_idx];
            int flip = anim_flip[n->facing][n->anim_frame];
            int nx_px = n->tile_x * TILE_PIXEL_SIZE + dir_dx(n->facing) * n->pixel_offset;
            int ny_px = n->tile_y * TILE_PIXEL_SIZE + dir_dy(n->facing) * n->pixel_offset;
            draw_sprite(nts, tiles, flip, nx_px - cam_x, ny_px - cam_y);
        }

        if (player_ts.texture.id) {
            int anim_idx = anim_table[player.facing][player.anim_frame];
            const uint8_t* tiles = sprite_frames[anim_idx];
            int flip = anim_flip[player.facing][player.anim_frame];
            draw_sprite(&player_ts, tiles, flip, player_px - cam_x, sprite_top_y);
        }
        EndTextureMode();

        BeginTextureMode(priority_layer);
        ClearBackground((Color) { 0, 0, 0, 255 });

        for (int i = 0; i < current.num_npcs; i++) {
            NPC* n = &current.npcs[i];
            if (!n->active) continue;
            if (n->sprite_id <= 0 || n->sprite_id >= NUM_NPC_SPRITES) continue;
            Tileset* nts = &npc_ts[n->sprite_id];
            if (!nts->texture.id) continue;

            int anim_idx = anim_table[n->facing][n->anim_frame];
            const uint8_t* tiles = sprite_frames[anim_idx];
            int flip = anim_flip[n->facing][n->anim_frame];
            int nx_px = n->tile_x * TILE_PIXEL_SIZE + dir_dx(n->facing) * n->pixel_offset;
            int ny_px = n->tile_y * TILE_PIXEL_SIZE + dir_dy(n->facing) * n->pixel_offset;
            int px = nx_px - cam_x, py = ny_px - cam_y;

            for (int q = 0; q < 4; q++) {
                float isprio = (n->grass_priority && (q & 2)) ? 1.0f : 0.0f;
                BeginShaderMode(prio_write_shader);
                SetShaderValue(prio_write_shader, loc_pw_isprio, &isprio, SHADER_UNIFORM_FLOAT);

                int src_i = flip ? (q ^ 1) : q;
                uint8_t tid = tiles[src_i];
                Rectangle src = sprite_quadrant_src(nts, tid, flip);
                int ox = (q & 1) ? 8 : 0;
                int oy = (q & 2) ? 8 : 0;
                Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
                DrawTexturePro(nts->texture, src, dst, (Vector2) { 0, 0 }, 0.0f, WHITE);
                EndShaderMode();
            }
        }

        if (player_ts.texture.id) {
            int anim_idx = anim_table[player.facing][player.anim_frame];
            const uint8_t* tiles = sprite_frames[anim_idx];
            int flip = anim_flip[player.facing][player.anim_frame];
            int px = player_px - cam_x, py = sprite_top_y;

            for (int q = 0; q < 4; q++) {
                float isprio = (player.grass_priority && (q & 2)) ? 1.0f : 0.0f;
                BeginShaderMode(prio_write_shader);
                SetShaderValue(prio_write_shader, loc_pw_isprio, &isprio, SHADER_UNIFORM_FLOAT);

                int src_i = flip ? (q ^ 1) : q;
                uint8_t tid = tiles[src_i];
                Rectangle src = sprite_quadrant_src(&player_ts, tid, flip);
                int ox = (q & 1) ? 8 : 0;
                int oy = (q & 2) ? 8 : 0;
                Rectangle dst = { (float)(px + ox), (float)(py + oy), TILE_SIZE, TILE_SIZE };
                DrawTexturePro(player_ts.texture, src, dst, (Vector2) { 0, 0 }, 0.0f, WHITE);
                EndShaderMode();
            }
        }
        EndTextureMode();

        int shade_idx = reg_shade(fade.current_bgp, 0);
        Color bg_col = sgb_color_to_rgba(sgb->colors[shade_idx]);
        Vector3 bg_col_f = { bg_col.r / 255.0f, bg_col.g / 255.0f, bg_col.b / 255.0f };
        SetShaderValue(priority_shader, loc_bg_color, &bg_col_f, SHADER_UNIFORM_VEC3);

        BeginTextureMode(target);
        /* -------- ADDED: if a battle is active, render the battle
         *          screen instead of the overworld -------- */
        if (battle.active) {
            battle_render(&battle);
        }
        else if (fade.hide_world) {
            ClearBackground(bg_col);
        }
        else {
            ClearBackground((Color) { 0, 0, 0, 0 });
            BeginShaderMode(priority_shader);
            SetShaderValueTexture(priority_shader, loc_bg_tex, bg_layer.texture);
            SetShaderValueTexture(priority_shader, loc_spr_tex, sprite_layer.texture);
            SetShaderValueTexture(priority_shader, loc_prio_tex, priority_layer.texture);
            DrawTexturePro(white_tex,
                (Rectangle) {
                0, 0, 1, 1
            },
                (Rectangle) {
                0, 0, (float)GB_WIDTH, (float)GB_HEIGHT
            },
                (Vector2) {
                0, 0
            }, 0.0f, WHITE);
            EndShaderMode();
        }
        EndTextureMode();
        /* ------------------------------------------------- */

        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src_rect = { 0, 0, (float)GB_WIDTH, (float)GB_HEIGHT };
        Rectangle dst_rect = { 0, 0, (float)(GB_WIDTH * SCALE), (float)(GB_HEIGHT * SCALE) };
        DrawTexturePro(target.texture, src_rect, dst_rect, (Vector2) { 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    if (player_ts.texture.id) UnloadTexture(player_ts.texture);
    if (player_ts.pixels) free(player_ts.pixels);
    for (int i = 0; i < NUM_NPC_SPRITES; i++) {
        if (npc_ts[i].texture.id) UnloadTexture(npc_ts[i].texture);
        if (npc_ts[i].pixels) free(npc_ts[i].pixels);
    }
    if (shadow_tex.id) UnloadTexture(shadow_tex);
    if (flower_anim.frame_data) free(flower_anim.frame_data);
    if (water_anim.working_tile) free(water_anim.working_tile);
    free_map_gpu(&current);
    free_map_data(&current);

    UnloadShader(priority_shader);
    UnloadShader(prio_write_shader);
    UnloadTexture(white_tex);
    UnloadRenderTexture(priority_layer);
    UnloadRenderTexture(sprite_layer);
    UnloadRenderTexture(bg_layer);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}