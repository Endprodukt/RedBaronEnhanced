#!/bin/sh
# capture.sh - record a run of Red Baron (MAME set redbaron) for rbcompare.
#
#   sh tools/capture.sh OUT.rbcap SECONDS ["PLAY SCRIPT"]
#
# MAME and its ROM folder: $RB_MAME (default D:/Mame/mame.exe) and
# $RB_MAME_ROMS (default D:/Mame/roms). MAME runs without picture and
# sound, as fast as it can, with a configuration folder of its own
# (_scratch/mame), so that the owner's MAME settings stay untouched.
# The play script: see tools/mame/rbcapture.lua.
here=$(cd "$(dirname "$0")/.." && { pwd -W 2>/dev/null || pwd; })
out=$1
secs=${2:-150}
play=$3
case "$out" in
    /*|?:*) ;;
    *) out="$({ pwd -W 2>/dev/null || pwd; })/$out" ;;
esac
# (a fresh configuration every time: MAME keeps option switches a script
# has set, and the next recording would start with them)
rm -rf "$here/_scratch/mame/cfg" "$here/_scratch/mame/nvram"
mkdir -p "$here/_scratch/mame/cfg" "$here/_scratch/mame/nvram"
cd "$here/_scratch/mame" || exit 1
RBCAP_OUT="$out" RBCAP_PLAY="$play" RBCAP_OWNED="$out.owned" "${RB_MAME:-D:/Mame/mame.exe}" redbaron \
    -rompath "${RB_MAME_ROMS:-D:/Mame/roms}" -cfg_directory cfg -nvram_directory nvram \
    -autoboot_script "$here/tools/mame/rbcapture.lua" \
    -video none -sound none -nothrottle -skip_gameinfo -seconds_to_run "$secs" 2>&1 | grep -i "rbcapture\|error"
