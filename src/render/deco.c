/* deco.c - see deco.h. */
#include "render/deco.h"

#include "core/tags.h"
#include "game/game.h"

#include <math.h>
#include <string.h>

#define LINE(k)  ((uint8_t)((k) * 6 + 1))     /* a visible line to point k */
#define MOVE(k)  ((uint8_t)((k) * 6))         /* a move to it */

/* The game's tree (PFLOB, its second object), two thirds of its size: the
 * trunk, the crown. */
static const int8_t TREE_PTS[] = { -1, -5, -1, 0, -3, 0, 0, 11, 3, 0, 1, 0, 1, -5 };
static const uint8_t TREE_LINES[] = { MOVE(0), LINE(1), MOVE(5), LINE(6), MOVE(2), LINE(3), LINE(4), LINE(2), 0xff };
static const rb_deco_face TREE_FACES[] = { { 3, { 2, 3, 4 } }, { 4, { 0, 1, 5, 6 } } };

/* A fir after it: the trunk, two tiers. */
static const int8_t FIR_PTS[] = { -1, -5, -1, 0, 1, 0, 1, -5, -4, 0, 0, 8, 4, 0, -3, 5, 0, 15, 3, 5 };
static const uint8_t FIR_LINES[] = { MOVE(0), LINE(1), MOVE(2), LINE(3), MOVE(4), LINE(5), LINE(6), LINE(4),
                                     MOVE(7), LINE(8), LINE(9), 0xff };
static const rb_deco_face FIR_FACES[] = { { 3, { 4, 5, 6 } }, { 3, { 7, 8, 9 } }, { 4, { 0, 1, 2, 3 } } };

/* A cloud: a long low outline, flat below. */
static const int8_t CLOUD_PTS[] = { -56, 0, -50, 10, -42, 16, -33, 14, -25, 25, -14, 30, -3, 25, 6, 33, 17, 27, 25, 19,
                                    36, 22, 45, 14, 53, 8, 56, 0 };
static const uint8_t CLOUD_LINES[] = { MOVE(0), LINE(1), LINE(2), LINE(3), LINE(4), LINE(5), LINE(6), LINE(7),
                                       LINE(8), LINE(9), LINE(10), LINE(11), LINE(12), LINE(13), LINE(0), 0xff };
static const rb_deco_face CLOUD_FACES[] = { { 7, { 0, 1, 2, 3, 4, 5, 6 } }, { 8, { 6, 7, 8, 9, 10, 11, 12, 13 } },
                                            { 3, { 0, 6, 13 } } };

#define SHAPE(p, l, f) { p, (int)(sizeof p / 2), l, (int)sizeof l, f, (int)(sizeof f / sizeof f[0]) }
static const rb_deco_shape SHAPES[] = {
    SHAPE(TREE_PTS, TREE_LINES, TREE_FACES),
    SHAPE(FIR_PTS, FIR_LINES, FIR_FACES),
    SHAPE(CLOUD_PTS, CLOUD_LINES, CLOUD_FACES),
};

const rb_deco_shape *rb_deco_shape_of(unsigned model)
{
    if (model < RB_OBJ_DECO_TREE || model > RB_OBJ_CLOUD) return NULL;
    return &SHAPES[model - RB_OBJ_DECO_TREE];
}

/* --- the world ---------------------------------------------------------------- */

/* The game's projection (POSITP, measured: _scratch/probe.c): a thing dx
 * to the side and dy above the eye (I4YPOS) at depth d is at
 * 400 * dx / d, 400 * dy / d + 40 (hex) on the screen, which shows about
 * -208..208 by -190..190 (hex) of that. At the usual height (210) the
 * ground shows from a depth of about 490 to the horizon (1000). */

/* Trees: cells of the ground, CELL_X across and CELL_Z deep. The player
 * flies through them at SPEED (units of depth a second). */
#define TREE_CELL_X   0x0100
#define TREE_CELL_Z   0x0100
#define TREE_NEAR     0x0400        /* nearer than this: gone (below the screen, mostly) */
#define TREE_FAR      0x0F00        /* farther: not yet there (the horizon is at 1000) */
#define TREE_SPEED    700.0
#define TREE_SPREAD   0.6           /* how far to the sides, as a part of the depth (the screen's edge) */
#define TREE_FADE     0x0500        /* fading in over this much depth before TREE_FAR */
#define MAX_TREES     20            /* at most (the vector monitor's time); seldom reached: one
                                     * cell in ten has a tree, about 12 are in sight, and a tree
                                     * left out for the limit would pop up when a nearer one goes */

/* Clouds: far behind the mountains, high up. */
#define CLOUD_CELL_X  0x0A00
#define CLOUD_CELL_Z  0x0600
#define CLOUD_NEAR    0x1600
#define CLOUD_FAR     0x3000
#define CLOUD_SPEED   120.0
#define CLOUD_SPREAD  0.7
#define CLOUD_HEIGHT  0x0700        /* above the ground, and up to 300 more */
#define CLOUD_FADE    0x0600        /* fading in after CLOUD_FAR, out before CLOUD_NEAR */
#define MAX_CLOUDS    8             /* (one cell in four: about 5 in sight) */

static float smooth01(float v)
{
    v = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
    return v * v * (3.0f - 2.0f * v);
}

/* A new scene (the world shown again after the high score pages or the
 * end of a life, a new game): all of them fade in together over this many
 * seconds. g_scene: how far that is, for the picture rb_deco_draw drew last. */
#define SCENE_FADE    1.0
static float g_scene = 1.0f;

float rb_deco_fade(const rb_line *l)
{
    /* the distance its points were drawn at (rb_note_point: OBJECT+4, the d below) */
    float w = l->w0 > l->w1 ? l->w0 : l->w1;
    if (w <= 0.0f) return g_scene;
    float d = 1.0f / w;
    switch (l->model) {
    case RB_OBJ_DECO_TREE:
    case RB_OBJ_DECO_FIR:
        return g_scene * smooth01((TREE_FAR - d) / TREE_FADE);
    case RB_OBJ_CLOUD: {
        float in = (CLOUD_FAR - d) / CLOUD_FADE, out = (d - CLOUD_NEAR) / CLOUD_FADE;
        return g_scene * smooth01(in < out ? in : out);
    }
    default:
        return g_scene;
    }
}

/* The scene's fade for the picture at t. g_shown: whether the last picture
 * showed any of them (none shown, or none to be drawn: the next that shows
 * some begins a scene). */
static int g_shown;

static void scene(double t)
{
    static double t0, last = -1.0;
    if (last < 0.0) t0 = t - SCENE_FADE;            /* (the first picture of all: no fading) */
    else if ((!g_shown && t != last) || t < last || t - last > 0.5) t0 = t;
    last = t;
    g_scene = smooth01((float)((t - t0) / SCENE_FADE));
}

/* Room in the other buffer for one more thing before the shapes' data (a
 * cloud takes 4 bytes a line, 14 lines, and its beam's place: 0x48). */
static int room(const rb_machine *m, uint16_t data)
{
    return rb_peek16(m, RB_RAMPTR) + 0x60 < data;
}

static uint32_t hash(int32_t i, int32_t j, uint32_t salt)
{
    uint32_t h = (uint32_t)i * 0x9E3779B1u ^ (uint32_t)j * 0x85EBCA77u ^ salt;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return h;
}

/* The shapes' points and lines go into the copy's other display buffer,
 * which this picture does not use; the additions' own list goes there too. */
static void put(rb_machine *m, uint16_t at, const void *src, int n)
{
    memcpy(m->mem + at, src, (size_t)n);
}

static void wr16(rb_machine *m, uint16_t a, uint16_t v)
{
    m->mem[a] = (uint8_t)v;
    m->mem[a + 1] = (uint8_t)(v >> 8);
}

/* One thing at world X (16 bits, as UNIV4X), height Y above the ground,
 * depth D: drawn as GRDISP draws an object on the ground. Returns 1 if it
 * showed. */
static int draw_one(rb_machine *m, int16_t x, int16_t y, int16_t d, uint16_t pts, int npts, uint16_t lines,
                    unsigned model, uint8_t bright)
{
    wr16(m, RB_OBJECT, (uint16_t)x);
    wr16(m, RB_OBJECT + 2, (uint16_t)y);
    wr16(m, RB_OBJECT + 4, (uint16_t)d);
    rb_pftrig(m);                               /* the horizon's bank */
    rb_positp(m);                               /* where it is: XPOS (ends with Y, X = its X) */
    rb_setbm(m);                                /* on the screen? the beam to it */
    if (m->r.c) return 0;
    rb_trns0(m);
    m->r.a = 4;
    m->r.y = bright;
    rb_vgstat(m);                               /* its brightness */
    m->r.a = (uint8_t)(pts >> 8);
    m->r.x = (uint8_t)pts;
    m->r.y = (uint8_t)(2 * (npts - 1));
    rb_project(m);                              /* its points, scaled by the distance and banked */
    m->note_obj = (uint8_t)model;
    m->inst++;
    m->r.a = (uint8_t)(lines >> 8);
    m->r.x = (uint8_t)lines;
    rb_dcmpa0(m);                               /* its lines */
    m->note_obj = 0;
    m->note = 0;
    return 1;
}

int rb_deco_draw(rb_machine *m, uint16_t start, double t, int trees, int clouds)
{
    uint8_t gamode = m->mem[RB_GAMODE], gmend1 = m->mem[RB_GMEND1];
    int on = (trees || clouds) && !(gamode & 2)    /* the high score pages */
             && !((uint8_t)(gmend1 << 2) & 0x80);   /* the stars of the end of a life */
    scene(t);
    if (!on) {
        g_shown = 0;
        return 0;
    }

    /* The list ends with CNTR, HALT (VGHALT): find the HALT. */
    uint16_t own = (uint16_t)(start & 0xf800), other = own ^ 0x0800;
    uint16_t pc = start, end = 0;
    while (pc < own + 0x0530) {
        uint16_t w = rb_peek16(m, pc);
        if ((w >> 13) == 1) { end = (uint16_t)(pc + 2); break; }
        pc = (uint16_t)(pc + ((w >> 13) == 0 ? 4 : 2));
    }
    if (!end || end < own + 8 || end + 2 > own + 0x0530) {             /* (below the drone's picture, 2530) */
        g_shown = 0;
        return 0;
    }

    /* the data: the shapes' points and lines, at the top of the other buffer */
    uint16_t data = (uint16_t)(other + 0x0780), at = data;
    uint16_t pts[3], lns[3];
    for (int i = 0; i < 3; i++) {
        const rb_deco_shape *sh = &SHAPES[i];
        pts[i] = at; put(m, at, sh->points, sh->n_points * 2); at = (uint16_t)(at + sh->n_points * 2);
        lns[i] = at; put(m, at, sh->lines, sh->n_lines); at = (uint16_t)(at + sh->n_lines);
    }

    /* the list: a JSRL to ours in place of the CNTR, then CNTR, HALT again */
    uint16_t sub = (uint16_t)(other + 0x0002);
    wr16(m, RB_RAMPTR, (uint16_t)(end - 4));
    uint16_t tag = m->tag;
    m->tag = RB_TAG_DECO;
    m->r.a = (uint8_t)(sub >> 8);
    m->r.x = (uint8_t)sub;
    rb_vgjsrl(m);
    uint16_t back = rb_peek16(m, RB_RAMPTR);
    wr16(m, RB_RAMPTR, sub);
    rb_vgcntr(m);

    int16_t univ = (int16_t)rb_peek16(m, RB_UNIV4X);
    int shown = 0;
    if (trees) {
        /* near first: the nearest show most, and there is a limit */
        double travel = t * TREE_SPEED;
        int32_t jz0 = (int32_t)floor((travel + TREE_NEAR) / TREE_CELL_Z), jz1 = (int32_t)ceil((travel + TREE_FAR) / TREE_CELL_Z);
        int drawn = 0;
        for (int32_t j = jz0; j <= jz1 && drawn < MAX_TREES; j++) {
            for (int32_t i = -24; i <= 24 && drawn < MAX_TREES && room(m, data); i++) {
                int32_t cx = i + univ / TREE_CELL_X;
                uint32_t h = hash(cx, j, 0x7ee5u);
                if ((h & 0xff) >= 26) continue;                     /* (most cells are open land: one in ten has a tree) */
                double wz = j * (double)TREE_CELL_Z + (double)((h >> 8) & 0x7f) * TREE_CELL_Z / 128.0;
                double d = wz - travel;
                if (d < TREE_NEAR || d > TREE_FAR) continue;
                int32_t wx = cx * TREE_CELL_X + (int32_t)((h >> 16) & 0xff) * TREE_CELL_X / 256;
                int32_t dx = (int16_t)(uint16_t)(wx - univ);
                if (fabs((double)dx) > d * TREE_SPREAD) continue;
                unsigned model = (h >> 24) & 1 ? RB_OBJ_DECO_FIR : RB_OBJ_DECO_TREE;
                int k = model - RB_OBJ_DECO_TREE;
                /* brighter as it comes nearer, as the game does its objects */
                int b = 0xb0 - (int)(d * 0x70 / TREE_FAR);
                int one = draw_one(m, (int16_t)(uint16_t)wx, 0, (int16_t)d, pts[k], SHAPES[k].n_points, lns[k], model,
                                   (uint8_t)(b & 0xf0));
                drawn += one;
                shown += one;
            }
        }
    }
    if (clouds) {
        double travel = t * CLOUD_SPEED;
        int32_t jz0 = (int32_t)floor((travel + CLOUD_NEAR) / CLOUD_CELL_Z), jz1 = (int32_t)ceil((travel + CLOUD_FAR) / CLOUD_CELL_Z);
        int drawn = 0;
        for (int32_t j = jz1; j >= jz0 && drawn < MAX_CLOUDS; j--) {
            for (int32_t i = -8; i <= 8 && drawn < MAX_CLOUDS && room(m, data); i++) {
                int32_t cx = (int32_t)(i + univ / CLOUD_CELL_X);
                uint32_t h = hash(cx, j, 0xc10du);
                if ((h & 3) != 0) continue;                         /* (most cells are clear sky) */
                double wz = j * (double)CLOUD_CELL_Z + (double)((h >> 8) & 0x7f) * CLOUD_CELL_Z / 128.0;
                double d = wz - travel;
                if (d < CLOUD_NEAR || d > CLOUD_FAR) continue;
                int32_t wx = cx * CLOUD_CELL_X + (int32_t)((h >> 16) & 0xff) * CLOUD_CELL_X / 256;
                int32_t dx = (int16_t)(uint16_t)(wx - univ);
                if (fabs((double)dx) > d * CLOUD_SPREAD) continue;
                int16_t height = (int16_t)(CLOUD_HEIGHT + (int)((h >> 24) & 0x3f) * 0x0c);
                int one = draw_one(m, (int16_t)(uint16_t)wx, height, (int16_t)d, pts[2], SHAPES[2].n_points, lns[2],
                                   RB_OBJ_CLOUD, 0x50);
                drawn += one;
                shown += one;
            }
        }
    }
    rb_vgrtsl(m);
    m->tag = tag;
    /* and the end of the list as it was */
    wr16(m, RB_RAMPTR, back);
    rb_vghalt(m);
    g_shown = shown > 0;
    return shown;
}
