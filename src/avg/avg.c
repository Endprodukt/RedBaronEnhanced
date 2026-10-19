#include "avg/avg.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAX_INSTRUCTIONS 50000
#define UNIT 65536.0f           /* the beam's position is 16.16 fixed point */

void rb_avg_list_free(rb_avg_list *l)
{
    free(l->lines);
    memset(l, 0, sizeof *l);
}

int rb_avg_list_reserve(rb_avg_list *l, int n)
{
    if (n <= l->cap) return 0;
    int cap = l->cap ? l->cap : 1024;
    while (cap < n) cap *= 2;
    rb_line *p = (rb_line *)realloc(l->lines, (size_t)cap * sizeof *p);
    if (!p) return -1;
    l->lines = p;
    l->cap = cap;
    return 0;
}

static int push_line(rb_avg_list *l, const rb_line *ln)
{
    if (l->count == l->cap && rb_avg_list_reserve(l, l->count + 1) != 0) return -1;
    l->lines[l->count++] = *ln;
    return 0;
}

static unsigned word_at(const uint8_t *mem, unsigned addr)
{
    return (unsigned)mem[addr & 0x7fff] | ((unsigned)mem[(addr + 1) & 0x7fff] << 8);
}

/* The normalization the board does before a vector: both deltas are
 * shifted up until one of them uses its top bit; every shift halves the
 * time the vector timer runs. Returns the shifts. */
static int normalize(unsigned *dvy, unsigned *dvx)
{
    int i = 0;
    while (((*dvy ^ (*dvy << 1)) & 0x1000) == 0 && ((*dvx ^ (*dvx << 1)) & 0x1000) == 0 && i < 16) {
        i++;
        *dvy = (*dvy & 0x1000) | ((*dvy << 1) & 0x1fff);
        *dvx = (*dvx & 0x1000) | ((*dvx << 1) & 0x1fff);
    }
    return i;
}

/* Cut the line to the window (x0..x1, y0..y1); 0 if nothing is left. */
static int clip(double *ax, double *ay, double *bx, double *by, double x0, double y0, double x1, double y1)
{
    double t0 = 0.0, t1 = 1.0, dx = *bx - *ax, dy = *by - *ay;
    const double p[4] = { -dx, dx, -dy, dy };
    const double q[4] = { *ax - x0, x1 - *ax, *ay - y0, y1 - *ay };
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0) {
            if (q[i] < 0.0) return 0;
        } else {
            double t = q[i] / p[i];
            if (p[i] < 0.0) { if (t > t1) return 0; if (t > t0) t0 = t; }
            else            { if (t < t0) return 0; if (t < t1) t1 = t; }
        }
    }
    double sx = *ax, sy = *ay;
    *ax = sx + t0 * dx; *ay = sy + t0 * dy;
    *bx = sx + t1 * dx; *by = sy + t1 * dy;
    return 1;
}

int rb_line_clip(rb_line *l)
{
    double ax = l->x0, ay = l->y0, bx = l->x1, by = l->y1;
    if (!clip(&ax, &ay, &bx, &by, l->win[0], l->win[1], l->win[2], l->win[3])) return 0;
    l->x0 = (float)ax; l->y0 = (float)ay; l->x1 = (float)bx; l->y1 = (float)by;
    return 1;
}

int rb_avg_run(const uint8_t *mem, uint16_t start, rb_avg_list *out, int max_instructions)
{
    int status = rb_avg_run_raw(mem, start, NULL, NULL, out, max_instructions);
    int n = 0;
    for (int i = 0; i < out->count; i++) {
        rb_line l = out->lines[i];
        float x0 = l.x0, y0 = l.y0, x1 = l.x1, y1 = l.y1;
        if (!rb_line_clip(&l)) { out->clipped++; continue; }
        if (l.x0 != x0 || l.y0 != y0 || l.x1 != x1 || l.y1 != y1) out->clipped++;
        out->lines[n++] = l;
    }
    out->count = n;
    return status;
}

int rb_avg_run_raw(const uint8_t *mem, uint16_t start, const uint16_t *tags, const uint32_t *notes,
                   rb_avg_list *out, int max_instructions)
{
    return rb_avg_run_inst(mem, start, tags, notes, NULL, out, max_instructions);
}

int rb_avg_run_inst(const uint8_t *mem, uint16_t start, const uint16_t *tags, const uint32_t *notes,
                    const uint16_t *insts, rb_avg_list *out, int max_instructions)
{
    /* where the beam is, by the notes: the object's point and 1 / its distance */
    unsigned at_pt = 0, at_model = 0;
    float at_w = 0.0f;
    unsigned pc = start;
    unsigned tag = 0, ord = 0;
    unsigned stack[4] = { 0, 0, 0, 0 };
    unsigned sp = 0;
    int32_t xpos = 0, ypos = 0;             /* 16.16, the center at 0, y up */
    unsigned scale = 0, bin_scale = 0;      /* as VSTOP leaves them */
    unsigned intensity = 0;
    /* The window: nothing set means everything shows. */
    int32_t win_x0 = INT32_MIN / 2, win_y0 = INT32_MIN / 2, win_x1 = INT32_MAX / 2, win_y1 = INT32_MAX / 2;
    int lst = 1, hst = 1;                   /* 0: the next instruction latches that corner */
    int depth = 0;
    unsigned src = 0, sub = 0, nline = 0, call = 0;

    if (max_instructions <= 0) max_instructions = DEFAULT_MAX_INSTRUCTIONS;
    out->count = 0;
    out->instructions = 0;
    out->status = RB_AVG_OK;
    out->clipped = 0;

    for (;;) {
        if (out->instructions >= max_instructions) return out->status = RB_AVG_LIMIT;
        out->instructions++;

        /* The analog switches of the window open at the instruction after
         * the STAT that asked for it: the beam is where it was moved to. */
        if (!hst) { win_x1 = xpos; win_y1 = ypos; }
        if (!lst) { win_x0 = xpos; win_y0 = ypos; }
        lst = hst = 1;

        /* An instruction in vector RAM, of the list or of a picture the list
         * calls there (Red Baron builds its planes so), tells what it is;
         * one of a vector ROM routine is what its caller was. */
        int in_ram = pc >= 0x2000 && pc < 0x3000;
        if (depth == 0) {
            sub = 0;
            nline = 0;
            call = pc;
        }
        if (depth == 0 || in_ram) {
            src = pc;
            unsigned t = (tags && in_ram) ? tags[(pc - 0x2000) >> 1] : 0;
            if (t != tag) { tag = t; ord = 0; }
        }
        unsigned w = word_at(mem, pc);
        pc += 2;
        unsigned dvy = 0, dvx = 0, z = 0;
        int k, vector = 0, cycles = 0;

        switch (w >> 13) {
        case 0:  /* VCTR */
            dvy = w & 0x1fff;
            w = word_at(mem, pc);
            pc += 2;
            dvx = w & 0x1fff;
            z = w >> 13;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles = k < 15 ? 0x8000 >> k : 1;
            vector = 1;
            break;
        case 1:  /* HALT */
            return out->status;
        case 2:  /* SVEC */
            dvy = (w >> 8) & 0x1f;
            dvy = ((dvy & 0x10) << 8) | ((dvy & 0x0f) << 8);
            dvx = w & 0x1f;
            dvx = ((dvx & 0x10) << 8) | ((dvx & 0x0f) << 8);
            z = (w >> 5) & 7;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles = k < 8 ? 0x100 >> k : 1;
            vector = 1;
            break;
        case 3:
            if (w & 0x1000) {           /* SCAL */
                scale = w & 0xff;
                bin_scale = (w >> 8) & 7;
            } else {                    /* STAT */
                intensity = (w >> 4) & 0x0f;
                if (!(w & 0x400)) {
                    lst = (w & 0x200) != 0;
                    hst = !lst;
                }
            }
            break;
        case 4:  /* CNTR */
            xpos = ypos = 0;
            at_pt = 0;
            at_w = 0.0f;
            break;
        case 5:  /* JSRL */
            stack[sp & 3] = pc;
            sp = (sp + 1) & 0xf;
            pc = 0x2000 + ((w & 0x1fff) << 1);
            if (depth == 0) sub = pc;
            depth++;
            break;
        case 6:  /* RTSL */
            sp = (sp - 1) & 0xf;
            pc = stack[sp & 3];
            if (depth > 0) depth--;
            break;
        case 7:  /* JMPL */
            pc = 0x2000 + ((w & 0x1fff) << 1);
            break;
        }

        if (vector) {
            /* The DACs take the top ten bits of each delta. */
            int64_t dx = (int64_t)((int32_t)((dvx >> 3) ^ 0x200) - 0x200) * cycles * (int32_t)(scale ^ 0xff);
            int64_t dy = (int64_t)((int32_t)((dvy >> 3) ^ 0x200) - 0x200) * cycles * (int32_t)(scale ^ 0xff);
            int32_t nx = xpos + (int32_t)(dx >> 4), ny = ypos + (int32_t)(dy >> 4);
            unsigned lum = z == 1 ? intensity : z << 1;
            /* A vector of the list itself goes to the point its note names;
             * what a vector ROM routine draws from there (the explosion's
             * picture) is as far away as where it began. */
            unsigned to_pt = at_pt, to_model = at_model;
            float to_w = at_w;
            if (depth == 0 || in_ram) {
                uint32_t note = (notes && src >= 0x2000 && src < 0x3000) ? notes[(src - 0x2000) >> 1] : 0;
                int16_t ahead = (int16_t)(note & 0xffff);
                if (note && ahead > 0) {
                    to_pt = (note >> 16) & 0xff;
                    to_model = note >> 24;
                    to_w = 1.0f / (float)ahead;
                } else if (dvx || dvy) {
                    to_pt = 0;
                    to_w = 0.0f;
                }
            } else {
                to_pt = 0;
            }
            if (lum) {
                rb_line ln;
                ln.w0 = at_w; ln.w1 = to_w;
                ln.p0 = (uint8_t)at_pt; ln.p1 = (uint8_t)to_pt;
                ln.model = (uint8_t)to_model;
                ln.x0 = (float)(xpos / UNIT); ln.y0 = (float)(ypos / UNIT);
                ln.x1 = (float)(nx / UNIT); ln.y1 = (float)(ny / UNIT);
                ln.intensity = (uint8_t)lum;
                ln.z = (uint8_t)z;
                ln.src = (uint16_t)src;
                ln.sub = (uint16_t)sub;
                ln.call = (uint16_t)call;
                ln.inst = (insts && src >= 0x2000 && src < 0x3000) ? insts[(src - 0x2000) >> 1] : 0;
                ln.n = (uint16_t)nline;
                ln.tag = (uint16_t)tag;
                ln.ord = (uint16_t)ord;
                ln.win[0] = (float)(win_x0 / UNIT); ln.win[1] = (float)(win_y0 / UNIT);
                ln.win[2] = (float)(win_x1 / UNIT); ln.win[3] = (float)(win_y1 / UNIT);
                if (push_line(out, &ln) != 0) return out->status = RB_AVG_LIMIT;
            }
            nline++;
            if (depth == 0) ord++;
            xpos = nx;
            ypos = ny;
            at_pt = to_pt;
            at_model = to_model;
            at_w = to_w;
        }
    }
}

uint32_t rb_avg_cycles(const uint8_t *mem, uint16_t start)
{
    /* States the machine goes through per instruction, 8 clock cycles each
     * (the state PROM, STATE2.MAC). */
    static const uint8_t STATES[8] = { 8, 4, 6, 7, 5, 5, 3, 2 };
    unsigned pc = start, sp = 0, bin_scale = 0;
    unsigned stack[4] = { 0, 0, 0, 0 };
    uint32_t cycles = 0;

    for (int n = 0; n < DEFAULT_MAX_INSTRUCTIONS; n++) {
        unsigned w = word_at(mem, pc), dvy, dvx;
        int k;
        pc += 2;
        cycles += 8u * STATES[w >> 13];
        switch (w >> 13) {
        case 0:
            dvy = w & 0x1fff;
            dvx = word_at(mem, pc) & 0x1fff;
            pc += 2;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles += k < 15 ? 0x8000u >> k : 1u;
            break;
        case 1:
            return cycles;
        case 2:
            dvy = (w & 0x1f00);
            dvx = (w & 0x1f) << 8;
            k = normalize(&dvy, &dvx) + (int)bin_scale;
            cycles += k < 8 ? 0x100u >> k : 1u;
            break;
        case 3:
            if (w & 0x1000) bin_scale = (w >> 8) & 7;
            break;
        case 4:  /* CNTR: the beam's way back takes a timer run too */
            dvy = w & 0x1fff;
            dvx = 0;
            k = normalize(&dvy, &dvx);
            cycles += k < 15 ? 0x8000u >> k : 1u;
            break;
        case 5:
            stack[sp & 3] = pc;
            sp++;
            pc = 0x2000 + ((w & 0x1fff) << 1);
            break;
        case 6:
            sp--;
            pc = stack[sp & 3];
            break;
        case 7:
            pc = 0x2000 + ((w & 0x1fff) << 1);
            break;
        }
    }
    return cycles;      /* a list that never halts: the interrupt restarts the generator anyway */
}
