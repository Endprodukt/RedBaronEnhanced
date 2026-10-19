/* deco.h - trees on the ground, clouds in the sky (settings `trees`,
 * `clouds`).
 *
 * The cabinet's world is a horizon, its mountains and, in the ground
 * sequence, a few objects on them. These additions put trees on the land
 * below the horizon and clouds above it. They are drawn by the game's own
 * routines, as it draws its objects on the ground: POSITP places them by
 * the player's place, height and bank, SETBM leaves out what is off the
 * screen, PROJECT and DECODE draw their outlines scaled by the distance,
 * on a copy of the machine after the frame's picture (the copy the
 * picture between two frames is drawn on, render/subframe.h). So they move,
 * bank and shrink exactly as the mountains do.
 *
 * Where they are: on a grid of the world, each cell with one tree or one
 * cloud a little off its middle (the same every time); the player flies
 * forward through it at a steady speed and turns across it with the game
 * (UNIV4X). The trees' shapes are the game's own tree (PFLOB) and a fir
 * after it; the clouds are long low outlines far behind the mountains.
 */
#ifndef RB_DECO_H
#define RB_DECO_H

#include <stdint.h>

#include "avg/avg.h"
#include "core/machine.h"

typedef struct rb_deco_face { uint8_t n; uint8_t pt[8]; } rb_deco_face;

/* A shape: points as the game keeps those of its objects on the ground
 * (PFPNTS: x/2 and y*2, a byte each), lines as DECODE reads them (point
 * number * 6, plus 1 for a visible line; FF ends), faces for occlusion. */
typedef struct rb_deco_shape {
    const int8_t       *points;     /* x, y, ... */
    int                 n_points;
    const uint8_t      *lines;
    int                 n_lines;
    const rb_deco_face *faces;
    int                 n_faces;
} rb_deco_shape;

/* The shape of a model (RB_OBJ_DECO_xxx), or NULL. */
const rb_deco_shape *rb_deco_shape_of(unsigned model);

/* Draw the trees and clouds into the picture the machine m holds (the
 * display list at start, 2002 or 2802; m is a copy, changed: its other
 * buffer and its game routines' scratch cells). t: the game's clock in
 * seconds (they move with it). Returns how many were drawn. */
int rb_deco_draw(rb_machine *m, uint16_t start, double t, int trees, int clouds);

/* How much of a line of a tree or cloud shows, 0..1, by its distance: they
 * fade in far away and (clouds) out where they leave, instead of popping
 * up. 1 for anything else. (The game's 15 brightnesses are too coarse for
 * it: the drawing multiplies by this.) */
float rb_deco_fade(const rb_line *l);

#endif
