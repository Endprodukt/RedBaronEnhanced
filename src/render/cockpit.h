/* cockpit.h - the cockpit of the cabinet's bezel, drawn (setting `cockpit`).
 *
 * The cabinet shows the screen through a cockpit printed on its bezel: two
 * machine guns at the sides, a dashboard with three instruments below. Here
 * it is drawn into the picture, and it works:
 *
 *   the guns       kick back and flash at the muzzle as they fire, left and
 *                  right in turn as the game fires them (S.XFLG, D6)
 *   altimeter      left: the player's height (I4YPOS)
 *   airspeed       middle: faster in a dive, slower in a climb (PLDELY)
 *   thermometer    right: the guns' heat (GUN.ST; D6 overheated: blinks red)
 *
 *   the struts     two each side from the middle wing up, leaning out;
 *                  the middle wing at the sides, the bracing wires crossing
 *                  in front, the small windscreen; below, the cockpit is
 *                  closed (the panel's shape reaches out to the corners)
 *   the nose       the cowling's top between the guns, the propeller turning
 *                  in front of it (two blades and the hub; the game's own
 *                  propeller is left out: rb_cockpit_drop_own_prop)
 *   ring sights    on the guns' jackets, with their cross
 *
 * It is seen from the pilot's seat (a small 3D model, the vanishing point in
 * the middle of the screen): the guns run from the lower corners towards
 * the middle. What is nearer hides what is behind it: the guns the
 * dashboard and the struts.
 *
 * Only a picture: it reads the game's state and changes nothing. Its shapes
 * hide the world behind them (render/solid.h, rb_solid.front), as the bezel
 * did; the texts stay in front.
 */
#ifndef RB_COCKPIT_H
#define RB_COCKPIT_H

#include "avg/avg.h"
#include "render/solid.h"

#include <stdint.h>

#define RB_COCKPIT_HULLS 16      /* guns 6, struts 4, the nose, the wings 2, the panel */

/* A convex outline (screen units, counter-clockwise). */
typedef struct rb_cockpit_hull { int n; float x[RB_SOLID_POLY_MAX], y[RB_SOLID_POLY_MAX]; } rb_cockpit_hull;

typedef struct rb_cockpit {
    double last_t;
    float  recoil[2];       /* left, right: 1 just fired, falling to 0 */
    int    side;            /* the gun that fired last (S.XFLG D6) */
    float  speed, alt, heat;    /* the needles, eased */
    int    have;
    rb_cockpit_hull hull[RB_COCKPIT_HULLS];    /* what hides the world, of the last picture */
    int    n_hulls;
} rb_cockpit;

void rb_cockpit_init(rb_cockpit *c);

/* The cockpit's lines for the moment t (the game's clock, seconds), from
 * the game's memory. Returns the lines written (at most max). */
int rb_cockpit_lines(rb_cockpit *c, const uint8_t *mem, double t, rb_line *out, int max);

/* The game's own propeller (three streaks at the bottom, turned by the
 * interrupt) taken out of a picture's lines: the cockpit draws its own.
 * Returns how many are left. */
int rb_cockpit_drop_own_prop(rb_line *l, int n);

/* The shapes that hide what is behind them (for rb_solid.front), as of
 * the last rb_cockpit_lines. Returns how many (at most max). */
int rb_cockpit_polys(const rb_cockpit *c, rb_solid_poly *out, int max);

#endif
