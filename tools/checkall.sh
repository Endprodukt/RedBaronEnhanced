#!/bin/sh
# checkall.sh - hold the port against every MAME recording of a folder.
#
#   sh tools/checkall.sh [FOLDER] [--emu | --nmi]
#
# Without an option: the C routines of src/game, frame by frame
# (rbcompare). --emu: the original program on the 6502 core instead.
# --nmi: the interrupt routine and the power-on in C against the
# original's on the core (rbnmi), from the memory of the recordings.
# Recordings: tools/captures.sh. The programs: build/pc.
cd "$(dirname "$0")/.." || exit 1
dir=_scratch/cap
mode=
for a in "$@"; do
    case "$a" in
        --emu|--nmi) mode=$a ;;
        *) dir=$a ;;
    esac
done
bad=0
for f in "$dir"/*.rbcap; do
    if [ "$mode" = --nmi ]; then
        ./build/pc/rbnmi "$f" || bad=1
    else
        ./build/pc/rbcompare "$f" $mode --max 3 --show 8 || bad=1
    fi
done
[ $bad = 0 ] && echo "ALL THE SAME" || echo "DIFFERENCES"
exit $bad
