/* sound.h - Red Baron's sound: the POKEY and the discrete circuits.
 *
 * The program writes the POKEY's registers (1810-181F: the tunes, and the
 * engine's hum that rises as a plane comes near) and one latch (1808):
 *
 *   D7-D4  crash: the explosion's volume
 *   D3     start LED (low: lit)
 *   D2     shot: the machine gun
 *   D1     spiral: the squeal of the nose dive
 *   D0     which pot of the stick the POKEY reads (no sound)
 *
 * The discrete sounds follow MAME's model of the board
 * (src/mame/atari/redbaron.cpp, BSD-3-Clause, Brad Oliver and Nicola
 * Salmoria): a 16-bit noise shift register clocked at 12 kHz; the crash is
 * the noise through a slow filter at the latch's volume; the shot is the
 * noise gated onto a capacitor's decaying charge; the squeal a 555 whose
 * pitch falls as a capacitor charges.
 *
 * The machine's sound_write hook delivers the writes; rb_sound_run makes
 * the samples of the time that has passed, into a ring the sound device
 * reads from another thread. The cabinet has one speaker: both channels
 * carry the same.
 *
 * With recordings (sound/samples.h, rb_sound_use_samples) they take the
 * place of the discrete circuits and of the POKEY's engine hum; the tunes
 * stay the POKEY's.
 */
#ifndef RB_SOUND_H
#define RB_SOUND_H

#include <stdint.h>

#include "sound/pokey.h"
#include "sound/samples.h"

#define RB_SOUND_RATE 48000
#define RB_SOUND_RING 16384     /* a third of a second, in pairs of left and right */

typedef struct rb_sound {
    pokey    pk;
    uint8_t  latch;

    /* the discrete circuits (the names are MAME's) */
    int32_t  poly_counter;
    uint16_t poly_shift;
    int32_t  filter_counter;
    uint8_t  crash_amp;
    uint16_t shot_amp;
    int32_t  shot_amp_counter;
    uint16_t squeal_amp;
    int32_t  squeal_amp_counter, squeal_off_counter, squeal_on_counter;
    uint8_t  squeal_out;
    int16_t  vol_crash[16];

    double   dc;                /* the output's coupling capacitor (takes the DC away) */
    double   tick_frac;         /* POKEY clock ticks owed */
    double   frac;              /* samples owed */
    float    volume;            /* 0..1 */

    rb_samples smp;             /* the recordings (loaded by the platform) */
    int      use_samples;       /* they sound instead of the circuits, if loaded */

    int16_t  ring[RB_SOUND_RING * 2];
    volatile uint32_t ring_w, ring_r;
} rb_sound;

void rb_sound_init(rb_sound *s);

/* A write to 1810-181F, 1808 or 180A (rb_machine.sound_write, user = the rb_sound). */
void rb_sound_write(void *user, uint16_t addr, uint8_t value);

/* The recordings instead of the circuits (1), or not (0). */
void rb_sound_use_samples(rb_sound *s, int on);

/* Make the samples of cpu_cycles 6502 cycles (1.512 MHz) of time. */
void rb_sound_run(rb_sound *s, int cpu_cycles);

/* Take up to `pairs` pairs of samples out of the ring (the sound device's
 * thread). Returns how many there were. */
int rb_sound_read(rb_sound *s, int16_t *out, int pairs);

#endif
