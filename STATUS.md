# Status

Stand: 2026-10-14

## Ziel

Red Baron (Atari, 1981) vollständig und unverändert in C, geprüft gegen
MAME. Danach Ergänzungen wie bei Battlezone Enhanced und Star Wars
Enhanced (flüssiges Bild, Ton, Farbe), jede abschaltbar.

Der erste Teil ist erreicht. Ergänzungen (2026-10-14, Wünsche des
Besitzers): Zwischenbilder genau wie bei Star Wars, Kalibrierung des
Knüppels, Farbe, Verdeckung, Bäume und Wolken, Testmenü (0.3.0); Freispiel,
Titel, Highscore-Tafel, Initialeneingabe und Cockpit (0.4.0).

## Was läuft

Das Spiel als C-Programm (`src/game`, 174 Routinen, rund 9.000 Zeilen),
erzeugt aus dem Quelltext von 1981:

* `tools/mac65.py` assembliert `upstream/` und vergleicht mit den ROMs:
  13.408 von 14.336 Bytes, 0 Abweichungen. Der Rest ist der Selbsttest
  (die Include-Datei MBDIAG fehlt im Repository).
* `tools/mac2c.py` schreibt daraus die C-Routinen: Originalnamen,
  Originalkommentare, ein Makro je 6502-Befehl.
* Der Quelltext ist MAMEs Satz `redbaron` (alle sieben Programm-ROMs gleich).

Das Programm `redbaron` (Fenster am PC, über das VectorPie-SDK): Bild mit
60 Bildern pro Sekunde, Ton (POKEY, Maschinengewehr, Sturzflug,
Explosion), Steuerung frei belegbar, Testmenü auf der Service-Taste (F2),
Einstellungen und
Optionsschalter in `redbaron.ini` (eigene in `save/redbaron.ini`), EAROM
(Highscores) in `save/earom.bin`. Im normalen
Betrieb läuft kein 6502; `--original` lässt zum Vergleich das
Originalprogramm auf dem 6502-Kern laufen.

## Was der Port hinzufügt

Die Spielmechanik ist unverändert. Alle Vergleiche gegen MAME sind nach
diesen Ergänzungen weiter vollständig gleich.

* **Flüssiges Bild aus dem Spielzustand** (`smooth = 3`, Standard;
  `render/subframe.c`, nach `subframe.c` von Star Wars Enhanced). Das Spiel
  bewegt alles 10,25-mal pro Sekunde (MAIN bis zur Marke `20$`) und
  zeichnet dann (MAIN_VIEW; der Übersetzer teilt MAIN dort). Für jedes Bild
  dazwischen werden Blickrichtung, Höhe und Schräglage des Spielers, die
  Flugzeuge, Drohnen und der Zeppelin samt Winkeln, die Berge und Bodenziele
  und alle Schüsse auf ihrem Weg vom einen Frame zum nächsten eingesetzt,
  und die Zeichenroutinen des Spiels rechnen das Bild dieses Augenblicks
  auf einer Kopie der Maschine neu. Ohne Verzögerung: das C rechnet ein
  Frame gleich nach dem Pufferwechsel, beide Enden sind also bekannt,
  sobald ein Frame auf dem Schirm steht. Die Propeller dreht wie im
  Original der Interrupt. `smooth = 0`: jedes Frame wie am Automaten.
* **Zwischenbilder für explodierende Flugzeuge und Drohnen** (2026-10-14):
  bisher wurden nur Flugzeuge mit Status A (aktiv) interpoliert, die übrigen
  sprangen mit 10 Hz; jetzt jedes gezeigte (A, E Explosion, die
  Drohnenzustände ED/P/F), solange es dieselbe Art bleibt. Das Aufplatzen
  der Trümmer folgt dem Zähler des Spiels (Byte 7) und springt weiter.
* **Initialeneingabe mit dem Yoke** (`platform/controls.c`): LETCHK ändert
  den Buchstaben, solange ein Poti im obersten oder untersten Achtel steht,
  und schaut auf beide Achsen. Ein Yoke kommt da ungenau und schräg hin.
  Während der Eingabe (GAMODE xxxx0111) bekommt das Spiel nur die stärker
  ausgelenkte Achse, und die ganz (ab 45 %) oder gar nicht.
* **Highscore oben rechts größer** (`render/screens.c`): HIGH SCORE und
  Wert/Initialen 1,7-fach, um ihre obere Mitte, auf dem Schirm gehalten
  (mit `screens`).
* **Zeppelin** (`render/subframe.c`, `render/color.c`): Fliegt einer vorbei,
  setzt das Spiel im selben Objekt sofort den nächsten weit hinten ein; die
  Zwischenbilder schoben ihn dabei quer über den Schirm ("teleportiert").
  Für den Zeppelin gilt ein Schritt über 400 hex in X, Y oder Z jetzt als
  neues Objekt (er bewegt sich 80 hex je Frame). Farben nach seinen Teilen
  (DBLIMP, immer gleiche Reihenfolge, `rb_color_mark`): Hülle silbern, ein
  roter Ring, zwei stahlblaue, die Gondel messing.
* **Überhitzen der MGs**: im Spiel vorhanden und geprüft (nach rund 6 s
  Dauerfeuer GUN.ST D6, die MGs schweigen, rot GUNS OVERHEATED über der
  Mitte, nach dem Loslassen in rund 1 s abgekühlt). Im Cockpit werden die
  Kühlmäntel ab halber Hitze orange und blinken überhitzt rot.
* **Explosionen** (`explosions = yes`; `render/fx.c`): Wo Trümmer erscheinen
  (Modell PIECES oder Kennung EXPLOSION, im Bild davor keine), ein
  Feuerball um ihre Mitte: Blitzkreuz, ein unregelmäßiger Ring, wachsend
  und verglühend (weiß, orange, rot), acht Funken nach außen und fallend,
  0,7 s, rund 20 Linien; hinter dem Cockpit verdeckt.
* **Echtes Gameplay im Attract** (`attract_demo = yes`; `render/demo.c`):
  Im Original fliegt im Attract nur ein Flugzeug heran und rückwärts
  wieder weg (an MAMEs Aufnahme geprüft; dem Besitzer zu unsinnig, ein
  erster Versuch, es umdrehen zu lassen, wieder entfernt). Stattdessen
  spielt eine zweite, stumme Maschine (dasselbe C-Programm, eigenes EAROM,
  nie gespeichert) ein Spiel: selbst bemünzt und gestartet, gesteuert vom
  Zielautomaten der MAME-Aufnahmen (steuert nach der Bildposition des
  Gegners, PLSTAT+8..B, probiert die Richtung aus, feuert jeden vierten
  Frame), nach Spielende das nächste. Gezeigt wird ihr Bild, solange das
  echte Spiel im Attract die Welt zeigt (GAMODE 00); darüber INSERT COINS,
  was eine Münze kauft, CREDITS und der Highscore des echten Spiels (oder
  die Freispiel-Texte). Titel und Tafel kommen wie bisher. Das echte Spiel
  läuft darunter unverändert und nimmt Münzen und START an. Die Demo-Maschine
  läuft nur, solange sie gezeigt wird.
* **Kalibrierung des Knüppels** (`stick_calibrate = yes`, Standard;
  `rb_stick_calibrate` in `game/frame.c`). Ursache des Ziehens: Der
  Interrupt lernt den Bereich jeder Achse erst aus den Bewegungen des
  Spielers (POTMIN, PTRNGE, POT READING ROUTINE in RBGRND.MAC), POTSCL
  nimmt die Mitte dieses Bereichs als Mitte des Knüppels. Nach dem ersten
  Ausschlag zur einen Seite steht der losgelassene Knüppel am Rand des
  gelernten Bereichs und gilt als halb zur anderen Seite gedrückt: einmal
  rechts, dann losgelassen, gibt eine volle Linkskurve (PLDELX -30), bis
  der Knüppel einmal ganz links war. Weil der Port die Lesewerte selbst
  erzeugt (64 bis 192, Mitte 128), werden Untergrenze und Bereich beim
  Einschalten gleich richtig gesetzt. Die Routine lernt danach weiter wie
  im Original.

* **Farbe** (`color_lines = yes`, Standard; `render/color.c`). Jede Linie
  trägt, wozu sie gehört (`core/tags.h`): Der Übersetzer setzt um die
  Aufrufe der Zeichenroutinen eine Kennung (`TAG_AT` in `tools/mac2c.py`,
  `RB_TAGGED`), die Maschine merkt sie sich zu jedem Wort des Vektor-RAM,
  der Vektorgenerator gibt sie mit jeder Linie weiter, auch für die Bilder,
  die das Spiel als Unterprogramme im Vektor-RAM baut (Flugzeug, Drohne,
  Propeller). Farben: Der Spieler ist der Rote Baron, die Gegner sind
  Briten: feindlicher Doppeldecker khaki (PC10 einer Sopwith Camel, hell
  genug für die Röhre) mit Kokarden (blauer Ring, roter Kern, perspektivisch
  auf dem Oberflügel nahe den Spitzen, aus den Punkten 12, 13, 29, 30 von
  DB.PLN, `rb_fx_roundels`, mit `color_lines`), Propeller weiß, Drohnen
  (Begleiter) im selben Khaki (das Spiel macht eine Drohne zum Gegner; mit
  eigener Farbe wechselte sie dabei die Farbe), Land und Horizont grün, Berge gelbgrün, Bodenziele erdfarben,
  Flak orange, Schüsse gelb, Explosionen goldgelb, Zeppelin silber,
  Einschusslöcher weiß, Highscore-Seiten gold; Punkte, Texte und Leben im
  Blau des Automaten. `color_lines = no`: alles in der einen Farbe.
* **Verdeckung** (`occlusion = yes`, Standard; `render/solid.c`, nach
  Battlezone). Was verdeckt:
  * der Doppeldecker: Flächen fest eingetragen nach seinen Punkten im ROM
    (DB.PLN): beide Flügel, Rumpf als achteckige Pyramide mit Nase, Seiten-
    und Höhenleitwerk;
  * die Bodenobjekte (PFLOB: Zelt, Baum, Geschütz, Bunker): ihre
    geschlossenen Umrisse, etwas vor dem Berg, auf dem sie stehen;
  * die Berge: das Land unter dem Grat bis zum Fuß, aus den Punkten der
    Landschaft im ROM (SSEGS) und der Lage der gezeichneten Punkte auf dem
    Schirm (eine affine Abbildung, weil ein Berg flach in einer Entfernung
    steht);
  * Bäume und Wolken (unten).
  Wo jeder Punkt auf dem Schirm liegt und wie weit weg, notiert DECODE beim
  Zeichnen (`rb_note_point`, per `INSERT_AFTER` im Übersetzer; Aufrufe mit
  `NOTE_AT` bekommen eine eigene Nummer je Zeichnung, `vginst`). Geschnitten
  werden Linien dieser Dinge und der Horizont, wo etwas Näheres davor
  steht. Schüsse, Explosionen, Drohnen, Texte und der eigene Propeller nie.
* **Explosionen in der Farbe des Flugzeugs**: Die Teile eines zerplatzenden
  Doppeldeckers bleiben khaki, die einer Drohne ebenso; nur der Blitz eines
  getroffenen Bodenobjekts (EXCRCL) ist goldgelb.
* **Farbige Bodenobjekte**: Teilefarben nach ihren Punkten: Zelt (Plane
  sandfarben, Stange braun), Baum (Stamm braun, Krone grün), Geschütz
  (Sockel grau, Aufbau oliv), Bunker (Wände betongrau, Seiten dunkler,
  Dach rotbraun).
* **Bäume und Wolken** (`trees`, `clouds`, Standard an; `render/deco.c`).
  Gezeichnet mit den Routinen des Spiels selbst, so wie es seine Objekte am
  Boden zeichnet (POSITP, SETBM, PROJECT, DECODE): sie wandern, neigen sich
  mit dem Horizont und werden mit der Entfernung kleiner wie die Berge.
  Bäume: die Baumform des Spiels und eine Tanne danach, zwei Drittel so
  groß, auf einem festen Raster der Welt, durch das der Spieler fliegt;
  in jeder zehnten Zelle einer, im Mittel rund 12 im Bild, höchstens 20,
  zwischen Tiefe 400 und F00 (hex; der Horizont liegt bei 1000). Wolken:
  lange flache Umrisse hinter den Bergen, hoch oben, in jeder vierten
  Zelle, höchstens 8. Sie liegen in einer Kopie der Displayliste (der freie
  zweite Puffer), das Spiel merkt nichts.
  **Einblenden statt Aufploppen** (2026-10-14, Wunsch des Besitzers): Bäume
  werden über die letzten 500 (hex) Tiefe vor F00 stufenlos heller, Wolken
  blenden hinten ein und vorn aus (`rb_deco_fade`; die Zeichnung
  multipliziert die Helligkeit damit, weil die 15 Stufen des Spiels dafür
  zu grob sind). Erscheint die Welt neu (Spielstart, neues Leben, nach den
  Highscore-Seiten), blenden alle zusammen in einer Sekunde ein. Die
  Obergrenze greift kaum noch (1 % der Bilder statt ständig); vorher fehlten
  ferne Bäume wegen ihr und tauchten mitten im Bild auf, wenn ein naher
  verschwand. Gemessen mit `_scratch/fadecheck.c` (2 Minuten Spiel,
  Zwischenbilder wie im Programm): von rund 350 neu erscheinenden Bäumen
  kommen alle entweder schwach oder am Bildrand herein. Die Projektion ist ausgemessen (`_scratch/probe.c`): 400 * dx / d
  zur Seite, 400 * (Höhe - I4YPOS) / d + 40 nach oben.
* **Testmenü** (Service-Taste, oder Start und Feuer zwei Sekunden halten;
  `platform/menu.c`, nach Battlezone Enhanced). Das Spiel steht, solange es
  offen ist; die Service-Taste speichert nach `save/redbaron.ini` und
  kehrt zurück. Die Service-Taste ist AdvanceMAMEs Aktion `service`, wie
  `advmame.rc` sie belegt (dorthin schreiben VectorPies CONTROLS-Einstellungen
  und AdvanceMAMEs Menü); steht dort nichts oder "auto", gilt F2 wie in MAME.
  Der Menühinweis nennt die Taste. Tab öffnet das Menü nicht mehr (Tab ist
  AdvanceMAMEs UI CONFIG).
  Nur Grundfunktionen (der Rest steht in `redbaron.ini`). Seiten:
  * STEUERUNG: Knüppel analog (Tasten und Digitalstick wirken zusätzlich)
    oder digital, Empfindlichkeit (digital: wie schnell der gehaltene
    Knüppel ausschlägt, `digital_rate`; analog: `gain`); Belegung von
    Knüppelachsen, Richtungen, Feuer, Start, Münze (Feuer auf der Zeile,
    dann Taste/Knopf/Achse; links/rechts zurück auf „auto" = advmame.rc);
    hoch/runter umkehren. `platform/controls.c`. Daneben live der
    Yoke-Test: Knüppel, wie das Spiel ihn bekommt (Kreuz), die
    Analogachsen (Quadrat), Potiwerte, Feuer/Start/Münze.
  * BILD: Zwischenbilder, Farben, Verdeckung, Bäume, Wolken, Cockpit,
    Titel und Highscores, Gameplay im Attract, Helligkeit.
  * TESTBILD: ein Gitter mit Rand und Kreis.
  * TON: Lautstärke, Soundtest (Maschinengewehr, Explosion, Sturzflug,
    POKEY-Ton).
  * SPIELOPTIONEN: Flugzeuge, Bonus, Sprache, Münzen. Wirken sofort.
  * HIGHSCORES LÖSCHEN (Feuer zweimal): Tabelle auf null (mit
    `default_scores` wieder die Asse), die EAROM-Routine des Spiels schreibt
    sie mit Prüfsummen (wie nach einem neuen Eintrag).
* **Highscores im VectorPie-Menü** (Marquee, Hinweiszeile): `game.ini`
  `hiscores = yes`; das Spiel schreibt seine Tafel (HSCRES, 7 Einträge, BCD
  mal 10) zusätzlich als `save/highscores.txt` im Format des SDK
  (`scores_text`/`scores_file` in `main.c`), sobald sie sich geändert hat,
  nicht während der Initialeneingabe und nicht, solange das EAROM arbeitet;
  neue Datei, dann umbenannt. Am PC geprüft (7 Asse in der Datei); ob das
  Menü sie zeigt, ist am Automaten noch nicht gesehen (das Menü liest
  `game.ini` womöglich erst nach seinem Neustart).
* **Freispiel** (`coin_mode = free`, im Menü COINS: FREE PLAY; `main.c`).
  Der Freispiel-Schalter der Platine setzt im Attract sofort zwei Credits
  und bleibt für immer bei PRESS START stehen (CREDIT in RBARON.MAC), der
  Attract läuft dann nicht mehr. Hier steht die Platine auf "1 Münze 1
  Spiel"; START im Attract gibt dem Spiel die Münze (der Münzschalter
  einen Puls lang) und hält START, bis das Spiel beginnt. Statt INSERT
  COINS und CREDITS steht FREE PLAY und blinkend PRESS START (in der
  Sprache des Spiels).
* **Neue Bildschirme** (`screens = yes`; `render/screens.c`): Auf den
  Seiten des Spiels selbst (GAMODE) andere Bilder desselben Zustands; was
  das Spiel dort zeichnet (Kennung STATES), wird weggelassen.
  * Titel statt Banner (GAMODE 13): Streifen gleiten herein, der Kreis
    zeichnet sich, das Eiserne Kreuz dreht sich herein, der Adlerkopf
    blendet ein, "Red Baron" schreibt sich in einer Fraktur-Hand (breite
    Feder, beide Federkanten gezeichnet); danach alle 4 s ein Glanz über
    den Schriftzug. Nach dem Flyer und dem Marquee.
  * Highscore-Tafel (GAMODE x3): Überschrift fällt herein, zwei kleine
    drehende Kreuze, die Zeilen gleiten nacheinander herein, Platz 1
    wechselt die Farbe.
  * Initialeneingabe (GAMODE x7): NEUER REKORD blinkend, Punkte und Platz,
    drei Kästen (Buchstaben nach ihrer sichtbaren Breite mittig), der
    gewählte rot mit Pfeilen, Hinweise in der Sprache des Spiels, ein
    Balken für die verbleibende Zeit (TIMER). Bedient wird wie am Automaten
    (Knüppel auf/ab ändert, Feuer übernimmt: LETCHK, STATES).
  INSERT COINS und was eine Münze kauft rücken auf diesen Seiten nach unten.
  Texte in der Schrift des Spiels (`render/glyph.c`: die Zeichenroutinen
  des Vektor-ROMs einmal durch den Vektorgenerator gelaufen und als
  Linien behalten, frei skalier- und neigbar; Bindestrich und Punkt fehlen
  im Spiel).
* **Asse in leerer Tafel** (`default_scores = yes`; `main.c`): Ist die
  Tafel nach dem Einschalten leer (oder nach HIGHSCORES LÖSCHEN), stehen in
  den leeren Plätzen MVR 20000, UDT 15000, LOE 12000, VOS 10000, BOE 8000,
  IMM 6000, JAC 4000, immer unter dem letzten echten Eintrag; das EAROM
  wird über die Routine des Spiels geschrieben. Damit zeigt der Attract
  die Tafel (das Spiel tut das nur, wenn sie nicht leer ist).
* **Cockpit** (`cockpit = yes`; `render/cockpit.c`): das Cockpit eines
  Fokker Dr.I aus dem Sitz gesehen, nach einem Foto des Besitzers
  (kleines 3D-Modell, Fluchtpunkt in der Bildmitte): zwei Spandau-MGs nah
  an der Mitte, ihre Gehäuse auf dem Instrumentenbrett (Kühlmantel mit
  Löchern, Mündung, Kimme, Korn, vorn Ringvisiere mit Fadenkreuz); der
  Bug so breit wie der Rumpf (die MGs liegen oben drauf), davor der
  Propeller schwach als Kreis und Blattschleier; der Mittelflügel links
  und rechts, darauf je zwei Streben nach außen geneigt zur oberen
  Tragfläche; zwei gekreuzte Spanndrähte, eine kleine Windschutzscheibe,
  (Bug kurz, Propeller stilisiert wie im Film: an jeder Blattspitze ein
  glatter Wischbogen, vorn hell, nach hinten auslaufend, dazu ein schwächerer
  weiter innen, 27 Grad je Bild gedreht; kein Kreis; der Propeller des Spiels, drei Striche unten, fällt mit
  Cockpit weg; der gepolsterte Rand ist wieder entfernt; unten bis in die
  Ecken geschlossen, kein Blick nach draußen); das Instrumentenbrett mit
  Höhenmesser
  (I4YPOS), Fahrtmesser (schneller im Sturz: PLDELY) und
  Waffenthermometer (GUN.ST, überhitzt blinkt es rot). Die MGs schlagen beim Feuern abwechselnd zurück, wie das
  Spiel sie abwechselnd feuert (S.XFLG D6), mit Mündungsfeuer. Eigene
  Verdeckung: Kanonen und Streben sind Körper aus Flächen, gezeichnet
  werden nur die sichtbaren Kanten (Rückseiten weg, bei Rohren und Streben
  nur der Umriss; was ein Teil vor einem anderen verdeckt, fällt weg). Die
  Welt (Land, Berge, Flugzeuge, Zeppelin, Schüsse, der eigene Propeller)
  und die Copyright-Zeile verschwinden hinter allen Teilen
  (`rb_solid.front`); Texte und Anzeigen des Spiels bleiben davor, ebenso
  die Kugeln: das Spiel lässt sie unten seitlich beginnen (etwa x +-200,
  y -60), wo jetzt Flügel und Streben sind; verdeckt waren sie nicht mehr
  zu sehen. Nur ein
  Bild: am Spiel ändert sich nichts. Im Attract mit Welt auch sichtbar,
  nicht auf Titel/Tafel/Eingabe.
* **Aufgenommene Sounds** (`sounds = samples`, Testmenü SOUND → SOUNDS
  RECORDINGS/ORIGINAL; `sound/samples.c`, Dateien in `package/sounds`,
  geschnitten von `tools/make_sounds.py` aus Freesound-Aufnahmen in
  `assets/freesound_raw`, Quellen in `package/sounds/CREDITS.txt`). Die
  Aufnahmen hängen an denselben Signalen wie die Platine (Latch 1808 und
  POKEY), am Spiel ändert sich nichts:
  * eigener Motor: das Brummen (POKEY-Kanäle 3/4, AUDC A1) wird zum
    Fokker-Doppeldecker (Hollywood-Lichtton der 30er, „G01-10 Fokker
    Biplane Steady“, 14-s-Schleife mit Überblendung); im Sturz schneller
    und lauter, im Steigen langsamer (PLDELY, wie der Fahrtmesser);
    eigene Lautstärke `engine_volume` (Testmenü ENGINE VOLUME);
  * anfliegender Gegner (AUDF3 unter F8): ein Nieuport mit Umlaufmotor,
    höher und lauter, je höher der Ton des Originals steigt;
  * MG: jedes Ansteigen des Schuss-Bits (15-mal je Sekunde) ein Einzelschuss
    eines Maxim (die Waffe des LMG 08/15, 4 Varianten), beim Loslassen der
    Nachhall; gegnerisches Feuer leiser und tiefer (eigenes Feuer: SWSTAT
    D7 und nicht überhitzt);
  * Explosion: der Crash-Pegel steigt → eine von 4 Explosionen, so laut wie
    der Pegel;
  * Spirale (D1): ein trudelndes Flugzeug, jedes anders: eine von drei
    Aufnahmen (Warner-Brothers-Trudeln früh und spät, Sturzflug-Heulen),
    Tonhöhe 0,9 bis 1,12, fällt um 15 bis 35 % in 3 bis 5 s, Einstieg an
    zufälliger Stelle, wird beim Fallen leiser;
  * Spirale und Explosion nach Entfernung: das getroffene Bewegungsobjekt
    (PLOBDB, Status D6) mit X und Z, Lautstärke (400 / Abstand)^0,8
    zwischen 0,2 und 1, in der Ferne dumpfer (Tiefpass bis etwa 800 Hz);
    eigener Absturz (GMEND1 D7) voll, Bodenziele 0,6.
  Die Jingles (Punkte, Bonusleben, neues Flugzeug, 300 Punkte) bleiben beim
  POKEY. Die Kanäle 3/4 sind nur stumm, solange sie das Brummen tragen
  (AUDC A1), ein Jingle darauf ist zu hören. Alle Dateien Mono (ein
  Lautsprecher), Ogg Vorbis; 16 kHz für die alten Lichtton-Aufnahmen
  (darüber nur Rauschen, gemessen), 32 kHz für das MG; zusammen 363 KB.
  Soundtest im Menü: MACHINE GUN, EXPLOSION, NOSE DIVE, ENGINE, PLANE
  CLOSING, POKEY TONE.

## Geprüft

Gegen MAME 0.289, 20 Aufnahmen (`tools/captures.sh`): Attract, gespielte
Partien, lange Partien mit Bodensequenzen und Zeppelinen, mehr als fünf
Leben, Highscore-Eingabe und deren Zeitablauf, vier Sprachen,
Optionsschalter, Münzmodi, Freispiel, Slam, POKEY-Prüfung.

| Prüfung | Umfang | Ergebnis |
|---|---|---|
| C-Routinen Frame für Frame (`checkall.sh`) | 60.689 Frames, 4.969 Bytes je Frame | alle gleich |
| Originalprogramm auf dem 6502-Kern (`--emu`) | dieselben | alle gleich |
| Interrupt-Routine und Einschalten in C gegen das Original (`--nmi`) | alle 20 Aufnahmen | alle gleich |
| Gegenprobe: eine Konstante im C absichtlich verfälscht | 721 Frames | alle als abweichend gemeldet |
| Spiel im Live-Betrieb ohne Bildschirm (`rbshot`), C und `--original` | 60 s mit Münze, Start, Schießen | beide 90 Punkte, beide danach in der Initialen-Eingabe |
| Bild (`rbshot`, `docs/bilder`) | Attract und Spiel | Flugzeug, Horizont, Berge, Anzeigen, Texte wie erwartet |
| Programmstart im Fenster | 6 s | 60 Bilder pro Sekunde, rund 340 Vektoren je Bild, 0,3 ms je Bild mit Zwischenbildern |
| Zwischenbilder, t = 0 (`rbshot --smooth`) | 913 Frames einer gespielten Partie | in allen das Frame Wort für Wort |
| Zwischenbilder, t = 1 gegen das nächste Frame (`_scratch/subdiag2.c`) | 512 Frames | bei gleicher Linienzahl 0,9 Einheiten Abstand je Linienende (Bildbreite 520); in der Hälfte der Frames kommen Linien dazu oder fallen weg (neue Schüsse, Objekte im Bild, Blinken): die wechseln im Takt der Frames |
| Zwischenbilder am Bild (`rbshot --smooth --at`) | sechs Bilder in 100 ms | Flugzeuge wandern von Bild zu Bild |
| Farbe und Verdeckung am Bild (`rbshot`, `docs/bilder/farbe_verdeckung.png`) | Attract-Flugzeug zu vier Zeitpunkten, Spielszenen | Rumpflinien hinter der Nase und Streben hinter den Flügeln verschwinden, Kanten bleiben; je Bild 14 bis 26 Linien geschnitten |
| MAME-Vergleiche nach dem Einbau der Kennungen | alle 20 Aufnahmen | weiter alle gleich |
| Explosion, Bodensequenz, Bäume, Wolken am Bild (`rbshot --capture` mit Frames aus `kills` und `long`) | 8 Frames | Teile rot, Bodenobjekte mehrfarbig, Berge verdecken Bäume und Wolken dahinter |
| Nach den Ergänzungen: MAME-Vergleiche, Zwischenbilder bei t = 0 | alle 20 Aufnahmen, 605 Frames | weiter alle gleich |
| Verdeckung durch Berge (`RB_FACES=1 rbshot` zeichnet die verdeckenden Flächen mit) | Attract und Bodensequenz | jeder Berg eine geschlossene Fläche vom Grat bis zum Fuß, deckungsgleich mit seinen Linien |
| Knüppel: einmal rechts, dann losgelassen (`_scratch/pottrace.c`) | 10 s | ohne Kalibrierung PLDELX -30 auf Dauer, mit ihr zurück auf 0 |

Abdeckung der C-Routinen durch die Aufnahmen (`tools/coverage.sh`):
`math.c` und `alpha.c` 100 %, `display.c`, `main.c`, `grnd.c`, `int.c`
über 99 %, `objects.c`, `ground.c`, `collide.c`, `coin.c`, `sound.c` über
98 %, `vgut.c` 97 %, `earom.c` 93 %, `states.c` 91 %. Nicht durchlaufen:
EAROM-Lesen mit gültiger Prüfsumme (die Aufnahmen starten mit leerem
EAROM), Überträge in den Zählern für Spielanzahl und Spielzeit, Teile der
Selbstanpassung der Schwierigkeit, der Sprung in den Selbsttest und
einzelne seltene Zweige.

## Noch von niemandem gesehen oder gehört

* **Das Spielfenster in Bewegung.** Zwei Standbilder sind geprüft, der
  Start des Fensters auch. Gespielt hat es niemand.
* **Der Ton.** POKEY und die diskreten Sounds erzeugen Pegel (gemessen an
  einer WAV-Datei). Gehört hat ihn niemand; Klang und Lautstärken der
  Teile zueinander sind MAMEs Modell entnommen, nicht abgeglichen.
* **Die aufgenommenen Sounds.** Offline geprüft: `rbshot --samples`
  rendert ein Spiel (95 s: Motor, 5 anfliegende Gegner, 439 eigene und 44
  gegnerische Schüsse, 6 Explosionen, 5 Spiralen begonnen), Pegel
  verglichen (Motor allein etwa -32 dB gegen -43 dB des Original-Brummens,
  Feuern etwa -25 dB). Hörprobe: `docs/hoerprobe/` (neu und Original,
  dasselbe Spiel). Am Automaten nicht gehört; die Lautstärken der Teile
  zueinander (`*_GAIN` in `sound/samples.c`) sind nach Messung gesetzt,
  nicht nach Gehör.
* **Tastatur und Joystick.** Nur per Skript gefahren. Auf diesem PC sind
  vier Joystick-Geräte angemeldet (darunter vJoy und ein Pedal); ob eines
  davon den Knüppel verzieht, ist offen (der Steuerungstest zeigt es).
* **Testmenü.** Am PC-Fenster per Skript durchgegangen (Seiten als
  Bild geprüft, Belegen einer Taste, Start+Feuer-Halten, Speichern,
  Optionen wirken live, Highscores löschen schreibt das EAROM). Der
  Soundtest erzeugt Pegel (gemessen), gehört hat ihn niemand. Am Automaten
  nicht bedient.
* **Highscore-Datei.** Laden und Speichern sind geschrieben, aber nicht
  über einen Neustart hinweg ausprobiert.
* **Am Automaten gespielt.** Installiert und gestartet ist es
  (2026-10-14, siehe unten), gespielt hat es dort noch niemand.
* **Zwischenbilder in Bewegung.** Nur an Standbildern und Messwerten
  geprüft. Wie Bodensequenz, Zeppelin und Absturz aussehen, ist ungesehen.

## Bekannte Abweichungen vom Automaten

* **Tempo.** Der Automat rechnet 10,25 Spielframes pro Sekunde (alle 24
  Interrupts) und braucht für einen Frame bis zu 172.000 Zyklen, also
  manchmal länger als die 147.456, die er hat. Das C braucht keine Zeit
  und läuft immer mit 10,25. Im Banner des Attract-Modus sind es 20,5.
* **Zufallszahlen.** Im Live-Betrieb aus einem eigenen Schieberegister,
  nicht aus einem zyklusgenauen POKEY.
* **Math Box.** Auf Routinen-Ebene wie in MAME, sofort fertig.
* **Vektorgenerator.** Gilt im Live-Betrieb beim nächsten Interrupt als
  fertig.
* **Start-Knopf.** Bit 6 von 1802 liegt hier wie am Automaten bei
  gedrücktem Knopf auf 0. MAMEs Port ist andersherum; dort startet das
  Spiel von selbst, sobald ein Credit da ist.
* **Münzzähler, Start-LED** sind nicht nach außen geführt.

## Am Automaten

Installiert mit `tools/deploy.sh` (nur kopieren) in `/persistent/games/rbenhanced`,
Stand 2026-10-15 der aktuelle Build (0.5.1, mit den aufgenommenen Sounds in
`sounds/`); gestartet wird er vom Besitzer aus dem Menü. Vektorplatine (USB-DVG), 60
Bilder pro Sekunde; mit Cockpit, Bäumen, Wolken und Demo bis rund 700
Vektoren (siehe Grenzen). Die ROMs kommen aus
`/usr/local/share/advance/rom/redbaron.zip`. Marquee: `package/marquee.png`
(1920x360, die Größe des HDMI-Marquee-Schirms; `advmame.rc` streckt mit
`vector_aux_marquee_display_mode stretch`), vom Besitzer geliefert. Paket:
`tools/make_vpk.sh` macht `build/rbenhanced-<Version>.vpk`; 0.5.1 liegt seit
2026-10-16 im öffentlichen Google-Drive-Ordner „games“ (vom Besitzer
geteilt, neben den anderen Paketen). Mit INSTALL GAMES installiert hat es
noch niemand. Ob der Menüeintrag erscheint, ohne dass das Menü neu startet, ist nicht geprüft.

## Bekannte Grenzen der Ergänzungen

* Zwischenbilder: Was neu ins Bild kommt oder verschwindet, Blinken,
  Zähler und Texte wechseln wie im Original im Takt der Frames. Der
  Knüppel wirkt erst mit dem nächsten Frame (wie im Original). Drehungen
  werden geradlinig überblendet. Ein Frame, in dem der Spieler springt
  (neues Spiel, neues Leben), wird nicht überblendet. Die Seiten ohne Welt
  (Banner, Highscores) haben keine Zwischenbilder; bei Star Wars wandern
  dort die Linien zum nächsten Frame, hier noch nicht.
* Verdeckung: Drohnen, Zeppelin und Explosionsteile verdecken nichts.
  Innerhalb eines Bergzugs (vier Stücke einer Landschaft, alle in einer
  Entfernung) bleiben sich kreuzende Grate stehen. Linien ohne bekannte
  Entfernung (Horizont) gelten als dahinter.
* Bäume und Wolken kosten Zeit auf der Vektorplatine: rund 470 statt 340
  Vektoren je Bild, der Leerweg verdoppelt sich etwa. Ob das Bild auf der
  Röhre noch ruhig steht, ist am Automaten zu prüfen; sonst `trees = no`
  oder weniger Bäume (`MAX_TREES` in `render/deco.c`).
* Bäume stehen auch dort, wo in der Bodensequenz Berge herankommen (sie
  gehören zu keinem Berg).
* Am Bildrand erscheint ein Baum ganz, sobald seine Mitte im Bild ist (SETBM
  des Spiels prüft nur die Mitte), also halb hereingeschoben.
* Texte oben und Meldungen haben eigene Farben (Beschriftungen bernstein,
  Punkte warmweiß, Wert grün, Highscore hellblau, INSERT COINS gold, was
  eine Münze kauft violett, BONUS LIFE / 300 POINTS / BEHIND YOU rosa,
  überhitzte Waffen rot, Leben rot): eigene Kennungen um die Aufrufe in
  ALPHAS.
* Farben sind ein erster Vorschlag, mit dem Besitzer abzustimmen
  (`render/color.c`, eine Tabelle).
* Kalibrierung: gilt für die Werte, die der Port selbst erzeugt. Mit
  `--original` (6502-Kern) nicht.
* Testmenü: Eine Taste, die kürzer als ein Bild (1/60 s) gedrückt ist,
  wird nicht gesehen (Tastaturzustand je Bild). Beim Belegen zählt eine
  Taste erst, wenn vorher alles losgelassen war. Esc beendet außerhalb des
  Belegens das Programm (SDK). Start+Feuer-Halten startet im Attract-Modus
  bei Credit nebenbei ein Spiel.
* Wolken liegen über den Texten (z. B. INSERT COINS), sie werden nicht
  von ihnen verdeckt.
* Cockpit: kostet Vektoren (rund 200 Linien mehr je Bild). Am Automaten
  gemessen (2026-10-14): mit Cockpit, Bäumen, Wolken und Demo rund 700
  Vektoren, Strahlweg rund 95 000 hell + 140 000 dunkel je Bild; die
  USB-DVG schafft das nicht in 1/60 s, die Bildrate fällt nach einigen
  Sekunden auf 30. Ohne Bäume/Wolken stabil 60. Die Rechenzeit ist es
  nicht (unter 0,6 ms je Bild am PC). Ein Umsortieren der Linien nach
  kürzestem Leerweg ist verboten (Besitzer: funktioniert praktisch nie,
  das Bild wackelt) und wieder entfernt. Offen: weniger Linien/Details. Es verdeckt,
  wie im echten Flugzeug, viel: unten das Brett, an den Seiten unterhalb
  von etwa -70 den Mittelflügel; Ziele dort sind nicht zu sehen. Der
  Fahrtmesser ist erfunden (das Spiel kennt keine Geschwindigkeit), er
  folgt dem Steigen und Sinken. Das Armaturenbrett deckt den unteren Rand
  (Copyright) ab, wie die Blende am Automaten. Der Propeller dreht sich
  langsam wie im Film (ein echter wäre ein gleichmäßiger Schleier).
* Titel: Die Seite dauert, so lange das Banner des Spiels flöge (rund
  11 s); die Animation endet nicht weich, sie bricht mit dem Seitenwechsel
  ab. Die Fraktur ist eine eigene Zeichnung nach dem Flyer, keine Schrift.
* Freispiel: Die Münzen zählen in der Statistik des Spiels mit (EAROM,
  Münzzähler). Das ursprüngliche Freispiel der Platine ist nicht mehr
  wählbar.
* Leeres EAROM (frische Installation): Bis die Lese-Routine des Spiels
  fertig ist, steht als Highscore kurz „FFFF0" (wie am Automaten mit
  leerem Baustein). HIGHSCORES LÖSCHEN im Testmenü schreibt ein gültiges.

* Aufgenommene Sounds: Die Lichtton-Aufnahmen rauschen (nicht
  entrauscht, nur oberhalb 8 kHz abgeschnitten). Der eigene Motor folgt
  nur Steigen und Sinken; Gas gibt es im Spiel nicht. Ob ein Schuss vom
  Spieler oder vom Gegner kommt, wird aus dem Feuerknopf geschlossen;
  feuern beide zugleich, klingt alles wie eigenes Feuer. Explosion und
  Spirale wählen nicht nach dem, was getroffen ist (Flugzeug, Zeppelin,
  Bodenziel), das Spiel gibt dafür dasselbe Signal. Die Entfernung zählt
  nur X und Z, nicht die Höhe; Bodenziele haben eine feste Lautstärke
  (0,6), ihre Lage wird nicht gelesen.

## Nicht portiert

* Der Selbsttest (RBINT.MAC ab `RAMTST`). Er prüft Hardware, die es hier
  nicht gibt, und sein Quelltext ist unvollständig. Mit `--original` läuft
  er auf dem 6502-Kern (Testschalter noch auf keiner Taste).
* Die Absturzprüfung im Interrupt (Stack, verlorene Frames) hält das
  Programm hier an und meldet es, statt auf den Watchdog zu warten.

## Offene Punkte

1. Spielen, Ton anhören, Joystick ausprobieren, Zwischenbilder ansehen
   (siehe oben).
2. Weitere Ergänzungen festlegen und bauen (siehe RESUME.md).
3. 0.5.1 (kopiert 2026-10-15) ansehen; Testmenü, Titel, Cockpit, Freispiel und die
   aufgenommenen Sounds am Automaten ansehen, anhören und bedienen
   (Lautstärken der Sounds nach Gehör nachstellen).
4. Das `.vpk` über INSTALL GAMES am Automaten installieren und prüfen.
   0.5.2 (Highscores fürs Menü) ist gebaut (`build/rbenhanced-0.5.2.vpk`),
   aber weder auf den Automaten kopiert noch in den Drive-Ordner geladen.
5. Kein Git-Repository angelegt, nichts committet.
6. Abdeckung: Aufnahme mit gefülltem EAROM (zweiter Lauf ohne Löschen von
   `_scratch/mame/nvram`).

## Nächste Schritte

Siehe [RESUME.md](RESUME.md).
