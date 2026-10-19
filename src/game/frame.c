/* frame.c - the main line's wait for the interrupt, and what runs when. */
#include "game/game.h"
#include "game/m6502.h"

/* INTWAIT (497D), by hand: the original loops here until the interrupt has
 * counted a frame's time. The loop is rb_main_poll, called after each
 * interrupt.
 *
 * INTWAIT:LSR CALFLG       ;WAIT FOR FRAME TIME
 *         BCC INTWAIT
 *         INC S.FLAG       ;CLR NMI VG RESTART FLAG
 * 5$:     LSR INTFLG       ;WAIT FOR VG FRAME
 *         BCC 5$
 * 10$:    BIT A,TESTSW     ;CHECK FOR VG HALT
 *         BVC 10$
 *         RTS
 */
void rb_intwait(rb_machine *m)
{
    m->main_state = RB_MAIN_WAIT_CAL;
}

void rb_view_begin(rb_machine *m)
{
    if (m->view_hook) m->view_hook(m->view_user, m);
    rb_main_view(m);
}

void rb_note_point(rb_machine *m)
{
    if (!m->note_obj) {
        m->note = 0;
        return;
    }
    unsigned x = m->r.x;
    unsigned obj = m->note_obj;
    int d;
    if (obj == RB_OBJ_PLANE || obj == RB_OBJ_PIECES) {
        /* DB.TRP holds 6 bytes a point: Z, X, Y (after the rotation; D.LOOP
         * divided X and Y by Z plus the object's distance O.DPTH) */
        int z = (int16_t)(uint16_t)(m->mem[RB_DB_TRP + x] | (m->mem[RB_DB_TRP + x + 1] << 8));
        d = z + (int16_t)(uint16_t)(m->mem[RB_O_DPTH] | (m->mem[RB_O_DPTH + 1] << 8));
    } else {
        /* a flat thing (PROJECT): all its points as far away as the object (OBJECT+4) */
        d = (int16_t)(uint16_t)(m->mem[RB_OBJECT + 4] | (m->mem[RB_OBJECT + 5] << 8));
        if (obj == RB_OBJ_GROUND) {
            obj += (m->mem[RB_TEMP2] >> 1) & 3;     /* (GRDISP: the type, times 2) */
            d -= d / 64;                            /* (it stands in front of the mountain it is on) */
        } else if (obj == RB_OBJ_MOUNTAIN) {
            /* which of the four pieces of landscape (SSEGS): PFDSPL keeps its
             * number times 8 in SAVY+1, plus the part it draws times 2 */
            obj = RB_OBJ_MOUNTAIN_SEG + ((m->mem[RB_SAVY + 1] >> 3) & 3);
        }
    }
    if (d < 1) d = 1;
    if (d > 0x7fff) d = 0x7fff;
    m->note = ((uint32_t)obj << 24) | ((uint32_t)(x / 6 + 1) << 16) | (uint32_t)d;
}

void rb_hang(rb_machine *m)
{
    m->hung = 1;
}

void rb_not_ported(rb_machine *m, const char *what)
{
    m->not_ported = what;
}

void rb_power_on(rb_machine *m)
{
    m->sp = 0;
    m->hung = 0;
    m->not_ported = 0;
    m->main_state = RB_MAIN_OFF;
    rb_pwron(m);            /* (ends in START and the first pass of MAIN) */
}

void rb_interrupt(rb_machine *m)
{
    if (!(m->in.in0 & RB_IN0_TEST)) return;         /* (active low: the switch is on) */
    rb_regs save = m->r;                            /* the interrupt leaves the registers as they were */
    rb_nmi(m);
    m->r = save;
    m->nmis++;
}

void rb_stick_calibrate(rb_machine *m)
{
    for (int axis = 0; axis < 2; axis++) {          /* (0: left-right, 1: up-down) */
        m->mem[RB_POTMIN + axis] = 64;
        m->mem[RB_PTRNGE + axis] = 128;
        m->mem[RB_POTVAL + axis] = 64;              /* the middle */
        m->mem[RB_POTUSE + axis] = 0;
    }
}

int rb_main_poll(rb_machine *m)
{
    switch (m->main_state) {
    case RB_MAIN_WAIT_CAL:
        LSR(RB_CALFLG);                             /* WAIT FOR FRAME TIME */
        if (BCC) return 0;
        INC(RB_S_FLAG);                             /* CLR NMI VG RESTART FLAG */
        m->main_state = RB_MAIN_WAIT_VG;
        /* fall through */
    case RB_MAIN_WAIT_VG:
        LSR(RB_INTFLG);                             /* WAIT FOR VG FRAME */
        if (BCC) return 0;
        m->main_state = RB_MAIN_WAIT_HALT;
        /* fall through */
    case RB_MAIN_WAIT_HALT:
        BIT(RD(RB_TESTSW));                         /* CHECK FOR VG HALT */
        if (BVC) return 0;
        rb_main2(m);                                /* the end of MAIN: show the frame */
        rb_main(m);                                 /* and MAIN again, up to INTWAIT */
        m->frames++;
        return 1;
    default:
        return 0;
    }
}

void rb_frame(rb_machine *m)
{
    /* (the record was taken in INTWAIT's LSR CALFLG, with the 1 shifted out) */
    INC(RB_S_FLAG);
    for (int i = 0; i < 16; i++) {
        LSR(RB_INTFLG);
        if (BCS) break;
    }
    for (int i = 0; i < 16; i++) {
        BIT(RD(RB_TESTSW));
        if (BVS) break;
    }
    rb_main2(m);
    rb_main(m);
    m->frames++;
}
