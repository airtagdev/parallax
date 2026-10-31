#ifndef PARALLAX_H
#define PARALLAX_H
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <gl/GL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

#define WORLD_X 256
#define WORLD_Z 256
#define WORLD_Y 96
#define CHUNK 16
#define CHUNKS (WORLD_X / CHUNK)
#define MOB_COUNT 40
#define BLOCK_COUNT 17
#define ACH_COUNT 24
#define PI 3.14159265358979323846
#define WATER_LEVEL 32
#define SAVE_VERSION 2
enum {
    AIR,
    GRASS,
    DIRT,
    STONE,
    SAND,
    LOG,
    LEAVES,
    PLANKS,
    COAL,
    IRON,
    DIAMOND,
    WATER,
    SNOW,
    BRICK,
    GLASS,
    BEDROCK,
    TORCH
};
typedef struct {
    float x, y, z;
} Vec3;
typedef struct {
    float x, y, z, yaw, think, vx, vz;
    int kind, hp;
} Mob;
typedef struct {
    uint32_t seed;
    double time;
    Vec3 pos;
    float yaw, pitch, health, hunger;
    uint32_t inventory[BLOCK_COUNT];
    uint32_t achievements;
    uint32_t broken, placed, walked, killed, crafted;
    float walk_distance;
    int creative;
    char name[48];
    Mob mobs[MOB_COUNT];
} WorldState;
typedef struct {
    const char *title, *description;
} Achievement;
extern unsigned char *blocks;
extern WorldState world;
extern const char *block_names[BLOCK_COUNT];
extern const Achievement achievements[ACH_COUNT];
extern int target[3], adjacent[3], has_target, selected, flying;
extern int keys[256], screen, win_w, win_h, mouse_x, mouse_y, mouse_click;
extern HWND window;
extern char data_dir[MAX_PATH], asset_dir[MAX_PATH], status_text[160];
extern float status_time;
extern unsigned int textures[BLOCK_COUNT], font_base;
extern int meshes_dirty[CHUNKS][CHUNKS], view_distance;
extern unsigned int chunk_lists[CHUNKS][CHUNKS];

enum { MENU, GAME, PAUSE, INVENTORY, ACHIEVEMENTS, CREATE, WORLDS };
unsigned char block_at(int x, int y, int z);
void set_block(int x, int y, int z, int b);
void generate_world(uint32_t seed, const char *name, int creative);
void update_game(float dt);
void update_target(void);
void interact(int place);
void unlock(int id);
void notify(const char *text);
int save_world(int slot);
int load_world(int slot);
int save_info(int slot, WorldState *info);
int self_test(void);
int craft_item(int recipe);
void render_init(void);
void render_shutdown(void);
void render_frame(float dt);
void draw_ui(void);
void rect(float x, float y, float w, float h, float r, float g, float b, float a);
void text_at(float x, float y, const char *text, float r, float g, float b);
int button(float x, float y, float w, float h, const char *label);
void capture_mouse(int capture);
void switch_screen(int next);
void start_world(int slot);
void create_world(int slot, const char *name, uint32_t seed, int creative);
int current_slot(void);
void play_sound(int kind);
uint32_t hash_coords(int x, int y, int z, uint32_t seed);
int solid(int b);
#endif
