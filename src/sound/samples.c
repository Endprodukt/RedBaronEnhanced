#include "sound/samples.h"

#include "core/rb_symbols.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *const rb_sample_names[RB_SMP_COUNT] = {
    "engine", "enemy", "spiral1", "spiral2", "spiral3",
    "shot1", "shot2", "shot3", "shot4", "shot_tail",
    "explosion1", "explosion2", "explosion3", "explosion4",
};

/* How loud each is in the mix (the loops are cut at -17 dB RMS, the
 * one-shots to -1 dB peak: tools/make_sounds.py). The POKEY's tunes reach
 * about 0.1 to 0.25. */
#define ENGINE_GAIN     0.3f
#define ENEMY_GAIN      0.7f
#define SPIRAL_GAIN     0.4f
#define SHOT_GAIN       0.3f
#define ENEMY_SHOT_GAIN 0.14f
#define TAIL_GAIN       0.28f
#define EXPL_GAIN       0.6f

#define HOLD_S          0.35        /* the hum not written for this long: the engine stops */
#define SHOT_S          0.115       /* one shot's length: no new one after it, the echo */
#define HUM_AUDC        0xa1        /* the hum and the closing plane's tone: pure, volume 1 */
#define HUM_AUDF        0xf8
#define CLOSE_AUDF      0x30        /* the tone ends when it gets this high */
#define NEAR_D          400.0f      /* what falls or blows up this near (X, Z) is at full volume, */
#define NEAR_POW        0.8f        /*   farther softer: (NEAR_D / d) ^ NEAR_POW (1000: 0.5, 4000: 0.16) */
#define FAR_MIN         0.2f        /* the farthest: this loud */

static float frand(rb_samples *s)
{
    s->seed = s->seed * 1103515245u + 12345u;
    return (float)((s->seed >> 8) & 0xffff) / 65535.0f;
}

void rb_samples_init(rb_samples *s, int rate)
{
    memset(s, 0, sizeof *s);
    s->rate = rate;
    s->engine = s->enemy = s->spiral = -1;
    s->engine_volume = 1.0f;
    s->enemy_pitch = 1.0f;
    s->crash_near = 1.0f;
    s->seed = 0x2545f491u;
}

void rb_samples_free(rb_samples *s)
{
    int rate = s->rate;
    for (int i = 0; i < RB_SMP_COUNT; i++) free(s->clip[i].pcm);
    rb_samples_init(s, rate);
}

/* A voice for a clip; the quietest is taken when all are busy. */
static int play(rb_samples *s, int clip, float vol, float pitch, int loop, float fade_in_s, int rate)
{
    const rb_clip *c = &s->clip[clip];
    if (!c->pcm) return -1;
    int k = -1;
    for (int i = 0; i < RB_SMP_VOICES && k < 0; i++)
        if (!s->v[i].clip) k = i;
    if (k < 0) {                        /* all busy: the quietest that is no loop */
        for (int i = 0; i < RB_SMP_VOICES; i++)
            if (!s->v[i].loop && (k < 0 || s->v[i].vol < s->v[k].vol)) k = i;
        if (k < 0) return -1;
    }
    rb_voice *v = &s->v[k];
    v->clip = c;
    v->pos = 0.0;
    v->step = (double)pitch * c->rate / rate;
    v->loop = loop;
    v->lp = 1.0f;
    v->lp_y = 0.0f;
    v->target = vol;
    if (fade_in_s > 0.0f) {
        v->vol = 0.0f;
        v->fade = vol / (fade_in_s * rate);
    } else {
        v->vol = vol;
        v->fade = 0.0f;
    }
    return k;
}

/* The low pass of a sound this near (1: none; far off about 800 Hz). */
static float dull(float near)
{
    return 0.08f + 0.92f * near * near;
}

/* Fade a voice to vol in fade_s seconds (to 0: it ends). */
static void fade_to(rb_samples *s, int k, float vol, float fade_s, int rate)
{
    if (k < 0) return;
    rb_voice *v = &s->v[k];
    v->target = vol;
    v->fade = fabsf(vol - v->vol) / (fade_s * rate) + 1e-7f;
}

void rb_samples_latch(rb_samples *s, uint8_t value)
{
    uint8_t was = s->latch;
    s->latch = value;
    if (!s->loaded) return;
    const int rate = s->rate;
    /* the shot rose: one shot */
    if ((value & 0x04) && !(was & 0x04)) {
        int k = RB_SMP_SHOT1 + (int)(frand(s) * RB_SMP_SHOTS) % RB_SMP_SHOTS;
        float pitch = s->player_fires ? 0.97f + 0.06f * frand(s) : 0.88f + 0.04f * frand(s);
        play(s, k, s->player_fires ? SHOT_GAIN : ENEMY_SHOT_GAIN, pitch, 0, 0.0f, rate);
        s->shot_at = s->now + 1;
        s->shot_player = s->player_fires;
        if (s->player_fires) s->n_shots++;
        else s->n_enemy_shots++;
    }
    /* the crash's volume rose: an explosion, as loud */
    if ((value >> 4) > (was >> 4)) {
        int k = RB_SMP_EXPL1 + (int)(frand(s) * RB_SMP_EXPLS) % RB_SMP_EXPLS;
        int v = play(s, k, EXPL_GAIN * s->crash_near * (float)(value >> 4) / 15.0f, 0.9f + 0.2f * frand(s), 0,
                     0.0f, rate);
        if (v >= 0) s->v[v].lp = dull(s->crash_near);
        s->n_explosions++;
    }
}

void rb_samples_pokey(rb_samples *s, int reg, uint8_t value)
{
    switch (reg) {
    case 4:                             /* AUDF3: the hum, or the closing plane's tone */
        s->audf3 = value;
        if (s->audc3 == HUM_AUDC && value != HUM_AUDF) {
            float close = (float)(HUM_AUDF - value) / (float)(HUM_AUDF - CLOSE_AUDF);
            if (close < 0.0f) close = 0.0f;
            if (close > 1.0f) close = 1.0f;
            s->enemy_pitch = 0.85f + 0.6f * close;
            s->enemy_vol = ENEMY_GAIN * (0.25f + 0.75f * close);
            s->enemy_at = s->now + 1;
        }
        break;
    case 5:                             /* AUDC3: the hum on (or a tune on its channel: playing) */
        s->audc3 = value;
        if (value & 0x0f) s->hum_at = s->now + 1;
        break;
    case 7:
        s->audc4 = value;
        break;
    }
}

void rb_samples_game(rb_samples *s, const uint8_t *mem)
{
    /* the climb as the cockpit's airspeed shows it (render/cockpit.c) */
    float climb = (float)(int16_t)(mem[RB_PLDELY] | (mem[RB_PLDELY + 1] << 8));
    float d = -climb / 460.0f;
    s->speed = d < -1.0f ? -1.0f : d > 1.0f ? 1.0f : d;
    s->player_fires = (mem[RB_GAMODE] & 0x80) && (mem[RB_SWSTAT] & 0x80) && !(mem[RB_GUN_ST] & 0x40);

    /* how near what falls or blows up is: the player's own end is near; else
     * the nearest motion object (plane, drones, blimp) hit (status D6), by
     * its X and Z; none: something on the ground */
    float near = -1.0f;
    if (mem[RB_GMEND1] & 0x80) {
        near = 1.0f;
    } else {
        for (int o = 0; o < 3; o++) {
            uint16_t b = (uint16_t)(RB_PLOBDB + o * RB_L_OBJ);
            if (!(mem[b + 6] & 0x40)) continue;
            float x = (float)(int16_t)(mem[b] | (mem[b + 1] << 8));
            float z = (float)(int16_t)(mem[b + 4] | (mem[b + 5] << 8));
            float g = powf(NEAR_D / (hypotf(x, z) + 1.0f), NEAR_POW);
            g = g < FAR_MIN ? FAR_MIN : g > 1.0f ? 1.0f : g;
            if (g > near) near = g;
        }
        if (near < 0.0f) near = 0.6f;
    }
    s->crash_near = near;
}

static float voice_next(rb_samples *s, int k)
{
    rb_voice *v = &s->v[k];
    const rb_clip *c = v->clip;
    int i = (int)v->pos;
    if (i >= c->frames) {
        if (!v->loop) { v->clip = NULL; return 0.0f; }
        v->pos -= c->frames;
        i = (int)v->pos;
    }
    float f = (float)(v->pos - i);
    int j = i + 1 < c->frames ? i + 1 : (v->loop ? 0 : i);
    float x = (c->pcm[i] + f * (c->pcm[j] - c->pcm[i])) * (1.0f / 32768.0f);
    if (v->lp < 1.0f) x = v->lp_y += v->lp * (x - v->lp_y);
    v->pos += v->step;
    if (v->vol < v->target) {
        v->vol += v->fade;
        if (v->vol > v->target) v->vol = v->target;
    } else if (v->vol > v->target) {
        v->vol -= v->fade;
        if (v->vol <= v->target) {
            v->vol = v->target;
            if (v->target <= 0.0f) { v->clip = NULL; return 0.0f; }
        }
    }
    return x * v->vol;
}

/* A looped voice of the clip: on, at vol and pitch; or off (faded out). */
static void keep(rb_samples *s, int *k, int clip, int on, float vol, float pitch, float fade_s, int rate)
{
    if (*k >= 0 && s->v[*k].clip != &s->clip[clip]) *k = -1;     /* (it ended, the voice taken) */
    if (on) {
        if (*k < 0) {
            *k = play(s, clip, vol, pitch, 1, fade_s, rate);
            if (*k >= 0) {
                if (clip == RB_SMP_ENGINE) s->n_engine++;
                else s->n_closing++;
            }
        }
        else if (s->v[*k].target != vol) fade_to(s, *k, vol, fade_s, rate);
        if (*k >= 0) s->v[*k].step = (double)pitch * s->clip[clip].rate / rate;
    } else if (*k >= 0) {
        if (s->v[*k].target > 0.0f) fade_to(s, *k, 0.0f, fade_s, rate);
    }
}

float rb_samples_next(rb_samples *s)
{
    const int rate = s->rate;
    s->now++;
    if (!s->loaded) return 0.0f;

    /* (the loops' pitch and volume follow every 64 samples) */
    if ((s->now & 63) == 0) {
        const float dt = 64.0f / rate;
        uint64_t hold = (uint64_t)(HOLD_S * rate);
        s->speed_now += (s->speed - s->speed_now) * (dt / 0.5f);

        int hum = s->hum_at && s->now - s->hum_at < hold;
        keep(s, &s->engine, RB_SMP_ENGINE, hum, ENGINE_GAIN * s->engine_volume * (1.0f + 0.2f * s->speed_now),
             1.0f + 0.12f * s->speed_now, 0.3f, rate);

        int close = hum && s->enemy_at && s->now - s->enemy_at < hold;
        static const float glide = 0.15f;
        if (close && s->enemy >= 0 && s->v[s->enemy].clip == &s->clip[RB_SMP_ENEMY]) {
            double want = (double)s->enemy_pitch * s->clip[RB_SMP_ENEMY].rate / rate;
            rb_voice *v = &s->v[s->enemy];
            v->step += (want - v->step) * (dt / glide);
            if (v->target != s->enemy_vol) fade_to(s, s->enemy, s->enemy_vol, 0.1f, rate);
        } else {
            keep(s, &s->enemy, RB_SMP_ENEMY, close, s->enemy_vol, s->enemy_pitch, 0.25f, rate);
        }

        /* the spiral: each one different (which, its pitch, how far and how
         * fast it falls, where in the loop it begins); its pitch falls as the
         * squeal's; as loud and as dull as the plane is near */
        int spiral = (s->latch & 0x02) != 0;
        if (s->spiral >= 0 && s->v[s->spiral].clip != &s->clip[s->spiral_clip]) s->spiral = -1;
        if (spiral && s->spiral < 0 && s->spiral_t == 0.0) {
            int k = RB_SMP_SPIRAL1 + (int)(frand(s) * RB_SMP_SPIRALS) % RB_SMP_SPIRALS;
            if (k == s->spiral_clip) k = RB_SMP_SPIRAL1 + (k - RB_SMP_SPIRAL1 + 1) % RB_SMP_SPIRALS;
            s->spiral_clip = k;
            s->spiral_pitch = 0.9f + 0.22f * frand(s);
            s->spiral_fall = 0.15f + 0.2f * frand(s);
            s->spiral_fall_s = 3.0f + 2.0f * frand(s);
            s->spiral = play(s, k, SPIRAL_GAIN * s->crash_near, s->spiral_pitch, 1, 0.05f, rate);
            if (s->spiral >= 0) {
                s->v[s->spiral].pos = frand(s) * (float)(s->clip[k].frames - 1);
                s->v[s->spiral].lp = dull(s->crash_near);
                s->n_spirals++;
            }
        }
        s->spiral_t = spiral ? s->spiral_t + dt : 0.0;
        if (s->spiral >= 0) {
            rb_voice *v = &s->v[s->spiral];
            if (spiral) {
                float fall = (float)(s->spiral_t / s->spiral_fall_s);
                float pitch = s->spiral_pitch * (1.0f - s->spiral_fall * (fall > 1.0f ? 1.0f : fall));
                v->step = (double)pitch * v->clip->rate / rate;
                /* (and it fades a little as it falls away) */
                float want = SPIRAL_GAIN * s->crash_near * (1.0f - 0.4f * (fall > 1.0f ? 1.0f : fall));
                if (v->target != want) fade_to(s, s->spiral, want, 0.15f, rate);
                v->lp += (dull(s->crash_near) - v->lp) * (dt / 0.15f);
            } else if (v->target > 0.0f) {
                fade_to(s, s->spiral, 0.0f, 0.4f, rate);
            }
        }

        /* the guns stopped: their echo */
        if (s->shot_at && s->now - s->shot_at > (uint64_t)(SHOT_S * rate)) {
            play(s, RB_SMP_SHOT_TAIL, s->shot_player ? TAIL_GAIN : TAIL_GAIN * 0.5f, 1.0f, 0, 0.0f, rate);
            s->shot_at = 0;
        }
    }

    float sum = 0.0f;
    for (int k = 0; k < RB_SMP_VOICES; k++)
        if (s->v[k].clip) sum += voice_next(s, k);
    return sum;
}
