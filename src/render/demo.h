/* demo.h - real gameplay in the attract mode (setting `attract_demo`).
 *
 * The cabinet's attract mode shows a plane flying at the player and then
 * backing away the same way. Instead, while the game is in its attract mode
 * and shows the world (not the banner, not the high scores), a second
 * machine plays a game: the same C program, coined and started by itself,
 * flown by an autopilot that steers at the enemy and fires (as the MAME
 * recordings' "aim": tools/mame/rbcapture.lua). Its picture is shown, the
 * real machine's INSERT COINS and CREDITS (or free play's texts) over it.
 *
 * The real machine is not touched: it goes on with its attract mode
 * underneath and takes coins and start as always. The demo machine has no
 * sound, its own EAROM (never saved), and starts a new game when its game
 * ends. It only runs while it is shown.
 */
#ifndef RB_DEMO_H
#define RB_DEMO_H

#include "core/machine.h"
#include "render/subframe.h"

typedef struct rb_demo {
    rb_machine  m;
    rb_subframe sub;
    double      t;              /* its clock: seconds of its interrupts */
    double      t_frame, period;
    int         played;         /* its game has begun */
    /* the autopilot */
    int         frames, sx, sy, n;
    int         px, py;
    int         fire_left;
} rb_demo;

/* A demo machine with the ROMs (3000-7FFF) and the option switches, run up
 * to the start of a game. */
void rb_demo_init(rb_demo *d, const uint8_t *rom3000, uint8_t opt0, uint8_t opt1);

/* One interrupt of it (and its main line), the autopilot at the controls. */
void rb_demo_nmi(rb_demo *d);

/* Its picture of this moment (the time since its last interrupt: ahead,
 * seconds) into scratch, as the program draws the game's (render/subframe.h).
 * Returns where the display list starts. */
uint16_t rb_demo_render(rb_demo *d, double ahead, rb_machine *scratch);

#endif
