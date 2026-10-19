/* fx.h - explosions a little nicer (setting `explosions`).
 *
 * The game's explosions are its planes' pieces flying apart (PLNEXP), the
 * drones' (DPCTBL) and a circle for what is hit on the ground (EXCRCL), all
 * at ten frames a second. Where pieces of an explosion show up in a picture
 * (lines of the model PIECES or the tag EXPLOSION, none in the picture
 * before), a fireball is added round their middle: a flash, then a ring of
 * fire growing and cooling from white over orange to red, and sparks flying
 * out and falling, for 0.7 seconds. Only a picture.
 */
#ifndef RB_FX_H
#define RB_FX_H

#include "avg/avg.h"

#define RB_FX_MAX 4

typedef struct rb_fx_blast {
    double t0;              /* when it began (the game's clock), < 0: none */
    float  x, y, size;      /* its middle and size on the screen */
    unsigned seed;
} rb_fx_blast;

typedef struct rb_fx {
    rb_fx_blast blast[RB_FX_MAX];
    int         had;        /* the last picture had pieces */
} rb_fx;

void rb_fx_init(rb_fx *f);

/* Look at a picture's lines (in, n) at the moment t, and append the
 * fireballs' lines to out (at most max). Returns how many. */
int rb_fx_lines(rb_fx *f, const rb_line *in, int n, double t, rb_line *out, int max);

/* The enemy's roundels (British: a blue ring, a red middle) on the top
 * wing of every whole biplane in the picture, where its wing's corners are
 * (DB.PLN points 12, 13, 29, 30, as DECODE notes them), lying in the wing:
 * near each tip. Appended to out (at most max); returns how many. */
int rb_fx_roundels(const rb_line *in, int n, rb_line *out, int max);

#endif
