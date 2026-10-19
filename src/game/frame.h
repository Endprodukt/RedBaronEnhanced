/* frame.h - how the routines of the program are run.
 *
 * The original has two threads: the main line (START, MAIN: one pass
 * computes one game frame and builds its display list, then waits in
 * INTWAIT) and the interrupt, 246 times a second (NMI: the stick's pots,
 * the switches, coins, the sound latch, the POKEY's tunes, counting the
 * score up, and the clocks the main line waits for).
 *
 * Here the interrupt is a call (rb_interrupt), and the main line runs
 * between two interrupts, a whole frame at a time, when the interrupt has
 * counted far enough (rb_main_poll). The C needs no time for a frame; the
 * 6502 took most of the 24 interrupts it has.
 *
 *   C.CNTR  counts interrupts down from CALCNT (24; 12 while the banner
 *           shows) and then bumps CALFLG: time for the next game frame,
 *           10.25 a second
 *   F.CNTR  counts down from 4 and bumps INTFLG: the vector generator is
 *           started again, 61.5 times a second (by the interrupt itself
 *           while the main line computes)
 */
#ifndef RB_FRAME_H
#define RB_FRAME_H

#include "core/machine.h"

enum {
    RB_MAIN_OFF = 0,        /* before power on */
    RB_MAIN_WAIT_CAL,       /* INTWAIT: for the frame time (CALFLG) */
    RB_MAIN_WAIT_VG,        /* for the vector generator's frame (INTFLG) */
    RB_MAIN_WAIT_HALT,      /* for the vector generator to halt */
};

/* Power on: PWRON, START and the first pass of MAIN. The machine must have
 * its ROMs. */
void rb_power_on(rb_machine *m);

/* One interrupt (NMI). The self test switch gates it off, as on the board. */
void rb_interrupt(rb_machine *m);

/* The main line, to be called after every interrupt: goes on where INTWAIT
 * left off and, when its waits are over, shows the frame computed and
 * computes the next. Returns 1 when it did. */
int rb_main_poll(rb_machine *m);

/* One frame without interrupts, for checking against a recording: from
 * inside INTWAIT, just after its LSR CALFLG has shifted a 1 out (the
 * machine holding the memory of that moment), through the rest of INTWAIT,
 * the end of MAIN and the next pass of MAIN, until it would wait again. */
void rb_frame(rb_machine *m);

/* An addition (setting stick_calibrate): the program's own calibration of
 * the stick, as if it had already seen the stick at both ends. To be
 * called after rb_power_on.
 *
 * The interrupt (NMI, "POT READING ROUTINE") keeps for each axis the lowest
 * reading (POTMIN), the range above it (PTRNGE) and the reading less the
 * lowest (POTVAL); POTSCL takes the middle of the range for the stick's
 * middle. At power on PWRINT sets POTMIN to FF and the range to 0, and the
 * routine learns both from the readings, one step per reading: a reading
 * below POTMIN lowers it, one above the range widens it. With the stick at
 * rest in the middle POTMIN comes down to 128 and the range stays 1. The
 * first push, say to the right, widens the range to 64; let go, the stick
 * reads 0 of a range of 64, which is taken for half way to the left, and
 * the plane turns left at full rate (PLDELX -30) until the player has once
 * pushed the stick to the left end too. (Measured: _scratch/pottrace.c.)
 * Here the stick's readings run from 64 to 192 (main.c, the controls), so
 * the range is known from the start. */
void rb_stick_calibrate(rb_machine *m);

/* --- called by the translated routines --- */

/* An addition (render/solid.h): DECODE is about to draw a vector to the
 * point X / 6 of DB.TRP. If an object is being noted (m->note_obj), the
 * note for the vector's words says which point and how far away it is. */
void rb_note_point(rb_machine *m);

/* MAIN has moved everything for the frame and begins to draw it (its label
 * 20$): gives the state to m->view_hook, then draws (rb_main_view). */
void rb_view_begin(rb_machine *m);
/* INTWAIT: the main line waits here; the frame is over. */
void rb_intwait(rb_machine *m);
/* The program stopped itself in a loop and waits for the watchdog. */
void rb_hang(rb_machine *m);
/* A jump out of what is ported (the self test). */
void rb_not_ported(rb_machine *m, const char *what);

#endif
