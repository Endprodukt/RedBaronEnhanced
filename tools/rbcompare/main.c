/* rbcompare - check the game frame by frame against MAME.
 *
 *   rbcompare CAPTURE.rbcap [--emu] [--syms rb_symbols.h]
 *             [--show N] [--max N] [--from F] [--to F] [--trace F] [--quiet]
 *
 * The capture (tools/mame/rbcapture.lua, MAME set redbaron) holds the
 * original's memory at the start of every game frame (FRAM). For each pair
 * of consecutive frames the machine is loaded with the first, fed the
 * random numbers and the reads the original made of what the interrupt
 * keeps, runs one frame, and must then hold what the second has: RAM, the
 * math box and the display list just written. Every frame is checked on
 * its own, so one difference does not hide the next.
 *
 *   default  the C routines of src/game
 *   --emu    the original program on the 6502 core (checks the core, the
 *            machine and the math box; needs the capture's own ROMs only)
 *
 * Not compared: what the interrupt wrote during the frame (the capture
 * lists it frame by frame: NWRT), and the 6502's stack.
 *
 * Exit status 0 when every frame compared matches.
 */
#include "capture/capture.h"
#include "core/emu.h"
#include "core/machine.h"
#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- symbol names for the report --------------------------------------------- */

typedef struct sym { uint16_t addr; char name[32]; } sym;
static sym  *syms;
static int   nsyms;

static int sym_cmp(const void *a, const void *b)
{
    return (int)((const sym *)a)->addr - (int)((const sym *)b)->addr;
}

static void load_syms(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256], name[64];
    unsigned addr;
    int ram = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "/* RBARON */")) ram = 1;
        else if (!strncmp(line, "/* ", 3)) ram = 0;
        if (sscanf(line, "#define RB_%63s 0x%x", name, &addr) != 2) continue;
        if (addr >= 0x400 || !ram) continue;
        /* (constants share the header: a RAM cell's name is taken from the first module only,
         * and of two names for one address the later one is kept as an alias) */
        syms = (sym *)realloc(syms, (size_t)(nsyms + 1) * sizeof *syms);
        syms[nsyms].addr = (uint16_t)addr;
        snprintf(syms[nsyms].name, sizeof syms[nsyms].name, "%.31s", name);
        nsyms++;
    }
    fclose(f);
    qsort(syms, (size_t)nsyms, sizeof *syms, sym_cmp);
}

/* The RAM cells' names are found by the layout of the source (the .BLKB
 * lines), which this table repeats for the report only. */
static const struct { uint16_t addr; const char *name; } CELLS[] = {
    { 0x00, "S.FLAG" }, { 0x01, "BUFFER" }, { 0x02, "F.CNTR" }, { 0x03, "C.CNTR" }, { 0x04, "CALFLG" },
    { 0x05, "RAMPTR" }, { 0x07, "POINTR" }, { 0x09, "COPYPT" }, { 0x0B, "XPOS" }, { 0x0F, "UNIV4X" },
    { 0x13, "XCOMP" }, { 0x17, "P.1ST" }, { 0x33, "P.2ST" }, { 0x4F, "PLSTAT" }, { 0x6B, "OBJECT" },
    { 0x73, "S.XFLG" }, { 0x74, "TEMP1" }, { 0x76, "TEMP2" }, { 0x78, "TEMP3" }, { 0x81, "TEMP4" },
    { 0x83, "FLAG" }, { 0x85, "I.TEMP" }, { 0x86, "SAVY" }, { 0x89, "SAVX" }, { 0x8C, "XPOINT" },
    { 0x90, "DRNFLG" }, { 0x91, "N.OBJ" }, { 0x92, "INTFLG" }, { 0x93, "INTCNT" }, { 0x94, "V.BRIT" },
    { 0x95, "VGBRIT" }, { 0x96, "VGSIZE" }, { 0x97, "PLVALU" }, { 0x98, "BNTIME" }, { 0x99, "SCRTAB" },
    { 0x9D, "OBJKLD" }, { 0x9F, "BEFLAG" }, { 0xA0, "N.PLNZ" }, { 0xA1, "VALFRC" }, { 0xA3, "EXLIFE" },
    { 0xA4, "GMLEVL" }, { 0xA5, "GRMODE" }, { 0xA6, "ST.LED" }, { 0xA7, "$BCCNT" }, { 0xA8, "$BC" },
    { 0xA9, "SCORE" }, { 0xAB, "FRAME" }, { 0xAC, "SECCNT" }, { 0xAE, "EAWRTE" }, { 0xAF, "SCRTIM" },
    { 0xB0, "LIVES" }, { 0xB1, "OPTION" }, { 0xB2, "GAMODE" }, { 0xB3, "SHMODE" }, { 0xB4, "STARTF" },
    { 0xB5, "MODECT" }, { 0xB6, "GRNDCT" }, { 0xB7, "GRDSP" }, { 0xB8, "NEWCT" }, { 0xB9, "GTIMER" },
    { 0xBB, "TIMER" }, { 0xBD, "PLDELX" }, { 0xBE, "PLDELY" }, { 0xC0, "PFROTN" }, { 0xC2, "PFXSCR" },
    { 0xC4, "DRNSET" }, { 0xC5, "DRNSFL" }, { 0xC6, "GREND" }, { 0xC7, "GMEND0" }, { 0xC9, "B.HOLE" },
    { 0xCA, "EOGTMR" }, { 0xCB, "GUN.ST" }, { 0xCC, "SPIRAL" }, { 0xCD, "EXCNTR" }, { 0xCE, "EXPVAL" },
    { 0xCF, "S.VAL" }, { 0xD0, "MESTM0" }, { 0xD3, "HSTMER" }, { 0xD4, "ENSIDE" }, { 0xD5, "POLYL" },
    { 0xD7, "RESULT" }, { 0xDB, "POTMIN" }, { 0xDD, "POTVAL" }, { 0xDF, "PTRNGE" }, { 0xE1, "POTUSE" },
    { 0xE3, "P.CNTR" }, { 0xE4, "POTFLG" }, { 0xE5, "PLYPRP" }, { 0xE6, "PROP.F" }, { 0xE7, "P.LTCH" },
    { 0xE8, "SWSTAT" }, { 0xE9, "FR.DB" }, { 0xEA, "LANG" }, { 0xEB, "$CMODE" }, { 0xEC, "$CCTIM" },
    { 0xEF, "$PSTSL" }, { 0xF2, "$LMTIM" }, { 0xF3, "$CNCT" }, { 0xF4, "$$CRDT" }, { 0xF5, "$CNSTT" },
    { 0xF8, "EASRCE" }, { 0xFA, "EAX" }, { 0xFB, "EABC" }, { 0xFC, "EAFLG" }, { 0xFD, "EACS" },
    { 0xFE, "ATGVAL" }, { 0x100, "GRSHLS" }, { 0x146, "EAFILL" }, { 0x148, "PRCTIM" }, { 0x14D, "GCOUNT" },
    { 0x155, "HSCRES" }, { 0x178, "PKYCNT" }, { 0x17A, "STATUS" }, { 0x17C, "SLAMSN" }, { 0x180, "stack" },
    { 0x200, "DB.TRP" }, { 0x300, "PFOBJ" }, { 0x32C, "SHELLS" }, { 0x3B4, "POINT" }, { 0x3BC, "CURRENT" },
    { 0x3C4, "FRAMES" }, { 0x3CC, "COUNT" }, { 0x3D4, "ATGDEL" }, { 0x3D6, "PKYRAN" }, { 0x3DA, "EACNT" },
    { 0x3DB, "EABAD" }, { 0x3DC, "BULPOS" }, { 0x3E0, "PLYRDL" }, { 0x3E4, "PFSNDF" }, { 0x3E5, "PRPDEL" },
    { 0x3E9, "(free)" },
};
#define NCELLS ((int)(sizeof CELLS / sizeof CELLS[0]))

static const char *sym_at(uint16_t a, char *buf, int size)
{
    int best = -1;
    for (int i = 0; i < NCELLS && CELLS[i].addr <= a; i++) best = i;
    if (a >= 0x2000) snprintf(buf, (size_t)size, "VG RAM");
    else if (best < 0) snprintf(buf, (size_t)size, "?");
    else if (CELLS[best].addr == a) snprintf(buf, (size_t)size, "%s", CELLS[best].name);
    else snprintf(buf, (size_t)size, "%s+%X", CELLS[best].name, (unsigned)(a - CELLS[best].addr));
    return buf;
}

/* --- what the original read ---------------------------------------------------- */

typedef struct feed { const uint8_t *v; uint32_t n, used, over; } feed;

static uint8_t feed_rnd(void *u)
{
    feed *fd = (feed *)u;
    if (fd->used < fd->n) return fd->v[fd->used++];
    fd->over++;
    return 0;
}

typedef struct rdfeed { const uint8_t *r; uint32_t n, used, passed, skipped; } rdfeed;

static int g_trace;

/* A read of an address a recording may hold: the value the original read
 * at this point of the frame, if the record has this read next (a few
 * records are looked ahead: the 6502 makes reads the C does not, when an
 * index crosses a page). A read the record does not have is one of memory
 * the interrupt had not touched: the machine's own value stands. */
static uint8_t feed_read(void *u, uint16_t a, uint8_t cur)
{
    rdfeed *fd = (rdfeed *)u;
    for (uint32_t k = 0; k < 6 && fd->used + k < fd->n; k++) {
        const uint8_t *rec = fd->r + 3 * (fd->used + k);
        if ((uint16_t)(rec[0] | (rec[1] << 8)) == a) {
            if (g_trace) printf("    read %04X = %02X (record %u of %u)\n", a, rec[2], fd->used + k, fd->n);
            fd->skipped += k;
            fd->used += k + 1;
            return rec[2];
        }
    }
    fd->passed++;
    return cur;
}

/* --- one frame ------------------------------------------------------------------ */

static void load(rb_machine *m, const rbcap_reader *cap, const rbcap_frame *s)
{
    rb_machine_init(m);
    rb_machine_set_rom(m, cap->rom);
    memcpy(m->mem, s->ram, RBCAP_RAM_SIZE);
    memcpy(m->mem + 0x2000, s->vgram, RBCAP_VGRAM_SIZE);
    memcpy(m->mb.reg, s->mb_reg, sizeof m->mb.reg);
    m->mb.result = s->mb_result;
    m->mem[RB_CALFLG] >>= 1;        /* the LSR the record was taken in */
}

static const char *arg(int argc, char **argv, const char *name)
{
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], name)) return argv[i + 1];
    return NULL;
}

static int flag(int argc, char **argv, const char *name)
{
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], name)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--emu") || !strcmp(argv[i], "--quiet") || !strcmp(argv[i], "--leftover")) continue;
        if (argv[i][0] == '-') { i++; continue; }
        path = argv[i];
    }
    if (!path) {
        fprintf(stderr, "usage: rbcompare CAPTURE.rbcap [--emu] [--show N] [--max N] [--from F] [--to F] [--trace F]\n");
        return 2;
    }
    int use_emu = flag(argc, argv, "--emu");
    int quiet = flag(argc, argv, "--quiet");
    int show = atoi(arg(argc, argv, "--show") ? arg(argc, argv, "--show") : "12");
    int max_report = atoi(arg(argc, argv, "--max") ? arg(argc, argv, "--max") : "10");
    uint32_t from = (uint32_t)atoi(arg(argc, argv, "--from") ? arg(argc, argv, "--from") : "0");
    uint32_t to = (uint32_t)strtoul(arg(argc, argv, "--to") ? arg(argc, argv, "--to") : "4000000000", NULL, 10);
    (void)load_syms;

    rbcap_reader cap;
    if (rbcap_open(&cap, path) != 0) {
        fprintf(stderr, "rbcompare: cannot read %s\n", path);
        return 2;
    }

    static rb_machine m;
    static rb_emu emu;
    static rbcap_frame prev, next;
    static uint8_t payload[0x40000];
    static uint8_t rnd[0x10000], irds[0x30000];
    static uint8_t skip[0x3000];
    uint32_t nrnd = 0, nirds = 0, len;
    int have_prev = 0, poked = 0;
    char tag[4];
    uint32_t compared = 0, differing = 0, reported = 0, stuck = 0;
    uint32_t rnd_wrong = 0, left_over = 0;
    uint64_t cycles_sum = 0, bytes_compared = 0;
    uint32_t cycles_max = 0;
    double t_first = -1.0, t_last = 0.0;
    int rc;

    memset(skip, 0, sizeof skip);
    while ((rc = rbcap_next_chunk(&cap, tag, payload, sizeof payload, &len)) == 1) {
        if (!memcmp(tag, "RNDM", 4)) {
            /* (the chunk was read into payload: only as much as fits there) */
            nrnd = len < sizeof payload ? len : (uint32_t)sizeof payload;
            if (nrnd > sizeof rnd) nrnd = sizeof rnd;
            memcpy(rnd, payload, nrnd);
            continue;
        }
        if (!memcmp(tag, "IRDS", 4)) {
            nirds = len < sizeof payload ? len : (uint32_t)sizeof payload;
            if (nirds > sizeof irds) nirds = sizeof irds;
            memcpy(irds, payload, nirds);
            nirds /= 3;
            continue;
        }
        if (!memcmp(tag, "NWRT", 4)) {
            uint32_t n = (len < sizeof payload ? len : (uint32_t)sizeof payload) / 2;
            for (uint32_t i = 0; i < n; i++) {
                unsigned a = payload[2 * i] | (payload[2 * i + 1] << 8);
                if (a < sizeof skip) skip[a] = 1;
            }
            continue;
        }
        if (!memcmp(tag, "TIME", 4) && len == 8) {
            double t;
            memcpy(&t, payload, 8);
            if (t_first < 0.0) t_first = t;
            t_last = t;
            continue;
        }
        if (!memcmp(tag, "POKE", 4)) { poked = 1; continue; }
        if (memcmp(tag, "FRAM", 4) != 0 || len != RBCAP_FRAM_SIZE) continue;

        rbcap_parse_frame(payload, &next);
        if (have_prev && !poked && prev.frame >= from && prev.frame <= to && cap.have_rom) {
            load(&m, &cap, &prev);
            feed fr = { rnd, nrnd, 0, 0 };
            rdfeed fd = { irds, nirds, 0, 0, 0 };
            m.prng = feed_rnd;
            m.prng_user = &fr;
            m.read_hook = feed_read;
            m.read_user = &fd;
            rbcap_watch_set(m.watch);

            g_trace = arg(argc, argv, "--trace") && (uint32_t)atoi(arg(argc, argv, "--trace")) == prev.frame;
            if (g_trace) {
                printf("frame %u, the original read:", prev.frame);
                for (uint32_t i = 0; i < nirds; i++)
                    printf(" %04X=%02X", irds[3 * i] | (irds[3 * i + 1] << 8), irds[3 * i + 2]);
                printf("\n");
            }
            int cy = 0;
            if (use_emu) {
                rb_emu_init(&emu, &m);
                cy = rb_emu_frame(&emu, 3000000);
            } else {
                rb_frame(&m);
            }
            if (cy < 0 || m.hung || m.not_ported) {
                stuck++;
                if (!quiet && reported++ < (uint32_t)max_report)
                    printf("frame %u: the program did not come back to INTWAIT (%s)\n", prev.frame,
                           m.not_ported ? m.not_ported : m.hung ? "hung" : "ran on");
            } else {
                compared++;
                cycles_sum += (uint32_t)cy;
                if ((uint32_t)cy > cycles_max) cycles_max = (uint32_t)cy;
                int nd = 0;
                for (unsigned a = 0; a < 0x400; a++) {
                    if (skip[a] || (a >= 0x180 && a < 0x200)) continue;
                    bytes_compared++;
                    if (m.mem[a] != next.ram[a]) nd++;
                }
                for (unsigned a = 0; a < 0x1000; a++) {
                    if (skip[0x2000 + a]) continue;
                    bytes_compared++;
                    if (m.mem[0x2000 + a] != next.vgram[a]) nd++;
                }
                int mbd = 0;
                for (int i = 0; i < 16; i++) mbd += m.mb.reg[i] != next.mb_reg[i];
                mbd += m.mb.result != next.mb_result;
                int rnd_bad = fr.over || fr.used != fr.n;
                if (rnd_bad) rnd_wrong++;
                if (fd.used != fd.n) {
                    left_over++;
                    if (flag(argc, argv, "--leftover") && left_over <= (uint32_t)max_report) {
                        printf("frame %u: recorded reads %u of %u used:", prev.frame, fd.used, fd.n);
                        for (uint32_t i = fd.used; i < fd.n && i < fd.used + 10; i++)
                            printf(" %04X=%02X", irds[3 * i] | (irds[3 * i + 1] << 8), irds[3 * i + 2]);
                        printf("\n");
                    }
                }
                if (nd || mbd || rnd_bad) {
                    differing++;
                    if (!quiet && reported++ < (uint32_t)max_report) {
                        printf("frame %u: %d byte(s) differ, math box %d; random numbers %u of %u%s, recorded reads %u of %u (%u passed by)\n",
                               prev.frame, nd, mbd, fr.used, fr.n, fr.over ? " (ran out)" : "", fd.used, fd.n, fd.skipped);
                        int shown = 0;
                        char nm[64];
                        for (unsigned a = 0; a < 0x400 && shown < show; a++) {
                            if (skip[a] || (a >= 0x180 && a < 0x200) || m.mem[a] == next.ram[a]) continue;
                            printf("    %04X %-12s here %02X  MAME %02X  (was %02X)\n", a, sym_at((uint16_t)a, nm, sizeof nm),
                                   m.mem[a], next.ram[a], prev.ram[a]);
                            shown++;
                        }
                        for (unsigned a = 0; a < 0x1000 && shown < show; a++) {
                            if (skip[0x2000 + a] || m.mem[0x2000 + a] == next.vgram[a]) continue;
                            printf("    %04X %-12s here %02X  MAME %02X  (was %02X)\n", 0x2000 + a, "VG RAM",
                                   m.mem[0x2000 + a], next.vgram[a], prev.vgram[a]);
                            shown++;
                        }
                        for (int i = 0; i < 16 && mbd; i++)
                            if (m.mb.reg[i] != next.mb_reg[i])
                                printf("    math box R%X here %04X  MAME %04X\n", i, (uint16_t)m.mb.reg[i], (uint16_t)next.mb_reg[i]);
                        if (mbd && m.mb.result != next.mb_result)
                            printf("    math box result here %04X  MAME %04X\n", (uint16_t)m.mb.result, (uint16_t)next.mb_result);
                    }
                }
            }
        }
        prev = next;
        have_prev = 1;
        poked = 0;
        nrnd = nirds = 0;
        memset(skip, 0, sizeof skip);
    }
    rbcap_close(&cap);
    if (rc < 0) fprintf(stderr, "rbcompare: %s is damaged at its end\n", path);

    printf("%s, %s: %u frames compared, %u differ", path, use_emu ? "the original on the 6502 core" : "the C routines",
           compared, differing);
    if (stuck) printf(", %u stuck", stuck);
    if (rnd_wrong) printf("; random numbers off in %u", rnd_wrong);
    if (left_over) printf("; recorded reads left over in %u", left_over);
    printf("\n");
    if (compared) printf("  %.0f bytes compared a frame\n", (double)bytes_compared / compared);
    if (use_emu && compared)
        printf("  6502 cycles a frame: average %u, most %u (an interrupt period is %d; 24 of them make a frame)\n",
               (unsigned)(cycles_sum / compared), cycles_max, RB_CPU_CYCLES_PER_NMI);
    if (t_last > t_first && compared)
        printf("  MAME: %.2f frames a second over %.1f s\n", (double)compared / (t_last - t_first), t_last - t_first);
    return differing || stuck ? 1 : 0;
}
