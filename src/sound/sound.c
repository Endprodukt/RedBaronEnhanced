#include "sound/sound.h"

#include "core/rb_symbols.h"

#include <math.h>
#include <string.h>

#define CPU_HZ 1512000.0

/* volume of the shot against its capacitor's charge (MAME: m_vol_lookup) */
static int16_t vol_lookup[32768];
static int     lookup_made;

void rb_sound_init(rb_sound *s)
{
    memset(s, 0, sizeof *s);
    pokey_reset(&s->pk);
    rb_samples_init(&s->smp, RB_SOUND_RATE);
    s->volume = 1.0f;
    if (!lookup_made) {
        for (int i = 0; i < 0x8000; i++) vol_lookup[0x7fff - i] = (int16_t)(0x7fff / exp(1.0 * i / 4096));
        lookup_made = 1;
    }
    for (int i = 0; i < 16; i++) {
        /* r0 = R18 and R24, r1 = open; R14 to R17 by the latch's bits */
        double r0 = 1.0 / (5600 + 680), r1 = 1 / 6e12;
        if (i & 1) r1 += 1.0 / 8200; else r0 += 1.0 / 8200;
        if (i & 2) r1 += 1.0 / 3900; else r0 += 1.0 / 3900;
        if (i & 4) r1 += 1.0 / 2200; else r0 += 1.0 / 2200;
        if (i & 8) r1 += 1.0 / 1000; else r0 += 1.0 / 1000;
        r0 = 1.0 / r0;
        r1 = 1.0 / r1;
        s->vol_crash[i] = (int16_t)(32767 * r0 / (r0 + r1));
    }
}

/* With the recordings the hum's channels are not heard (AUDC A1: the
 * engine's hum and the closing plane's tone; a tune on them is). */
static void mute_hum(rb_sound *s)
{
    int on = s->use_samples && s->smp.loaded;
    s->pk.mute = (uint8_t)(on ? (s->pk.audc[2] == 0xa1 ? 4 : 0) | (s->pk.audc[3] == 0xa1 ? 8 : 0) : 0);
}

void rb_sound_write(void *user, uint16_t addr, uint8_t value)
{
    rb_sound *s = (rb_sound *)user;
    if (addr >= RB_POKEY && addr < RB_POKEY + 0x10) {
        pokey_write(&s->pk, addr - RB_POKEY, value);
        rb_samples_pokey(&s->smp, addr - RB_POKEY, value);
        mute_hum(s);
    } else if (addr == RB_CRSHSN) {
        s->latch = value;
        rb_samples_latch(&s->smp, value);
    }
}

void rb_sound_use_samples(rb_sound *s, int on)
{
    s->use_samples = on;
    mute_hum(s);
}

/* One sample of the discrete circuits, 0..32767 (MAME's sound_stream_update). */
static int discrete(rb_sound *s)
{
    int sum = 0;

    /* polynomial shifter E5 and F4 (LS164) clocked with 12 kHz */
    s->poly_counter -= 12000;
    while (s->poly_counter <= 0) {
        s->poly_counter += RB_SOUND_RATE;
        if ((s->poly_shift & 1) == ((s->poly_shift >> 14) & 1)) s->poly_shift = (uint16_t)((s->poly_shift << 1) | 1);
        else s->poly_shift = (uint16_t)(s->poly_shift << 1);
    }

    /* the crash: the noise through a low pass, at the latch's volume */
    s->filter_counter -= 330;
    while (s->filter_counter <= 0) {
        s->filter_counter += RB_SOUND_RATE;
        s->crash_amp = (s->poly_shift & 1) ? s->latch >> 4 : 0;
    }
    sum += s->vol_crash[s->crash_amp] * 35 / 100;

    /* the shot: not active, C32 is charged; active, it discharges */
    if ((s->latch & 0x04) == 0) {
        s->shot_amp = 32767;
    } else if ((s->poly_shift & 0x8000) == 0) {
        if (s->shot_amp > 0) {
            const int C32_DISCHARGE_TIME = (int)(32767 / 0.03264);
            s->shot_amp_counter -= C32_DISCHARGE_TIME;
            while (s->shot_amp_counter <= 0) {
                s->shot_amp_counter += RB_SOUND_RATE;
                if (--s->shot_amp == 0) break;
            }
            sum += vol_lookup[s->shot_amp] * 35 / 100;
        }
    }

    /* the squeal: a 555 as pulse position modulator, 1134 Hz, slowed as C5 charges */
    if ((s->latch & 0x02) == 0) {
        s->squeal_amp = 0;
    } else {
        if (s->squeal_amp < 32767) {
            const int C5_CHARGE_TIME = (int)(32767 / 1.01728);
            s->squeal_amp_counter -= C5_CHARGE_TIME;
            while (s->squeal_amp_counter <= 0) {
                s->squeal_amp_counter += RB_SOUND_RATE;
                if (++s->squeal_amp == 32767) break;
            }
        }
        if (s->squeal_out) {
            s->squeal_off_counter -= 2268 - 1134 * s->squeal_amp / 32767;
            while (s->squeal_off_counter <= 0) {
                s->squeal_off_counter += RB_SOUND_RATE;
                s->squeal_out = 0;
            }
        } else {
            s->squeal_on_counter -= 1134;
            while (s->squeal_on_counter <= 0) {
                s->squeal_on_counter += RB_SOUND_RATE;
                s->squeal_out = 1;
            }
        }
    }
    if (s->squeal_out) sum += 32767 * 40 / 100;
    return sum;
}

void rb_sound_run(rb_sound *s, int cpu_cycles)
{
    s->frac += (double)cpu_cycles * RB_SOUND_RATE / CPU_HZ;
    while (s->frac >= 1.0) {
        s->frac -= 1.0;
        s->tick_frac += CPU_HZ / RB_SOUND_RATE;
        unsigned ticks = (unsigned)s->tick_frac;
        s->tick_frac -= ticks;
        /* MAME's mix: the POKEY at 1.0, the discrete sounds at 0.5; or the recordings */
        double v = (double)pokey_run(&s->pk, ticks) / 60.0;
        if (s->use_samples && s->smp.loaded) v += rb_samples_next(&s->smp);
        else v += 0.5 * (double)discrete(s) / 32768.0;
        s->dc += (v - s->dc) * 0.0008;          /* (the squeal's and the crash's levels are one-sided) */
        double out = (v - s->dc) * 0.6 * s->volume;
        if (out > 1.0) out = 1.0;
        if (out < -1.0) out = -1.0;
        int16_t smp = (int16_t)(out * 32000.0);
        uint32_t w = s->ring_w;
        if (w - s->ring_r >= RB_SOUND_RING) continue;       /* full: the device is behind */
        s->ring[(w % RB_SOUND_RING) * 2] = smp;
        s->ring[(w % RB_SOUND_RING) * 2 + 1] = smp;
        s->ring_w = w + 1;
    }
}

int rb_sound_read(rb_sound *s, int16_t *out, int pairs)
{
    int n = 0;
    uint32_t r = s->ring_r, w = s->ring_w;
    while (n < pairs && r != w) {
        out[2 * n] = s->ring[(r % RB_SOUND_RING) * 2];
        out[2 * n + 1] = s->ring[(r % RB_SOUND_RING) * 2 + 1];
        r++;
        n++;
    }
    s->ring_r = r;
    return n;
}
