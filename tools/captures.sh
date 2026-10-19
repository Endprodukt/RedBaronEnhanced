#!/bin/sh
# captures.sh - record the standard set of MAME runs the port is checked
# against (tools/checkall.sh), into a folder (default _scratch/cap).
#
#   sh tools/captures.sh [FOLDER]
#
# Each run is MAME's redbaron played by a script (tools/mame/rbcapture.lua):
# coins, start, then a player that steers at the plane and fires, or one
# that flies about at random. What play does not reach by itself is poked
# into the machine.
#   LIVES 00B0, SCORE 00A9/00AA (BCD, in tens), credits ($$CRDT) 00F4,
#   $LMTIM (the slam's timer) 00F2, GMLEVL (difficulty) 00A4,
#   STATUS+1 (the POKEY check's result) 017B, TIMER+1 00BC.
# The option switches: DSW0 is OPT0 (0A00, coins; FD as shipped), DSW1 is
# OPT1 (0C00; E7 as shipped: English, bonus at 6k/20k/50k, 3 lives).
cd "$(dirname "$0")/.." || exit 1
dir=${1:-_scratch/cap}
mkdir -p "$dir"
COIN="60 coin 2;70 coin 2;80 coin 2;90 coin 2"
GAME="$COIN;130 start 2"
LIVES="400 poke B0 05;900 poke B0 05;1500 poke B0 05;2200 poke B0 05;3000 poke B0 05;4000 poke B0 05;5000 poke B0 05;6000 poke B0 05;7000 poke B0 05;8000 poke B0 05"

cap() { echo "== $1"; sh tools/capture.sh "$dir/$1.rbcap" "$2" "$3" | grep rbcapture; }

cap attract   150 ""
cap play1     240 "$GAME;140 auto 4"
cap kills     400 "$GAME;140 aim 3"
# a long game: the levels, the ground sequences, blimps
cap long      900 "$GAME;140 aim 3;$LIVES"
cap long2     900 "$GAME;140 aim 2;150 poke A4 03;$LIVES;2500 auto 3;3500 aim 4;5500 auto 6;6500 aim 3"
# more lives than the row shows
cap lives7    150 "$GAME;140 aim 3;150 poke B0 07;400 poke B0 09"
# game over with a high score: the initials by the stick (up and down
# through the letters) and the fire button, then a game without a score,
# then one that goes to the top of the table
INIT="stick 128 250"
HS="$GAME;140 aim 3;600 poke B0 01;610 auto 3"
HS="$HS;900 stick 128 250;930 stick 128 128;935 fire 2;945 stick 128 6;990 stick 128 128;995 fire 2;1005 stick 128 250;1012 stick 128 128;1016 fire 2"
HS="$HS;1100 start 2;1110 stick 128 128;1300 poke B0 01;1310 auto 5"
HS="$HS;1700 start 2;1710 aim 3;1800 poke AA 30;2200 poke B0 01;2210 auto 3;2500 stick 128 6;2540 fire 2;2550 fire 2;2560 fire 2"
cap hiscore   400 "$HS"
# nobody enters initials: the entry times out
cap hstime    330 "$GAME;140 aim 3;500 poke B0 01;510 stick 128 128;700 stick 128 128"
# the other languages (OPT1 D1,D0)
cap german    200 "5 dip DSW1 E4;$GAME;140 aim 3;700 poke B0 01;710 auto 6"
cap french    200 "5 dip DSW1 E5;$GAME;140 aim 3;700 poke B0 01;710 auto 6"
cap spanish   200 "5 dip DSW1 E6;$GAME;140 aim 3;700 poke B0 01;710 auto 6"
# the other option settings: 5 lives and the bonus at 2000; no bonus, 2
# lives; two plays minimum; the game's own adjusting of the difficulty
cap bonus2k   300 "5 dip DSW1 CF;$GAME;140 aim 2;$LIVES"
cap nobonus   200 "5 dip DSW1 F3;$GAME;140 aim 3"
cap twomin    250 "5 dip DSW1 A7;60 coin 2;200 coin 2;260 start 2;270 aim 3;700 poke B0 01;710 auto 4;1300 start 2;1310 aim 4"
cap adjust    500 "5 dip DSW1 67;$GAME;140 aim 3;500 poke B0 01;510 auto 4;900 start 2;910 aim 3;1500 poke B0 01;1510 auto 4;2100 start 2;2110 auto 3;2600 start 2;2610 aim 3"
# free play; 2 plays a coin with bonus coins; 2 coins a play
cap freeplay  150 "5 dip DSW0 FF;130 start 2;140 aim 4"
cap twoplays  150 "5 dip DSW0 3E;60 coin 2;64 coin 2;68 coin 2;130 start 2;140 auto 6"
cap twocoins  150 "5 dip DSW0 5C;$COIN;100 coin 2;110 coin 2;130 start 2;140 auto 6"
# the slam switch (its timer poked), more credits than the program keeps,
# and the POKEY check failing
cap slam      120 "$GAME;140 aim 3;300 poke F2 F0;500 poke F4 60;700 poke F2 80"
cap nopokey   100 "$GAME;140 aim 3;300 poke 017B 80;500 poke 017B 80"
ls -la "$dir"
