/* objects.c - NEW MOTION OBJECTS, SHELLS AND PLANES; UPDATE OBJECTS
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- NEW MOTION OBJECTS ---- */


/* ---- GENERATE NEW SHELLS ---- */

/* ENTRY  EXIT */
/* ACC DC  BLITZED */
/* X DC  BLITZED */
/* Y DC  BLITZED */
/* NEWSHL (526E) */
void rb_newshl(rb_machine *m)
{
    BIT(RD(RB_SWSTAT));                     /* FIRE BUTTON? */
    if (BPL) { rb_nwshl1(m); return; }      /* NO */
    LDA(RD(RB_S_XFLG));                     /* TOGGLE D6 (ALTERNATE BETWEEN */
    EOR(0x40);                              /* LEFT & RIGHT GUNS) */
    STA(RB_S_XFLG);
    BIT(RD(RB_GUN_ST));
    if (BVC) goto l10;
    return;
l10:
    INC(RB_GUN_ST);                         /* INC GUN USE COUNTER (OVERHEAT) */
    LDY(RD(RB_S_XFLG));                     /* PLAYER SHELLS */
    rb_nwshl0(m);                           /* (falls into it) */
}

/* NWSHL0 (5281) */
void rb_nwshl0(rb_machine *m)
{
    LDX(RB_N_SHL);                          /* ELSE CHECK FOR FREE SHELL */
l5:
    LDA(RD(AX(RB_SHELLS + 7)));             /* AVAILABLE? */
    if (BPL) { rb_setshl(m); return; }      /* YES */
    TXA();
    SEC();
    SBC(RB_L_SHL);
    TAX();
    if (BCS) goto l5;
    return;
}

/* NWSHL1 (5290) */
void rb_nwshl1(rb_machine *m)
{
    SEC();
    LDA(RD(RB_GUN_ST));
    SBC(3);                                 /* GUNS COOL 3 TIMES AS FAST AS HEAT */
    if (BCS) goto l10;
    LDA(0);
l10:
    STA(RB_GUN_ST);                         /* UPDATE GUN STATUS */
    return;
}

/* INITIALIZE SHELL */
/* SETSHL (529C) */
void rb_setshl(rb_machine *m)
{
    BIT(RD(RB_GAMODE));
    if (BPL) goto l5;                       /* SOUNDS ONLY IN PLAY */
    INC(RB_S_VAL);
l5:
    STY(RB_TEMP4);                          /* TEMP4 = PLAYER/ENEMY FLAG (D7) */
    STX(RB_SAVX);
    LDX(RD(RB_PFROTN + 1));                 /* PF ANGLE OF ROTATION MSB */
    LDY(RD(RB_PFROTN));                     /* LSB */
    rb_d_comp(m);                           /* COMPENSATE FOR CURRENT ROTATION */
    rb_trig(m);                             /* SET SINE/COSINE */
    LDX(RB_S_INTX / 0x100);
    LDY(RB_S_INTX & 0xFF);
    BIT(RD(RB_TEMP4));
    if (BVC) goto l20;
    rb_d_comp(m);                           /* D6 = ENEMY X SIDE */
l20:
    STX(RB_MM_XM);                          /* INITIAL X MSB */
    STY(RB_MM_XL);                          /* LSB */
    LDA(RB_S_INTY & 0xFF);
    LDX((RB_S_INTY & 0xFF00) / 0x100);
    BIT(RD(RB_TEMP4));
    if (BPL) goto l25;                      /* PLAYER */
    LDA(RB_E_INTY & 0xFF);
    LDX((RB_E_INTY & 0xFF00) / 0x100);
l25:
    CLC();
    ADC(RD(RB_I4YPOS));
    STA(RB_MM_YL);
    TXA();
    ADC(RD(RB_I4YPOS + 1));
    TAX();
    rb_mrsab0(m);                           /* ROTATE ABOUT 0 */
    STX(RB_TEMP1);
    rb_multb4(m);
    LDX(RD(RB_SAVX));
    STA(AXW(RB_SHELLS + 2));                /* Y LSB */
    LDA(RD(RB_TEMP1));
    STA(AXW(RB_SHELLS + 3));                /* Y MSB */
    LDA(RD(RB_RESULT));
    ASLA();
    ROL(RB_RESULT + 1);
    ASLA();
    ROL(RB_RESULT + 1);
    CLC();
    ADC(RD(RB_UNIV4X));
    STA(AXW(RB_SHELLS));                    /* X LSB */
    LDA(RD(RB_RESULT + 1));
    ADC(RD(RB_UNIV4X + 1));
    STA(AXW(RB_SHELLS + 1));                /* X MSB */
    LDA(0x80);
    BIT(RD(RB_TEMP4));
    if (BPL) goto l30;
    LDA(0xC0);                              /* ENEMY SHELL */
l30:
    STA(AXW(RB_SHELLS + 7));
    LDA(RB_S_DPTH & 0xFF);                  /* INITIALIZE Z (LSB) */
    STA(AXW(RB_SHELLS + 4));
    LDA((RB_S_DPTH & 0xFF00) / 0x100);      /* INITIALIZE Z (MSB) */
    STA(AXW(RB_SHELLS + 5));
    return;
}


/* ---- CREATE NEW PLANE ---- */

/* NWPLNE (5314) */
void rb_nwplne(rb_machine *m)
{
    LDA(RD(RB_PLSTAT + 6));                 /* PLANE #0 */
    AND(0xC0);
    if (BNE) goto l17;                      /* FLIGHT/EXPLOSION */
    DEC(RB_PLSTAT + 7);                     /* DEC PLANE COUNT */
    if (BNE) goto l17;
    LDX(RB_L_OBJ - 4);
l10:
    STA(ZX(RB_PLSTAT));                     /* CLR PLANE DB */
    DEX();
    if (BPL) goto l10;
    BIT(RD(RB_GMEND0));                     /* RETURNING PLANE */
    if (BPL) goto l15;                      /* NO */
    rb_nwenme(m); return;                   /* ELSE - INITIALIZE */
l15:
    LDA(RD(RB_SCRTAB + 3));                 /* WAIT FOR SCORES */
    if (BEQ) goto l20;
    LDA(1);
    STA(RB_PLSTAT + 7);                     /* CHECK NEXT FRAME */
l17:
    return;
l20:
    DEC(RB_NEWCT);                          /* MODE COUNT? */
    if (BNE) goto l22;                      /* NO */
    INC(RB_MODECT);                         /* STEP MODE COUNT */
    LDA(RD(RB_MODECT));
    AND(0xF);
    STA(RB_MODECT);
    LSRA();
    TAX();
    LDA(RD(AX(RB_MCOUNT)));
    if (BCC) goto l21;
    LDA(1);
l21:
    STA(RB_NEWCT);
l22:
    LDA(RD(RB_MODECT));
    LSRA();
    if (BCC) { rb_stplne(m); return; }      /* PLANE WAVE */
    rb_initgr(m); return;                   /* GROUND WAVE */
}

/* STPLNE (5354) */
void rb_stplne(rb_machine *m)
{
    BIT(RD(RB_GAMODE));
    if (BPL) goto l15;                      /* NOT PLAY MODE */
    LDY(1);
l5:
    LDX(RD(ZY(RB_VALFRC)));
    INX();
    CPX(0x19);
    if (BCC) goto l10;
    LDX(0x18);
l10:
    STX(ZY(RB_VALFRC));
    DEY();
    if (BPL) goto l5;
l15:
    rb_random(m);                           /* RANDOM X POSITION */
    TAY();
    LDX(2);
    BIT(RD(RB_PLDELX));
    rb_dpabs(m);
    CLC();
    TYA();
    ADC(RD(RB_UNIV4X));
    STA(RB_PLSTAT);
    TXA();
    ADC(RD(RB_UNIV4X + 1));
    STA(RB_PLSTAT + 1);
    LDY(RB_MAXDEL & 0xFF);                  /* TAIL TOWARDS RIGHT */
    LDX((RB_MAXDEL & 0xFF00) / 0x100);
    STY(RB_PLSTAT + 0xE);                   /* Y DELTA MSB */
    STX(RB_PLSTAT + 0xF);                   /* Y DELTA */
    LDA(0xFE);
    BIT(RD(RB_PLDELX));
    if (BPL) goto l20;
    rb_dpabs(m);
    LDA(2);                                 /* TAIL TOWARDS LEFT */
l20:
    STA(RB_PLSTAT + 0x15);                  /* SET ROTATION (X/Z) 90 DEGREES */
    STX(RB_PLSTAT + 0xD);                   /* X DELTA */
    STY(RB_PLSTAT + 0xC);                   /* X DELTA MSB */
    rb_random(m);                           /* RANDOM Y POSITION */
    CLC();
    ADC(0x80);
    STA(RB_PLSTAT + 2);
    AND(1);
    ADC(1);
    STA(RB_PLSTAT + 3);
    LDY(1 * 8 + 7);
    rb_sndon(m);
    LDA(RB_P_INDP & 0xFF);
    STA(RB_PLSTAT + 4);
    STA(RB_PLSTAT + 0x19);
    LDA(RB_P_INDP / 0x100);                 /* Z DEPTH (MSB) */
    STA(RB_PLSTAT + 5);
    STA(RB_PLSTAT + 0x1A);
    LDA(RD(RB_N_PLNZ));
    CMP(4);
    if (BCC) goto l25;
    rb_random(m);
    AND(0xC);
    if (BNE) goto l25;
    rb_cintbp(m);                           /* RANDOM BLIMP */
l25:
    LDA(0);                                 /* 1 PLANE */
    STA(RB_P_2ST + 6);                      /* CLR 2ND DRONE */
    BIT(RD(RB_BLOBJ + 0x18));               /* IF BLIMP ONLY 1 PLANE */
    if (BMI) goto l35;                      /* BLIMP */
    STA(RB_P_1ST + 6);                      /* CLR 1ST DRONE */
    LDX(RD(RB_SCORE + 1));
    if (BEQ) goto l30;                      /* SCORE < 1000 */
    LDA(3);                                 /* 3 PLANES */
l30:
    LDX(RD(RB_SCORE));
    CPX(0x30);
    if (BCC) goto l35;                      /* SCORE < 300 */
    ORA(1);                                 /* AT LEAST 2 PLANES */
l35:
    STA(RB_N_OBJ);                          /* SET # OF PLANES */
    rb_random(m);
    AND(0x30);
    if (BNE) goto l40;
    STA(RB_N_OBJ);                          /* RANDOM (0.25) ONLY 1 PLANE */
l40:
    LDA(RD(RB_GMLEVL));
    LDX(0xB0);
    CMP(4);
    if (BCC) goto l50;                      /* NORMAL PLANE */
    CMP(5);
    if (BEQ) goto l45;
    rb_random(m);
    LSRA();
    if (BCS) goto l50;                      /* STILL NORMAL */
l45:
    LDX(0xB8);                              /* @ PLAYER */
l50:
    STX(RB_PLSTAT + 6);
    LDA(0x20);
    STA(RB_PLSTAT + 7);                     /* GIVE PLANE ^20 FRAMES TO CROSS CENTER */
    LSR(RB_N_OBJ);                          /* DRONE #1 ? */
    if (BCC) goto l65;                      /* NO */
    LDX(RB_N_MOB);
    LDY(0);                                 /* INITIALIZE DRONE #1 */
    rb_plnxcg(m);
    LDA(2);
    STA(RB_P_1ST + 6);                      /* FORMATION FLIGHT (=2) */
    LDA(RB_DRINZ / 0x100);
    STA(RB_P_1ST + 5);
    LDX(3);
l55:
    LDA(RD(AX(RB_PLANE1)));
    STA(ZX(RB_P_1ST));
    DEX();
    if (BPL) goto l55;
    LSR(RB_N_OBJ);                          /* DRONE #2 ? */
    if (BCC) goto l65;                      /* NO */
    LDX(RB_N_MOB);
    LDY(RB_L_OBJ);
    rb_plnxcg(m);                           /* INITIALIZE DRONE #2 */
    LDA(2);
    STA(RB_P_2ST + 6);                      /* FORMATION FLIGHT (=2) */
    LDA(RB_DRINZ / 0x100);
    STA(RB_P_2ST + 5);
    LDX(3);
l60:
    LDA(RD(AX(RB_PLANE2)));
    STA(ZX(RB_P_2ST));
    DEX();
    if (BPL) goto l60;
l65:
    return;
}


/* ---- CHKSUM 3 (5000-57FF) [FF] ---- */

/* 543E CHKSM3: .BYTE 66 */
/* PLNZD (543F) */
void rb_plnzd(rb_machine *m)
{
    LDA(RD(RB_GAMODE));
    if (BPL) { rb_plnzd0(m); return; }
    INC(RB_N_PLNZ);                         /* COUNT # OF PLANES IN GAME */
    rb_plnzd0(m);                           /* (falls into it) */
}

/* PLNZD0 (5445) */
void rb_plnzd0(rb_machine *m)
{
    LDX(0x10);
    LDA(RD(RB_OBJKLD + 1));
    if (BNE) goto l2;
    LDA(RD(RB_OBJKLD));
    LSRA();
    CMP(0x10);
    if (BCS) goto l2;
    TAX();
l2:
    LDA(RD(AX(RB_PLNLVL)));
    STA(RB_GMLEVL);
    TAX();
    LDA(RD(AX(RB_PLPOSZ)));
    STA(RB_PLSTAT + 0x1B);                  /* PLANE MOTION DEPTH DELTA */
    LDA(RD(RB_N_PLNZ));
    LSRA();
    LDA(0);
    if (BCS) goto l5;
    LDA(0x20);
l5:
    CLC();
    ADC(0x20);
    ADC(RD(RB_PRPDEL));                     /* CARRY CLR (ADD PERCENTAGE DELTA) */
    STA(RB_TEMP1);
    LDA(RD(RB_PRPDEL + 1));                 /* MSB */
    ADC(0);
    STA(RB_TEMP1 + 1);
    LDA(RD(RB_N_PLNZ));
    LDX(-8);
    rb_multpl(m);
    TYA();
    SEC();
    SBC(RD(RB_TEMP1));
    TAY();
    TXA();
    SBC(RD(RB_TEMP1 + 1));
    BIT(RD(RB_GMEND0));
    if (BPL) goto l15;                      /* NORMAL */
    STA(RB_TEMP1);
    TYA();
    rb_multb4(m);
    TAY();
    LDX(RD(RB_TEMP1));
    rb_d_comp(m);                           /* RETURNING */
l15:
    STY(RB_PLSTAT + 0x10);                  /* Z DELTA */
    STA(RB_PLSTAT + 0x11);                  /* Z DELTA */
    return;
}

/* PLNZND (549A) */
void rb_plnznd(rb_machine *m)
{
    STY(RB_MM_YL);
    LDY(3);
l10:
    LDA(RD(AY(RB_PLNSND)));
    STA(AYW(RB_MM_AL));
    DEY();
    if (BPL) goto l10;
    STA(RB_ATGVAL);
    rb_mrsab0(m);
    STX(RB_TEMP2);                          /* MSB */
    LSR(RB_TEMP2);
    RORA();
    ROR(RB_TEMP2);
    RORA();                                 /* RESULT /4 MSB */
    STA(RB_ATGDEL + 1);                     /* SET PLANE CLOSER SOUND(S) */
    LDA(RD(RB_TEMP2));
    RORA();
    AND(0xC0);                              /* /4 LSB */
    STA(RB_ATGDEL);
    LDA(0xF8);
    STA(RB_ATGVAL + 1);
    return;
}

/* BUMP GAME LEVEL */
/* LVLSTP (54C5) */
void rb_lvlstp(rb_machine *m)
{
    BIT(RD(RB_GAMODE));
    if (BPL) goto l10;                      /* NOT IN ATTRACT */
    INC(RB_OBJKLD);
    if (BNE) goto l10;
    INC(RB_OBJKLD + 1);
l10:
    return;
}

/* 54D0 PLNLVL: .BYTE 0,0,0,0,1,2,2,2,3,3,3,4,4,4,4,4,5 */
/* 54E1 PLANE1: .WORD -100,100 */
/* 54E5 PLANE2: .WORD -100,-100 */
/* 54E9 PLPOSZ: .BYTE -4,-10,-20,-30,-40,-50,-60,-70,-80 */
/* 54F2 PLNSND: .WORD 2800,0 */

/* ---- CREATE RETURNING PLANE ---- */

/* NWENME (54F6) */
void rb_nwenme(rb_machine *m)
{
    CLC();
    LDA(RD(RB_I4YPOS));
    ADC(RB__EXPSY & 0xFF);
    STA(RB_PLSTAT + 2);                     /* SET Y POSITION OF PLANE */
    LDA(RD(RB_I4YPOS + 1));
    ADC((RB__EXPSY & 0xFF00) / 0x100);
    STA(RB_PLSTAT + 3);
    LDX((RB__EXPSX & 0xFF00) / 0x100);
    LDY(RB__EXPSX & 0xFF);
    BIT(RD(RB_PLDELX));                     /* ENEMY ENTERS ON SIDE TO INTERCEPT PLAYER */
    rb_dpabs(m);
    TYA();
    CLC();
    ADC(RD(RB_UNIV4X));
    STA(RB_PLSTAT);
    TXA();
    ADC(RD(RB_UNIV4X + 1));
    STA(RB_PLSTAT + 1);
    rb_plnzd(m);
    LDA(0x80);
    STA(RB_PLSTAT + 6);                     /* ENABLE PLANE */
    LDA(1);
    STA(RB_PLSTAT + 5);
    LDA(RD(RB_GMEND0));
    AND(0x81);
    STA(RB_GMEND0);                         /* SET FLAG */
    LSRA();
    if (BCC) goto l10;                      /* NOT EOL */
    BIT(RD(RB_GAMODE));
    if (BPL) goto l10;                      /* ONLY IN PLAY */
    LDA(0x80);
    STA(RB_GMEND1);                         /* EOL */
l10:
    return;
}


/* ---- UPDATE OBJECTS (POSITION, ROTATION, & STATUS) ---- */

/* PLANE/DRONE MOTION */
/* PLMOTN (5534) */
void rb_plmotn(rb_machine *m)
{
    LDX(RB_N_MOB);                          /* CHECK ALL 3 PLANES */
l10:
    STX(RB_SAVX + 1);
    TXA();
    if (BNE) goto l12;                      /* ONLY PLANE/DRONE */
    LDA(RD(ZX(RB_PLOBDB + 0x18)));
    if (BMI) goto l20;                      /* BLIMP */
l12:
    LDA(RD(ZX(RB_PLOBDB + 6)));
    if (BMI) goto l17;                      /* PLANE FLIGHT (D7) */
    ASLA();
    if (BMI) goto l16;                      /* PLANE EXPLOSION (D6) */
    LSRA();
    LSRA();
    if (BCC) goto l20;                      /* ABSENT/DRONE EXPLOSION */
    rb_updrne(m);                           /* UPDATE DRONE */
    goto l20;
l16:
    LDA(RD(RB_GMEND1));
    AND(0x10);
    if (BNE) goto l20;                      /* EOL */
    rb_updpln(m);
    rb_upplex(m);
    goto l20;
l17:
    AND(0x20);                              /* (D5) */
    if (BEQ) goto l18;                      /* ALREADY ROTATED & CENTER CROSSED */
    rb_formtn(m);
    LDX(RD(RB_SAVX + 1));
l18:
    LDA(RD(RB_PROP_F));
    AND(0x7F);                              /* ALLOW PLANE PROP */
    STA(RB_PROP_F);
    rb_updpln(m);                           /* CHECK FOR ATTACK (OR @ 17$) */
    rb_updmob(m);
    rb_plonsn(m);
l20:
    LDA(RD(RB_SAVX + 1));
    SEC();
    SBC(RB_L_OBJ);
    TAX();
    if (BCS) goto l10;
    return;
}


/* ---- UPDATE PLANE POSITION AND ROTATION ---- */

/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X DC   BLITZED */
/* Y DC   BLITZED */
/* UPDPLN (5580) */
void rb_updpln(rb_machine *m)
{
    LDA(RD(RB_PLSTAT + 0xC));
    STA(RB_TEMP2);
    LDA(RD(RB_PLSTAT + 0xD));
    rb_divby4(m);
    TAY();
    LDA(RD(RB_TEMP2));
    CLC();
    ADC(RD(RB_PLSTAT));                     /* UPDATE X POSITION */
    STA(RB_PLSTAT);
    TYA();
    ADC(RD(RB_PLSTAT + 1));                 /* MSB BYTE */
    STA(RB_PLSTAT + 1);
    LDA(RD(RB_PLSTAT + 0xE));
    STA(RB_TEMP2);
    LDA(RD(RB_PLSTAT + 0xF));
    rb_divby4(m);
    TAY();
    LDA(RD(RB_TEMP2));
    CLC();
    ADC(RD(RB_PLSTAT + 2));                 /* UPDATE Y POSITION */
    STA(RB_TEMP2);
    TYA();
    ADC(RD(RB_PLSTAT + 3));                 /* MSB BYTE */
    TAY();
    LDA(RD(RB_TEMP2));
    CMP(RB_PFPLOW & 0xFF);
    TYA();
    SBC(RB_PFPLOW / 0x100);
    if (BCS) goto l10;
    LDA(RB_PFPLOW & 0xFF);
    LDY(RB_PFPLOW / 0x100);
    if (BPL) goto l20;                      /* BLIMP */
l10:
    LDA(RD(RB_TEMP2));
    CMP(RB_PFPHI & 0xFF);
    TYA();
    SBC(RB_PFPHI / 0x100);
    LDA(RD(RB_TEMP2));
    if (BCC) goto l20;
    LDY(RB_PFPHI / 0x100);
    LDA(RB_PFPHI & 0xFF);
l20:
    STY(RB_PLSTAT + 3);
    STA(RB_PLSTAT + 2);
    return;
}

/* UPDMOB (55CE) */
void rb_updmob(rb_machine *m)
{
    LDY(RD(RB_PLSTAT + 0xC));
    LDX(RD(RB_PLSTAT + 0xD));
    rb_dpabs(m);
    CPY(RB_P_MAXR & 0xFF);
    SBC((RB_P_MAXR & 0xFF00) / 0x100);
    LDY(RD(RB_PLSTAT + 0xC));
    LDX(RD(RB_PLSTAT + 0xD));
    if (BCC) goto l5;
    LDY(RB_P_MAXR & 0xFF);
    LDX((RB_P_MAXR & 0xFF00) / 0x100);
    BIT(RD(RB_PLSTAT + 0xD));
    rb_dpabs(m);
l5:
    rb_d_comp(m);
    STY(RB_PLSTAT + 0x12);                  /* PLANE X/Y ROTATION=-4*DELTA X */
    STX(RB_PLSTAT + 0x13);
    LDA(RD(RB_PLSTAT + 6));
    AND(0x10);
    if (BEQ) goto l25;                      /* PLANE ROTATED AWAY (X/Z ROTATION) */
    LDA(RD(RB_PLSTAT + 0x15));
    if (BMI) goto l15;
    ORA(RD(RB_PLSTAT + 0x14));
    if (BEQ) goto l25;                      /* ROTATED */
    LDA(-0x40);
    LDY(-1);
    if (BMI) goto l20;                      /* ALWAYS */
l15:
    LDA(0x40);
    LDY(0);
l20:
    CLC();
    ADC(RD(RB_PLSTAT + 0x14));
    STA(RB_PLSTAT + 0x14);
    TYA();
    ADC(RD(RB_PLSTAT + 0x15));
    STA(RB_PLSTAT + 0x15);
    return;
l25:
    LDA(RD(RB_PLSTAT + 6));
    AND(0xEF);
    STA(RB_PLSTAT + 6);                     /* D4=0 (PLANE FACING AWAY) */
    rb_frepar(m);                           /* FREE PARALLEL DRONES */
    LDA(RD(RB_PLSTAT + 0xB));               /* Y POSITION/4 */
    LDX(RD(RB_PLSTAT + 0xA));
    STX(RB_TEMP2);
    rb_divby4(m);
    TAX();
    LDY(RD(RB_TEMP2));
    rb_d_comp(m);
    STX(RB_PLSTAT + 0x17);                  /* DEFINED Y/Z ROTATION */
    STY(RB_PLSTAT + 0x16);
    LDA(RD(RB_PLSTAT + 6));
    AND(0x30);                              /* CHECK CENTER WINDOW CROSSING & ROTATION (X/Z) */
    if (BEQ) goto l35;                      /* YES */
    goto plndel;
l35:
    CLC();
    LDA(RD(RB_PLSTAT + 4));                 /* UPDATE PLANE MOTION DEPTH */
    ADC(RD(RB_PLSTAT + 0x10));
    STA(RB_PLSTAT + 4);
    LDA(RD(RB_PLSTAT + 5));
    ADC(RD(RB_PLSTAT + 0x11));
    STA(RB_PLSTAT + 5);
    BIT(RD(RB_GMEND0));                     /* NORMAL PLANE */
    if (BPL) goto l50;                      /* YES */
    LDA(RD(RB_PLSTAT + 0x1B));
    rb_comp(m);
    rb_multb4(m);
    CLC();
    ADC(RD(RB_PLSTAT + 0x19));
    STA(RB_PLSTAT + 0x19);
    LDA(RD(RB_TEMP1));
    AND(3);
    ADC(RD(RB_PLSTAT + 0x1A));
    STA(RB_PLSTAT + 0x1A);
    LDX(0);                                 /* RETURNING PLANE */
    STX(RB_PLVALU);
    LDA(RD(RB_PLSTAT + 5));
    CMP(RB_P_MNDM);                         /* MIN DEPTH? */
    if (BCC) goto l40;                      /* NO */
    LDX(0);
    STX(RB_PLSTAT + 0x19);
    BIT(RD(RB_GMEND1));
    if (BMI) goto l45;
    STX(RB_GMEND0);                         /* CLR RETURNING (X=0) */
    rb_plnzd(m);
    rb_plnznd(m);                           /* PLANE SOUND DELTAS */
    LDA(0x10);
    STA(RB_PLSTAT + 0x1A);                  /* INITIALIZE DEPTH */
l40:
    return;
l45:
    LDA(RB_WO_END);                         /* DISABLE PLANE (WO.END COUNTS) */
    STA(RB_PLSTAT + 7);
    rb_plnclr(m); return;
l50:
    CLC();
    LDA(RD(RB_PLSTAT + 0x19));
    ADC(RD(RB_PLSTAT + 0x1B));
    STA(RB_PLSTAT + 0x19);
    if (BCS) goto l52;
    DEC(RB_PLSTAT + 0x1A);
l52:
    LDA(RD(RB_PLSTAT + 5));
    LDX(RD(RB_VALFRC));                     /* GET SCORE MULTIPLIER */
    rb_multpl(m);                           /* DEPTH*FRAC */
    TXA();
    STY(RB_TEMP2);
    rb_divby4(m);
    rb_divby4(m);
    LDA(RD(RB_TEMP2));
    CMP(RD(RB_VALMIN));
    if (BCS) goto l55;
    LDA(RD(RB_VALMIN));
l55:
    STA(RB_PLVALU);
    LDY(RD(RB_PLSTAT + 4));
    CPY(RB_P_MNDP & 0xFF);                  /* NORMAL PLANE */
    LDA(RD(RB_PLSTAT + 5));
    SBC((RB_P_MNDP & 0xFF00) / 0x100);
    if (BPL) goto plndel;                   /* NOT YET */
    BIT(RD(RB_GMEND0));
    if (BMI) goto p_upd0_l5;
    LDA(0xC0);                              /* ENABLE ENEMY & RETURNING PLANE SHELLS */
    STA(RB_GMEND0);
    LDA(0x10);
    STA(RB_MESTM2);                         /* BEHIND YOU MESSAGE TIMER */
    LDA(RD(RB_PLSTAT + 9));
    AND(0x80);
    STA(RB_ENSIDE);                         /* D7 = SIGN OF X POSITION */
p_upd0_l5:
    LDA(RB_WO_RTN);
    STA(RB_PLSTAT + 7);                     /* DISABLE PLANE FOR WO.RTN FRAMES */
    LDA(0);
    STA(RB_PLVALU);                         /* CLR PLANE VALUE */
    STA(RB_ATGVAL + 1);                     /* INSURE LOW FREQUENCY BACKGROUND SOUND */
    STA(RB_PLSTAT + 6);                     /* CLR PLANE */
    return;
plndel:
    LDA(RD(RB_PLSTAT + 6));
    AND(0x20);
    if (BEQ) goto plndel_l2;
    return;
plndel_l2:
    LDX(2);                                 /* CENTER CROSSED */
    SEC();
    LDA(RD(ZX(RB_PLSTAT + 8)));             /* PLSTAT+10. */
    SBC(RB_HORIZN);
    STA(RB_XPOS);
    LDA(RD(ZX(RB_PLSTAT + 9)));             /* PLSTAT+11. */
    SBC(0);
    STA(RB_XPOS + 1);
p_windw:
    STX(RB_SAVX);                           /* OFFSET FOR Y DELTA */
    LDA(RD(RB_GMLEVL));
    CMP(5);
    if (BCC) goto p_windw_l5;
    LDA(4);
p_windw_l5:
    ASLA();
    STA(RB_SAVY);
    LDA(RD(ZX(RB_PLSTAT + 0xD)));
    STA(RB_FLAG);                           /* SET DELTA DIRECTION FLAG */
    EOR(RD(RB_XPOS + 1));
    STA(RB_FLAG + 1);                       /* POSITION .EOR. DELTA (SIGNS) */
    LDY(RD(RB_XPOS));
    LDX(RD(RB_XPOS + 1));
    rb_dpabs(m);
    STY(RB_TEMP1);                          /* X,Y=ABSOLUTE (POSITION) */
    TYA();
    LDY(RD(RB_SAVY));
    CMP(RD(AY(RB_P_OLIM)));                 /* CHECK OUTER WINDOW (LSB) */
    TXA();
    SBC(RD(AY(RB_P_OLIM + 1)));             /* (MSB) */
    if (BCC) goto p_insd;                   /* INSIDE */
    LDA(RD(RB_PLSTAT + 0x19));
    CMP(4);
    if (BCS) goto p_windw_l10;              /* W/I DEPTH NO RETURN TO SCREEN */
    LDA(RD(RB_FLAG + 1));
    EOR(0xFF);
    STA(RB_FLAG + 1);                       /* RETURN TO WARDS SCREEN CENTER */
p_windw_l10:
    goto p_wchk;
p_insd:
    LDA(RD(RB_TEMP1));
    CMP(RD(AY(RB_P_ILIM)));                 /* CHECK INNER WINDOW (LSB) */
    TXA();
    SBC(RD(AY(RB_P_ILIM + 1)));             /* (MSB) */
    if (BCC) goto p_iner;                   /* INSIDE INNER WINDOW */
    LDA(0xFF);
    STA(RB_FLAG + 1);                       /* NO DIRECTION CHANGES */
    LDA(RB__LEVLS * 2);
    goto p_inr0;                            /* POINT @ INNER WINDOW LIMITS */
p_iner:
    LDA(RD(RB_FLAG + 1));
    EOR(0xFF);
    STA(RB_FLAG + 1);                       /* REVERSE FLAG (HEAD AWAY FROM CENTER) */
    LDA(RB__LEVLS * 4);
p_inr0:
    CLC();
    ADC(RD(RB_SAVY));
    TAY();
    /* CONTINUE TO P.WCHK */
    /* ---- PLANE WINDOW CHECK ---- */
p_wchk:
    LDA(RD(AY(RB_P_ODLX)));
    STA(RB_DELTA);
    LDA(RD(AY(RB_P_ODLX + 1)));
    STA(RB_DELTA + 1);
    LDX(RD(RB_SAVX));
    BIT(RD(RB_FLAG + 1));                   /* DELTA TOWARDS LIMIT */
    if (BPL) goto p_wchk_l40;               /* NEED TO REVERSE DIRECTION */
    LDY(RD(ZX(RB_PLSTAT + 0xC)));
    LDA(RD(ZX(RB_PLSTAT + 0xD)));
    TAX();
    rb_dpabs(m);
    TYA();                                  /* X,Y=ABSOLUTE (DELTA) */
    SEC();
    SBC(RD(RB_DELTA));                      /* CURRENT DELTA-MAX DELTA */
    STA(RB_TEMP1);
    TAY();
    TXA();
    SBC(RD(RB_DELTA + 1));
    TAX();
    ORA(RD(RB_TEMP1));
    if (BEQ) goto p_wchk_l25;               /* CURRENT DELTA=MAX DELTA */
    PHP();                                  /* SAVE CARRY */
    TXA();
    rb_dpabs(m);                            /* ACC,Y=ABSOULTE (DELTA-MAX) */
    CPY(RB_ACCEL + 1);
    SBC(0);
    if (BCS) goto p_wchk_l30;               /* ACCELERATE SO DELTA=MAX */
    PLP();                                  /* > ABSOLUTE (ACCELERATION) */
    LDX(RD(RB_DELTA + 1));                  /* ELSE SET DELTA TO MAX */
    LDY(RD(RB_DELTA));
    BIT(RD(RB_FLAG));
    rb_dpabs(m);
    LDX(RD(RB_SAVX));
    STA(ZX(RB_PLSTAT + 0xD));               /* SET DELTA */
    STY(ZX(RB_PLSTAT + 0xC));
p_wchk_l25:
    LDX(RD(RB_SAVX));
    goto p_witr;
p_wchk_l30:
    PLP();
    LDA(RB_ACCEL);
    if (BCC) goto p_wchk_l35;               /* INCREASE DELTA (+ ACCEL) */
    LDA(-RB_ACCEL);                         /* ELSE DECEASE (- ACCEL) */
p_wchk_l35:
    LDX(RD(RB_SAVX));
    LDY(RD(ZX(RB_PLSTAT + 0xD)));           /* CURRENT DELTA */
    if (BPL) goto p_wchk_l50;               /* PLUS */
    if (BMI) goto p_wchk_l45;               /* ALWAYS */
p_wchk_l40:
    LDA(RB_ACCEL);                          /* CHANGE DIRECTON */
    LDY(RD(ZX(RB_PLSTAT + 0xD)));           /* CURRENT DELTA */
    if (BMI) goto p_wchk_l50;               /* - (ACCEL +) */
p_wchk_l45:
    rb_comp(m);                             /* ELSE + (ACCEL -) */
p_wchk_l50:
    CLC();
    LDY(0);
    ORA(0);                                 /* SET SIGN */
    if (BPL) goto p_wchk_l55;
    DEY();
p_wchk_l55:
    ADC(RD(ZX(RB_PLSTAT + 0xC)));
    STA(ZX(RB_PLSTAT + 0xC));
    TYA();
    ADC(RD(ZX(RB_PLSTAT + 0xD)));
    STA(ZX(RB_PLSTAT + 0xD));
p_witr:
    DEX();
    DEX();
    LDA(RD(ZX(RB_PLSTAT + 8)));             /* X DISPLAY */
    STA(RB_XPOS);
    LDA(RD(ZX(RB_PLSTAT + 9)));
    STA(RB_XPOS + 1);
    TXA();
    if (BMI) goto p_witr_l10;
    goto p_windw;
p_witr_l10:
    return;
}

/* PLONSN (57C1) */
void rb_plonsn(rb_machine *m)
{
    BIT(RD(RB_GMEND0));
    if (BMI) goto l5;                       /* KEEP RETURNING ON SCREEN */
    LDA(RD(RB_N_PLNZ));
    CMP(5);
    if (BCS) goto l50;                      /* ELSE ONLY FIRST FOUR */
l5:
    LDA(RD(RB_PLSTAT + 0x19));              /* PLANE DEPTH */
    STA(RB_MM_AL);
    LDA(RD(RB_PLSTAT + 0x1A));
    STA(RB_MM_AM);
    LDA(0xA0);                              /* SCALE WINDOW SIZE (1A0*1A0) BY DEPTH */
    STA(RB_MM_XL);
    LDX(1);
    STX(RB_MM_XM);
    DEX();                                  /* X=0 */
    STX(RB_MM_YL);
    rb_mrsab0(m);                           /* MULTIPLIED */
    LDA(RD(RB_RESULT));
    STA(RB_RESULT + 1);                     /* RESULT * ^100 */
    LDX(0);
    STX(RB_RESULT);
    STA(RB_MM_XM);
    STX(RB_MM_XL);
    STX(RB_MM_YL);
    LDY(RD(RB_PFROTN));
    LDX(RD(RB_PFROTN + 1));
    rb_d_comp(m);                           /* ROTATE BY INVERSE OF UNIVERSE ROTATION */
    rb_trig(m);
    LDX(RD(RB_RESULT));
    rb_mrslt0(m);                           /* ROTATE WINDOW LIMIT (RESULT,+1=X,Y LIMIT(S)) */
    LDX(2);
l10:
    STX(RB_SAVX);
    SEC();
    LDA(RD(ZX(RB_PLSTAT)));                 /* PLANE POSITION */
    SBC(RD(ZX(RB_UNIV4X)));                 /* - UNIVERSE CENTER */
    TAY();
    LDA(RD(ZX(RB_PLSTAT + 1)));
    SBC(RD(ZX(RB_UNIV4X + 1)));
    STA(RB_TEMP2);
    TAX();
    rb_dpabs(m);                            /* ABSOLUTE OF POSITION ON SCREEN */
    LDX(RD(RB_SAVX));
    CPY(RD(RB_RESULT));                     /* COMPARE TO LIMIT */
    SBC(RD(RB_RESULT + 1));
    if (BCC) goto l40;                      /* PLANE W/I WINDOW */
    LDY(RD(RB_RESULT));                     /* ELSE SET POSITION TO LIMIT */
    LDX(RD(RB_RESULT + 1));
    BIT(RD(RB_TEMP2));
    rb_dpabs(m);
    PHA();
    LDX(RD(RB_SAVX));
    CLC();
    TYA();
    ADC(RD(ZX(RB_UNIV4X)));                 /* ADD TO UNIVERSE CENTER */
    STA(ZX(RB_PLSTAT));                     /* SET PLANE POSITION */
    PLA();
    ADC(RD(ZX(RB_UNIV4X + 1)));
    STA(ZX(RB_PLSTAT + 1));
l40:
    DEX();
    DEX();
    if (BPL) goto l10;
l50:
    return;
}

/* 583E P.OLIM: .WORD 40   ;RETURN TO CENTER LIMIT */
/* 5840  .WORD 80 */
/* 5842  .WORD 120 */
/* 5844  .WORD 1A0 */
/* 5846  .WORD 200 */
/* 5848 P.ILIM: .WORD 20,30,80   ;HEAD AWAY FROM CENTER LIMIT */
/* 584E  .WORD 120,160 */
/* 5852 P.ODLX: .WORD 2*90,2*8C,2*84,2*7C */
/* 585A  .WORD 80*2 */
/* 585C P.IDLX: .WORD 3*8,3*14,3*1C,3*24 */
/* 5864  .WORD 2C*3 */
/* 5866 P.IIDL: .WORD 3*0,3*10,3*18,3*28 */
/* 586E  .WORD 40*3 */

/* ---- EXPLODING PLANE UPDATE ---- */

/* UPPLEX (5870) */
void rb_upplex(rb_machine *m)
{
    CLC();
    LDA(RD(RB_PLSTAT + 0xE));
    ADC(RB_EX_ACY);
    STA(RB_PLSTAT + 0xE);
    LDA(RD(RB_PLSTAT + 0xF));
    ADC((RB_EX_ACY & 0xFF00) / 0x100);
    STA(RB_PLSTAT + 0xF);
    INC(RB_PLSTAT + 7);                     /* STEP EXPLOSION COUNTER */
    LDA(RD(RB_PLSTAT + 7));
    CMP(RB__EXPL1);
    if (BCC) goto l20;
    if (BNE) goto l10;
    LDX(0);
    STX(RB_SPIRAL);                         /* CLR SPIRAL */
    LDX(10);                                /* START EXPLOSION SOUND/PICTURE */
    STX(RB_EXCNTR);
    LDX(0xF0);
    STX(RB_EXPVAL);
l10:
    INC(RB_GTIMER + 1);
    LDX(0);
    STX(RB_PLSTAT + 0x16);
    STX(RB_PLSTAT + 0x17);
    CMP(RB__EXPL2);                         /* FINISHED? */
    if (BCS) { rb_upplx1(m); return; }      /* YES */
    if (BCC) { rb_upplx0(m); return; }      /* ALWAYS */
l20:
    LDY(RD(RB_PLSTAT + 0x16));              /* ROTATE TOWARDS GROUND */
    CPY(1);
    LDA(RD(RB_PLSTAT + 0x17));              /* ABOUT X AXIS */
    SBC(0xFE);
    if (BMI) { rb_upplx0(m); return; }
    SEC();
    TYA();
    SBC(0x60);
    STA(RB_PLSTAT + 0x16);
    if (BCS) { rb_upplx0(m); return; }
    DEC(RB_PLSTAT + 0x17);
    rb_upplx0(m);                           /* (falls into it) */
}

/* UPPLX0 (58B5) */
void rb_upplx0(rb_machine *m)
{
    CLC();
    LDA(RD(RB_PLSTAT + 0x12));              /* ROTATE ABOUT Z AXIS */
    ADC(0x80);
    STA(RB_PLSTAT + 0x12);
    LDA(RD(RB_PLSTAT + 0x13));              /* MSB */
    ADC(1);
    STA(RB_PLSTAT + 0x13);
    return;
}

/* UPPLX1 (58C3) */
void rb_upplx1(rb_machine *m)
{
    BIT(RD(RB_DRNSFL));
    if (BPL) goto l20;                      /* NO DRONES IN FLIGHT (D7) */
    LDX(RD(RB_DRNSET));
    LDA(0);
    STA(RB_DRNSFL);                         /* CLR SELECT FLAG */
    STA(ZX(RB_PLOBDB + 6));                 /* CLR DRONE */
    LDY(RB_N_MOB);
    rb_plnxcg(m);                           /* NEW PLANE=DRONE */
    LDA(0);
    LDX(5);
l5:
    STA(ZX(RB_PLSTAT + 0xC));               /* CLR VELOCITIES/ACCL */
    DEX();
    if (BPL) goto l5;
    LDA(0x20);
    STA(RB_PLSTAT + 7);                     /* TIMEOUT FOR WINDOW CROSS */
    rb_plnzd0(m);                           /* SET DEPTH */
    LDY(1 * 8 + 7);
    rb_sndon(m);                            /* ANNOUNCE PLANE */
    LDA(0xA0);
    LDX(RD(RB_GMLEVL));
    CPX(5);
    if (BCC) goto l10;
    LDA(0xA8);
l10:
    STA(RB_PLSTAT + 6);                     /* PLANE IN FLIGHT */
    return;
l20:
    LDA(0);
    STA(RB_PLSTAT + 6);                     /* CLR PLANE (DRONE(S) EXPLODING) */
    LDA(RB_WO_CNT);
    STA(RB_PLSTAT + 7);                     /* DISABLE FOR COUNTS */
    return;
}

/* PLNSCR (58FF) */
void rb_plnscr(rb_machine *m)
{
    LDA(RD(RB_PLVALU));
    LDX(RD(RB_PLSTAT + 5));
    CPX(0x10);
    if (BCC) { rb_nwscre(m); return; }      /* PLANE PAST BONUS DEPTH */
    rb_drnscr(m);                           /* (falls into it) */
}

/* DRNSCR (5907) */
void rb_drnscr(rb_machine *m)
{
    LDY(3 * 8 + 7);
    rb_sndon(m);
    LDA(RB_DRNPNT);                         /* XTRA POINTS IF DIM */
    rb_nwscre(m);                           /* (falls into it) */
}

/* NWSCRE (590E) */
void rb_nwscre(rb_machine *m)
{
    CMP(RB_DRNPNT);
    if (BNE) goto l10;
    LDX(14);                                /* 300 POINT MESSAGE */
    STX(RB_MESTM1);
l10:
    STA(RB_SCRTAB);                         /* QUEUE SCORE */
    return;
}

/* UPDATE FREE DRONES */
/* UPDRNE (5919) */
void rb_updrne(rb_machine *m)
{
    LDY(4);                                 /* ASSUME DRONE 1 */
    TXA();
    if (BNE) goto l10;
    LDY(0);                                 /* DRONE 0 */
l10:
    STY(RB_SAVY);
    LDA(RD(AY(RB_DRPOST)));                 /* COMPARE X POSITION TO DESTINATION */
    CMP(RD(ZX(RB_PLOBDB + 8)));
    LDA(RD(AY(RB_DRPOST + 1)));
    LDY(0);
    SBC(RD(ZX(RB_PLOBDB + 9)));
    if (BMI) goto l20;                      /* X > DEST */
    LDA(0x10);                              /* ADD 10 */
    if (BPL) goto l30;                      /* ALWAYS */
l20:
    LDA(-0x10);                             /* ADD -10 */
    DEY();                                  /* Y=0FF */
l30:
    CLC();
    ADC(RD(ZX(RB_PLOBDB)));
    STA(ZX(RB_PLOBDB));
    TYA();
    ADC(RD(ZX(RB_PLOBDB + 1)));
    STA(ZX(RB_PLOBDB + 1));
    LDY(RD(RB_SAVY));
    LDA(RD(AY(RB_DRPOST + 2)));             /* COMPARE Y POSITION TO DESTINATION */
    CMP(RD(ZX(RB_PLOBDB + 2)));
    LDA(RD(AY(RB_DRPOST + 3)));
    LDY(0);
    SBC(RD(ZX(RB_PLOBDB + 3)));
    if (BMI) goto l40;                      /* Y > DEST */
    LDA(0x10);                              /* ADD 10 */
    if (BPL) goto l50;
l40:
    LDA(-0x10);                             /* ADD -10 */
    DEY();                                  /* Y=0FF */
l50:
    CLC();
    ADC(RD(ZX(RB_PLOBDB + 2)));
    STA(ZX(RB_PLOBDB + 2));
    TYA();
    ADC(RD(ZX(RB_PLOBDB + 3)));
    STA(ZX(RB_PLOBDB + 3));
    BIT(RD(RB_DRNSFL));                     /* DRONE GROWTH? */
    if (BPL) goto l60;                      /* NO */
    CPX(RD(RB_DRNSET));                     /* SELECTED DRONE? */
    if (BNE) goto l60;                      /* NO */
    LDA(0);
    CMP(RD(ZX(RB_PLOBDB + 4)));
    LDA(0xE);
    SBC(RD(ZX(RB_PLOBDB + 5)));
    if (BCS) goto l60;
    LDA(RD(ZX(RB_PLOBDB + 4)));
    SBC(0x60);
    STA(ZX(RB_PLOBDB + 4));
    if (BCS) goto l60;
    DEC(ZX(RB_PLOBDB + 5));
l60:
    BIT(RD(RB_GMEND1));
    if (BMI) goto l90;
    SEC();
    LDA(RD(ZX(RB_PLOBDB)));
    SBC(RD(RB_UNIV4X));
    TAY();
    LDA(RD(ZX(RB_PLOBDB + 1)));
    SBC(RD(RB_UNIV4X + 1));
    TAX();
    STA(RB_TEMP1);
    rb_dpabs(m);                            /* DRONE SCREEN POSITION */
    CPY(0xC0);
    SBC(5);
    if (BCC) goto l90;
    LDY(0xC0);
    LDX(5);
    BIT(RD(RB_TEMP1));
    rb_dpabs(m);
    STX(RB_TEMP1);
    LDX(RD(RB_SAVX + 1));
    TYA();
    CLC();
    ADC(RD(RB_UNIV4X));
    STA(ZX(RB_PLOBDB));
    LDA(RD(RB_TEMP1));
    ADC(RD(RB_UNIV4X + 1));
    STA(ZX(RB_PLOBDB + 1));
l90:
    return;
}

/* 59B2 DRPOST: .WORD 160,300,-180,380 */
/* X=SOURCE, Y=DESTINATION */
/* PLNXCG (59BA) */
void rb_plnxcg(rb_machine *m)
{
    LDA(RB_L_OBJ - 1);                      /* OBJECT DB LENGTH */
    STA(RB_TEMP1);
l10:
    LDA(RD(ZX(RB_PLOBDB)));                 /* MOVE PLANE STATUS */
    STA(AYW(RB_PLOBDB));                    /* INTO NEW DB */
    INX();
    INY();
    DEC(RB_TEMP1);
    if (BPL) goto l10;
    return;
}


/* ---- UPDATE CENTER OF SCREEN VARIABLES ---- */

/* SET PLANE POSITION FLAG */
/* PFMOTN (59CA) */
void rb_pfmotn(rb_machine *m)
{
    LDY(RD(RB_PLSTAT + 8));                 /* X SCREEN POSITION */
    LDX(RD(RB_PLSTAT + 9));
    rb_dpabs(m);                            /* ABSOLUTE */
    STY(RB_TEMP1);
    STX(RB_TEMP1 + 1);
    LDY(RD(RB_PLSTAT + 0xA));               /* Y SCREEN POSITION */
    LDX(RD(RB_PLSTAT + 0xB));
    rb_dpabs(m);                            /* ABSOLUTE */
    CLC();
    TYA();
    ADC(RD(RB_TEMP1));
    TAY();
    TXA();
    ADC(RD(RB_TEMP1 + 1));
    STA(RB_TEMP1);
    TYA();
    rb_multb4(m);                           /* TEMP2=MSB(Y*4)+MSB(X*4) */
    LDA(RD(RB_PLSTAT + 5));
    ASLA();                                 /* ADD MSB (Z POSITION) */
    CLC();
    ADC(RD(RB_TEMP1));
    TAX();
    LDA(0x20);
    CPX(8 * 3);
    if (BCS) goto l10;
    ASLA();
    CPX(4 * 3);
    if (BCS) goto l10;
    ASLA();
l10:
    BIT(RD(RB_GRMODE));
    if (BPL) goto l20;
    LDA(0x40);                              /* GROUND MODE (SLOW MOTION) */
l20:
    STA(RB_TEMP3);                          /* D7=CLOSE, D6=MIDDLE, D5=FAR */
    LDX(0);
    LDA(RD(RB_PLDELX));
    if (BPL) goto l30;
    DEX();
l30:
    STX(RB_TEMP1);
    rb_multb4(m);
    rb_dischk(m);                           /* DISTANCE/POT CORRECTION */
    STX(RB_PLYRDL);
    STA(RB_PLYRDL + 1);                     /* SAVE PLAYER X DELTA */
    TAY();                                  /* Y=MSB SCALED X DELTA */
    CLC();
    TXA();
    ADC(RD(RB_UNIV4X));                     /* UPDATE UNIVERSE CENTER */
    STA(RB_UNIV4X);
    TYA();
    ADC(RD(RB_UNIV4X + 1));                 /* MSB */
    STA(RB_UNIV4X + 1);
    CLC();
    TXA();
    ADC(RD(RB_PFXSCR));                     /* UPDATE PF CENTER */
    STA(RB_PFXSCR);
    TYA();
    ADC(RD(RB_PFXSCR + 1));                 /* MSB */
    STA(RB_PFXSCR + 1);
    LDX(0);
    LDA(RD(RB_PLDELX));                     /* PLAYER DELTA X */
    if (BPL) goto l40;
    DEX();
l40:
    STX(RB_PFROTN + 1);
    ASLA();
    ASLA();
    ROL(RB_PFROTN + 1);
    ASLA();
    ROL(RB_PFROTN + 1);
    STA(RB_PFROTN);
    TAY();
    LDX(RD(RB_PFROTN + 1));
    rb_dpabs(m);
    CPY(0);
    SBC(1);
    if (BCC) goto l50;
    LDX(1);
    LDY(0);
    BIT(RD(RB_PFROTN + 1));
    rb_dpabs(m);
    STX(RB_PFROTN + 1);
    STY(RB_PFROTN);
l50:
    LDA(RD(RB_PLDELY));
    LDX(RD(RB_PLDELY + 1));
    STX(RB_TEMP1);
    rb_dischk(m);                           /* DISTANCE/POT CORRECTION */
    STX(RB_RESULT + 2);
    STA(RB_RESULT + 3);
    STX(RB_PLYRDL + 2);                     /* Y PLAYER DELTA */
    STA(RB_PLYRDL + 3);
    TXA();
    LDX(RD(RB_RESULT + 3));
    CLC();
    ADC(RD(RB_I4YPOS));                     /* PLAYER Y POSITION */
    TAY();
    TXA();
    ADC(RD(RB_I4YPOS + 1));                 /* MSB OF Y POSITION */
    TAX();
    CPY(RB_PLYMIN & 0xFF);
    SBC(RB_PLYMIN / 0x100);                 /* CHECK MIN */
    if (BPL) goto l60;
    LDY(RB_PLYMIN & 0xFF);                  /* BELOW MIN SET TO MIN */
    LDX(RB_PLYMIN / 0x100);
    if (BPL) goto l70;                      /* ALWAYS */
l60:
    CPY(RB_PLYMAX & 0xFF);                  /* CHECK MAX */
    TXA();
    SBC(RB_PLYMAX / 0x100);
    if (BMI) goto l70;                      /* LESS THAN MAX */
    LDY(RB_PLYMAX & 0xFF);                  /* SET TO MAX */
    LDX(RB_PLYMAX / 0x100);
l70:
    STY(RB_I4YPOS);                         /* SET PLAYER Y POSITION */
    STX(RB_I4YPOS + 1);
    /* JMP PFOBMN  ;CONTINUE TO PFOBMN */
    LDX(RB_N_PFOB);
    LDA(RD(RB_GRNDCT));
    if (BNE) goto pfobmn_l10;
    LDA(0);
pfobmn_l5:
    ORA(RD(AX(RB_PFOBJ + 8)));
    ORA(RD(AX(RB_PFOBJ + 9)));
    ORA(RD(AX(RB_PFOBJ + 0xA)));
    TAY();
    TXA();
    SEC();
    SBC(RB_L_PFOB);
    TAX();
    TYA();
    if (BCS) goto pfobmn_l5;
    LDX(RB_N_PFOB);
    AND(0xC0);                              /* CHECK FOR VISIBLE GROUND OBJECTS */
    if (BNE) goto pfobmn_l10;
    BIT(RD(RB_GRMODE));
    if (BPL) goto pfobmn_l10;
    BIT(RD(RB_GREND));
    if (BMI) goto pfobmn_l10;
    STA(RB_GRMODE);
    rb_pfsoff(m);                           /* CLR GROUND SOUNDS */
    LDA(1);
    STA(RB_PLSTAT + 7);                     /* START PLANES NEXT FRAME */
pfobmn_l10:
    LDA(RD(AX(RB_PFOBJ + 6)));              /* STATUS */
    STX(RB_SAVX);
    if (BPL) goto pfobmn_l15;
    goto pfobmn_l45;                        /* PF ON HORIZON */
pfobmn_l15:
    SEC();                                  /* FREE PF OBJECTS */
    LDA(RD(AX(RB_PFOBJ)));
    SBC(RD(RB_PLYRDL));
    STA(AXW(RB_PFOBJ));
    TAY();
    LDA(RD(AX(RB_PFOBJ + 1)));
    SBC(RD(RB_PLYRDL + 1));
    STA(AXW(RB_PFOBJ + 1));
    rb_wrapit(m);
    if (BCC) goto pfobmn_l20;
    LDX(RD(RB_SAVX));
    STA(AXW(RB_PFOBJ + 1));
    TYA();
    STA(AXW(RB_PFOBJ));
pfobmn_l20:
    LDX(RD(RB_SAVX));
    CLC();
    BIT(RD(RB_GRMODE));                     /* GROUND SEQUENCE ? */
    if (BPL) goto pfobmn_l25;               /* NO */
    LDA(RD(AX(RB_PFOBJ + 2)));              /* DIVISOR */
    STA(RB_DD_DML);
    LDA(RD(AX(RB_PFOBJ + 3)));
    STA(RB_DD_DMM);
    LDY(RD(RB_I4YPOS));                     /* DIVIDEND */
    LDX(RD(RB_I4YPOS + 1));
    rb_ddivit(m);                           /* SCALE */
    SEC();
    TYA();
    SBC(RD(RB_I4YPOS));                     /* SUBTRACT HORIZON POSITION */
    TAY();
    TXA();
    SBC(RD(RB_I4YPOS + 1));
    CPY(0);                                 /* CHECK FOR VISIBILITY */
    SBC(8);
    LDX(RD(RB_SAVX));
    if (BCC) goto pfobmn_l25;               /* STILL VISIBLE */
    rb_pfsclr(m);                           /* CHECK PF SOUND CLR */
    LDA(0x80);
    STA(AXW(RB_PFOBJ + 7));                 /* DISABLE MOUNTAIN RANGE */
    LDA(0);
    STA(AXW(RB_PFOBJ + 8));                 /* CLEAR GROUND OBJECTS */
    STA(AXW(RB_PFOBJ + 9));
    STA(AXW(RB_PFOBJ + 0xA));
pfobmn_l25:
    CLC();
    LDA(0x20);                              /* STANDARD DELTA */
    ADC(RD(RB_PRPDEL));                     /* ADD PERCENTAGING DELTA */
    STA(RB_TEMP1);
    SEC();
    LDA(RD(AX(RB_PFOBJ + 4)));              /* ADD DELTA TO EPTH */
    SBC(RD(RB_TEMP1));
    STA(AXW(RB_PFOBJ + 4));                 /* Y DEPTH LSB */
    STA(AXW(RB_PFOBJ + 2));                 /* X DEPTH LSB */
    TAY();
    LDA(RD(AX(RB_PFOBJ + 5)));              /* MSB DELTA ADD */
    SBC(0);
    STA(AXW(RB_PFOBJ + 5));                 /* Y DEPTH MSB */
    STA(AXW(RB_PFOBJ + 3));
    CPY(0xC0);                              /* MINIMUM DEPTH */
    SBC(1);
    if (BCS) goto pfobmn_l35;               /* NO */
    rb_pfsclr(m);                           /* CHECK PF SOUND CLR */
    LDA(RD(AX(RB_PFOBJ + 6)));              /* ON HORIZON */
    ORA(0x80);
    STA(AXW(RB_PFOBJ + 6));
    LDA(0);
    STA(AXW(RB_PFOBJ + 2));
    STA(AXW(RB_PFOBJ + 4));
    LDA(0);
    STA(AXW(RB_PFOBJ + 7));
    LDA(RB_PF_XZ / 0x100);
    STA(AXW(RB_PFOBJ + 3));
    LDA((RB_P_OBZI & 0xFF00) / 0x100);
    STA(AXW(RB_PFOBJ + 5));
pfobmn_l35:
    TXA();
    SEC();
    SBC(RB_L_PFOB);
    TAX();
    if (BCC) goto pfobmn_l40;
    goto pfobmn_l10;
pfobmn_l40:
    return;
pfobmn_l45:
    LDA(0);
    STA(AXW(RB_PFOBJ + 7));                 /* CLR GROUND SEQUENCE PARAMS */
    STA(AXW(RB_PFOBJ + 8));                 /* CLR EXPLOSION COUNTERS */
    STA(AXW(RB_PFOBJ + 9));
    STA(AXW(RB_PFOBJ + 0xA));
    CLC();                                  /* PF OBJECT STILL PART OF HORIZON */
    LDA(RB_P_OBDZ & 0xFF);                  /* STANDARD DELTA */
    ADC(RD(RB_PRMDEL));                     /* ADD PERCENT DELTA */
    STA(RB_TEMP1);
    LDA(RB_P_OBDZ / 0x100);                 /* MSB */
    ADC(RD(RB_PRMDEL + 1));
    STA(RB_TEMP1 + 1);
    SEC();
    LDA(RD(AX(RB_PFOBJ + 4)));              /* Y DEPTH LSB */
    SBC(RD(RB_TEMP1));                      /* DELTA LSB */
    STA(AXW(RB_PFOBJ + 4));                 /* NEW Y EPTH LSB */
    TAY();
    LDA(RD(AX(RB_PFOBJ + 5)));              /* Y DEPTH MSB */
    SBC(RD(RB_TEMP1 + 1));                  /* DELTA MSB */
    STA(AXW(RB_PFOBJ + 5));                 /* Y DEPTH MSB */
    CPY(RB_P_MAXZ & 0xFF);
    SBC((RB_P_MAXZ & 0xFF00) / 0x100);      /* MAX Y DEPTH ? */
    if (BCS) goto pfobmn_l35;               /* NO */
    LDA(RD(AX(RB_PFOBJ + 6)));              /* ELSE START PF OBJECT 'FALL' */
    AND(0xF);
    STA(AXW(RB_PFOBJ + 6));
    LDY(RD(RB_PFXSCR));
    LDX(RD(RB_PFXSCR + 1));
    rb_d_comp(m);
    STA(RB_TEMP1);
    LDX(RD(RB_SAVX));
    TYA();
    STA(AXW(RB_PFOBJ));
    CLC();
    LDA(-3);
    ADC(RD(AX(RB_PFOBJ + 6)));
    ASLA();
    ASLA();
    CLC();
    ADC(RD(RB_TEMP1));
    AND(0x1F);
    CMP(0x10);
    if (BCC) goto pfobmn_l50;
    ORA(0xF0);                              /* SIGN EXTEND */
pfobmn_l50:
    STA(AXW(RB_PFOBJ + 1));
    BIT(RD(RB_GRMODE));
    if (BPL) goto pfobmn_l60;
    TAY();
    rb_spabs(m);
    DEC(RB_GTIMER);
    if (BMI) goto pfobmn_l55;               /* PF OBJECT TIME-OUT */
    CMP(8);
    if (BCS) goto pfobmn_l60;
pfobmn_l55:
    LDA(RD(RB_GRNDCT));
    if (BEQ) goto pfobmn_l60;
    DEC(RB_GRNDCT);
    STX(RB_PFSNDF);                         /* SET PF SOUND FLAG */
    LDA(0);
    STA(RB_ATGVAL);
    LDA(0xF8);
    STA(RB_ATGVAL + 1);
    LDX(-0x1B);
    LDA(RD(RB_PRPDEL));
    CMP(0x80);
    RORA();
    CLC();
    ADC(0x10);
    rb_multpl(m);
    STX(RB_ATGDEL + 1);
    STY(RB_ATGDEL);
    LDX(RD(RB_SAVX));
    LDA(1);
    STA(RB_GTIMER);
    rb_random(m);
    AND(3);
    STA(RB_TEMP1);
    ASLA();
    ADC(RD(RB_TEMP1));
    STA(AXW(RB_PFOBJ + 7));                 /* RANDOM PF OBJECT GROUPS */
    LDA(0x80);
    STA(AXW(RB_PFOBJ + 8));
    STA(AXW(RB_PFOBJ + 9));
    STA(AXW(RB_PFOBJ + 0xA));
pfobmn_l60:
    LDA(0);
    STA(AXW(RB_PFOBJ + 2));                 /* SET MOUNTAIN DEPTH ( MOTION & DISPLAY ) */
    LDA(0x10);
    STA(AXW(RB_PFOBJ + 3));
    goto pfobmn_l35;
}

/* DISTANCE TO POT CORRECTION CONVERSION */
/* DISCHK (5C44) */
void rb_dischk(rb_machine *m)
{
    TAX();                                  /* X = LSB */
    LDA(RD(RB_TEMP1));                      /* A = MSB */
    STX(RB_TEMP2);                          /* TEMP2 = LSB */
    BIT(RD(RB_TEMP3));                      /* DISTANCE FLAG */
    if (BVS) goto l10;                      /* 0.5 RANGE (0.625 * VALUE) */
    if (BMI) goto l10;                      /* 0.25 RANGE (0.375 * VALUE) */
    TXA();
    ASLA();
    TAX();
    LDA(RD(RB_TEMP1));
    ADC(RD(RB_TEMP1));                      /* 1.0 RANGE (1.0 * VALUE) */
    return;
l10:
    rb_divby4(m);                           /* GET 1/4 * VALUE */
    STA(RB_TEMP2 + 1);
    TXA();
    BIT(RD(RB_TEMP3));
    if (BVS) goto l20;
    SEC();
    SBC(RD(RB_TEMP2));
    TAX();
    LDA(RD(RB_TEMP1));
    SBC(RD(RB_TEMP2 + 1));                  /* ACC = MSB */
    return;
l20:
    CLC();
    ADC(RD(RB_TEMP2));
    TAX();
    LDA(RD(RB_TEMP1));
    ADC(RD(RB_TEMP2 + 1));
    return;
}

/* FREPAR (5C73) */
void rb_frepar(rb_machine *m)
{
    LDX(RB_L_OBJ);
l10:
    rb_frdrne(m);
    TXA();
    SEC();
    SBC(RB_L_OBJ);
    TAX();
    if (BCS) goto l10;
    return;
}

/* FRDRNE (5C80) */
void rb_frdrne(rb_machine *m)
{
    LDA(RD(ZX(RB_PLOBDB + 6)));
    CMP(2);                                 /* PARALLEL */
    if (BNE) goto l10;                      /* NO */
    LSR(ZX(RB_PLOBDB + 6));                 /* FREE DRONE */
    CLC();
    LDA(RD(RB_PLSTAT));
    ADC(RD(ZX(RB_PLOBDB)));                 /* X LSB (OFFSET FROM PLANE) */
    STA(ZX(RB_PLOBDB));                     /* ABSOLUTE LOCATION */
    LDA(RD(RB_PLSTAT + 1));
    ADC(RD(ZX(RB_PLOBDB + 1)));             /* X MSB */
    STA(ZX(RB_PLOBDB + 1));
    CLC();
    LDA(RD(RB_PLSTAT + 2));
    ADC(RD(ZX(RB_PLOBDB + 2)));
    STA(ZX(RB_PLOBDB + 2));                 /* Y LSB (ABSOLUTE) */
    LDA(RD(RB_PLSTAT + 3));
    ADC(RD(ZX(RB_PLOBDB + 3)));
    STA(ZX(RB_PLOBDB + 3));                 /* Y MSB (ABSOLUTE) */
l10:
    return;
}

/* CHECK FOR FORMATION CHANGES */
/* FORMTN (5CA3) */
void rb_formtn(rb_machine *m)
{
    LDY(RD(RB_PLSTAT + 8));
    LDX(RD(RB_PLSTAT + 9));
    rb_dpabs(m);
    CPY(0xE0);
    SBC(0);
    if (BCC) goto l10;
    DEC(RB_PLSTAT + 7);
    if (BNE) goto l20;
l10:
    rb_plnzd(m);
    rb_plnznd(m);                           /* SET SOUND DELTAS */
    LDA(RD(RB_PLSTAT + 6));
    AND(0xDF);
    STA(RB_PLSTAT + 6);
l20:
    return;
}


/* ---- CHKSUM 4 (5800-5FFF) [FF] ---- */

/* 5CC1 CHKSM4: .BYTE 0E8 */
