/* solid.h - things you cannot see through (the setting `occlusion`).
 *
 * The game draws wire frames: every line shows, the far wing through the
 * near one, the mountains through the fuselage, the mountain's ridge
 * through the bunker standing on it. Here the lines of a picture are cut
 * where something nearer covers them. Nothing of the game changes; it is
 * done to the picture on its way to the tube. (After Battlezone Enhanced's
 * render/solid.)
 *
 * What covers:
 *   the biplane: its faces by its points in the ROM (DB.PLN): the two
 *     wings, the fuselage (eight sides from the nose to the tail), its
 *     nose, the fin, the tailplane
 *   the objects on the ground (PFLOB): the tent, the tree, the gun, the
 *     bunker, each as the closed outlines of its points
 *   the mountains: the land under each mountain's ridge, down to its foot
 *     (worked out from the landscape's points in the ROM, SSEGS)
 *   the additions (render/deco.h): trees, clouds
 * Where each point is on the screen and how far away, DECODE notes while
 * it draws (rb_note_point; rb_line.p0, p1, w0, w1, model, inst).
 *
 * What is cut: the lines of all these that lie behind one of them, and the
 * horizon (infinitely far). Shells, explosions, the drones, the texts, the
 * score and the player's own propeller are never cut.
 *
 * Besides, shapes in front of everything (the cockpit, render/cockpit.h):
 * they cut the lines of the world and the copyright line below them, even
 * with the faces off.
 */
#ifndef RB_SOLID_H
#define RB_SOLID_H

#include "avg/avg.h"

#define RB_SOLID_POLY_MAX 64        /* corners of a covering shape at most */

typedef struct rb_solid_poly {
    int      n;                     /* its corners */
    float    x[RB_SOLID_POLY_MAX], y[RB_SOLID_POLY_MAX];
    float    a, b, c;               /* 1 / distance across it: a x + b y + c */
    float    wmax;
    float    x0, y0, x1, y1;        /* the box around it */
    uint32_t inst;                  /* the drawing it is a face of */
    uint64_t points;                /* its points, as bits (lines between them are its own edges) */
} rb_solid_poly;

typedef struct rb_solid {
    const uint8_t *mem;             /* the ROMs: the landscape's points */
    rb_solid_poly *polys;
    int            n_polys, cap_polys;
    rb_line       *out;
    int            n_out, cap_out;
    int            cut;             /* of the last run: lines cut or taken away */
    int            no_faces;        /* 1: the world's own faces left out (occlusion off) */
    const rb_solid_poly *front;     /* shapes in front of everything (rb_solid_front), or NULL */
    int            n_front;
} rb_solid;

void rb_solid_free(rb_solid *s);

/* A shape in front of everything, from its corners (screen units). Returns
 * 0 if it covers nothing. */
int rb_solid_front_poly(rb_solid_poly *p, const float *x, const float *y, int n);

/* The whole lines of a picture (rb_avg_run_inst with the tags, notes and
 * drawings), cut. mem: the machine's memory (for the ROM's landscape).
 * Returns how many there are now; *out stays valid until the next call. */
int rb_solid_run(rb_solid *s, const uint8_t *mem, const rb_line *in, int n, const rb_line **out);

#endif
