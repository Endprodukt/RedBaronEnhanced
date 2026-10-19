#include "core/machine.h"

#include "avg/avg.h"

#include <string.h>

void rb_machine_init(rb_machine *m)
{
    memset(m, 0, sizeof *m);
    m->in.in0 = 0x3f;       /* nothing closed (active low) */
    m->in.opt0 = 0xfd;      /* MAME's defaults: normal coinage */
    m->in.opt1 = 0xe7;      /* English, 6k 20k 50k, 3 lives, one play minimum, no self adjust */
    m->in.in4 = RB_IN4_START;   /* (start is low when pressed) */
    m->in.pot_x = m->in.pot_y = 0x80;
    m->lfsr = 0x1ffff;
    memset(m->earom.rom, 0xff, sizeof m->earom.rom);
    rb_mathbox_reset(&m->mb);
}

void rb_machine_set_rom(rb_machine *m, const uint8_t *rom3000)
{
    memcpy(m->mem + 0x3000, rom3000, 0x5000);
    m->have_rom = 1;
}

/* The POKEY's random number: its 17-bit polynomial counter runs at the
 * chip's clock, so what a read sees depends on when it comes. Here it is
 * stepped a varying amount per read. With D0 and D1 of SKCTL clear the
 * counter is held; the program does that outside a game and checks it. */
static uint8_t lfsr_read(rb_machine *m)
{
    if ((m->pokey[0x0f] & 3) == 0) return 0xff;
    unsigned steps = 9 + (m->lfsr & 0x3f) + (m->nmis & 7);
    for (unsigned i = 0; i < steps; i++) {
        uint32_t fb = ((m->lfsr >> 16) ^ (m->lfsr >> 11)) & 1u;     /* x^17 + x^12 + 1 */
        m->lfsr = ((m->lfsr << 1) | fb) & 0x1ffff;
    }
    return (uint8_t)(m->lfsr >> 8);
}

/* --- the EAROM (after MAME's er2055.cpp) -------------------------------------- */

enum { EA_CK = 1, EA_C1 = 2, EA_C2 = 4, EA_CS1 = 8, EA_CS2 = 16 };

static void earom_update(rb_earom *e)
{
    switch (e->control & (EA_C1 | EA_C2)) {
    case 0:         /* write: can only clear bits (erase first) */
        e->rom[e->address] &= e->data;
        e->dirty = 1;
        break;
    case EA_C2:     /* erase */
        e->rom[e->address] = 0xff;
        e->dirty = 1;
        break;
    default:
        break;
    }
}

/* EACTL: D0 the clock, D1 C2, D2 C1 inverted, D3 chip select. */
static void earom_control(rb_earom *e, uint8_t v)
{
    uint8_t old = e->control;
    e->control = (uint8_t)((old & EA_CK) | ((v & 4) ? 0 : EA_C1) | ((v & 2) ? EA_C2 : 0) |
                           ((v & 8) ? EA_CS1 : 0) | EA_CS2);
    if ((e->control & (EA_CS1 | EA_CS2)) == (EA_CS1 | EA_CS2) && e->control != old) earom_update(e);

    old = e->control;
    if (v & 1) e->control |= EA_CK;
    else e->control &= (uint8_t)~EA_CK;
    /* it acts on the falling edge of the clock, when selected */
    if ((e->control & (EA_CS1 | EA_CS2)) == (EA_CS1 | EA_CS2) && e->control != old && !(v & 1)) {
        if (e->control & EA_C1) e->data = e->rom[e->address];      /* read */
        earom_update(e);
    }
}

/* --- the bus -------------------------------------------------------------------- */

static uint8_t io_read(rb_machine *m, uint16_t a)
{
    switch (a) {
    case RB_TESTSW:
        return (uint8_t)((m->in.in0 & 0x3f) | (m->vg_left == 0 ? RB_IN0_HALT : 0) |
                         ((m->cpu_cycles & 0x100) ? RB_IN0_3KHZ : 0));
    case RB_OPT0:   return m->in.opt0;
    case RB_OPT1:   return m->in.opt1;
    case RB_MATHST: return 0x00;                    /* the math box is always done */
    case RB_FIRE:   return m->in.in4;
    case RB_M_PRDL: return rb_mathbox_lo(&m->mb);
    case RB_M_PRDM: return rb_mathbox_hi(&m->mb);
    case RB_POTIN:
        /* (ALLPOT; the board switches one of the stick's two pots to the POKEY) */
        return (m->sound_latch & RB_SND_POTSEL) ? m->in.pot_x : m->in.pot_y;
    case RB_POKEY + 0x0a:
        m->n_rnd++;
        return m->prng ? m->prng(m->prng_user) : lfsr_read(m);
    default:
        if (a >= RB_EAIN && a < RB_EAIN + 0x40) return m->earom.data;
        return 0x00;
    }
}

uint8_t rb_rd8(rb_machine *m, uint16_t a)
{
    a &= 0x7fff;
    if (a >= 0x3000) return m->mem[a];
    uint8_t v = (a < 0x400 || a >= 0x2000) ? m->mem[a] : io_read(m, a);
    if (m->read_hook && m->watch[a]) {
        v = m->read_hook(m->read_user, a, v);
        if (a < 0x400 || a >= 0x2000) m->mem[a] = v;
    }
    return v;
}

void rb_wr8(rb_machine *m, uint16_t a, uint8_t v)
{
    a &= 0x7fff;
    if (a < 0x400) { m->mem[a] = v; return; }
    if (a >= 0x2000) {
        if (a < 0x3000) {                   /* vector RAM; the ROMs do not take writes */
            m->mem[a] = v;
            m->vgtag[(a - 0x2000) >> 1] = m->tag;
            m->vgnote[(a - 0x2000) >> 1] = m->note;
            m->vginst[(a - 0x2000) >> 1] = m->inst;
        }
        return;
    }
    if (a >= RB_MATH && a < RB_MATH + 0x20) {
        m->n_mbox++;
        rb_mathbox_go(&m->mb, a - RB_MATH, v);
        return;
    }
    if (a >= RB_POKEY && a < RB_POKEY + 0x10) {
        m->pokey[a - RB_POKEY] = v;
        if (m->sound_write) m->sound_write(m->sound_user, a, v);
        return;
    }
    if (a >= RB_EADAL && a < RB_EADAL + 0x40) {
        m->earom.address = (uint8_t)(((a - RB_EADAL) ^ 0x20) & 0x3f);
        m->earom.data = v;
        return;
    }
    switch (a) {
    case RB_CRSHSN:
        m->sound_latch = v;
        if (m->sound_write) m->sound_write(m->sound_user, a, v);
        return;
    case RB_NSERST:
        if (m->sound_write) m->sound_write(m->sound_user, a, v);
        return;
    case RB_EACTL:  earom_control(&m->earom, v); return;
    case RB_COINCT: m->coin_counter = v; return;
    case RB_VGON:
        m->vg_starts++;
        m->vg_left = rb_avg_cycles(m->mem, 0x2000);
        return;
    case RB_VGRST:  m->vg_left = 0; return;
    case RB_WATCH:  m->watchdog++; return;
    default: return;
    }
}
