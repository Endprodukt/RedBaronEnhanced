/* controls.h - keys, buttons and sticks as the Red Baron board's controls.
 *
 * The cabinet has an analog stick: two pots the POKEY reads (ALLPOT, one at
 * a time by D0 of the sound latch), and fire (D7 of 1802), start (D6 of
 * 1802, low when pressed) and the coin switches. Here the stick is a
 * place, -1..1 each way, which becomes the pots' readings 64..192 (MAME's
 * range; the program learns the range by itself, or is told it:
 * stick_calibrate).
 *
 * Two ways to fly (rb_config.input_mode; chosen in the test menu):
 *
 *   digital  a joystick of switches or the keys: held, the stick moves out
 *            at digital_rate as far as digital_reach; let go, it comes back
 *            at digital_return (0: at once). So a tap gives a little, a long
 *            push the full bank.
 *   analog   a yoke or analog stick: its place, beyond deadzone, times gain.
 *            Keys and a digital stick still work as above.
 *
 * Every action can be bound to any key, any button, hat or axis of any
 * joystick (rb_config.bind, kept in redbaron.ini). An action left at
 * "auto" is the cabinet's own control by the names of advmame.rc
 * (vp_input):
 *
 *   up down left right       p1_up p1_down p1_left p1_right (PC: the arrow keys)
 *   the stick's axes         p1_stickx, p1_sticky
 *   fire, start, coin        p1_button1, start1, coin1 (coin2: the middle mech)
 *   the test menu            the cabinet's service key: AdvanceMAME's action
 *                            "service" as advmame.rc binds it (the file
 *                            VectorPie's CONTROLS settings write), F2 where it
 *                            does not; a binding adds a second way; start and
 *                            fire held two seconds too (menu_hold)
 */
#ifndef RB_CONTROLS_H
#define RB_CONTROLS_H

#include "core/machine.h"
#include "render/config.h"

#include <stddef.h>
#include <vectorpie/vp_input.h>

#define RB_MAX_JOYSTICKS 8
#define RB_MAX_AXES      16
#define RB_COIN_PULSE_NMIS 25   /* a coin: the switch closed about 100 ms, well inside what MOOLAH takes */

typedef struct rb_bind {
    uint8_t type;           /* what it is bound to (controls.c) */
    uint8_t joy;            /* which joystick, from 0 */
    int16_t index;          /* the scancode, button, axis or hat */
    int8_t  dir;            /* an axis: 1 or -1, the way that counts; a hat: its direction bit */
} rb_bind;

/* Where the controls point for a menu: -1, 0, 1 each way (y: up is -1),
 * and fire as it goes down. */
typedef struct rb_nav {
    int x, y, fire;
} rb_nav;

typedef struct rb_controls {
    int     mode;                       /* RB_INPUT_xxx */
    float   deadzone, gain, rate, reach, ret;
    int     invert_y;
    rb_bind bind[RB_ACT_COUNT];

    /* what the last read found */
    float   x, y;                       /* the stick, -1..1 (right and down positive) */
    float   ax, ay;                     /* the analog axes alone, as read (before dead zone and gain) */
    int     dx, dy;                     /* the digital directions held, -1, 0, 1 */
    int     fire, start, coin;
    uint8_t in4;
    int     coin_pulse[2];              /* interrupts left with the coin switch closed */
    rb_nav  nav;
    int     menu;                       /* the test menu's key went down (the service key, or what is bound to it) */
    int     service_rc;                 /* advmame.rc binds "service": the SDK reads it */
    int     service_key;                /* else this key (SDL scancode): F2 */
    char    service_name[24];           /* its name, for the menu's hints */
    int     fire_was, coin_was, menu_was;
    int     menu_hold;
    uint32_t hold_since;

    void   *joy[RB_MAX_JOYSTICKS];      /* SDL_Joystick */
    int     joys;

    /* taking a new binding from the next thing pressed */
    int     cap_action;                 /* -1: none */
    int     cap_armed;
    int     cap_still;
    int16_t cap_rest[RB_MAX_JOYSTICKS][RB_MAX_AXES];
} rb_controls;

void rb_controls_init(rb_controls *c, const rb_config *cfg);
void rb_controls_close(rb_controls *c);

/* The service key's name ("F2", or what advmame.rc binds), for hints. */
const char *rb_controls_service_name(const rb_controls *c);

/* Take the mode, the sensitivities and the bindings from the settings
 * (again after they have changed). */
void rb_controls_configure(rb_controls *c, const rb_config *cfg, vp_input *in);

/* Once per picture: sample everything; dt: seconds since the last read
 * (the digital stick moves with time). */
void rb_controls_read(rb_controls *c, vp_input *in, double dt);

/* Once per interrupt, before it: present the controls to the board. */
void rb_controls_tick(rb_controls *c, rb_machine *m);

/* A new binding for an action: begin, then once per picture after
 * rb_controls_read. Returns 0 while waiting, 1 with the binding written to
 * spec (rb_config.bind's form), -1 if given up (Esc). For the stick's two
 * axes only an axis counts, pushed to the right (RB_ACT_STICK_X) or
 * down (RB_ACT_STICK_Y). */
void rb_controls_capture_begin(rb_controls *c, int action);
int  rb_controls_capture_poll(rb_controls *c, char *spec, size_t size);

/* For the control test: the number of joysticks, a joystick's name, and
 * its axes (-1..1), buttons and hats. */
int         rb_controls_joysticks(const rb_controls *c);
const char *rb_controls_joy_name(const rb_controls *c, int j);
int         rb_controls_joy_axes(const rb_controls *c, int j, float *out, int max);
int         rb_controls_joy_buttons(const rb_controls *c, int j, uint8_t *out, int max);

#endif
