#include "horizon_input.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
static uint32_t fbits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}
static uint64_t dbits(double f) {
    uint64_t u;
    memcpy(&u, &f, 8);
    return u;
}
int main(void) {
    HxMouse m;
    HxRotation r = {0};
    hx_mouse_init(&m);
    for (int i = 0; i < 4096; i++) {
        if (i % 64 == 0) {
            hx_mouse_init(&m);
            memset(&r, 0, sizeof(r));
        }
        HxLookOptions o = {(double)(i % 5) / 4., (i & 1) != 0, (i & 2) != 0, (i & 4) != 0, (i & 8) != 0};
        double dx = (double)(i % 41) - 20.25, dy = (double)(i % 37) - 18.5;
        hx_mouse_move(&m, (double)i * 1.25, -(double)i * .75, dx, dy, i % 17 != 0, i % 13 != 0);
        if (i % 3 == 0)
            hx_mouse_add(&m, dx * .5, dy * .25);
        HxD2 turn = {0};
        int valid = hx_mouse_handle(&m, (double)i / 60., i % 17 != 0, i % 13 != 0, o, &turn);
        if (valid)
            hx_turn(&r, turn.x, turn.y);
        HxWheelResult w = hx_mouse_scroll(&m, i % 2 == 0 ? .4 : -.7, i % 5 == 0 ? .7 : .4, i % 7 == 0,
                                          i % 11 == 0 ? .5 : 1.);
        printf("M %d %d %016" PRIx64 " %016" PRIx64 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32
               " %d %d %d %d\n",
               i, valid, dbits(turn.x), dbits(turn.y), fbits(r.pitch), fbits(r.yaw), fbits(r.old_pitch),
               fbits(r.old_yaw), w.valid, w.x, w.y, w.wheel);
    }
    float factors[4] = {0, .2f, .3f, .5f};
    for (int keys = 0; keys < 16; keys++)
        for (int use = 0; use < 4; use++)
            for (int sneak = 0; sneak < 4; sneak++) {
                HxF2 d = hx_keyboard_direction(keys & 1, keys & 2, keys & 4, keys & 8),
                     v = hx_modify_input(d, use != 0, factors[use], sneak != 0, factors[sneak]);
                printf("K %d %d %d %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n", keys, use,
                       sneak, fbits(d.x), fbits(d.y), fbits(v.x), fbits(v.y));
            }
    return 0;
}
