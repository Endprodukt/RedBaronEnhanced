/* m6502.h - the 6502's registers and instructions as C, for the routines
 * of src/game.
 *
 * The original passes values between its routines in A, X, Y and the
 * carry, tests flags a routine set long before, and leaves scratch values
 * behind that a later frame reads. So that the C computes the same in
 * every bit (tools/rbcompare holds every byte of RAM against MAME), the
 * routines are written with the registers kept in the machine and one
 * macro per instruction; the original's labels and comments stand next to
 * them. All memory goes through rb_rd8/rb_wr8, the hardware included.
 *
 *   LDA AX,PKYRAN      LDA(RD(AX(RB_PKYRAN)));
 *   STA ZX,PLDELX      STA(ZX(RB_PLDELX));
 *   EOR NY,COPYPT      EOR(RD(IY(RB_COPYPT)));
 *   BNE 10$            if (BNE) goto l10;
 *
 * Addressing follows the source's prefixes: I is the value itself, A and Z
 * an address, AX and AY the address plus the register, ZX and ZY the same
 * within page 0 (it wraps there), NY and NX the address a pointer in page
 * 0 gives. What PHA and PHP push goes on a small stack of its own in the
 * machine; return addresses are C's.
 */
#ifndef RB_GAME_M6502_H
#define RB_GAME_M6502_H

#include "core/machine.h"

#define A  (m->r.a)
#define X  (m->r.x)
#define Y  (m->r.y)
#define C  (m->r.c)

#define RD(a)     rb_rd8(m, (uint16_t)(a))
#define WR(a, v)  rb_wr8(m, (uint16_t)(a), (uint8_t)(v))
/* Page 0 indexed: the sum stays in page 0. (The 6502 reads the unindexed
 * address first; a recording has that read too.) */
#define ZX(zp)    ((void)RD(zp), (uint8_t)((zp) + X))
#define ZY(zp)    ((void)RD(zp), (uint8_t)((zp) + Y))
/* The halves of an address (the source's macros LAH, LXL, ...). */
#define HI(a)     (((a) >> 8) & 0xff)
#define LO(a)     ((a) & 0xff)
/* The 6502 takes a cycle to carry an index into the address's high byte and
 * reads the address without the carry in it; a store or a shift of memory
 * always does. The reads are made here too, because a recording has them. */
static inline uint16_t rb_indexed(rb_machine *m, uint16_t base, uint8_t index, int always)
{
    uint16_t ea = (uint16_t)(base + index);
    if (always || ((base ^ ea) & 0xff00)) (void)rb_rd8(m, (uint16_t)((base & 0xff00) | (ea & 0xff)));
    return ea;
}
/* AX, AY: the address plus the register, to read from; AXW, AYW: to write to. */
#define AX(a)     rb_indexed(m, (uint16_t)(a), X, 0)
#define AY(a)     rb_indexed(m, (uint16_t)(a), Y, 0)
#define AXW(a)    rb_indexed(m, (uint16_t)(a), X, 1)
#define AYW(a)    rb_indexed(m, (uint16_t)(a), Y, 1)
/* NY: the address (zp),Y points at; NYW: to write to. */
#define IY(zp)    rb_indexed(m, (uint16_t)(RD(zp) | (RD((uint8_t)((zp) + 1)) << 8)), Y, 0)
#define IYW(zp)   rb_indexed(m, (uint16_t)(RD(zp) | (RD((uint8_t)((zp) + 1)) << 8)), Y, 1)
/* NX: the address (zp,X) points at. */
#define IX(zp)    ((uint16_t)(RD((uint8_t)((zp) + X)) | (RD((uint8_t)((zp) + X + 1)) << 8)))

static inline uint8_t rb_nz(rb_machine *m, unsigned v)
{
    m->r.z = (uint8_t)((v & 0xff) == 0);
    m->r.n = (uint8_t)((v >> 7) & 1);
    return (uint8_t)v;
}
#define NZ(v) rb_nz(m, (unsigned)(v))

static inline void rb_adc(rb_machine *m, uint8_t v)
{
    if (m->r.d) {
        /* decimal: the program adds valid BCD only and uses the carry, not N, V or Z */
        unsigned lo = (m->r.a & 0x0fu) + (v & 0x0fu) + m->r.c;
        unsigned hi = (m->r.a & 0xf0u) + (v & 0xf0u);
        uint8_t bin = (uint8_t)(m->r.a + v + m->r.c);
        if (lo > 9) { lo += 6; hi += 0x10; }
        m->r.z = (uint8_t)(bin == 0);
        m->r.n = (uint8_t)((hi >> 7) & 1);
        m->r.v = (uint8_t)((~(m->r.a ^ v) & (m->r.a ^ hi) & 0x80) != 0);
        if (hi > 0x90) hi += 0x60;
        m->r.c = (uint8_t)(hi > 0xff);
        m->r.a = (uint8_t)((lo & 0x0f) | (hi & 0xf0));
    } else {
        unsigned sum = (unsigned)m->r.a + v + m->r.c;
        m->r.v = (uint8_t)((~(m->r.a ^ v) & (m->r.a ^ sum) & 0x80) != 0);
        m->r.c = (uint8_t)(sum > 0xff);
        m->r.a = rb_nz(m, sum);
    }
}

static inline void rb_sbc(rb_machine *m, uint8_t v)
{
    /* (the program never subtracts in decimal mode) */
    unsigned diff = (unsigned)m->r.a - v - (m->r.c ? 0u : 1u);
    m->r.v = (uint8_t)(((m->r.a ^ v) & (m->r.a ^ diff) & 0x80) != 0);
    m->r.c = (uint8_t)!(diff & 0x100);
    m->r.a = rb_nz(m, diff);
}

static inline void rb_cmp(rb_machine *m, uint8_t r, uint8_t v)
{
    m->r.c = (uint8_t)(r >= v);
    rb_nz(m, (unsigned)(r - v));
}

static inline void rb_bit(rb_machine *m, uint8_t v)
{
    m->r.z = (uint8_t)((m->r.a & v) == 0);
    m->r.n = (uint8_t)(v >> 7);
    m->r.v = (uint8_t)((v >> 6) & 1);
}

static inline uint8_t rb_asl(rb_machine *m, uint8_t v) { m->r.c = (uint8_t)(v >> 7); return rb_nz(m, (unsigned)v << 1); }
static inline uint8_t rb_lsr(rb_machine *m, uint8_t v) { m->r.c = (uint8_t)(v & 1); return rb_nz(m, (unsigned)v >> 1); }
static inline uint8_t rb_rol(rb_machine *m, uint8_t v)
{
    unsigned r = ((unsigned)v << 1) | m->r.c;
    m->r.c = (uint8_t)(v >> 7);
    return rb_nz(m, r);
}
static inline uint8_t rb_ror(rb_machine *m, uint8_t v)
{
    unsigned r = ((unsigned)v >> 1) | ((unsigned)m->r.c << 7);
    m->r.c = (uint8_t)(v & 1);
    return rb_nz(m, r);
}

/* loads, stores, transfers */
#define LDA(v)   (A = NZ(v))
#define LDX(v)   (X = NZ(v))
#define LDY(v)   (Y = NZ(v))
#define STA(a)   WR(a, A)
#define STX(a)   WR(a, X)
#define STY(a)   WR(a, Y)
#define TAX()    (X = NZ(A))
#define TAY()    (Y = NZ(A))
#define TXA()    (A = NZ(X))
#define TYA()    (A = NZ(Y))
#define INX()    (X = NZ(X + 1))
#define INY()    (Y = NZ(Y + 1))
#define DEX()    (X = NZ(X - 1))
#define DEY()    (Y = NZ(Y - 1))

/* arithmetic and logic on A */
#define ADC(v)   rb_adc(m, (uint8_t)(v))
#define SBC(v)   rb_sbc(m, (uint8_t)(v))
#define AND(v)   (A = NZ(A & (v)))
#define ORA(v)   (A = NZ(A | (v)))
#define EOR(v)   (A = NZ(A ^ (v)))
#define CMP(v)   rb_cmp(m, A, (uint8_t)(v))
#define CPX(v)   rb_cmp(m, X, (uint8_t)(v))
#define CPY(v)   rb_cmp(m, Y, (uint8_t)(v))
#define BIT(v)   rb_bit(m, (uint8_t)(v))
#define ASLA()   (A = rb_asl(m, A))
#define LSRA()   (A = rb_lsr(m, A))
#define ROLA()   (A = rb_rol(m, A))
#define RORA()   (A = rb_ror(m, A))

/* on memory */
#define ASL(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, rb_asl(m, RD(a_))); } while (0)
#define LSR(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, rb_lsr(m, RD(a_))); } while (0)
#define ROL(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, rb_rol(m, RD(a_))); } while (0)
#define ROR(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, rb_ror(m, RD(a_))); } while (0)
#define INC(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, NZ(RD(a_) + 1)); } while (0)
#define DEC(a)   do { uint16_t a_ = (uint16_t)(a); WR(a_, NZ(RD(a_) - 1)); } while (0)

/* flags */
#define CLC()    (C = 0)
#define SEC()    (C = 1)
#define CLD()    (m->r.d = 0)
#define SED()    (m->r.d = 1)
#define BEQ      (m->r.z)
#define BNE      (!m->r.z)
#define BMI      (m->r.n)
#define BPL      (!m->r.n)
#define BCS      (m->r.c)
#define BCC      (!m->r.c)
#define BVS      (m->r.v)
#define BVC      (!m->r.v)

/* the stack: values only */
#define PHA()    (m->stk[m->sp++ & 63] = A)
#define PLA()    (A = NZ(m->stk[--m->sp & 63]))
#define PHP()    (m->stk[m->sp++ & 63] = (uint8_t)(m->r.c | (m->r.z << 1) | (m->r.d << 3) | (m->r.v << 6) | (m->r.n << 7)))
#define PLP()    do { uint8_t p_ = m->stk[--m->sp & 63]; m->r.c = p_ & 1; m->r.z = (p_ >> 1) & 1;                       m->r.d = (p_ >> 3) & 1; m->r.v = (p_ >> 6) & 1; m->r.n = (p_ >> 7) & 1; } while (0)
#define NOP()    ((void)0)

/* Additions, for the picture only (core/tags.h, render/solid.h): a call
 * drawing with a tag, or drawing an object whose points are noted. */
#define RB_TAGGED(t, call)  do { uint16_t tag_ = m->tag; m->tag = (t); call; m->tag = tag_; } while (0)
#define RB_NOTED(o, call)   do { m->note_obj = (o); m->inst++; call; m->note_obj = 0; m->note = 0; } while (0)

/* MATSTT: wait for the math box (BIT MATHST, BMI back). It is always done here. */
#define MATSTT() BIT(RD(RB_MATHST))

#endif
