/* audio.h - the sound's samples to the sound device.
 *
 * The sound is made as the game runs (sound/sound.h: into its ring). It
 * goes out through the SDK's mixer as the game's own stream
 * (vp_stream_play): on the cabinet the VectorPie menu's MUSIC VOLUME then
 * applies to it (the SDK mixes every game's streams at that level; its
 * MASTER VOLUME is the system's), on top of the game's own VOLUME. With no
 * sound device the game runs silent.
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

/* Hand what the ring holds to the stream, keeping about a tenth of a second
 * queued there: when the device takes samples a little slower than the
 * game makes them, what is too much is dropped. Call once per picture. */
void rb_audio_pump(rb_sound *snd);

#endif
