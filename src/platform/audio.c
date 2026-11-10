#include "platform/audio.h"

#include <vectorpie/vp.h>

/* the SDK's Ogg Vorbis decoder (compiled in the SDK; here its declarations) */
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vp_audio *g_audio;
static vp_voice  g_stream;

int rb_audio_open(rb_sound *snd)
{
    (void)snd;
    if (vp_audio_open(&g_audio) != VP_OK) {
        fprintf(stderr, "redbaron: no sound: %s\n", vp_last_error());
        g_audio = NULL;
        return -1;
    }
    g_stream = vp_stream_play(g_audio, RB_SOUND_RATE, 1.0f);
    if (!g_stream) {
        fprintf(stderr, "redbaron: no sound stream: %s\n", vp_last_error());
        vp_audio_close(g_audio);
        g_audio = NULL;
        return -1;
    }
    return 0;
}

void rb_audio_close(void)
{
    if (g_audio) vp_audio_close(g_audio);
    g_audio = NULL;
    g_stream = 0;
}

int rb_audio_load_samples(rb_samples *smp)
{
    int missing = 0;
    for (int i = 0; i < RB_SMP_COUNT; i++) {
        char rel[64], path[1024];
        snprintf(rel, sizeof rel, "sounds/%s.ogg", rb_sample_names[i]);
        vp_asset_path(rel, path, sizeof path);
        int channels = 0, rate = 0;
        short *pcm = NULL;
        int frames = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
        if (frames <= 0 || !pcm || channels < 1) {
            fprintf(stderr, "redbaron: sound %s missing or unreadable\n", path);
            free(pcm);
            missing++;
            continue;
        }
        /* (mono: the cabinet has one speaker) */
        int16_t *mono = (int16_t *)malloc((size_t)frames * sizeof *mono);
        if (!mono) { free(pcm); missing++; continue; }
        for (int f = 0; f < frames; f++) {
            int sum = 0;
            for (int c = 0; c < channels; c++) sum += pcm[f * channels + c];
            mono[f] = (int16_t)(sum / channels);
        }
        free(pcm);
        free(smp->clip[i].pcm);
        smp->clip[i].pcm = mono;
        smp->clip[i].frames = frames;
        smp->clip[i].rate = rate;
    }
    smp->loaded = missing == 0;
    return missing;
}

void rb_audio_pump(rb_sound *snd)
{
    /* (in pairs of left and right) The game makes a sixtieth of a second
     * a picture; the SDK wants 50 to 100 ms written ahead. Above a sixth of
     * a second waiting the device is behind: what comes is dropped. */
    const int MOST = RB_SOUND_RATE / 6, LOW = RB_SOUND_RATE / 60, FILL = RB_SOUND_RATE * 6 / 100;
    static int16_t pcm[2 * 2048];
    static float out[2 * 2048];
    static float last[2];
    int queued = g_audio ? vp_stream_queued(g_audio, g_stream) : MOST;
    int got;
    while ((got = rb_sound_read(snd, pcm, 2048)) > 0) {
        if (!g_audio || queued >= MOST) continue;
        for (int i = 0; i < 2 * got; i++) out[i] = pcm[i] * (1.0f / 32768.0f);
        last[0] = out[2 * got - 2];
        last[1] = out[2 * got - 1];
        queued += vp_stream_write(g_audio, g_stream, out, got);
    }
    /* Nearly dry (just started, or the game stalled): the last level dies
     * away over enough to be ahead again, instead of a click and gaps. */
    if (g_audio && queued < LOW) {
        int n = FILL - queued;
        if (n > 2048) n = 2048;
        for (int i = 0; i < n; i++)
            for (int k = 0; k < 2; k++) {
                last[k] -= last[k] / 64.0f;
                out[2 * i + k] = last[k];
            }
        vp_stream_write(g_audio, g_stream, out, n);
    }
}
