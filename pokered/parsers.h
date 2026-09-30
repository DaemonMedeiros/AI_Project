#ifndef PARSERS_H
#define PARSERS_H

#include "types.h"

void strip_underscores(char* s);
void build_blk_filename(const char* map_const, char* out, size_t out_size);

void parse_map_const(const char* path, const char* map_name, int* w, int* h);
void apply_tileset_alias(char* stem);
int  parse_map_tileset(const char* header_path, char* out_stem, size_t stem_size);
int  parse_border_block(const char* objects_path, uint8_t* out_border);
int  parse_connections(const char* header_path, MapConnection* out, int max);
int  parse_warps(const char* objects_path, WarpEvent* out, int max);
int  parse_objects(const char* objects_path, NPC* out, int max);

uint8_t parse_movement_byte1(const char* s);
uint8_t parse_movement_byte2(const char* s);

void parse_fly_warps(const char* path, FlyWarpTable* out);
int  lookup_fly_warp(const FlyWarpTable* table, const char* map_name, int* out_x, int* out_y);

void build_map_pascal(const char* map_const, char* out, size_t out_size);
void parse_scripts(const char* scripts_path, const char* text_path, TextTable* out);
const char* lookup_text(const struct TextTable* table, const char* symbol);

#define MOVE_WALK        0xFE
#define MOVE_STAY        0xFF
#define MOVE_ANY_DIR     0x00
#define MOVE_UP_DOWN     0x01
#define MOVE_LEFT_RIGHT  0x02
#define MOVE_DIR_DOWN    0xD0
#define MOVE_DIR_UP      0xD1
#define MOVE_DIR_LEFT    0xD2
#define MOVE_DIR_RIGHT   0xD3
#define MOVE_DIR_NONE    0xFF

#endif
