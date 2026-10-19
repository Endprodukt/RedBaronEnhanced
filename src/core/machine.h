/* machine.h - the Red Baron board, as the program sees it.
 *
 * The port keeps the original memory layout: RAM at 0000-03FF, vector RAM
 * at 2000-2FFF, the ROMs at 3000-7FFF, every variable at the address the
 * source gives it (core/rb_symbols.h). The game's state can then be
 * compared byte for byte with a MAME recording, and the 1981 source reads
 * one to one against the C.
 *
 *   0800       TESTSW: coins, slam, self test, diagnostic step, VG halted, 3 kHz
 *   0A00 0C00  option switches (OPT0: coins, OPT1: language, bonus, lives)
 *   1000       coin counters      1200 VG go    1400 watchdog   1600 VG reset
 *   1800       math box status    1804 1806 its result, low and high
 *   1802       FIRE: D7 fire, D6 start
 *   1808       CRSHSN: D0 which pot the POKEY reads, D1 spiral, D2 shot,
 *              D3 start LED (low: on), D4-D7 crash volume
 *   180A       noise reset        180C EAROM control
 *   1810-181F  POKEY: the sounds; reg 8 read is the stick, reg A random
 *   1820-185F  EAROM (high scores, game times): address and data latch
 *   1860-187F  math box
 */
#ifndef RB_MACHINE_H
#define RB_MACHINE_H

#include <stdint.h>

#include "core/rb_symbols.h"
#include "core/tags.h"
#include "mbox/mathbox.h"

/* TESTSW (0800), as the switches present it: coins, self test and
 * diagnostic step are active LOW. */
#define RB_IN0_COIN_R  0x01    /* right mech (MAME: Coin 1) */
#define RB_IN0_COIN_M  0x02    /* middle mech (MAME: Coin 2) */
#define RB_IN0_COIN_L  0x04    /* left mech */
#define RB_IN0_SLAM    0x08
#define RB_IN0_TEST    0x10    /* self test switch */
#define RB_IN0_DIAG    0x20    /* diagnostic step */
#define RB_IN0_HALT    0x40    /* the vector generator has halted (active high) */
#define RB_IN0_3KHZ    0x80

/* FIRE (1802). */
#define RB_IN4_START   0x40    /* low while the button is held */
#define RB_IN4_FIRE    0x80

/* CRSHSN (1808). */
#define RB_SND_POTSEL  0x01    /* which axis of the stick the POKEY reads */
#define RB_SND_SPIRAL  0x02    /* the nose dive's squeal */
#define RB_SND_SHOT    0x04    /* machine gun */
#define RB_SND_LED     0x08    /* start LED, low: lit */
#define RB_SND_CRASH   0xF0    /* explosion volume */

typedef struct rb_inputs {
    uint8_t in0;     /* D0-D5 of 0800 */
    uint8_t opt0;    /* 0A00: coin modes */
    uint8_t opt1;    /* 0C00: language, bonus life, lives, 2 play minimum, self adjust */
    uint8_t in4;     /* 1802 */
    uint8_t pot_x;   /* the stick, left-right: 0..255, 128 in the middle */
    uint8_t pot_y;   /* up-down */
} rb_inputs;

/* The 6502's registers and flags, for the C routines (game/m6502.h): they
 * pass values in A, X, Y and the carry just as the original does. */
typedef struct rb_regs {
    uint8_t a, x, y;
    uint8_t c, z, n, v;    /* each 0 or 1 */
    uint8_t d;             /* decimal mode */
} rb_regs;

/* The ER2055 EAROM: 64 bytes that survive power off. */
typedef struct rb_earom {
    uint8_t rom[64];
    uint8_t address, data;  /* the latches */
    uint8_t control;        /* CK 1, C1 2, C2 4, CS1 8, CS2 16 */
    uint8_t dirty;          /* written since the owner last looked (to save it) */
} rb_earom;

typedef struct rb_machine rb_machine;

typedef uint8_t (*rb_prng_fn)(void *user);
/* A read of a watched address: gets the value the machine would give and
 * returns the value to use (checking against a recording). */
typedef uint8_t (*rb_read_fn)(void *user, uint16_t addr, uint8_t value);
/* A write to the POKEY (1810-181F) or the sound latch (1808, 180A). */
typedef void (*rb_sound_fn)(void *user, uint16_t addr, uint8_t value);

struct rb_machine {
    uint8_t    mem[0x8000];     /* RAM 0000-03FF, vector RAM 2000-2FFF, ROM 3000-7FFF */
    int        have_rom;
    rb_inputs  in;
    rb_mathbox mb;
    rb_earom   earom;

    uint8_t    pokey[16];       /* the POKEY's registers as last written */
    uint8_t    sound_latch;     /* 1808 */
    uint8_t    coin_counter;    /* 1000 */
    uint32_t   vg_starts;       /* writes to VGON: a new picture for the renderer */
    uint32_t   vg_left;         /* master clock cycles until the vector generator halts; 0: halted */
    uint64_t   cpu_cycles;      /* for the 3 kHz bit of TESTSW (the 6502 core counts them) */
    uint32_t   watchdog;        /* writes to WATCH */

    rb_prng_fn prng;            /* NULL: the machine's own LFSR */
    void      *prng_user;
    uint32_t   lfsr;

    rb_read_fn read_hook;       /* NULL: none */
    void      *read_user;
    uint8_t    watch[0x3000];   /* 1: reads of this address pass through read_hook */

    rb_sound_fn sound_write;    /* NULL: nobody listens */
    void      *sound_user;

    /* MAIN has moved everything and begins to draw (rb_view_begin): the
     * state the frame's picture is drawn from (render/subframe.h). NULL:
     * nobody keeps it. */
    void     (*view_hook)(void *user, const rb_machine *m);
    void      *view_user;

    /* --- for the renderer only: notes the C routines make next to what
     * they write (core/tags.h). They are no part of the original's memory
     * and change nothing the game computes. --- */
    uint16_t   vgtag[0x800];    /* what each word of vector RAM belongs to */
    uint16_t   tag;             /* the tag being written now */
    /* Which point of which object a vector goes to and how far away that
     * point is: object << 24, the point's number plus one << 16, its
     * distance (render/solid.h). 0: nothing. */
    uint32_t   vgnote[0x800];
    uint32_t   note;            /* the note being written now */
    uint8_t    note_obj;        /* the object DECODE draws now (RB_OBJ_xxx), 0: none to note */
    uint16_t   vginst[0x800];   /* which drawing of an object each word belongs to */
    uint16_t   inst;            /* counts the drawings of objects (RB_NOTED) */

    /* --- the C routines (src/game) --- */
    rb_regs    r;
    uint8_t    stk[64];         /* what PHA and PHP push */
    uint8_t    sp;
    uint8_t    main_state;      /* RB_MAIN_xxx (game/frame.h) */
    uint8_t    hung;            /* the program stopped itself (waits for the watchdog) */
    const char *not_ported;     /* a routine outside the port was called (the self test) */
    uint32_t   nmis;            /* interrupts run */
    uint32_t   frames;          /* game frames run */
    uint32_t   n_rnd;           /* random numbers read */
    uint32_t   n_mbox;          /* math box routines started */
};

void rb_machine_init(rb_machine *m);
/* The program and vector ROMs, as one image of 3000-7FFF (0x5000 bytes). */
void rb_machine_set_rom(rb_machine *m, const uint8_t *rom3000);

uint8_t rb_rd8(rb_machine *m, uint16_t addr);
void    rb_wr8(rb_machine *m, uint16_t addr, uint8_t v);

/* Little-endian words, as the 6502 keeps them. */
static inline uint16_t rb_peek16(const rb_machine *m, uint16_t addr)
{
    return (uint16_t)(m->mem[addr & 0x7fff] | (m->mem[(addr + 1) & 0x7fff] << 8));
}

#endif
