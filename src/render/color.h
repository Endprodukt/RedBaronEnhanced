/* color.h - a colour for every line (the setting `color`).
 *
 * The cabinet's tube is white behind a blue sheet: everything one blue.
 * Here each line gets the colour of what it is (core/tags.h): the enemy's
 * biplane red, the land green, shells yellow; the texts at the top each a
 * colour of their own.
 */
#ifndef RB_COLOR_H
#define RB_COLOR_H

#include <stdint.h>

#include "avg/avg.h"

/* The colour of line l (rgb, full brightness); `cabinet` is the one colour
 * of the cabinet, for what keeps it. */
void rb_color_of(const rb_line *l, const uint8_t cabinet[3], uint8_t rgb[3]);

/* Before rb_color_of on a picture's lines: notes the blimp's parts (its
 * envelope, rings, gondola) for their colours. */
void rb_color_mark(rb_line *l, int n);

#endif
