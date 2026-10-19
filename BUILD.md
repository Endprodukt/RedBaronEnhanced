# Bauen und Prüfen

## Einmalig holen

Zwei Dinge gehören nicht zum Projekt und werden daneben geholt (beide
stehen in `.gitignore`):

```bash
git clone --depth 1 https://github.com/historicalsource/red-baron.git upstream
```

```bash
git clone --depth 1 https://github.com/Goldfinch2/VectorPie.git external/VectorPie
```

`upstream` ist der Originalquelltext (nur lesen). `external/VectorPie` ist
das SDK für die Ausgabe. Ohne SDK lassen sich die Werkzeuge trotzdem bauen
(`-DRB_BUILD_GAME=OFF`).

## PC (Windows)

Voraussetzung: MSYS2 mit MinGW-w64-GCC und SDL2 unter `C:\msys64\mingw64`,
dazu CMake und Ninja.

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

## Die C-Routinen neu erzeugen

`src/game/*.c` (außer `frame.c`), `src/game/game.h` und
`src/core/rb_symbols.h` werden aus `upstream/` erzeugt:

```bash
python -I tools/mac65.py --listing _scratch/rbaron.lst
```

```bash
python -I tools/mac2c.py
```

Das erste assembliert den Quelltext, vergleicht mit den ROMs in `upstream/`
und schreibt ein Listing mit allen Adressen. Das zweite schreibt die
C-Dateien. Was der Übersetzer nicht nach Regel kann, steht in den Tabellen
am Anfang von `tools/mac2c.py`.

## Prüfen gegen MAME

MAME 0.289 und sein ROM-Ordner: `$RB_MAME` (Vorgabe `D:/Mame/mame.exe`) und
`$RB_MAME_ROMS` (Vorgabe `D:/Mame/roms`). MAME läuft ohne Bild und Ton mit
einem eigenen Konfigurationsordner unter `_scratch/mame`. Die eigenen
MAME-Einstellungen bleiben unberührt.

Alle Standardaufnahmen neu machen (7 Minuten, 350 MB in `_scratch/cap`):

```bash
sh tools/captures.sh
```

Alles vergleichen:

```bash
sh tools/checkall.sh
```

```bash
sh tools/checkall.sh --emu
```

```bash
sh tools/checkall.sh --nmi
```

Ohne Option prüft das die C-Routinen Frame für Frame, mit `--emu` das
Originalprogramm auf dem 6502-Kern, mit `--nmi` Interrupt-Routine und
Einschalten in C gegen das Original auf dem Kern. Am Ende steht
`ALL THE SAME` oder `DIFFERENCES`.

Eine einzelne Aufnahme (Skript: siehe `tools/mame/rbcapture.lua`, Zeiten
in Spielframes):

```bash
sh tools/capture.sh _scratch/cap/test.rbcap 120 "60 coin 2;130 start 2;140 aim 3"
```

```bash
build/pc/rbcompare.exe _scratch/cap/test.rbcap --show 20
```

`rbcompare` kennt `--emu`, `--from F`, `--to F`, `--max N`, `--show N`,
`--trace F` (die aufgezeichneten Lesezugriffe des Frames) und `--leftover`.

### Wie der Vergleich arbeitet

Die Aufnahme enthält zu jedem Spielframe den Speicher des Originals in dem
Moment, in dem die Hauptschleife in INTWAIT weiterläuft (RAM, Vektor-RAM,
Math Box). Dazu kommt, was die Hauptschleife im Frame davor von außen
bekam: die Zufallszahlen, ihre Lesezugriffe auf Speicher, den der
Interrupt schreibt, und auf Schalter und EAROM. `rbcompare` lädt den
Speicher eines Frames, lässt einen Frame laufen und vergleicht mit dem
nächsten. Jeder Frame wird für sich geprüft.

Nicht verglichen werden die Adressen, die der Interrupt im jeweiligen
Frame geschrieben hat (die Aufnahme nennt sie), und der Stack. Für den
Interrupt selbst gibt es `rbnmi`.

## Abdeckung

```bash
cmake -S . -B build/cov -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DRB_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_C_FLAGS=--coverage -O0"
```

```bash
cmake --build build/cov
```

```bash
sh tools/coverage.sh --lines
```

## Bild und Ton ohne Bildschirm

```bash
build/pc/rbshot.exe --roms D:/Mame/roms --seconds 14 --play "3 coin;6 start;8 fire on;9 stick 0.3 -0.2" --svg _scratch/shots/play.svg --at 13 --wav _scratch/shots/play.wav
```

Mit `--samples package/sounds` spielen die aufgenommenen Sounds statt der
Schaltungen; am Ende steht, wie oft jeder begann.

Die Sounds selbst neu schneiden (aus `assets/freesound_raw`, braucht
numpy, scipy, soundfile):

```bash
py -3.10 -I tools/make_sounds.py
```

`magick -density 60 bild.svg bild.png` macht daraus ein PNG. Zeiten im
Skript sind Sekunden. Mit `--original` läuft das Originalprogramm auf dem
Kern. Am Ende stehen Spielmodus, Punkte, Leben und der Pegel des Tons.

## Automat (Raspberry Pi, VectorPie)

Von Windows aus, ohne WSL: clang und lld vom PC gegen eine Kopie der
Bibliotheken des Automaten (die des Tempest-2000-Ports). In Git Bash:

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

Das Paket für die Installation über das Menü (INSTALL GAMES, vom USB-Stick
oder aus `/persistent/games/packages/`):

```bash
sh tools/make_vpk.sh
```

Es schreibt `build/rbenhanced-<Version>.vpk` (Programm, `game.ini`,
`redbaron.ini`, Marquee, `sounds/`; keine ROMs) und setzt die Rechte im
Archiv selbst (unter Windows hat das Dateisystem keine).

`deploy.sh` kopiert Programm, `game.ini`, `redbaron.ini`, `run.sh`,
`marquee.png` und `sounds/` nach `/persistent/games/rbenhanced` auf dem Automaten
(192.168.0.122, Schlüssel `~/.ssh/id_ed25519_vectorpie`). Nur kopieren: was
dort läuft, bleibt unberührt (ein laufendes Red Baron behält das alte
Programm bis zum nächsten Start aus dem Menü). `sh tools/deploy.sh
--restart` beendet zusätzlich ein laufendes Red Baron und startet das neue
(das Menü wird dabei gestoppt) – nur auf ausdrücklichen Wunsch des
Besitzers. Die ROMs findet das
Programm über `dir_rom` aus `advmame.rc` (dort liegt
`/usr/local/share/advance/rom/redbaron.zip`). Eigene Einstellungen auf dem
Automaten: `/persistent/games/rbenhanced/save/redbaron.ini`.
