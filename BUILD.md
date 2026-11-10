# Building and checking

## Fetch once

Two things are not part of the project and are fetched next to it (both
are in `.gitignore`):

```bash
git clone --depth 1 https://github.com/historicalsource/red-baron.git upstream
```

```bash
git clone --depth 1 https://github.com/Goldfinch2/VectorPie.git external/VectorPie
```

`upstream` is the original source code (read only). `external/VectorPie`
is the SDK for the output (0.9.0 or later: the game's sound goes out as
an SDK stream, `vp_stream_play`; 0.6.0 was built with VectorPie commit
`b00d537`). Without the SDK the tools still build
(`-DRB_BUILD_GAME=OFF`).

## PC (Windows)

Needs MSYS2 with MinGW-w64 GCC and SDL2 in `C:\msys64\mingw64`, plus
CMake and Ninja.

```bash
export PATH="/c/msys64/mingw64/bin:$PATH"
```

```bash
cmake -S . -B build/pc -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
```

```bash
cmake --build build/pc
```

```bash
build/pc/redbaron.exe --windowed --roms D:/Mame/roms
```

## Generating the C routines again

`src/game/*.c` (except `frame.c`), `src/game/game.h` and
`src/core/rb_symbols.h` are generated from `upstream/`:

```bash
python -I tools/mac65.py --listing _scratch/rbaron.lst
```

```bash
python -I tools/mac2c.py
```

The first assembles the source, compares it with the ROMs in `upstream/`
and writes a listing with every address. The second writes the C files.
What the translator cannot do by rule is in the tables at the top of
`tools/mac2c.py`.

## Checking against MAME

MAME 0.289 and its ROM folder: `$RB_MAME` (default `D:/Mame/mame.exe`) and
`$RB_MAME_ROMS` (default `D:/Mame/roms`). MAME runs without picture and
sound with a configuration folder of its own in `_scratch/mame`. Your own
MAME settings are not touched.

Make all the standard recordings again (7 minutes, 350 MB in
`_scratch/cap`):

```bash
sh tools/captures.sh
```

Compare everything:

```bash
sh tools/checkall.sh
```

```bash
sh tools/checkall.sh --emu
```

```bash
sh tools/checkall.sh --nmi
```

Without an option this checks the C routines frame by frame, with `--emu`
the original program on the 6502 core, with `--nmi` the interrupt routine
and power on in C against the original on the core. At the end it says
`ALL THE SAME` or `DIFFERENCES`.

A single recording (script: see `tools/mame/rbcapture.lua`, times in game
frames):

```bash
sh tools/capture.sh _scratch/cap/test.rbcap 120 "60 coin 2;130 start 2;140 aim 3"
```

```bash
build/pc/rbcompare.exe _scratch/cap/test.rbcap --show 20
```

`rbcompare` knows `--emu`, `--from F`, `--to F`, `--max N`, `--show N`,
`--trace F` (the frame's recorded reads) and `--leftover`.

### How the comparison works

For every game frame the recording holds the original's memory at the
moment the main loop goes on in INTWAIT (RAM, vector RAM, Math Box). With
it comes what the main loop got from outside in the frame before: the
random numbers, its reads of memory the interrupt writes, and of switches
and EAROM. `rbcompare` loads a frame's memory, runs one frame and
compares with the next. Every frame is checked on its own.

Not compared are the addresses the interrupt wrote in that frame (the
recording names them) and the stack. For the interrupt itself there is
`rbnmi`.

## Coverage

```bash
cmake -S . -B build/cov -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DRB_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_C_FLAGS=--coverage -O0"
```

```bash
cmake --build build/cov
```

```bash
sh tools/coverage.sh --lines
```

## Picture and sound without a screen

```bash
build/pc/rbshot.exe --roms D:/Mame/roms --seconds 14 --play "3 coin;6 start;8 fire on;9 stick 0.3 -0.2" --svg _scratch/shots/play.svg --at 13 --wav _scratch/shots/play.wav
```

`magick -density 60 picture.svg picture.png` makes a PNG of it. Times in
the script are seconds. With `--original` the original program runs on
the core. At the end it prints the game mode, score, lives and the sound's
level. With `--samples package/sounds` the recorded sounds play instead of
the circuits; at the end it says how often each one began.

Cutting the sounds again (from `assets/freesound_raw`, needs numpy, scipy,
soundfile):

```bash
py -3.10 -I tools/make_sounds.py
```

## Cabinet (Raspberry Pi, VectorPie)

From Windows, without WSL: clang and lld of the PC against a copy of the
cabinet's libraries (those of the Tempest 2000 port). In Git Bash:

```bash
export PATH="/c/Users/ocare/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin:$PATH"
```

```bash
cmake -S . -B build/pi -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-pi-clang.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "-DRB_PI_SYSROOT=F:/Tempest 2000 Port/build/pi-sysroot"
```

```bash
cmake --build build/pi --target redbaron
```

```bash
sh tools/deploy.sh
```

The package for installing from the menu (INSTALL GAMES, from a USB drive
or from `/persistent/games/packages/`):

```bash
sh tools/make_vpk.sh
```

It writes `build/rbenhanced-<version>.vpk` (program, `game.ini`,
`redbaron.ini`, marquee, `sounds/`; no ROMs) and sets the file modes in
the archive itself (on Windows the file system has none).

`deploy.sh` copies the program, `game.ini`, `redbaron.ini`, `run.sh`,
`marquee.png` and `sounds/` to `/persistent/games/rbenhanced` on the
cabinet (192.168.0.122, key `~/.ssh/id_ed25519_vectorpie`). Copying only:
what runs there is not touched (a running Red Baron keeps the old program
until it is started again from the menu). `sh tools/deploy.sh --restart`
also ends a running Red Baron and starts the new one (the menu is stopped
meanwhile), only when the owner asks for it. The program finds the ROMs
through `dir_rom` in `advmame.rc` (there:
`/usr/local/share/advance/rom/redbaron.zip`). Your own settings on the
cabinet: `/persistent/games/rbenhanced/save/redbaron.ini`.
