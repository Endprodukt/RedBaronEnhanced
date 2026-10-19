#!/bin/sh
# coverage.sh - which lines of the C routines (src/game) the recordings
# run through. Needs the coverage build (BUILD.md) in build/cov.
#
#   sh tools/coverage.sh [FOLDER] [--lines]
#
# --lines also lists every line never run.
cd "$(dirname "$0")/.." || exit 1
dir=_scratch/cap
lines=
for a in "$@"; do
    case "$a" in
        --lines) lines=1 ;;
        *) dir=$a ;;
    esac
done
command -v gcov > /dev/null || { echo "coverage.sh: gcov is not in the PATH (BUILD.md)"; exit 1; }
find build/cov -name "*.gcda" -delete
for f in "$dir"/*.rbcap; do
    ./build/cov/rbcompare "$f" --quiet > /dev/null
    ./build/cov/rbnmi "$f" > /dev/null
done
mkdir -p build/cov/gc
cd build/cov/gc || exit 1
for f in main states objects ground display alpha collide math coin sound grnd vgut earom int frame; do
    gcov ../CMakeFiles/rbcore.dir/src/game/$f.c.gcda 2>/dev/null | grep -A1 "src/game/$f.c'" | grep "Lines" | sed "s/^/$f.c: /"
    if [ -n "$lines" ]; then
        grep "#####" $f.c.gcov | sed 's/^ *#####: *//' | cut -c1-110
    fi
done
