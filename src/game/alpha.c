/* alpha.c - ALPHANUMERICS
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- ALPHANUMERICS ---- */

/* ALPHAS (698F) */
void rb_alphas(rb_machine *m)
{
    LDA(RD(RB_OPTION));
    AND(3);
    STA(RB_LANG);                           /* OPTIONS DEFINE LANGUAGE */
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDA(-316 / 4);
    LDX(320 / 4);
    LDY(0);
    rb_vgvtr(m);                            /* POSITION BEAM */
    LDX(RB_SCORE + 1);
    RB_TAGGED(RB_TAG_SCORE, rb_digt1s(m));
    RB_TAGGED(RB_TAG_SCORE, rb_alph_0(m));
    BIT(RD(RB_GRMODE));
    if (BMI) goto l10;
    LDA(160 / 4);
    LDX(0);
    rb_vgvtr1(m);                           /* POSITION BEAM FOR PLANE'S VALUE */
    LDA(RD(RB_PLVALU));
    rb_decimal(m);
    LDX(RD(RB_PLSTAT + 5));
    CPX(0x10);
    if (BCC) goto l5;
    LDA(0x30);                              /* PLANE BONUS VALUE */
    STA(RB_NUMB1);
l5:
    LDY(0);
    LDX(RB_NUMB1);
    RB_TAGGED(RB_TAG_VALUE, rb_digtys(m));
    RB_TAGGED(RB_TAG_VALUE, rb_alph_0(m));
l10:
    rb_vgcntr(m);
    LDA(204 / 4);
    LDX(336 / 4);
    rb_vgvtr1(m);                           /* POSITION BEAM FOR HIGH SCORE */
    LDA(2);
    rb_vgsca1(m);
    LDA(RD(RB_HSCRES));
    STA(RB_TEMP3);
    LDA(RD(RB_HSCRES + 1));
    STA(RB_TEMP3 + 1);
    LDX(RB_TEMP3 + 1);
    RB_TAGGED(RB_TAG_HISCORE, rb_digt1s(m));
    RB_TAGGED(RB_TAG_HISCORE, rb_alph_0(m));
    LDY(0);
    RB_TAGGED(RB_TAG_HISCORE, rb_dspalp(m));
    LDX(0);
    RB_TAGGED(RB_TAG_HISCORE, rb_hsintl(m));
    LDA(1);
    rb_vgsca1(m);
    LDA(24);                                /* PLAYER DATA STRINGS */
    RB_TAGGED(RB_TAG_LABEL, rb_message(m));
    BIT(RD(RB_GRMODE));
    if (BMI) goto l20;
    LDA(26);
    RB_TAGGED(RB_TAG_LABEL, rb_message(m));
l20:
    LDA(44);
    RB_TAGGED(RB_TAG_LABEL, rb_message(m));
    BIT(RD(RB_GUN_ST));                     /* OVERHEATED GUNS ? */
    if (BVC) goto l25;                      /* NO */
    LDA(16);                                /* ELSE DISPLAY MESSAGE */
    RB_TAGGED(RB_TAG_WARN, rb_message(m));
l25:
    BIT(RD(RB_GAMODE));
    if (BMI) goto l60;                      /* PLAY MODE */
    LDA(RD(RB_GAMODE));
    AND(4);
    if (BNE) goto l60;                      /* BNE 94$ */
    LDA(32);
    RB_TAGGED(RB_TAG_CREDITS, rb_message(m));
    LDA(RD(RB_CRDT));                       /* # OF CREDITS */
    CMP(50);
    if (BCC) goto l35;
    LDA(50);
l35:
    rb_decimal(m);
    LDX(RB_NUMB1);
    LDY(0);
    RB_TAGGED(RB_TAG_CREDITS, rb_digtys(m));
    LDA(RD(RB_CNCT));                       /* ANY HALF CREDITS ? */
    if (BEQ) goto l40;                      /* NO */
    LDX(RB_HALF & 0xFF);
    LDA(RB_HALF / 0x100);
    RB_TAGGED(RB_TAG_CREDITS, rb_vgjsrl(m)); /* DISPLAY 1/2 */
l40:
    BIT(RD(RB_GAMODE));
    if (BVC) goto l45;
    goto l90;                               /* RTP */
l45:
    LDA(RD(RB_OPTION));
    AND(0x40);
    if (BEQ) goto l50;
    rb_vgcntr(m);
    LDX(RD(RB_LANG));
    LDA(RD(AX(RB_PLY2MN)));
    LDX(112 / 4);
    rb_vgvtr1(m);
    LDA(34);
    RB_TAGGED(RB_TAG_ATTRACT, rb_message(m));
    LDA(10);
    RB_TAGGED(RB_TAG_ATTRACT, rb_message(m));
    LDA(14);
    RB_TAGGED(RB_TAG_ATTRACT, rb_message(m));
l50:
    LDA(RD(RB_GAMODE));
    AND(8);
    if (BEQ) goto l55;
    LDA(RD(RB_FRAME));
    AND(8);
    if (BNE) goto l60;                      /* BNE 94$ */
l55:
    rb_vgcntr(m);
    LDX(1);                                 /* ASSUME 1 COIN */
    LDA(RD(RB_CMODE));                      /* D1,D0=COIN MODES */
    AND(3);
    if (BNE) goto l65;
l60:
    goto l94;
l65:
    SEC();
    SBC(1);
    ASLA();
    ASLA();
    STA(RB_TEMP2);
    CMP(8);
    if (BCC) goto l70;                      /* 1 COIN */
    INX();                                  /* ELSE 2 COINS */
l70:
    STX(RB_TEMP3);                          /* TEMP3=# OF COINS */
    LDA(RD(RB_LANG));
    ORA(RD(RB_TEMP2));
    TAY();
    LDA(RD(AY(RB_CMMESS)));                 /* X POSITION OF MESSAGE */
    LDX(152 / 4);                           /* Y POSITION */
    rb_vgvtr1(m);                           /* POSITION BEAM */
    LDX(RB_TEMP3);
    LDY(0);
    RB_TAGGED(RB_TAG_ATTRACT, rb_digtys(m)); /* DISPLAY # OF COINS */
    LSR(RB_TEMP3);
    LDA(4);                                 /* ASSUME COIN */
    if (BCS) goto l75;
    LDA(6);                                 /* COINS */
l75:
    RB_TAGGED(RB_TAG_ATTRACT, rb_message(m)); /* DISPLAY COIN(S) */
    LDX(1);
    LDA(RD(RB_TEMP2));
    if (BNE) goto l80;
    INX();                                  /* X=2 */
l80:
    STX(RB_TEMP3);                          /* TEMP3=# OF PLAYS */
    LDX(RB_TEMP3);
    LDY(0);
    RB_TAGGED(RB_TAG_ATTRACT, rb_digtys(m)); /* DISPLAY # OF PLAYS */
    LSR(RB_TEMP3);
    LDA(8);                                 /* ASSUME PLAY */
    if (BCS) goto l85;
    LDA(10);                                /* ELSE PLAYS */
l85:
    RB_TAGGED(RB_TAG_ATTRACT, rb_message(m)); /* DISPLAY PLAY(S) */
    LDA(RD(RB_FRAME));
    AND(8);                                 /* FLASH INSERT COINS */
    if (BNE) goto l94;
    LDA(2);
    RB_TAGGED(RB_TAG_INSERT, rb_message(m)); /* DISPLAY 'INSERT COINS' */
    goto l94;
l90:
    LDA(RD(RB_CMODE));
    AND(3);                                 /* COIN MODE(S) */
    if (BNE) goto l92;                      /* CREDIT MODES */
    LDA(28);                                /* FREE PLAY */
    RB_TAGGED(RB_TAG_INSERT, rb_message(m));
l92:
    LDA(RD(RB_FRAME));
    AND(4);
    if (BNE) goto l94;
    RB_TAGGED(RB_TAG_INSERT, rb_message(m)); /* DISPLAY 'PRESS START' */
l94:
    LDX(2);                                 /* 3 MESSAGES */
l95:
    STX(RB_SAVX);
    LDA(RD(ZX(RB_MESTM0)));                 /* MESSAGE COUNTER */
    if (BEQ) goto l96;
    DEC(ZX(RB_MESTM0));                     /* DEC COUNTER */
    LSRA();
    if (BCS) goto l96;                      /* FLASH MESSAGE */
    LDA(RD(AX(RB_MESFL)));                  /* MESSAGE NUMBER */
    RB_TAGGED(RB_TAG_BONUS, rb_message(m)); /* DISPLAY MESSAGE */
l96:
    LDX(RD(RB_SAVX));
    DEX();
    if (BPL) goto l95;                      /* DO ALL MESSAGES */
    return;
}

/* 6B0B MESFL: .BYTE 22.,30.,12. */
/* ALPH.0 (6B0E) */
void rb_alph_0(rb_machine *m)
{
    LDX((RB_JSCHR0 & 0xFF00) / 0x100);      /* DISPLAY 0 (ZERO) */
    LDA(RB_JSCHR0 & 0xFF);
    rb_vgadd2(m); return;
}

/* DRNTST (6B15) */
void rb_drntst(rb_machine *m)
{
    LDX((RB__PLPNT - 3) * 2);
l10:
    STX(RB_SAVX);
    LDY(RD(AX(RB_DB_TRP + 2)));
    LDA(RD(AX(RB_DB_TRP + 3)));
    TAX();
    rb_ddivit(m);
    LDX(RD(RB_SAVX));
    STA(AXW(RB_DB_TRP + 3));
    TYA();
    STA(AXW(RB_DB_TRP + 2));
    LDY(RD(AX(RB_DB_TRP + 4)));
    LDA(RD(AX(RB_DB_TRP + 5)));
    TAX();
    rb_ddivit(m);
    LDX(RD(RB_SAVX));
    STA(AXW(RB_DB_TRP + 5));
    TYA();
    STA(AXW(RB_DB_TRP + 4));
    TXA();
    SEC();
    SBC(6);
    TAX();
    if (BCS) goto l10;
    return;
}

