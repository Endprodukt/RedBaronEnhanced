# Status

As of: 2026-11-10 (version 0.6.0)

## Goal

Red Baron (Atari, 1981) complete and unchanged in C, checked against
MAME. Then additions as in Battlezone Enhanced and Star Wars Enhanced
(smooth picture, sound, color), each one switchable.

The first part is reached. Additions (the owner's wishes): pictures
between the frames exactly as in Star Wars, stick calibration, color,
occlusion, trees and clouds, test menu (0.3.0); free play, title, high
score table, initials entry and cockpit (0.4.0); recorded sounds (0.5.0,
0.5.1); high scores in the cabinet's menu (0.5.2); VectorPie SDK 0.9.0,
the sound through the SDK's mixer at the menu's volume (0.6.0).

## What runs

The game as a C program (`src/game`, 174 routines, about 9,000 lines),
generated from the source code of 1981:

* `tools/mac65.py` assembles `upstream/` and compares with the ROMs:
  13,408 of 14,336 bytes, 0 differences. The rest is the self test (the
  include file MBDIAG is missing from the repository).
* `tools/mac2c.py` writes the C routines from it: original names,
  original comments, one macro per 6502 instruction.
* The source is MAME's set `redbaron` (all seven program ROMs the same).

The program `redbaron` (a window on the PC, the vector board on the
cabinet, through the VectorPie SDK): picture at 60 pictures a second,
sound (POKEY, machine gun, nose dive, explosion; or recordings), freely
bindable controls, test menu on the service key (F2), settings and option
switches in `redbaron.ini` (your own in `save/redbaron.ini`), EAROM (high
scores) in `save/earom.bin`. In normal operation no 6502 runs;
`--original` runs the original program on the 6502 core for comparison.

## What the port adds

The game mechanics are unchanged. All comparisons against MAME are still
completely the same after these additions.

* **Smooth picture from the game's state** (`smooth = 3`, default;
  `render/subframe.c`, after `subframe.c` of Star Wars Enhanced). The game
  moves everything 10.25 times a second (MAIN up to the label `20$`) and
  then draws (MAIN_VIEW; the translator splits MAIN there). For every
  picture in between, the player's heading, height and bank, the planes,
  drones and the blimp with their angles, the mountains and ground targets
  and all shells are put on their way from one frame to the next, and the
  game's drawing routines compute the picture of that moment on a copy of
  the machine. No delay: the C computes a frame right after the buffer
  swap, so both ends are known as soon as a frame is on the screen. The
  propellers turn as in the original, by the interrupt. `smooth = 0`:
  every frame as on the cabinet.
* **Pictures between frames for exploding planes and drones**: every
  shown object (A, E explosion, the drone states ED/P/F) is interpolated
  as long as it stays the same kind. The bursting of the debris follows the
  game's counter (byte 7) and still jumps.
* **Initials entry with the yoke** (`platform/controls.c`): LETCHK changes
  the letter while a pot is in its top or bottom eighth and looks at both
  axes. A yoke gets there imprecisely and at a slant. During the entry
  (GAMODE xxxx0111) the game gets only the more deflected axis, and that
  either fully (from 45 %) or not at all.
* **High score top right larger** (`render/screens.c`): HIGH SCORE and
  value/initials 1.7 times, about their top middle, kept on the screen
  (with `screens`).
* **Blimp** (`render/subframe.c`, `render/color.c`): when one has flown
  past, the game at once puts the next one far back into the same object;
  the pictures between frames used to push it across the screen
  ("teleported"). For the blimp a step over 400 hex in X, Y or Z now
  counts as a new object (it moves 80 hex a frame). Colors by its parts
  (DBLIMP, always in the same order, `rb_color_mark`): silver envelope, a
  red band, two steel blue rings, a brass gondola.
* **Overheating machine guns**: in the game and checked (after about 6 s
  of continuous fire GUN.ST D6, the guns go quiet, GUNS OVERHEATED in red
  above the middle, cooled about 1 s after letting go). In the cockpit the
  gun jackets turn orange from half the heat and blink red when
  overheated.
* **Explosions** (`explosions = yes`; `render/fx.c`): where debris appears
  (model PIECES or tag EXPLOSION, none in the picture before), a fireball
  around its middle: a flash cross, a ragged ring growing and cooling
  (white, orange, red), eight sparks flying out and falling, 0.7 s, about
  20 lines; hidden behind the cockpit.
* **Real gameplay in the attract mode** (`attract_demo = yes`;
  `render/demo.c`): in the original only a plane flies at the player in
  the attract mode and backs away again (checked against MAME's recording;
  too silly for the owner, a first attempt to turn it round removed
  again). Instead a second, silent machine (the same C program, its own
  EAROM, never saved) plays a game: coins in and started by itself,
  steered by the aiming automaton of the MAME recordings (steers by the
  enemy's screen position, PLSTAT+8..B, tries the direction, fires every
  fourth frame), the next one after game over. Its picture is shown while
  the real game shows the world in the attract mode (GAMODE 00); over it
  INSERT COINS, what a coin buys, CREDITS and the real game's high score
  (or the free play texts). Title and table come as before. The real game
  runs underneath unchanged and takes coins and START. The demo machine
  runs only while it is shown.
* **Stick calibration** (`stick_calibrate = yes`, default;
  `rb_stick_calibrate` in `game/frame.c`). Why the plane drifted: the
  interrupt learns each axis's range only from the player's moves (POTMIN,
  PTRNGE, POT READING ROUTINE in RBGRND.MAC), POTSCL takes the middle of
  that range as the stick's middle. After the first deflection to one side
  the released stick sits at the edge of the learnt range and counts as
  half pushed to the other side: once right, then let go, gives a full
  left turn (PLDELX -30) until the stick has been fully left once. Since
  the port makes the readings itself (64 to 192, middle 128), the lower
  limit and the range are set right at power on. The routine goes on
  learning as in the original.
* **Color** (`color_lines = yes`, default; `render/color.c`). Every line
  carries what it belongs to (`core/tags.h`): the translator puts a tag
  around the calls of the drawing routines (`TAG_AT` in `tools/mac2c.py`,
  `RB_TAGGED`), the machine remembers it for every word of the vector RAM,
  the vector generator passes it on with every line, also for the pictures
  the game builds as subroutines in the vector RAM (plane, drone,
  propeller). Colors: the player is the Red Baron, the foes are British:
  the enemy biplane khaki (PC10 of a Sopwith Camel, light enough for the
  tube) with roundels (blue ring, red middle, in perspective on the upper
  wing near the tips, from points 12, 13, 29, 30 of DB.PLN,
  `rb_fx_roundels`, with `color_lines`), propeller white, drones
  (escorts) in the same khaki (the game turns a drone into the enemy; with
  a color of its own it changed color doing so), land and horizon green,
  mountains yellow green, ground targets earth colored, flak orange,
  bullets yellow, explosions golden, blimp in its parts' colors, bullet
  holes white, high score pages gold; score, texts and lives in the
  cabinet's blue. `color_lines = no`: everything in the one color.
* **Occlusion** (`occlusion = yes`, default; `render/solid.c`, after
  Battlezone). What hides:
  * the biplane: faces entered by hand after its points in the ROM
    (DB.PLN): both wings, the fuselage as an octagonal pyramid with a
    nose, fin and tailplane;
  * the ground objects (PFLOB: tent, tree, gun, bunker): their closed
    outlines, a little in front of the mountain they stand on;
  * the mountains: the land under the ridge down to its foot, from the
    landscape's points in the ROM (SSEGS) and the drawn points' places on
    the screen (an affine map, since a mountain stands flat at one
    distance);
  * trees and clouds (below).
  Where every point lies on the screen and how far away, DECODE notes
  while drawing (`rb_note_point`, through `INSERT_AFTER` in the
  translator; calls with `NOTE_AT` get a number of their own per drawing,
  `vginst`). Lines of these things and the horizon are cut where something
  nearer stands in front. Shells, explosions, drones, texts and the own
  propeller never.
* **Explosions in the plane's color**: the parts of a bursting biplane
  stay khaki, those of a drone too; only the flash of a hit ground object
  (EXCRCL) is golden.
* **Colored ground objects**: part colors by their points: tent (canvas
  sand, pole brown), tree (trunk brown, crown green), gun (base grey,
  turret olive), bunker (walls concrete grey, sides darker, roof red
  brown).
* **Trees and clouds** (`trees`, `clouds`, on by default;
  `render/deco.c`). Drawn with the game's own routines, the way it draws
  its objects on the ground (POSITP, SETBM, PROJECT, DECODE): they move,
  tilt with the horizon and get smaller with distance like the mountains.
  Trees: the game's tree shape and a fir after it, two thirds the size,
  on a fixed grid of the world the player flies through; one in every
  tenth cell, about 12 in the picture on average, at most 20, between depth
  400 and F00 (hex; the horizon is at 1000). Clouds: long flat outlines
  behind the mountains, high up, in every fourth cell, at most 8. They are
  in a copy of the display list (the free second buffer), the game notices
  nothing.
  **Fading in instead of popping up** (the owner's wish): trees get
  brighter smoothly over the last 500 (hex) of depth before F00, clouds
  fade in at the back and out at the front (`rb_deco_fade`; the drawing
  multiplies the brightness by it, since the game's 15 steps are too
  coarse). When the world appears anew (game start, new life, after the
  high score pages) they all fade in together in one second. The upper
  limit hardly matters any more (1 % of the pictures instead of all the
  time); before, distant trees were missing because of it and appeared in
  the middle of the picture when a near one went. Measured with
  `_scratch/fadecheck.c` (2 minutes of play, pictures between frames as in
  the program): of about 350 newly appearing trees all come in either
  faint or at the edge of the picture. The projection is measured
  (`_scratch/probe.c`): 400 * dx / d to the side, 400 * (height - I4YPOS)
  / d + 40 up.
* **Test menu** (service key, or hold start and fire for two seconds;
  `platform/menu.c`, after Battlezone Enhanced). The game stands still
  while it is open; the service key saves to `save/redbaron.ini` and goes
  back. The service key is AdvanceMAME's action `service` as `advmame.rc`
  binds it (VectorPie's CONTROLS settings and AdvanceMAME's menu write
  there); if nothing or "auto" is there, F2 as in MAME. The menu's hint
  names the key. Tab no longer opens the menu (Tab is AdvanceMAME's UI
  CONFIG).
  Only basic functions (the rest is in `redbaron.ini`). Pages:
  * CONTROLS: stick analog (keys and digital stick work as well) or
    digital, sensitivity (digital: how fast the held stick deflects,
    `digital_rate`; analog: `gain`); bindings of the stick axes,
    directions, fire, start, coin (fire on the line, then a key, button or
    axis; left/right back to "auto" = advmame.rc); up and down reversed.
    `platform/controls.c`. Next to it, live, the yoke test: the stick as
    the game gets it (cross), the analog axes (square), the pot readings,
    fire/start/coin.
  * PICTURE: smooth motion, colors, occlusion, trees, clouds, cockpit,
    title and high scores, gameplay in attract, explosions, brightness.
  * TEST PICTURE: a crosshatch with border and circle.
  * SOUND: volume, sounds (RECORDINGS/ORIGINAL), engine volume, sound
    test (machine gun, explosion, nose dive, engine, plane closing, POKEY
    tone).
  * GAME OPTIONS: planes, bonus, language, coins. Take effect at once.
  * RESET HIGH SCORES (fire twice): the table to zero (with
    `default_scores` the aces again), the game's EAROM routine writes it
    with checksums (as after a new entry).
* **High scores in the VectorPie menu** (marquee, hint bar): `game.ini`
  `hiscores = yes`; the game also writes its table (HSCRES, 7 entries, BCD
  times 10) as `save/highscores.txt` in the SDK's format
  (`scores_text`/`scores_file` in `main.c`) once it has changed, not
  during the initials entry and not while the EAROM is busy; a new file,
  then renamed. Checked on the PC (7 aces in the file); whether the menu
  shows them has not been seen on the cabinet yet (the menu may read
  `game.ini` only after it restarts).
* **Free play** (`coin_mode = free`, in the menu COINS: FREE PLAY;
  `main.c`). The board's free play switch puts two credits in at once in
  the attract mode and stands at PRESS START for ever (CREDIT in
  RBARON.MAC), the attract mode no longer runs. Here the board is set to
  "1 coin 1 play"; START in the attract mode gives the game its coin (the
  coin switch for one pulse) and holds START until the game begins.
  Instead of INSERT COINS and CREDITS it says FREE PLAY and, blinking,
  PRESS START (in the game's language).
* **New screens** (`screens = yes`; `render/screens.c`): on the game's own
  pages (GAMODE) other pictures of the same state; what the game draws
  there (tag STATES) is left out.
  * Title instead of the banner (GAMODE 13): stripes slide in, the circle
    draws itself, the iron cross turns in, the eagle's head fades in, "Red
    Baron" writes itself in a Fraktur hand (broad nib, both edges of the
    nib drawn); then every 4 s a shine across the lettering. After the
    flyer and the marquee.
  * High score table (GAMODE x3): the heading drops in, two small turning
    crosses, the lines slide in one after another, first place changes
    color.
  * Initials entry (GAMODE x7): NEW RECORD blinking, score and place, three
    boxes (letters centred by their visible width), the chosen one red
    with arrows, hints in the game's language, a bar for the time left
    (TIMER). Used as on the cabinet (stick up/down changes, fire takes:
    LETCHK, STATES).
  INSERT COINS and what a coin buys move down on these pages. Texts in the
  game's own font (`render/glyph.c`: the vector ROM's character routines
  run once through the vector generator and kept as lines, freely scaled
  and slanted; hyphen and dot are missing in the game).
* **Aces in an empty table** (`default_scores = yes`; `main.c`): if the
  table is empty after power on (or after RESET HIGH SCORES), the empty
  places hold MVR 20000, UDT 15000, LOE 12000, VOS 10000, BOE 8000, IMM
  6000, JAC 4000, always below the last real entry; the EAROM is written
  through the game's routine. So the attract mode shows the table (the
  game does that only when it is not empty).
* **Cockpit** (`cockpit = yes`; `render/cockpit.c`): the cockpit of a
  Fokker Dr.I seen from the seat, after a photo of the owner's (a small 3D
  model, vanishing point in the middle of the picture): two Spandau
  machine guns near the middle, their receivers on the dashboard (jacket
  with holes, muzzle, rear and front sight, ring sights with cross hairs
  in front); the nose as wide as the fuselage (the guns lie on top), in
  front of it the propeller stylised as in the films (a smooth swish arc at
  every blade tip, bright at the front, trailing off, a fainter one
  further in, turned 27 degrees a picture; no circle; the game's own
  propeller, three strokes at the bottom, goes with the cockpit); the
  middle wing left and right, on it two struts each leaning out to the
  upper wing; two crossed bracing wires, a small windscreen; closed down
  to the corners at the bottom, no view out; the dashboard with altimeter
  (I4YPOS), airspeed indicator (faster in a dive: PLDELY) and gun
  thermometer (GUN.ST, blinks red when overheated). The guns kick in turn
  as they fire, as the game fires them in turn (S.XFLG D6), with muzzle
  flash. Occlusion of its own: guns and struts are bodies of faces, only
  the visible edges are drawn (back faces gone, for barrels and struts
  only the outline; what one part hides of another is gone). The world
  (land, mountains, planes, blimp, shells, the own propeller) and the
  copyright line vanish behind all parts (`rb_solid.front`); the game's
  texts and displays stay in front, and so do the bullets: the game starts
  them low at the sides (about x +-200, y -60), where the wings and struts
  are now; hidden they could not be seen. Only a picture: nothing changes
  in the game. Also shown in the attract mode with the world, not on
  title/table/entry.
* **Recorded sounds** (`sounds = samples`, test menu SOUND -> SOUNDS
  RECORDINGS/ORIGINAL; `sound/samples.c`, files in `package/sounds`, cut
  by `tools/make_sounds.py` from Freesound recordings in
  `assets/freesound_raw`, sources in `package/sounds/CREDITS.txt`). The
  recordings hang on the same signals as the board (latch 1808 and the
  POKEY), nothing changes in the game:
  * own engine: the hum (POKEY channels 3/4, AUDC A1) becomes a Fokker
    biplane (Hollywood optical sound of the 1930s, "G01-10 Fokker Biplane
    Steady", a 14 s loop with a crossfade); faster and louder in a dive,
    slower climbing (PLDELY, as the airspeed indicator); a volume of its
    own, `engine_volume` (test menu ENGINE VOLUME);
  * the enemy coming at the player (AUDF3 below F8): a Nieuport with a
    rotary engine, higher and louder as the original's tone rises;
  * machine gun: every rise of the shot bit (15 times a second) one
    single shot of a Maxim (the gun of the LMG 08/15, 4 variants), the echo
    when firing stops; the enemy's fire quieter and lower (own fire: SWSTAT
    D7 and not overheated);
  * explosion: the crash level rises -> one of 4 explosions, as loud as
    the level;
  * spiral (D1): a plane spinning down, each one different: one of three
    recordings (the Warner Brothers spin early and late, a dive's whine),
    pitch 0.9 to 1.12, falling by 15 to 35 % in 3 to 5 s, starting at a
    random place, getting quieter as it falls;
  * spiral and explosion by distance: the hit motion object (PLOBDB,
    status D6) by X and Z, volume (400 / distance)^0.8 between 0.2 and 1,
    duller far off (a low pass down to about 800 Hz); the own crash
    (GMEND1 D7) full, ground targets 0.6.
  The jingles (points, bonus life, new plane, 300 points) stay the
  POKEY's. Channels 3/4 are muted only while they carry the hum (AUDC A1),
  a jingle on them is heard. All files mono (one speaker), Ogg Vorbis;
  16 kHz for the old optical recordings (only noise above that,
  measured), 32 kHz for the machine gun; 363 KB together.
* **The sound through the SDK's mixer** (0.6.0; `platform/audio.c`): the
  samples the emulation makes go out as the game's own SDK stream
  (`vp_stream_play`, SDK 0.9.0) instead of an SDL device of the game's own.
  On the cabinet the VectorPie menu's MUSIC VOLUME applies to it (the SDK
  mixes every game's streams at that level; the owner chose MUSIC over
  SOUND, 2026-11-10), on top of the game's own VOLUME; the menu's MASTER
  VOLUME is the system's. About 50 to 160 ms are kept queued in the stream;
  when it nearly runs dry the last level dies away over 60 ms instead of
  clicking. Checked on the PC: a POKEY tone arrives in the SDK's mix
  (`_scratch/streamtest.c`, `vp_audio_recent`).

## Checked

Against MAME 0.289, 20 recordings (`tools/captures.sh`): attract mode,
played games, long games with ground sequences and blimps, more than five
lives, high score entry and its timing, four languages, option switches,
coin modes, free play, slam, POKEY check.

| Check | Extent | Result |
|---|---|---|
| C routines frame by frame (`checkall.sh`) | 60,689 frames, 4,969 bytes a frame | all the same (again with SDK 0.9.0, 2026-11-10) |
| Original program on the 6502 core (`--emu`) | the same | all the same |
| Interrupt routine and power on in C against the original (`--nmi`) | all 20 recordings | all the same |
| Counter check: a constant in the C falsified on purpose | 721 frames | all reported as different |
| Game in live operation without a screen (`rbshot`), C and `--original` | 60 s with coin, start, shooting | both 90 points, both in the initials entry afterwards |
| Picture (`rbshot`, `docs/images`) | attract mode and game | plane, horizon, mountains, displays, texts as expected |
| Program start in a window | 6 s | 60 pictures a second, about 340 vectors a picture, 0.3 ms a picture with pictures between frames |
| Pictures between frames, t = 0 (`rbshot --smooth`) | 913 frames of a played game | in all of them the frame word for word |
| Pictures between frames, t = 1 against the next frame (`_scratch/subdiag2.c`) | 512 frames | with the same number of lines 0.9 units apart per line end (picture width 520); in half of the frames lines come or go (new shots, objects in the picture, blinking): those change with the frames |
| Pictures between frames on the picture (`rbshot --smooth --at`) | six pictures in 100 ms | planes move from picture to picture |
| Color and occlusion on the picture (`rbshot`, `docs/images/color_occlusion.png`) | attract plane at four moments, game scenes | fuselage lines behind the nose and struts behind the wings vanish, edges stay; 14 to 26 lines cut per picture |
| MAME comparisons after the tags went in | all 20 recordings | still all the same |
| Explosion, ground sequence, trees, clouds on the picture (`rbshot --capture` with frames from `kills` and `long`) | 8 frames | parts colored, ground objects multicolored, mountains hide trees and clouds behind them |
| After the additions: MAME comparisons, pictures between frames at t = 0 | all 20 recordings, 605 frames | still all the same |
| Occlusion by mountains (`RB_FACES=1 rbshot` draws the hiding faces too) | attract mode and ground sequence | every mountain a closed face from ridge to foot, matching its lines |
| Stick: once right, then let go (`_scratch/pottrace.c`) | 10 s | without calibration PLDELX -30 for good, with it back to 0 |
| Recorded sounds in a played game (`rbshot --samples`) | 95 s | engine, 5 closing enemies, 439 own and 44 enemy shots, 6 explosions, 5 spirals begun; engine alone about -32 dB against -43 dB of the original hum, firing about -25 dB |
| Distance of falling planes (`_scratch/crashvol.c`) | 240 s, 17 crashes | three spiral recordings in turn, near ones full, far ones 0.2 and dull |
| Sound through the SDK stream (`_scratch/streamtest.c`) | 1.5 s POKEY tone | arrives in the SDK's mix (rms 0.039) |

Coverage of the C routines by the recordings (`tools/coverage.sh`):
`math.c` and `alpha.c` 100 %, `display.c`, `main.c`, `grnd.c`, `int.c`
over 99 %, `objects.c`, `ground.c`, `collide.c`, `coin.c`, `sound.c` over
98 %, `vgut.c` 97 %, `earom.c` 93 %, `states.c` 91 %. Not run through:
reading the EAROM with a valid checksum (the recordings start with an
empty EAROM), carries in the counters for number of games and game time,
parts of the difficulty's self adjustment, the jump into the self test and
single rare branches.

## Not yet seen or heard by anyone

* **The game window in motion.** Two still pictures are checked, the
  window's start too. Nobody has played it at the screen.
* **The sound.** POKEY and the discrete sounds make levels (measured in a
  WAV file). The sound and the levels of the parts against each other come
  from MAME's model, not tuned by ear.
* **The recorded sounds.** Checked offline (above). Listen:
  `docs/listen/` (new and original, the same game). The levels of the
  parts against each other (`*_GAIN` in `sound/samples.c`) are set by
  measurement, not by ear.
* **The menu's volume with SDK 0.9.0 on the cabinet** (0.6.0 has not been
  on the cabinet yet).
* **Keyboard and joystick.** Only driven by script. Four joystick devices
  are registered on this PC (among them vJoy and a pedal); whether one of
  them pulls the stick is open (the control test shows it).
* **Test menu.** Gone through by script on the PC window (pages checked as
  pictures, binding a key, holding start+fire, saving, options taking
  effect live, reset high scores writing the EAROM). Not used on the
  cabinet.
* **Pictures between frames in motion.** Only checked on still pictures
  and measurements. How the ground sequence, the blimp and a crash look is
  unseen.

## Known differences from the cabinet

* **Speed.** The cabinet computes 10.25 game frames a second (every 24
  interrupts) and needs up to 172,000 cycles for a frame, sometimes more
  than the 147,456 it has. The C needs no time and always runs at 10.25.
  In the attract mode's banner it is 20.5.
* **Random numbers.** In live operation from a shift register of its own,
  not from a cycle exact POKEY.
* **Math Box.** As in MAME at the level of routines, done at once.
* **Vector generator.** Counts as done at the next interrupt in live
  operation.
* **Start button.** Bit 6 of 1802 is 0 with the button pressed, as on the
  cabinet. MAME's port is the other way round; there the game starts by
  itself as soon as there is a credit.
* **Coin counter, start LED** are not led out.

## On the cabinet

Installed with `tools/deploy.sh` (copying only) in
`/persistent/games/rbenhanced`, as of 2026-10-15 version 0.5.1 with the
recorded sounds in `sounds/`; the owner starts it from the menu. Vector
board (USB-DVG), 60 pictures a second; with cockpit, trees, clouds and
demo up to about 700 vectors (see limits). The ROMs come from
`/usr/local/share/advance/rom/redbaron.zip`. Marquee:
`package/marquee.png` (1920x360, the size of the HDMI marquee screen;
`advmame.rc` stretches with `vector_aux_marquee_display_mode stretch`),
supplied by the owner. Package: `tools/make_vpk.sh` makes
`build/rbenhanced-<version>.vpk`; 0.5.1 is in the public Google Drive
folder "games" since 2026-10-16 (shared by the owner, next to the other
packages) and a GitHub release since 2026-10-19. Nobody has installed it
with INSTALL GAMES yet. Whether the menu entry appears without the menu
restarting is not checked.

## Known limits of the additions

* Pictures between frames: what comes into the picture or leaves it,
  blinking, counters and texts change with the frames as in the original.
  The stick takes effect with the next frame only (as in the original).
  Turns are blended in a straight line. A frame in which the player jumps
  (new game, new life) is not blended. The pages without the world
  (banner, high scores) have no pictures between frames; in Star Wars the
  lines wander to the next frame there, here not yet.
* Occlusion: drones, blimp and explosion debris hide nothing. Within a
  mountain range (four pieces of a landscape, all at one distance)
  crossing ridges stay. Lines without a known distance (horizon) count as
  behind.
* Trees and clouds cost time on the vector board: about 470 instead of 340
  vectors a picture, the blank moves about double. Whether the picture
  stands still on the tube is to be checked on the cabinet; otherwise
  `trees = no` or fewer trees (`MAX_TREES` in `render/deco.c`).
* Trees also stand where mountains come near in the ground sequence (they
  belong to no mountain).
* At the edge of the picture a tree appears whole as soon as its middle is
  in the picture (the game's SETBM checks only the middle), so half pushed
  in.
* Texts at the top and messages have colors of their own (labels amber,
  score warm white, value green, high score light blue, INSERT COINS gold,
  what a coin buys violet, BONUS LIFE / 300 POINTS / BEHIND YOU pink,
  overheated guns red, lives red): tags of their own around the calls in
  ALPHAS.
* The colors are a first proposal, to be tuned with the owner
  (`render/color.c`, one table).
* Calibration: applies to the readings the port makes itself. Not with
  `--original` (6502 core).
* Test menu: a key pressed shorter than a picture (1/60 s) is not seen
  (keyboard state per picture). When binding, a key counts only if
  everything was released before. Esc ends the program outside of binding
  (SDK). Holding start+fire starts a game on the side in the attract mode
  if there is a credit.
* Clouds lie over the texts (e.g. INSERT COINS), they are not hidden by
  them.
* Cockpit: costs vectors (about 200 lines more a picture). Measured on
  the cabinet (2026-10-14): with cockpit, trees, clouds and demo about 700
  vectors, beam path about 95,000 lit + 140,000 dark a picture; the
  USB-DVG cannot do that in 1/60 s, the picture rate drops to 30 after a
  few seconds. Without trees/clouds a stable 60. It is not the computing
  time (under 0.6 ms a picture on the PC). Reordering the lines by the
  shortest blank move is forbidden (owner: practically never works, the
  picture wobbles) and removed again. Open: fewer lines/details. It hides,
  as in a real plane, a lot: the dashboard at the bottom, the middle wing
  at the sides below about -70; targets there cannot be seen. The airspeed
  indicator is made up (the game knows no speed), it follows climbing and
  sinking. The dashboard covers the bottom edge (copyright), like the
  bezel on the cabinet.
* Title: the page lasts as long as the game's banner would fly (about
  11 s); the animation does not end softly, it breaks off with the page
  change. The Fraktur is a drawing of its own after the flyer, not a font.
* Free play: the coins count in the game's statistics (EAROM, coin
  counter). The board's original free play can no longer be chosen.
* Empty EAROM (fresh installation): until the game's reading routine is
  done, the high score briefly shows "FFFF0" (as on the cabinet with an
  empty chip). RESET HIGH SCORES in the test menu writes a valid one.
* Recorded sounds: the optical recordings hiss (not denoised, only cut
  above 8 kHz). The own engine follows only climbing and sinking; there is
  no throttle in the game. Whether a shot is the player's or the enemy's
  is inferred from the fire button; if both fire at once everything sounds
  like own fire. Explosion and spiral do not choose by what was hit
  (plane, blimp, ground target), the game gives the same signal for all.
  The distance counts only X and Z, not the height; ground targets have a
  fixed volume (0.6), their place is not read.
* The SDK mixes the game's stream at the menu's MUSIC VOLUME: with the
  menu's music at 0 the game is silent too.

## Not ported

* The self test (RBINT.MAC from `RAMTST`). It checks hardware that does
  not exist here, and its source is incomplete. With `--original` it runs
  on the 6502 core (test switch not on any key yet).
* The crash check in the interrupt (stack, lost frames) stops the program
  here and reports it, instead of waiting for the watchdog.

## Open points

1. Play, listen, try the joystick, look at the pictures between frames
   (see above).
2. Decide on and build more additions (see RESUME.md).
3. Look at 0.6.0 on the cabinet: test menu, title, cockpit, free play, the
   recorded sounds and the menu's volume; set the sounds' levels by ear.
4. Install the `.vpk` with INSTALL GAMES on the cabinet and check it.
5. Coverage: a recording with a filled EAROM (a second run without
   clearing `_scratch/mame/nvram`).

## Next steps

See [RESUME.md](RESUME.md).

Releases: every new package goes to the GitHub releases
(`gh release create v<version> build/rbenhanced-<version>.vpk`) and to the
owner's Google Drive folder "games". v0.5.1 (2026-10-19), v0.6.0
(2026-11-10).
