/* romset.h - the original ROMs, read from the owner's MAME ROM set.
 *
 * The game's data (the vector ROM's pictures and letters, the planes, the
 * landscape, the tunes) stays in the ROMs and is read from the cabinet's
 * own set at start-up; none of it is part of this program.
 *
 * The source this port follows (historicalsource/red-baron, linked
 * 14-SEP-81) is the program of MAME's sets "redbaron" and "redbarona"
 * (the same bytes; redbaron has 4800 and 5800 in one 4K ROM).
 *
 * Files are matched by CRC32, so names and zip layout do not matter.
 */
#ifndef RB_ROMSET_H
#define RB_ROMSET_H

#include <stdint.h>

typedef struct rb_romset {
    uint8_t  rom[0x5000];       /* 3000-7FFF: vector ROM, then the program */
    uint8_t  avg_prom[0x100];   /* the vector generator's state PROM (not needed) */
    uint8_t  mb_map[0x20];      /* math box mapping PROM (not needed) */
    uint8_t  mb_prom[6][0x100]; /* math box micro-code, six 256x4 PROMs (not needed) */
    uint32_t found;             /* bit per entry of the ROM table */
    uint32_t slots;             /* bit per 2K of 3000-7FFF that is filled */
} rb_romset;

/* Look in each path (a .zip or a directory) for the set's ROMs, in order,
 * until all are found. Returns 0 when the game can run; otherwise -1 and
 * rb_romset_missing() names what is missing. */
int rb_romset_load(rb_romset *rs, const char *const *paths, int npaths);

/* The ROMs not found, as a comma-separated list (empty when complete). */
const char *rb_romset_missing(const rb_romset *rs, char *buf, int size);

/* The default search list: $RB_ROMS (a ';'- or ':'-separated list), then
 * ./roms and the usual AdvanceMAME/MAME rom folders, each tried as
 * redbaron.zip, redbarona.zip and a directory. Fills up to max entries with
 * pointers into an internal buffer; returns the count. */
int rb_romset_default_paths(const char **out, int max);

/* The same list for one more folder (a command line option, a setting):
 * its forms appended at out[n]; returns the new count. */
int rb_romset_add_dir(const char **out, int n, int max, const char *dir);

#endif
