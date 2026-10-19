/* rbnmi - check the interrupt routine in C against the original's.
 *
 *   rbnmi CAPTURE.rbcap [--every N] [--nmis N] [--max N]
 *
 * rbcompare leaves out what the interrupt writes (the stick's pots, the
 * switches, coins and credits, the clocks, the score counting up, the
 * tunes, the propellers): the interrupt came at other moments in MAME
 * than it does here. This holds the interrupt itself, routine against
 * routine: from the memory of every Nth frame of a capture (default 25)
 * two machines run N interrupts each (default 400), one the C routine
 * (rb_nmi), one the original's NMI code on the 6502 core, with the same
 * switches and stick (coins dropped, the slam switch hit, start and fire
 * pressed, the stick moved, by a fixed pattern), the same random numbers,
 * and the main line's part played the same for both (the frame flags
 * taken, points put up to be counted, sounds started). After every
 * interrupt both must hold the same memory and math box and have written
 * the same to the POKEY, the sound latch, the coin counters and the vector
 * generator.
 *
 * It also holds the power on (PWRON, START and the first pass of MAIN, up
 * to INTWAIT) in C against the original's.
 *
 * Exit status 0 when all are the same.
 */
#include "capture/capture.h"
#include "core/emu.h"
#include "core/machine.h"
#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct side {
    rb_machine m;
    uint32_t   rnd;             /* the random numbers' place */
    uint32_t   writes;          /* a running checksum of the sound writes */
    uint32_t   nwrites;
} side;

static uint8_t next_rnd(void *u)
{
    side *s = (side *)u;
    s->rnd = s->rnd * 1103515245u + 12345u;
    return (uint8_t)(s->rnd >> 16);
}

static void on_sound(void *u, uint16_t addr, uint8_t v)
{
    side *s = (side *)u;
    s->writes = s->writes * 31u + addr * 257u + v;
    s->nwrites++;
}

static void hook(side *s, uint32_t seed)
{
    s->m.prng = next_rnd;
    s->m.prng_user = s;
    s->m.sound_write = on_sound;
    s->m.sound_user = s;
    s->rnd = seed;
    s->writes = s->nwrites = 0;
}

static void load(side *s, const rbcap_reader *cap, const rbcap_frame *f)
{
    rb_machine_init(&s->m);
    rb_machine_set_rom(&s->m, cap->rom);
    memcpy(s->m.mem, f->ram, RBCAP_RAM_SIZE);
    memcpy(s->m.mem + 0x2000, f->vgram, RBCAP_VGRAM_SIZE);
    memcpy(s->m.mb.reg, f->mb_reg, sizeof s->m.mb.reg);
    s->m.mb.result = f->mb_result;
    hook(s, f->frame * 2654435761u);
}

/* The switches and the stick at interrupt i of the run that starts at frame f. */
static void switches(rb_machine *m, uint32_t f, int i)
{
    uint8_t in0 = 0x3f, in4 = RB_IN4_START;     /* (start is low when pressed) */
    unsigned kind = f / 25u;
    /* a coin in one of the three mechs: closed 30 interrupts (120 ms), now and then too short or too long */
    int mech = (int)(kind % 3u);
    int len = (kind % 7u) == 3 ? 3 : (kind % 11u) == 5 ? 260 : 30;
    if (i >= 20 && i < 20 + len) in0 &= (uint8_t)~(1u << mech);
    if ((kind % 4u) == 1 && i >= 120 && i < 150) in0 &= (uint8_t)~(1u << ((mech + 1) % 3));
    /* a second coin into the same mech right behind the first: its credit comes early */
    if ((kind % 6u) == 4 && i >= 70 && i < 100) in0 &= (uint8_t)~(1u << mech);
    /* the slam switch, sometimes right after the coin */
    if ((kind % 5u) == 2 && i >= 60 && i < 64) in0 &= (uint8_t)~RB_IN0_SLAM;
    /* start, late enough for the coin to have become a credit */
    if ((kind % 2u) == 0 && i >= 250 && i < 262) in4 &= (uint8_t)~RB_IN4_START;
    if (i & 64) in4 |= RB_IN4_FIRE;
    m->in.in0 = in0;
    m->in.in4 = in4;
    /* the stick: wanders, with jumps beyond what the routine takes for noise */
    unsigned t = (unsigned)i + kind * 7u;
    m->in.pot_x = (uint8_t)(0x80 + (int)((t * 5u) % 120u) - 60 + ((t / 31u) % 3u) * 9);
    m->in.pot_y = (uint8_t)(0x80 - (int)((t * 3u) % 100u) + 50 + ((t / 17u) % 2u) * 30);
    m->vg_left = (i % 5) == 4 ? 1000 : 0;       /* the vector generator: halted most of the time */
}

/* The main line's part, the same for both. */
static void main_line(rb_machine *m, uint32_t f, int i)
{
    unsigned kind = f / 25u;
    /* INTWAIT takes the frame flags; MAIN sets and clears S.FLAG */
    if (m->mem[RB_CALFLG]) { m->mem[RB_CALFLG] >>= 1; m->mem[RB_S_FLAG] ^= 0xff; }
    if (i % 9 == 8) m->mem[RB_INTFLG] >>= 1;
    if (i % 50 == 0) m->mem[RB_FRAME]++;
    m->mem[RB_CMODE] = (uint8_t)(kind * 37u);       /* every coin mode in turn */
    /* points to count up, sometimes many; near a bonus life now and then */
    if (i % 131 == 40) m->mem[RB_SCRTAB + 3] = (uint8_t)(1 + (kind % 5u) * 7u);
    if (i == 10 && (kind % 4u) == 3) { m->mem[RB_SCORE + 1] = 0x05; m->mem[RB_SCORE] = 0x98; }
    /* a shot, an explosion, the spiral: what the interrupt puts on the sound latch */
    if (i % 61 == 5) m->mem[RB_S_VAL] = 3;
    if (i % 113 == 7) m->mem[RB_EXPVAL] = 0xf0;
    if (i % 113 == 60) m->mem[RB_EXPVAL] = 0;
    if (i % 97 == 3) m->mem[RB_SPIRAL] ^= 2;
    if (i % 211 == 100) m->mem[RB_SLAMSN] ^= 0xf6;
}

static int emu_nmi(rb_emu *e)
{
    m6502 *c = &e->cpu;
    c->pc = RB_INTWAIT;
    c->s = 0xfd;
    c->p = M6502_U;
    c->a = 0x11; c->x = 0x22; c->y = 0x33;
    c->jammed = 0;
    m6502_nmi(c);
    for (int n = 0; n < 200000; n++) {
        m6502_step(c);
        if (c->s == 0xfd && c->pc == RB_INTWAIT) return c->a == 0x11 && c->x == 0x22 && c->y == 0x33 ? 0 : -2;
    }
    return -1;
}

/* Power on in C against the original's: from reset until the main line
 * waits in INTWAIT for the first time. Returns the number of bytes that
 * differ. */
static int pwron_check(const rbcap_reader *cap)
{
    static side a, b;
    static rb_emu e;
    rb_machine_init(&a.m);
    rb_machine_set_rom(&a.m, cap->rom);
    hook(&a, 12345);
    rb_power_on(&a.m);
    rb_machine_init(&b.m);
    rb_machine_set_rom(&b.m, cap->rom);
    hook(&b, 12345);
    rb_emu_init(&e, &b.m);
    int n;
    for (n = 0; n < 4000000 && e.cpu.pc != RB_INTWAIT; n++) m6502_step(&e.cpu);
    if (e.cpu.pc != RB_INTWAIT) return -1;
    int nd = 0;
    for (unsigned i = 0; i < 0x3000; i++) {
        if (i >= 0x180 && i < 0x200) continue;      /* the stack */
        if (i >= 0x400 && i < 0x2000) continue;
        if (a.m.mem[i] != b.m.mem[i]) {
            if (nd < 8) printf("    power on: %04X here %02X  original %02X\n", i, a.m.mem[i], b.m.mem[i]);
            nd++;
        }
    }
    nd += memcmp(a.m.pokey, b.m.pokey, sizeof a.m.pokey) != 0;
    nd += a.writes != b.writes || a.rnd != b.rnd;
    nd += memcmp(&a.m.mb, &b.m.mb, sizeof a.m.mb) != 0;
    nd += a.m.main_state != RB_MAIN_WAIT_CAL || a.m.hung || a.m.not_ported;
    return nd;
}

static const char *arg(int argc, char **argv, const char *name)
{
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], name)) return argv[i + 1];
    return NULL;
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { i++; continue; }
        path = argv[i];
    }
    if (!path) {
        fprintf(stderr, "usage: rbnmi CAPTURE.rbcap [--every N] [--nmis N] [--max N]\n");
        return 2;
    }
    uint32_t every = (uint32_t)atoi(arg(argc, argv, "--every") ? arg(argc, argv, "--every") : "25");
    int nmis = atoi(arg(argc, argv, "--nmis") ? arg(argc, argv, "--nmis") : "400");
    int max_report = atoi(arg(argc, argv, "--max") ? arg(argc, argv, "--max") : "5");

    rbcap_reader cap;
    if (rbcap_open(&cap, path) != 0) {
        fprintf(stderr, "rbnmi: cannot read %s\n", path);
        return 2;
    }
    static side nat, org;
    static rb_emu emu;
    static rbcap_frame fr;
    static uint8_t payload[0x40000];
    char tag[4];
    uint32_t len, runs = 0, bad = 0, total = 0, credits = 0, scored = 0, hung = 0;
    int rc, reported = 0, pwron_done = 0;

    while ((rc = rbcap_next_chunk(&cap, tag, payload, sizeof payload, &len)) == 1) {
        if (memcmp(tag, "FRAM", 4) != 0 || len != RBCAP_FRAM_SIZE || !cap.have_rom) continue;
        rbcap_parse_frame(payload, &fr);
        if (!pwron_done) {
            int nd = pwron_check(&cap);
            printf("%s: power on (PWRON to INTWAIT): %s\n", path,
                   nd == 0 ? "the same" : nd < 0 ? "the original did not reach INTWAIT" : "DIFFERS");
            if (nd) bad++;
            pwron_done = 1;
        }
        if (every == 0 || fr.frame % every != 0) continue;
        load(&nat, &cap, &fr);
        load(&org, &cap, &fr);
        rb_emu_init(&emu, &org.m);
        runs++;
        uint8_t credit0 = nat.m.mem[RB_CRDT];
        uint16_t score0 = (uint16_t)(nat.m.mem[RB_SCORE] | (nat.m.mem[RB_SCORE + 1] << 8));
        for (int i = 0; i < nmis; i++) {
            switches(&nat.m, fr.frame, i);
            switches(&org.m, fr.frame, i);
            rb_nmi(&nat.m);
            if (nat.m.hung) { hung++; break; }      /* (the routine stops itself: the stack check, a lost frame) */
            int er = emu_nmi(&emu);
            total++;
            int nd = memcmp(nat.m.mem, org.m.mem, 0x180) != 0 ||
                     memcmp(nat.m.mem + 0x200, org.m.mem + 0x200, 0x200) != 0 ||
                     memcmp(nat.m.mem + 0x2000, org.m.mem + 0x2000, 0x1000) != 0 ||
                     memcmp(&nat.m.mb, &org.m.mb, sizeof nat.m.mb) != 0 ||
                     nat.writes != org.writes || nat.nwrites != org.nwrites ||
                     nat.m.vg_starts != org.m.vg_starts || nat.m.coin_counter != org.m.coin_counter ||
                     nat.m.sound_latch != org.m.sound_latch || nat.rnd != org.rnd;
            if (nd || er) {
                bad++;
                if (reported++ < max_report) {
                    printf("frame %u, interrupt %d: differs%s\n", fr.frame, i,
                           er == -1 ? " (the original's did not return)" : er == -2 ? " (the original's did not restore the registers)" : "");
                    int shown = 0;
                    for (unsigned a = 0; a < 0x400 && shown < 10; a++) {
                        if (a >= 0x180 && a < 0x200) continue;
                        if (nat.m.mem[a] != org.m.mem[a]) {
                            printf("    %04X here %02X  original %02X\n", a, nat.m.mem[a], org.m.mem[a]);
                            shown++;
                        }
                    }
                    if (nat.nwrites != org.nwrites || nat.writes != org.writes)
                        printf("    sound writes: here %u, original %u\n", nat.nwrites, org.nwrites);
                    if (nat.m.sound_latch != org.m.sound_latch)
                        printf("    sound latch: here %02X, original %02X\n", nat.m.sound_latch, org.m.sound_latch);
                    if (nat.m.vg_starts != org.m.vg_starts)
                        printf("    VG starts: here %u, original %u\n", nat.m.vg_starts, org.m.vg_starts);
                    if (nat.rnd != org.rnd) printf("    random numbers read differ\n");
                }
                break;      /* (what follows a difference differs too) */
            }
            main_line(&nat.m, fr.frame, i);
            main_line(&org.m, fr.frame, i);
        }
        if (nat.m.mem[RB_CRDT] != credit0) credits++;
        if ((uint16_t)(nat.m.mem[RB_SCORE] | (nat.m.mem[RB_SCORE + 1] << 8)) != score0) scored++;
    }
    rbcap_close(&cap);
    printf("%s: the interrupt, %u runs, %u interrupts: %u run(s) differ (%u changed the credits, %u the score)\n",
           path, runs, total, bad, credits, scored);
    if (hung) printf("  %u run(s) ended in the routine's own stop\n", hung);
    return bad ? 1 : 0;
}
