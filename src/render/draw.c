#include "render/draw.h"

#include "core/tags.h"
#include "render/color.h"
#include "render/deco.h"

#include <math.h>
#include <string.h>

#define BOARD_MAX 4095

void rb_map_point(const rb_config *c, float x, float y, int *bx, int *by)
{
    float x0 = (float)c->margin_left, x1 = (float)(BOARD_MAX - c->margin_right);
    float y0 = (float)c->margin_bottom, y1 = (float)(BOARD_MAX - c->margin_top);
    float u = (x - c->view_left) / (c->view_right - c->view_left);
    float v = (y - c->view_bottom) / (c->view_top - c->view_bottom);
    if (c->flip_x) u = 1.0f - u;
    if (c->flip_y) v = 1.0f - v;
    *bx = (int)lroundf(x0 + u * (x1 - x0));
    *by = (int)lroundf(y0 + v * (y1 - y0));
}

void rb_draw_lines(vp_dvg *d, const rb_config *c, const rb_line *lines, int count)
{
    int have_pos = 0, px = 0, py = 0, have_col = 0;
    uint8_t col[3] = { 0, 0, 0 };

    for (int i = 0; i < count; i++) {
        const rb_line *l = &lines[i];
        float k = c->floor + (1.0f - c->floor) * powf((float)l->intensity / 15.0f, c->gamma);
        k *= c->brightness;
        if (l->tag == RB_TAG_DECO) k *= rb_deco_fade(l);   /* (trees and clouds fade in) */
        if (k > 1.0f) k = 1.0f;
        uint8_t base[3];
        if (c->color) rb_color_of(l, c->color_cabinet, base);
        else memcpy(base, c->color_cabinet, 3);
        uint8_t r = (uint8_t)lroundf(base[0] * k), g = (uint8_t)lroundf(base[1] * k),
                b = (uint8_t)lroundf(base[2] * k);
        if (!(r | g | b)) continue;
        int x0, y0, x1, y1;
        rb_map_point(c, l->x0, l->y0, &x0, &y0);
        rb_map_point(c, l->x1, l->y1, &x1, &y1);
        if (!have_col || r != col[0] || g != col[1] || b != col[2]) {
            vp_color(d, r, g, b);
            col[0] = r; col[1] = g; col[2] = b;
            have_col = 1;
        }
        if (!have_pos || x0 != px || y0 != py) vp_moveto(d, x0, y0);
        if (x0 == x1 && y0 == y1) x1 += 1;      /* a dot: the beam has to move to show */
        vp_lineto(d, x1, y1);
        px = x1; py = y1;
        have_pos = 1;
    }
}
