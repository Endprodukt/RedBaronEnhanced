#include "platform/audio.h"

#include <SDL.h>
#include <vectorpie/vp.h>

/* the SDK's Ogg Vorbis decoder (compiled in the SDK; here its declarations) */
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_AudioDeviceID g_dev;
static int16_t g_last[2];

static void SDLCALL fill(void *user, Uint8 *stream, int len)
{
    rb_sound *snd = (rb_sound *)user;
    int16_t *out = (int16_t *)stream;
    int want = len / 4;                 /* pairs of left and right */
    int got = rb_sound_read(snd, out, want);
    if (got > 0) { g_last[0] = out[2 * got - 2]; g_last[1] = out[2 * got - 1]; }
    /* Run dry (the game stalled or is just starting): let the last level
     * die away instead of clicking. */
    for (int i = got; i < want; i++)
        for (int k = 0; k < 2; k++) {
            g_last[k] = (int16_t)(g_last[k] - g_last[k] / 64);
            out[2 * i + k] = g_last[k];
        }
}

int rb_audio_open(rb_sound *snd)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "redbaron: no sound: %s\n", SDL_GetError());
        return -1;
    }
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    want.freq = RB_SOUND_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = fill;
    want.userdata = snd;
    g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!g_dev) {
        fprintf(stderr, "redbaron: no sound device: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioDevice(g_dev, 0);
    return 0;
}

void rb_audio_close(void)
{
    if (g_dev) SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
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

void rb_audio_balance(rb_sound *snd)
{
    /* More than a quarter of a second waiting: the device is behind. Drop
     * what is oldest, down to a tenth of a second. */
    uint32_t waiting = snd->ring_w - snd->ring_r;
    if (waiting > RB_SOUND_RATE / 4) {
        if (g_dev) SDL_LockAudioDevice(g_dev);
        snd->ring_r = snd->ring_w - RB_SOUND_RATE / 10;
        if (g_dev) SDL_UnlockAudioDevice(g_dev);
    }
}
