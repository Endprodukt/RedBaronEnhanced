/* subframe.h - the picture between two frames of the game, computed.
 *
 * As in Star Wars Enhanced (smooth = 3). The game moves everything once a
 * frame (10.25 times a second; MAIN up to its label 20$) and then draws
 * (MAIN_VIEW). Here the state each frame draws from is kept (through
 * rb_machine.view_hook), and for any time between two frames the things
 * that move are put where they are on their way: the player's view
 * (heading, height, bank), the planes, drones and the blimp with their
 * angles, the landscape's objects, the shells. The game's own drawing
 * routines then draw the picture of that moment, on a copy of the machine:
 * with the right perspective, and with what comes into view coming into
 * view. The game itself does not notice.
 *
 * At t = 0 the picture is the frame's own display list, word for word.
 *
 * The program shows a frame's list from the moment MAIN switches the
 * buffers until the next switch; the C computes the next frame right
 * after a switch. So both ends of that time are known when it begins, and
 * the pictures between come without delay.
 */
#ifndef RB_SUBFRAME_H
#define RB_SUBFRAME_H

#include "core/machine.h"

typedef struct rb_subframe {
    rb_machine a;       /* the state the frame on the screen was drawn from */
    rb_machine b;       /* and the one the next frame is drawn from */
    int        have;    /* states kept so far (2: a and b are both there) */
    uint32_t   cuts;    /* pictures not computed: the view jumped (a new game, a new life) */
} rb_subframe;

/* For rb_machine.view_hook (user: the rb_subframe). */
void rb_subframe_hook(void *user, const rb_machine *m);

/* Only the state at t, which the picture is drawn from, into s. Returns 0
 * if there is none (a cut). */
int rb_subframe_state(rb_subframe *f, float t, rb_machine *s);

/* The picture at t (0: the frame on the screen, 1: the next one's) drawn
 * into scratch's vector RAM. `live` is the machine now: the propellers the
 * interrupt turns are taken from there. Returns where the list starts
 * (2002 or 2802), or 0 if no picture can be computed (then the frame's own
 * list is to be shown). */
uint16_t rb_subframe_render(rb_subframe *f, const rb_machine *live, float t, rb_machine *scratch);

#endif
