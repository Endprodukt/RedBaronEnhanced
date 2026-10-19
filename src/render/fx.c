/* fx.c - see fx.h. */
#include "render/fx.h"

#include "core/tags.h"

#include <math.h>
#include <string.h>

#define PI       3.14159265f
#define LIFE     0.7f       /* seconds */
#define SPARKS   8

void rb_fx_init(rb_fx *f)
{
    memset(f, 0, sizeof *f);
    for (int i = 0; i < RB_FX_MAX; i++) f->blast[i].t0 = -1.0;
}

static int piece(const rb_line *l)
{
    return l->tag == RB_TAG_EXPLOSION || (l->tag == RB_TAG_PLANE && l->model == RB_OBJ_PIECES);
}

static void seg(rb_line *out, int *n, int max, float x0, float y0, float x1, float y1, int b, int tag)
{
    if (*n >= max || b <= 0) return;
    rb_line *q = &out[(*n)++];
    memset(q, 0, sizeof *q);
    q->x0 = x0; q->y0 = y0; q->x1 = x1; q->y1 = y1;
    q->intensity = (uint8_t)(b > 15 ? 15 : b);
    q->tag = (uint16_t)tag;
    q->win[0] = q->win[1] = -1e6f;
    q->win[2] = q->win[3] = 1e6f;
}

static float rnd(unsigned *s)
{
    *s = *s * 1103515245u + 12345u;
    return (float)((*s >> 8) & 0xffff) / 65535.0f;
}

/* The top wing's corners of the biplane drawing `inst`: 12, 13 the leading
 * edge (left, right), 29, 30 the trailing edge (DB.PLN; the wing 80 wide,
 * 32 deep). Returns 1 if all four are known. */
static int wing_corners(const rb_line *in, int n, uint16_t inst, float (*c)[2])
{
    static const uint8_t PT[4] = { 12, 13, 29, 30 };
    int have = 0;
    for (int i = 0; i < n && have != 15; i++) {
        const rb_line *l = &in[i];
        if (l->inst != inst || l->model != RB_OBJ_PLANE) continue;
        for (int k = 0; k < 4; k++) {
            if (l->p0 == PT[k] + 1) { c[k][0] = l->x0; c[k][1] = l->y0; have |= 1 << k; }
            if (l->p1 == PT[k] + 1) { c[k][0] = l->x1; c[k][1] = l->y1; have |= 1 << k; }
        }
    }
    return have == 15;
}

int rb_fx_roundels(const rb_line *in, int n, rb_line *out, int max)
{
    uint16_t done[8];
    int nd = 0, m = 0;
    for (int i = 0; i < n && nd < 8; i++) {
        const rb_line *l = &in[i];
        if (l->model != RB_OBJ_PLANE || !l->inst || l->tag != RB_TAG_PLANE) continue;
        int seen = 0;
        for (int k = 0; k < nd; k++) seen |= done[k] == l->inst;
        if (seen) continue;
        done[nd++] = l->inst;
        float c[4][2] = { { 0 } };
        if (!wing_corners(in, n, l->inst, c)) continue;
        /* a point of the wing: u across its span (0 left .. 1 right), v
         * across its chord (0 leading .. 1 trailing) */
        for (int side = 0; side < 2; side++) {
            float u0 = side ? 0.8f : 0.2f, v0 = 0.5f;
            for (int ring = 0; ring < 2; ring++) {
                /* (radius 8 of the model's units: a tenth of the span, a quarter of the chord) */
                float ru = ring ? 0.045f : 0.10f, rv = ring ? 0.11f : 0.25f;
                int segs = ring ? 5 : 8, tag = ring ? RB_TAG_ROUNDEL_RED : RB_TAG_ROUNDEL_BLUE;
                float px = 0, py = 0;
                for (int s = 0; s <= segs; s++) {
                    float a = 2.0f * PI * s / segs;
                    float u = u0 + ru * cosf(a), v = v0 + rv * sinf(a);
                    float lx = c[0][0] + u * (c[1][0] - c[0][0]), ly = c[0][1] + u * (c[1][1] - c[0][1]);
                    float tx = c[2][0] + u * (c[3][0] - c[2][0]), ty = c[2][1] + u * (c[3][1] - c[2][1]);
                    float x = lx + v * (tx - lx), y = ly + v * (ty - ly);
                    if (s) seg(out, &m, max, px, py, x, y, 12, tag);
                    px = x; py = y;
                }
            }
        }
    }
    return m;
}

int rb_fx_lines(rb_fx *f, const rb_line *in, int n, double t, rb_line *out, int max)
{
    /* the pieces in this picture: where they are */
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    int pieces = 0;
    for (int i = 0; i < n; i++)
        if (piece(&in[i])) {
            x0 = fminf(x0, fminf(in[i].x0, in[i].x1)); x1 = fmaxf(x1, fmaxf(in[i].x0, in[i].x1));
            y0 = fminf(y0, fminf(in[i].y0, in[i].y1)); y1 = fmaxf(y1, fmaxf(in[i].y0, in[i].y1));
            pieces++;
        }
    /* pieces where there were none: a new explosion */
    if (pieces && !f->had) {
        int k = 0;
        for (int i = 1; i < RB_FX_MAX; i++)
            if (f->blast[i].t0 < f->blast[k].t0) k = i;
        rb_fx_blast *b = &f->blast[k];
        b->t0 = t;
        b->x = 0.5f * (x0 + x1);
        b->y = 0.5f * (y0 + y1);
        b->size = fmaxf(12.0f, fminf(90.0f, 0.5f * hypotf(x1 - x0, y1 - y0)));
        b->seed = (unsigned)(t * 1000.0) * 2654435761u + 1u;
    }
    f->had = pieces > 0;

    int m = 0;
    for (int k = 0; k < RB_FX_MAX; k++) {
        rb_fx_blast *b = &f->blast[k];
        if (b->t0 < 0.0) continue;
        float a = (float)(t - b->t0);
        if (a < 0.0f || a > LIFE) {
            if (a > LIFE) b->t0 = -1.0;
            continue;
        }
        float u = a / LIFE;
        /* the colour cools: white, yellow, orange, red; and fades */
        int tag = u < 0.15f ? RB_TAG_BLAST : u < 0.45f ? RB_TAG_FIRE : RB_TAG_EMBER;
        int bright = (int)(15.0f * (1.0f - u * u));
        /* the fireball: a ring, growing, a little ragged */
        float r = b->size * (0.25f + 1.1f * sqrtf(u));
        unsigned s = b->seed;
        const int N = 12;
        float px = 0, py = 0;
        for (int i = 0; i <= N; i++) {
            float ang = 2.0f * PI * i / N;
            float rr = r * (0.8f + 0.35f * rnd(&s));
            if (i == N) { s = b->seed; rr = r * (0.8f + 0.35f * rnd(&s)); }
            float x = b->x + rr * cosf(ang), y = b->y + rr * sinf(ang) * 0.85f;
            if (i) seg(out, &m, max, px, py, x, y, bright, tag);
            px = x; py = y;
        }
        /* the first moment: a flash cross */
        if (u < 0.12f) {
            float c = b->size * 0.9f;
            seg(out, &m, max, b->x - c, b->y, b->x + c, b->y, 15, RB_TAG_BLAST);
            seg(out, &m, max, b->x, b->y - c, b->x, b->y + c, 15, RB_TAG_BLAST);
        }
        /* sparks: out and down, each a short streak */
        s = b->seed ^ 0x5bd1e995u;
        for (int i = 0; i < SPARKS; i++) {
            float ang = 2.0f * PI * (i + rnd(&s) * 0.8f) / SPARKS;
            float speed = b->size * (1.4f + 1.6f * rnd(&s));
            float d = speed * a, fall = 90.0f * a * a;
            float sx = b->x + d * cosf(ang), sy = b->y + d * sinf(ang) - fall;
            float len = 0.12f * speed * (1.0f - u);
            float ex = sx - len * cosf(ang), ey = sy - len * sinf(ang) + 0.3f * fall * 0.2f;
            seg(out, &m, max, ex, ey, sx, sy, bright, u < 0.3f ? RB_TAG_BLAST : RB_TAG_FIRE);
        }
    }
    return m;
}
