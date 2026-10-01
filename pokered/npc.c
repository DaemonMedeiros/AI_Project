#include "npc.h"
#include "raylib.h"
#include "gb.h"
#include "map.h"
#include "data_tables.h"
#include "parsers.h"

void update_npc(ActiveMap* am, NPC* n, int idx, Player* player) {
    if (!n->active || n->frozen) return;
    int player_walk_counter = player->walk_counter;

    if (n->movement_status == MSTAT_READY) {
        if (player_walk_counter != 0) return;

        Direction chosen;
        int should_step = 1;

        if (n->move_type == MOVE_STAY) {
            should_step = 0;
            switch (n->move_param) {
            case MOVE_DIR_DOWN:  chosen = DIR_DOWN;  break;
            case MOVE_DIR_UP:    chosen = DIR_UP;    break;
            case MOVE_DIR_LEFT:  chosen = DIR_LEFT;  break;
            case MOVE_DIR_RIGHT: chosen = DIR_RIGHT; break;
            default:             chosen = DIR_DOWN;  break;
            }
        }
        else {
            switch (n->move_param) {
            case MOVE_UP_DOWN:
                chosen = (GetRandomValue(0, 1) == 0) ? DIR_UP : DIR_DOWN; break;
            case MOVE_LEFT_RIGHT:
                chosen = (GetRandomValue(0, 1) == 0) ? DIR_LEFT : DIR_RIGHT; break;
            case MOVE_DIR_DOWN:  chosen = DIR_DOWN;  break;
            case MOVE_DIR_UP:    chosen = DIR_UP;    break;
            case MOVE_DIR_LEFT:  chosen = DIR_LEFT;  break;
            case MOVE_DIR_RIGHT: chosen = DIR_RIGHT; break;
            default:             chosen = (Direction)GetRandomValue(0, 3); break;
            }
        }

        n->facing = chosen;

        if (should_step) {
            int nx = n->tile_x + dir_dx(chosen);
            int ny = n->tile_y + dir_dy(chosen);
            if (can_walk_tile(am, nx, ny) &&
                !npc_occupies(am, idx, nx, ny) &&
                !player_occupies(player, nx, ny)) {
                n->target_x = nx;
                n->target_y = ny;
                n->pixel_offset = 0;
                n->walk_counter = NPC_STEP_FRAMES;
                n->intra_frame = 0;
                n->anim_frame = 0;
                n->movement_status = MSTAT_WALKING;
            }
            else {
                n->movement_delay = GetRandomValue(1, 0x40);
                n->movement_status = MSTAT_DELAYED;
            }
        }
        else {
            n->movement_delay = GetRandomValue(1, 0x40);
            n->movement_status = MSTAT_DELAYED;
        }
    }
    else if (n->movement_status == MSTAT_WALKING) {
        advance_walk_anim(&n->intra_frame, &n->anim_frame);
        n->pixel_offset += 1;
        n->walk_counter--;
        if (n->walk_counter == 0) {
            n->tile_x = n->target_x;
            n->tile_y = n->target_y;
            n->pixel_offset = 0;
            if (n->move_type == MOVE_WALK) {
                n->movement_delay = GetRandomValue(1, 0x40);
                n->movement_status = MSTAT_DELAYED;
            }
            else {
                n->movement_status = MSTAT_READY;
            }
        }
    }
    else if (n->movement_status == MSTAT_DELAYED) {
        n->movement_delay--;
        if (n->movement_delay <= 0) n->movement_status = MSTAT_READY;
    }
}
