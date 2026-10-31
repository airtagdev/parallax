/* C port of Horizon's input primitives; see third_party/horizon/LICENSE. */
#ifndef PX_HORIZON_INPUT_H
#define PX_HORIZON_INPUT_H
#include <stdint.h>
typedef struct {
    double x, y;
} HxD2;
typedef struct {
    float x, y;
} HxF2;
typedef struct {
    double target, remaining, last;
} HxSmooth;
typedef struct {
    double x, y;
} HxScroll;
typedef struct {
    float pitch, yaw, old_pitch, old_yaw;
} HxRotation;
typedef struct {
    double sensitivity;
    int smooth, scoping, invert_x, invert_y;
} HxLookOptions;
typedef struct {
    double x, y, dx, dy, last_time;
    int ignore_first;
    HxSmooth smooth_x, smooth_y;
    HxScroll scroll;
} HxMouse;
typedef struct {
    int x, y, wheel, valid;
} HxWheelResult;
double hx_signum(double value);
double hx_smooth(HxSmooth *state, double delta, double time);
void hx_mouse_init(HxMouse *mouse);
void hx_mouse_ignore_first(HxMouse *mouse);
void hx_mouse_move(HxMouse *mouse, double x, double y, double dx, double dy, int active, int grabbed);
void hx_mouse_add(HxMouse *mouse, double dx, double dy);
int hx_mouse_handle(HxMouse *mouse, double time, int active, int grabbed, HxLookOptions options, HxD2 *turn);
HxWheelResult hx_scroll(HxScroll *scroll, double x, double y);
HxWheelResult hx_mouse_scroll(HxMouse *mouse, double x, double y, int discrete, double sensitivity);
int32_t hx_next_selection(double wheel, int32_t selected, int32_t limit);
float hx_spectator_speed(float speed, int32_t steps);
void hx_turn(HxRotation *rotation, double x, double y);
double hx_degrees_per_count(HxLookOptions options);
HxD2 hx_motion_for_turn(double yaw, double pitch, HxLookOptions options);
double hx_ease_in_out(double time);
int hx_grab_position(int grab, int was_grabbed, uint32_t width, uint32_t height, float scale, HxF2 *position);
int hx_right_click(int *emulating, int pressed, int control, int enabled);
HxF2 hx_keyboard_direction(int forward, int backward, int left, int right);
HxF2 hx_modify_input(HxF2 input, int using_item, float use_multiplier, int sneaking, float sneak_multiplier);
#endif
