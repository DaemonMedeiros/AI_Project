#ifndef GB_H
#define GB_H

#define GB_WIDTH            160
#define GB_HEIGHT           144
#define SCALE               2

#define TILE_SIZE           8
#define TILE_PIXEL_SIZE     16
#define BLOCK_SIZE          4
#define BLOCK_PIXEL_SIZE    (TILE_SIZE * BLOCK_SIZE)
#define TILES_PER_BLOCK     16
#define BLOCK_ENTRY_SIZE    16
#define TILESET_TILES_WIDE  16

#define BORDER_MARGIN       8

#define WALK_FRAME_TICKS    4
#define WALK_STEP_FRAMES    8
#define NPC_STEP_FRAMES     16
#define HOP_STEP_FRAMES     16
#define HOP_PEAK_PIXELS     8

#define FADE_STEP_TICKS     4

#define REG_BGP             0xE4
#define REG_OBP0            0xD0
#define REG_OBP1            0xC0

#define REPO_ROOT           "pokered"

#endif
