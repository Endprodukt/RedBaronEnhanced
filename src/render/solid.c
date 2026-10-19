#include "render/solid.h"

#include "core/rb_symbols.h"
#include "core/tags.h"
#include "render/deco.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PTS      48
#define MIN_AREA     1.0f       /* a face seen edge-on covers nothing (screen units squared) */
#define W_REL        0.004f     /* a line is behind a face if it is this much farther away ... */
#define XY_SLACK     0.75f      /* ... and more than this much of a screen unit's worth of the face's slope */
#define MIN_PIECE    0.2f       /* what is left of a line and shorter is dropped */
#define MAX_CUTS     64
#define MAX_INST     48
#define FACE_MAX     8

typedef struct face { uint8_t n; uint8_t pt[FACE_MAX]; } face;

/* The biplane, by its points in DB.PLN (RBARON.MAC, PLANE POINTS DB): 0
 * the tail's end, 1 2 the tailplane's tips, 3 the fin's top, 4-11 the nose
 * (an octagon), 12 13 the top wing's leading edge, 14 15 the lower wing's,
 * 16 the tail joint, 29 30 and 31 32 the wings' trailing edges (P.BACK). */
static const face PLANE_FACES[] = {
    { 4, { 12, 29, 30, 13 } },                  /* top wing */
    { 4, { 14, 31, 32, 15 } },                  /* lower wing */
    { 8, { 4, 5, 6, 7, 8, 9, 10, 11 } },        /* the nose */
    { 3, { 4, 5, 0 } }, { 3, { 5, 6, 0 } }, { 3, { 6, 7, 0 } }, { 3, { 7, 8, 0 } },   /* the fuselage */
    { 3, { 8, 9, 0 } }, { 3, { 9, 10, 0 } }, { 3, { 10, 11, 0 } }, { 3, { 11, 4, 0 } },
    { 3, { 0, 3, 16 } },                        /* the fin */
    { 3, { 16, 1, 2 } },                        /* the tailplane */
};
/* The objects on the ground, by their points (PFLOB in the vector ROM). */
static const face TENT_FACES[] = { { 3, { 0, 3, 2 } } };
static const face TREE_FACES[] = { { 3, { 2, 3, 4 } }, { 4, { 0, 1, 5, 6 } } };
static const face GUN_FACES[] = { { 4, { 0, 1, 2, 3 } }, { 4, { 4, 5, 6, 7 } } };
static const face BUNKER_FACES[] = { { 4, { 0, 1, 2, 3 } }, { 4, { 4, 5, 6, 7 } }, { 4, { 8, 9, 10, 11 } },
                                     { 4, { 15, 14, 13, 12 } } };

typedef struct inst {
    uint32_t key;               /* the drawing (rb_line.inst) */
    unsigned model;
    uint64_t have;              /* points whose place is known */
    float    x[MAX_PTS], y[MAX_PTS], w[MAX_PTS];
} inst;

void rb_solid_free(rb_solid *s)
{
    free(s->polys);
    free(s->out);
    memset(s, 0, sizeof *s);
}

/* What may be covered, and so cut. */
static int world(const rb_line *l)
{
    switch (l->tag) {
    case RB_TAG_PLANE: case RB_TAG_LAND: case RB_TAG_HORIZON: case RB_TAG_MOUNTAIN: case RB_TAG_GROUND:
    case RB_TAG_GSHELL: case RB_TAG_DECO:
        return 1;
    default:
        return 0;
    }
}

static void note_point(inst *in, unsigned p, float x, float y, float w)
{
    if (!p || p > MAX_PTS || w <= 0.0f) return;
    p--;
    in->have |= 1ull << p;
    in->x[p] = x; in->y[p] = y; in->w[p] = w;
}

static rb_solid_poly *new_poly(rb_solid *s)
{
    if (s->n_polys == s->cap_polys) {
        int cap = s->cap_polys ? s->cap_polys * 2 : 64;
        rb_solid_poly *q = (rb_solid_poly *)realloc(s->polys, (size_t)cap * sizeof *q);
        if (!q) return NULL;
        s->polys = q;
        s->cap_polys = cap;
    }
    rb_solid_poly *p = &s->polys[s->n_polys];
    memset(p, 0, sizeof *p);
    p->x0 = p->y0 = 1e30f;
    p->x1 = p->y1 = -1e30f;
    return p;
}

/* The poly's box, size and plane of 1 / distance; 0 if it covers nothing. */
static int finish_poly(rb_solid_poly *p, const float *w)
{
    double mx = 0, my = 0, mw = 0, area = 0;
    for (int i = 0; i < p->n; i++) {
        if (p->x[i] < p->x0) p->x0 = p->x[i];
        if (p->x[i] > p->x1) p->x1 = p->x[i];
        if (p->y[i] < p->y0) p->y0 = p->y[i];
        if (p->y[i] > p->y1) p->y1 = p->y[i];
        if (w[i] > p->wmax) p->wmax = w[i];
        mx += p->x[i]; my += p->y[i]; mw += w[i];
        int j = (i + 1) % p->n;
        area += (double)p->x[i] * p->y[j] - (double)p->x[j] * p->y[i];
    }
    if (fabs(area) * 0.5 < MIN_AREA) return 0;
    /* 1 / distance is straight across the face: the plane through its
     * corners, by least squares (flat across for a thing all at one distance) */
    mx /= p->n; my /= p->n; mw /= p->n;
    double sxx = 0, sxy = 0, syy = 0, sxw = 0, syw = 0;
    for (int i = 0; i < p->n; i++) {
        double dx = p->x[i] - mx, dy = p->y[i] - my, dw = w[i] - mw;
        sxx += dx * dx; sxy += dx * dy; syy += dy * dy; sxw += dx * dw; syw += dy * dw;
    }
    double det = sxx * syy - sxy * sxy;
    if (fabs(det) < 1e-6) return 0;
    p->a = (float)((sxw * syy - syw * sxy) / det);
    p->b = (float)((syw * sxx - sxw * sxy) / det);
    p->c = (float)(mw - p->a * mx - p->b * my);
    return 1;
}

static void add_face(rb_solid *s, const inst *in, const face *fc)
{
    float w[FACE_MAX];
    rb_solid_poly *p = new_poly(s);
    if (!p) return;
    p->n = fc->n;
    p->inst = in->key;
    for (int i = 0; i < p->n; i++) {
        unsigned k = fc->pt[i];
        if (!(in->have >> k & 1)) return;       /* (a point not drawn to: nothing known of the face) */
        p->points |= 1ull << k;
        p->x[i] = in->x[k]; p->y[i] = in->y[k]; w[i] = in->w[k];
    }
    if (finish_poly(p, w)) s->n_polys++;
}

/* A mountain: the land under its ridge. Its points' own places (SSEGS:
 * SCAPE0-3, PFPNTS: x/2, y*2) give the shape; the points drawn give where
 * it is on the screen, which for a flat thing is an affine map of them. */
static void add_mountain(rb_solid *s, const uint8_t *mem, const inst *in, const rb_line *lines, int n)
{
    unsigned seg = (in->model - RB_OBJ_MOUNTAIN_SEG) & 3;
    unsigned at = (unsigned)(mem[RB_SSEGS + 2 * seg] | (mem[RB_SSEGS + 2 * seg + 1] << 8));
    int count = mem[RB__SSEG + seg] / 2 + 1;
    if (count > MAX_PTS) count = MAX_PTS;
    float lx[MAX_PTS], ly[MAX_PTS];
    for (int i = 0; i < count; i++) {
        lx[i] = (float)(int8_t)mem[(at + 2 * i) & 0x7fff];
        ly[i] = (float)(int8_t)mem[(at + 2 * i + 1) & 0x7fff];
    }
    /* the affine map local -> screen, by least squares over the points drawn */
    double sum[3][3] = { { 0 } }, bx[3] = { 0 }, by[3] = { 0 };
    int known = 0;
    float wsum = 0;
    for (int i = 0; i < count; i++) {
        if (!(in->have >> i & 1)) continue;
        double v[3] = { lx[i], ly[i], 1.0 };
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) sum[r][c] += v[r] * v[c];
            bx[r] += v[r] * in->x[i];
            by[r] += v[r] * in->y[i];
        }
        wsum += in->w[i];
        known++;
    }
    if (known < 3) return;
    /* solve the 3 x 3 normal equations (Cramer) */
    double a = sum[0][0], b = sum[0][1], c = sum[0][2], d = sum[1][0], e = sum[1][1], f = sum[1][2],
           g = sum[2][0], h = sum[2][1], k = sum[2][2];
    double det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
    if (fabs(det) < 1e-9) return;
    double mx[3], my[3];
    for (int axis = 0; axis < 2; axis++) {
        double *r = axis ? by : bx, *o = axis ? my : mx;
        o[0] = (r[0] * (e * k - f * h) - b * (r[1] * k - f * r[2]) + c * (r[1] * h - e * r[2])) / det;
        o[1] = (a * (r[1] * k - f * r[2]) - r[0] * (d * k - f * g) + c * (d * r[2] - r[1] * g)) / det;
        o[2] = (a * (e * r[2] - r[1] * h) - b * (d * r[2] - r[1] * g) + r[0] * (d * h - e * g)) / det;
    }
    /* the ridge: the highest of the lines drawn at every x (in the
     * mountain's own frame), from the x of every end and every crossing */
    float sx[MAX_PTS * 2], sy[MAX_PTS * 2], ex[MAX_PTS * 2], ey[MAX_PTS * 2];
    int ns = 0;
    for (int i = 0; i < n && ns < MAX_PTS * 2; i++) {
        const rb_line *l = &lines[i];
        if (l->inst != in->key || !l->p1 || l->p1 > (unsigned)count) continue;
        unsigned p0 = l->p0;
        if (!p0) {
            /* (the first line of a part begins where the part before ended,
             * where the beam was put without a note: the point that is there) */
            float best = 1.5f;
            for (int k = 0; k < count; k++)
                if ((in->have >> k & 1) && fabsf(in->x[k] - l->x0) + fabsf(in->y[k] - l->y0) < best) {
                    best = fabsf(in->x[k] - l->x0) + fabsf(in->y[k] - l->y0);
                    p0 = (unsigned)k + 1;
                }
            if (!p0) continue;
        }
        if (p0 > (unsigned)count) continue;
        float x0 = lx[p0 - 1], y0 = ly[p0 - 1], x1 = lx[l->p1 - 1], y1 = ly[l->p1 - 1];
        if (x0 == x1) continue;
        if (x0 > x1) { float t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        sx[ns] = x0; sy[ns] = y0; ex[ns] = x1; ey[ns] = y1;
        ns++;
    }
    if (!ns) return;
    float xs[RB_SOLID_POLY_MAX * 4];
    int nx = 0;
    for (int i = 0; i < ns && nx + 2 < (int)(sizeof xs / sizeof xs[0]); i++) {
        xs[nx++] = sx[i];
        xs[nx++] = ex[i];
        for (int j = i + 1; j < ns && nx < (int)(sizeof xs / sizeof xs[0]); j++) {
            /* where two of them cross */
            float lo = sx[i] > sx[j] ? sx[i] : sx[j], hi = ex[i] < ex[j] ? ex[i] : ex[j];
            if (lo >= hi) continue;
            float ki = (ey[i] - sy[i]) / (ex[i] - sx[i]), kj = (ey[j] - sy[j]) / (ex[j] - sx[j]);
            if (fabsf(ki - kj) < 1e-6f) continue;
            float xc = (sy[j] - kj * sx[j] - sy[i] + ki * sx[i]) / (ki - kj);
            if (xc > lo && xc < hi) xs[nx++] = xc;
        }
    }
    /* sorted, without repeats */
    for (int i = 1; i < nx; i++)
        for (int j = i; j > 0 && xs[j - 1] > xs[j]; j--) { float t = xs[j]; xs[j] = xs[j - 1]; xs[j - 1] = t; }
    rb_solid_poly *p = new_poly(s);
    if (!p) return;
    float w[RB_SOLID_POLY_MAX];
    float wm = wsum / (float)known, last = -1e30f;
    int np = 0;
    for (int i = 0; i < nx && np < RB_SOLID_POLY_MAX - 2; i++) {
        if (xs[i] - last < 0.01f) continue;
        last = xs[i];
        float top = -1e30f;
        for (int j = 0; j < ns; j++)
            if (xs[i] >= sx[j] - 1e-4f && xs[i] <= ex[j] + 1e-4f) {
                float y = sy[j] + (ey[j] - sy[j]) * (xs[i] - sx[j]) / (ex[j] - sx[j]);
                if (y > top) top = y;
            }
        if (top < 0) top = 0;
        p->x[np] = (float)(mx[0] * xs[i] + mx[1] * top + mx[2]);
        p->y[np] = (float)(my[0] * xs[i] + my[1] * top + my[2]);
        w[np++] = wm;
    }
    if (np < 2) return;
    /* down to its foot */
    float xr = xs[nx - 1], xl = xs[0];
    p->x[np] = (float)(mx[0] * xr + mx[2]); p->y[np] = (float)(my[0] * xr + my[2]); w[np++] = wm;
    p->x[np] = (float)(mx[0] * xl + mx[2]); p->y[np] = (float)(my[0] * xl + my[2]); w[np++] = wm;
    p->n = np;
    p->inst = in->key;
    p->points = in->have;               /* (its own lines are its own: never cut by it) */
    if (finish_poly(p, w)) s->n_polys++;
}

static int inside(const rb_solid_poly *p, float x, float y)
{
    int in = 0;
    for (int i = 0, j = p->n - 1; i < p->n; j = i++)
        if ((p->y[i] > y) != (p->y[j] > y) &&
            x < (p->x[j] - p->x[i]) * (y - p->y[i]) / (p->y[j] - p->y[i]) + p->x[i])
            in = !in;
    return in;
}

static int float_cmp(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : x > y;
}

static void emit(rb_solid *s, const rb_line *l)
{
    if (s->n_out == s->cap_out) {
        int cap = s->cap_out ? s->cap_out * 2 : 512;
        rb_line *q = (rb_line *)realloc(s->out, (size_t)cap * sizeof *q);
        if (!q) return;
        s->out = q;
        s->cap_out = cap;
    }
    s->out[s->n_out++] = *l;
}

int rb_solid_front_poly(rb_solid_poly *p, const float *x, const float *y, int n)
{
    float w[RB_SOLID_POLY_MAX];
    memset(p, 0, sizeof *p);
    p->x0 = p->y0 = 1e30f;
    p->x1 = p->y1 = -1e30f;
    if (n > RB_SOLID_POLY_MAX) n = RB_SOLID_POLY_MAX;
    p->n = n;
    for (int i = 0; i < n; i++) {
        p->x[i] = x[i];
        p->y[i] = y[i];
        w[i] = 1.0f;
    }
    if (!finish_poly(p, w)) return 0;
    /* nearer than anything: 1 / distance far above any the game draws */
    p->a = p->b = 0.0f;
    p->c = p->wmax = 1e6f;
    return 1;
}

/* What the shapes in front cut: everything out there (the world, what flies
 * in it, the player's own propeller, as the bezel did), and the copyright
 * line; not the texts and gauges of the game, nor the bullet holes in the
 * windshield, nor the shells: the game's start below at the sides, where
 * the cockpit is, and are to be seen as on the cabinet. */
static int behind_front(const rb_line *l)
{
    switch (l->tag) {
    case RB_TAG_COPYRIGHT: case RB_TAG_COCKPIT: case RB_TAG_PROP: case RB_TAG_DRONE: case RB_TAG_EXPLOSION: case RB_TAG_BLIMP:
    case RB_TAG_BLAST: case RB_TAG_FIRE: case RB_TAG_EMBER: case RB_TAG_ROUNDEL_BLUE: case RB_TAG_ROUNDEL_RED:
        return 1;
    default:
        return world(l);
    }
}

/* The parts of a line that a face covers, as stretches of 0..1 along it,
 * added to cut[]. */
static int cover(const rb_solid_poly *p, const rb_line *l, float (*cut)[2], int n)
{
    float lx0 = l->x0 < l->x1 ? l->x0 : l->x1, lx1 = l->x0 < l->x1 ? l->x1 : l->x0;
    float ly0 = l->y0 < l->y1 ? l->y0 : l->y1, ly1 = l->y0 < l->y1 ? l->y1 : l->y0;
    if (lx1 < p->x0 || lx0 > p->x1 || ly1 < p->y0 || ly0 > p->y1) return n;
    if (l->w0 > p->wmax * 1.02f && l->w1 > p->wmax * 1.02f) return n;      /* all of it nearer */

    /* A line of the same drawing: the face's own edges show; one that
     * leaves a corner of the face is before it or behind it as a whole. */
    int own = l->inst && l->inst == p->inst;
    int on0 = own && l->p0 && (p->points >> (l->p0 - 1) & 1);
    int on1 = own && l->p1 && (p->points >> (l->p1 - 1) & 1);
    if (on0 && on1) return n;
    if (own && (!l->p0 || !l->p1)) return n;    /* (an end not noted: not known where it is) */

    float dx = l->x1 - l->x0, dy = l->y1 - l->y0;
    float ts[RB_SOLID_POLY_MAX + 2];
    int nt = 0;
    ts[nt++] = 0.0f;
    for (int i = 0, j = p->n - 1; i < p->n && nt < RB_SOLID_POLY_MAX + 1; j = i++) {
        float ex = p->x[i] - p->x[j], ey = p->y[i] - p->y[j];
        float den = dx * ey - dy * ex;
        if (fabsf(den) < 1e-9f) continue;
        float t = ((p->x[j] - l->x0) * ey - (p->y[j] - l->y0) * ex) / den;
        float u = ((p->x[j] - l->x0) * dy - (p->y[j] - l->y0) * dx) / den;
        if (t > 0.0f && t < 1.0f && u >= 0.0f && u <= 1.0f) ts[nt++] = t;
    }
    ts[nt++] = 1.0f;
    qsort(ts, (size_t)nt, sizeof ts[0], float_cmp);

    float slack = W_REL * p->wmax + XY_SLACK * (fabsf(p->a) + fabsf(p->b));
    for (int i = 0; i + 1 < nt && n < MAX_CUTS; i++) {
        float ta = ts[i], tb = ts[i + 1];
        if (tb - ta < 1e-5f) continue;
        float tm = 0.5f * (ta + tb);
        if (!inside(p, l->x0 + dx * tm, l->y0 + dy * tm)) continue;
        /* how much farther than the face the line is at each end of the
         * stretch (above 0: behind); the horizon is infinitely far */
        float da = p->a * (l->x0 + dx * ta) + p->b * (l->y0 + dy * ta) + p->c - (l->w0 + (l->w1 - l->w0) * ta);
        float db = p->a * (l->x0 + dx * tb) + p->b * (l->y0 + dy * tb) + p->c - (l->w0 + (l->w1 - l->w0) * tb);
        if (da <= slack && db <= slack) continue;
        if (!(on0 || on1) && (da <= slack || db <= slack)) {
            /* goes through the face's plane: behind it from there on */
            float tc = ta + (tb - ta) * (slack - da) / (db - da);
            if (da <= slack) ta = tc;
            else tb = tc;
        }
        cut[n][0] = ta;
        cut[n][1] = tb;
        n++;
    }
    return n;
}

static int cut_cmp(const void *a, const void *b)
{
    float x = ((const float *)a)[0], y = ((const float *)b)[0];
    return x < y ? -1 : x > y;
}

static void faces_of(rb_solid *s, const uint8_t *mem, const inst *in, const rb_line *lines, int n)
{
    const face *f = NULL;
    int nf = 0;
    switch (in->model) {
    case RB_OBJ_PLANE:  f = PLANE_FACES; nf = (int)(sizeof PLANE_FACES / sizeof PLANE_FACES[0]); break;
    case RB_OBJ_TENT:   f = TENT_FACES; nf = 1; break;
    case RB_OBJ_TREE:   f = TREE_FACES; nf = 2; break;
    case RB_OBJ_GUN:    f = GUN_FACES; nf = 2; break;
    case RB_OBJ_BUNKER: f = BUNKER_FACES; nf = 4; break;
    default:
        if (in->model >= RB_OBJ_MOUNTAIN_SEG && in->model < RB_OBJ_MOUNTAIN_SEG + 4) {
            add_mountain(s, mem, in, lines, n);
            return;
        }
        {
            /* the additions: their outlines */
            const rb_deco_shape *sh = rb_deco_shape_of(in->model);
            if (!sh) return;
            for (int i = 0; i < sh->n_faces; i++) {
                face fc;
                fc.n = sh->faces[i].n;
                memcpy(fc.pt, sh->faces[i].pt, sizeof fc.pt);
                add_face(s, in, &fc);
            }
        }
        return;
    }
    for (int i = 0; i < nf; i++) add_face(s, in, &f[i]);
}

int rb_solid_run(rb_solid *s, const uint8_t *mem, const rb_line *in, int n, const rb_line **out)
{
    static inst insts[MAX_INST];
    int n_inst = 0;
    s->n_polys = 0;
    s->n_out = 0;
    s->cut = 0;

    /* The drawings of objects in the picture: where each of their points is. */
    for (int i = 0; i < n; i++) {
        const rb_line *l = &in[i];
        if (!l->inst || !l->model || (!l->p0 && !l->p1) || l->model == RB_OBJ_PIECES) continue;
        int k = 0;
        while (k < n_inst && insts[k].key != l->inst) k++;
        if (k == n_inst) {
            if (n_inst == MAX_INST) continue;
            insts[k].key = l->inst;
            insts[k].model = l->model;
            insts[k].have = 0;
            n_inst++;
        }
        note_point(&insts[k], l->p0, l->x0, l->y0, l->w0);
        note_point(&insts[k], l->p1, l->x1, l->y1, l->w1);
    }
    if (!s->no_faces)
        for (int k = 0; k < n_inst; k++) faces_of(s, mem, &insts[k], in, n);

    for (int i = 0; i < n; i++) {
        const rb_line *l = &in[i];
        int by_faces = s->n_polys && world(l), by_front = s->n_front && behind_front(l);
        if (!by_faces && !by_front) { emit(s, l); continue; }
        float cut[MAX_CUTS][2];
        int nc = 0;
        if (by_faces)
            for (int k = 0; k < s->n_polys && nc < MAX_CUTS; k++) nc = cover(&s->polys[k], l, cut, nc);
        if (by_front)
            for (int k = 0; k < s->n_front && nc < MAX_CUTS; k++) nc = cover(&s->front[k], l, cut, nc);
        if (!nc) { emit(s, l); continue; }
        s->cut++;
        qsort(cut, (size_t)nc, sizeof cut[0], cut_cmp);
        float len = hypotf(l->x1 - l->x0, l->y1 - l->y0);
        if (len <= 0.0f) continue;              /* a dot under a face */
        float at = 0.0f;
        for (int k = 0; k <= nc; k++) {
            float to = k < nc ? cut[k][0] : 1.0f;
            if (to - at > 0.0f && (to - at) * len >= MIN_PIECE) {
                rb_line piece = *l;
                piece.x0 = l->x0 + (l->x1 - l->x0) * at; piece.y0 = l->y0 + (l->y1 - l->y0) * at;
                piece.x1 = l->x0 + (l->x1 - l->x0) * to; piece.y1 = l->y0 + (l->y1 - l->y0) * to;
                piece.w0 = l->w0 + (l->w1 - l->w0) * at;
                piece.w1 = l->w0 + (l->w1 - l->w0) * to;
                emit(s, &piece);
            }
            if (k < nc && cut[k][1] > at) at = cut[k][1];
        }
    }
    *out = s->out;
    return s->n_out;
}
