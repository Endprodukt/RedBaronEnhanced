/* glyph.c - see glyph.h. */
#include "render/glyph.h"

#include "core/rb_symbols.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define N_CODES   40                /* VGMSGA: 0x50 bytes, two a character */
#define MAX_SEGS  24                /* lines of one character at most */

typedef struct glyph {
    int   n;
    float x0[MAX_SEGS], y0[MAX_SEGS], x1[MAX_SEGS], y1[MAX_SEGS];
    float advance;
    float xmin, xmax;               /* where its lines are, across */
} glyph;

static glyph g_glyph[N_CODES];
static float g_cap = 1.0f;          /* the height of a capital as drawn */
static int   g_ready;

static void wr16(uint8_t *mem, unsigned a, unsigned v)
{
    mem[a] = (uint8_t)v;
    mem[a + 1] = (uint8_t)(v >> 8);
}

void rb_glyph_init(const uint8_t *rom_mem)
{
    static uint8_t mem[0x8000];
    static rb_avg_list l;
    memcpy(mem, rom_mem, sizeof mem);
    float cap = 0.0f;
    for (int c = 0; c < N_CODES; c++) {
        glyph *g = &g_glyph[c];
        memset(g, 0, sizeof *g);
        /* CNTR, STAT (full brightness), SCAL (full size), the character's
         * JSRL, then a short line to mark where the beam ended, HALT */
        unsigned a = RB_VGRAM;
        wr16(mem, a, 0x8040); a += 2;
        wr16(mem, a, 0x64f0); a += 2;
        wr16(mem, a, 0x7000); a += 2;
        wr16(mem, a, (unsigned)(mem[RB_VGMSGA + 2 * c] | (mem[RB_VGMSGA + 2 * c + 1] << 8))); a += 2;
        wr16(mem, a, 0x0000); a += 2;
        wr16(mem, a, 0xe000 | 1); a += 2;   /* z 7, one unit to the right */
        wr16(mem, a, 0x2000);
        rb_avg_run(mem, RB_VGRAM, &l, 4000);
        if (l.count < 1) continue;
        const rb_line *mark = &l.lines[l.count - 1];
        g->advance = mark->x0;
        g->xmin = 1e9f;
        g->xmax = -1e9f;
        for (int i = 0; i < l.count - 1 && g->n < MAX_SEGS; i++) {
            const rb_line *q = &l.lines[i];
            g->x0[g->n] = q->x0; g->y0[g->n] = q->y0;
            g->x1[g->n] = q->x1; g->y1[g->n] = q->y1;
            if (q->y0 > cap) cap = q->y0;
            if (q->y1 > cap) cap = q->y1;
            g->xmin = fminf(g->xmin, fminf(q->x0, q->x1));
            g->xmax = fmaxf(g->xmax, fmaxf(q->x0, q->x1));
            g->n++;
        }
        if (!g->n) {                    /* (a blank: as wide as it moves) */
            g->xmin = 0.0f;
            g->xmax = g->advance;
        }
    }
    g_cap = cap > 0.0f ? cap : 1.0f;
    g_ready = 1;
}

int rb_glyph_ready(void)
{
    return g_ready;
}

int rb_glyph_code(char c)
{
    if (c >= '0' && c <= '9') return 2 + 2 * (c - '0');
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return 0x16 + 2 * (c - 'A');
    if (c == '_') return 0x4c;
    if (c == '-') return 0x4a;
    if (c == '.') return 0x4e;
    return 0;
}

char rb_glyph_char(int code)
{
    if (code >= 2 && code <= 0x14) return (char)('0' + (code - 2) / 2);
    if (code >= 0x16 && code <= 0x48) return (char)('A' + (code - 0x16) / 2);
    if (code == 0x4c) return '_';
    if (code == 0x4a) return '-';
    return ' ';
}

static const glyph *glyph_of(int code)
{
    int i = (code / 2) % N_CODES;
    return &g_glyph[i < 0 ? 0 : i];
}

float rb_text_width(const char *s, float size)
{
    float k = size / g_cap, w = 0.0f;
    for (; *s; s++) w += glyph_of(rb_glyph_code(*s))->advance * k;
    return w;
}

int rb_glyph(rb_line *out, int max, int code, float x, float y, float sx, float sy, uint8_t intensity, uint16_t tag,
             float slant)
{
    const glyph *g = glyph_of(code);
    float kx = sx / g_cap, ky = sy / g_cap;
    int n = 0;
    for (int i = 0; i < g->n && n < max; i++) {
        rb_line *q = &out[n++];
        memset(q, 0, sizeof *q);
        q->y0 = y + g->y0[i] * ky;
        q->y1 = y + g->y1[i] * ky;
        q->x0 = x + g->x0[i] * kx + slant * (q->y0 - y);
        q->x1 = x + g->x1[i] * kx + slant * (q->y1 - y);
        q->intensity = intensity;
        q->tag = tag;
        q->win[0] = q->win[1] = -1e6f;         /* (no window) */
        q->win[2] = q->win[3] = 1e6f;
    }
    return n;
}

int rb_text(rb_line *out, int max, const char *s, float x, float y, float size, int align, uint8_t intensity,
            uint16_t tag, float slant)
{
    if (!g_ready || !*s) return 0;
    float k = size / g_cap;
    if (align != RB_TEXT_LEFT) {
        /* by what shows: from the first character's left edge to the last's
         * right edge (the advance leaves room after a character) */
        size_t len = strlen(s);
        const glyph *first = glyph_of(rb_glyph_code(s[0])), *last = glyph_of(rb_glyph_code(s[len - 1]));
        float lead = first->xmin * k;
        float w = rb_text_width(s, size) - last->advance * k + last->xmax * k - lead;
        x -= lead + (align == RB_TEXT_CENTER ? w * 0.5f : w);
    }
    int n = 0;
    for (; *s && n < max; s++) {
        int code = rb_glyph_code(*s);
        n += rb_glyph(out + n, max - n, code, x, y, size, size, intensity, tag, slant);
        x += glyph_of(code)->advance * k;
    }
    return n;
}
