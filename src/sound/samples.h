/* samples.h - the sounds as recordings (setting `sounds = samples`).
 *
 * The game drives its sounds as on the board: the latch at 1808 (the crash's
 * volume, the shot, the spiral) and the POKEY (the engine's hum, the tone of
 * a plane coming at the player, the tunes). Here the same signals start
 * recordings instead (package/sounds, made by tools/make_sounds.py):
 *
 *   the hum (POKEY channels 3 and 4 at AUDC A1, AUDF F8)   the own engine, a
 *       Fokker's: looped, faster and louder diving, slower climbing
 *   AUDF 3 below F8 (the plane coming at the player)       a Nieuport's rotary,
 *       higher and louder as the tone rises
 *   the shot rising (D2, 15 times a second while firing)   one shot of a Maxim
 *       (the gun of the German LMG 08/15); its echo when the guns stop; the
 *       enemy's guns quieter
 *   the crash's volume rising (D7-D4)                      an explosion, as loud
 *   the spiral (D1)                                        a plane spinning down
 *       (one of three, its pitch and how it falls varied), its pitch falling
 *       as the squeal's does
 *
 * A plane spiralling down and what blows up are as loud as they are near
 * (the falling or exploding motion object's X and Z, PLOBDB), and duller
 * far off (a low pass).
 *
 * The tunes stay the POKEY's. Only the sound: the game is not touched.
 */
#ifndef RB_SAMPLES_H
#define RB_SAMPLES_H

#include <stdint.h>

enum {
    RB_SMP_ENGINE = 0, RB_SMP_ENEMY, RB_SMP_SPIRAL1, RB_SMP_SPIRAL2, RB_SMP_SPIRAL3,
    RB_SMP_SHOT1, RB_SMP_SHOT2, RB_SMP_SHOT3, RB_SMP_SHOT4, RB_SMP_SHOT_TAIL,
    RB_SMP_EXPL1, RB_SMP_EXPL2, RB_SMP_EXPL3, RB_SMP_EXPL4,
    RB_SMP_COUNT
};
#define RB_SMP_SPIRALS 3
#define RB_SMP_SHOTS 4
#define RB_SMP_EXPLS 4
#define RB_SMP_VOICES 24

/* The file of each sound (package/sounds/<name>.ogg). */
extern const char *const rb_sample_names[RB_SMP_COUNT];

typedef struct rb_clip {
    int16_t *pcm;               /* mono; NULL: not loaded */
    int      frames, rate;
} rb_clip;

typedef struct rb_voice {
    const rb_clip *clip;        /* NULL: free */
    double   pos, step;         /* in the clip's frames; step = pitch * rate / output rate */
    float    vol, target, fade; /* volume, where it goes, by how much per output sample */
    float    lp, lp_y;          /* a low pass (far off): its coefficient (1: none), its state */
    int      loop;
} rb_voice;

typedef struct rb_samples {
    rb_clip  clip[RB_SMP_COUNT];
    int      loaded;            /* every clip there */
    int      rate;              /* of the output */
    rb_voice v[RB_SMP_VOICES];
    int      engine, enemy, spiral;     /* their voices (looped), -1: none */
    float    engine_volume;     /* 0..1, the setting */

    /* what the board was told */
    uint8_t  latch;
    uint8_t  audf3, audc3, audc4;
    uint64_t now;               /* output samples made */
    uint64_t hum_at, enemy_at;  /* when the hum, the enemy's tone was last written */
    uint64_t shot_at;           /* when the shot last rose; 0: the guns are quiet */
    int      shot_player;       /* it was the player's */
    double   spiral_t;          /* seconds the spiral has sounded */
    int      spiral_clip;       /* this spiral's: RB_SMP_SPIRALx, its pitch, how far and how fast it falls */
    float    spiral_pitch, spiral_fall, spiral_fall_s;

    /* from the game's memory (rb_samples_game) */
    float    speed;             /* -1 climbing .. 1 diving */
    float    speed_now;         /* (smoothed) */
    int      player_fires;
    float    crash_near;        /* how near what falls or blows up is: 1 near .. 0.15 far */
    float    enemy_pitch, enemy_vol;    /* where the enemy's sound goes */
    unsigned seed;

    /* how often each began (for tools/rbshot) */
    unsigned n_shots, n_enemy_shots, n_explosions, n_spirals, n_closing, n_engine;
} rb_samples;

/* rate: of the output. The clips are loaded by the platform (platform/audio.h). */
void rb_samples_init(rb_samples *s, int rate);
void rb_samples_free(rb_samples *s);

/* A write to the latch (1808) or the POKEY (reg 0-15), as the board gets it. */
void rb_samples_latch(rb_samples *s, uint8_t value);
void rb_samples_pokey(rb_samples *s, int reg, uint8_t value);

/* What the sound cannot hear from the board, from the game's memory: is the
 * player diving or climbing, is it the player who fires. Once a frame or so. */
void rb_samples_game(rb_samples *s, const uint8_t *mem);

/* The next output sample, about -1..1. */
float rb_samples_next(rb_samples *s);

#endif
