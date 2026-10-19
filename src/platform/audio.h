/* audio.h - the sound's samples to the sound device.
 *
 * The SDK's vp_audio plays files and ready samples; this sound is made as
 * the game runs, so it has an SDL audio device of its own that pulls from
 * the ring of sound/sound.h. With no sound device the game runs silent.
 */
#ifndef RB_AUDIO_H
#define RB_AUDIO_H

#include "sound/sound.h"

/* Returns 0, or -1 if no device could be opened (a message is printed). */
int  rb_audio_open(rb_sound *snd);
void rb_audio_close(void);

/* Load the recordings (sound/samples.h) from the package's sounds folder
 * (Ogg Vorbis, decoded once, mixed to mono). Returns how many are missing
 * (a message is printed for each); with any missing the circuits sound. */
int  rb_audio_load_samples(rb_samples *smp);

/* Keep the ring from running full when the device takes samples a little
 * slower than the game makes them: call once per picture. */
void rb_audio_balance(rb_sound *snd);

#endif
