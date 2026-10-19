/* grnd.c - RBGRND: NMI, POSITION BEAM, PROJECT, ROTATE, DECODE
 *
 * Red Baron (Atari, 1981), RBGRND of the original source as C.
 * Written by tools/mac2c.py from upstream/RBGRND.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* INTERRUPT OCCURS EVERY 4 MS. [FROM HARDWARE] */
/* NMI (72C8) */
void rb_nmi(rb_machine *m)
{
    PHA();                                  /* SAVE REGISTERS */
    TXA();
    PHA();
    TYA();
    PHA();
    CLD();
    /* POT READING ROUTINE */
    LDA(RD(RB_POTFLG));
    if (BMI) goto l60;                      /* WAIT FOR COUNTER TO ADJUST TO POT */
    AND(1);
    TAX();
    STA(RB_POKEY + 0xB);                    /* POTGO */
    LDA(RD(RB_POTIN));                      /* GET POT READING */
    SEC();                                  /* A=POT READING */
    SBC(RD(ZX(RB_POTMIN)));                 /* CHK MIN */
    if (BCS) goto l20;                      /* BRANCH > MIN */
    DEC(ZX(RB_POTMIN));
    LDA(0);
l20:
    CMP(RD(ZX(RB_PTRNGE)));                 /* CHK RANGE */
    if (BCC) goto l30;                      /* WITHIN RANGE */
    INC(ZX(RB_PTRNGE));                     /* ELSE INCREASE */
    LDA(RD(ZX(RB_PTRNGE)));
l30:
    TAY();
    SEC();
    SBC(RD(ZX(RB_POTVAL)));                 /* CHECK FOR NOISE (+3-3) READING */
    if (BCC) goto l50;                      /* CHECK -3 */
    LSRA();                                 /* ELSE CHECK +3 */
    LSRA();
    if (BEQ) goto l60;                      /* NO CHANGE */
l40:
    STY(ZX(RB_POTVAL));                     /* UPDATE POT VALUE */
    INC(ZX(RB_POTUSE));                     /* COUNT POT CHANGES */
    LDA(RD(ZX(RB_POTUSE)));
    CMP(100);
    if (BCC) goto l60;
    INC(ZX(RB_POTMIN));
    DEC(ZX(RB_PTRNGE));
    DEC(ZX(RB_PTRNGE));
    LDA(0);
    STA(ZX(RB_POTUSE));
    if (BCS) goto l60;                      /* ALWAYS */
l50:
    ADC(3);                                 /* CARRY IS CLR */
    if (BMI) goto l40;                      /* BEYOND NOISE */
l60:
    DEC(RB_P_CNTR);                         /* COUNT DOWN */
    if (BNE) goto nmtime;                   /* NO YET */
    LDA(RD(RB_POTFLG));
    EOR(0x80);                              /* CHANGE STATUS BIT */
    TAX();
    if (BPL) goto l70;                      /* WAS WAITING */
    EOR(1);                                 /* ELSE SWITCH POT LATCH */
    TAX();
    LDA(RD(RB_P_LTCH));
    EOR(RB_PS_BIT);
    STA(RB_P_LTCH);                         /* CHANGE ANALOG SWITCH */
l70:
    STX(RB_POTFLG);
    LDA(RB_P_COUNT);                        /* COUNT VALUE */
    STA(RB_P_CNTR);
nmtime:
    LDA(RD(RB_FIRE));
    EOR(0x40);                              /* START + TRUE */
    STA(RB_SWSTAT);
    LDA(0);
    BIT(RD(RB_GAMODE));
    if (BPL) goto nmtime_l6;                /* NO SOUNDS */
    LDX(RD(RB_S_VAL));                      /* SHOT SOUND TIMER */
    if (BEQ) goto nmtime_l4;
    LDX(RB_SH_BIT);                         /* ENABLE BITS */
    LDA(RD(RB_INTCNT));
    AND(8);
    if (BNE) goto nmtime_l4;
    LDX(0);
nmtime_l4:
    TXA();
    ORA(RD(RB_EXPVAL));
    ORA(RD(RB_SPIRAL));
nmtime_l6:
    ORA(RD(RB_SLAMSN));
    ORA(RD(RB_ST_LED));
    ORA(RD(RB_P_LTCH));
    STA(RB_CRSHSN);                         /* SET SOUNDS/POT LATCH/START LED */
    TAX();
    LDA(RD(RB_GAMODE));
    ORA(RD(RB_LMTIM));
    if (BNE) goto nmtime_l10;
    STX(RB_NSERST);
nmtime_l10:
    INC(RB_INTCNT);                         /* INCREMENT INT (4 MS) COUNTER */
    LDA(RD(RB_INTCNT));
    CMP(250);                               /* 250.*.004=1 */
    if (BCC) goto nmtime_l12;
    LDA(0);
    STA(RB_INTCNT);                         /* CLR COUNT */
    INC(RB_SECCNT);                         /* COUNT SECONDS */
    if (BNE) goto nmtime_l12;
    INC(RB_SECCNT + 1);
nmtime_l12:
    LDA(RD(RB_CALFLG));                     /* CHK FRAME(S) */
    CMP(3);
    if (BCS) goto nmtime_l15;               /* 3*CALCULATION LOOP TIME W/O FRAME */
    LDX(0xFF);
    CPX(0x90);                              /* STACK TOO LOW? */
    if (BCS) goto nmtime_l20;               /* NO */
nmtime_l15:
    rb_hang(m); return;                     /* WAIT FOR WATCHDOG TO RESTART */
nmtime_l20:
    STA(RB_WATCH);                          /* CLR WATCHDOG */
    rb_moolah(m);                           /* COIN ROUTINE */
    LDA(0);
    STA(RB_I_TEMP);
    LDA(RB_CC_BIT);
    LDX(2);
nmtime_l25:
    LDY(RD(ZX(RB_CCTIM)));
    if (BPL) goto nmtime_l30;
    PHA();
    ORA(RD(RB_I_TEMP));                     /* SET COIN COUNTER */
    STA(RB_I_TEMP);
    PLA();
nmtime_l30:
    ASLA();
    DEX();
    if (BPL) goto nmtime_l25;
    LDA(RD(RB_I_TEMP));
    STA(RB_COINCT);
    DEC(RB_C_CNTR);                         /* CALCULATION FRAME CNTR */
    if (BNE) goto nmtime_l50;
    LDX(RB_CALCNT);
    LDA(RD(RB_GAMODE));
    AND(RB_BNRBIT);
    if (BEQ) goto nmtime_l35;
    LDX(RB_BNRCNT);
nmtime_l35:
    STX(RB_C_CNTR);
    INC(RB_CALFLG);
nmtime_l50:
    DEC(RB_F_CNTR);                         /* CHK FOR EOF [END OF FRAME] */
    if (BNE) goto nmexit;
    LDA(RB_FRMECNT);
    STA(RB_F_CNTR);                         /* RESET FRAME COUNT */
    INC(RB_INTFLG);                         /* FRAME SYNC [EVERY FRMECNT:4 MS] */
nmexit:
    rb_scorem(m);
    rb_modsnd(m);                           /* UPDATE SOUNDS */
    BIT(RD(RB_S_FLAG));                     /* PROGRAM CALCULATING? */
    if (BPL) goto nmexit_l10;               /* NO */
    LDA(RD(RB_INTFLG));                     /* EOF (VG) ? */
    if (BEQ) goto nmexit_l10;               /* NO */
    BIT(RD(RB_TESTSW));                     /* ELSE - SO VG HALTED? */
    if (BVC) goto nmexit_l10;               /* NOT YET */
    rb_plprop(m);
    STA(RB_VGON);                           /* ELSE - AUTO RESTART & CLR FLAG */
    DEC(RB_INTFLG);                         /* CLEAR FLAG FOR EOF (VG) */
nmexit_l10:
    PLA();                                  /* RESTORE REGISTERS */
    TAY();
    PLA();
    TAX();
    PLA();
    return;
}


/* ---- POSITION BEAM ---- */

/* POSITP (73DC) */
void rb_positp(rb_machine *m)
{
    LDA(RD(RB_UNIV4X));
    STA(RB_MATH + 4);                       /* X ABOUT CENTER */
    LDA(RD(RB_UNIV4X + 1));
    STA(RB_MATH + 5);
    LDA(0);
    STA(RB_MATH + 6);                       /* Y ABOUT PLAYER POSITION */
    STA(RB_MATH + 7);
    rb_posith(m);                           /* (falls into it) */
}

/* POSITH (73EE) */
void rb_posith(rb_machine *m)
{
    LDA(RD(RB_OBJECT + 4));                 /* X DEPTH */
    STA(RB_OBJECT + 6);                     /* Y DEPTH */
    LDA(RD(RB_OBJECT + 5));
    STA(RB_OBJECT + 7);
    LDA(RD(RB_OBJECT));                     /* X LSB */
    STA(RB_MATH + 8);
    LDA(RD(RB_OBJECT + 1));                 /* X MSB */
    STA(RB_MATH + 9);
    SEC();
    LDA(RD(RB_OBJECT + 2));                 /* Y LSB */
    SBC(RD(RB_I4YPOS));                     /* -EYE POSITION */
    STA(RB_MATH + 0xA);
    LDA(RD(RB_OBJECT + 3));                 /* Y MSB */
    SBC(RD(RB_I4YPOS + 1));
    STA(RB_MATH + 0xB);                     /* START MM */
    MATSTT();
    LDA(RD(RB_YLOW));
    STA(RB_XPOS);
    LDA(RD(RB_YHIGH));
    STA(RB_MATH + 0x12);                    /* START 2ND MM */
    STA(RB_XPOS + 1);
    MATSTT();
    LDA(RD(RB_YLOW));                       /* Y POSITION */
    LDX(RD(RB_YHIGH));
    STA(RB_MATH + 0xF);
    STX(RB_MATH + 0x10);
    LDA(RD(RB_OBJECT + 6));                 /* SCALE BY DEPTH */
    STA(RB_MATH + 0x15);
    LDA(RD(RB_OBJECT + 7));
    STA(RB_MATH + 0x16);
    STA(RB_MATH + 0x14);                    /* START DIVIDE */
    MATSTT();
    CLC();
    LDA(RD(RB_YLOW));
    ADC(RB_HORIZN);
    STA(RB_XPOS + 2);                       /* Y RESULT LSB */
    LDA(RD(RB_YHIGH));
    ADC(0);
    STA(RB_XPOS + 3);
    LDA(RD(RB_OBJECT + 4));
    STA(RB_MATH + 0x15);
    LDA(RD(RB_OBJECT + 5));
    STA(RB_MATH + 0x16);
    LDA(RD(RB_XPOS));                       /* SCALE X (LSB) */
    STA(RB_MATH + 0xF);
    LDA(RD(RB_XPOS + 1));                   /* SCALE X (MSB) */
    STA(RB_MATH + 0x10);
    STA(RB_MATH + 0x14);                    /* START DIVIDE */
    MATSTT();
    LDY(RD(RB_YLOW));
    STY(RB_XPOS);
    LDX(RD(RB_YHIGH));
    STX(RB_XPOS + 1);
    return;
}

/* SETBM (747A) */
void rb_setbm(rb_machine *m)
{
    TXA();                                  /* X,Y = X POSITION (SET SIGN) */
    rb_dpabs(m);
    CPX(3);                                 /* X POSITION VISIBLE */
    if (BCS) { rb_plac_0(m); return; }      /* NO */
    LDY(RD(RB_XPOS + 2));                   /* Y POSITION LSB */
    LDX(RD(RB_XPOS + 3));                   /* Y POSITION MSB */
    rb_dpabs(m);
    CPX(3);                                 /* Y POSITION VISIBLE */
    if (BCS) { rb_plac_0(m); return; }      /* NO */
    rb_placbm(m);                           /* (falls into it) */
}

/* PLACBM (748D) */
void rb_placbm(rb_machine *m)
{
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDA(0);
    STA(RB_VGBRIT);
    LDX(RB_XPOS);
    rb_vgvctr(m);                           /* VCTR TO POSITION */
    CLC();                                  /* 0,0 CENTER OF OBJECT */
    rb_plac_0(m);                           /* (falls into it) */
}

/* PLAC.0 (749A) */
void rb_plac_0(rb_machine *m)
{
    return;
    (void)m;
}

/* GROUND SHELL BEAM SET */
/* SETGRS (749B) */
void rb_setgrs(rb_machine *m)
{
    TXA();                                  /* X,Y=X POSITION(SET SIGN) */
    rb_dpabs(m);
    CPY(0x20);
    SBC(2);
    if (BCS) goto l10;
    LDY(RD(RB_XPOS + 2));                   /* XPOS+2,+3=Y POSITION */
    LDX(RD(RB_XPOS + 3));
    rb_dpabs(m);
    CPY(0x88);
    SBC(1);
    if (BCC) { rb_placbm(m); return; }
l10:
    return;
}


/* ---- PROJECT POINTS(TRANSLATE,ROTATE) ---- */

/* PROJECT (74B3) */
void rb_project(rb_machine *m)
{
    STA(RB_POINTR + 1);
    STX(RB_POINTR);
proj0:
    LDA(RD(RB_OBJECT + 4));                 /* X DEPTH */
    STA(RB_MATH + 0x15);                    /* DEPTH @ POINT */
    LDA(RD(RB_OBJECT + 5));                 /* X DEPTH (MSB) */
    STA(RB_MATH + 0x16);                    /* (MSB) */
    LDX(0);
    LDA(RD(IY(RB_POINTR)));                 /* X */
    if (BPL) goto proj0_l3;
    DEX();
proj0_l3:
    STX(RB_TEMP1 + 1);
    ASLA();
    ROL(RB_TEMP1 + 1);
    ASLA();
    ROL(RB_TEMP1 + 1);
    ASLA();
    ROL(RB_TEMP1 + 1);
    ASLA();
    ROL(RB_TEMP1 + 1);
    STA(RB_MATH + 0xF);
    LDA(RD(RB_TEMP1 + 1));
    STA(RB_MATH + 0x10);
    STA(RB_MATH + 0x14);                    /* START DIVIDE */
    MATSTT();
    LDA(RD(RB_YLOW));
    LDX(RD(RB_YHIGH));
    STA(RB_MATH + 8);
    STX(RB_MATH + 9);
    LDA(RD(RB_OBJECT + 6));                 /* Y DEPTH */
    STA(RB_MATH + 0x15);
    LDA(RD(RB_OBJECT + 7));
    STA(RB_MATH + 0x16);                    /* MSB */
    INY();
    LDX(0);
    LDA(RD(IY(RB_POINTR)));                 /* Y(LSB) */
    if (BPL) goto proj0_l6;
    DEX();
proj0_l6:
    STX(RB_TEMP1);
    ASLA();
    ROL(RB_TEMP1);
    ASLA();
    ROL(RB_TEMP1);
    STA(RB_MATH + 0xF);
    LDA(RD(RB_TEMP1));                      /* Y(MSB) */
    STA(RB_MATH + 0x10);
    STA(RB_MATH + 0x14);                    /* START DIVIDE */
    MATSTT();
    LDA(RD(RB_YLOW));
    LDX(RD(RB_YHIGH));
    STA(RB_MATH + 0xA);
    STX(RB_MATH + 0xB);
    TYA();
    ASLA();
    STY(RB_TEMP1);
    ADC(RD(RB_TEMP1));                      /* CARRY CLR */
    TAX();
    MATSTT();
    LDA(RD(RB_YLOW));
    STA(AXW(RB_DB_TRP - 1));                /* RESULT X LSB */
    LDA(RD(RB_YHIGH));
    STA(RB_MATH + 0x12);                    /* START 2ND MM */
    STA(AXW(RB_DB_TRP));                    /* X MSB */
    MATSTT();
    LDA(RD(RB_YHIGH));
    STA(AXW(RB_DB_TRP + 2));                /* Y MSB */
    LDA(RD(RB_YLOW));
    STA(AXW(RB_DB_TRP + 1));                /* Y LSB */
    TYA();
    SEC();
    SBC(3);                                 /* NEXT POINT (2 BYTES/POINT) */
    TAY();
    if (BCC) goto proj0_l10;
    goto proj0;
proj0_l10:
    return;
}


/* ---- ROTATE,PROJECT,DECODE POINTS ---- */

/* ROTATE PLANE ABOUT Z */
/* PLNABZ (755F) */
void rb_plnabz(rb_machine *m)
{
    TXA();
    ASLA();
    TAX();
    LDA(RD(AX(RB_PLNDB)));
    STA(RB_POINTR);
    LDA(RD(AX(RB_PLNDB + 1)));
    STA(RB_POINTR + 1);
    /* ROTATE ABOUT Z AXIS */
    LDA(0);
    STA(RB_MM_EL);                          /* TRANSLATE ABOUT 0,0,0 */
    STA(RB_MM_EM);
    STA(RB_MM_FL);
    STA(RB_MM_FM);
    /* ROTATE ABOUT Z */
    /* ENTRY   EXIT */
    /* ACC DC   BLITZED */
    /* X DC   BLITZED */
    /* Y LAST POINT OFFSET BLITZED */
    /* USES - SAVY,+1,POINTR,+1 */
zaxis:
    STY(RB_SAVY);
    TYA();
    ASLA();
    STA(RB_SAVY + 1);
    LDX(0);
    LDA(RD(IY(RB_POINTR)));                 /* Z */
    if (BPL) goto zaxis_l10;
    DEX();                                  /* SIGN EXTEND */
zaxis_l10:
    LDY(RD(RB_SAVY + 1));
    STA(AYW(RB_DB_TRP));                    /* Z LSB */
    TXA();
    STA(AYW(RB_DB_TRP + 1));                /* Z MSB */
    LDY(RD(RB_SAVY));
    INY();
    LDX(0);
    LDA(RD(IY(RB_POINTR)));                 /* X (*2) */
    ASLA();
    if (BCC) goto zaxis_l20;
    DEX();                                  /* SIGN EXTEND */
zaxis_l20:
    STA(RB_MM_XL);                          /* X LSB (*4) */
    STX(RB_MM_XM);                          /* X MSB (*4) */
    INY();
    LDX(0);
    LDA(RD(IY(RB_POINTR)));                 /* Y (*4) */
    if (BPL) goto zaxis_l30;
    DEX();                                  /* SIGN EXTEND */
zaxis_l30:
    STA(RB_MM_YL);                          /* Y LSB (*4) */
    STX(RB_MM_YM);                          /* Y MSB (*4) */
    LDY(RD(RB_SAVY + 1));
    MATSTT();
    LDA(RD(RB_YLOW));                       /* X RESULT (LSB) */
    LDX(RD(RB_YHIGH));                      /* X RESULT (MSB) */
    STA(RB_M_SYM);                          /* START 2ND MULTIPLY */
    STA(AYW(RB_DB_TRP + 2));
    TXA();
    STA(AYW(RB_DB_TRP + 3));
    MATSTT();
    LDA(RD(RB_YLOW));                       /* Y RESULT (LSB) */
    STA(AYW(RB_DB_TRP + 4));
    LDA(RD(RB_YHIGH));                      /* Y RESULT (MSB) */
    STA(AYW(RB_DB_TRP + 5));
    LDA(RD(RB_SAVY));
    SEC();
    SBC(3);
    TAY();
    if (BCS) goto zaxis;
    return;
}

/* ROTATE ABOUT Y */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X LAST POINT OFFSET BLITZED */
/* Y DC   BLITZED */
/* USES - SAVX,TEMP1,RESULT,+1 */
/* CALLS - MRSULT */
/* YAXIS (75E0) */
void rb_yaxis(rb_machine *m)
{
top:
    STX(RB_SAVX);
    LDA(RD(AX(RB_DB_TRP + 2)));             /* X LSB */
    ASLA();
    STA(RB_TEMP1);
    LDA(RD(AX(RB_DB_TRP + 3)));             /* X MSB */
    ROLA();
    ASL(RB_TEMP1);
    ROLA();
    STA(RB_MM_XM);                          /* X MSB (*4) */
    LDA(RD(RB_TEMP1));
    STA(RB_MM_XL);                          /* X LSB (*4) */
    LDA(RD(AX(RB_DB_TRP)));                 /* Z LSB */
    ASLA();
    STA(RB_TEMP1);
    LDA(RD(AX(RB_DB_TRP + 1)));             /* Z MSB */
    ROLA();
    ASL(RB_TEMP1);
    ROLA();
    LDY(RD(RB_TEMP1));
    STY(RB_MM_YL);                          /* Z LSB (*4) */
    STA(RB_MM_YM);                          /* Z MSB (*4) */
    rb_mrsult(m);                           /* DO ROTATION */
    LDY(RD(RB_SAVX));
    STA(AYW(RB_DB_TRP));                    /* Z (LSB) RESULT */
    TXA();
    STA(AYW(RB_DB_TRP + 1));                /* Z (MSB) */
    LDA(RD(RB_RESULT));
    STA(AYW(RB_DB_TRP + 2));                /* X (LSB) */
    LDA(RD(RB_RESULT + 1));
    STA(AYW(RB_DB_TRP + 3));                /* X (MSB) */
    TYA();
    SEC();
    SBC(6);
    TAX();
    if (BCS) goto top;
    return;
}

/* ROTATE ABOUT X */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X LAST POINT OFFSET BLITZED */
/* Y DC   BLITZED */
/* USES - SAVX,TEMP1,RESULT,+1 */
/* CALLS - MRSULT */
/* XAXIS (762A) */
void rb_xaxis(rb_machine *m)
{
top:
    STX(RB_SAVX);
    LDA(RD(AX(RB_DB_TRP + 4)));             /* Y LSB */
    ASLA();
    STA(RB_TEMP1);
    LDA(RD(AX(RB_DB_TRP + 5)));             /* Y MSB */
    ROLA();
    ASL(RB_TEMP1);
    ROLA();
    STA(RB_MM_XM);                          /* Y MSB (*4) */
    LDA(RD(RB_TEMP1));
    STA(RB_MM_XL);                          /* Y LSB (*4) */
    LDA(RD(AX(RB_DB_TRP)));                 /* Z LSB */
    ASLA();
    STA(RB_TEMP1);
    LDA(RD(AX(RB_DB_TRP + 1)));             /* Z MSB */
    ROLA();
    ASL(RB_TEMP1);
    ROLA();
    LDY(RD(RB_TEMP1));
    STY(RB_MM_YL);                          /* Z LSB (*4) */
    STA(RB_MM_YM);                          /* Z MSB (*4) */
    rb_mrsult(m);                           /* DO ROTATION */
    LDY(RD(RB_SAVX));
    STA(AYW(RB_DB_TRP));                    /* Z (LSB) RESULT */
    TXA();
    STA(AYW(RB_DB_TRP + 1));                /* Z (MSB) */
    LDA(RD(RB_RESULT));
    STA(AYW(RB_DB_TRP + 4));                /* Y (LSB) */
    LDA(RD(RB_RESULT + 1));
    STA(AYW(RB_DB_TRP + 5));                /* Y (MSB) */
    TYA();
    SEC();
    SBC(6);
    TAX();
    if (BCS) goto top;
    return;
}

/* PROJECT X & Y BY Z */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X # POINTS * 6  BLITZED */
/* Y DC   BLITZED */
/* USES - O.DPTH,+1 */
/* MODIFYS - DB.TRP(S),MBOX(S) */
/* D.LOOP (7674) */
void rb_d_loop(rb_machine *m)
{
top:
    CLC();
    LDA(RD(AX(RB_DB_TRP)));                 /* Z LSB */
    ADC(RD(RB_O_DPTH));                     /* + OBJECT DEPTH */
    STA(RB_DD_DML);
    LDA(RD(AX(RB_DB_TRP + 1)));             /* Z MSB */
    ADC(RD(RB_O_DPTH + 1));
    STA(RB_DD_DMM);
    LDA(RD(AX(RB_DB_TRP + 2)));             /* X  LSB */
    STA(RB_D_NMHL);
    LDA(RD(AX(RB_DB_TRP + 3)));             /* X  MSB */
    STA(RB_D_NMHH);
    STA(RB_D_SZXD);                         /* START DIVISION */
    LDY(RD(AX(RB_DB_TRP + 4)));             /* Y LSB */
    MATSTT();                               /* WAIT FOR DIVISION */
    LDA(RD(RB_YLOW));
    STA(AXW(RB_DB_TRP + 2));                /* SCALED X LSB */
    LDA(RD(RB_YHIGH));
    STY(RB_D_NMHL);
    LDY(RD(AX(RB_DB_TRP + 5)));             /* Y MSB */
    STY(RB_D_NMHH);
    STA(RB_D_SZXD);                         /* START DIVISION */
    STA(AXW(RB_DB_TRP + 3));                /* SCALED X MSB */
    MATSTT();                               /* WAIT FOR DIVISION */
    LDA(RD(RB_YLOW));
    STA(AXW(RB_DB_TRP + 4));                /* SCALED Y LSB */
    LDA(RD(RB_YHIGH));
    STA(AXW(RB_DB_TRP + 5));                /* SCALED Y MSB */
    TXA();
    SEC();
    SBC(6);
    TAX();
    if (BCS) goto top;                      /* DO NEXT POINT */
    return;
}

/* DECODE POINT CONNECTION INSTRUCTIONS */
/* DB.TRP CONTAINS 6 BYTE FIELDS OF POINT */
/* (Z LSB,Z MSB,X LSB,X MSB,Y LSB ,Y MSB) */
/* ENTRY   EXIT */
/* ACC ADDR OF INSTR.(MSB) BLITZED */
/* X   "   "   (LSB) BLITZED */
/* Y DC   BLITZED */
/* USES - POINTR,+1,SAVY,VGBRIT,XPOINT,+1,YPOINT,+1,XCOMP,+1 */
/* YCOMP,+1,DB.TRP(S) */
/* CALLS - VGVCTR */
/* DCMPA0 (76CD) */
void rb_dcmpa0(rb_machine *m)
{
    LDY(0);
    STY(RB_XPOINT);                         /* INITIAL POINT IS 0,0 */
    STY(RB_XPOINT + 1);
    STY(RB_YPOINT);
    STY(RB_YPOINT + 1);
    rb_decmap(m);                           /* (falls into it) */
}

/* DECMAP (76D7) */
void rb_decmap(rb_machine *m)
{
    STA(RB_POINTR + 1);                     /* INSTRUCTION POINTER */
    STX(RB_POINTR);
    LDY(0);                                 /* START @ BYTE 0 */
decode:
    LDX(0x20);                              /* ASSUME VISIBLE VCTR */
    LDA(RD(IY(RB_POINTR)));                 /* VCTR INSTRUCTION */
    CMP(0xFF);
    if (BEQ) goto decodn;                   /* END POINT CONNECT GRAPH */
    STY(RB_SAVY);                           /* SAVE INDEX */
    LSRA();                                 /* (D0 = 0 = BLANK VCTR) */
    if (BCS) goto decode_l10;
    LDX(0);                                 /* NO INTENSITY */
decode_l10:
    STX(RB_VGBRIT);                         /* SET INTENSITY */
    ASLA();                                 /* POINT # * 6 */
    TAX();
    rb_note_point(m);                       /* (an addition: game/frame.h) */
    LDA(RD(AX(RB_DB_TRP + 2)));             /* X LSB */
    TAY();
    SEC();
    SBC(RD(RB_XPOINT));                     /* - CURRENT X LSB */
    STA(RB_XCOMP);                          /* X DELTA (LSB) */
    STY(RB_XPOINT);                         /* BECOMES NEW CURRENT ENDPOINT */
    LDA(RD(AX(RB_DB_TRP + 3)));             /* X MSB */
    TAY();
    SBC(RD(RB_XPOINT + 1));                 /* - CURRENT X MSB */
    STA(RB_XCOMP + 1);                      /* X DELTA (MSB) */
    STY(RB_XPOINT + 1);                     /* BECOMES NEW CURRENT */
    LDA(RD(AX(RB_DB_TRP + 4)));             /* Y LSB */
    TAY();
    SEC();
    SBC(RD(RB_YPOINT));                     /* - CURRENT Y LSB */
    STA(RB_YCOMP);                          /* Y DELTA (LSB) */
    STY(RB_YPOINT);                         /* BECOMES NEW CURRENT ENDPOINT */
    LDA(RD(AX(RB_DB_TRP + 5)));             /* Y MSB */
    TAY();
    SBC(RD(RB_YPOINT + 1));                 /* - CURRENT Y MSB */
    STA(RB_YCOMP + 1);                      /* Y DELTA (MSB) */
    STY(RB_YPOINT + 1);                     /* BECOMES NEW CURRENT */
    rb_vgvtr2(m);
    m->note = 0;                            /* (an addition: game/frame.h) */
    LDY(RD(RB_SAVY));
    INY();                                  /* DECODE NEXT BYTE */
    if (BNE) goto decode;                   /* ALWAYS */
decodn:
    return;
}


/* ---- DISPLAY DB'S ---- */

/* 7723 SCAPE0: .BYTE -128/2,0*2 */
/* 7725  .BYTE -104/2,24*2 */
/* 7727  .BYTE -88/2,24*2 */
/* 7729  .BYTE -64/2,0*2 */
/* 772B  .BYTE -48/2,16*2 */
/* 772D  .BYTE -16/2,24*2 */
/* 772F  .BYTE 0/2,8*2 */
/* 7731  .BYTE 24/2,16*2 */
/* 7733  .BYTE 48/2,0*2 */
/* 7735  .BYTE 64/2,0*2 */
/* 7737  .BYTE 80/2,0*2 */
/* 7739  .BYTE 96/2,24*2 */
/* 773B  .BYTE 120/2,0*2 */
/* 773D  .BYTE 112/2,8*2 */
/* 773F  .BYTE 120/2,20*2 */
/* 7741  .BYTE 128/2,0*2 */
/* 7743  .BYTE -32/2,8*2 */
/* 7745  .BYTE -48/2,0*2 */
/* 7747  .BYTE 8/2,0*2 */
/* 7749  .BYTE 100/2,8*2 */
/* 774B  .BYTE 112/2,0*2 */
/* 774D SCAPE1: .BYTE -128/2,0*2 */
/* 774F  .BYTE -112/2,0*2 */
/* 7751  .BYTE -64/2,12*2 */
/* 7753  .BYTE 0/2,0*2 */
/* 7755  .BYTE 16/2,16*2 */
/* 7757  .BYTE 48/2,16*2 */
/* 7759  .BYTE 64/2,0*2 */
/* 775B  .BYTE 96/2,32*2 */
/* 775D  .BYTE 112/2,8*2 */
/* 775F  .BYTE 104/2,16*2 */
/* 7761  .BYTE 112/2,24*2 */
/* 7763  .BYTE 128/2,0*2 */
/* 7765  .BYTE -48/2,4*2 */
/* 7767  .BYTE -64/2,0*2 */
/* 7769  .BYTE 92/2,16*2 */
/* 776B  .BYTE 104/2,0*2 */
/* 776D SCAPE2: .BYTE -128/2,0*2 */
/* 776F  .BYTE -88/2,8*2 */
/* 7771  .BYTE -64/2,0*2 */
/* 7773  .BYTE -32/2,24*2 */
/* 7775  .BYTE -16/2,0*2 */
/* 7777 PFPNT0: .BYTE 0/2,0*2 */
/* 7779  .BYTE 16/2,0*2 */
/* 777B  .BYTE 32/2,16*2 */
/* 777D  .BYTE 48/2,16*2 */
/* 777F  .BYTE 64/2,24*2 */
/* 7781  .BYTE 88/2,0*2 */
/* 7783  .BYTE 72/2,16*2 */
/* 7785  .BYTE 96/2,32*2 */
/* 7787  .BYTE 128/2,0*2 */
/* 7789  .BYTE -96/2,0*2 */
/* 778B  .BYTE -40/2,0*2 */
/* 778D  .BYTE 40/2,0*2 */
/* 778F  .BYTE 56/2,8*2 */
/* 7791 SCAPE3: .BYTE -128/2,0*2 */
/* 7793  .BYTE -112/2,16*2 */
/* 7795  .BYTE -96/2,16*2 */
/* 7797  .BYTE -64/2,32*2 */
/* 7799  .BYTE -32/2,24*2 */
/* 779B  .BYTE 0/2,0*2 */
/* 779D  .BYTE 16/2,24*2 */
/* 779F  .BYTE 64/2,0*2 */
/* 77A1  .BYTE 96/2,0*2 */
/* 77A3  .BYTE 104/2,16*2 */
/* 77A5  .BYTE 128/2,0*2 */
/* 77A7  .BYTE -96/2,0*2 */
/* 77A9  .BYTE -56/2,16*2 */
/* 77AB  .BYTE 8/2,0*2 */
/* 77AD  .BYTE 24/2,8*2 */
/* 77AF SSEGS: .WORD SCAPE0,SCAPE1,SCAPE2,SCAPE3 */
/* 77B7 .SSEG: .BYTE SCAPE1-SCAPE0-2,SCAPE2-SCAPE1-2 */
/* 77B9  .BYTE SCAPE3-SCAPE2-2,SSEGS-SCAPE3-2 */
/* 77BB BULLT0: .WORD 0,6*2,0*2 */
/* 77C1  .WORD 0,-6*2,0*2 */
/* 77C7  .WORD 0,0*2,-6*2 */
/* 77CD  .WORD 0,0*2,6*2 */
/* 77D3 BULLT1: .WORD 0,4*2,4*2 */
/* 77D9  .WORD 0,-4*2,-4*2 */
/* 77DF  .WORD 0,4*2,-4*2 */
/* 77E5  .WORD 0,-4*2,4*2 */
/* 77EB BULLDE: .BYTE 0*6 */
/* 77EC  .BYTE 1*6+1 */
/* 77ED  .BYTE 2*6+1 */
/* 77EE  .BYTE 3*6+1 */
/* 77EF  .BYTE 0*6+1 */
/* 77F0  .BYTE 2*6+1 */
/* 77F1  .BYTE 1*6 */
/* 77F2  .BYTE 3*6+1 */
/* 77F3  .BYTE ^H0FF */
/* 77F4 PFOCOL: .WORD -104*8,20*8 */
/* 77F8  .WORD -88*8,32*8 */
/* 77FC  .WORD 48*8,0*8 */
/* 7800  .WORD 64*8,12*8 */
/* 7804  .WORD 64*8,0*8 */
/* 7808  .WORD 80*8,12*8 */
/* 780C  .WORD -128*8,0*8 */
/* 7810  .WORD -112*8,12*8 */
/* 7814  .WORD 16*8,12*8 */
/* 7818  .WORD 32*8,24*8 */
/* 781C  .WORD 32*8,12*8 */
/* 7820  .WORD 48*8,24*8 */
/* 7824  .WORD -16*8,0*8 */
/* 7828  .WORD 0*8,12*8 */
/* 782C  .WORD 0*8,0*8 */
/* 7830  .WORD 16*8,12*8 */
/* 7834  .WORD 32*8,12*8 */
/* 7838  .WORD 48*8,24*8 */
/* 783C  .WORD -112*8,12*8 */
/* 7840  .WORD -96*8,24*8 */
/* 7844  .WORD 64*8,0*8 */
/* 7848  .WORD 80*8,12*8 */
/* 784C  .WORD 80*8,0*8 */
/* 7850  .WORD 96*8,12*8 */
/* 7854 VCTRMP: .BYTE 0*6 */
/* 7855  .BYTE ^H0FF */
