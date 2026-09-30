#include "player.h"
#include "raylib.h"
#include <string.h>
#include "gb.h"
#include "map.h"
#include "data_tables.h"

int read_input_direction(void) {
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) return DIR_UP;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) return DIR_DOWN;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) return DIR_LEFT;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) return DIR_RIGHT;
    return -1;
}

void advance_walk_anim(int* intra_frame, int* anim_frame) {
    (*intra_frame)++;
    if (*intra_frame >= WALK_FRAME_TICKS) {
        *intra_frame = 0;
        *anim_frame = (*anim_frame + 1) & 3;
    }
}

void begin_step(Player* p, Direction d, int nx, int ny, int frames) {
    p->target_x = nx;
    p->target_y = ny;
    p->pixel_offset = 0;
    p->walk_counter = frames;
    p->intra_frame = 0;
    p->state = PSTATE_MOVING;
    p->facing = d;
}

void do_map_transition(ActiveMap* am, Player* p, const MapPaths* paths) {
    MapConnection* conn = check_connection(am, p->tile_x, p->tile_y);
    if (!conn) return;

    char new_map[64];
    strncpy(new_map, conn->map_name, sizeof(new_map) - 1);
    new_map[sizeof(new_map) - 1] = 0;

    int old_tx = p->tile_x;
    int old_ty = p->tile_y;
    Direction d = p->facing;
    int offset = conn->offset;

    if (!load_map(am, new_map, paths->map_const_path, paths->headers_dir,
        paths->objects_dir, paths->maps_dir, paths->tilesets_dir,
        paths->blocksets_dir, paths->collision_path))
        return;

    switch (d) {
    case DIR_UP:    p->tile_y = am->map.height * 2 - 1; p->tile_x = old_tx - offset * 2; break;
    case DIR_DOWN:  p->tile_y = 0;                       p->tile_x = old_tx - offset * 2; break;
    case DIR_LEFT:  p->tile_x = am->map.width * 2 - 1;   p->tile_y = old_ty - offset * 2; break;
    case DIR_RIGHT: p->tile_x = 0;                       p->tile_y = old_ty - offset * 2; break;
    }
    if (p->tile_x < 0) p->tile_x = 0;
    if (p->tile_y < 0) p->tile_y = 0;
    if (p->tile_x >= am->map.width * 2)  p->tile_x = am->map.width * 2 - 1;
    if (p->tile_y >= am->map.height * 2) p->tile_y = am->map.height * 2 - 1;

    p->target_x = p->tile_x;
    p->target_y = p->tile_y;
    p->pixel_offset = 0;
    p->walk_counter = 0;
    p->state = PSTATE_NOT_MOVING;
    p->hop_active = 0;
    p->hop_frames_remaining = 0;
}

void do_warp(ActiveMap* am, Player* p, WarpEvent* warp, const MapPaths* paths) {
    char dest_map[64];
    strncpy(dest_map, warp->dest_map, sizeof(dest_map) - 1);
    dest_map[sizeof(dest_map) - 1] = 0;

    if (strcmp(dest_map, "LASTMAP") == 0) {
        if (am->prev_map[0] == 0) {
            TraceLog(LOG_WARNING, "LASTMAP warp but no previous outdoor map");
            return;
        }
        strncpy(dest_map, am->prev_map, sizeof(dest_map) - 1);
        dest_map[sizeof(dest_map) - 1] = 0;
    }

    int dest_id = warp->dest_warp_id;
    if (!load_map(am, dest_map, paths->map_const_path, paths->headers_dir,
        paths->objects_dir, paths->maps_dir, paths->tilesets_dir,
        paths->blocksets_dir, paths->collision_path))
        return;

    if (dest_id < 0 || dest_id >= am->num_warps) return;
    WarpEvent* dst = &am->warps[dest_id];
    place_player_at_warp(am, p, dst);

    if (dst->warp_type == WARP_TYPE_DOOR) {
        Direction forced = choose_forced_dir(am, p->tile_x, p->tile_y);
        p->forced_move_ticks = WALK_STEP_FRAMES;
        p->target_x = p->tile_x + dir_dx(forced);
        p->target_y = p->tile_y + dir_dy(forced);
        p->pixel_offset = 0;
        p->walk_counter = WALK_STEP_FRAMES;
        p->facing = forced;
        p->state = PSTATE_MOVING;
    }
    p->warp_cooldown = 8;
}
