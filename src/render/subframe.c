/* subframe.c - see subframe.h. */
#include "render/subframe.h"

#include "game/game.h"

#include <math.h>
#include <string.h>

/* The motion objects (PLOBDB: the plane, two drones; or the blimp), 28
 * bytes each (RBARON.MAC, DATA STRUCTURE FORMATS). */
#define OBJ_BASE  RB_PLOBDB
#define OBJ_SIZE  RB_L_OBJ
#define OBJ_N     3
#define O_STATUS  0x06      /* D7 active, D6 exploding */
#define O_TYPE    0x18

/* The landscape's objects (PFOBJ), the shells, the ground's shells. */
#define PF_N      4
#define PF_SIZE   RB_L_PFOB
#define SH_N      (RB_N_SHL / RB_L_SHL + 1)     /* (13; N.SHL is the last one's offset) */
#define SH_SIZE   RB_L_SHL
#define GS_N      5
#define GS_SIZE   RB_GRLSHL

static uint16_t rd16(const rb_machine *m, uint16_t a)
{
    return (uint16_t)(m->mem[a] | (m->mem[a + 1] << 8));
}

/* Words [addr, addr + 2n) of s: on their way from a to b. Things wrap
 * around (the world is a circle), so it is the signed step that is taken.
 * A step larger than `most` is no move but a new thing in the old one's
 * place: then nothing is changed and 0 is returned. */
static int lerp(rb_machine *s, const rb_subframe *f, uint16_t addr, int n, float t, int most)
{
    for (int i = 0; i < n; i++) {
        int d = (int16_t)(uint16_t)(rd16(&f->b, (uint16_t)(addr + 2 * i)) - rd16(&f->a, (uint16_t)(addr + 2 * i)));
        if (d > most || d < -most) return 0;
    }
    for (int i = 0; i < n; i++) {
        uint16_t a = rd16(&f->a, (uint16_t)(addr + 2 * i));
        int d = (int16_t)(uint16_t)(rd16(&f->b, (uint16_t)(addr + 2 * i)) - a);
        uint16_t v = (uint16_t)(a + (int)lrintf((float)d * t));
        s->mem[addr + 2 * i] = (uint8_t)v;
        s->mem[addr + 2 * i + 1] = (uint8_t)(v >> 8);
    }
    return 1;
}

/* A display position (X, Y: 16 bits each, the screen's middle at 0) not far
 * off the screen (-208..208 by -190..190 hex shows). */
static int on_screen(const rb_machine *m, uint16_t addr)
{
    int x = (int16_t)rd16(m, addr), y = (int16_t)rd16(m, (uint16_t)(addr + 2));
    return x > -0x300 && x < 0x300 && y > -0x300 && y < 0x300;
}

void rb_subframe_hook(void *user, const rb_machine *m)
{
    rb_subframe *f = (rb_subframe *)user;
    if (f->have) f->a = f->b;
    f->b = *m;
    if (f->have < 2) f->have++;
}

/* The copy is on its own: no sound, no recording, nobody keeping its state. */
static void detach(rb_machine *s)
{
    s->sound_write = NULL;
    s->read_hook = NULL;
    s->view_hook = NULL;
}

int rb_subframe_state(rb_subframe *f, float t, rb_machine *s)
{
    if (f->have < 2) return 0;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    const rb_machine *a = &f->a, *b = &f->b;

    *s = *a;
    detach(s);

    if (t > 0) {
        /* The player: where he is, how high, the horizon's bank and
         * scroll. A frame in which these jump is a cut (a new game, a new
         * life, the end of one): the frame's own picture is shown then. */
        if (!lerp(s, f, RB_UNIV4X, 2, t, 0x1000) ||
            !lerp(s, f, RB_PFROTN, 2, t, 0x800)) {
            f->cuts++;
            return 0;
        }
        /* the planes, the drones, the blimp: where they are, where they
         * show, how they are turned; those that are the same thing in
         * both states */
        for (int i = 0; i < OBJ_N; i++) {
            uint16_t o = (uint16_t)(OBJ_BASE + i * OBJ_SIZE);
            /* (STATUS: A active, E explosion, C, R, @ the others may change;
             * ED, P, F the drones' states. One that is shown in both, as the
             * same kind of thing: a plane, an exploding one, a drone) */
            uint8_t sa = a->mem[o + O_STATUS], sb = b->mem[o + O_STATUS];
            if (!(sa & 0xc7) || !(sb & 0xc7) || a->mem[o + O_TYPE] != b->mem[o + O_TYPE]) continue;
            if ((sa & 0x47) != (sb & 0x47)) continue;
            /* X, Y, Z: the blimp moves slowly (Z by 80 hex a frame); a larger
             * step is the next blimp set in the place of the one gone by */
            int blimp = (a->mem[o + O_TYPE] & 0x80) != 0;
            if (!lerp(s, f, o, 3, t, blimp ? 0x400 : 0x2000)) continue;
            /* where it shows: only when it showed in both (a thing that has just
             * come has a place far off the screen there) */
            if (on_screen(a, (uint16_t)(o + 0x08)) && on_screen(b, (uint16_t)(o + 0x08)))
                lerp(s, f, (uint16_t)(o + 0x08), 2, t, 0x2000);
            lerp(s, f, (uint16_t)(o + 0x12), 3, t, 0x4000);  /* its angles */
            lerp(s, f, (uint16_t)(o + 0x19), 1, t, 0x2000);  /* its Z position */
        }
        /* the landscape: the mountains and what stands on the ground */
        for (int i = 0; i < PF_N; i++) {
            uint16_t o = (uint16_t)(RB_PFOBJ + i * PF_SIZE);
            if (a->mem[o + 6] != b->mem[o + 6] || a->mem[o + 7] != b->mem[o + 7]) continue;
            lerp(s, f, o, 3, t, 0x1000);                 /* X, its depths */
        }
        /* the shells, the player's and the enemy's */
        for (int i = 0; i < SH_N; i++) {
            uint16_t o = (uint16_t)(RB_SHELLS + i * SH_SIZE);
            if (!(a->mem[o + 7] & 0x80) || !(b->mem[o + 7] & 0x80)) continue;
            lerp(s, f, o, 3, t, 0x1000);
        }
        /* the shells from the ground (D7 of the Z position's high byte: active) */
        for (int i = 0; i < GS_N; i++) {
            uint16_t o = (uint16_t)(RB_GRSHLS + i * GS_SIZE);
            if (!(a->mem[o + 5] & 0x80) || !(b->mem[o + 5] & 0x80)) continue;
            lerp(s, f, o, 3, t, 0x1000);
        }
    }

    return 1;
}

uint16_t rb_subframe_render(rb_subframe *f, const rb_machine *live, float t, rb_machine *s)
{
    if (!rb_subframe_state(f, t, s)) return 0;
    const rb_machine *a = &f->a;
    /* The frame's drawing, as MAIN does it from 20$ on: into the buffer the
     * frame on the screen was written to. */
    uint16_t start = (a->mem[RB_BUFFER] & 0x80) ? 0x2802 : 0x2002;
    rb_main_view(s);
    /* The propellers turn with the interrupt (PLPROP), between the frames:
     * as they are now. */
    uint16_t off = (uint16_t)(start - 0x2002);
    static const uint16_t PROPS[] = { 0x2012, 0x2013, 0x2794, 0x2795 };
    for (int i = 0; i < 4; i++) s->mem[PROPS[i] + off] = live->mem[PROPS[i] + off];
    return start;
}
