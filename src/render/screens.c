/* screens.c - see screens.h. */
#include "render/screens.h"

#include "core/rb_symbols.h"
#include "core/tags.h"
#include "render/glyph.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265f

/* --- drawing into a list of lines ----------------------------------------------- */

typedef struct sink {
    rb_line *l;
    int      n, max;
} sink;

static void seg(sink *k, float x0, float y0, float x1, float y1, int intensity, int tag)
{
    if (k->n >= k->max || intensity <= 0) return;
    rb_line *q = &k->l[k->n++];
    memset(q, 0, sizeof *q);
    q->x0 = x0; q->y0 = y0; q->x1 = x1; q->y1 = y1;
    q->intensity = (uint8_t)(intensity > 15 ? 15 : intensity);
    q->tag = (uint16_t)tag;
    q->win[0] = q->win[1] = -1e6f;             /* (no window: rb_line_clip leaves it whole) */
    q->win[2] = q->win[3] = 1e6f;
}

static void text(sink *k, const char *s, float x, float y, float size, int align, int intensity, int tag, float slant)
{
    if (intensity <= 0) return;
    k->n += rb_text(k->l + k->n, k->max - k->n, s, x, y, size, align, (uint8_t)(intensity > 15 ? 15 : intensity),
                    (uint16_t)tag, slant);
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static float ease_out(float v) { v = clamp01(v); return 1.0f - (1.0f - v) * (1.0f - v) * (1.0f - v); }

/* A place on a circle. */
static void on_circle(float cx, float cy, float r, float a, float *x, float *y)
{
    *x = cx + r * cosf(a);
    *y = cy + r * sinf(a);
}

/* A circle, from angle a0 as far as the part `part` of it (drawing itself on). */
static void circle(sink *k, float cx, float cy, float r, int segs, float a0, float part, int intensity, int tag)
{
    float x0, y0, x1, y1;
    int n = (int)ceilf(segs * clamp01(part));
    on_circle(cx, cy, r, a0, &x0, &y0);
    for (int i = 1; i <= n; i++) {
        float a = a0 + 2.0f * PI * fminf((float)i / segs, part);
        on_circle(cx, cy, r, a, &x1, &y1);
        seg(k, x0, y0, x1, y1, intensity, tag);
        x0 = x1; y0 = y1;
    }
}

/* --- the iron cross ------------------------------------------------------------- */

/* Its outline in units of the arm's length: four arms flaring out from the
 * middle, their sides curved in, their ends straight (the Eisernes Kreuz of
 * the flyer). */
#define CROSS_SIDE 6
#define CROSS_PTS  (4 * (2 * CROSS_SIDE + 1))

static int cross_outline(float *px, float *py, float k)
{
    const float in = 0.17f, out = 0.52f;
    int n = 0;
    for (int arm = 0; arm < 4; arm++) {
        float ca = cosf(arm * PI * 0.5f), sa = sinf(arm * PI * 0.5f);
        /* this arm along +x: from the corner below it out, across its end, back in */
        float ax[2 * CROSS_SIDE + 1], ay[2 * CROSS_SIDE + 1];
        int m = 0;
        for (int i = 0; i <= CROSS_SIDE; i++) {          /* lower side: (in, -in) -> (1, -out), curved in */
            float t = (float)i / CROSS_SIDE;
            float x = in + (1.0f - in) * t, y = -(in + (out - in) * t * t);
            ax[m] = x; ay[m] = y; m++;
        }
        for (int i = CROSS_SIDE; i >= 1; i--) {         /* upper side, back (its inner corner is the next arm's first) */
            float t = (float)i / CROSS_SIDE;
            float x = in + (1.0f - in) * t, y = in + (out - in) * t * t;
            ax[m] = x; ay[m] = y; m++;
        }
        for (int i = 0; i < m; i++) {
            px[n] = k * (ax[i] * ca - ay[i] * sa);
            py[n] = k * (ax[i] * sa + ay[i] * ca);
            n++;
        }
    }
    return n;
}

static void cross_lines(sink *k, float x, float y, float r, float angle, int intensity, int rim, int body)
{
    float px[CROSS_PTS], py[CROSS_PTS];
    float c = cosf(angle), s = sinf(angle);
    for (int pass = 0; pass < 2; pass++) {
        int n = cross_outline(px, py, pass == 0 ? 1.0f : 0.84f);
        int tag = pass == 0 ? rim : body;
        if (!tag) continue;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float x0 = x + r * (px[i] * c - py[i] * s), y0 = y + r * (px[i] * s + py[i] * c);
            float x1 = x + r * (px[j] * c - py[j] * s), y1 = y + r * (px[j] * s + py[j] * c);
            seg(k, x0, y0, x1, y1, intensity, tag);
        }
    }
}

int rb_screens_cross(rb_line *out, int max, float x, float y, float r, float angle, uint8_t intensity)
{
    sink k = { out, 0, max };
    cross_lines(&k, x, y, r, angle, intensity, RB_TAG_LOGO_RIM, RB_TAG_LOGO_CROSS);
    return k.n;
}

/* --- the eagle's head (the flyer's: looking left, out of the cross) ------------- */

static const float EAGLE[][2] = {
    { 0.36f, -0.30f }, { 0.31f, 0.05f }, { 0.20f, 0.25f }, { 0.00f, 0.34f }, { -0.20f, 0.29f }, { -0.33f, 0.19f },
    { -0.48f, 0.16f }, { -0.62f, 0.09f }, { -0.67f, -0.04f }, { -0.60f, -0.07f }, { -0.56f, -0.01f }, { -0.43f, -0.02f },
    { -0.31f, -0.08f }, { -0.23f, -0.18f }, { -0.15f, -0.32f }, { -0.05f, -0.45f }, { 0.05f, -0.36f }, { 0.13f, -0.50f },
    { 0.23f, -0.38f }, { 0.33f, -0.48f },
};

static void eagle(sink *k, float x, float y, float r, int intensity)
{
    int n = (int)(sizeof EAGLE / sizeof EAGLE[0]);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        seg(k, x + r * EAGLE[i][0], y + r * EAGLE[i][1], x + r * EAGLE[j][0], y + r * EAGLE[j][1], intensity,
            RB_TAG_LOGO_EAGLE);
    }
    /* the line of the beak, the eye */
    seg(k, x + r * -0.48f, y + r * 0.16f, x + r * -0.30f, y + r * 0.05f, intensity, RB_TAG_LOGO_EAGLE);
    float ex = x + r * -0.17f, ey = y + r * 0.15f, e = r * 0.045f;
    seg(k, ex - e, ey, ex, ey + e, intensity, RB_TAG_LOGO_EAGLE);
    seg(k, ex, ey + e, ex + e, ey, intensity, RB_TAG_LOGO_EAGLE);
    seg(k, ex + e, ey, ex, ey - e, intensity, RB_TAG_LOGO_EAGLE);
    seg(k, ex, ey - e, ex - e, ey, intensity, RB_TAG_LOGO_EAGLE);
    /* the gold and red streaming back out of the neck */
    for (int b = 0; b < 2; b++) {
        float y0 = y + r * (b == 0 ? -0.06f : -0.20f), h = r * 0.09f;
        float xa = x + r * 0.30f, xb = x + r * 0.86f;
        int tag = b == 0 ? RB_TAG_LOGO_GOLD : RB_TAG_LOGO_RED;
        seg(k, xa, y0, xb, y0, intensity, tag);
        seg(k, xb, y0, xb + h, y0 - h, intensity, tag);
        seg(k, xb + h, y0 - h, xa + h * 0.5f, y0 - h, intensity, tag);
    }
}

/* --- "Red Baron" in a broken script ----------------------------------------------- */

/* Each letter: strokes as the middle line of a broad pen held at an angle,
 * in units of the capital's height, the baseline at 0. The pen gives thick
 * and thin as a Fraktur hand: both edges of the nib are drawn. */
typedef struct stroke { int n; float p[8][2]; } stroke;
typedef struct letter { int n; stroke s[3]; float adv; } letter;

static const letter SCRIPT[] = {
    /* R */ { 3, { { 5, { { 0.02f, 0.05f }, { 0.12f, 0.00f }, { 0.18f, 0.08f }, { 0.18f, 0.82f }, { 0.07f, 0.94f } } },
                   { 7, { { 0.18f, 0.88f }, { 0.32f, 1.00f }, { 0.55f, 0.95f }, { 0.62f, 0.78f }, { 0.55f, 0.60f },
                          { 0.30f, 0.52f }, { 0.18f, 0.55f } } },
                   { 5, { { 0.35f, 0.52f }, { 0.55f, 0.30f }, { 0.62f, 0.06f }, { 0.72f, 0.00f }, { 0.80f, 0.06f } } } },
             0.86f },
    /* e */ { 1, { { 8, { { 0.55f, 0.12f }, { 0.40f, 0.00f }, { 0.15f, 0.06f }, { 0.08f, 0.32f }, { 0.15f, 0.56f },
                          { 0.32f, 0.62f }, { 0.48f, 0.52f }, { 0.10f, 0.36f } } } },
             0.64f },
    /* d */ { 2, { { 7, { { 0.48f, 0.55f }, { 0.25f, 0.62f }, { 0.08f, 0.45f }, { 0.08f, 0.15f }, { 0.22f, 0.00f },
                          { 0.45f, 0.05f }, { 0.52f, 0.30f } } },
                   { 4, { { 0.52f, 0.20f }, { 0.52f, 0.70f }, { 0.38f, 0.95f }, { 0.18f, 1.00f } } } },
             0.68f },
    /*   */ { 0, { { 0 } }, 0.36f },
    /* B */ { 3, { { 6, { { 0.05f, 0.92f }, { 0.15f, 1.00f }, { 0.20f, 0.90f }, { 0.20f, 0.10f }, { 0.10f, 0.00f },
                          { 0.02f, 0.06f } } },
                   { 6, { { 0.20f, 0.95f }, { 0.45f, 1.00f }, { 0.62f, 0.88f }, { 0.60f, 0.68f }, { 0.40f, 0.55f },
                          { 0.22f, 0.55f } } },
                   { 6, { { 0.40f, 0.55f }, { 0.65f, 0.45f }, { 0.70f, 0.22f }, { 0.58f, 0.04f }, { 0.30f, 0.00f },
                          { 0.20f, 0.06f } } } },
             0.86f },
    /* a */ { 2, { { 6, { { 0.45f, 0.52f }, { 0.25f, 0.62f }, { 0.08f, 0.45f }, { 0.08f, 0.15f }, { 0.22f, 0.00f },
                          { 0.45f, 0.10f } } },
                   { 5, { { 0.38f, 0.55f }, { 0.50f, 0.62f }, { 0.50f, 0.08f }, { 0.58f, 0.00f }, { 0.66f, 0.06f } } } },
             0.74f },
    /* r */ { 2, { { 5, { { 0.04f, 0.52f }, { 0.12f, 0.62f }, { 0.18f, 0.55f }, { 0.18f, 0.06f }, { 0.10f, 0.00f } } },
                   { 4, { { 0.18f, 0.45f }, { 0.32f, 0.62f }, { 0.46f, 0.55f }, { 0.50f, 0.47f } } } },
             0.56f },
    /* o */ { 1, { { 7, { { 0.30f, 0.62f }, { 0.10f, 0.50f }, { 0.07f, 0.18f }, { 0.28f, 0.00f }, { 0.50f, 0.12f },
                          { 0.53f, 0.44f }, { 0.30f, 0.62f } } } },
             0.64f },
    /* n */ { 2, { { 5, { { 0.03f, 0.52f }, { 0.12f, 0.62f }, { 0.18f, 0.55f }, { 0.18f, 0.05f }, { 0.08f, 0.00f } } },
                   { 6, { { 0.18f, 0.48f }, { 0.35f, 0.62f }, { 0.50f, 0.52f }, { 0.50f, 0.06f }, { 0.58f, 0.00f },
                          { 0.65f, 0.05f } } } },
             0.72f },
};
#define N_SCRIPT ((int)(sizeof SCRIPT / sizeof SCRIPT[0]))

static float script_width(void)
{
    float w = 0.0f;
    for (int i = 0; i < N_SCRIPT; i++) w += SCRIPT[i].adv;
    return w;
}

/* A stroke drawn as far as `part` of its length, both edges of the pen. */
static void pen_stroke(sink *k, const stroke *st, float x, float y, float h, float part, int intensity, int tag)
{
    const float nx = 0.05f, ny = 0.065f;       /* half the nib, at about 50 degrees */
    float len = 0.0f;
    for (int i = 1; i < st->n; i++) len += hypotf(st->p[i][0] - st->p[i - 1][0], st->p[i][1] - st->p[i - 1][1]);
    float left = len * clamp01(part);
    if (left <= 0.0f) return;
    float ax = st->p[0][0], ay = st->p[0][1];
    seg(k, x + h * (ax - nx), y + h * (ay - ny), x + h * (ax + nx), y + h * (ay + ny), intensity, tag);  /* its start */
    for (int i = 1; i < st->n && left > 0.0f; i++) {
        float bx = st->p[i][0], by = st->p[i][1];
        float d = hypotf(bx - ax, by - ay);
        if (d > left) {
            bx = ax + (bx - ax) * left / d;
            by = ay + (by - ay) * left / d;
            d = left;
        }
        seg(k, x + h * (ax + nx), y + h * (ay + ny), x + h * (bx + nx), y + h * (by + ny), intensity, tag);
        seg(k, x + h * (ax - nx), y + h * (ay - ny), x + h * (bx - nx), y + h * (by - ny), intensity, tag);
        left -= d;
        ax = bx; ay = by;
        if (left <= 0.0f || i == st->n - 1)
            seg(k, x + h * (ax - nx), y + h * (ay - ny), x + h * (ax + nx), y + h * (ay + ny), intensity, tag);
    }
}

/* "Red Baron", centred at x, its baseline at y, capitals h high; written on
 * as `part` goes from 0 to 1, letter after letter. A shine passes at
 * shine_x (screen units). */
static void script(sink *k, float x, float y, float h, float part, float shine_x, int intensity)
{
    float w = script_width() * h, at = x - w * 0.5f;
    for (int i = 0; i < N_SCRIPT; i++) {
        const letter *le = &SCRIPT[i];
        float lp = clamp01(part * N_SCRIPT - (float)i);
        float mid = at + le->adv * h * 0.5f;
        int b = fabsf(mid - shine_x) < h * 0.6f ? 15 : intensity;
        for (int s = 0; s < le->n; s++) pen_stroke(k, &le->s[s], at, y, h, lp, b, RB_TAG_LOGO_TEXT);
        at += le->adv * h;
    }
}

/* --- the pages -------------------------------------------------------------------- */

static const char *const T_HIGH_SCORES[4] = { "HIGH SCORES", "RECORDS", "RECORDS", "BESTENLISTE" };
static const char *const T_NEW_HIGH[4] = { "NEW HIGH SCORE", "NUEVO RECORD", "NOUVEAU RECORD", "NEUER REKORD" };
static const char *const T_YOUR_SCORE[4] = { "YOUR SCORE", "PUNTOS", "VOTRE SCORE", "DEINE PUNKTE" };
static const char *const T_PLACE[4] = { "PLACE", "PUESTO", "PLACE", "PLATZ" };
static const char *const T_HINT1[4] = { "STICK UP OR DOWN CHANGES THE LETTER", "PALANCA ARRIBA ABAJO CAMBIA LA LETRA",
                                        "MANCHE HAUT BAS CHANGE LA LETTRE", "KNUEPPEL AUF AB AENDERT DEN BUCHSTABEN" };
static const char *const T_HINT2[4] = { "FIRE TAKES IT", "FUEGO LA ACEPTA", "TIR VALIDE", "FEUER UEBERNIMMT IHN" };
static const char *const T_FREE[4] = { "FREE PLAY", "JUEGO LIBRE", "JEU GRATUIT", "FREISPIEL" };
static const char *const T_PRESS[4] = { "PRESS START", "PULSE START", "APPUYEZ SUR START", "START DRUECKEN" };

/* A score as the game keeps it (two bytes of BCD, lo first; a 0 behind). */
static void score_text(char *s, uint8_t lo, uint8_t hi)
{
    char d[6];
    snprintf(d, sizeof d, "%X%X%X%X", hi >> 4, hi & 15, lo >> 4, lo & 15);
    const char *p = d;
    while (*p == '0' && p[1]) p++;
    if (!strcmp(p, "0")) snprintf(s, 8, "0");
    else snprintf(s, 8, "%s0", p);
}

static void initials(char *s, const uint8_t *mem, int at)
{
    for (int i = 0; i < 3; i++) {
        int c = mem[at + i] & 0xfe;
        s[i] = c >= 0x4e ? ' ' : rb_glyph_char(c);
        if (s[i] == '_') s[i] = ' ';
    }
    s[3] = '\0';
}

static void draw_title(sink *k, double tau)
{
    float tt = (float)tau;
    /* the stripes, sliding in from the sides */
    float slide = 1.0f - ease_out(tt / 0.9f);
    for (int side = -1; side <= 1; side += 2) {
        for (int b = 0; b < 2; b++) {
            int tag = b == 0 ? RB_TAG_LOGO_GOLD : RB_TAG_LOGO_RED;
            float y_in = 118.0f - b * 12.0f, y_out = 128.0f - b * 12.0f, hh = 7.0f;
            float x_in = side * 74.0f, x_out = side * (250.0f + 300.0f * slide);
            float off = side * 300.0f * slide;
            seg(k, x_in + off, y_in, x_out, y_out, 12, tag);
            seg(k, x_in + off, y_in - hh, x_out, y_out - hh, 12, tag);
            seg(k, x_in + off, y_in, x_in + off, y_in - hh, 12, tag);
        }
    }
    /* the circle drawing itself, then the cross turning in, then the eagle */
    const float cx = 0.0f, cy = 52.0f;
    circle(k, cx, cy, 68.0f, 40, PI * 0.5f, tt / 1.0f, 13, RB_TAG_LOGO_RIM);
    circle(k, cx, cy, 63.0f, 40, PI * 0.5f, (tt - 0.2f) / 1.0f, 8, RB_TAG_LOGO_RIM);
    float e = ease_out((tt - 0.3f) / 1.3f);
    if (e > 0.0f) cross_lines(k, cx, cy, 56.0f * e, (1.0f - e) * 2.0f * PI, 13, RB_TAG_LOGO_RIM, RB_TAG_LOGO_CROSS);
    int ei = (int)(15.0f * clamp01((tt - 1.3f) / 0.6f));
    if (ei > 0) eagle(k, cx - 6.0f, cy + 2.0f, 46.0f, ei);
    /* the lettering, written on; then a shine across it every four seconds */
    float shine = -999.0f;
    if (tt > 3.8f) {
        float c = fmodf(tt - 3.8f, 4.0f);
        if (c < 1.2f) shine = -260.0f + 520.0f * c / 1.2f;
    }
    script(k, 0.0f, -96.0f, 60.0f, (tt - 1.5f) / 2.2f, shine, 12);
}

static void draw_scores(sink *k, const uint8_t *mem, double tau, int lang)
{
    float tt = (float)tau;
    float drop = 1.0f - ease_out(tt / 0.6f);
    float hy = 128.0f + 120.0f * drop;
    text(k, T_HIGH_SCORES[lang & 3], 0.0f, hy, 20.0f, RB_TEXT_CENTER, 15, RB_TAG_SCREEN_HEAD, 0.2f);
    float hw = rb_text_width(T_HIGH_SCORES[lang & 3], 20.0f) * 0.5f;
    k->n += rb_screens_cross(k->l + k->n, k->max - k->n, -hw - 26.0f, hy + 9.0f, 13.0f, tt * 0.8f, 12);
    k->n += rb_screens_cross(k->l + k->n, k->max - k->n, hw + 26.0f, hy + 9.0f, 13.0f, -tt * 0.8f, 12);
    seg(k, -150.0f, 112.0f, 150.0f, 112.0f, 8 * (drop < 0.5f), RB_TAG_LOGO_GOLD);
    for (int i = 0; i < 7; i++) {
        int at = RB_HSCRES + 5 * i;
        if (!(mem[at] | mem[at + 1])) break;
        float slide = 1.0f - ease_out((tt - 0.35f - 0.12f * i) / 0.5f);
        if (slide >= 1.0f) continue;
        float dx = 360.0f * slide, y = 84.0f - 26.0f * i;
        char rank[4], ini[4], sc[8];
        snprintf(rank, sizeof rank, "%d", i + 1);
        initials(ini, mem, at + 2);
        score_text(sc, mem[at], mem[at + 1]);
        int tag = RB_TAG_SCREEN_TEXT, b = 13;
        if (i == 0) {
            tag = fmodf(tt, 0.8f) < 0.4f ? RB_TAG_SCREEN_HI : RB_TAG_SCREEN_HEAD;
            b = 15;
        }
        text(k, rank, -128.0f + dx, y, 14.0f, RB_TEXT_RIGHT, b, tag, 0.0f);
        text(k, ini, -86.0f + dx, y, 14.0f, RB_TEXT_LEFT, b, tag, 0.0f);
        text(k, sc, 128.0f + dx, y, 14.0f, RB_TEXT_RIGHT, b, tag, 0.0f);
        /* a dotted line between */
        for (float x = -26.0f; x < 52.0f - 6.0f * (float)strlen(sc); x += 8.0f)
            seg(k, x + dx, y + 2.0f, x + dx + 1.5f, y + 2.0f, 6, RB_TAG_SCREEN_TEXT);
    }
}

static void draw_entry(sink *k, const uint8_t *mem, double tau, int lang)
{
    float tt = (float)tau;
    int head = fmodf(tt, 0.6f) < 0.3f ? RB_TAG_SCREEN_HEAD : RB_TAG_SCREEN_HI;
    float pop = 1.0f + 0.4f * (1.0f - ease_out(tt / 0.5f));
    text(k, T_NEW_HIGH[lang & 3], 0.0f, 118.0f, 22.0f * pop, RB_TEXT_CENTER, 15, head, 0.2f);
    char sc[8], line[48];
    score_text(sc, mem[RB_SCORE], mem[RB_SCORE + 1]);
    int place = mem[RB_LETR] / 5 + 1;
    snprintf(line, sizeof line, "%s %s", T_YOUR_SCORE[lang & 3], sc);
    text(k, line, 0.0f, 86.0f, 12.0f, RB_TEXT_CENTER, 13, RB_TAG_SCREEN_TEXT, 0.0f);
    snprintf(line, sizeof line, "%s %d", T_PLACE[lang & 3], place);
    text(k, line, 0.0f, 66.0f, 12.0f, RB_TEXT_CENTER, 13, RB_TAG_SCREEN_TEXT, 0.0f);

    /* the three letters in their boxes; the one being chosen bright, with
     * arrows above and below */
    int at = RB_INITL + mem[RB_LETR], cur = mem[RB_LETR + 1];
    const float bw = 44.0f, bh = 52.0f, gap = 14.0f, by = -10.0f;
    for (int i = 0; i < 3; i++) {
        float bx = (i - 1) * (bw + gap);
        int active = i == cur;
        int tag = active ? RB_TAG_SCREEN_HI : RB_TAG_SCREEN_TEXT;
        int b = active ? 15 : 9;
        seg(k, bx - bw / 2, by - bh / 2, bx + bw / 2, by - bh / 2, b, tag);
        seg(k, bx + bw / 2, by - bh / 2, bx + bw / 2, by + bh / 2, b, tag);
        seg(k, bx + bw / 2, by + bh / 2, bx - bw / 2, by + bh / 2, b, tag);
        seg(k, bx - bw / 2, by + bh / 2, bx - bw / 2, by - bh / 2, b, tag);
        int c = mem[at + i] & 0xfe;
        int blank = c == 0x4c || c >= 0x4e || c == 0x4a;
        if (!blank) {
            char s[2] = { rb_glyph_char(c), 0 };
            text(k, s, bx, by - 15.0f, 30.0f, RB_TEXT_CENTER, active ? 15 : 12, active ? RB_TAG_SCREEN_HEAD : RB_TAG_SCREEN_TEXT, 0.0f);
        } else if (active && fmodf(tt, 0.5f) < 0.25f) {
            seg(k, bx - 12.0f, by - 18.0f, bx + 12.0f, by - 18.0f, 15, RB_TAG_SCREEN_HEAD);
        }
        if (active) {
            float ay = by + bh / 2 + 8.0f, dy = 2.0f * sinf(tt * 8.0f);
            seg(k, bx - 9.0f, ay + dy, bx, ay + 8.0f + dy, 13, RB_TAG_SCREEN_HI);
            seg(k, bx, ay + 8.0f + dy, bx + 9.0f, ay + dy, 13, RB_TAG_SCREEN_HI);
            ay = by - bh / 2 - 8.0f;
            seg(k, bx - 9.0f, ay - dy, bx, ay - 8.0f - dy, 13, RB_TAG_SCREEN_HI);
            seg(k, bx, ay - 8.0f - dy, bx + 9.0f, ay - dy, 13, RB_TAG_SCREEN_HI);
        }
    }
    text(k, T_HINT1[lang & 3], 0.0f, -72.0f, 8.0f, RB_TEXT_CENTER, 11, RB_TAG_SCREEN_TEXT, 0.0f);
    text(k, T_HINT2[lang & 3], 0.0f, -88.0f, 8.0f, RB_TEXT_CENTER, 11, RB_TAG_SCREEN_TEXT, 0.0f);
    /* the time left (TIMER counts the frames down from 400 hex) */
    float left = clamp01((float)(mem[RB_TIMER] | (mem[RB_TIMER + 1] << 8)) / 1024.0f);
    seg(k, -100.0f, -104.0f, -100.0f + 200.0f * left, -104.0f, 10, left < 0.2f ? RB_TAG_SCREEN_HI : RB_TAG_LOGO_GOLD);
    seg(k, -100.0f, -100.0f, -100.0f, -108.0f, 6, RB_TAG_SCREEN_TEXT);
    seg(k, 100.0f, -100.0f, 100.0f, -108.0f, 6, RB_TAG_SCREEN_TEXT);
}

/* --- putting it together -------------------------------------------------------------- */

void rb_screens_init(rb_screens *s)
{
    memset(s, 0, sizeof *s);
    s->page = -1;
    s->elapsed = -1.0;
}

int rb_screens_page(const uint8_t *mem)
{
    uint8_t g = mem[RB_GAMODE];
    if ((g & 0xbf) == RB_BNRBIT + 3) return RB_PAGE_TITLE;
    if ((g & 0x0f) == 7) return RB_PAGE_ENTRY;
    if ((g & 0x0f) == 3) return RB_PAGE_SCORES;
    return RB_PAGE_GAME;
}

static int is_insert(const rb_line *l)
{
    return l->tag == RB_TAG_INSERT || l->tag == RB_TAG_ATTRACT;
}

/* The best score top right: its initials and score (HISCORE) and the words
 * HIGH SCORE above (the LABEL on the right; SCORE and VALUE are left of
 * the middle). The game writes them small; they are shown larger. */
#define HISCORE_SCALE 1.7f

static int is_hiscore(const rb_line *l)
{
    return l->tag == RB_TAG_HISCORE || (l->tag == RB_TAG_LABEL && l->x0 + l->x1 > 120.0f);
}

int rb_screens_apply(rb_screens *s, const uint8_t *mem, double t, const rb_screens_opt *o, const rb_line *in, int n,
                     rb_line *out, int max)
{
    int page = rb_screens_page(mem);
    if (page != s->page) {
        s->page = page;
        s->t0 = t;
    }
    double tau = s->elapsed >= 0.0 ? s->elapsed : t - s->t0;
    int ours = o->screens && rb_glyph_ready() && page != RB_PAGE_GAME;
    int attract = !(mem[RB_GAMODE] & 0x80) && page != RB_PAGE_ENTRY;
    int free_texts = o->free_play && attract && rb_glyph_ready();

    /* where INSERT COINS and what a coin buys are: on the port's pages they
     * go down below them */
    float top = -1e9f;
    for (int i = 0; i < n; i++)
        if (is_insert(&in[i])) top = fmaxf(top, fmaxf(in[i].y0, in[i].y1));
    float move = ours && top > -1e8f ? -118.0f - top : 0.0f;
    /* with the cockpit on the game's own pages, CREDITS goes up out of the
     * instrument panel's way, under what a coin buys */
    int up = o->cockpit && page == RB_PAGE_GAME;
    float ctop = -1e9f;
    if (up)
        for (int i = 0; i < n; i++)
            if (in[i].tag == RB_TAG_CREDITS) ctop = fmaxf(ctop, fmaxf(in[i].y0, in[i].y1));
    float cmove = up && ctop > -1e8f ? 62.0f - ctop : 0.0f;
    /* the best score, larger: about the middle of its top, kept on the screen */
    float hx0 = 1e9f, hx1 = -1e9f, htop = -1e9f;
    if (o->screens)
        for (int i = 0; i < n; i++)
            if (is_hiscore(&in[i])) {
                hx0 = fminf(hx0, fminf(in[i].x0, in[i].x1));
                hx1 = fmaxf(hx1, fmaxf(in[i].x0, in[i].x1));
                htop = fmaxf(htop, fmaxf(in[i].y0, in[i].y1));
            }
    int big = hx1 > hx0;
    float hcx = 0.5f * (hx0 + hx1), htop2 = fminf(htop, 194.0f);
    if (big && hcx + 0.5f * (hx1 - hx0) * HISCORE_SCALE > 252.0f) hcx = 252.0f - 0.5f * (hx1 - hx0) * HISCORE_SCALE;
    float hcx0 = 0.5f * (hx0 + hx1);

    sink k = { out, 0, max };
    for (int i = 0; i < n && k.n < max; i++) {
        const rb_line *l = &in[i];
        if (ours && (l->tag == RB_TAG_STATES || (page == RB_PAGE_TITLE && (l->tag == RB_TAG_PLANE || l->tag == RB_TAG_PROP))))
            continue;
        if (free_texts && (is_insert(l) || l->tag == RB_TAG_CREDITS)) continue;
        out[k.n] = *l;
        if (move != 0.0f && is_insert(l)) {
            out[k.n].y0 += move;
            out[k.n].y1 += move;
        }
        if (cmove != 0.0f && l->tag == RB_TAG_CREDITS) {
            out[k.n].y0 += cmove;
            out[k.n].y1 += cmove;
        }
        if (big && is_hiscore(l)) {
            rb_line *q = &out[k.n];
            q->x0 = hcx + (q->x0 - hcx0) * HISCORE_SCALE;
            q->x1 = hcx + (q->x1 - hcx0) * HISCORE_SCALE;
            q->y0 = htop2 + (q->y0 - htop) * HISCORE_SCALE;
            q->y1 = htop2 + (q->y1 - htop) * HISCORE_SCALE;
            q->win[0] = q->win[1] = -1e6f;      /* (out of the game's window, if it set one) */
            q->win[2] = q->win[3] = 1e6f;
        }
        k.n++;
    }
    if (ours) {
        switch (page) {
        case RB_PAGE_TITLE:  draw_title(&k, tau); break;
        case RB_PAGE_SCORES: draw_scores(&k, mem, tau, o->language); break;
        case RB_PAGE_ENTRY:  draw_entry(&k, mem, tau, o->language); break;
        }
    }
    if (free_texts) {
        /* FREE PLAY where the credits were, PRESS START blinking above it;
         * with the cockpit where INSERT COINS was */
        int lang = o->language & 3;
        float yf = up ? 80.0f : -160.0f, yp = up ? 98.0f : -138.0f;
        text(&k, T_FREE[lang], 0.0f, yf, 11.0f, RB_TEXT_CENTER, 13, RB_TAG_CREDITS, 0.0f);
        if (fmod(t, 1.2) < 0.8) text(&k, T_PRESS[lang], 0.0f, yp, 12.0f, RB_TEXT_CENTER, 15, RB_TAG_FREE, 0.0f);
    }
    return k.n;
}
