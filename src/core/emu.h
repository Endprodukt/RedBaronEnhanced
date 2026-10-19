/* emu.h - the original program on the machine.
 *
 * The 6502 (cpu/m6502.h) runs the game's own code from the ROMs against
 * rb_machine: its RAM, the math box, the switches. Everything the game
 * does is then the original's, at the original's pace (the 6502's cycles
 * are counted; the interrupt comes every 6144 of them, 246 times a
 * second).
 *
 * This is the reference: the C routines of src/game compute the same,
 * frame for frame, and tools/rbcompare checks both against MAME.
 */
#ifndef RB_EMU_H
#define RB_EMU_H

#include "core/machine.h"
#include "cpu/m6502.h"

typedef struct rb_emu {
    rb_machine *m;
    m6502      cpu;
    int32_t    budget;      /* CPU cycles to run (negative: ran over into the next slice) */
    uint64_t   nmis;
} rb_emu;

#define RB_CPU_HZ             1512000.0     /* 12.096 MHz / 8 */
#define RB_CPU_CYCLES_PER_NMI 6144          /* 246.09 interrupts a second */
#define RB_NMI_HZ             (RB_CPU_HZ / RB_CPU_CYCLES_PER_NMI)

/* The machine must be initialized and have its ROMs. Resets the CPU: the
 * program starts at PWRON. */
void rb_emu_init(rb_emu *e, rb_machine *m);

/* One interrupt period: the CPU runs for 6144 cycles, then the NMI comes
 * (unless the self test switch is on, which gates it off). */
void rb_emu_nmi(rb_emu *e);

/* One game frame without interrupts, for checking against a recording:
 * from inside INTWAIT, just after its LSR CALFLG has shifted a 1 out (the
 * machine holding the memory of that moment), until the main line is back
 * at INTWAIT. Returns the CPU cycles it took, or -1 if it does not come
 * back within max_cycles. */
int rb_emu_frame(rb_emu *e, uint32_t max_cycles);

/* One interrupt routine alone, from the NMI vector to its RTI, without the
 * main line: for checking the C interrupt routine against the original.
 * Returns the cycles, or -1. */
int rb_emu_nmi_only(rb_emu *e, uint32_t max_cycles);

#endif
