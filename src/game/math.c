/* math.c - TRIG CALCULATION & Z PROJECTION, NUMERIC ROUTINES
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- TRIG CALCULATION & Z PROJECTION ---- */

/* GET SINES & COSINE FOR QUAD/ANGLE */
/* ENTRY   EXIT */
/* ACC QUAD (XXXX XQQA) BLITZED */
/* X DC   BLITZED */
/* Y ANGLE (AAAA AA.FF) BLITZED */
/* BLITZES-TEMP1,TEMP3,MBOX */
/* CALLS-T.SINE,D.COMP */
/* PFTRIG (6EC7) */
void rb_pftrig(rb_machine *m)
{
    LDA(RD(RB_PFROTN + 1));                 /* PF ANGLES */
    LDY(RD(RB_PFROTN));
    rb_trig(m);                             /* (falls into it) */
}

/* TRIG (6ECB) */
void rb_trig(rb_machine *m)
{
    LSRA();
    AND(3);
    TAX();                                  /* QUAD */
    TYA();
    RORA();
    LSRA();
    TAY();                                  /* ANGLE */
    LDA(RD(AX(RB_QUADSN)));                 /* COSINE/SINE SIGNS */
    STA(RB_TEMP3);                          /* D7=COSINE SIGN, D6=SINE SIGN */
    TXA();
    LSRA();                                 /* CARRY=D0 OF QUADRANT */
    TYA();
    if (BCC) goto l10;                      /* EVEN QUADRANT (ANGLE=ANGLE) */
    STY(RB_TEMP1);
    LDA(0x80);                              /* ELSE ANGLE=-(ANGLE) */
    SBC(RD(RB_TEMP1));                      /* CARRY SET */
l10:
    STA(RB_TEMP1);
    rb_t_sine(m);
    BIT(RD(RB_TEMP3));                      /* D6=SIGN OF SINE */
    if (BVC) goto l20;                      /* PLUS */
    rb_d_comp(m);                           /* ELSE 2'S COMP SINE */
l20:
    STX(RB_MM_BM);                          /* SET MATRIX +SINE */
    STY(RB_MM_BL);
    SEC();
    LDA(0x80);
    SBC(RD(RB_TEMP1));
    rb_t_sine(m);
    BIT(RD(RB_TEMP3));                      /* D7=SIGN OF COSINE */
    rb_dpabs(m);                            /* ELSE 2'S COMP COSINE */
    STX(RB_MM_AM);                          /* SET MATRIX COSINES */
    STY(RB_MM_AL);
    return;
}

/* ACC = ANGLE (0 TO 7F) */
/* EXIT - X,Y = MSB,LSB OF SINE OF ANGLE */
/* BLITZES-TEMP2,+1 */
/* T.SINE (6F09) */
void rb_t_sine(rb_machine *m)
{
    LSRA();
    if (BCC) goto l20;                      /* JUST GET FRACTION */
    ASLA();                                 /* ELSE EXTRAPOLATE BETWEEN */
    TAX();
    ADC(2);                                 /* ADD 2 (CARRY CLR) */
    TAY();
    SEC();
    LDA(RD(AY(RB_SINE)));
    SBC(RD(AX(RB_SINE)));
    STA(RB_TEMP2);
    LDA(RD(AY(RB_SINE + 1)));
    SBC(RD(AX(RB_SINE + 1)));
    CMP(0x80);                              /* MAINTAIN SIGN */
    RORA();                                 /* EXTRAPOLATION = 0.5 * DIFF */
    ROR(RB_TEMP2);
    STA(RB_TEMP2 + 1);
    CLC();
    LDA(RD(AX(RB_SINE)));                   /* GET SINE */
    ADC(RD(RB_TEMP2));                      /* ADD EXTRAPOLATION */
    TAY();
    LDA(RD(AX(RB_SINE + 1)));
    ADC(RD(RB_TEMP2 + 1));
    TAX();
    return;                                 /* X,Y = SINE */
l20:
    ASLA();
    TAX();
    LDY(RD(AX(RB_SINE)));
    LDA(RD(AX(RB_SINE + 1)));
    TAX();
    return;                                 /* X,Y = SINE */
}


/* ---- NUMBERIC ROUTINES (* / COMP) ---- */

/* MULTIPLY ROUTINE */
/* ENTRY   EXIT */
/* ACC MULTIPLIER  BLITZED */
/* X MULTIPLICAND(SIGNED) PRODUCT (MSB) */
/* Y D.C.   PRODUCT (LSB) */
/* MULTPL (6F3F) */
void rb_multpl(rb_machine *m)
{
    EOR(0xFF);
    STA(RB_TEMP2);                          /* LOW BITS=ADDITION */
    LDY(0);
    STY(RB_RESULT);                         /* CLR RESULTUCT */
    STY(RB_RESULT + 1);
    TXA();
    if (BPL) goto l10;
    DEY();
l10:
    STY(RB_TEMP2 + 1);                      /* SIGN EXTEND PRODUCT */
    LDY(8);                                 /* COUNTER */
l20:
    ASL(RB_TEMP2);
    if (BCS) goto l30;                      /* NO ADDITION */
    TXA();
    ADC(RD(RB_RESULT));
    STA(RB_RESULT);
    LDA(RD(RB_TEMP2 + 1));
    ADC(RD(RB_RESULT + 1));
    STA(RB_RESULT + 1);
l30:
    DEY();
    if (BEQ) goto l40;
    ASL(RB_RESULT);                         /* MULTIPLY BY 2 */
    ROL(RB_RESULT + 1);
    goto l20;                               /* REPEAT FOR 8 BITS */
l40:
    LDX(RD(RB_RESULT + 1));                 /* X,Y =PRODUCT */
    LDY(RD(RB_RESULT));
    return;
}

/* INITIALIZE X/Y TRANSLATES TO ZERO */
/* TRNS0 (6F6F) */
void rb_trns0(rb_machine *m)
{
    LDA(0);
    LDY(3);
l10:
    STA(AYW(RB_MM_EL));
    DEY();
    if (BPL) goto l10;
    return;
}

/* MRSULT RETURNS RESULT FROM MATRIX MULTIPLY */
/* ENTRY  EXIT */
/* ACC DC  Y RESULT (LSB) */
/* X DC  Y RESULT (MSB) */
/* Y DC  UNTOUCHED */
/* MRSAB0 (6F7A) */
void rb_mrsab0(rb_machine *m)
{
    rb_trns0(m);
    rb_mrslt0(m);                           /* (falls into it) */
}

/* MRSLT0 (6F7D) */
void rb_mrslt0(rb_machine *m)
{
    STX(RB_MM_YM);
    rb_mrsult(m);                           /* (falls into it) */
}

/* MRSULT (6F80) */
void rb_mrsult(rb_machine *m)
{
    MATSTT();
    LDA(RD(RB_M_PRDL));                     /* GET RESULT (X PRIME) */
    LDX(RD(RB_M_PRDM));
    STA(RB_M_SYM);                          /* START 2ND MM */
    STA(RB_RESULT);
    STX(RB_RESULT + 1);
    MATSTT();
    LDA(RD(RB_M_PRDL));
    STA(RB_RESULT + 2);                     /* STORE RESULT (Y PRIME) */
    LDX(RD(RB_M_PRDM));
    STX(RB_RESULT + 3);
    return;
}

/* 16 BIT SIGNED(DIVIDEND) BY SIGNED(DIVISOR) DIVIDE */
/* ENTRY   EXIT */
/* ACC DC   QUOTIENT (MSB) */
/* X DIVIDEND(MSB)  QUOTIENT (MSB) */
/* Y DIVIDEND(LSB)  QUOTIENT (LSB) */
/* DDIVIT (6FA2) */
void rb_ddivit(rb_machine *m)
{
    STY(RB_D_NMHL);                         /* STORE DIVIDEND */
    STX(RB_D_NMHH);
    rb_ddiv10(m);                           /* (falls into it) */
}

/* DDIV10 (6FA8) */
void rb_ddiv10(rb_machine *m)
{
    STA(RB_D_SZXD);                         /* START DIVIDE */
    MATSTT();
    LDY(RD(RB_M_PRDL));                     /* GET QUOTIENT */
    LDX(RD(RB_M_PRDM));
    TXA();
    return;
}

/* DPABS (6FB8) */
void rb_dpabs(rb_machine *m)
{
    if (BPL) { rb_dpcmp0(m); return; }
    rb_d_comp(m);                           /* (falls into it) */
}

/* DO 2 BYTE 2'S COMPLEMENT */
/* ENTRY EXIT */
/* ACC DC  MSB (COMP) */
/* X MSB  MSB (COMP) */
/* Y LSB  LSB (COMP) */
/* D.COMP (6FBA) */
void rb_d_comp(rb_machine *m)
{
    TYA();
    EOR(0xFF);                              /* 1'S COMP LSB */
    TAY();
    TXA();
    EOR(0xFF);                              /* 1'S COMP MSB */
    TAX();
    INY();                                  /* ADD 1 (LSB) */
    if (BNE) { rb_dpcmp0(m); return; }
    INX();                                  /* ADD CARRY (MSB) */
    rb_dpcmp0(m);                           /* (falls into it) */
}

/* DPCMP0 (6FC6) */
void rb_dpcmp0(rb_machine *m)
{
    TXA();                                  /* SET SIGN BIT */
    return;
}

/* COMPLEMENT ACC (2'S COMPL) */
/* SPABS (6FC8) */
void rb_spabs(rb_machine *m)
{
    if (BPL) { rb_comp_0(m); return; }
    rb_comp(m);                             /* (falls into it) */
}

/* COMP (6FCA) */
void rb_comp(rb_machine *m)
{
    EOR(0xFF);
    CLC();
    ADC(1);
    rb_comp_0(m);                           /* (falls into it) */
}

/* COMP.0 (6FCF) */
void rb_comp_0(rb_machine *m)
{
    return;
    (void)m;
}

/* DIVIDE BY 4 (2 RIGHT SHIFTS) */
/* DIVBY4 (6FD0) */
void rb_divby4(rb_machine *m)
{
    CMP(0x80);
    RORA();
    ROR(RB_TEMP2);
    rb_divby2(m);                           /* (falls into it) */
}

/* DIVBY2 (6FD5) */
void rb_divby2(rb_machine *m)
{
    CMP(0x80);
    RORA();
    ROR(RB_TEMP2);
    return;
}

/* MULTIPLY BY FACTORS OF 2 (LEFT SHIFTS) */
/* MULTB4 (6FDB) */
void rb_multb4(rb_machine *m)
{
    ASLA();
    ROL(RB_TEMP1);
    ASLA();
    ROL(RB_TEMP1);
    return;
}

/* RANDOM NUMBER GENERATOR */
/* ENTRY  EXIT */
/* ACC DC  RANDOM # (LSB) */
/* X DC  UNTOUCHED */
/* Y DC  UNTOUCHED */
/* RANDOM (6FE2) */
void rb_random(rb_machine *m)
{
    rb_rand2(m);                            /* 4 TIMES */
    rb_rand2(m);                            /* (falls into it) */
}

/* RAND2 (6FE5) */
void rb_rand2(rb_machine *m)
{
    rb_rand(m);                             /* 2 TIMES */
    rb_rand(m);                             /* (falls into it) */
}

/* RAND (6FE8) */
void rb_rand(rb_machine *m)
{
    ASL(RB_POLYL);
    ROL(RB_POLYH);
    if (BPL) goto l1;
    INC(RB_POLYL);
l1:
    LDA(RD(RB_POLYL));                      /* CHANGE BIT 0 DEPENDING ON BIT 1 */
    BIT(RD(RB_BIT1));
    if (BEQ) goto l2;
    EOR(1);
    STA(RB_POLYL);
l2:
    ORA(RD(RB_POLYH));                      /* BEWARE 0 CASE */
    if (BNE) goto l3;
    INC(RB_POLYL);
l3:
    LDA(RD(RB_POLYL));
    return;
}

/* BIT MASKS */
/* 7004 BIT6: .BYTE 40 */

/* ---- PLANE POINTS DB ---- */

/* PLANE POINTS */
/* 7005 DB.PLN: .BYTE 40,0*2,0*4 */
/* 7008  .BYTE 40,-16*2,0*4 */
/* 700B  .BYTE 40,16*2,0*4 */
/* 700E  .BYTE 40,0*2,16*4 */
/* 7011  .BYTE -36,-4*2,8*4 */
/* 7014  .BYTE -36,-8*2,4*4 */
/* 7017  .BYTE -36,-8*2,-4*4 */
/* 701A  .BYTE -36,-4*2,-8*4 */
/* 701D  .BYTE -36,4*2,-8*4 */
/* 7020  .BYTE -36,8*2,-4*4 */
/* 7023  .BYTE -36,8*2,4*4 */
/* 7026  .BYTE -36,4*2,8*4 */
/* 7029  .BYTE -40,-40*2,20*4 */
/* 702C  .BYTE -40,40*2,20*4 */
/* 702F  .BYTE -36,-40*2,-8*4 */
/* 7032  .BYTE -36,40*2,-8*4 */
/* 7035  .BYTE 18,0*2,0*4 */
/* 7038  .BYTE -28,-6*2,20*4 */
/* 703B  .BYTE -24,-3*2,7*4 */
/* 703E  .BYTE -28,6*2,20*4 */
/* 7041  .BYTE -24,3*2,7*4 */
/* 7044  .BYTE -24,4*2,-8*4 */
/* 7047  .BYTE -20,8*2,-16*4 */
/* 704A  .BYTE -24,-4*2,-8*4 */
/* 704D  .BYTE -20,-8*2,-16*4 */
/* 7050  .BYTE -24,7*2,-20*4 */
/* 7053  .BYTE -24,9*2,-12*4 */
/* 7056  .BYTE -24,-7*2,-20*4 */
/* 7059  .BYTE -24,-9*2,-12*4 */
/* BACK FACES */
/* 705C P.BACK: .BYTE -8,-40*2,20*4 */
/* 705F  .BYTE -8,40*2,20*4 */
/* 7062  .BYTE -4,-40*2,-8*4 */
/* 7065  .BYTE -4,40*2,-8*4 */
/* 7068  .BYTE -20,-6*2,20*4 */
/* 706B  .BYTE -20,6*2,20*4 */
/* 706E  .BYTE -16,4*2,-8*4 */
/* 7071  .BYTE -16,-4*2,-8*4 */
/* 7074  .BYTE -16,7*2,-20*4 */
/* 7077  .BYTE -16,9*2,-12*4 */
/* 707A  .BYTE -16,-7*2,-20*4 */
/* 707D  .BYTE -16,-9*2,-12*4 */
/* 7080  .BYTE -36,0*2,0*4 */
/* 7083 BLCOLL: .BYTE -40,16*2,16*4 */
/* 7086  .BYTE -40,16*2,-16*4 */
/* 7089  .BYTE -40,-16*2,16*4 */
/* 708C  .BYTE -40,-16*2,-16*4 */
/* 708F  .BYTE 40,16*2,16*4 */
/* 7092  .BYTE 40,16*2,-16*4 */
/* 7095  .BYTE 40,-16*2,16*4 */
/* 7098  .BYTE 40,-16*2,-16*4 */
/* 709B PPNTS: .BYTE .PLPNT-3,.DBPRP-3,.COLLD-3 */
/* 709E PLNDB: .WORD DB.PLN,DBPROP,COLLD,PIECE0 */
/* 70A6  .WORD PIECE1,PIECE2,PIECE3,BLIMP,EXCRCL */
/* 70B0  .WORD BLCOLL,STAR0,STAR1 */

/* ---- CHKSUM 7 (7000-77FF) [FF] ---- */

/* 70B6 CHKSM7: .BYTE 28 */
