#include "rom/romset.h"

#include "miniz.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

enum dest { D_ROM, D_AVG, D_MBMAP, D_MBPROM };

typedef struct rom_entry {
    const char *name;     /* MAME's name, for messages only */
    uint32_t    crc;
    uint32_t    size;
    enum dest   dest;
    uint32_t    offset;   /* D_ROM: the address; D_MBPROM: which */
    uint32_t    offset2;  /* D_ROM of 4K: where its second half goes */
    int         optional; /* not needed by the game */
} rom_entry;

/* MAME "redbaron" and "redbarona" (src/mame/atari/bzone.cpp). The two sets
 * hold the same program: redbaron has 4800 and 5800 in one 4K ROM. */
static const rom_entry ROMS[] = {
    { "037587-01.fh1", 0x60f23983, 0x1000, D_ROM, 0x4800, 0x5800, 0 },
    { "037001-01e.e1", 0xb9486a6a, 0x0800, D_ROM, 0x4800, 0, 0 },
    { "036999-01e.j1", 0x48d49819, 0x0800, D_ROM, 0x5800, 0, 0 },
    { "037000-01.e1",  0x69bed808, 0x0800, D_ROM, 0x5000, 0, 0 },
    { "036998-01.j1",  0xd1104dd7, 0x0800, D_ROM, 0x6000, 0, 0 },
    { "036997-01.k1",  0x7434acb4, 0x0800, D_ROM, 0x6800, 0, 0 },
    { "036996-01.lm1", 0xc0e7589e, 0x0800, D_ROM, 0x7000, 0, 0 },
    { "036995-01.n1",  0xad81d1da, 0x0800, D_ROM, 0x7800, 0, 0 },
    { "037006-01.bc3", 0x9fcffea0, 0x0800, D_ROM, 0x3000, 0, 0 },
    { "037007-01.a3",  0x60250ede, 0x0800, D_ROM, 0x3800, 0, 0 },
    { "036408-01.k7",  0x5903af03, 0x0100, D_AVG, 0, 0, 1 },
    { "036174-01.a1",  0x8b04f921, 0x0020, D_MBMAP, 0, 0, 1 },
    { "036175-01.e1",  0x2af82e87, 0x0100, D_MBPROM, 0, 0, 1 },
    { "036176-01.f1",  0xb31f6e24, 0x0100, D_MBPROM, 1, 0, 1 },
    { "036177-01.h1",  0x8119b847, 0x0100, D_MBPROM, 2, 0, 1 },
    { "036178-01.j1",  0x09f5a4d5, 0x0100, D_MBPROM, 3, 0, 1 },
    { "036179-01.k1",  0x823b61ae, 0x0100, D_MBPROM, 4, 0, 1 },
    { "036180-01.l1",  0x276eadd5, 0x0100, D_MBPROM, 5, 0, 1 },
};
#define NROMS ((int)(sizeof ROMS / sizeof ROMS[0]))
#define ALL_FOUND ((1u << NROMS) - 1u)

/* The 2K pieces of 3000-7FFF the game needs: all but 4000. */
#define SLOT(addr) (1u << (((addr) - 0x3000) / 0x800))
#define NEEDED_SLOTS (0x3ffu & ~SLOT(0x4000))

static int complete(const rb_romset *rs)
{
    return (rs->slots & NEEDED_SLOTS) == NEEDED_SLOTS;
}

static void place(rb_romset *rs, int i, const uint8_t *data)
{
    const rom_entry *e = &ROMS[i];
    switch (e->dest) {
    case D_ROM:
        memcpy(rs->rom + (e->offset - 0x3000), data, 0x800);
        rs->slots |= SLOT(e->offset);
        if (e->size == 0x1000) {
            memcpy(rs->rom + (e->offset2 - 0x3000), data + 0x800, 0x800);
            rs->slots |= SLOT(e->offset2);
        }
        break;
    case D_AVG:    memcpy(rs->avg_prom, data, e->size); break;
    case D_MBMAP:  memcpy(rs->mb_map, data, e->size); break;
    case D_MBPROM: memcpy(rs->mb_prom[e->offset], data, e->size); break;
    }
    rs->found |= 1u << i;
}

/* Offer one file's contents: placed if it is a wanted ROM not yet found. */
static void offer(rb_romset *rs, const uint8_t *data, size_t size)
{
    uint32_t crc = (uint32_t)mz_crc32(MZ_CRC32_INIT, data, size);
    for (int i = 0; i < NROMS; i++) {
        if (ROMS[i].crc == crc && ROMS[i].size == size && !(rs->found & (1u << i)))
            place(rs, i, data);
    }
}

static int wanted_size(size_t size)
{
    for (int i = 0; i < NROMS; i++)
        if (ROMS[i].size == size) return 1;
    return 0;
}

static void scan_zip(rb_romset *rs, const char *path)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof zip);
    if (!mz_zip_reader_init_file(&zip, path, 0)) return;
    mz_uint n = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < n && rs->found != ALL_FOUND; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_is_directory) continue;
        if (!wanted_size((size_t)st.m_uncomp_size)) continue;
        /* The zip's own CRC tells whether the file is worth extracting. */
        int match = 0;
        for (int k = 0; k < NROMS; k++)
            if (ROMS[k].crc == st.m_crc32 && !(rs->found & (1u << k))) match = 1;
        if (!match) continue;
        size_t size = 0;
        void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (data) {
            offer(rs, (const uint8_t *)data, size);
            mz_free(data);
        }
    }
    mz_zip_reader_end(&zip);
}

static void scan_file(rb_romset *rs, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return;
    uint8_t buf[0x1000 + 1];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (n < sizeof buf && wanted_size(n)) offer(rs, buf, n);
}

static int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ends_with_zip(const char *path)
{
    size_t n = strlen(path);
    return n > 4 && (strcmp(path + n - 4, ".zip") == 0 || strcmp(path + n - 4, ".ZIP") == 0);
}

static void scan_dir(rb_romset *rs, const char *path, int depth)
{
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *de;
    char full[2048];
    while ((de = readdir(d)) != NULL && rs->found != ALL_FOUND) {
        if (de->d_name[0] == '.') continue;
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        if (is_dir(full)) {
            /* an unpacked redbaron.zip keeps the other set in the folder redbarona */
            if (depth == 0 && strcmp(de->d_name, "redbarona") == 0) scan_dir(rs, full, 1);
        } else if (!ends_with_zip(full)) {
            scan_file(rs, full);
        }
    }
    closedir(d);
}

int rb_romset_load(rb_romset *rs, const char *const *paths, int npaths)
{
    memset(rs, 0, sizeof *rs);
    for (int i = 0; i < npaths && rs->found != ALL_FOUND; i++) {
        if (!paths[i] || !*paths[i]) continue;
        if (complete(rs)) break;
        if (is_dir(paths[i])) scan_dir(rs, paths[i], 0);
        else if (ends_with_zip(paths[i])) scan_zip(rs, paths[i]);
        else scan_file(rs, paths[i]);
    }
    return complete(rs) ? 0 : -1;
}

const char *rb_romset_missing(const rb_romset *rs, char *buf, int size)
{
    int len = 0;
    buf[0] = '\0';
    for (int i = 0; i < NROMS; i++) {
        const rom_entry *e = &ROMS[i];
        if (e->dest != D_ROM || (rs->found & (1u << i))) continue;
        if ((rs->slots & SLOT(e->offset)) && (!e->offset2 || (rs->slots & SLOT(e->offset2)))) continue;
        if (e->size == 0x800 && (e->offset == 0x4800 || e->offset == 0x5800)) continue;   /* (named as the 4K ROM) */
        len += snprintf(buf + len, len < size ? (size_t)(size - len) : 0, "%s%s", len ? ", " : "", e->name);
        if (len >= size) break;
    }
    return buf;
}

#define MAX_STORE 48
static char g_store[MAX_STORE][1024];
static int  g_nstore;

int rb_romset_add_dir(const char **out, int n, int max, const char *dir)
{
    static const char *const forms[] = { "%s/redbaron.zip", "%s/redbarona.zip", "%s" };
    for (int f = 0; f < 3 && n < max && g_nstore < MAX_STORE; f++) {
        snprintf(g_store[g_nstore], sizeof g_store[g_nstore], forms[f], dir);
        out[n++] = g_store[g_nstore++];
    }
    return n;
}

int rb_romset_default_paths(const char **out, int max)
{
    int n = 0;
    static char envcopy[2048];

    const char *env = getenv("RB_ROMS");
    if (env && *env) {
        snprintf(envcopy, sizeof envcopy, "%s", env);
        /* ';' separates on Windows, where a path holds a ':' */
        const char *sep = strchr(envcopy, ';') || (envcopy[1] == ':') ? ";" : ":";
        for (char *p = strtok(envcopy, sep); p && n < max; p = strtok(NULL, sep))
            n = rb_romset_add_dir(out, n, max, p);
    }

    n = rb_romset_add_dir(out, n, max, "roms");
    const char *home = getenv("HOME");
    static char h1[1024], h2[1024];
    if (home) {
        snprintf(h1, sizeof h1, "%s/.advance/rom", home);
        snprintf(h2, sizeof h2, "%s/mame/roms", home);
        n = rb_romset_add_dir(out, n, max, h1);
        n = rb_romset_add_dir(out, n, max, h2);
    }
    n = rb_romset_add_dir(out, n, max, "/home/pi/.advance/rom");
    return n;
}
