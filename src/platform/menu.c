/* menu.c - see menu.h. */
#include "platform/menu.h"

#include "core/tags.h"
#include "render/draw.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Every line the menu can show, on whichever page. */
enum {
    L_GAP = 0,
    L_PAGE_CONTROLS, L_PAGE_PICTURE, L_PAGE_HATCH, L_PAGE_SOUND, L_PAGE_OPTIONS,
    L_RESET_SCORES, L_EXIT, L_BACK,
    L_MODE, L_SENSITIVITY, L_BRIGHTNESS, L_VOLUME, L_SOUNDS, L_ENGINE, L_SOUND_TEST,
    L_LIVES, L_BONUS, L_LANGUAGE, L_COINS,
    L_TOG0,                 /* + T_xxx: an option that is on or off */
    L_BIND0 = L_TOG0 + 16,  /* + RB_ACT_xxx */
    L_END = 0xff
};

enum { P_MAIN, P_CONTROLS, P_PICTURE, P_HATCH, P_SOUND, P_OPTIONS, N_PAGES };

/* The options that are on or off. */
enum { T_SMOOTH, T_COLOR, T_OCCLUSION, T_TREES, T_CLOUDS, T_COCKPIT, T_SCREENS, T_DEMO, T_FX, T_INVERT, T_COUNT };
static const struct { const char *label; size_t at; } TOGGLE[T_COUNT] = {
    { "SMOOTH MOTION", offsetof(rb_config, smooth) },
    { "COLORS", offsetof(rb_config, color) },
    { "SOLID OBJECTS", offsetof(rb_config, occlusion) },
    { "TREES", offsetof(rb_config, trees) },
    { "CLOUDS", offsetof(rb_config, clouds) },
    { "COCKPIT", offsetof(rb_config, cockpit) },
    { "TITLE AND HIGH SCORES", offsetof(rb_config, screens) },
    { "GAMEPLAY IN ATTRACT", offsetof(rb_config, attract_demo) },
    { "EXPLOSIONS", offsetof(rb_config, explosions) },
    { "UP AND DOWN REVERSED", offsetof(rb_config, invert_y) },
};
static int *toggle_of(const rb_config *cfg, int t) { return (int *)((char *)cfg + TOGGLE[t].at); }

#define PAGE_MAX 20

static const char *const TITLE[N_PAGES] = {
    "TEST MENU", "CONTROLS", "PICTURE", "TEST PICTURE", "SOUND", "GAME OPTIONS",
};

static const char *const BIND_LABEL[RB_ACT_COUNT] = {
    "UP", "DOWN", "LEFT", "RIGHT", "STICK LEFT RIGHT", "STICK UP DOWN", "FIRE", "START", "COIN", "TEST MENU",
};

static const char *const LANGUAGE_TEXT[4] = { "ENGLISH", "SPANISH", "FRENCH", "GERMAN" };
static const char *const COINS_TEXT[4] = { "FREE PLAY", "1 COIN 2 PLAYS", "1 COIN 1 PLAY", "2 COINS 1 PLAY" };
static const char *const BONUS_TEXT[4] = { "NONE", "6000 20000 50000", "4000 15000 40000", "2000 10000 30000" };
static const char *const SOUND_TEXT[RB_TEST_COUNT] = { "MACHINE GUN", "EXPLOSION", "NOSE DIVE", "ENGINE", "PLANE CLOSING",
                                                         "POKEY TONE" };

static int fullscreen(int page)
{
    return page == P_HATCH;
}

/* The lines of a page, gaps included. */
static int page_lines(int page, const rb_config *cfg, uint8_t *out)
{
    int n = 0;
#define ADD(l) out[n++] = (uint8_t)(l)
    switch (page) {
    case P_CONTROLS:
        ADD(L_MODE); ADD(L_SENSITIVITY); ADD(L_GAP);
        if (cfg->input_mode == RB_INPUT_ANALOG) { ADD(L_BIND0 + RB_ACT_STICK_X); ADD(L_BIND0 + RB_ACT_STICK_Y); }
        for (int a = RB_ACT_UP; a <= RB_ACT_RIGHT; a++) ADD(L_BIND0 + a);
        ADD(L_BIND0 + RB_ACT_FIRE); ADD(L_BIND0 + RB_ACT_START); ADD(L_BIND0 + RB_ACT_COIN);
        ADD(L_GAP); ADD(L_TOG0 + T_INVERT); ADD(L_GAP); ADD(L_BACK);
        break;
    case P_PICTURE:
        for (int t = T_SMOOTH; t <= T_FX; t++) ADD(L_TOG0 + t);
        ADD(L_BRIGHTNESS); ADD(L_GAP); ADD(L_BACK);
        break;
    case P_SOUND:
        ADD(L_VOLUME); ADD(L_SOUNDS); ADD(L_ENGINE); ADD(L_SOUND_TEST); ADD(L_GAP); ADD(L_BACK);
        break;
    case P_OPTIONS:
        ADD(L_LIVES); ADD(L_BONUS); ADD(L_LANGUAGE); ADD(L_COINS); ADD(L_GAP); ADD(L_BACK);
        break;
    case P_MAIN:
        ADD(L_PAGE_CONTROLS); ADD(L_PAGE_PICTURE); ADD(L_PAGE_HATCH); ADD(L_PAGE_SOUND);
        ADD(L_PAGE_OPTIONS); ADD(L_GAP); ADD(L_RESET_SCORES); ADD(L_GAP); ADD(L_EXIT);
        break;
    default:
        break;
    }
#undef ADD
    out[n] = L_END;
    return n;
}

/* The nth line that is no gap, and how many there are. */
static int chosen_line(const uint8_t *lines, int item, int *count)
{
    int k = 0, line = L_GAP;
    for (int i = 0; lines[i] != L_END; i++) {
        if (lines[i] == L_GAP) continue;
        if (k == item) line = lines[i];
        k++;
    }
    *count = k;
    return line;
}

static int step(int v, int d, int lo, int hi)
{
    v += d;
    return v > hi ? lo : v < lo ? hi : v;
}

static float stepf(float v, int d, float by, float lo, float hi)
{
    v += (float)d * by;
    if (fabsf(v) < by * 0.01f) v = 0.0f;
    return v < lo ? lo : v > hi ? hi : v;
}

void rb_menu_open(rb_menu *u)
{
    u->open = 1;
    u->page = P_MAIN;
    u->capture = -1;
    u->confirm = 0;
    u->dir_x = u->dir_y = 0;
    u->settle = 0;
}

int rb_menu_update(rb_menu *u, rb_config *cfg, rb_controls *ctl, double now)
{
    if (u->page < 0 || u->page >= N_PAGES) u->page = P_MAIN;

    if (u->capture >= 0) {
        char spec[RB_BIND_SIZE];
        int r = rb_controls_capture_poll(ctl, spec, sizeof spec);
        if (r == 0) return RB_MENU_NOTHING;
        int action = u->capture;
        u->capture = -1;
        /* (what was pressed is still held, and from now on may move the
         * menu: it is not to count until it is let go) */
        u->settle = 1;
        ctl->fire_was = 1;
        if (r < 0) return RB_MENU_NOTHING;
        snprintf(cfg->bind[action], RB_BIND_SIZE, "%s", spec);
        return RB_MENU_CHANGED;
    }

    /* a push counts once, and again and again while it is held */
    int x = ctl->nav.x, y = ctl->nav.y, dx = 0, dy = 0;
    if (u->settle) {
        u->dir_x = x;
        u->dir_y = y;
        if (x || y) return RB_MENU_NOTHING;
        u->settle = 0;
    }
    if (x != u->dir_x || y != u->dir_y) {
        dx = x != u->dir_x ? x : 0;
        dy = y != u->dir_y ? y : 0;
        u->repeat_at = now + 0.5;
    } else if ((x || y) && now >= u->repeat_at) {
        dx = x;
        dy = y;
        u->repeat_at = now + 0.15;
    }
    u->dir_x = x;
    u->dir_y = y;
    int fire = ctl->nav.fire;
    /* (start as it goes down, followed on every page: the start that opened
     * a test page is not to close it again) */
    static int start_was;
    int back = ctl->start && !start_was;
    start_was = ctl->start;

    if (fullscreen(u->page)) {
        /* start or fire goes back */
        if (back || (u->page == P_HATCH && fire)) {
            u->page = P_MAIN;
            ctl->fire_was = 1;
        }
        return RB_MENU_NOTHING;
    }

    uint8_t lines[PAGE_MAX + 1];
    int n;
    page_lines(u->page, cfg, lines);
    int line = chosen_line(lines, u->item[u->page], &n);
    if (u->item[u->page] >= n) {
        u->item[u->page] = 0;
        line = chosen_line(lines, 0, &n);
    }
    if (dy) {
        u->item[u->page] = step(u->item[u->page], dy, 0, n - 1);
        u->confirm = 0;
        return RB_MENU_NOTHING;
    }
    int d = dx ? dx : (fire ? 1 : 0);
    if (!d) return RB_MENU_NOTHING;

    if (line >= L_BIND0) {
        int action = line - L_BIND0;
        if (fire) {
            u->capture = action;
            rb_controls_capture_begin(ctl, action);
            return RB_MENU_NOTHING;
        }
        if (!strcmp(cfg->bind[action], "auto")) return RB_MENU_NOTHING;
        snprintf(cfg->bind[action], RB_BIND_SIZE, "auto");
        return RB_MENU_CHANGED;
    }
    if (line >= L_TOG0 && line < L_TOG0 + T_COUNT) {
        int t = line - L_TOG0;
        int *v = toggle_of(cfg, t);
        *v = *v ? 0 : (t == T_SMOOTH ? 3 : 1);
        return RB_MENU_CHANGED;
    }
    switch (line) {
    case L_PAGE_CONTROLS: if (fire) u->page = P_CONTROLS; break;
    case L_PAGE_PICTURE:  if (fire) u->page = P_PICTURE; break;
    case L_PAGE_HATCH:    if (fire) { u->page = P_HATCH; ctl->fire_was = 1; } break;
    case L_PAGE_SOUND:    if (fire) u->page = P_SOUND; break;
    case L_PAGE_OPTIONS:  if (fire) u->page = P_OPTIONS; break;
    case L_BACK:          if (fire) u->page = P_MAIN; break;
    case L_EXIT:          if (fire) return RB_MENU_CLOSE; break;
    case L_RESET_SCORES:
        if (!fire) break;
        if (!u->confirm) { u->confirm = 1; break; }
        u->confirm = 0;
        return RB_MENU_RESET_SCORES;
    case L_MODE:
        cfg->input_mode = step(cfg->input_mode, d, 0, 1);
        return RB_MENU_CHANGED;
    case L_SENSITIVITY:
        /* digital: how fast the held stick moves out; analog: how far a push goes */
        if (cfg->input_mode == RB_INPUT_ANALOG) cfg->gain = stepf(cfg->gain, d, 0.1f, 0.5f, 2.0f);
        else cfg->digital_rate = stepf(roundf(cfg->digital_rate), d, 1.0f, 1.0f, 10.0f);
        return RB_MENU_CHANGED;
    case L_BRIGHTNESS: cfg->brightness = stepf(cfg->brightness, d, 0.1f, 0.3f, 3.0f); return RB_MENU_CHANGED;
    case L_VOLUME:   cfg->volume = stepf(cfg->volume, d, 0.1f, 0.0f, 1.0f); return RB_MENU_CHANGED;
    case L_SOUNDS:   cfg->samples = !cfg->samples; return RB_MENU_CHANGED;
    case L_ENGINE:   cfg->engine_volume = stepf(cfg->engine_volume, d, 0.1f, 0.0f, 1.0f); return RB_MENU_CHANGED;
    case L_SOUND_TEST:
        if (dx) u->sound = step(u->sound, dx, 0, RB_TEST_COUNT - 1);
        if (fire) return RB_MENU_SOUND;
        break;
    case L_LIVES:    cfg->lives = step(cfg->lives, d, 2, 5); return RB_MENU_CHANGED;
    case L_BONUS:    cfg->bonus = step(cfg->bonus, d, 0, 3); return RB_MENU_CHANGED;
    case L_LANGUAGE: cfg->language = step(cfg->language, d, 0, 3); return RB_MENU_CHANGED;
    case L_COINS:    cfg->coin_mode = step(cfg->coin_mode, d, 0, 3); return RB_MENU_CHANGED;
    }
    return RB_MENU_NOTHING;
}

static void upper(char *s)
{
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

/* A line's label and its value ("" for none). */
static void describe(int line, const rb_menu *u, const rb_config *cfg, const char **label, char *value, size_t size)
{
    value[0] = '\0';
    if (line >= L_BIND0) {
        int action = line - L_BIND0;
        *label = BIND_LABEL[action];
        if (u->capture == action) {
            snprintf(value, size, "%s", action == RB_ACT_STICK_X ? "PUSH IT TO THE RIGHT" :
                                        action == RB_ACT_STICK_Y ? "PULL IT DOWN" : "PRESS IT NOW");
        } else {
            snprintf(value, size, "%s", cfg->bind[action]);
            upper(value);
        }
        return;
    }
    if (line >= L_TOG0 && line < L_TOG0 + T_COUNT) {
        *label = TOGGLE[line - L_TOG0].label;
        snprintf(value, size, "%s", *toggle_of(cfg, line - L_TOG0) ? "ON" : "OFF");
        return;
    }
    switch (line) {
    case L_PAGE_CONTROLS: *label = "CONTROLS"; break;
    case L_PAGE_PICTURE:  *label = "PICTURE"; break;
    case L_PAGE_HATCH:    *label = "TEST PICTURE"; break;
    case L_PAGE_SOUND:    *label = "SOUND"; break;
    case L_PAGE_OPTIONS:  *label = "GAME OPTIONS"; break;
    case L_RESET_SCORES:  *label = "RESET HIGH SCORES"; snprintf(value, size, "%s", u->confirm ? "FIRE AGAIN" : ""); break;
    case L_EXIT:          *label = "SAVE AND EXIT"; break;
    case L_BACK:          *label = "BACK"; break;
    case L_MODE:          *label = "STICK"; snprintf(value, size, "%s", cfg->input_mode == RB_INPUT_ANALOG ? "ANALOG" : "DIGITAL"); break;
    case L_SENSITIVITY:
        *label = "SENSITIVITY";
        if (cfg->input_mode == RB_INPUT_ANALOG) snprintf(value, size, "%.1f", cfg->gain);
        else snprintf(value, size, "%d", (int)lroundf(cfg->digital_rate));
        break;
    case L_BRIGHTNESS:    *label = "BRIGHTNESS"; snprintf(value, size, "%d", (int)(cfg->brightness * 10.0f + 0.5f)); break;
    case L_VOLUME:        *label = "VOLUME"; snprintf(value, size, "%d", (int)(cfg->volume * 10.0f + 0.5f)); break;
    case L_SOUNDS:        *label = "SOUNDS"; snprintf(value, size, "%s", cfg->samples ? "RECORDINGS" : "ORIGINAL"); break;
    case L_ENGINE:        *label = "ENGINE VOLUME"; snprintf(value, size, "%d", (int)(cfg->engine_volume * 10.0f + 0.5f)); break;
    case L_SOUND_TEST:    *label = "SOUND TEST"; snprintf(value, size, "%s", SOUND_TEXT[u->sound % RB_TEST_COUNT]); break;
    case L_LIVES:         *label = "PLANES"; snprintf(value, size, "%d", cfg->lives); break;
    case L_BONUS:         *label = "BONUS PLANE AT"; snprintf(value, size, "%s", BONUS_TEXT[cfg->bonus & 3]); break;
    case L_LANGUAGE:      *label = "LANGUAGE"; snprintf(value, size, "%s", LANGUAGE_TEXT[cfg->language & 3]); break;
    case L_COINS:         *label = "COINS"; snprintf(value, size, "%s", COINS_TEXT[cfg->coin_mode & 3]); break;
    default:              *label = "?"; break;
    }
}

/* --- the test picture: a crosshatch in the game's own screen units ----------------- */

#define MAXL 160
static rb_line g_l[MAXL];
static int g_n;

static void add(float x0, float y0, float x1, float y1, int intensity)
{
    if (g_n >= MAXL) return;
    rb_line *q = &g_l[g_n++];
    memset(q, 0, sizeof *q);
    q->x0 = x0; q->y0 = y0; q->x1 = x1; q->y1 = y1;
    q->intensity = (uint8_t)intensity;
}

static void draw_hatch(const rb_config *cfg, vp_dvg *d)
{
    /* lines every 40 units, the edge, a circle that is round when the
     * proportions are right */
    float l = cfg->view_left, r = cfg->view_right, b = cfg->view_bottom, t = cfg->view_top;
    g_n = 0;
    for (float x = 0.0f; x <= r; x += 40.0f) { add(x, b, x, t, 10); if (x > 0) add(-x, b, -x, t, 10); }
    for (float y = 0.0f; y <= t; y += 40.0f) { add(l, y, r, y, 10); if (y > 0) add(l, -y, r, -y, 10); }
    add(l, b, r, b, 15); add(r, b, r, t, 15); add(r, t, l, t, 15); add(l, t, l, b, 15);
    for (int i = 0; i < 48; i++) {
        float a0 = (float)i / 48.0f * 6.2831853f, a1 = (float)(i + 1) / 48.0f * 6.2831853f;
        add(cosf(a0) * 160.0f, sinf(a0) * 160.0f, cosf(a1) * 160.0f, sinf(a1) * 160.0f, 15);
    }
    rb_config shown = *cfg;
    shown.color = 0;
    rb_draw_lines(d, &shown, g_l, g_n);
}

/* --- the control test ------------------------------------------------------------------- */

static void box(vp_dvg *d, int x0, int y0, int x1, int y1)
{
    vp_moveto(d, x0, y0);
    vp_lineto(d, x1, y0);
    vp_lineto(d, x1, y1);
    vp_lineto(d, x0, y1);
    vp_lineto(d, x0, y0);
}

/* A word bright when what it names is held, dim otherwise. */
static void lamp(vp_dvg *d, const char *word, int on, float x, float y)
{
    if (on) vp_color(d, 255, 255, 255);
    else vp_color(d, 70, 70, 70);
    vp_text(d, word, x, y, 70, VP_TEXT_LEFT);
}

/* The stick, live, beside the controls: what the game gets (a cross), the
 * analog axes as read (a small square), the pots, and what is held. */
static void draw_stick_panel(const rb_controls *c, const rb_machine *m, vp_dvg *d)
{
    char s[64];
    vp_text_font(VP_FONT_ASTEROIDS);

    const int cx = 3300, cy = 2550, half = 520;
    vp_color(d, 60, 200, 90);
    box(d, cx - half, cy - half, cx + half, cy + half);
    vp_color(d, 30, 100, 45);
    vp_moveto(d, cx - half, cy); vp_lineto(d, cx + half, cy);
    vp_moveto(d, cx, cy - half); vp_lineto(d, cx, cy + half);
    int px = cx + (int)lroundf(c->x * half), py = cy - (int)lroundf(c->y * half);
    vp_color(d, 255, 255, 255);
    vp_moveto(d, px - 70, py); vp_lineto(d, px + 70, py);
    vp_moveto(d, px, py - 70); vp_lineto(d, px, py + 70);
    int ax = cx + (int)lroundf(fmaxf(-1.0f, fminf(1.0f, c->ax)) * half), ay = cy - (int)lroundf(fmaxf(-1.0f, fminf(1.0f, c->ay)) * half);
    vp_color(d, 255, 120, 60);
    box(d, ax - 18, ay - 18, ax + 18, ay + 18);

    const float x = cx - half;
    vp_color(d, 80, 220, 255);
    snprintf(s, sizeof s, "X %+.2f  Y %+.2f", c->x, c->y);
    vp_text(d, s, x, 1880, 60, VP_TEXT_LEFT);
    snprintf(s, sizeof s, "POTS %u %u", m->in.pot_x, m->in.pot_y);
    vp_text(d, s, x, 1760, 60, VP_TEXT_LEFT);
    lamp(d, "FIRE", c->fire, x, 1580);
    lamp(d, "START", c->start, x, 1460);
    lamp(d, "COIN", c->coin, x, 1340);
}

/* --- the menu -------------------------------------------------------------------------- */

void rb_menu_draw(const rb_menu *u, const rb_config *cfg, const rb_controls *ctl, const rb_machine *m, vp_dvg *d)
{
    int page = u->page >= 0 && u->page < N_PAGES ? u->page : P_MAIN;

    if (fullscreen(page)) {
        draw_hatch(cfg, d);
        vp_text_font(VP_FONT_ASTEROIDS);
        vp_color(d, 110, 130, 255);
        vp_text(d, "FIRE OR START BACK", 2048, 300, 55, VP_TEXT_CENTER);
        return;
    }
    /* the controls: the stick live beside them, the lines narrower */
    float LEFT = 620.0f, RIGHT = 3560.0f;
    if (page == P_CONTROLS) {
        draw_stick_panel(ctl, m, d);
        LEFT = 300.0f;
        RIGHT = 2600.0f;
    }

    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 255, 200, 40);
    vp_text(d, TITLE[page], 2048, 3680, 95, VP_TEXT_CENTER);

    const float STEP = 170.0f, GAP = 100.0f, H = 80.0f;
    uint8_t lines[PAGE_MAX + 1];
    page_lines(page, cfg, lines);

    /* the lines, from the top, the gaps where the page has them; the
     * block centred between the title and the hint */
    float height = 0.0f;
    for (int i = 0; lines[i] != L_END; i++) height += lines[i] ? STEP : GAP;
    float y = 1980.0f + height * 0.5f - STEP * 0.5f;
    int k = 0, chosen_line_id = L_GAP;
    for (int i = 0; lines[i] != L_END; i++) {
        int line = lines[i];
        if (!line) { y -= GAP; continue; }
        const char *label;
        char value[RB_BIND_SIZE + 8];
        describe(line, u, cfg, &label, value, sizeof value);
        int chosen = k == u->item[page];
        if (chosen) {                   /* an arrow before the line chosen */
            chosen_line_id = line;
            vp_color(d, 255, 255, 255);
            vp_moveto(d, (int)LEFT - 160, (int)(y + H * 0.875f));
            vp_lineto(d, (int)LEFT - 70, (int)(y + H * 0.5f));
            vp_lineto(d, (int)LEFT - 160, (int)(y + H * 0.125f));
        } else {
            vp_color(d, 60, 200, 90);
        }
        vp_text(d, label, LEFT, y, H, VP_TEXT_LEFT);
        if (value[0]) {
            /* (a long binding is written smaller rather than into its label) */
            float room = RIGHT - LEFT - vp_text_width(label, H) - 150.0f, w = vp_text_width(value, H), hh = H;
            if (w > room && w > 0.0f) hh = H * room / w;
            if (u->capture >= 0 && chosen) vp_color(d, 255, 60, 50);
            else if (chosen) vp_color(d, 255, 255, 255);
            else vp_color(d, 80, 220, 255);
            vp_text(d, value, RIGHT, y, hh, VP_TEXT_RIGHT);
        }
        y -= STEP;
        k++;
    }
    vp_color(d, 110, 130, 255);
    char hint_main[96];
    snprintf(hint_main, sizeof hint_main, "UP DOWN CHOOSE   LEFT RIGHT CHANGE   FIRE DO   %s SAVE AND EXIT",
             rb_controls_service_name(ctl));
    const char *hint = hint_main;
    if (u->capture >= 0) hint = "PRESS THE NEW KEY OR BUTTON   ESC GIVES UP";
    else if (chosen_line_id >= L_BIND0) hint = "FIRE NEW KEY   LEFT RIGHT BACK TO AUTO";
    vp_text(d, hint, 2048, 300, 55, VP_TEXT_CENTER);
}
