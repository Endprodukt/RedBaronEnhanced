/* collide.c - COLLISION DETECT, HOUSEKEEPING (I/O)
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- COLLISION DETECT ---- */

/* SET COLLISION WINDOWS FOR PLANE,DRONES, & BLIMP */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X DC   BLITZED */
/* Y DC   BLITZED */
/* CDSSET (6B47) */
void rb_cdsset(rb_machine *m)
{
    rb_pftrig(m);
    LDY(0);                                 /* Y = COLLISION SET FLAG */
    STY(RB_TEMP4);
    BIT(RD(RB_BLOBJ + 0x18));               /* BLIMP ? */
    if (BPL) goto l5;                       /* NO */
    BIT(RD(RB_BLOBJ + 6));
    if (BVS) goto l5;                       /* EXPLOSION */
    LDA(RD(RB_BLOBJ + 4));                  /* BLIMP DEPTH LSB */
    STA(RB_DD_DML);
    STA(RB_O_DPTH);
    LDA(RD(RB_BLOBJ + 5));
    STA(RB_DD_DMM);
    STA(RB_O_DPTH + 1);
    LDX(9);
    LDY(RB__BLCL);
    rb_plnabz(m);                           /* ROTATE ABOUT Z */
    LDA(RD(RB_BLOBJ + 0x14));
    LDY(RD(RB_BLOBJ + 0x13));
    rb_trig(m);
    LDX(RB__BLCL * 2);
    rb_yaxis(m);                            /* ROTATE ABOUT Y */
    LDX(RB__BLCL * 2);
    rb_d_loop(m);
    LDX(RB__BLCL * 2 + 4);
    rb_mnmx0(m);                            /* DETERMINE MIN/MAX OF X,Y&Z */
    LDX(0xB);
l2:
    LDA(RD(AX(RB_DB_TRP)));
    STA(AXW(RB_DB_TRP + 0x20));
    DEX();
    if (BPL) goto l2;
    LDY(4);
    STY(RB_TEMP4);
l5:
    LDA(RD(RB_PLSTAT + 6));                 /* D7 PLANE STATUS */
    if (BPL) goto l10;                      /* NO COLLISION DETECT */
    LDA(RD(RB_PLSTAT + 4));                 /* PLANE DEPTH */
    STA(RB_DD_DML);
    STA(RB_O_DPTH);
    LDA(RD(RB_PLSTAT + 5));                 /* MSB */
    STA(RB_DD_DMM);
    STA(RB_O_DPTH + 1);
    LDX(2);
    rb_pltest(m);                           /* COLLISION FRAME */
    rb_minmax(m);                           /* SET MIN,MAX OF X,Y */
    LDA(RD(RB_TEMP4));
    ORA(1);
    TAY();
l10:
    LDX(0xB + 0xC);                         /* DB.TRP OFFSET */
    /* X = DEST OF DRONE COLLISION FRAME */
    LDA(RD(RB_P_1ST + 6));
    ORA(RD(RB_P_2ST + 6));                  /* DRONE STATS */
    AND(3);
    if (BEQ) goto l20;                      /* NO FLIGHT */
    TYA();
    ORA(2);
    PHA();                                  /* SAVE COLLISION CHECK STATUS */
    LDY(0xB);
l15:
    LDA(RD(AY(RB_DRNFRM)));
    STA(AXW(RB_DB_TRP));                    /* SET DRONE FRAME */
    DEX();
    DEY();
    if (BPL) goto l15;
    PLA();
    TAY();
l20:
    STY(RB_TEMP4);                          /* COLLISION CHECK STATUS */
    return;
}

/* SHCDCK (6BCE) */
void rb_shcdck(rb_machine *m)
{
    LDA(RD(RB_TEMP4));                      /* CHECK FOR PLANE COLLISION */
    LSRA();
    if (BCS) goto l5;
l2:
    goto l40;                               /* NO */
l5:
    LDA(RD(RB_GMEND0));
    AND(3);                                 /* CHECK EOL */
    if (BNE) goto l2;                       /* EOL RETURNING PLANE */
    LDA(0);
    STA(RB_POINTR + 1);
    LDA(RB_PLSTAT + 8);
    STA(RB_POINTR);
    LDY(2);
    STY(RB_SAVY);
    LDA(RD(RB_O_DPTH));
    LDY(RD(RB_O_DPTH + 1));
    rb_colstp(m);                           /* CHECK PLANE/SHELL COLLISION */
    if (BCC) goto l40;                      /* NO COLLISION */
    STA(RB_ATGVAL + 1);                     /* CLR PLANE CLOSE SOUND */
    BIT(RD(RB_PLSTAT + 6));
    if (BVS) goto l38;                      /* ALREADY HIT (SHELL IS DEAD) */
    rb_plnscr(m);
    rb_lvlstp(m);
    LDA(2);
    STA(RB_SPIRAL);                         /* SPIRAL SOUND */
    rb_frepar(m);
    LDX(0);                                 /* DRONE 0 OFFSET */
    LDY(RB_L_OBJ);                          /* DRONE 1 */
    LDA(RD(RB_P_1ST + 6));
    LSRA();                                 /* DRONE 0 IN FLIGHT? */
    if (BCS) goto l10;                      /* YES */
    LDX(RB_L_OBJ);                          /* DISABLE DRONE 0 SELECT */
l10:
    LDA(RD(RB_P_2ST + 6));
    LSRA();                                 /* DRONE 1 IN FLIGHT? */
    if (BCS) goto l15;                      /* YES */
    LDY(0);
    TXA();
    if (BNE) goto l30;                      /* NEITHER DRONE PRESENT */
l15:
    LDA(RD(RB_POLYL));
    LSRA();
    TXA();
    if (BCC) goto l25;
    TYA();
l25:
    STA(RB_DRNSET);                         /* DRONE SELECTED */
    TAX();
    LDA(RD(RB_PLSTAT + 6));
    AND(8);
    ORA(RD(ZX(RB_PLOBDB + 6)));
    STA(ZX(RB_PLOBDB + 6));
    LDA(0xFF);
    STA(RB_DRNSFL);                         /* DRONE GROWTH FLAG */
l30:
    LDY(0);
    rb_random(m);
    if (BPL) goto l35;
    DEY();
l35:
    STA(RB_PLSTAT + 0xC);                   /* RANDOM X DELTA */
    STY(RB_PLSTAT + 0xD);
    LDA(0x40);
    STA(RB_PLSTAT + 6);
    ASLA();                                 /* ACC=80 */
    STA(RB_PLSTAT + 0x10);                  /* DELTA FOR XY ROTATION */
    LDA(0);
    STA(RB_PLSTAT + 7);                     /* INITIALIZE DIVE PIC TIMER */
    STA(RB_PLSTAT + 0x16);
    STA(RB_PLSTAT + 0x17);
    STA(RB_GTIMER + 1);                     /* CLR EXPLOSION PIC TIMER */
    STA(RB_GMEND0);                         /* NEXT PLANE IS NORMAL */
    LDA(1);
    STA(RB_PLSTAT + 0x11);
l38:
    goto l99;                               /* SHELL IS GONE */
l40:
    LDA(RD(RB_TEMP4));                      /* DRONE COLLISION CHECK */
    AND(2);
    if (BEQ) goto l70;                      /* NO */
    LDX(RB_L_OBJ);
l45:
    STX(RB_SAVX + 1);
    LDA(RD(ZX(RB_PLOBDB + 6)));
    AND(3);
    if (BEQ) goto l60;                      /* TRY OTHER DRONE */
    CLC();
    TXA();
    ADC(RB_PLOBDB + 8);
    STA(RB_POINTR);
    LDY(0xE);
    STY(RB_SAVY);                           /* DRONE FRAME OFFSET */
    LDY(0x15);
    LDA(0);                                 /* DRONE DEPTH (LSB) */
    STA(RB_POINTR + 1);
    rb_colstp(m);                           /* CHECK DRONE/SHELL COLLISION */
    if (BCC) goto l60;
    rb_lvlstp(m);
    LDA(0xF0);                              /* START EXPLOSION SOUND */
    STA(RB_EXPVAL);
    LDA(10);
    STA(RB_EXCNTR);
    rb_drnscr(m);                           /* DRONE POINTS */
    LDX(RD(RB_SAVX + 1));
    rb_frdrne(m);                           /* IF PARALLEL, THEN FREE */
    BIT(RD(RB_DRNSFL));
    if (BPL) goto l56;
    CPX(RD(RB_DRNSET));
    if (BNE) goto l56;
    LDY(RB_L_OBJ);
    TXA();
    if (BEQ) goto l50;
    LDY(0);
l50:
    LDX(RD(ZY(RB_PLOBDB + 6)));
    CPX(1);
    if (BNE) goto l52;
    STY(RB_DRNSET);
    if (BEQ) goto l54;                      /* ALWAYS */
l52:
    LDA(0);
    STA(RB_DRNSFL);
l54:
    LDX(RD(RB_SAVX + 1));
l56:
    LDA(4);
    STA(ZX(RB_PLOBDB + 6));                 /* EXPLOSION */
    LDA(1);
    STA(ZX(RB_PLOBDB + 7));                 /* COLLISION OF DRONE */
l60:
    LDA(RD(RB_SAVX + 1));
    SEC();
    SBC(RB_L_OBJ);
    TAX();
    if (BCS) goto l45;                      /* ONLY DRONES */
l70:
    LDA(RD(RB_TEMP4));
    AND(4);
    if (BEQ) goto l99;                      /* NO BLIMP */
    LDA((RB_BLOBJ + 8) & 0xFF);
    STA(RB_POINTR);
    LDA((RB_BLOBJ + 8) / 0x100);
    STA(RB_POINTR + 1);
    LDY(0x22);
    STY(RB_SAVY);                           /* DB.TRP OFFSET */
    LDY(RD(RB_BLOBJ + 5));
    LDA(RD(RB_BLOBJ + 4));
    rb_colstp(m);
    if (BCC) goto l99;                      /* NO COLLISION */
    LDA(RD(RB_BLOBJ + 6));
    ORA(0x40);
    STA(RB_BLOBJ + 6);                      /* BLIMP EXPLOSION */
    LDA(0xF0);
    STA(RB_EXPVAL);
    LDA(10);
    STA(RB_EXCNTR);
    LDA(20);
    rb_nwscre(m); return;                   /* 200 POINTS */
l99:
    return;
}

/* DRONE POINT FRAME */
/* 6CEB DRNFRM: .WORD 0,-0C,-0C */
/* 6CF1  .WORD 200,0C,0C */
/* RETRIEVE MIN/MAX OF POINTS */
/* MINMAX (6CF7) */
void rb_minmax(rb_machine *m)
{
    LDX((RB__COLLD - 3) * 2 + 4);
    rb_mnmx0(m);                            /* (falls into it) */
}

/* MNMX0 (6CF9) */
void rb_mnmx0(rb_machine *m)
{
    TXA();
    SEC();
    SBC(2);
    STA(RB_TEMP1);
l1:
    STX(RB_SAVX + 1);
    LDA(RD(AX(RB_DB_TRP)));
    STA(RB_MIN);
    STA(RB_MAX);
    LDA(RD(AX(RB_DB_TRP + 1)));
    STA(RB_MIN + 1);
    STA(RB_MAX + 1);
l20:
    LDA(RD(RB_MIN));
    CMP(RD(AX(RB_DB_TRP)));
    LDA(RD(RB_MIN + 1));
    SBC(RD(AX(RB_DB_TRP + 1)));
    if (BMI) goto l30;
    LDA(RD(AX(RB_DB_TRP)));                 /* NEW MINIMUM */
    STA(RB_MIN);
    LDA(RD(AX(RB_DB_TRP + 1)));
    STA(RB_MIN + 1);
l30:
    LDA(RD(RB_MAX));
    CMP(RD(AX(RB_DB_TRP)));
    LDA(RD(RB_MAX + 1));
    SBC(RD(AX(RB_DB_TRP + 1)));
    if (BPL) goto l40;
    LDA(RD(AX(RB_DB_TRP)));                 /* NEW MAXIMUM */
    STA(RB_MAX);
    LDA(RD(AX(RB_DB_TRP + 1)));
    STA(RB_MAX + 1);
l40:
    TXA();
    SEC();
    SBC(6);
    TAX();
    if (BCS) goto l20;
    ADC(6);                                 /* CARRY CLR */
    TAX();
    LDA(RD(RB_MIN));
    STA(AXW(RB_DB_TRP));                    /* POINT 0 */
    LDA(RD(RB_MIN + 1));
    STA(AXW(RB_DB_TRP + 1));
    LDA(RD(RB_MAX));
    STA(AXW(RB_DB_TRP + 6));                /* POINT 1 */
    LDA(RD(RB_MAX + 1));
    STA(AXW(RB_DB_TRP + 7));
    LDX(RD(RB_SAVX + 1));
    DEX();
    DEX();
    CPX(RD(RB_TEMP1));
    if (BCS) goto l1;
    return;
}

/* CHECK SHELL TO OBJECT COLLISIONS */
/* DO X,Y,Z (MIN MAX) OF OBJECT FRAME & SHELL INTERSECT */
/* COLSTP (6D62) */
void rb_colstp(rb_machine *m)
{
    LDX(RD(RB_SAVX));                       /* SHELL OFFSET */
    SEC();
    SBC(RD(AX(RB_SHELLS + 4)));
    PHA();                                  /* SAVE LSB RESULT */
    TYA();                                  /* MSB DEPTH */
    SBC(RD(AX(RB_SHELLS + 5)));
    TAX();
    PLA();
    TAY();                                  /* LSB OF RESULT */
    rb_dpabs(m);
    CPY(0x80);
    SBC(0);
    if (BCS) goto l30;                      /* OUTSIDE PLANE (+/- 256.) */
    LDY(0);
    SEC();
    LDA(RD(RB_XPOS));                       /* SHELL X POS */
    SBC(RD(IY(RB_POINTR)));                 /* PLANE X POS */
    STA(RB_XPOINT);                         /* XPOINT = SHELL X - PLANE X */
    INY();
    LDA(RD(RB_XPOS + 1));
    SBC(RD(IY(RB_POINTR)));
    STA(RB_XPOINT + 1);                     /* MSB */
    SEC();
    INY();
    LDA(RD(RB_YPOS));                       /* SHELL Y POS */
    SBC(RD(IY(RB_POINTR)));                 /* PLANE Y POS */
    STA(RB_YPOINT);                         /* YPOINT = SHELL Y - PLANE Y */
    INY();
    LDA(RD(RB_YPOS + 1));
    SBC(RD(IY(RB_POINTR)));
    STA(RB_YPOINT + 1);                     /* MSB */
    LDY(RD(RB_SAVY));                       /* DB.TRP OFFSET (COLLISION FRAME) */
    LDX(2);                                 /* CHECK X AXIS */
l15:
    LDA(RD(ZX(RB_XPOINT)));
    CMP(RD(AY(RB_DB_TRP + 2)));
    LDA(RD(ZX(RB_XPOINT + 1)));
    SBC(RD(AY(RB_DB_TRP + 3)));
    if (BMI) goto l30;                      /* <MIN */
    LDA(RD(AY(RB_DB_TRP + 8)));
    CMP(RD(ZX(RB_XPOINT)));
    LDA(RD(AY(RB_DB_TRP + 9)));
    SBC(RD(ZX(RB_XPOINT + 1)));
    if (BMI) goto l30;                      /* >MAX */
    DEY();
    DEY();
    DEX();
    DEX();
    if (BPL) goto l15;                      /* CHECK X AXIS */
    LDX(RD(RB_SAVX));                       /* W/I MIN/MAX */
    LDA(0);
    STA(AXW(RB_SHELLS + 7));                /* CLR SHELL */
    SEC();
    return;
l30:
    CLC();                                  /* NO COLLISION */
    return;
}


/* ---- HOUSEKEEPING (I/O) ---- */

/* POT SCALING ROUTINE */
/* POTSCL (6DC5) */
void rb_potscl(rb_machine *m)
{
    LDA(RD(RB_PTRNGE + 1));                 /* RANGE */
    TAY();
    CPY(0x10);                              /* WAIT FOR SOME POT RANGE ********** */
    if (BCS) goto l10;
    LDX(5);                                 /* CENTER UNTIL RANGE */
    if (BPL) goto l30;                      /* ALWAYS */
l10:
    LDA(RD(RB_POTVAL + 1));                 /* CURRENT VALUE */
    CMP(RD(RB_PTRNGE + 1));                 /* FULL RANGE */
    if (BNE) goto l20;                      /* NO */
    LDX(10);                                /* ELSE FULL SCALE */
    if (BPL) goto l30;                      /* ALWAYS */
l20:
    STY(RB_DD_DML);                         /* RANGE = DIVISOR */
    LDY(0);                                 /* ACC,X = DIVIDEND (VALUE) */
    STY(RB_DD_DMM);
    STY(RB_D_NMLL);
    STA(RB_D_NMLH);
    STY(RB_D_NMHL);
    STY(RB_D_NMHH);
    LDA(0x10);
    STA(RB_DD_NL);
    rb_ddiv10(m);
    TYA();
    LDX(10);                                /* ACC,Y = FRACTION OF FULL SCALE */
    rb_multpl(m);                           /* X = 1 OUT OF 11. VALUES (0 TO 10.) */
    TYA();
    if (BPL) goto l30;
    INX();                                  /* ROUND UP */
l30:
    TXA();
    ASLA();
    TAX();
    LDA(RD(AX(RB_POTDLY)));                 /* GET Y SCALED VALUE */
    STA(RB_PLDELY);
    LDA(RD(AX(RB_POTDLY + 1)));
    STA(RB_PLDELY + 1);
    LDA(RD(RB_PFROTN + 1));                 /* ELSE RETURN TO CENTER */
    if (BMI) goto l55;
    ORA(RD(RB_PFROTN));
    if (BEQ) goto pot_x;                    /* ALREADY CENTERED */
    LDA(RD(RB_PFROTN));
    LDX(0xFE);                              /* ELSE RETURN (STEP=-2) */
    LSRA();                                 /* > 1 */
    if (BNE) goto l57;                      /* YES */
    INX();                                  /* ELSE = 1 (X = 0FF) */
    if (BMI) goto l57;                      /* ALWAYS */
l55:
    LDA(RD(RB_PFROTN));
    LDX(2);
    CMP(0xFF);
    if (BNE) goto l57;
    DEX();                                  /* ROTN = 0FFFF (CHANGE = 1) */
l57:
    LDY(0);                                 /* ASSUME + */
    TXA();
    if (BPL) goto l58;                      /* POSITIVE */
    DEY();                                  /* ELSE SIGN EXTEND (Y = 0FF) */
l58:
    CLC();
    ADC(RD(RB_PFROTN));
    STA(RB_PFROTN);                         /* REDUCE ANGLE */
    TYA();
    ADC(RD(RB_PFROTN + 1));
    STA(RB_PFROTN + 1);
pot_x:
    LDA(RD(RB_PTRNGE));
    LSRA();                                 /* 1/2 OF RANGE */
    STA(RB_TEMP1);
    LDA(RD(RB_POTVAL));
    SEC();
    SBC(RD(RB_TEMP1));
    SEC();
    SBC(RD(RB_PLDELX));                     /* ACC=POT VALUE-CURRENT SETTING */
    TAX();                                  /* X = DIFF (POT VALUE - CURRENT) */
    if (BPL) goto pot_x_l5;
    rb_comp(m);
pot_x_l5:
    CMP(3);                                 /* 2 COUNTS OF HYSTERSIS */
    if (BCC) goto pot_x_l17;                /* LEAVE ALONE */
    TXA();
    LSRA();
    LSRA();
    LSRA();
    CMP(0x10);
    if (BCC) goto pot_x_l10;
    ORA(0xE0);                              /* SIGN EXTEND (MINUS) */
pot_x_l10:
    TAY();
    if (BEQ) goto pot_x_l20;
pot_x_l15:
    CLC();
    ADC(RD(RB_PLDELX));
    STA(RB_PLDELX);                         /* STEP DELTA X */
pot_x_l17:
    rb_setdiv(m); return;                   /* RESET DIVISION COUNT */
pot_x_l20:
    TXA();                                  /* IS DIFF = 0? */
    if (BEQ) goto pot_x_l17;                /* YES */
    LDA(1);                                 /* ELSE STEP =1 */
    if (BPL) goto pot_x_l15;                /* ALWAYS */
}

/* DELTAS */
/* 6E6B POTDLY: .WORD -32.*4,-23.*4,-17.*4,-10.*4,-5*4,0*4,4*4 */
/* 6E79  .WORD 8*4,13.*4,18.*4,25.*4 */
/* STRGME (6E81) */
void rb_strgme(rb_machine *m)
{
    BIT(RD(RB_GAMODE));
    if (BMI) goto l1;                       /* PLAY */
    if (BVS) goto l2;                       /* RTP */
l1:
    return;
l2:
    LDA(RD(RB_FRAME));
    LDX(8);
    AND(2);
    if (BNE) goto l4;
    TAX();                                  /* X = 0 */
l4:
    STX(RB_ST_LED);                         /* FLASH LED */
    LDA(RD(RB_GAMODE));
    AND(4);
    if (BNE) goto l1;                       /* WAIT FOR INITIALS */
    BIT(RD(RB_SWSTAT));                     /* CHECK STARTSW */
    if (BVC) goto l1;                       /* NO STARTSW */
    DEC(RB_CRDT);                           /* TAKE COIN */
    LDA(RB__STGME);
    STA(RB_GAMODE);
    STA(RB_STARTF);                         /* SET START FLAG */
    LDA(RD(RB_OPTION));                     /* OPTIONS DEFINE INITIAL # OF LIVES */
    AND(0x30);
    LSRA();
    LSRA();
    LSRA();
    LSRA();
    TAX();
    LDA(RD(AX(RB_INITLF)));
    STA(RB_LIVES);                          /* INITIAL */
    LDA(0);
    LDX(RB_EAWRTE - RB_OBJKLD);
l10:
    STA(ZX(RB_OBJKLD));                     /* CLR GAME START VARIABLES */
    DEX();
    if (BPL) goto l10;
    LDA(7);
    STA(RB_VALFRC);                         /* INITIALLY 7/10*DEPTH PLANE SCORE */
    rb_gminit(m); return;                   /* INITIALIZE PF (&OBJECTS) */
}

/* 6EC3 INITLF: .BYTE 2,3,4,5 */
