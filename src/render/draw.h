/* draw.h - the vector generator's lines onto the VectorPie device layer. */
#ifndef RB_DRAW_H
#define RB_DRAW_H

#include "avg/avg.h"
#include "render/config.h"

#include <vectorpie/vp_dvg.h>

/* Screen units -> board units (0..4095, may fall outside: the SDK clips). */
void rb_map_point(const rb_config *c, float x, float y, int *bx, int *by);

/* Draw lines in list order, chaining strokes where one line ends where the
 * next begins and setting the color only when it changes. */
void rb_draw_lines(vp_dvg *d, const rb_config *c, const rb_line *lines, int count);

#endif
