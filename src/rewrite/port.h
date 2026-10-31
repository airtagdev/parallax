#ifndef PX_PORT_H
#define PX_PORT_H
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <gl/GL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "json.h"
typedef struct {
    char path[512];
    GLuint texture;
    unsigned char *pixels;
    unsigned width, height;
} Image;
typedef struct {
    Image *image;
    int cols, rows, height, ascent;
} FontAtlas;
typedef struct {
    int atlas, cell, advance;
    unsigned codepoint;
} Glyph;
typedef struct {
    char resources[MAX_PATH];
    char error[512];
    Json language, metadata;
    Image images[128];
    int image_count;
    FontAtlas atlases[8];
    int atlas_count;
    Glyph glyphs[8192];
    int glyph_count;
    int recipes, advancements, visible_advancements, criteria, missing_trigger_definitions;
} Resources;
extern Resources resources;
extern int window_width, window_height, mouse_x, mouse_y, click;
extern HWND main_window;
Image *image_load(const char *path);
int resources_open(const char *directory, int graphical);
void resources_close(void);
const char *translate(const char *key);
int font_width(const char *text);
void font_draw(float x, float y, const char *text, uint32_t color, int shadow);
void image_draw(Image *img, float x, float y, float width, float height, float u, float v, float uw, float vh,
                uint32_t tint);
void fill(float x, float y, float width, float height, uint32_t color);
int title_frame(double time, int frozen);
int verify_resource_catalog(void);
void bmp_capture(const char *path);
#endif
