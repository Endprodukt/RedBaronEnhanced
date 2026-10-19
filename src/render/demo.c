/* demo.c - see demo.h. */
#include "render/demo.h"

#include "core/emu.h"
#include "game/game.h"

#include <string.h>

#define NMI_PERIOD (1.0 / RB_NMI_HZ)
#define AIM_FIRE   4            /* fire every 4 frames, held 2 */

/* Coined and started from power on: the coin after one second, start after
 * one and a half, run until the game is on. */
static void start_game(rb_demo *d)
{
    rb_machine *m = &d->m;
    rb_power_on(m);
    rb_stick_calibrate(m);
    d->played = 0;
    for (long i = 0; i < (long)(4.0 * RB_NMI_HZ); i++) {
        double t = i * NMI_PERIOD;
        m->in.in0 = (uint8_t)(t > 1.0 && t < 1.12 ? 0x3f & ~RB_IN0_COIN_R : 0x3f);
        m->in.in4 = (uint8_t)(t > 1.5 && t < 1.7 ? 0 : RB_IN4_START);
        m->in.pot_x = m->in.pot_y = 128;
        m->vg_left = 0;
        rb_interrupt(m);
        rb_main_poll(m);
        if (m->mem[RB_GAMODE] & 0x80) break;
    }
    m->in.in0 = 0x3f;
    m->in.in4 = RB_IN4_START;
    d->frames = d->n = 0;
    d->sx = d->sy = 1;
    d->px = d->py = 1000;
    d->fire_left = 0;
}

void rb_demo_init(rb_demo *d, const uint8_t *rom3000, uint8_t opt0, uint8_t opt1)
{
    memset(d, 0, sizeof *d);
    rb_machine_init(&d->m);
    rb_machine_set_rom(&d->m, rom3000);
    d->m.in.opt0 = opt0;
    d->m.in.opt1 = opt1;
    d->m.view_hook = rb_subframe_hook;
    d->m.view_user = &d->sub;
    d->period = 24.0 / RB_NMI_HZ;
    start_game(d);
}

static int clamp(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

/* Once a frame: steer at the plane (where it shows: PLSTAT+8..B, the
 * screen's middle at 0; which way the stick has to go is found by trying),
 * fire now and then. */
static void autopilot(rb_demo *d)
{
    rb_machine *m = &d->m;
    d->frames++;
    if (m->mem[RB_PLSTAT + 6] & 0x80) {
        int x = (int16_t)(uint16_t)(m->mem[RB_PLSTAT + 8] | (m->mem[RB_PLSTAT + 9] << 8));
        int y = (int16_t)(uint16_t)(m->mem[RB_PLSTAT + 10] | (m->mem[RB_PLSTAT + 11] << 8));
        int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
        if (++d->n >= 6) {
            if (ax > d->px + 8) d->sx = -d->sx;
            if (ay > d->py + 8) d->sy = -d->sy;
            d->px = ax; d->py = ay; d->n = 0;
        }
        m->in.pot_x = (uint8_t)clamp(128 + d->sx * (x > 12 ? 56 : x < -12 ? -56 : 0));
        m->in.pot_y = (uint8_t)clamp(128 + d->sy * (y > 12 ? 56 : y < -12 ? -56 : 0));
    } else {
        m->in.pot_x = m->in.pot_y = 128;
        d->n = 0;
        d->px = d->py = 1000;
    }
    if (d->fire_left > 0) d->fire_left--;
    else if (d->frames % AIM_FIRE == 0) d->fire_left = AIM_FIRE / 2;
    m->in.in4 = (uint8_t)(RB_IN4_START | (d->fire_left > 0 ? RB_IN4_FIRE : 0));
}

void rb_demo_nmi(rb_demo *d)
{
    rb_machine *m = &d->m;
    m->vg_left = 0;
    rb_interrupt(m);
    if (rb_main_poll(m)) {
        if (d->t - d->t_frame > 0.01) d->period = d->t + NMI_PERIOD - d->t_frame;
        d->t_frame = d->t + NMI_PERIOD;
        autopilot(d);
    }
    d->t += NMI_PERIOD;
    if (m->mem[RB_GAMODE] & 0x80) d->played = 1;
    else if (d->played || m->hung || m->not_ported) start_game(d);     /* its game is over: the next */
}

uint16_t rb_demo_render(rb_demo *d, double ahead, rb_machine *scratch)
{
    float t = (float)((d->t + ahead - d->t_frame) / d->period);
    uint16_t st = rb_subframe_render(&d->sub, &d->m, t, scratch);
    if (!st) {
        *scratch = d->m;
        scratch->sound_write = NULL;
        scratch->view_hook = NULL;
        st = (d->m.mem[RB_BUFFER] & 0x80) ? 0x2002 : 0x2802;
    }
    return st;
}
