/* cockpit.c - see cockpit.h. */
#include "render/cockpit.h"

#include "core/rb_symbols.h"
#include "core/tags.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265f

/* The pilot's view: x right, y up, z ahead, the eye at 0. A point is seen
 * at F * x / z, F * y / z on the screen (-260..260 by -200..200), the
 * vanishing point in the middle, where the game's horizon is when flying
 * level. */
#define F 300.0f

/* The instrument panel (flat, the nearest thing but the guns): its top
 * edge from PANEL_TOP in the middle a little down to its sides at
 * +-PANEL_X, down to the screen's bottom. */
#define PANEL_TOP  (-118.0f)
#define PANEL_SAG  6.0f
#define PANEL_X    175.0f

/* The guns (Spandau LMG 08/15, two, on the fuselage right in front of the
 * pilot, close to the middle, their backs on the instrument panel, as in a
 * Fokker Dr.I): the axis of the left one, the receiver's box, the cooling
 * jacket, the muzzle (in the units of the eye's distance). The right one
 * is its mirror. */
#define GUN_X      (-0.17f)
#define GUN_Y      (-0.15f)
#define RECV_Z0    0.45f
#define RECV_Z1    0.70f
#define RECV_W     0.045f       /* half its width */
#define RECV_DOWN  0.050f
#define RECV_UP    0.045f
#define JACKET_Z1  1.12f
#define JACKET_R   0.030f
#define MUZZLE_Z1  1.22f
#define MUZZLE_R   0.016f
#define KICK       0.030f       /* how far a shot throws it back */

/* The ring sights on the jackets: their size, how high above. */
#define RING_R     0.024f
#define RING_UP    0.022f

/* The nose (the engine's cowling, round, the fuselage's width) under the
 * guns, and the propeller's disc in front of it. */
#define NOSE_R     0.45f        /* as wide as the fuselage: the guns lie on its top */
#define NOSE_Y     (GUN_Y - RECV_DOWN - NOSE_R)
#define NOSE_Z0    0.80f
#define NOSE_Z1    1.05f
#define PROP_Z     1.12f
#define PROP_R     0.75f

/* The middle wing at the sides, below the eye, from the fuselage out. */
#define WING_Y     (-0.345f)
#define WING_X0    0.52f
#define WING_Z0    0.55f
#define WING_Z1    1.45f

/* The struts from the middle wing to the upper one, two each side,
 * leaning out (a V seen from the seat). */
#define STRUT_W    0.011f

typedef struct sink { rb_line *l; int n, max; } sink;

static void seg(sink *k, float x0, float y0, float x1, float y1, int intensity, int tag)
{
    if (k->n >= k->max || intensity <= 0) return;
    rb_line *q = &k->l[k->n++];
    memset(q, 0, sizeof *q);
    q->x0 = x0; q->y0 = y0; q->x1 = x1; q->y1 = y1;
    q->intensity = (uint8_t)(intensity > 15 ? 15 : intensity);
    q->tag = (uint16_t)tag;
    q->win[0] = q->win[1] = -1e6f;             /* (no window) */
    q->win[2] = q->win[3] = 1e6f;
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

static void project(float x, float y, float z, float *sx, float *sy)
{
    *sx = F * x / z;
    *sy = F * y / z;
}

static float dash_y(float x)
{
    float u = x / PANEL_X;
    return PANEL_TOP - PANEL_SAG * u * u;
}

/* --- convex shapes: the hull of points, and a line cut where one covers it ---------- */

typedef rb_cockpit_hull hull;

static int pt_cmp(const void *a, const void *b)
{
    const float *p = (const float *)a, *q = (const float *)b;
    if (p[0] != q[0]) return p[0] < q[0] ? -1 : 1;
    return p[1] < q[1] ? -1 : p[1] > q[1];
}

static float cross3(const float *o, const float *a, const float *b)
{
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
}

/* The convex hull of n points (xy pairs), counter-clockwise (Andrew). */
static void make_hull(hull *h, float (*p)[2], int n)
{
    qsort(p, (size_t)n, sizeof p[0], pt_cmp);
    float out[2 * RB_SOLID_POLY_MAX][2];
    int k = 0;
    for (int i = 0; i < n; i++) {
        while (k >= 2 && cross3(out[k - 2], out[k - 1], p[i]) <= 0.0f) k--;
        out[k][0] = p[i][0]; out[k][1] = p[i][1]; k++;
    }
    for (int i = n - 2, t = k + 1; i >= 0; i--) {
        while (k >= t && cross3(out[k - 2], out[k - 1], p[i]) <= 0.0f) k--;
        out[k][0] = p[i][0]; out[k][1] = p[i][1]; k++;
    }
    k--;
    if (k > RB_SOLID_POLY_MAX) k = RB_SOLID_POLY_MAX;
    h->n = k;
    for (int i = 0; i < k; i++) { h->x[i] = out[i][0]; h->y[i] = out[i][1]; }
}

/* The stretch of a line (0..1) inside a convex, counter-clockwise shape
 * (Cyrus-Beck); 0 if none. */
static int inside_part(const hull *h, float x0, float y0, float x1, float y1, float *ta, float *tb)
{
    float a = 0.0f, b = 1.0f, dx = x1 - x0, dy = y1 - y0;
    for (int i = 0; i < h->n; i++) {
        int j = (i + 1) % h->n;
        float ex = h->x[j] - h->x[i], ey = h->y[j] - h->y[i];
        /* inside: left of the edge, cross(e, p - pi) >= 0 */
        float num = ex * (y0 - h->y[i]) - ey * (x0 - h->x[i]);
        float den = ex * dy - ey * dx;
        if (fabsf(den) < 1e-9f) {
            if (num < 0.0f) return 0;
            continue;
        }
        float t = -num / den;
        if (den > 0.0f) { if (t > a) a = t; }
        else if (t < b) b = t;
        if (a >= b) return 0;
    }
    *ta = a; *tb = b;
    return 1;
}

/* --- the guns and the struts: solids, and only the edges that show ------------------ */

/* Each part is a convex solid of flat faces. An edge shows where one of
 * its faces looks towards the eye (the solid's own back hides the rest),
 * a rod's edges along it only where it turns away (its outline), and no
 * edge where a face of another, nearer solid covers it. 1 / z is straight
 * across a flat face on the screen, so nearer or farther is a matter of
 * a plane, as in render/solid.c. */

typedef struct v3 { float x, y, z; } v3;

#define FACE_MAX  16
#define MAX_FACES 120
#define MAX_EDGES 360
#define MAX_SOLIDS 16

enum { E_FRONT = 0, E_OUTLINE, E_DECAL };

typedef struct face {
    int   n, solid, front;
    float x[FACE_MAX], y[FACE_MAX];     /* on the screen, counter-clockwise */
    float a, b, c;                      /* 1 / z = a x + b y + c */
    float x0, y0, x1, y1;
} face;

typedef struct edge {
    v3  p, q;
    int f0, f1, mode, solid, tag, bright;
} edge;

typedef struct mesh {
    int  nf, ne, ns;
    face f[MAX_FACES];
    edge e[MAX_EDGES];
    float sx[MAX_SOLIDS][64][2];        /* each solid's points on the screen (for its outline) */
    int   sn[MAX_SOLIDS];
} mesh;

static v3 V(float x, float y, float z) { v3 v = { x, y, z }; return v; }

static void note_point(mesh *m, int solid, v3 p)
{
    if (solid >= MAX_SOLIDS || m->sn[solid] >= 64) return;
    project(p.x, p.y, p.z, &m->sx[solid][m->sn[solid]][0], &m->sx[solid][m->sn[solid]][1]);
    m->sn[solid]++;
}

/* A face of a solid whose middle is mid; returns its number or -1. */
static int add_face(mesh *m, const v3 *p, int n, v3 mid, int solid)
{
    if (m->nf >= MAX_FACES || n > FACE_MAX || n < 3) return -1;
    face *f = &m->f[m->nf];
    f->n = n;
    f->solid = solid;
    /* its normal, outwards; it looks at the eye if the eye is on that side */
    v3 c = V(0, 0, 0);
    for (int i = 0; i < n; i++) { c.x += p[i].x; c.y += p[i].y; c.z += p[i].z; }
    c.x /= n; c.y /= n; c.z /= n;
    v3 u = V(p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z), w = V(p[2].x - p[0].x, p[2].y - p[0].y, p[2].z - p[0].z);
    v3 nn = V(u.y * w.z - u.z * w.y, u.z * w.x - u.x * w.z, u.x * w.y - u.y * w.x);
    if (nn.x * (c.x - mid.x) + nn.y * (c.y - mid.y) + nn.z * (c.z - mid.z) < 0.0f) { nn.x = -nn.x; nn.y = -nn.y; nn.z = -nn.z; }
    f->front = nn.x * -c.x + nn.y * -c.y + nn.z * -c.z > 0.0f;
    /* on the screen, and its plane of 1 / z */
    float area = 0.0f, iw[FACE_MAX];
    for (int i = 0; i < n; i++) {
        project(p[i].x, p[i].y, p[i].z, &f->x[i], &f->y[i]);
        iw[i] = 1.0f / p[i].z;
    }
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += f->x[i] * f->y[j] - f->x[j] * f->y[i];
    }
    if (area < 0.0f)
        for (int i = 0, j = n - 1; i < j; i++, j--) {
            float t;
            t = f->x[i]; f->x[i] = f->x[j]; f->x[j] = t;
            t = f->y[i]; f->y[i] = f->y[j]; f->y[j] = t;
            t = iw[i]; iw[i] = iw[j]; iw[j] = t;
        }
    float d = (f->x[1] - f->x[0]) * (f->y[2] - f->y[0]) - (f->x[2] - f->x[0]) * (f->y[1] - f->y[0]);
    if (fabsf(d) < 1e-6f) {
        f->a = f->b = 0.0f;
        f->c = iw[0];
    } else {
        f->a = ((iw[1] - iw[0]) * (f->y[2] - f->y[0]) - (iw[2] - iw[0]) * (f->y[1] - f->y[0])) / d;
        f->b = ((f->x[1] - f->x[0]) * (iw[2] - iw[0]) - (f->x[2] - f->x[0]) * (iw[1] - iw[0])) / d;
        f->c = iw[0] - f->a * f->x[0] - f->b * f->y[0];
    }
    f->x0 = f->y0 = 1e30f; f->x1 = f->y1 = -1e30f;
    for (int i = 0; i < n; i++) {
        if (f->x[i] < f->x0) f->x0 = f->x[i];
        if (f->x[i] > f->x1) f->x1 = f->x[i];
        if (f->y[i] < f->y0) f->y0 = f->y[i];
        if (f->y[i] > f->y1) f->y1 = f->y[i];
        note_point(m, solid, p[i]);
    }
    if (fabsf(area) < 0.5f) f->front = 0;       /* (edge-on: covers nothing) */
    return m->nf++;
}

static void add_edge(mesh *m, v3 p, v3 q, int f0, int f1, int mode, int solid, int tag, int bright)
{
    if (m->ne >= MAX_EDGES) return;
    edge *e = &m->e[m->ne++];
    e->p = p; e->q = q; e->f0 = f0; e->f1 = f1; e->mode = mode; e->solid = solid; e->tag = tag; e->bright = bright;
}

/* A rod along z (round, n sides) at cx, cy from za to zb; its faces, its
 * rings at both ends, its outline along. Returns the face looking straight
 * up (for what lies on it). */
static int rod(mesh *m, float cx, float cy, float r, float za, float zb, int n, int solid, int tag, int bright)
{
    v3 a[16], b[16], mid = V(cx, cy, 0.5f * (za + zb));
    float off = PI * 0.5f - PI / n;             /* (a face centred on the top) */
    for (int i = 0; i < n; i++) {
        float ang = off + 2.0f * PI * i / n;
        a[i] = V(cx + r * cosf(ang), cy + r * sinf(ang), za);
        b[i] = V(cx + r * cosf(ang), cy + r * sinf(ang), zb);
    }
    int side[16];
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        v3 q[4] = { a[i], a[j], b[j], b[i] };
        side[i] = add_face(m, q, 4, mid, solid);
    }
    int near_cap = add_face(m, a, n, mid, solid), far_cap = add_face(m, b, n, mid, solid);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n, h = (i + n - 1) % n;
        add_edge(m, a[i], a[j], side[i], near_cap, E_FRONT, solid, tag, bright - 1);
        add_edge(m, b[i], b[j], side[i], far_cap, E_FRONT, solid, tag, bright - 1);
        add_edge(m, a[i], b[i], side[h], side[i], E_OUTLINE, solid, tag, bright);
    }
    return side[0];
}

/* A box (or a slanted one) from its eight corners, bit 0 x, bit 1 y, bit 2 z;
 * along: the bit of its edges that show only as its outline (a rod), or 0. */
static void box(mesh *m, const v3 *c, int solid, int tag, int bright, int along)
{
    static const int FACES[6][4] = { { 0, 2, 6, 4 }, { 1, 5, 7, 3 }, { 0, 4, 5, 1 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 6, 7, 5 } };
    v3 mid = V(0, 0, 0);
    for (int i = 0; i < 8; i++) { mid.x += c[i].x / 8; mid.y += c[i].y / 8; mid.z += c[i].z / 8; }
    int fid[6];
    for (int f = 0; f < 6; f++) {
        v3 q[4] = { c[FACES[f][0]], c[FACES[f][1]], c[FACES[f][2]], c[FACES[f][3]] };
        fid[f] = add_face(m, q, 4, mid, solid);
    }
    for (int i = 0; i < 8; i++)
        for (int bit = 1; bit < 8; bit <<= 1) {
            int j = i | bit;
            if (j == i) continue;
            int adj[2], na = 0;
            for (int f = 0; f < 6 && na < 2; f++) {
                int has_i = 0, has_j = 0;
                for (int v = 0; v < 4; v++) { has_i |= FACES[f][v] == i; has_j |= FACES[f][v] == j; }
                if (has_i && has_j) adj[na++] = fid[f];
            }
            if (na == 2) add_edge(m, c[i], c[j], adj[0], adj[1], bit == along ? E_OUTLINE : E_FRONT, solid, tag, bright);
        }
}

/* The stretch of a line (0..1) inside a convex, counter-clockwise shape
 * given by its corners (Cyrus-Beck); 0 if none. */
static int inside_xy(const float *hx, const float *hy, int n, float x0, float y0, float x1, float y1, float *ta, float *tb)
{
    hull h;
    h.n = n;
    memcpy(h.x, hx, (size_t)n * sizeof *hx);
    memcpy(h.y, hy, (size_t)n * sizeof *hy);
    return inside_part(&h, x0, y0, x1, y1, ta, tb);
}

static int cut_cmp(const void *a, const void *b)
{
    float x = ((const float *)a)[0], y = ((const float *)b)[0];
    return x < y ? -1 : x > y;
}

/* A line on the screen with 1 / z at its ends (w0, w1; 0: behind all, a
 * flat thing), drawn where no face of another solid is in front of it.
 * solid FLAT: the dashboard, which only the guns (solids 0..5) are in
 * front of. */
#define FLAT (-2)
static void draw_hidden(sink *k, const mesh *m, int solid, float x0, float y0, float w0, float x1, float y1, float w1,
                        int bright, int tag)
{
    float cuts[48][2];
    int nc = 0;
    float lx0 = fminf(x0, x1), lx1 = fmaxf(x0, x1), ly0 = fminf(y0, y1), ly1 = fmaxf(y0, y1);
    for (int i = 0; i < m->nf && nc < 48; i++) {
        const face *f = &m->f[i];
        if (!f->front || f->solid == solid || (solid == FLAT && f->solid >= 6)) continue;
        if (lx1 < f->x0 || lx0 > f->x1 || ly1 < f->y0 || ly0 > f->y1) continue;
        float ta, tb;
        if (!inside_xy(f->x, f->y, f->n, x0, y0, x1, y1, &ta, &tb)) continue;
        /* how much nearer the face is than the line at both ends of the stretch */
        float da = f->a * (x0 + (x1 - x0) * ta) + f->b * (y0 + (y1 - y0) * ta) + f->c - (w0 + (w1 - w0) * ta);
        float db = f->a * (x0 + (x1 - x0) * tb) + f->b * (y0 + (y1 - y0) * tb) + f->c - (w0 + (w1 - w0) * tb);
        const float eps = 1e-4f;
        if (da <= eps && db <= eps) continue;
        if (da <= eps || db <= eps) {
            float tc = ta + (tb - ta) * (eps - da) / (db - da);
            if (da <= eps) ta = tc;
            else tb = tc;
        }
        cuts[nc][0] = ta; cuts[nc][1] = tb; nc++;
    }
    qsort(cuts, (size_t)nc, sizeof cuts[0], cut_cmp);
    float at = 0.0f;
    for (int i = 0; i <= nc; i++) {
        float to = i < nc ? cuts[i][0] : 1.0f;
        if (to - at > 0.003f)
            seg(k, x0 + (x1 - x0) * at, y0 + (y1 - y0) * at, x0 + (x1 - x0) * to, y0 + (y1 - y0) * to, bright, tag);
        if (i < nc && cuts[i][1] > at) at = cuts[i][1];
    }
}

static void draw_mesh(sink *k, const mesh *m)
{
    for (int i = 0; i < m->ne; i++) {
        const edge *e = &m->e[i];
        int a = e->f0 >= 0 && m->f[e->f0].front, b = e->f1 >= 0 && m->f[e->f1].front;
        int show = e->mode == E_OUTLINE ? a != b : e->mode == E_DECAL ? (e->f0 < 0 || a) : (a || b);
        if (!show) continue;
        float x0, y0, x1, y1;
        project(e->p.x, e->p.y, e->p.z, &x0, &y0);
        project(e->q.x, e->q.y, e->q.z, &x1, &y1);
        draw_hidden(k, m, e->solid, x0, y0, 1.0f / e->p.z, x1, y1, 1.0f / e->q.z, e->bright, e->tag);
    }
}

/* Lines already drawn from first on (flat things behind all of the solids:
 * the dashboard), cut by them. */
static void cut_flat(sink *k, int first, const mesh *m)
{
    static rb_line keep[512];
    int n = k->n - first;
    if (n > 512) n = 512;
    memcpy(keep, k->l + first, (size_t)n * sizeof *keep);
    k->n = first;
    for (int i = 0; i < n; i++)
        draw_hidden(k, m, FLAT, keep[i].x0, keep[i].y0, 0.0f, keep[i].x1, keep[i].y1, 0.0f, keep[i].intensity, keep[i].tag);
}

/* A gun: m -1 for the left, 1 for the right (mirror); kick: how far it is
 * thrown back. Its solids: the receiver, the jacket, the muzzle. */
static void gun(mesh *me, float m, float kick, int solid0, int hot)
{
    const int G = RB_TAG_GUN;
    const int J = hot;              /* the jacket and the muzzle: their heat's colour */
    const float gx = -m * GUN_X;
    float z0 = RECV_Z0 - kick, z1 = RECV_Z1 - kick, zj = JACKET_Z1 - kick, zm = MUZZLE_Z1 - kick;
    v3 c[8];
    for (int i = 0; i < 8; i++)
        c[i] = V(gx + ((i & 1) ? RECV_W : -RECV_W), GUN_Y + ((i & 2) ? RECV_UP : -RECV_DOWN), (i & 4) ? z1 : z0);
    box(me, c, solid0, G, 12, 0);
    /* the rear sight on the receiver's top */
    int top_face = me->nf - 3;                  /* (the box's y+ face: the fourth of six) */
    add_edge(me, V(gx, GUN_Y + RECV_UP, z1 - 0.05f), V(gx, GUN_Y + RECV_UP + 0.028f, z1 - 0.05f), top_face, -1,
             E_DECAL, solid0, G, 11);
    /* the jacket, its slots along the top (a short line each: the vector
     * board's time is short) */
    int top = rod(me, gx, GUN_Y, JACKET_R, z1, zj, 10, solid0 + 1, J, 12);
    float ytop = GUN_Y + JACKET_R * cosf(PI / 10) + 0.001f;
    for (int s = 0; s < 4; s++) {
        float z = z1 + (zj - z1) * (0.14f + 0.2f * s);
        add_edge(me, V(gx, ytop, z), V(gx, ytop, z + 0.05f), top, -1, E_DECAL, solid0 + 1, G, 7);
    }
    /* the ring sight on the jacket's front: a post, the ring, its cross */
    {
        float zs = zj - 0.05f, cy = ytop + RING_UP + RING_R;
        add_edge(me, V(gx, ytop, zs), V(gx, cy - RING_R, zs), -1, -1, E_DECAL, solid0 + 1, G, 11);
        for (int i = 0; i < 10; i++) {
            float a0 = 2.0f * PI * i / 10, a1 = 2.0f * PI * (i + 1) / 10;
            add_edge(me, V(gx + RING_R * cosf(a0), cy + RING_R * sinf(a0), zs),
                     V(gx + RING_R * cosf(a1), cy + RING_R * sinf(a1), zs), -1, -1, E_DECAL, solid0 + 1, G, 12);
        }
        add_edge(me, V(gx - RING_R, cy, zs), V(gx + RING_R, cy, zs), -1, -1, E_DECAL, solid0 + 1, G, 10);
        add_edge(me, V(gx, cy - RING_R, zs), V(gx, cy + RING_R, zs), -1, -1, E_DECAL, solid0 + 1, G, 10);
    }
    /* the muzzle, its front sight */
    rod(me, gx, GUN_Y, MUZZLE_R, zj, zm, 8, solid0 + 2, J, 12);
    add_edge(me, V(gx, GUN_Y + MUZZLE_R, zm - 0.03f), V(gx, GUN_Y + MUZZLE_R + 0.025f, zm - 0.03f), -1, -1, E_DECAL,
             solid0 + 2, G, 11);
}

/* The nose: the engine's round cowling, as wide as the fuselage, the guns
 * on its top; only its top shows above the dashboard. */
static void nose(mesh *me, int solid)
{
    rod(me, 0.0f, NOSE_Y, NOSE_R, NOSE_Z0, NOSE_Z1, 16, solid, RB_TAG_NOSE, 9);
}

/* The propeller turning in front of the nose, stylised as a fast one shows
 * on film (the cabinet shows its own as a few streaks): at each blade's tip
 * a smooth swish, bright at its head and fading behind it, and a fainter
 * one further in; turning fast (27 degrees a picture at 60 a second).
 * Behind the nose and the guns. */
static void propeller(sink *k, const mesh *me, double t)
{
    float cx, cy, r, w = 1.0f / PROP_Z;
    project(0.0f, NOSE_Y, PROP_Z, &cx, &cy);
    r = F * PROP_R / PROP_Z;
    float base = (float)fmod(t * 4.5, 1.0) * 2.0f * PI;
    static const float RAD[3] = { 0.62f, 0.84f, 0.96f }, SPAN[3] = { 0.55f, 0.95f, 1.15f };
    static const int HEAD[3] = { 6, 11, 13 };
    const int SEGS = 6;
    for (int blade = 0; blade < 2; blade++) {
        float lead = base + blade * PI;
        for (int i = 0; i < 3; i++) {
            float rr = r * RAD[i];
            for (int s = 0; s < SEGS; s++) {
                float a0 = lead - SPAN[i] * s / SEGS, a1 = lead - SPAN[i] * (s + 1) / SEGS;
                int b = HEAD[i] - (HEAD[i] - 2) * s / (SEGS - 1);      /* (fading behind the head) */
                draw_hidden(k, me, -1, cx + rr * cosf(a0), cy + rr * sinf(a0), w, cx + rr * cosf(a1), cy + rr * sinf(a1),
                            w, b, RB_TAG_PROPELLER);
            }
        }
    }
}

/* The struts on one side (m -1 left, 1 right), each a slim slanted box
 * standing on the middle wing, solids from solid0 on. */
static void struts(mesh *me, float m, int solid0)
{
    const v3 FRONT_LOW = { 0.70f, WING_Y, 1.30f }, FRONT_HIGH = { 1.15f, 1.40f, 1.30f };
    const v3 BACK_LOW = { 0.62f, WING_Y, 0.90f }, BACK_HIGH = { 1.05f, 1.40f, 0.90f };
    const v3 *legs[2][2] = { { &FRONT_LOW, &FRONT_HIGH }, { &BACK_LOW, &BACK_HIGH } };
    for (int s = 0; s < 2; s++) {
        v3 lo = *legs[s][0], hi = *legs[s][1];
        lo.x *= m; hi.x *= m;
        v3 c[8];
        for (int i = 0; i < 8; i++) {
            const v3 *e = (i & 2) ? &hi : &lo;
            c[i] = V(e->x + ((i & 1) ? STRUT_W : -STRUT_W), e->y, e->z + ((i & 4) ? STRUT_W : -STRUT_W));
        }
        box(me, c, solid0 + s, RB_TAG_STRUT, 12, 2);
    }
}

/* The middle wing on one side: a thin slab from the fuselage out. */
static void wing(mesh *me, float m, int solid)
{
    v3 c[8];
    for (int i = 0; i < 8; i++) {
        float x = (i & 1) ? 3.2f : WING_X0, y = (i & 2) ? WING_Y : WING_Y - 0.025f, z = (i & 4) ? WING_Z1 : WING_Z0;
        c[i] = V(m * x, y, z);
    }
    box(me, c, solid, RB_TAG_STRUT, 11, 0);
}

/* A line in space, drawn where nothing of the cockpit is in front of it. */
static void line3(sink *k, const mesh *me, v3 a, v3 b, int bright, int tag)
{
    float x0, y0, x1, y1;
    project(a.x, a.y, a.z, &x0, &y0);
    project(b.x, b.y, b.z, &x1, &y1);
    draw_hidden(k, me, -1, x0, y0, 1.0f / a.z, x1, y1, 1.0f / b.z, bright, tag);
}

/* The bracing wires crossing in front, and the small windscreen. */
static void wires(sink *k, const mesh *me)
{
    for (int s = -1; s <= 1; s += 2)
        line3(k, me, V(s * 1.00f, 1.35f, 1.25f), V(-s * 0.10f, -0.22f, 1.70f), 6, RB_TAG_WIRE);
    v3 prev = V(-0.10f, GUN_Y - 0.06f, 0.95f);
    for (int i = 1; i <= 10; i++) {
        float a = PI - PI * i / 10;
        v3 p = V(0.10f * cosf(a), GUN_Y - 0.06f + 0.12f * sinf(a), 0.95f);
        line3(k, me, prev, p, 6, RB_TAG_WINDSCREEN);
        prev = p;
    }
}

/* The outline of a solid (for what it hides of the world). */
static void solid_hull(const mesh *m, int solid, hull *h)
{
    float pts[64][2];
    int n = m->sn[solid];
    memcpy(pts, m->sx[solid], (size_t)n * sizeof pts[0]);
    if (n >= 3) make_hull(h, pts, n);
    else h->n = 0;
}

static void flash_at(sink *k, float gx, float zm, float flash, double t)
{
    float cx, cy;
    project(gx, GUN_Y, zm + 0.01f, &cx, &cy);
    int b = (int)(15.0f * flash);
    float size = 7.0f + 5.0f * flash;
    for (int r = 0; r < 8; r++) {
        float a = 2.0f * PI * r / 8 + 0.3f * sinf((float)t * 37.0f + r * 1.7f);
        float len = size * (r & 1 ? 0.55f : 1.0f);
        seg(k, cx + 1.5f * cosf(a), cy + 1.5f * sinf(a), cx + len * cosf(a), cy + len * sinf(a), b, RB_TAG_FLASH);
    }
}

/* --- the dashboard and its instruments ----------------------------------------------- */

static void circle(sink *k, float cx, float cy, float r, int segs, int b, int tag)
{
    float px = cx + r, py = cy;
    for (int i = 1; i <= segs; i++) {
        float a = 2.0f * PI * i / segs;
        float x = cx + r * cosf(a), y = cy + r * sinf(a);
        seg(k, px, py, x, y, b, tag);
        px = x; py = y;
    }
}

/* A dial: ticks from angle a0 to a1 (radians), and its needle at the
 * part v of that; the ticks from red_from on red. */
static void dial(sink *k, float cx, float cy, float r, float a0, float a1, int ticks, float v, int red_from)
{
    circle(k, cx, cy, r, 14, 11, RB_TAG_GAUGE);
    for (int i = 0; i <= ticks; i++) {
        float a = a0 + (a1 - a0) * i / ticks;
        int tag = i >= red_from ? RB_TAG_NEEDLE : RB_TAG_GAUGE;
        seg(k, cx + r * 0.72f * cosf(a), cy + r * 0.72f * sinf(a), cx + r * 0.92f * cosf(a), cy + r * 0.92f * sinf(a), 9,
            tag);
    }
    float a = a0 + (a1 - a0) * clamp01(v);
    seg(k, cx - r * 0.18f * cosf(a), cy - r * 0.18f * sinf(a), cx + r * 0.80f * cosf(a), cy + r * 0.80f * sinf(a), 15,
        RB_TAG_NEEDLE);
}

static void thermometer(sink *k, float cx, float cy, float r, float heat, int hot, double t)
{
    circle(k, cx, cy, r, 14, 11, RB_TAG_GAUGE);
    float x0 = cx - 2.0f, x1 = cx + 2.0f, ytop = cy + r * 0.62f, ybulb = cy - r * 0.45f;
    seg(k, x0, ybulb, x0, ytop, 10, RB_TAG_GAUGE);
    seg(k, x1, ybulb, x1, ytop, 10, RB_TAG_GAUGE);
    seg(k, x0, ytop, x1, ytop, 10, RB_TAG_GAUGE);
    circle(k, cx, ybulb - 2.5f, 3.5f, 6, 12, RB_TAG_NEEDLE);
    int b = hot && fmod(t, 0.4) < 0.2 ? 6 : 15;
    float top = ybulb + (ytop - ybulb) * clamp01(heat);
    seg(k, cx, ybulb, cx, top, b, RB_TAG_NEEDLE);
    for (int i = 1; i < 4; i++) {
        float y = ybulb + (ytop - ybulb) * i / 4.0f;
        seg(k, x1 + 1.0f, y, x1 + 4.0f, y, 8, RB_TAG_GAUGE);
    }
}

static void dash_hull(hull *h)
{
    float pts[24][2];
    int n = 0;
    for (int i = 0; i <= 12; i++) {
        pts[n][0] = -PANEL_X + 2.0f * PANEL_X * i / 12;
        pts[n][1] = dash_y(pts[n][0]);
        n++;
    }
    /* (below, out to the screen's corners: the cockpit's sides, where the
     * middle wing does not cover; nothing is seen out there) */
    pts[n][0] = 300.0f; pts[n][1] = -212.0f; n++;
    pts[n][0] = PANEL_X + 120.0f; pts[n][1] = -260.0f; n++;
    pts[n][0] = -PANEL_X - 120.0f; pts[n][1] = -260.0f; n++;
    pts[n][0] = -300.0f; pts[n][1] = -212.0f; n++;
    make_hull(h, pts, n);
}

/* --- all of it ---------------------------------------------------------------------------- */

void rb_cockpit_init(rb_cockpit *c)
{
    memset(c, 0, sizeof *c);
}

int rb_cockpit_lines(rb_cockpit *c, const uint8_t *mem, double t, rb_line *out, int max)
{
    double dt = c->have ? t - c->last_t : 0.0;
    if (dt < 0.0 || dt > 0.5) dt = 0.0;
    c->last_t = t;

    /* the game's state */
    int playing = (mem[RB_GAMODE] & 0x80) != 0;
    int firing = playing && (mem[RB_SWSTAT] & 0x80) && !(mem[RB_GUN_ST] & 0x40);
    int side = (mem[RB_S_XFLG] & 0x40) ? 1 : 0;
    float height = (float)(int16_t)(mem[RB_I4YPOS] | (mem[RB_I4YPOS + 1] << 8));
    float climb = (float)(int16_t)(mem[RB_PLDELY] | (mem[RB_PLDELY + 1] << 8));
    float heat = (float)(mem[RB_GUN_ST] & 0x7f) / 64.0f;

    /* a shot: the gun the game fired kicks back */
    if (firing && (side != c->side || !c->have)) c->recoil[side] = 1.0f;
    c->side = side;
    for (int g = 0; g < 2; g++) c->recoil[g] *= expf(-(float)dt * 14.0f);

    /* the needles follow (the game moves at ten frames a second) */
    float speed = 0.55f - climb / 1024.0f + 0.012f * sinf((float)t * 7.0f);
    float alt = height / 2048.0f;
    float ease = c->have ? 1.0f - expf(-(float)dt * 6.0f) : 1.0f;
    c->speed += (clamp01(speed) - c->speed) * ease;
    c->alt += (alt - c->alt) * ease;
    c->heat += (clamp01(heat) - c->heat) * (c->have ? 1.0f - expf(-(float)dt * 10.0f) : 1.0f);
    c->have = 1;

    sink k = { out, 0, max };
    /* the solids: the guns (3 each), the struts (3 each side) */
    static mesh me;
    me.nf = me.ne = 0;
    memset(me.sn, 0, sizeof me.sn);
    /* the guns' jackets: hot orange from half the heat on, overheated
     * (GUN.ST D6, the game stops firing) blinking red */
    int hot = RB_TAG_GUN;
    if (mem[RB_GUN_ST] & 0x40) hot = fmod(t, 0.3) < 0.15 ? RB_TAG_NEEDLE : RB_TAG_GUNHOT;
    else if (c->heat > 0.55f) hot = RB_TAG_GUNHOT;
    for (int g = 0; g < 2; g++) gun(&me, g ? 1.0f : -1.0f, KICK * c->recoil[g], 3 * g, hot);
    struts(&me, -1.0f, 6);
    struts(&me, 1.0f, 8);
    nose(&me, 10);
    wing(&me, -1.0f, 11);
    wing(&me, 1.0f, 12);
    /* the dashboard (flat, behind the guns, in front of the struts' feet):
     * its edge and trim, the instruments, cut where a gun covers them */
    int first = k.n;
    for (int i = 0; i < 12; i++) {
        float xa = -PANEL_X + 2.0f * PANEL_X * i / 12, xb = -PANEL_X + 2.0f * PANEL_X * (i + 1) / 12;
        seg(&k, xa, dash_y(xa), xb, dash_y(xb), 13, RB_TAG_DASH);
        seg(&k, xa, dash_y(xa) - 5.0f, xb, dash_y(xb) - 5.0f, 6, RB_TAG_DASH);
    }
    seg(&k, -PANEL_X, dash_y(-PANEL_X), -PANEL_X, -205.0f, 12, RB_TAG_DASH);
    seg(&k, PANEL_X, dash_y(PANEL_X), PANEL_X, -205.0f, 12, RB_TAG_DASH);
    const float gy = -164.0f;
    dial(&k, -88.0f, gy, 22.0f, PI * 0.5f, PI * 0.5f - 2.0f * PI * 0.9f, 9,
         fmodf(c->alt, 1.0f) + (c->alt < 0.0f ? 1.0f : 0.0f), 99);
    dial(&k, 88.0f, gy, 22.0f, PI * 1.25f, PI * -0.25f, 8, c->speed, 7);
    thermometer(&k, 0.0f, gy - 14.0f, 14.0f, c->heat, (mem[RB_GUN_ST] & 0x40) != 0, t);
    cut_flat(&k, first, &me);
    /* the solids' edges that show; the struts' below the dashboard's edge go */
    first = k.n;
    draw_mesh(&k, &me);
    propeller(&k, &me, t);
    wires(&k, &me);
    hull dash;
    dash_hull(&dash);
    {
        int end = k.n, w = first;
        static rb_line keep[512];
        int n = end - first > 512 ? 512 : end - first;
        memcpy(keep, k.l + first, (size_t)n * sizeof *keep);
        k.n = first;
        for (int i = 0; i < n; i++) {
            int behind = keep[i].tag == RB_TAG_STRUT || keep[i].tag == RB_TAG_NOSE || keep[i].tag == RB_TAG_PROPELLER ||
                         keep[i].tag == RB_TAG_WIRE || keep[i].tag == RB_TAG_WINDSCREEN;
            if (!behind) { k.l[k.n++] = keep[i]; continue; }
            float ta, tb;
            if (!inside_part(&dash, keep[i].x0, keep[i].y0, keep[i].x1, keep[i].y1, &ta, &tb)) { k.l[k.n++] = keep[i]; continue; }
            rb_line q = keep[i];
            float dx = q.x1 - q.x0, dy = q.y1 - q.y0;
            if (ta > 0.003f) { rb_line p = q; p.x1 = q.x0 + dx * ta; p.y1 = q.y0 + dy * ta; k.l[k.n++] = p; }
            if (tb < 0.997f) { rb_line p = q; p.x0 = q.x0 + dx * tb; p.y0 = q.y0 + dy * tb; k.l[k.n++] = p; }
        }
        (void)w;
    }
    /* the flashes */
    for (int g = 0; g < 2; g++)
        if (c->recoil[g] > 0.45f)
            flash_at(&k, (g ? 1.0f : -1.0f) * -GUN_X, MUZZLE_Z1 - KICK * c->recoil[g], (c->recoil[g] - 0.45f) / 0.55f, t);

    /* what hides the world: each solid's outline, the dashboard */
    c->n_hulls = 0;
    for (int s = 0; s < 13 && c->n_hulls < RB_COCKPIT_HULLS - 1; s++) {
        solid_hull(&me, s, &c->hull[c->n_hulls]);
        if (c->hull[c->n_hulls].n >= 3) c->n_hulls++;
    }
    c->hull[c->n_hulls++] = dash;
    return k.n;
}

int rb_cockpit_drop_own_prop(rb_line *l, int n)
{
    int w = 0;
    for (int i = 0; i < n; i++) {
        /* (the picture the interrupt turns, its JSRL at 2012 or 2812, no tag) */
        if (l[i].tag == RB_TAG_NONE && (l[i].src == 0x2012 || l[i].src == 0x2812)) continue;
        l[w++] = l[i];
    }
    return w;
}

int rb_cockpit_polys(const rb_cockpit *c, rb_solid_poly *out, int max)
{
    int n = 0;
    for (int i = 0; i < c->n_hulls && n < max; i++)
        if (rb_solid_front_poly(&out[n], c->hull[i].x, c->hull[i].y, c->hull[i].n)) n++;
    return n;
}
