/* redbaron - Red Baron (Atari, 1981), its program in C.
 *
 * Finds and checks the ROM set (the pictures, the landscape and the tunes
 * are read from it), then runs the game: the interrupt 246 times a second
 * and the main line after it, as the C routines of src/game, and shows
 * what they write into the display list.
 *
 *   redbaron [--roms PATH] [--config INI] [--original] [--nosound] [--hz N]
 *            [--dvg [PORT] | --window | --windowed]
 *
 *   --original  the original program on the 6502 core, to compare with
 *
 * Controls: every one freely bound, the stick digital or analog
 * (platform/controls.h); the test menu on the service key (F2, or what
 * advmame.rc binds to "service") where all this and
 * the picture, the sound and the game's options are chosen
 * (platform/menu.h), kept in save/redbaron.ini. The high scores and game
 * times the cabinet keeps in its EAROM are kept in save/earom.bin.
 */
#include "avg/avg.h"
#include "core/emu.h"
#include "core/machine.h"
#include "core/tags.h"
#include "game/game.h"
#include "platform/app.h"
#include "platform/audio.h"
#include "platform/controls.h"
#include "platform/menu.h"
#include "render/cockpit.h"
#include "render/config.h"
#include "render/deco.h"
#include "render/demo.h"
#include "render/draw.h"
#include "render/fx.h"
#include "render/color.h"
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

#define MAX_PATHS 48

/* More than this much real time behind (a stall, a debugger) is dropped
 * rather than caught up with. */
#define MAX_CATCHUP_S 0.25

static int rom_paths(int argc, char **argv, const rb_config *cfg, const char **paths)
{
    int n = 0;
    const char *arg = rb_arg(argc, argv, "--roms");
    if (arg) n = rb_romset_add_dir(paths, n, MAX_PATHS, arg);
    if (cfg->rom_path[0]) n = rb_romset_add_dir(paths, n, MAX_PATHS, cfg->rom_path);

    /* The cabinet's own ROM folders: AdvanceMAME's dir_rom. */
    char rcpath[1024];
    vp_rc *rc = NULL;
    if (vp_rc_default_path(rcpath, sizeof rcpath) == VP_OK && vp_rc_load(rcpath, &rc) == VP_OK) {
        const char *dirs = vp_rc_get(rc, "dir_rom");
        if (dirs) {
            static char copy[2048];
            snprintf(copy, sizeof copy, "%s", dirs);
            for (char *p = strtok(copy, ":;"); p && n < MAX_PATHS; p = strtok(NULL, ":;"))
                n = rb_romset_add_dir(paths, n, MAX_PATHS, p);
        }
        vp_rc_free(rc);
    }
    n += rb_romset_default_paths(paths + n, MAX_PATHS - n);
    return n;
}

/* --- the menu's sound test ---------------------------------------------------- */

/* While the menu is open the game stands still and its sounds are silent;
 * the test drives the board's sound latch and POKEY itself for a moment.
 * test: RB_TEST_xxx, t: seconds since it began. Returns 0 once it is over. */
static int sound_test(rb_sound *s, int test, double t)
{
    uint8_t latch = 0x08;                   /* (D3: the start LED, off) */
    int on = 1;
    s->smp.crash_near = 1.0f;               /* (the recordings: as if near) */
    switch (test) {
    case RB_TEST_GUN:                       /* the shot, again and again */
        on = t < 1.2;
        if (on && fmod(t, 0.08) < 0.04) latch |= 0x04;
        break;
    case RB_TEST_EXPLOSION:                 /* the crash, at a falling volume */
        on = t < 1.5;
        if (on) latch |= (uint8_t)((int)(15.99 * (1.0 - t / 1.5)) << 4);
        break;
    case RB_TEST_DIVE:                      /* the spiral's squeal */
        on = t < 2.0;
        if (on) latch |= 0x02;
        break;
    case RB_TEST_ENGINE:                    /* the hum on channels 3 and 4, as SOUNDS keeps it */
    case RB_TEST_CLOSING: {                 /* and a plane closing: its tone rising (AUDF3 F8 to 30) */
        on = t < 3.0;
        uint8_t f = 0xf8;
        if (test == RB_TEST_CLOSING && t > 0.5) f = (uint8_t)(0xf8 - (0xf8 - 0x30) * (t - 0.5 > 2.4 ? 1.0 : (t - 0.5) / 2.4));
        rb_sound_write(s, RB_POKEY + 0x08, 0x00);
        rb_sound_write(s, RB_POKEY + 0x0f, 0x03);
        rb_sound_write(s, RB_POKEY + 0x05, on ? 0xa1 : 0x00);
        rb_sound_write(s, RB_POKEY + 0x07, on ? 0xa1 : 0x00);
        rb_sound_write(s, RB_POKEY + 0x04, f);
        rb_sound_write(s, RB_POKEY + 0x06, (uint8_t)(f == 0xf8 ? 0xf7 : f + (f >> 5) - 8));
        break;
    }
    default:                                /* a pure tone on the POKEY's first channel */
        on = t < 1.0;
        rb_sound_write(s, RB_POKEY + 0x08, 0x00);
        rb_sound_write(s, RB_POKEY + 0x0f, 0x03);
        rb_sound_write(s, RB_POKEY + 0x00, 0x50);
        rb_sound_write(s, RB_POKEY + 0x01, on ? 0xa8 : 0x00);
        break;
    }
    rb_sound_write(s, RB_CRSHSN, latch);
    return on;
}

/* The board's sound silent (the menu opened), or as the game left it (closed). */
static void sound_hold(rb_sound *s, const rb_machine *m, int silent)
{
    rb_sound_write(s, RB_CRSHSN, silent ? (uint8_t)(m->sound_latch & 0x09) : m->sound_latch);
    for (int i = 0; i < 9; i++)
        rb_sound_write(s, RB_POKEY + i, silent && (i & 1) && i < 8 ? 0 : m->pokey[i]);
    if (!silent) rb_sound_write(s, RB_POKEY + 0x0f, m->pokey[0x0f]);
}

/* --- the high scores back to none ------------------------------------------------ */

/* The aces of 1917, for the places of an empty table (default_scores): the
 * score as the game keeps it (BCD, a tenth of the points), the initials. */
static const struct { uint16_t score; char ini[4]; } ACES[7] = {
    { 0x2000, "MVR" },      /* Manfred von Richthofen */
    { 0x1500, "UDT" },      /* Ernst Udet */
    { 0x1200, "LOE" },      /* Erich Loewenhardt */
    { 0x1000, "VOS" },      /* Werner Voss */
    { 0x0800, "BOE" },      /* Oswald Boelcke */
    { 0x0600, "IMM" },      /* Max Immelmann */
    { 0x0400, "JAC" },      /* Josef Jacobs */
};

/* The empty places of the table filled with the aces below the last
 * score in it (sorted, as the game keeps it). Returns how many. */
static int fill_scores(rb_machine *m)
{
    int filled = 0, j = 0;
    unsigned prev = 0x10000;
    for (int i = 0; i < 7; i++) {
        uint8_t *e = &m->mem[RB_HSCRES + 5 * i];
        unsigned sc = e[0] | (e[1] << 8);
        if (sc) { prev = sc; continue; }
        while (j < 7 && ACES[j].score >= prev) j++;
        if (j == 7) break;
        e[0] = (uint8_t)ACES[j].score;
        e[1] = (uint8_t)(ACES[j].score >> 8);
        for (int c = 0; c < 3; c++) e[2 + c] = (uint8_t)(0x16 + 2 * (ACES[j].ini[c] - 'A'));
        prev = ACES[j].score;
        j++;
        filled++;
    }
    return filled;
}

/* The program's own EAROM routine told to write the first six sets of five
 * bytes (the game times and the best three, as after a new entry:
 * states.c, HSCHK). */
static void write_scores(rb_machine *m)
{
    m->mem[RB_EAX] = 6 * 6 - 1;
    m->mem[RB_EABC] = 6 * 6;
    m->mem[RB_EACNT] = 6 * 6;
    m->mem[RB_EASRCE] = RB_EAFILL & 0xff;
    m->mem[RB_EASRCE + 1] = RB_EAFILL >> 8;
    m->mem[RB_EAFLG] = 0x80;                /* (STEAROM: erase first, then write) */
    m->mem[RB_EABAD] = 0;
}

/* The table cleared (and the aces put in: aces), and written. Returns 0
 * while the EAROM is still busy (try again later). */
static int reset_scores(rb_machine *m, int aces)
{
    if (m->mem[RB_EAFLG]) return 0;
    memset(&m->mem[RB_HSCRES], 0, 7 * 5);
    if (aces) fill_scores(m);
    write_scores(m);
    return 1;
}

/* After power on, once the game has read its EAROM: the aces in the empty
 * places. Returns 0 while that is not yet done. */
static int default_scores(rb_machine *m, double game_t)
{
    if (game_t < 1.5 || m->mem[RB_EAFLG]) return 0;
    if (fill_scores(m)) write_scores(m);
    return 1;
}

/* --- the EAROM's file ----------------------------------------------------------- */

static int earom_load(rb_machine *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    uint8_t buf[64];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (n != sizeof buf) return -1;
    memcpy(m->earom.rom, buf, sizeof buf);
    return 0;
}

static int earom_save(const rb_machine *m, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t n = fwrite(m->earom.rom, 1, sizeof m->earom.rom, f);
    return fclose(f) == 0 && n == sizeof m->earom.rom ? 0 : -1;
}

/* --- the table for the cabinet's menu ------------------------------------------- */

/* The VectorPie menu shows a game's best scores (on the marquee, in its hint
 * bar) from save/highscores.txt (SDK docs/06-hiscores.md; game.ini
 * hiscores = yes). The game keeps its own in the EAROM; its table in memory
 * (HSCRES: seven sets of a BCD score, a tenth of the points, and three
 * letters in the game's codes) is written there in the menu's format too.
 * Returns how many lines, or -1 while the table is not one (an unread
 * EAROM: FFFF). */
static int scores_text(const rb_machine *m, char *out, size_t size)
{
    int lines = 0;
    size_t at = 0;
    out[0] = '\0';
    for (int i = 0; i < 7; i++) {
        const uint8_t *e = &m->mem[RB_HSCRES + 5 * i];
        unsigned bcd = e[0] | (e[1] << 8), points = 0;
        for (int k = 3; k >= 0; k--) {
            unsigned d = (bcd >> (4 * k)) & 15;
            if (d > 9) return -1;
            points = points * 10 + d;
        }
        if (!points) continue;
        char name[4];
        for (int c = 0; c < 3; c++) {
            char ch = rb_glyph_char(e[2 + c]);
            name[c] = (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' ? ch : '.';
        }
        name[3] = '\0';
        int n = snprintf(out + at, size - at, "%s %u\n", name, points * 10);
        if (n < 0 || (size_t)n >= size - at) break;
        at += (size_t)n;
        lines++;
    }
    return lines;
}

/* Written as the SDK does: a new file, then renamed over the old one. */
static int scores_file(const char *text)
{
    const char *path = vp_hiscore_path();
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.new", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;
    int ok = fputs(text, f) >= 0;
    ok = fclose(f) == 0 && ok;
    if (!ok) return -1;
#ifdef _WIN32
    remove(path);                           /* (rename does not replace there) */
#endif
    return rename(tmp, path) == 0 ? 0 : -1;
}

static void draw_text_lines(vp_dvg *d, const char *const *text, int n, int y)
{
    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 160, 160, 160);
    for (int i = 0; i < n; i++, y -= 160)
        vp_text(d, text[i], 2048, (float)y, 100, VP_TEXT_CENTER);
}

int main(int argc, char **argv)
{
    static rb_config cfg;
    rb_config_defaults(&cfg);
    char cfg_path[1024], buf[1024];
    const char *arg = rb_arg(argc, argv, "--config");
    if (arg) {
        snprintf(cfg_path, sizeof cfg_path, "%s", arg);
    } else {
        /* Package defaults, then the owner's own in save/. */
        rb_config_load(&cfg, vp_asset_path("redbaron.ini", buf, sizeof buf));
        snprintf(cfg_path, sizeof cfg_path, "%s/redbaron.ini", vp_save_dir());
    }
    rb_config_load(&cfg, cfg_path);

    static rb_romset roms;
    const char *paths[MAX_PATHS];
    int npaths = rom_paths(argc, argv, &cfg, paths);
    int roms_ok = rb_romset_load(&roms, paths, npaths) == 0;
    char missing[512];
    if (roms_ok) printf("redbaron: ROM set complete\n");
    else fprintf(stderr, "redbaron: ROMs missing: %s\n", rb_romset_missing(&roms, missing, sizeof missing));

    rb_app app;
    if (rb_app_open(&app, "Red Baron Enhanced", argc, argv, 60.0) != 0) return 1;

    static rb_machine m;
    static rb_emu emu;
    static rb_sound snd;
    static rb_avg_list list;
    static rb_subframe sub;
    static rb_solid solid;
    static rb_machine scratch;
    static rb_controls ctl;
    static rb_menu menu;
    static rb_screens screens;
    static rb_cockpit cockpit;
    static rb_demo demo;
    static rb_fx fx;
    rb_fx_init(&fx);
    static rb_avg_list demo_list;
    int demo_ready = 0;
    rb_cockpit_init(&cockpit);
    int test_sound = -1, scores_pending = 0, defaults_pending = cfg.default_scores;
    int free_start_was = 0;
    double free_hold = 0.0;                 /* free play: start held for the game this much longer */
    rb_screens_init(&screens);
    double test_t = 0.0;
    menu.capture = -1;
    rb_controls_init(&ctl, &cfg);
    rb_controls_configure(&ctl, &cfg, app.in);
    int original = rb_flag(argc, argv, "--original");
    int have_sound = 0;
    char ea_path[1024];
    snprintf(ea_path, sizeof ea_path, "%s/earom.bin", vp_save_dir());
    double ea_dirty_since = -1.0;

    rb_machine_init(&m);
    m.in.opt0 = rb_config_opt0(&cfg);
    m.in.opt1 = rb_config_opt1(&cfg);
    if (roms_ok) {
        rb_machine_set_rom(&m, roms.rom);
        rb_glyph_init(m.mem);
        if (earom_load(&m, ea_path) == 0) printf("redbaron: high scores from %s\n", ea_path);
        if (!rb_flag(argc, argv, "--nosound")) {
            rb_sound_init(&snd);
            snd.volume = cfg.volume;
            int missing = rb_audio_load_samples(&snd.smp);
            if (!missing) printf("redbaron: recorded sounds loaded\n");
            else fprintf(stderr, "redbaron: %d recorded sounds missing: the board's sounds play\n", missing);
            snd.smp.engine_volume = cfg.engine_volume;
            rb_sound_use_samples(&snd, cfg.samples);
            m.sound_write = rb_sound_write;
            m.sound_user = &snd;
            have_sound = rb_audio_open(&snd) == 0;
        }
        if (original) {
            rb_emu_init(&emu, &m);
        } else {
            /* (always: smooth motion can be switched on in the menu) */
            m.view_hook = rb_subframe_hook;
            m.view_user = &sub;
            rb_power_on(&m);
            if (cfg.stick_calibrate) rb_stick_calibrate(&m);
        }
    }

    const double nmi_period = 1.0 / RB_NMI_HZ;
    double acc = 0.0, dt;
    double game_t = 0.0;                    /* the game's clock: its interrupts so far */
    double t_frame = 0.0, period = 24.0 / RB_NMI_HZ;   /* when the frame on the screen came, and how long one lasts */
    int status = 0;
    while (rb_app_poll(&app, &dt)) {
        rb_controls_read(&ctl, app.in, dt);

        /* The test menu: the service key opens it, and saves and leaves it. */
        int leave = 0;
        if (ctl.menu && roms_ok && !(menu.open && menu.capture >= 0)) {
            if (menu.open) {
                leave = 1;
            } else {
                rb_menu_open(&menu);
                if (have_sound) sound_hold(&snd, &m, 1);
            }
        }
        if (menu.open && !leave) {
            switch (rb_menu_update(&menu, &cfg, &ctl, app.clock)) {
            case RB_MENU_CHANGED:
                rb_controls_configure(&ctl, &cfg, app.in);
                m.in.opt0 = rb_config_opt0(&cfg);
                m.in.opt1 = rb_config_opt1(&cfg);
                demo.m.in.opt0 = m.in.opt0;
                demo.m.in.opt1 = m.in.opt1;
                snd.volume = cfg.volume;
                snd.smp.engine_volume = cfg.engine_volume;
                rb_sound_use_samples(&snd, cfg.samples);
                break;
            case RB_MENU_SOUND:
                test_sound = menu.sound;
                test_t = 0.0;
                break;
            case RB_MENU_RESET_SCORES:
                scores_pending = 1;
                break;
            case RB_MENU_CLOSE:
                leave = 1;
                break;
            }
        }
        if (leave) {
            menu.open = 0;
            menu.capture = -1;
            test_sound = -1;
            if (have_sound) sound_hold(&snd, &m, 0);
            if (rb_config_save(&cfg, cfg_path) != 0) fprintf(stderr, "redbaron: cannot write %s\n", cfg_path);
        }
        app.hold_cancel = menu.open && menu.capture >= 0;      /* (Esc gives a binding up) */
        /* (the EAROM may be busy with the game's own writing: then a little later) */
        if (scores_pending && roms_ok && reset_scores(&m, cfg.default_scores)) scores_pending = 0;
        if (defaults_pending && roms_ok && !original && default_scores(&m, game_t)) defaults_pending = 0;

        /* Free play (the port's): start in the attract mode gives the game
         * the coin it waits for, and is held for it until the game begins. */
        if (cfg.coin_mode == 0 && roms_ok && !menu.open) {
            uint8_t gm = m.mem[RB_GAMODE];
            if (ctl.start && !free_start_was && !(gm & 0xc4) && m.mem[RB_CRDT] == 0 && free_hold <= 0.0) {
                ctl.coin_pulse[0] = RB_COIN_PULSE_NMIS;
                free_hold = 2.0;
            }
        }
        free_start_was = ctl.start;

        /* In the menu the game stands still; its sound test is heard, and
         * the control test shows the pots as they would be. */
        if (menu.open) {
            rb_controls_tick(&ctl, &m);
            if (have_sound) {
                if (test_sound >= 0) {
                    test_t += dt > 0.1 ? 0.1 : dt;
                    if (!sound_test(&snd, test_sound, test_t)) test_sound = -1;
                }
                rb_sound_run(&snd, (int)((dt > 0.1 ? 0.1 : dt) * RB_CPU_CYCLES_PER_NMI * RB_NMI_HZ));
                rb_audio_balance(&snd);
            }
        }

        /* The attract mode with the world: the demo machine plays (render/demo.h) */
        int demo_on = roms_ok && !original && cfg.attract_demo && rb_screens_page(m.mem) == RB_PAGE_GAME &&
                      !(m.mem[RB_GAMODE] & 0xc0);
        if (demo_on && !demo_ready) {
            rb_demo_init(&demo, roms.rom, m.in.opt0, m.in.opt1);
            demo_ready = 1;
        }

        if (roms_ok && !menu.open) {
            acc += dt;
            if (acc > MAX_CATCHUP_S) acc = nmi_period;
            while (acc >= nmi_period) {
                acc -= nmi_period;
                rb_controls_tick(&ctl, &m);
                if (free_hold > 0.0) {
                    m.in.in4 &= (uint8_t)~RB_IN4_START;     /* (low: pressed) */
                    free_hold -= nmi_period;
                    if (m.mem[RB_GAMODE] & 0x80) free_hold = 0.0;
                }
                if (original) {
                    rb_emu_nmi(&emu);
                } else {
                    /* (the vector generator is done long before the next interrupt) */
                    m.vg_left = 0;
                    rb_interrupt(&m);
                    if (rb_main_poll(&m)) {
                        if (game_t - t_frame > 0.01) period = game_t + nmi_period - t_frame;
                        t_frame = game_t + nmi_period;
                    }
                }
                game_t += nmi_period;
                if (demo_on) rb_demo_nmi(&demo);
                if (have_sound) {
                    rb_samples_game(&snd.smp, m.mem);
                    rb_sound_run(&snd, RB_CPU_CYCLES_PER_NMI);
                }
            }
            if (have_sound) rb_audio_balance(&snd);

            /* The EAROM was written (a high score, the game times): keep it,
             * once the program has been done with it for a moment. */
            if (m.earom.dirty) {
                m.earom.dirty = 0;
                ea_dirty_since = app.clock;
            }
            if (ea_dirty_since >= 0.0 && app.clock - ea_dirty_since > 1.0) {
                ea_dirty_since = -1.0;
                if (earom_save(&m, ea_path) != 0) fprintf(stderr, "redbaron: cannot write %s\n", ea_path);
            }
            /* The table for the menu: when it has changed and stayed so for a
             * moment (once the game has read its EAROM and the aces are in). */
            if (game_t > 2.0 && !m.mem[RB_EAFLG] && (m.mem[RB_GAMODE] & 0x0f) != 7) {   /* (not while initials are entered) */
                static char was[600] = "?";
                char now[600];
                if (scores_text(&m, now, sizeof now) >= 0 && strcmp(now, was) != 0) {
                    if (scores_file(now) == 0) printf("redbaron: high scores for the menu in %s\n", vp_hiscore_path());
                    else fprintf(stderr, "redbaron: cannot write %s\n", vp_hiscore_path());
                    snprintf(was, sizeof was, "%s", now);
                }
            }
            if (m.hung || m.not_ported) {
                fprintf(stderr, "redbaron: the program stopped (%s)\n", m.not_ported ? m.not_ported : "its own check");
                status = 1;
                break;
            }
        }

        rb_app_begin_frame(&app);
        if (!roms_ok) {
            const char *text[] = { "RED BARON ENHANCED", "ROMS NOT FOUND", "NEEDED: MAME SET REDBARON", "REDBARON.ZIP",
                                   "IN THE ROM FOLDER OR --ROMS" };
            draw_text_lines(app.dvg, text, 5, 2600);
        } else if (menu.open) {
            rb_menu_draw(&menu, &cfg, &ctl, &m, app.dvg);
        } else {
            /* The picture of this moment between two frames (render/subframe.h),
             * or the frame's own. */
            uint16_t st = 0;
            if (demo_on)
                st = rb_demo_render(&demo, acc, &scratch);
            else if (cfg.smooth == 3 && !original)
                st = rb_subframe_render(&sub, &m, (float)((game_t + acc - t_frame) / period), &scratch);
            const rb_machine *pic = st ? &scratch : &m;
            /* trees and clouds (render/deco.h): drawn into a copy of the picture */
            if (!original && (cfg.trees || cfg.clouds)) {
                if (!st) {
                    scratch = m;
                    scratch.sound_write = NULL;
                    scratch.view_hook = NULL;
                    st = (m.mem[RB_BUFFER] & 0x80) ? 0x2002 : 0x2802;   /* (the buffer on the screen) */
                }
                rb_deco_draw(&scratch, st, game_t + acc, cfg.trees, cfg.clouds);
                pic = &scratch;
            }
            /* whole lines with what they are (core/tags.h), then what is
             * hidden (render/solid.h), then the window the list sets */
            rb_avg_run_inst(pic->mem, st ? st : RB_VGRAM, pic->vgtag, pic->vgnote, pic->vginst, &list, 0);
            const rb_line *lines = list.lines;
            int n = list.count;
            if (demo_on) {
                /* the demo's picture, and of the real machine's: INSERT COINS, what a
                 * coin buys, CREDITS, the high score (its own instead of the demo's) */
                rb_avg_run_inst(m.mem, (m.mem[RB_BUFFER] & 0x80) ? 0x2002 : 0x2802, m.vgtag, m.vgnote, m.vginst,
                                &demo_list, 0);
                int w = 0;
                for (int i = 0; i < list.count; i++)
                    if (list.lines[i].tag != RB_TAG_HISCORE) list.lines[w++] = list.lines[i];
                for (int i = 0; i < demo_list.count && w < list.cap; i++) {
                    uint16_t tg = demo_list.lines[i].tag;
                    if (tg == RB_TAG_INSERT || tg == RB_TAG_ATTRACT || tg == RB_TAG_CREDITS || tg == RB_TAG_HISCORE)
                        list.lines[w++] = demo_list.lines[i];
                }
                list.count = n = w;
            }
            int game_page = rb_screens_page(pic->mem) == RB_PAGE_GAME;
            /* the cockpit draws its own propeller: the game's goes */
            if (cfg.cockpit && !original && game_page) list.count = n = rb_cockpit_drop_own_prop(list.lines, n);
            /* the enemy's roundels (render/fx.h), with the colours */
            if (cfg.color && !original && list.cap > n)
                list.count = n += rb_fx_roundels(list.lines, n, list.lines + n, list.cap - n);
            /* the explosions' fireballs (render/fx.h), into the world: the cockpit hides them */
            if (cfg.explosions && !original && list.cap > n)
                list.count = n += rb_fx_lines(&fx, list.lines, n, game_t + acc, list.lines + n, list.cap - n);
            /* the cockpit (render/cockpit.h): its lines, and its shapes in front of the world */
            static rb_line ckl[1024];
            static rb_solid_poly front[RB_COCKPIT_HULLS];
            int nck = 0;
            solid.front = NULL;
            solid.n_front = 0;
            if (cfg.cockpit && !original && rb_screens_page(pic->mem) == RB_PAGE_GAME) {
                nck = rb_cockpit_lines(&cockpit, pic->mem, game_t + acc, ckl, 1024);
                solid.front = front;
                solid.n_front = rb_cockpit_polys(&cockpit, front, RB_COCKPIT_HULLS);
            }
            solid.no_faces = !cfg.occlusion;
            if ((cfg.occlusion || solid.n_front) && !original)
                n = rb_solid_run(&solid, pic->mem, list.lines, list.count, &lines);
            /* the port's title, table and entry, free play's texts (render/screens.h) */
            static rb_line scr[8192];
            if (!original) {
                rb_screens_opt so = { cfg.screens, cfg.coin_mode == 0, cfg.language, cfg.cockpit };
                n = rb_screens_apply(&screens, demo_on ? m.mem : pic->mem, game_t + acc, &so, lines, n, scr, 8192);
                lines = scr;
            }
            static rb_line shown[8192];
            int k = 0;
            for (int i = 0; i < n && k < 8192; i++) {
                shown[k] = lines[i];
                if (rb_line_clip(&shown[k])) k++;
            }
            for (int i = 0; i < nck && k < 8192; i++) shown[k++] = ckl[i];
            rb_color_mark(shown, k);
            rb_draw_lines(app.dvg, &cfg, shown, k);
        }
        if (rb_app_end_frame(&app) != 0) {
            status = 1;
            break;
        }
    }
    if (roms_ok && ea_dirty_since >= 0.0) earom_save(&m, ea_path);
    rb_audio_close();
    rb_samples_free(&snd.smp);
    rb_controls_close(&ctl);
    rb_app_close(&app);
    rb_avg_list_free(&list);
    rb_avg_list_free(&demo_list);
    rb_solid_free(&solid);
    return status;
}
