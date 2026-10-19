# Weiterarbeiten

Stand und offene Punkte stehen in [STATUS.md](STATUS.md), Bauen und Prüfen
in [BUILD.md](BUILD.md). Vorlage für alles, was noch kommt, ist
`F:\Battlezone Enhanced` (gleiche Platinenfamilie; `src/cpu`, `src/mbox`,
`src/avg`, `src/rom`, `src/sound/pokey.*`, `src/platform/app.*` und
`audio.*` stammen von dort).

## Aufbau

```
upstream/               Originalquelltext (historicalsource/red-baron), nur lesen
external/VectorPie/     das SDK, unverändert
tools/mac65.py          Assembler für den Quelltext, prüft gegen die ROMs
tools/mac2c.py          erzeugt src/game/*.c, game.h und src/core/rb_symbols.h
src/game/main.c         MAIN, SOUNDS, EOLSEQ, GMINIT, SCOREM ...       (erzeugt)
src/game/states.c       STATES, Highscore-Tabelle                      (erzeugt)
src/game/objects.c      neue Flugzeuge und Schüsse, Bewegung           (erzeugt)
src/game/ground.c       Bodensequenz                                   (erzeugt)
src/game/display.c      Landschaft, Flugzeug, Explosionen, Schüsse     (erzeugt)
src/game/alpha.c, collide.c, math.c                                    (erzeugt)
src/game/grnd.c         NMI, Projektion, Drehung, DECODE (RBGRND.MAC)  (erzeugt)
src/game/int.c, sound.c, coin.c, earom.c, vgut.c                       (erzeugt)
src/game/frame.[ch]     von Hand: INTWAIT, rb_power_on, rb_interrupt, rb_main_poll, rb_frame
src/game/m6502.h        die Befehlsmakros
src/core/machine.[ch]   Platine: Speicher, Ein-/Ausgabe, EAROM
src/core/emu.[ch]       Originalprogramm auf dem 6502-Kern (Referenz)
src/sound/              POKEY, diskrete Sounds, Ring für das Tongerät
src/sound/samples.[ch]  aufgenommene Sounds: Mixer, an Latch und POKEY gehängt
package/sounds/         die Sounds (Ogg, mono), CREDITS.txt mit den Quellen
assets/freesound_raw/   Freesound-Originale (WAV), SOURCES.txt; nicht im Paket
tools/make_sounds.py    schneidet die Sounds daraus (Schleifen, Einzelschüsse)
src/render/config, draw Einstellungen, Zeichnen über das SDK
src/render/subframe.[ch] Zwischenbilder aus dem Spielzustand (smooth = 3)
src/render/color.[ch]   Farbe je Kennung
src/render/solid.[ch]   Verdeckung: Doppeldecker, Bodenobjekte, Berge, Bäume, Wolken
src/render/deco.[ch]    Bäume und Wolken, mit den Routinen des Spiels gezeichnet
src/render/glyph.[ch]   die Schrift des Spiels als Linien (aus dem Vektor-ROM)
src/render/screens.[ch] Titel, Highscore-Tafel, Initialeneingabe, Freispiel-Texte
src/render/cockpit.[ch] Cockpit: 3D-Körper mit eigener Kantenverdeckung, Instrumente
src/render/demo.[ch]    Attract-Demo: zweite Maschine mit Autopilot
src/render/fx.[ch]      Explosionen: Feuerball und Funken
src/core/tags.h         Kennungen der Linien
src/platform/app, audio Fenster, Ton
src/platform/controls.[ch] Steuerung: Belegung, digital/analog, Navigation fürs Menü
src/platform/menu.[ch]  Testmenü (Grundfunktionen; Wunsch des Besitzers: schlank halten)
src/main.c              Programmschleife, Menü, Soundtest, Highscores löschen, EAROM-Datei
tools/mame/rbcapture.lua, capture.sh, captures.sh   Aufnahmen aus MAME
tools/rbcompare, rbnmi, rbshot, checkall.sh, coverage.sh   Prüfen
                        (rbshot --samples package/sounds: Ton mit Aufnahmen, zählt was begann)
```

Kennungen und Notizen für Farbe und Verdeckung kommen ebenfalls aus den
Tabellen des Übersetzers: `TAG_AT` (Aufruf einer Zeichenroutine mit
Kennung, nach Quelldatei und Zeile), `NOTE_AT` (Aufruf, bei dem DECODE die
Punkte eines Objekts notiert), `INSERT_AFTER` (C-Zeile nach einer Zeile
des Quelltexts). Sie ändern nichts am Speicher des Spiels; die Vergleiche
mit MAME bleiben gleich.

Erzeugte Dateien nicht von Hand ändern. Änderungen gehören in
`tools/mac2c.py` (Tabellen am Anfang: `HAND`, `SPLIT_AFTER_JSR`,
`JUMP_RETURNS`, `LIMIT`) und werden mit `python -I tools/mac2c.py` neu
geschrieben. Was die Ergänzungen im Spiel selbst brauchen (etwa Notizen
der Zeichenroutinen wie `m->tag` bei Battlezone), sollte der Übersetzer
über eine Tabelle "nach Zeile N von Modul M einfügen" bekommen; die gibt
es noch nicht.

## Wie die C-Routinen aussehen

`rb_xxx` ist die Routine `XXX` des Quelltexts. Register stehen in `m->r`,
jeder Befehl ist ein Makro:

```c
LDA(RD(AX(RB_PKYRAN)));     /* LDA AX,PKYRAN */
STA(ZX(RB_PLDELX));         /* STA ZX,PLDELX */
if (BNE) goto l10;          /* BNE 10$ */
```

* Ein Label ist eine Funktion, wenn es per JSR gerufen oder von außerhalb
  seiner Funktion angesprungen wird. Sprung oder Durchfallen in eine
  andere Funktion ist Aufruf plus `return`.
* Lokale Marken `10$` heißen `l10`, in späteren Abschnitten einer
  Funktion `name_l10`.
* `PHA`/`PHP` legen auf einen eigenen kleinen Stapel in der Maschine.
* Symbole: `RB_` plus Originalname, `.` wird `_`, `$` fällt weg.
  MAC65 unterscheidet nur die ersten sechs Zeichen eines Namens.
* MAIN ist zweimal geteilt (`SPLIT_AT_LOCAL`, `SPLIT_AFTER_JSR`):
  `rb_main` bewegt alles (bis `20$`), ruft `rb_view_begin` (frame.c: gibt
  den Zustand an `m->view_hook`, für die Zwischenbilder) und das ruft
  `rb_main_view`, das zeichnet bis `JSR INTWAIT`. `rb_main2` zeigt das
  Frame (Pufferwechsel). `rb_main_poll` ruft `rb_main2` und `rb_main`,
  wenn die Wartebedingungen von INTWAIT erfüllt sind.

## Zeitablauf

* Interrupt 246,09 pro Sekunde. `C.CNTR` zählt 24 (im Banner 12) und setzt
  `CALFLG`: ein Spielframe, 10,25 pro Sekunde. `F.CNTR` zählt 4 und setzt
  `INTFLG`: der Vektorgenerator wird 61,5-mal pro Sekunde neu gestartet,
  dabei wechselt der Interrupt die Propellerbilder (PLPROP schreibt dafür
  in beide Displaypuffer).
* Der Interrupt macht echte Spielarbeit: Knüppel lesen (mit eigener
  Kalibrierung über POTMIN/PTRNGE), Schalter, Münzen, Punkte hochzählen
  (SCOREM, mit Bonusleben), Melodien (MODSND), Sound-Latch.
* Zwei Displaypuffer (2002 und 2802), 2000 ist ein Sprung auf den gezeigten.

`rbshot --capture DATEI --frame N --svg bild.svg` zeichnet ein Frame
einer MAME-Aufnahme mit allen Ergänzungen (ein Frame wird dafür mit den
C-Routinen gerechnet, damit jede Linie ihre Kennung hat); `--frames` listet
die Frames mit Bodensequenz und mit explodierendem Flugzeug. So lassen sich
Szenen ansehen, die ein Testspiel nicht erreicht.

## Fallen

* Keine Quelltexte über Shell-Heredocs schreiben (`\n` wird dort zum
  Zeilenumbruch). Skripte mit dem Write-Werkzeug nach `_scratch/` legen.
* Das Aufnahmeskript darf Lesezugriffe im Vektor-RAM nicht aufzeichnen:
  MAMEs Vektorgenerator liest über denselben Adressraum.
* Der 6502 liest bei indizierten Adressen zusätzlich die Adresse ohne
  Übertrag. Kern und C-Makros (`ZX`, `AX`, `AXW`, `IYW`) machen das mit,
  sonst passen die aufgezeichneten Lesezugriffe nicht.
* POKEY-Zufall: außerhalb eines Spiels hält das Programm den Zähler an
  (SKCTL = 0) und prüft das. `machine.c` liefert dann FF.
* `emu.print_info` aus einem Tap in MAMEs Lua erscheint nicht auf der
  Konsole. Zum Nachsehen in eine Datei schreiben.
* Punktnummern für die Verdeckung trägt nur ein Vektor, den DECODE selbst
  zeichnet (`m->note = 0` danach, per `INSERT_AFTER`). Sonst trägt der
  Leerweg, mit dem das Spiel den Strahl an ein neues Stück setzt, die
  Nummer des vorigen Punkts, und die Fläche eines Bergs liegt daneben.
  `RB_FACES=1 rbshot ...` zeichnet die Flächen zur Kontrolle mit.
* Die Ergänzungen (Bäume, Wolken) schreiben ihre Displayliste in den
  zweiten Puffer der Kopie; ihr Platz endet vor den Formdaten (`room` in
  `render/deco.c`). Darüber hinaus überschreiben sie die Daten, und die
  Liste läuft weg (so beobachtet: lange Striche, leeres Bild).
* MAME merkt sich Optionsschalter und EAROM. `tools/capture.sh` leert
  deshalb vor jeder Aufnahme seinen Konfigurationsordner.
* Bildschirmfotos des PC-Fensters: mit dem GL-Renderer bleiben sie
  schwarz. `SDL_RENDER_DRIVER=software` setzen, dann geht `PrintWindow`.
  Tasten per `PostMessage` (WM_KEYDOWN/UP mit Scancode) schicken, mit
  mindestens 80 ms dazwischen: das Programm liest den Tastaturzustand
  einmal je Bild. Esc beendet das Programm.
* libsndfile (Python `soundfile`) stürzt beim Schreiben einer langen
  Ogg-Datei in einem Aufruf ab (Exit 127, Datei 4 KB): blockweise
  schreiben (`tools/make_sounds.py`).
* Testmenü: F2 (VK 0x71) statt Tab. Eine andere Service-Taste zum Testen:
  `VP_ADVMAME_RC` auf eine Datei mit `input_map[service] keyboard[0,f3]`.
* Winkel der Flugzeuge (PLOBDB +12..+17): ein Kreis sind 800 hex
  (QQ AAAAAAA FF), nicht 10000. PLTEST dreht um die Y-Achse nur, wenn im
  Status R (D4) gesetzt ist, sonst um die X-Achse.
* Highscores löschen (`reset_scores` in `main.c`) schreibt das EAROM nicht
  selbst, sondern stellt die Variablen ein, mit denen HSCHK die EAROM-
  Routine des Spiels startet (6 Sätze ab EAFILL, erst löschen, dann
  schreiben). Ist das EAROM gerade beschäftigt, später noch einmal.

## Nächste Schritte

1. **Am Bildschirm spielen und anhören** (STATUS.md, "Noch von niemandem
   gesehen").
2. **Am Automaten ansehen**: ob das Bild mit Bäumen und Wolken ruhig
   steht (`run.log`: Vektoren, Leerweg), sonst ihre Zahl senken.
   Farben abstimmen (`render/color.c`). Weitere Flächen für die
   Verdeckung (Zeppelin: Punkte in PLNDB Eintrag 7, Drohnen).
3. **Zwischenbilder verfeinern**, wenn am Bildschirm nötig: weitere
   Felder überblenden (`render/subframe.c`, welche Zellen bei t = 1 noch
   abweichen zeigt `_scratch/subdiag.c`); für Banner und Highscore-Seite
   die Linien zum nächsten Frame wandern lassen wie bei Star Wars
   (`render/picture.c` dort).
4. **Aufgenommene Sounds am Automaten anhören** (0.5.1) und die Pegel
   nach Gehör stellen: `*_GAIN` in `sound/samples.c`, Motor auch über
   `engine_volume`. Andere Stellen oder Aufnahmen: `tools/make_sounds.py`
   (Zeiten und Raten dort), dann `py -3.10 -I tools/make_sounds.py`; im
   Vorrat liegen weitere Freesound-Aufnahmen (assets/freesound_raw/
   SOURCES.txt: Umlaufmotor mit Blip-Schalter,
   Fokker-Vorbeiflug). Den Original-Ton noch gegen eine MAME-Aufnahme
   (`-wavwrite`) abgleichen.
5. **0.5.1 ansehen** (kopiert 2026-10-15):
   Titel, Tafel, Eingabe, Cockpit (Vektorlast, Lesbarkeit der Instrumente),
   Freispiel. Vorschau ohne Automat: `rbshot --capture _scratch/cap/X.rbcap
   --frame N --screens --cockpit [--free] [--page-t S] --svg F`, dann
   `magick F.svg F.png`. Seiten in den Aufnahmen: Banner attract.rbcap
   Frame 300, Tafel hiscore.rbcap 760, Eingabe hiscore.rbcap 735
   (`_scratch/modes.c` listet GAMODE je Frame).
   Video vom laufenden Programm: `python3.10 -I _scratch/video.py OUT.mp4 80`
   (startet das Fenster mit Software-Renderer, spielt eine Tastenfolge ab,
   nimmt 30 Bilder/s per PrintWindow auf, schreibt MP4 mit OpenCV; ffmpeg
   gibt es hier nicht). Fürs Handy unter 30 MB bleiben (640x480). Die
   Kugeln sind einzelne Punkte: das Skript verdickt jedes Bild vor dem
   Verkleinern, sonst verschwinden sie im Video.
6. **Testmenü am Automaten bedienen** (gebaut in 0.3.0): Knüppel belegen,
   Steuerungstest ansehen. Bewusst schlank (Besitzer, 2026-10-14: „Grund-
   funktionen für Tests"): keine Farbbalken, Kalibrierbilder, Joystick-
   Listen; weitere Werte nur in `redbaron.ini`. Start-LED und Münzzähler
   fehlen noch (Battlezone: `vp_lamps_set`).
7. **Automat:** `.vpk` (`tools/make_vpk.sh`) per INSTALL GAMES
   installieren, Menüeintrag prüfen. Neue Versionen in den Drive-Ordner
   „games“ des Besitzers hochladen (Chrome; Drive öffnet beim Hochladen
   einen Dateidialog: vorher `HTMLInputElement.prototype.click` für
   Datei-Felder abfangen, dann das Feld mit file_upload füllen).
8. **Git-Repository** anlegen, wenn der Besitzer es will.
