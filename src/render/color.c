#include "render/color.h"

#include "core/tags.h"

#include <string.h>

/* By tag; { 0, 0, 0 }: the cabinet's colour. */
static const uint8_t PALETTE[RB_TAG_COUNT][3] = {
    [RB_TAG_NONE]      = { 0, 0, 0 },
    [RB_TAG_COCKPIT]   = { 200, 200, 210 },     /* the player's own guns and propeller */
    [RB_TAG_LAND]      = { 90, 200, 110 },
    [RB_TAG_HORIZON]   = { 90, 200, 110 },      /* green land */
    [RB_TAG_MOUNTAIN]  = { 150, 210, 90 },
    [RB_TAG_GROUND]    = { 220, 170, 90 },      /* what stands on the ground: earth */
    [RB_TAG_GSHELL]    = { 255, 140, 40 },      /* flak from the ground */
    [RB_TAG_PLANE]     = { 205, 190, 95 },      /* the enemy: the player is the Red Baron; his foes the
                                                 * British, their planes khaki (a Sopwith Camel's
                                                 * PC10, light enough for the tube) */
    [RB_TAG_PROP]      = { 220, 220, 220 },
    [RB_TAG_DRONE]     = { 205, 190, 95 },      /* the escorts: the same (a drone becomes a plane) */
    [RB_TAG_EXPLOSION] = { 255, 200, 60 },
    [RB_TAG_BLIMP]     = { 190, 205, 230 },     /* silver */
    [RB_TAG_SHELL]     = { 255, 240, 120 },
    [RB_TAG_COPYRIGHT] = { 130, 140, 170 },
    [RB_TAG_LIVES]     = { 255, 90, 70 },       /* the planes left */
    [RB_TAG_EOL]       = { 255, 255, 255 },     /* bullet holes in the windshield */
    [RB_TAG_STATES]    = { 255, 210, 60 },      /* the high score pages, the banner */
    [RB_TAG_TEXT]      = { 0, 0, 0 },
    [RB_TAG_SCORE]     = { 255, 245, 170 },     /* the score: bright, warm white */
    [RB_TAG_VALUE]     = { 120, 255, 140 },     /* what the plane is worth: green */
    [RB_TAG_HISCORE]   = { 120, 220, 255 },     /* the best score: light blue */
    [RB_TAG_LABEL]     = { 255, 170, 40 },      /* SCORE, VALUE, HIGH SCORE: amber */
    [RB_TAG_WARN]      = { 255, 60, 50 },       /* overheated guns: red */
    [RB_TAG_CREDITS]   = { 120, 220, 255 },
    [RB_TAG_ATTRACT]   = { 200, 160, 255 },     /* what a coin buys: violet */
    [RB_TAG_INSERT]    = { 255, 210, 60 },      /* INSERT COINS, PRESS START: gold */
    [RB_TAG_BONUS]     = { 255, 110, 210 },     /* BONUS LIFE, 300 POINTS, BEHIND YOU: pink */
    [RB_TAG_DECO]      = { 60, 190, 80 },
    [RB_TAG_LOGO_CROSS]  = { 235, 40, 30 },     /* the title: the flyer's red cross, */
    [RB_TAG_LOGO_RIM]    = { 235, 235, 240 },   /*   white rim, */
    [RB_TAG_LOGO_EAGLE]  = { 255, 200, 40 },    /*   golden eagle, */
    [RB_TAG_LOGO_RED]    = { 235, 50, 35 },     /*   red and gold stripes */
    [RB_TAG_LOGO_GOLD]   = { 255, 205, 40 },
    [RB_TAG_LOGO_TEXT]   = { 255, 240, 220 },   /*   and the lettering white */
    [RB_TAG_SCREEN_HEAD] = { 255, 200, 40 },
    [RB_TAG_SCREEN_TEXT] = { 120, 220, 255 },
    [RB_TAG_SCREEN_HI]   = { 255, 70, 55 },
    [RB_TAG_FREE]        = { 255, 210, 60 },
    [RB_TAG_GUN]         = { 160, 170, 200 },   /* the cockpit: gun metal, */
    [RB_TAG_DASH]        = { 235, 110, 45 },    /*   the bezel's orange, */
    [RB_TAG_STRUT]       = { 215, 60, 40 },     /*   red struts, */
    [RB_TAG_NOSE]        = { 225, 70, 45 },     /*   the red nose, */
    [RB_TAG_PROPELLER]   = { 200, 160, 110 },   /*   the wooden propeller, */
    [RB_TAG_WIRE]        = { 170, 170, 180 },   /*   steel wires, */
    [RB_TAG_WINDSCREEN]  = { 170, 210, 240 },   /*   glass, */
    [RB_TAG_COAMING]     = { 175, 130, 95 },    /*   leather, */
    [RB_TAG_GUNHOT]      = { 255, 120, 40 },    /*   hot guns; */
    [RB_TAG_BLAST]       = { 255, 245, 200 },   /* explosions: white-hot, */
    [RB_TAG_FIRE]        = { 255, 150, 40 },    /*   fire, */
    [RB_TAG_EMBER]       = { 220, 50, 25 },     /*   embers */
    [RB_TAG_ROUNDEL_BLUE] = { 70, 110, 255 },   /* the roundels: blue, */
    [RB_TAG_ROUNDEL_RED]  = { 255, 45, 35 },    /*   red */
    [RB_TAG_GAUGE]       = { 225, 225, 215 },   /*   white faces, */
    [RB_TAG_NEEDLE]      = { 255, 70, 50 },     /*   red needles */
    [RB_TAG_FLASH]       = { 255, 230, 120 },
};

/* The parts of the objects on the ground and of the additions, by their
 * points (render/solid.c, render/deco.c: the same numbers). */
static int on(const rb_line *l, unsigned mask)
{
    return l->p0 && l->p1 && (mask >> (l->p0 - 1) & 1) && (mask >> (l->p1 - 1) & 1);
}

/* The blimp (BLMOTN: DBLIMP decoded into vector RAM, always in the same
 * order): its envelope's lines along it, then three rings round it, then
 * the gondola. rb_color_mark notes which, in model and p0. */
static const uint8_t *blimp_part(const rb_line *l)
{
    static const uint8_t ENVELOPE[3] = { 215, 222, 235 }, BAND[3] = { 230, 55, 45 }, RING[3] = { 120, 150, 205 },
                         GONDOLA[3] = { 225, 160, 70 };
    switch (l->p0) {
    case 1: return ENVELOPE;
    case 2: return BAND;
    case 3: return RING;
    case 4: return GONDOLA;
    default: return NULL;
    }
}

void rb_color_mark(rb_line *l, int n)
{
    unsigned first = 0xffff;
    for (int i = 0; i < n; i++)
        if (l[i].tag == RB_TAG_BLIMP && !l[i].sub && l[i].src < first) first = l[i].src;
    if (first == 0xffff) return;
    for (int i = 0; i < n; i++) {
        if (l[i].tag != RB_TAG_BLIMP || l[i].sub) continue;
        unsigned at = (l[i].src - first) / 4;      /* (a vector is two words) */
        l[i].model = RB_OBJ_BLIMP;
        l[i].p0 = (uint8_t)(at < 32 ? 1 : at < 41 ? 2 : at < 60 ? 3 : 4);
    }
}

static const uint8_t *part(const rb_line *l)
{
    static const uint8_t TRUNK[3] = { 150, 95, 45 }, CROWN[3] = { 70, 205, 80 }, FIR[3] = { 40, 165, 90 },
                         CANVAS[3] = { 235, 215, 150 }, POLE[3] = { 150, 95, 45 },
                         GUN_BASE[3] = { 175, 175, 165 }, GUN_TOP[3] = { 150, 165, 90 }, BARREL[3] = { 220, 220, 220 },
                         WALL[3] = { 185, 175, 155 }, WING[3] = { 140, 130, 120 }, ROOF[3] = { 210, 90, 60 },
                         CLOUD[3] = { 205, 215, 235 };
    switch (l->model) {
    case RB_OBJ_TENT:   return on(l, 0x0a) ? POLE : CANVAS;             /* the pole: points 1 and 3 */
    case RB_OBJ_TREE:
    case RB_OBJ_DECO_TREE: return on(l, 0x63) ? TRUNK : CROWN;          /* the trunk: 0 1 5 6 */
    case RB_OBJ_DECO_FIR:  return on(l, 0x0f) ? TRUNK : FIR;            /* the trunk: 0-3 */
    case RB_OBJ_GUN:    return on(l, 0x0f) ? GUN_BASE : on(l, 0xf0) ? GUN_TOP : BARREL;
    case RB_OBJ_BUNKER: return on(l, 0x000f) ? WALL : on(l, 0xf000) ? ROOF : WING;
    case RB_OBJ_CLOUD:  return CLOUD;
    default:            return NULL;
    }
}

void rb_color_of(const rb_line *l, const uint8_t cabinet[3], uint8_t rgb[3])
{
    const uint8_t *c = (l->tag == RB_TAG_GROUND || l->tag == RB_TAG_DECO) ? part(l) :
                       (l->tag == RB_TAG_BLIMP && l->model == RB_OBJ_BLIMP) ? blimp_part(l) : NULL;
    if (!c) c = l->tag < RB_TAG_COUNT ? PALETTE[l->tag] : PALETTE[0];
    if (!(c[0] | c[1] | c[2])) c = cabinet;
    memcpy(rgb, c, 3);
}
