# Carrying on

State and open points are in [STATUS.md](STATUS.md), building and
checking in [BUILD.md](BUILD.md). The model for everything still to come
is `F:\Battlezone Enhanced` (same board family; `src/cpu`, `src/mbox`,
`src/avg`, `src/rom`, `src/sound/pokey.*` and `src/platform/app.*` come
from there).

## Layout

```
upstream/               the original source (historicalsource/red-baron), read only
external/VectorPie/     the SDK, unchanged (0.9.0 or later)
tools/mac65.py          assembler for the source, checks against the ROMs
tools/mac2c.py          writes src/game/*.c, game.h and src/core/rb_symbols.h
src/game/main.c         MAIN, SOUNDS, EOLSEQ, GMINIT, SCOREM ...       (generated)
src/game/states.c       STATES, high score table                       (generated)
src/game/objects.c      new planes and shells, motion                  (generated)
src/game/ground.c       ground sequence                                (generated)
src/game/display.c      landscape, plane, explosions, shells           (generated)
src/game/alpha.c, collide.c, math.c                                    (generated)
src/game/grnd.c         NMI, projection, rotation, DECODE (RBGRND.MAC) (generated)
src/game/int.c, sound.c, coin.c, earom.c, vgut.c                       (generated)
src/game/frame.[ch]     by hand: INTWAIT, rb_power_on, rb_interrupt, rb_main_poll, rb_frame
src/game/m6502.h        the instruction macros
src/core/machine.[ch]   the board: memory, input/output, EAROM
src/core/emu.[ch]       the original program on the 6502 core (reference)
src/sound/              POKEY, discrete sounds, the ring of samples
src/sound/samples.[ch]  recorded sounds: a mixer driven by the latch and the POKEY
package/sounds/         the sounds (Ogg, mono), CREDITS.txt with the sources
assets/freesound_raw/   the Freesound originals (WAV), SOURCES.txt; not in the package
tools/make_sounds.py    cuts the sounds from them (loops, single shots)
tools/make_vpk.sh       the cabinet package (.vpk)
src/render/config, draw settings, drawing through the SDK
src/render/subframe.[ch] pictures between the frames from the game's state (smooth = 3)
src/render/color.[ch]   a color for every tag
src/render/solid.[ch]   occlusion: biplanes, ground objects, mountains, trees, clouds
src/render/deco.[ch]    trees and clouds, drawn with the game's routines
src/render/glyph.[ch]   the game's font as lines (from the vector ROM)
src/render/screens.[ch] title, high score table, initials entry, free play texts
src/render/cockpit.[ch] cockpit: 3D bodies with their own hidden lines, instruments
src/render/demo.[ch]    attract demo: a second machine with an autopilot
src/render/fx.[ch]      explosions: fireball and sparks; the enemy's roundels
src/core/tags.h         the lines' tags
src/platform/app        window, board, input
src/platform/audio      the sound to the SDK's stream (vp_stream_play); loads the recordings
src/platform/controls.[ch] controls: bindings, digital/analog, navigation for the menu
src/platform/menu.[ch]  test menu (basic functions; the owner wants it kept lean)
src/main.c              program loop, menu, sound test, high scores (EAROM file, the menu's highscores.txt)
tools/mame/rbcapture.lua, capture.sh, captures.sh   recordings from MAME
tools/rbcompare, rbnmi, rbshot, checkall.sh, coverage.sh   checking
                        (rbshot --samples package/sounds: sound with the recordings, counts what began)
```

Tags and notes for color and occlusion also come from the translator's
tables: `TAG_AT` (a call of a drawing routine with a tag, by source file
and line), `NOTE_AT` (a call where DECODE notes an object's points),
`INSERT_AFTER` (a C line after a line of the source). They change nothing
in the game's memory; the comparisons with MAME stay the same.

Do not change generated files by hand. Changes belong in
`tools/mac2c.py` (the tables at the top: `HAND`, `SPLIT_AFTER_JSR`,
`JUMP_RETURNS`, `LIMIT`) and are written again with
`python -I tools/mac2c.py`.

## What the C routines look like

`rb_xxx` is the routine `XXX` of the source. The registers are in `m->r`,
every instruction is a macro:

```c
LDA(RD(AX(RB_PKYRAN)));     /* LDA AX,PKYRAN */
STA(ZX(RB_PLDELX));         /* STA ZX,PLDELX */
if (BNE) goto l10;          /* BNE 10$ */
```

* A label is a function if it is called with JSR or jumped to from outside
  its function. A jump or a fall into another function is a call plus
  `return`.
* Local labels `10$` are called `l10`, in later sections of a function
  `name_l10`.
* `PHA`/`PHP` push onto a small stack of its own in the machine.
* Symbols: `RB_` plus the original name, `.` becomes `_`, `$` is dropped.
  MAC65 tells names apart by their first six characters only.
* MAIN is split twice (`SPLIT_AT_LOCAL`, `SPLIT_AFTER_JSR`): `rb_main`
  moves everything (up to `20$`), calls `rb_view_begin` (frame.c: hands
  the state to `m->view_hook`, for the pictures between frames), and that
  calls `rb_main_view`, which draws up to `JSR INTWAIT`. `rb_main2` shows
  the frame (buffer swap). `rb_main_poll` calls `rb_main2` and `rb_main`
  when INTWAIT's conditions are met.

## Timing

* The interrupt runs 246.09 times a second. `C.CNTR` counts 24 (12 in the
  banner) and sets `CALFLG`: a game frame, 10.25 a second. `F.CNTR` counts
  4 and sets `INTFLG`: the vector generator is started 61.5 times a
  second, and the interrupt changes the propeller pictures (PLPROP writes
  into both display buffers for that).
* The interrupt does real game work: reading the stick (with its own
  calibration through POTMIN/PTRNGE), switches, coins, counting up the
  score (SCOREM, with bonus lives), tunes (MODSND), the sound latch.
* Two display buffers (2002 and 2802), 2000 is a jump to the one shown.

`rbshot --capture FILE --frame N --svg picture.svg` draws a frame of a
MAME recording with all the additions (one frame is run by the C routines
for it, so that every line has its tag); `--frames` lists the frames with
a ground sequence and with an exploding plane. That way scenes a test game
does not reach can be looked at.

## Sound

The game writes the board's sound latch (1808) and the POKEY; `rb_sound`
(src/sound/sound.c) makes the samples of each interrupt's time into a
ring, either with the board's circuits (MAME's model) or, with
`sounds = samples`, with the recordings (src/sound/samples.c: started and
steered by the same writes, plus a little of the game's memory for the
climb, who fires and how far away a falling plane is). `rb_audio_pump`
(src/platform/audio.c) hands the ring to the SDK's stream once a picture,
keeping 50 to 160 ms queued. The SDK mixes streams at the menu's MUSIC
VOLUME (the owner chose that over SOUND VOLUME on 2026-11-10: a single
stream stays sample-exact; the SDK puts every stream under MUSIC).

## Pitfalls

* Do not write source code through shell heredocs (`\n` becomes a line
  break there, even inside a quoted heredoc). Write scripts with the Write
  tool into `_scratch/`.
* The recording script must not record reads of the vector RAM: MAME's
  vector generator reads through the same address space.
* The 6502 also reads the address without the carry on indexed
  addressing. The core and the C macros (`ZX`, `AX`, `AXW`, `IYW`) do the
  same, otherwise the recorded reads do not fit.
* POKEY random numbers: outside a game the program stops the counter
  (SKCTL = 0) and checks that. `machine.c` then returns FF.
* `emu.print_info` from a tap in MAME's Lua does not show on the console.
  Write to a file to look.
* Only a vector DECODE draws itself carries point numbers for the
  occlusion (`m->note = 0` after it, through `INSERT_AFTER`). Otherwise
  the blank move with which the game puts the beam at a new piece carries
  the number of the point before, and a mountain's face is off.
  `RB_FACES=1 rbshot ...` draws the faces as well, to check.
* The additions (trees, clouds) write their display list into the second
  buffer of the copy; their room ends before the shape data (`room` in
  `render/deco.c`). Beyond it they overwrite the data, and the list runs
  away (seen: long strokes, an empty picture).
* MAME remembers option switches and the EAROM. `tools/capture.sh` empties
  its configuration folder before every recording.
* Screenshots of the PC window: with the GL renderer they stay black. Set
  `SDL_RENDER_DRIVER=software`, then `PrintWindow` works. Send keys with
  `PostMessage` (WM_KEYDOWN/UP with the scan code), at least 80 ms apart:
  the program reads the keyboard state once a picture. Esc ends the
  program.
* libsndfile (Python `soundfile`) crashes when it writes a long Ogg file
  in one call (exit 127, a 4 KB file): write in blocks
  (`tools/make_sounds.py`).
* Test menu: F2 (VK 0x71), not Tab. Another service key for testing:
  `VP_ADVMAME_RC` pointing to a file with `input_map[service] keyboard[0,f3]`.
* The planes' angles (PLOBDB +12..+17): a circle is 800 hex
  (QQ AAAAAAA FF), not 10000. PLTEST turns about the Y axis only if R (D4)
  is set in the status, otherwise about the X axis.
* Clearing the high scores (`reset_scores` in `main.c`) does not write
  the EAROM itself; it sets the variables with which HSCHK starts the
  game's EAROM routine (6 sets from EAFILL, erase first, then write). If
  the EAROM is busy, again later.
* The motion objects (PLOBDB, 28 bytes each): X, Y, Z (depth) as the first
  three words, the status at +6 (D6: hit, falling or exploding).
* Never sort or reorder the vector lines to save beam time: on the
  cabinet's board it makes the picture wobble (the owner forbids it).

## Next steps

1. **Play and listen at a screen** (STATUS.md, "Not yet seen or heard").
2. **Look at it on the cabinet**: whether the picture with trees and
   clouds stands still (`run.log`: vectors, blank moves), otherwise fewer
   of them. Tune the colors (`render/color.c`). More faces for the
   occlusion (blimp: points in PLNDB entry 7, drones).
3. **Refine the pictures between frames** if needed at the screen: blend
   more fields (`render/subframe.c`; which cells still differ at t = 1
   shows `_scratch/subdiag.c`); for the banner and the high score page let
   the lines wander to the next frame as in Star Wars (`render/picture.c`
   there).
4. **Listen to the recorded sounds on the cabinet** and set the levels by
   ear: `*_GAIN` in `sound/samples.c`, the engine also through
   `engine_volume`. Other parts or recordings: `tools/make_sounds.py`
   (times and rates there), then `py -3.10 -I tools/make_sounds.py`; more
   Freesound recordings are in stock (assets/freesound_raw/SOURCES.txt:
   rotary engine with blip switch, Fokker fly-by). Compare the original
   sound with a MAME recording (`-wavwrite`).
5. **Look at the current version on the cabinet**: title, table, entry,
   cockpit (vector load, readability of the instruments), free play, the
   high scores in the menu. Preview without the cabinet:
   `rbshot --capture _scratch/cap/X.rbcap --frame N --screens --cockpit
   [--free] [--page-t S] --svg F`, then `magick F.svg F.png`. Pages in the
   recordings: banner attract.rbcap frame 300, table hiscore.rbcap 760,
   entry hiscore.rbcap 735 (`_scratch/modes.c` lists GAMODE per frame).
   A video of the running program: `python3.10 -I _scratch/video.py OUT.mp4 80`
   (starts the window with the software renderer, plays a key sequence,
   records 30 pictures a second with PrintWindow, writes MP4 with OpenCV;
   there is no ffmpeg here). For a phone stay under 30 MB (640x480). The
   bullets are single dots: the script thickens every picture before
   scaling it down, otherwise they vanish in the video.
6. **Use the test menu on the cabinet**: bind the stick, look at the
   control test. Deliberately lean (owner, 2026-10-14: "basic functions
   for tests"): no color bars, calibration pictures, joystick lists; more
   values only in `redbaron.ini`. Start LED and coin counter are still
   missing (Battlezone: `vp_lamps_set`).
7. **Cabinet:** install the `.vpk` (`tools/make_vpk.sh`) with INSTALL
   GAMES, check the menu entry. New versions go to the GitHub releases and
   to the owner's Google Drive folder "games" (Chrome; Drive opens a file
   dialog for uploads: catch `HTMLInputElement.prototype.click` for file
   inputs first, then fill the input with file_upload).
8. **The SDK's other new parts** (0.9.0): a game's own controls
   (`vp_controls.h`, save/controls.ini), per-board frame rate in
   `game.ini`. Not used yet.
