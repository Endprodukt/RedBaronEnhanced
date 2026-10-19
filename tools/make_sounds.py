"""make_sounds.py - the sounds of the setting `sounds = samples`, cut from
the Freesound recordings (assets/freesound_raw, see assets/freesound_raw/
SOURCES.txt) into package/sounds.

Every sound mono (the cabinet has one speaker), Ogg Vorbis, at the lowest
rate that keeps what is in it:

  16 kHz  the 1930s optical recordings (engines, the spiral, explosions):
          above 8 kHz they hold only the film's hiss (measured: the bands
          above 8 kHz sit flat at the noise floor, 35-45 dB down)
  32 kHz  the machine gun, a modern recording whose crack reaches 16 kHz

Loops are cut with a crossfade of their end into their start, so they run
seamless. One-shots are trimmed where they have died away, and faded.

usage (from the project's folder):  py -3.10 -I tools/make_sounds.py
needs numpy, scipy, soundfile (libsndfile with Vorbis).
"""
import os
import sys
from math import gcd

import numpy as np
import soundfile as sf
from scipy.signal import butter, resample_poly, sosfiltfilt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, 'assets', 'freesound_raw')
OUT = os.path.join(ROOT, 'package', 'sounds')
QUALITY = 0.55          # libsndfile's compression level for Vorbis: 0 best .. 1 smallest


def load(name):
    """The original (WAV) if it is there, else Freesound's preview (Ogg)."""
    path = os.path.join(RAW, name + '.wav')
    if not os.path.exists(path): path = os.path.join(RAW, name + '.ogg')
    x, r = sf.read(path, always_2d=True)
    return x.mean(axis=1), r


def resample(y, r, rate):
    g = gcd(int(r), int(rate))
    return resample_poly(y, rate // g, r // g)


def highpass(y, rate, hz=45.0):
    return sosfiltfilt(butter(2, hz, 'highpass', fs=rate, output='sos'), y)


def fade(y, rate, t_in, t_out):
    y = y.copy()
    n = int(t_in * rate)
    if n: y[:n] *= np.sin(np.linspace(0, np.pi / 2, n)) ** 2
    n = int(t_out * rate)
    if n: y[-n:] *= np.cos(np.linspace(0, np.pi / 2, n)) ** 2
    return y


def rms_db(y):
    return 20 * np.log10(np.sqrt(np.mean(y ** 2)) + 1e-12)


def loop(name, start, length, rate, xfade=0.5, level=-18.0):
    """A seamless loop of `length` seconds from `start`: the part after its
    end is crossfaded into its beginning (equal power)."""
    y, r = load(name)
    y = highpass(resample(y[int(start * r):int((start + length + xfade + 0.1) * r)], r, rate), rate)
    n, x = int(length * rate), int(xfade * rate)
    a = np.linspace(0, np.pi / 2, x)
    out = y[:n].copy()
    out[:x] = y[:x] * np.sin(a) + y[n:n + x] * np.cos(a)
    return out * 10 ** ((level - rms_db(out)) / 20)


def shot(name, start, length, rate, t_out, level=-1.0, pre=0.002):
    y, r = load(name)
    y = y[int((start - pre) * r):int((start + length) * r)]
    y = fade(highpass(resample(y, r, rate), rate, 60.0), rate, 0.001, t_out)
    return y * 10 ** (level / 20) / (np.abs(y).max() + 1e-12)


def oneshot(name, start, rate, floor_db=-36.0, longest=4.0, level=-1.0):
    """From `start` until the sound has fallen floor_db below its peak (100 ms
    windows), at most `longest` seconds; faded out over its last third."""
    y, r = load(name)
    y = highpass(resample(y[int(start * r):], r, rate), rate)
    w = int(0.1 * rate)
    env = np.array([rms_db(y[i:i + w]) for i in range(0, len(y) - w, w)])
    peak = env.max()
    end = len(env)
    for k in range(int(np.argmax(env)), len(env)):
        if env[k] < peak + floor_db:
            end = k
            break
    n = min(end * w, int(longest * rate))
    y = fade(y[:n], rate, 0.003, n / rate / 3)
    return y * 10 ** (level / 20) / (np.abs(y).max() + 1e-12)


def write(name, y, rate):
    path = os.path.join(OUT, name + '.ogg')
    y = np.clip(y, -1, 1).astype(np.float32)
    # (in blocks: libsndfile's Vorbis encoder falls over on one long write)
    with sf.SoundFile(path, 'w', rate, 1, format='OGG', subtype='VORBIS', compression_level=QUALITY) as f:
        for i in range(0, len(y), 4096):
            f.write(y[i:i + 4096])
    print(f'{name + ".ogg":16s} {rate:6d} Hz  {len(y) / rate:5.2f} s  {os.path.getsize(path) / 1024:6.1f} KB')
    return os.path.getsize(path)


def main():
    if not os.path.isdir(RAW):
        sys.exit(f'no recordings in {RAW}')
    os.makedirs(OUT, exist_ok=True)
    total = 0
    # the own engine: the Fokker's steady flight, 6-20 s of it
    total += write('engine', loop('fokker_steady_437717', 6.0, 14.0, 16000, level=-17.0), 16000)
    # the enemy coming at the player: a Nieuport's rotary passing, its loudest part
    total += write('enemy', loop('nieuport_flyby_437741', 18.0, 10.0, 16000, level=-17.0), 16000)
    # a plane spiralling down, three of them: the Warner Brothers spin early
    # (lower, about 290 Hz) and late (higher, about 400 Hz), and a dive's whine
    total += write('spiral1', loop('spins_dives_437748', 16.0, 8.0, 16000, level=-17.0), 16000)
    total += write('spiral2', loop('spins_dives_437748', 36.0, 8.0, 16000, level=-17.0), 16000)
    total += write('spiral3', loop('sustained_dive_437747', 22.0, 9.0, 16000, level=-17.0), 16000)
    # the machine gun: the first shot of each of the Maxim's bursts (clean,
    # no tail of a shot before it), cut before the next shot (125 ms later)
    for k, t in enumerate((1.090, 3.307, 5.442, 8.762)):
        total += write(f'shot{k + 1}', shot('maxim_very_near_160772', t, 0.115, 32000, 0.035), 32000)
    # what rings on when the guns stop: the last shot's echo, without the shot
    total += write('shot_tail', shot('maxim_very_near_160772', 5.825 + 0.110, 2.6, 32000, 1.2, level=-6.0, pre=0.0),
                   32000)
    # explosions
    for k, (name, t) in enumerate((('expl_heavy_sharp_674896', 0.0), ('expl_medium_674906', 0.0),
                                   ('expl_dull_674887', 0.0), ('expl_heavy2_674908', 0.0))):
        total += write(f'explosion{k + 1}', oneshot(name, t, 16000, longest=3.0), 16000)
    print(f'together {total / 1024:.0f} KB')


if __name__ == '__main__':
    main()
