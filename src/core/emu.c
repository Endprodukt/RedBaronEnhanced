#include "core/emu.h"

#include <string.h>

static uint8_t bus_read(void *user, uint16_t a)
{
    return rb_rd8(((rb_emu *)user)->m, a);
}

static void bus_write(void *user, uint16_t a, uint8_t v)
{
    rb_wr8(((rb_emu *)user)->m, a, v);
}

void rb_emu_init(rb_emu *e, rb_machine *m)
{
    memset(e, 0, sizeof *e);
    e->m = m;
    m6502_init(&e->cpu, bus_read, bus_write, e);
    m6502_reset(&e->cpu);
}

static void run_step(rb_emu *e)
{
    rb_machine *m = e->m;
    int cy = m6502_step(&e->cpu);
    e->budget -= cy;
    m->cpu_cycles = e->cpu.cycles;
    uint32_t mc = (uint32_t)cy * 8;
    m->vg_left = m->vg_left > mc ? m->vg_left - mc : 0;
}

void rb_emu_nmi(rb_emu *e)
{
    /* The CPU runs first: the board's first interrupt comes a period after
     * power on, when PWRON has long set the stack pointer. */
    e->budget += RB_CPU_CYCLES_PER_NMI;
    while (e->budget > 0) run_step(e);
    if (e->m->in.in0 & RB_IN0_TEST) {       /* (active low: the switch is off) */
        m6502_nmi(&e->cpu);
        e->nmis++;
        e->m->nmis++;
    }
}

int rb_emu_frame(rb_emu *e, uint32_t max_cycles)
{
    m6502 *c = &e->cpu;
    /* At INTWAIT's BCC, the carry set by the LSR CALFLG before it. MAIN
     * called INTWAIT with the stack at its top: the return address is in
     * the recorded RAM. */
    c->pc = RB_INTWAIT + 2;
    c->s = 0xfd;
    c->p = M6502_C | M6502_U | M6502_I;
    c->a = c->x = c->y = 0;
    c->nmi_pending = 0;
    c->jammed = 0;
    uint64_t start = c->cycles;
    e->budget = (int32_t)max_cycles;
    while (e->budget > 0) {
        run_step(e);
        if (c->pc == RB_INTWAIT) return (int)(c->cycles - start);
    }
    return -1;
}

int rb_emu_nmi_only(rb_emu *e, uint32_t max_cycles)
{
    m6502 *c = &e->cpu;
    /* A return address the interrupt routine comes back to: any place
     * outside it. */
    c->pc = RB_INTWAIT;
    c->s = 0xfd;
    c->p = M6502_U | M6502_I;
    c->jammed = 0;
    m6502_nmi(c);
    uint64_t start = c->cycles;
    e->budget = (int32_t)max_cycles;
    int first = 1;
    while (e->budget > 0) {
        run_step(e);
        if (!first && c->pc == RB_INTWAIT) return (int)(c->cycles - start);
        first = 0;
    }
    return -1;
}
