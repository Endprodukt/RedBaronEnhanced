/* rbshot - run the game without a screen: its picture as SVG, its sound as WAV.
 *
 *   rbshot [--roms PATH] [--seconds S] [--original] [--play "SCRIPT"]
 *          [--svg FILE --at T] [--wav FILE] [--opt1 HEX] [--opt0 HEX] [--smooth] [--calibrate]
 *
 * The game runs as in the program itself (the interrupt 246 times a
 * second, the main line after it). The script is a list of "T WHAT [ARGS]"
 * separated by ';', T in seconds from power on:
 *   coin | start         the switch, closed for a quarter of a second
 *   fire on | fire off
 *   stick X Y            each -1..1 (right and down positive)
 * At the end it prints what the game is at (mode, score, lives, frames).
 *
 * The SVG is drawn as the program shows it: in colour (--classic: the
 * cabinet's one), with occlusion (--nosolid: without).
 *
 * --calibrate: the stick's calibration set at power on (rb_stick_calibrate).
 *
 * --samples DIR: the sounds as recordings (sound/samples.h) from DIR
 * (package/sounds); it counts what of them began.
 *
 * --screens: the port's title, table and entry (render/screens.h) on their
 * pages, --page-t S as if the page had shown S seconds; --free: free play's
 * texts. --cockpit: the bezel's cockpit (render/cockpit.h).
 *
 * --smooth: the pictures between the frames, computed from the game's
 * state (render/subframe.h), as the program shows them; the picture of
 * --at is one of them. And it checks them: at each frame the computed
 * picture for t = 0 must be the frame's display list word for word, and
 * the one for t = 1 is held against the next frame's list.
 */
#include "avg/avg.h"
#include "core/emu.h"
#include "core/machine.h"
#include "game/game.h"
#include "capture/capture.h"
#include "render/cockpit.h"
#include "render/color.h"
#include "render/deco.h"
#include "render/fx.h"
#include "render/glyph.h"
#include "render/screens.h"
#include "render/solid.h"
#include "render/subframe.h"
#include "rom/romset.h"
#include "sound/sound.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RB_HAVE_VORBIS
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

/* --samples DIR: the recordings (sound/samples.h) from DIR/<name>.ogg, as
 * the program loads them (platform/audio.c). Returns how many are missing. */
static int load_samples(rb_samples *smp, const char *dir)
{
    int missing = 0;
    for (int i = 0; i < RB_SMP_COUNT; i++) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%s.ogg", dir, rb_sample_names[i]);
        int channels = 0, rate = 0;
        short *pcm = NULL;
        int frames = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
        if (frames <= 0 || !pcm || channels < 1) { fprintf(stderr, "rbshot: no %s\n", path); free(pcm); missing++; continue; }
        int16_t *mono = (int16_t *)malloc((size_t)frames * sizeof *mono);
        for (int f = 0; f < frames; f++) {
            int sum = 0;
            for (int c = 0; c < channels; c++) sum += pcm[f * channels + c];
            mono[f] = (int16_t)(sum / channels);
        }
        free(pcm);
        smp->clip[i].pcm = mono;
        smp->clip[i].frames = frames;
        smp->clip[i].rate = rate;
    }
    smp->loaded = missing == 0;
    return missing;
}
#endif

typedef struct event { double t; char what[16]; char a[16], b[16]; } event;

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

static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void put32(FILE *f, unsigned v) { put16(f, v & 0xffff); put16(f, v >> 16); }

/* The picture as the program shows it: whole lines with their tags,
 * occlusion (unless nosolid), the window; in colour (unless classic). */
static int g_nodeco;
static double g_t;
static int g_screens, g_free, g_cockpit, g_fx;
static double g_fx_age = 0.2;
static double g_page_t = -1.0;

static void write_svg(const char *path, const rb_machine *pic0, uint16_t start, int classic, int nosolid)
{
    static rb_avg_list l;
    static rb_solid solid;
    static rb_machine pic;
    pic = *pic0;
    pic.sound_write = NULL;
    pic.view_hook = NULL;
    if (start == RB_VGRAM) start = (pic.mem[RB_BUFFER] & 0x80) ? 0x2002 : 0x2802;   /* (the buffer on the screen) */
    if (!g_nodeco) printf("rbshot: %d trees and clouds\n", rb_deco_draw(&pic, start, g_t, !getenv("RB_NOTREES"), !getenv("RB_NOCLOUDS")));
    int avg_st = rb_avg_run_inst(pic.mem, start, pic.vgtag, pic.vgnote, pic.vginst, &l, 0);
    if (avg_st) printf("rbshot: the display list did not end (%d instructions)\n", l.instructions);
    const rb_line *lines = l.lines;
    int n = l.count;
    int n_all = l.count;
    if (g_cockpit && rb_screens_page(pic.mem) == RB_PAGE_GAME) l.count = n_all = rb_cockpit_drop_own_prop(l.lines, n_all);
    if (!classic && l.cap > n_all) l.count = n_all += rb_fx_roundels(l.lines, n_all, l.lines + n_all, l.cap - n_all);
    if (g_fx && l.cap > n_all) {
        static rb_fx fx;
        rb_fx_init(&fx);
        /* (one picture: as if the explosion began g_fx_age seconds ago) */
        rb_fx_lines(&fx, l.lines, n_all, g_t - g_fx_age, l.lines + n_all, 0);
        l.count = n_all += rb_fx_lines(&fx, l.lines, n_all, g_t, l.lines + n_all, l.cap - n_all);
    }
    n = l.count;
    lines = l.lines;
    /* the cockpit: its lines, and its shapes in front of the world */
    static rb_cockpit ck;
    static rb_line ckl[1024];
    static rb_solid_poly front[RB_COCKPIT_HULLS];
    int nck = 0;
    solid.front = NULL;
    solid.n_front = 0;
    if (g_cockpit && rb_screens_page(pic.mem) == RB_PAGE_GAME) {
        rb_cockpit_init(&ck);
        nck = rb_cockpit_lines(&ck, pic.mem, g_t, ckl, 1024);
        solid.front = front;
        solid.n_front = rb_cockpit_polys(&ck, front, RB_COCKPIT_HULLS);
    }
    solid.no_faces = nosolid;
    if (!nosolid || solid.n_front) n = rb_solid_run(&solid, pic.mem, l.lines, l.count, &lines);
    if (g_screens || g_free) {
        static rb_line scr[8192];
        static rb_screens sc;
        rb_screens_init(&sc);
        rb_glyph_init(pic.mem);
        sc.elapsed = g_page_t;
        rb_screens_opt o = { g_screens, g_free, 0, g_cockpit };
        n = rb_screens_apply(&sc, pic.mem, g_t, &o, lines, n, scr, 8192);
        lines = scr;
    }
    if (nck) {
        static rb_line all[8192];
        memcpy(all, lines, (size_t)n * sizeof *all);
        memcpy(all + n, ckl, (size_t)nck * sizeof *all);
        n += nck;
        lines = all;
    }
    {
        static rb_line fin[8192];
        memcpy(fin, lines, (size_t)n * sizeof *fin);
        rb_color_mark(fin, n);
        lines = fin;
    }
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "rbshot: cannot write %s\n", path); return; }
    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-260 -200 520 400\" width=\"1040\" height=\"800\">\n"
               "<rect x=\"-260\" y=\"-200\" width=\"520\" height=\"400\" fill=\"black\"/>\n");
    static const uint8_t cabinet[3] = { 110, 190, 255 };
    int shown = 0;
    for (int i = 0; i < n; i++) {
        rb_line q = lines[i];
        if (!rb_line_clip(&q)) continue;
        uint8_t rgb[3];
        if (classic) memcpy(rgb, cabinet, 3);
        else rb_color_of(&q, cabinet, rgb);
        fprintf(f, "<line data-tag=\"%u\" data-src=\"%04X\" data-sub=\"%04X\" data-model=\"%u\" data-p=\"%u-%u\" x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"rgb(%u,%u,%u)\" stroke-opacity=\"%.2f\" "
                   "stroke-width=\"0.8\" stroke-linecap=\"round\"/>\n",
                q.tag, q.src, q.sub, q.model, q.p0, q.p1, q.x0, -q.y0, q.x1, -q.y1, rgb[0], rgb[1], rgb[2], 0.25 + 0.75 * q.intensity / 15.0);
        shown++;
    }
    /* (RB_FACES=1: the covering shapes too, faint) */
    if (getenv("RB_FACES") && !nosolid) {
        for (int k = 0; k < solid.n_polys; k++) {
            const rb_solid_poly *p = &solid.polys[k];
            fprintf(f, "<polygon fill=\"rgb(255,0,255)\" fill-opacity=\"0.18\" stroke=\"none\" points=\"");
            for (int i = 0; i < p->n; i++) fprintf(f, "%.1f,%.1f ", p->x[i], -p->y[i]);
            fprintf(f, "\"/>\n");
        }
        printf("rbshot: %d covering shapes\n", solid.n_polys);
    }
    fprintf(f, "</svg>\n");
    fclose(f);
    printf("rbshot: %d lines drawn, %d cut by occlusion\n", shown, nosolid ? 0 : solid.cut);
}

int main(int argc, char **argv)
{
    static rb_romset roms;
    const char *paths[16];
    int np = 0;
    if (arg(argc, argv, "--roms")) np = rb_romset_add_dir(paths, np, 16, arg(argc, argv, "--roms"));
    np += rb_romset_default_paths(paths + np, 16 - np);
    if (rb_romset_load(&roms, paths, np) != 0) {
        char missing[512];
        fprintf(stderr, "rbshot: ROMs missing: %s\n", rb_romset_missing(&roms, missing, sizeof missing));
        return 2;
    }
    double seconds = atof(arg(argc, argv, "--seconds") ? arg(argc, argv, "--seconds") : "20");
    double at = atof(arg(argc, argv, "--at") ? arg(argc, argv, "--at") : "-1");
    const char *svg = arg(argc, argv, "--svg"), *wav = arg(argc, argv, "--wav");
    int original = flag(argc, argv, "--original");

    static event ev[256];
    int nev = 0;
    if (arg(argc, argv, "--play")) {
        static char copy[8192];
        snprintf(copy, sizeof copy, "%s", arg(argc, argv, "--play"));
        for (char *p = strtok(copy, ";"); p && nev < 256; p = strtok(NULL, ";")) {
            event *e = &ev[nev];
            memset(e, 0, sizeof *e);
            if (sscanf(p, "%lf %15s %15s %15s", &e->t, e->what, e->a, e->b) >= 2) nev++;
        }
    }

    static rb_machine m;
    static rb_emu emu;
    static rb_sound snd;
    static rb_avg_list list;
    rb_machine_init(&m);
    rb_machine_set_rom(&m, roms.rom);
    if (arg(argc, argv, "--opt0")) m.in.opt0 = (uint8_t)strtoul(arg(argc, argv, "--opt0"), NULL, 16);
    if (arg(argc, argv, "--opt1")) m.in.opt1 = (uint8_t)strtoul(arg(argc, argv, "--opt1"), NULL, 16);
    rb_sound_init(&snd);
    m.sound_write = rb_sound_write;
    m.sound_user = &snd;
    if (arg(argc, argv, "--samples")) {
#ifdef RB_HAVE_VORBIS
        if (load_samples(&snd.smp, arg(argc, argv, "--samples")) != 0) return 2;
        rb_sound_use_samples(&snd, 1);
#else
        fprintf(stderr, "rbshot: built without the Ogg Vorbis decoder (the SDK): no --samples\n");
        return 2;
#endif
    }
    int smooth = flag(argc, argv, "--smooth") && !original;
    int classic = flag(argc, argv, "--classic"), nosolid = flag(argc, argv, "--nosolid") || original;
    g_nodeco = flag(argc, argv, "--nodeco") || original;
    g_screens = flag(argc, argv, "--screens") && !original;
    g_free = flag(argc, argv, "--free") && !original;
    g_cockpit = flag(argc, argv, "--cockpit") && !original;
    g_fx = flag(argc, argv, "--fx") && !original;
    if (arg(argc, argv, "--fx-age")) g_fx_age = atof(arg(argc, argv, "--fx-age"));
    if (arg(argc, argv, "--page-t")) g_page_t = atof(arg(argc, argv, "--page-t"));
    /* --capture FILE --frame N: the picture of frame N of a MAME recording
     * (its memory, one frame run by the C routines so that every line says
     * what it is); --frames lists those of the ground sequences with
     * objects on the ground. */
    if (arg(argc, argv, "--capture")) {
        rbcap_reader cap;
        if (rbcap_open(&cap, arg(argc, argv, "--capture")) != 0) return 2;
        static uint8_t payload[0x40000];
        static rbcap_frame fr;
        char tag[4];
        uint32_t len, want = (uint32_t)atoi(arg(argc, argv, "--frame") ? arg(argc, argv, "--frame") : "0");
        int list_them = flag(argc, argv, "--frames"), found = 0;
        while (rbcap_next_chunk(&cap, tag, payload, sizeof payload, &len) == 1) {
            if (memcmp(tag, "FRAM", 4) != 0 || len != RBCAP_FRAM_SIZE) continue;
            rbcap_parse_frame(payload, &fr);
            if (list_them) {
                int objs = 0;
                for (int k = 0; k < 4; k++)
                    for (int j = 8; j < 11; j++) objs += (fr.ram[RB_PFOBJ + k * RB_L_PFOB + j] & 0x80) != 0;
                if ((fr.ram[RB_GRMODE] & 0x80) && objs) printf("frame %u: ground sequence, %d objects\n", fr.frame, objs);
                if (fr.ram[RB_PLSTAT + 6] & 0x40) printf("frame %u: the plane blows up\n", fr.frame);
                continue;
            }
            if (fr.frame != want) continue;
            memcpy(m.mem, fr.ram, RBCAP_RAM_SIZE);
            memcpy(m.mem + 0x2000, fr.vgram, RBCAP_VGRAM_SIZE);
            m.mem[RB_CALFLG] >>= 1;
            m.sound_write = NULL;
            rb_frame(&m);
            found = 1;
            break;
        }
        rbcap_close(&cap);
        if (list_them) return 0;
        if (!found || !svg) { fprintf(stderr, "rbshot: no frame %u\n", want); return 2; }
        g_t = (double)want / 10.25;
        uint16_t st = (m.mem[RB_BUFFER] & 0x80) ? 0x2802 : 0x2002;     /* (the list just drawn) */
        write_svg(svg, &m, st, classic, nosolid);
        return 0;
    }
    static rb_subframe sub;
    static rb_machine scratch;
    static uint8_t ahead[0x800];
    uint16_t ahead_start = 0;
    uint32_t checked = 0, same0 = 0, same1 = 0, cut = 0;
    double words1 = 0.0, diff1 = 0.0;
    double t_frame = 0.0, period = 24.0 / RB_NMI_HZ;
    if (smooth) {
        m.view_hook = rb_subframe_hook;
        m.view_user = &sub;
    }
    if (original) rb_emu_init(&emu, &m);
    else rb_power_on(&m);
    if (flag(argc, argv, "--calibrate") && !original) rb_stick_calibrate(&m);

    FILE *wf = NULL;
    uint32_t nsamples = 0;
    if (wav && (wf = fopen(wav, "wb")) != NULL)
        for (int i = 0; i < 44; i++) fputc(0, wf);
    double peak = 0.0, energy = 0.0;

    double coin_until = -1, start_until = -1;
    int fire = 0, shot_done = 0;
    uint32_t pictures = 0, lines_sum = 0, lines_max = 0;
    long nmis = (long)(seconds * RB_NMI_HZ);
    for (long i = 0; i < nmis; i++) {
        double t = (double)i / RB_NMI_HZ;
        for (int k = 0; k < nev; k++) {
            event *e = &ev[k];
            if (e->t < 0 || e->t > t) continue;
            if (!strcmp(e->what, "coin")) coin_until = t + 0.25;
            else if (!strcmp(e->what, "start")) start_until = t + 0.25;
            else if (!strcmp(e->what, "fire")) fire = !strcmp(e->a, "on");
            else if (!strcmp(e->what, "stick")) {
                m.in.pot_x = (uint8_t)(128 + 64 * atof(e->a));
                m.in.pot_y = (uint8_t)(128 + 64 * atof(e->b));
            }
            e->t = -1;
        }
        m.in.in0 = (uint8_t)(t < coin_until ? 0x3f & ~RB_IN0_COIN_R : 0x3f);
        m.in.in4 = (uint8_t)((t < start_until ? 0 : RB_IN4_START) | (fire ? RB_IN4_FIRE : 0));
        uint32_t starts = m.vg_starts;
        if (original) {
            rb_emu_nmi(&emu);
        } else {
            m.vg_left = 0;
            rb_interrupt(&m);
            if (rb_main_poll(&m) && smooth) {
                period = t - t_frame > 0.01 ? t - t_frame : period;
                t_frame = t;
                if (sub.have == 2) {
                    uint16_t st = (sub.a.mem[RB_BUFFER] & 0x80) ? 0x2802 : 0x2002;
                    uint8_t prev_ahead[0x800];
                    uint16_t prev_start = ahead_start;
                    memcpy(prev_ahead, ahead, sizeof ahead);
                    /* the frame's list: up to its HALT */
                    int len = 0;
                    while (len < 0x3fe && rb_peek16(&m, (uint16_t)(st + 2 * len)) != 0x2000) len++;
                    len++;
                    uint16_t s0 = rb_subframe_render(&sub, &m, 0.0f, &scratch);
                    checked++;
                    if (s0 == st && memcmp(scratch.mem + st, m.mem + st, (size_t)len * 2) == 0) same0++;
                    if (prev_start) {
                        int d = 0;
                        for (int w = 0; w < len; w++)
                            d += prev_ahead[2 * w] != m.mem[st + 2 * w] || prev_ahead[2 * w + 1] != m.mem[st + 2 * w + 1];
                        if (d == 0) same1++;
                        words1 += len;
                        diff1 += d;
                    } else {
                        cut++;
                    }
                    /* the picture for t = 1 of the frame now on the screen, to hold against the next */
                    ahead_start = rb_subframe_render(&sub, &m, 1.0f, &scratch);
                    if (ahead_start) memcpy(ahead, scratch.mem + ahead_start, sizeof ahead);
                }
            }
        }
        if (m.hung || m.not_ported) {
            fprintf(stderr, "rbshot: the program stopped at %.2f s (%s)\n", t, m.not_ported ? m.not_ported : "its own check");
            break;
        }
        if (m.vg_starts != starts) {
            rb_avg_run(m.mem, RB_VGRAM, &list, 0);
            pictures++;
            lines_sum += (uint32_t)list.count;
            if ((uint32_t)list.count > lines_max) lines_max = (uint32_t)list.count;
            if (svg && !shot_done && at >= 0 && t >= at) {
                g_t = t;
                float ft = (float)((t - t_frame) / period);
                uint16_t st = smooth ? rb_subframe_render(&sub, &m, ft, &scratch) : 0;
                if (st) rb_avg_run(scratch.mem, st, &list, 0);
                write_svg(svg, st ? &scratch : &m, st ? st : RB_VGRAM, classic, nosolid);
                printf("rbshot: picture at %.2f s%s: %d lines, to %s\n", t,
                       st ? " (between two frames)" : "", list.count, svg);
                if (st) printf("rbshot: that is %.2f of the way from the frame on the screen to the next\n", ft);
                shot_done = 1;
            }
        }
        rb_samples_game(&snd.smp, m.mem);
        rb_sound_run(&snd, RB_CPU_CYCLES_PER_NMI);
        static int16_t buf[4096];
        int got;
        while ((got = rb_sound_read(&snd, buf, 2048)) > 0) {
            for (int k = 0; k < got; k++) {
                double v = buf[2 * k] / 32768.0;
                if (v > peak) peak = v;
                if (-v > peak) peak = -v;
                energy += v * v;
                if (wf) put16(wf, (unsigned)(uint16_t)buf[2 * k]);
            }
            nsamples += (uint32_t)got;
        }
    }
    if (svg && !shot_done) {
        rb_avg_run(m.mem, RB_VGRAM, &list, 0);
        write_svg(svg, &m, RB_VGRAM, classic, nosolid);
        printf("rbshot: the last picture: %d lines, to %s\n", list.count, svg);
    }
    if (wf) {
        fseek(wf, 0, SEEK_SET);
        fwrite("RIFF", 1, 4, wf); put32(wf, 36 + nsamples * 2); fwrite("WAVEfmt ", 1, 8, wf);
        put32(wf, 16); put16(wf, 1); put16(wf, 1); put32(wf, RB_SOUND_RATE); put32(wf, RB_SOUND_RATE * 2);
        put16(wf, 2); put16(wf, 16); fwrite("data", 1, 4, wf); put32(wf, nsamples * 2);
        fclose(wf);
    }
    printf("rbshot: %.1f s, %u game frames (%.2f a second), %u pictures (%.1f a second), %u lines a picture, most %u\n",
           seconds, (unsigned)m.frames, m.frames / seconds, pictures, pictures / seconds,
           pictures ? lines_sum / pictures : 0, lines_max);
    printf("rbshot: GAMODE %02X, score %02X%02X0, lives %u, credits %u, level %u, EAROM %s\n", m.mem[RB_GAMODE],
           m.mem[RB_SCORE + 1], m.mem[RB_SCORE], m.mem[RB_LIVES], m.mem[RB_CRDT], m.mem[RB_GMLEVL],
           m.earom.dirty ? "written" : "untouched");
    if (smooth)
        printf("rbshot: pictures between the frames: %u frames checked; t = 0 the frame's own list in %u; "
               "t = 1 the next frame's list in %u, %.1f %% of its words differ (%u cuts, %u in the module)\n",
               checked, same0, same1, words1 > 0 ? 100.0 * diff1 / words1 : 0.0, cut, sub.cuts);
    printf("rbshot: sound: %u samples, peak %.3f, rms %.4f\n", nsamples, peak,
           nsamples ? sqrt(energy / nsamples) : 0.0);
    if (snd.use_samples)
        printf("rbshot: recordings begun: engine %u, plane closing %u, shots %u (the enemy's %u), explosions %u, "
               "spirals %u\n", snd.smp.n_engine, snd.smp.n_closing, snd.smp.n_shots, snd.smp.n_enemy_shots,
               snd.smp.n_explosions, snd.smp.n_spirals);
    rb_avg_list_free(&list);
    return 0;
}
