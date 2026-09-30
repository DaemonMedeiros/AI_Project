#include "palettes.h"
#include <string.h>

const uint8_t dmg_shade[4] = { 255, 170, 85, 0 };

const SGBPalette sgb_super_palettes[NUM_SGB_PALS] = {
    {{0x7FBF, 0x2F95, 0x7E94, 0x0843}},
    {{0x7FBF, 0x6F99, 0x7F54, 0x0843}},
    {{0x7FBF, 0x0F11, 0x7F54, 0x0843}},
    {{0x7FBF, 0x43D7, 0x7F54, 0x0843}},
    {{0x7FBF, 0x7A91, 0x7F54, 0x0843}},
    {{0x7FBF, 0x6E1B, 0x7F54, 0x0843}},
    {{0x7FBF, 0x025E, 0x7F54, 0x0843}},
    {{0x7FBF, 0x5BF0, 0x7F54, 0x0843}},
    {{0x7FBF, 0x59FF, 0x7F54, 0x0843}},
    {{0x7FBF, 0x195A, 0x7F54, 0x0843}},
    {{0x7FBF, 0x6356, 0x7F54, 0x0843}},
    {{0x7FBF, 0x0F3B, 0x7F54, 0x0843}},
    {{0x7FBF, 0x7F54, 0x2FEA, 0x0843}},
    {{0x7FBF, 0x7FD1, 0x2FEA, 0x1015}},
    {{0x7FBF, 0x7FD1, 0x4B32, 0x3B03}},
    {{0x7FBF, 0x673B, 0x5A6B, 0x0843}},
    {{0x7FBF, 0x6337, 0x3B77, 0x0843}},
    {{0x7FBF, 0x5333, 0x4B4E, 0x0843}},
    {{0x7FBF, 0x7F4A, 0x6A46, 0x0843}},
    {{0x7FBF, 0x5B3E, 0x4F37, 0x0843}},
    {{0x7FBF, 0x6BB8, 0x5B5C, 0x0843}},
    {{0x7FBF, 0x6758, 0x4F49, 0x0843}},
    {{0x7FBF, 0x56F4, 0x4F4A, 0x0843}},
    {{0x7FBF, 0x6BB8, 0x6BD6, 0x0843}},
    {{0x7FBF, 0x7F5E, 0x6A80, 0x0843}},
    {{0x7FBF, 0x6AD7, 0x3F3B, 0x0843}},
    {{0x7FBF, 0x6AD7, 0x6B8A, 0x0843}},
    {{0x7FBF, 0x7F51, 0x6758, 0x0843}},
    {{0x7FBF, 0x5FB1, 0x6758, 0x0843}},
    {{0x7FBF, 0x4F5D, 0x6758, 0x0843}},
    {{0x7FBF, 0x3F39, 0x1843, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x4F4A, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x6A80, 0x0843}},
    {{0x7FBF, 0x7B4B, 0x6A46, 0x0843}},
    {{0x7FBF, 0x6337, 0x4B4E, 0x0843}},
    {{0x7FBF, 0x5B49, 0x5F36, 0x0843}},
    {{0x7FBF, 0x7F5E, 0x5F49, 0x0843}},
};

Color sgb_color_to_rgba(uint16_t c) {
    Color out;
    out.r = (uint8_t)(((c & 0x1F) * 255) / 31);
    out.g = (uint8_t)((((c >> 5) & 0x1F) * 255) / 31);
    out.b = (uint8_t)((((c >> 10) & 0x1F) * 255) / 31);
    out.a = 255;
    return out;
}

int reg_shade(uint8_t reg, int index) { return (reg >> (index * 2)) & 3; }

typedef struct { const char* map_name; SGBPaletteID pals[4]; } MapSGBPalettes;

static const MapSGBPalettes map_sgb_palettes[] = {
    {"PALLETTOWN",     {PAL_PALLET,    PAL_PALLET,    PAL_PALLET,    PAL_PALLET}},
    {"PALLET_TOWN",    {PAL_PALLET,    PAL_PALLET,    PAL_PALLET,    PAL_PALLET}},
    {"VIRIDIANCITY",   {PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN}},
    {"VIRIDIAN_CITY",  {PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN,  PAL_VIRIDIAN}},
    {"PEWTERCITY",     {PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER}},
    {"PEWTER_CITY",    {PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER,    PAL_PEWTER}},
    {"CERULEANCITY",   {PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN}},
    {"CERULEAN_CITY",  {PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN,  PAL_CERULEAN}},
    {"LAVENDERTOWN",   {PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER}},
    {"LAVENDER_TOWN",  {PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER,  PAL_LAVENDER}},
    {"VERMILIONCITY",  {PAL_VERMILION, PAL_VERMILION, PAL_VERMILION, PAL_VERMILION}},
    {"VERMILION_CITY", {PAL_VERMILION, PAL_VERMILION, PAL_VERMILION, PAL_VERMILION}},
    {"CELADONCITY",    {PAL_CELADON,   PAL_CELADON,   PAL_CELADON,   PAL_CELADON}},
    {"CELADON_CITY",   {PAL_CELADON,   PAL_CELADON,   PAL_CELADON,   PAL_CELADON}},
    {"FUCHSIACITY",    {PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA}},
    {"FUCHSIA_CITY",   {PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA,   PAL_FUCHSIA}},
    {"CINNABARISLAND", {PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR}},
    {"CINNABAR_ISLAND",{PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR,  PAL_CINNABAR}},
    {"INDIGOPLATEAU",  {PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO}},
    {"INDIGO_PLATEAU", {PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO,    PAL_INDIGO}},
    {"SAFFRONCITY",    {PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON}},
    {"SAFFRON_CITY",   {PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON,   PAL_SAFFRON}},
    {"ROUTE1",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_1",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE2",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_2",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE3",         {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {"ROUTE_3",        {PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE,     PAL_ROUTE}},
    {NULL, {0, 0, 0, 0}}
};

int lookup_sgb_palette_ids(const char* map_name, int out[4]) {
    for (int i = 0; map_sgb_palettes[i].map_name; i++) {
        if (strcmp(map_sgb_palettes[i].map_name, map_name) == 0) {
            for (int j = 0; j < 4; j++) out[j] = (int)map_sgb_palettes[i].pals[j];
            return 1;
        }
    }
    for (int j = 0; j < 4; j++) out[j] = PAL_ROUTE;
    return 0;
}
