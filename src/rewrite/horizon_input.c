/* Derived from Horizon mouse.rs and input.rs at the revision in horizon.lock.json.
 * Copyright (c) 2026 Ludovic Marie and the Horizon contributors. MIT License.
 * C translation: Parallax contributors. No Minecraft Java source is embedded.
 */
#include "horizon_input.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
double hx_signum(double v) {
    return v == 0.0 || isnan(v) ? v : copysign(1.0, v);
}
static int32_t java_int(double v) {
    if (isnan(v))
        return 0;
    if (v >= INT32_MAX)
        return INT32_MAX;
    if (v <= INT32_MIN)
        return INT32_MIN;
    return (int32_t)v;
}
static float clamp(float v, float min, float max) {
    if (v < min)
        return min;
    if (isnan(v) || isnan(max))
        return NAN;
    return v < max ? v : max;
}
double hx_smooth(HxSmooth *s, double target_delta, double time) {
    s->target += target_delta;
    double delta = s->target - s->remaining;
    double next = s->last + 0.5 * (delta - s->last), sign = hx_signum(delta);
    if (sign * delta > sign * s->last)
        delta = next;
    s->last = next;
    s->remaining += delta * time;
    return delta * time;
}
void hx_mouse_init(HxMouse *m) {
    memset(m, 0, sizeof(*m));
    m->ignore_first = 1;
    m->last_time = DBL_TRUE_MIN;
}
void hx_mouse_ignore_first(HxMouse *m) {
    m->ignore_first = 1;
}
void hx_mouse_move(HxMouse *m, double x, double y, double dx, double dy, int active, int grabbed) {
    if (m->ignore_first) {
        m->x = x;
        m->y = y;
        m->ignore_first = 0;
    } else {
        if (active) {
            m->dx += grabbed ? dx : x - m->x;
            m->dy += grabbed ? dy : y - m->y;
        }
        m->x = x;
        m->y = y;
    }
}
void hx_mouse_add(HxMouse *m, double dx, double dy) {
    m->dx += dx;
    m->dy += dy;
}
static void scales(double sensitivity, double *mod, double *sens) {
    double ss = sensitivity * (double)0.6f + (double)0.2f;
    *mod = ss * ss * ss;
    *sens = *mod * 8.0;
}
int hx_mouse_handle(HxMouse *m, double time, int active, int grabbed, HxLookOptions o, HxD2 *turn) {
    double elapsed = time - m->last_time;
    m->last_time = time;
    int valid = active && grabbed;
    if (valid) {
        double mod, sens, x, y;
        scales(o.sensitivity, &mod, &sens);
        if (o.smooth) {
            x = hx_smooth(&m->smooth_x, m->dx * sens, elapsed * sens);
            y = hx_smooth(&m->smooth_y, m->dy * sens, elapsed * sens);
        } else {
            memset(&m->smooth_x, 0, sizeof(m->smooth_x));
            memset(&m->smooth_y, 0, sizeof(m->smooth_y));
            x = m->dx * (o.scoping ? mod : sens);
            y = m->dy * (o.scoping ? mod : sens);
        }
        turn->x = o.invert_x ? -x : x;
        turn->y = o.invert_y ? -y : y;
    }
    m->dx = 0;
    m->dy = 0;
    return valid;
}
HxWheelResult hx_scroll(HxScroll *s, double x, double y) {
    if (s->x != 0 && hx_signum(x) != hx_signum(s->x))
        s->x = 0;
    if (s->y != 0 && hx_signum(y) != hx_signum(s->y))
        s->y = 0;
    s->x += x;
    s->y += y;
    int32_t wx = java_int(s->x), wy = java_int(s->y);
    if (wx == 0 && wy == 0)
        return (HxWheelResult){0};
    s->x -= (double)wx;
    s->y -= (double)wy;
    uint32_t neg_bits = 0u - (uint32_t)wx;
    int32_t neg_x;
    memcpy(&neg_x, &neg_bits, sizeof(neg_x));
    return (HxWheelResult){wx, wy, wy == 0 ? neg_x : wy, 1};
}
HxWheelResult hx_mouse_scroll(HxMouse *m, double x, double y, int discrete, double sensitivity) {
    return hx_scroll(&m->scroll, (discrete ? hx_signum(x) : x) * sensitivity,
                     (discrete ? hx_signum(y) : y) * sensitivity);
}
int32_t hx_next_selection(double wheel, int32_t selected, int32_t limit) {
    assert(limit > 0);
    if (limit <= 0)
        return selected;
    uint32_t bits = (uint32_t)selected - (uint32_t)java_int(hx_signum(wheel));
    int32_t current;
    memcpy(&current, &bits, 4);
    if (current < -1)
        current = -1;
    while (current < 0)
        current += limit;
    while (current >= limit)
        current -= limit;
    return current;
}
float hx_spectator_speed(float speed, int32_t steps) {
    return clamp(speed + (float)steps * .005f, 0, .2f);
}
static void set_pitch(HxRotation *r, float value) {
    if (isfinite(value))
        r->pitch = clamp(fmodf(value, 360.f), -90.f, 90.f);
}
void hx_turn(HxRotation *r, double x, double y) {
    float dx = (float)y * .15f, dy = (float)x * .15f;
    set_pitch(r, r->pitch + dx);
    float yaw = r->yaw + dy;
    if (isfinite(yaw))
        r->yaw = yaw;
    set_pitch(r, clamp(r->pitch, -90.f, 90.f));
    r->old_pitch += dx;
    r->old_yaw += dy;
    r->old_pitch = clamp(r->old_pitch, -90.f, 90.f);
}
double hx_degrees_per_count(HxLookOptions o) {
    double mod, sens;
    scales(o.sensitivity, &mod, &sens);
    return .15 * (o.scoping && !o.smooth ? mod : sens);
}
HxD2 hx_motion_for_turn(double yaw, double pitch, HxLookOptions o) {
    double per = hx_degrees_per_count(o);
    return (HxD2){yaw / per * (o.invert_x ? -1. : 1.), pitch / per * (o.invert_y ? -1. : 1.)};
}
double hx_ease_in_out(double t) {
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;
    return .5 - .5 * cos(3.14159265358979323846 * t);
}
int hx_grab_position(int grab, int was, uint32_t width, uint32_t height, float scale, HxF2 *p) {
    if (grab == was)
        return 0;
    p->x = (float)width / 2.f / scale;
    p->y = (float)height / 2.f / scale;
    return 1;
}
int hx_right_click(int *emulating, int pressed, int control, int enabled) {
    if (pressed)
        *emulating = enabled && control;
    int right = *emulating;
    if (!pressed)
        *emulating = 0;
    return right;
}
static float length(HxF2 v) {
    float squared = v.x * v.x + v.y * v.y;
    return (float)sqrt((double)squared);
}
static HxF2 scale(HxF2 v, float f) {
    return (HxF2){v.x * f, v.y * f};
}
static float impulse(int positive, int negative) {
    return !!positive == !!negative ? 0.f : positive ? 1.f : -1.f;
}
HxF2 hx_keyboard_direction(int forward, int backward, int left, int right) {
    HxF2 v = {impulse(left, right), impulse(forward, backward)};
    float mag = length(v);
    return mag < 1.e-4f ? (HxF2){0, 0} : (HxF2){v.x / mag, v.y / mag};
}
HxF2 hx_modify_input(HxF2 input, int using_item, float use_multiplier, int sneaking, float sneak_multiplier) {
    if (input.x * input.x + input.y * input.y == 0.f)
        return input;
    HxF2 v = scale(input, .98f);
    if (using_item)
        v = scale(v, use_multiplier);
    if (sneaking)
        v = scale(v, sneak_multiplier);
    float mag = length(v);
    if (mag <= 0)
        return v;
    HxF2 unit = scale(v, 1.f / mag);
    float x = fabsf(unit.x), y = fabsf(unit.y), ratio = y > x ? x / y : y / x;
    float distance = (float)sqrt((double)(1.f + ratio * ratio)), speed = mag * distance;
    return scale(unit, isnan(speed) || speed < 1.f ? speed : 1.f);
}
