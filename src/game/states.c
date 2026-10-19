/* states.c - STATES, HIGH SCORE TABLE ROUTINES
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- STATES ---- */

/* MAINTAIN DISPLAY STATES */
/* STATES (4ED9) */
void rb_states(rb_machine *m)
{
    LDA(RD(RB_GAMODE));
    AND(0xBF);
    if (BEQ) goto l10;                      /* ATTRACT/RTP */
    CMP(RB_BNRBIT + 3);
    if (BNE) goto l5;
    rb_dspbnr(m); return;                   /* ATTRACT BANNER */
l5:
    AND(0xF);
    CMP(7);
    if (BEQ) goto l25;                      /* GET INPUT FOR HIGH SCORE TABLE */
    CMP(3);
    if (BNE) goto l15;                      /* PLAY/EOG */
    rb_hstbl(m); return;                    /* DISPLAY HIGH SCCORES */
l10:
    DEC(RB_TIMER);                          /* ATTRACT/RTP */
    if (BNE) goto l15;
    LDA(156);
    STA(RB_HSTMER);
    STA(RB_TIMER);                          /* HSTB EVERY 20 SECONDS */
    LDA(RD(RB_HSCRES));
    ORA(RD(RB_HSCRES + 1));
    if (BEQ) goto l20;
    LDA(3);
    ORA(RD(RB_GAMODE));
    STA(RB_GAMODE);
l15:
    return;
l20:
    rb_nwbnnr(m); return;                   /* CHECK FOR ATTRACT BANNER */
l25:
    LDY(RD(RB_LETR + 2));
    LDX(1);
l30:
    rb_letchk(m);                           /* JOYSTICK CONTROLS LETTER CHANGE */
    DEX();
    if (BPL) goto l30;
    STY(RB_LETR + 2);
    CLC();
    LDA(RD(RB_LETR));
    ADC(RD(RB_LETR + 1));
    TAX();
    TYA();
    if (BMI) goto l35;
    CMP(3);
    LDA(RD(AX(RB_INITL)));
    if (BCC) goto l45;
    ADC(1);                                 /* CARRY SET */
    CMP(0x4C);
    if (BCC) goto l40;
    LDA(0x16);
    if (BCS) goto l40;                      /* ALWAYS */
l35:
    CMP(-3);
    LDA(RD(AX(RB_INITL)));
    if (BCS) goto l45;
    SBC(1);                                 /* -2 (CARRY CLR) */
    CMP(0x16);
    if (BCS) goto l40;
    LDA(0x4A);
l40:
    STA(AXW(RB_INITL));
    LDA(0);
    STA(RB_LETR + 2);                       /* RESET */
l45:
    BIT(RD(RB_SWSTAT));
    if (BMI) goto l50;
    LDA(0x80);
    STA(RB_FR_DB);                          /* INITIALIZE DEBOUNCE */
    if (BNE) goto l70;                      /* ALWAYS */
l50:
    ASL(RB_FR_DB);                          /* DEBOUNCE */
    if (BCC) goto l70;
    LDA(0);
    STA(RB_LETR + 2);
    LDX(RD(RB_LETR + 1));
    INX();
    CPX(3);
    if (BCC) goto l65;                      /* NOT YET ALL 3 INITIALS */
l55:
    LDA(RD(RB_GAMODE));
    AND(0xF8);                              /* D2 CLR ( + GAME STATUS) */
    ORA(3);
    STA(RB_GAMODE);                         /* DISPLAY TABLE */
    LDA(80);                                /* FOR COUNTS */
    STA(RB_HSTMER);
    LDX(6 * 6 - 1);
    STX(RB_EAX);                            /* START @ END */
    INX();
    STX(RB_EABC);
    STX(RB_EACNT);                          /* MAX COUNT */
    LDA(RB_EAFILL / 0x100);
    STA(RB_EASRCE + 1);
    LDA(RB_EAFILL & 0xFF);
    STA(RB_EASRCE);
    LDA(RD(RB_LETR));
    CMP(15);
    if (BCC) goto l60;
    LDX(6 * 3 - 1);
    STX(RB_EAX);
    INX();
    STX(RB_EABC);
    STX(RB_EACNT);
l60:
    rb_stearom(m);                          /* CHECK EAROM STORE */
    rb_gminit(m); return;                   /* REINIT POKEY */
l65:
    STX(RB_LETR + 1);
    TXA();
    CLC();
    ADC(RD(RB_LETR));
    TAX();
    LDA(0x16);
    STA(AXW(RB_INITL));
l70:
    LDA(18);
    rb_message(m);                          /* DISPLAY HIGH-SCORE INITIAL INFO */
    LDA(38);
    rb_message(m);
    LDA(40);
    rb_message(m);
    LDA(42);
    rb_message(m);
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDA(-24 / 4);
    LDX(0);
    LDY(0);
    rb_vgvtr(m);                            /* POSITION BEAM */
    LDX(RD(RB_LETR));
    rb_hsintl(m);
    DEC(RB_TIMER);
    if (BNE) goto l85;
    DEC(RB_TIMER + 1);
    if (BNE) goto l85;
    LDX(RD(RB_LETR));
    LDY(0);
l75:
    LDA(RD(AX(RB_INITL)));
    CMP(0x4C);                              /* IF UNDERLINE */
    if (BNE) goto l80;
    LDA(0);
    STA(AXW(RB_INITL));                     /* BLANK */
l80:
    INX();
    INY();
    CPY(3);
    if (BCC) goto l75;
    goto l55;                               /* ALWAYS */
l85:
    return;
}


/* ---- HIGH SCORE TABLE ROUTINES ---- */

/* CHECK FOR NEW HIGH SCORE TABLE ENTRY */
/* NW.HSC (4FEB) */
void rb_nw_hsc(rb_machine *m)
{
    LDA(RD(RB_PRCTIM + 2));
    STA(RB_TEMP1 + 1);
    LDA(RD(RB_PRCTIM + 1));
    STA(RB_TEMP1);
    LDA(RD(RB_PRCTIM));
    rb_div32(m);                            /* DETERMINE 1 SAMPLE */
    SEC();
    LDA(RD(RB_SECCNT));                     /* NEW SAMPLE */
    SBC(RD(RB_TEMP1));                      /* -AVERAGE SAMPLE */
    TAX();
    LDA(RD(RB_SECCNT + 1));                 /* MIDDLE BYTE */
    SBC(RD(RB_TEMP1 + 1));
    TAY();
    LDA(0);                                 /* MSB */
    SBC(RD(RB_TEMP1 + 2));
    PHA();
    CLC();
    TXA();
    ADC(RD(RB_PRCTIM));                     /* ADD NEW DIFFERENCE TO AVERAGE SUM */
    STA(RB_PRCTIM);
    TYA();
    ADC(RD(RB_PRCTIM + 1));                 /* MIDDLE BYTE */
    STA(RB_PRCTIM + 1);
    PLA();
    ADC(RD(RB_PRCTIM + 2));                 /* MSB */
    STA(RB_PRCTIM + 2);
    LDX(0);
    TXA();
    BIT(RD(RB_OPTION));
    if (BPL) goto l9;
    LDA(RD(RB_PRCTIM + 2));                 /* DETERMINE 1 SAMPLE */
    STA(RB_TEMP1 + 1);
    LDA(RD(RB_PRCTIM + 1));
    STA(RB_TEMP1);
    LDA(RD(RB_PRCTIM));
    rb_div32(m);
    if (BEQ) goto l5;
    LDA(0x20);
    if (BNE) goto l9;                       /* TIME VERY LARGE */
l5:
    LDA(RD(RB_OPTION));
    LSRA();
    LSRA();
    AND(0xF);
    TAX();                                  /* OPTION SELECTION */
    SEC();
    LDA(RD(RB_TEMP1));
    SBC(RD(AX(RB_GTOPTS)));
    STA(RB_TEMP1);
    LDA(RD(RB_TEMP1 + 1));
    SBC(0);
    CMP(0x80);
    RORA();
    ROR(RB_TEMP1);
    LDX(RD(RB_TEMP1));
    TAY();
    if (BPL) goto l7;
    CPX(-0x10);
    SBC(-1);
    LDA(-1);
    if (BCS) goto l9;
    LDX(-0x10);
    if (BMI) goto l9;                       /* ALWAYS */
l7:
    CPX(0x21);
    SBC(0);
    LDA(0);
    if (BCC) goto l9;
    LDX(0x20);
l9:
    STA(RB_PRPDEL + 1);
    STX(RB_PRPDEL);
    STA(RB_PRMDEL + 1);
    TXA();
    ASLA();
    ASLA();
    STA(RB_PRMDEL);
    LDX(0);
    STX(RB_LETR + 1);                       /* START @ FIRST LETTER */
    STX(RB_LETR + 2);                       /* INITIAL CHANGE DEBOUNCE */
    LDY(2);                                 /* 3 BYTES OF GAME COUNT */
l11:
    INC(AXW(RB_GCOUNT));
    if (BNE) goto l12;                      /* NO CARRY */
    INX();
    DEY();
    if (BPL) goto l11;                      /* PROPAGATE CARRY */
l12:
    CLC();
    LDA(RD(RB_SECCNT));
    ADC(RD(RB_GTIME));
    STA(RB_GTIME);                          /* ADD CURRENT GAME TIME TO SUM */
    LDA(RD(RB_SECCNT + 1));
    ADC(RD(RB_GTIME + 1));
    STA(RB_GTIME + 1);                      /* MSB */
    if (BCC) goto l14;                      /* NO CARRY */
    LDY(2);                                 /* 5 TOTAL BYTES OF GAME TIME SUM */
    LDX(0);                                 /* START WITH THIRD BYTE */
l13:
    INC(AXW(RB_GTIME + 2));                 /* ADD IN CARRY */
    if (BNE) goto l14;                      /* NO MORE */
    INX();
    DEY();
    if (BPL) goto l13;                      /* CONTINUE CARRY */
l14:
    LDY(RD(AX(RB_HSCRES)));
    CPY(RD(RB_SCORE));
    LDA(RD(AX(RB_HSCRES + 1)));
    SBC(RD(RB_SCORE + 1));
    if (BCC) goto l20;                      /* X=PLACE IN TABLE */
    TXA();
    ADC(4);                                 /* ADD 5 (CARRY SET) */
    TAX();
    CPX(7 * 5);                             /* 7 HIGH SCORES */
    if (BCC) goto l14;
    LDA(RD(RB_HSCRES));
    ORA(RD(RB_HSCRES + 1));
    if (BEQ) goto l15;                      /* NO HST */
    LDA(80);
    STA(RB_HSTMER);
    LDA(3);
l15:
    STA(RB_GAMODE);                         /* ATTRACT */
    rb_gminit(m); return;                   /* REINIT POKEY */
l20:
    STX(RB_LETR);
    SEC();
    LDA(3 * 5);                             /* 3 ENTRIES * 5 */
    SBC(RD(RB_LETR));                       /* INVERT ENTRY # */
    TAX();
    CPX(5);
    if (BEQ) goto l22;
    CPX(10);
    if (BEQ) goto l21;
    INX();
l21:
    INX();
l22:
    INX();
    /* STX EABC  ;BYTE COUNT = 6 * # OF ENTRIES */
    LDA(7);
    STA(RB_GAMODE);                         /* GET HIGH SCORE INITIALS */
    LDA(4);
    STA(RB_TIMER + 1);
    LDA(0);
    STA(RB_TIMER);                          /* TIMEOUT @ APPROX. 2 MINUTES FOR INITIALS */
    LDX(6 * 5);
l25:
    CPX(RD(RB_LETR));
    if (BEQ) goto l30;
    LDA(RD(AX(RB_HSCRES - 5)));             /* MOVE SCORE DOWN */
    STA(AXW(RB_HSCRES));
    LDA(RD(AX(RB_HSCRES - 4)));
    STA(AXW(RB_HSCRES + 1));
    LDA(RD(AX(RB_INITL - 5)));
    STA(AXW(RB_INITL));
    LDA(RD(AX(RB_INITL - 4)));
    STA(AXW(RB_INITL + 1));
    LDA(RD(AX(RB_INITL - 3)));
    STA(AXW(RB_INITL + 2));
    TXA();
    SEC();
    SBC(5);
    TAX();
    if (BCS) goto l25;                      /* ALWAYS */
l30:
    LDA(RD(RB_SCORE));
    STA(AXW(RB_HSCRES));
    LDA(RD(RB_SCORE + 1));
    STA(AXW(RB_HSCRES + 1));
    LDA(0x16);
    LDX(RD(RB_LETR));
    STA(AXW(RB_INITL));                     /* CLR FIRST INITIAL */
    LDA(0x4C);
    STA(AXW(RB_INITL + 1));                 /* INITIALIZE TO UNDERLINE */
    STA(AXW(RB_INITL + 2));
    rb_initial(m); return;
}

/* DIVIDE BY 32. */
/* DIV32 (513E) */
void rb_div32(rb_machine *m)
{
    ASLA();
    ROL(RB_TEMP1);
    ROL(RB_TEMP1 + 1);
    ROLA();
    ROL(RB_TEMP1);
    ROL(RB_TEMP1 + 1);
    ROLA();
    ROL(RB_TEMP1);
    ROL(RB_TEMP1 + 1);
    ROLA();
    AND(3);
    STA(RB_TEMP1 + 2);
    return;
}

/* CHANGE LETTER OFFSET (BY POT VALUES) */
/* LETCHK (5153) */
void rb_letchk(rb_machine *m)
{
    SEC();
    LDA(RD(ZX(RB_PTRNGE)));
    SBC(RD(ZX(RB_POTVAL)));
    STA(RB_TEMP1);
    LDA(RD(ZX(RB_PTRNGE)));
    LSRA();
    LSRA();
    LSRA();
    CMP(RD(RB_TEMP1));                      /* TOP 1/8 RANGE PREVIOUS LETTER */
    PHA();                                  /* SAVE DATA */
    TXA();                                  /* WHICH AXIS */
    if (BEQ) goto l30;
    PLA();                                  /* RESTORE DATA */
    if (BCS) goto l10;                      /* TOP 1/8 VERTICAL */
    CMP(RD(ZX(RB_POTVAL)));
    if (BCC) goto l20;
l5:
    INY();                                  /* BOTTOM 1/8 VERTICAL */
    return;
l10:
    DEY();
l20:
    return;
l30:
    PLA();
    if (BCS) goto l5;                       /* TOP 1/8 HORIZONTAL */
    CMP(RD(ZX(RB_POTVAL)));
    if (BCS) goto l10;                      /* BOTTOM 1/8 HORIZONTAL */
    return;
}

/* DISPLAY HSTD */
/* HSTBL (5178) */
void rb_hstbl(rb_machine *m)
{
    LDA(20);
    rb_message(m);                          /* HIGH SCORES */
    LDY(0);
l10:
    STY(RB_SAVY);
    LDA(RD(AY(RB_HSCRES)));
    ORA(RD(AY(RB_HSCRES + 1)));
    if (BEQ) goto l22;                      /* NO MORE HIGH SCORES */
    rb_vgcntr(m);
    LDA(RD(RB_SAVY));
    LDX(-32 / 0x10);                        /* DETERMINE Y POSITION */
    rb_multpl(m);
    TYA();
    CLC();
    ADC(32 / 4);                            /* START @ 32. Y */
    TAX();
    LDA(-132 / 4);                          /* -132. X */
    rb_vgvtr1(m);
    LDX(RD(RB_SAVY));
    rb_hsintl(m);                           /* DISPLAY INITIALS & BUMP SAVX TO NEXT INITIAL SET */
    LDA(72 / 4);
    LDX(0);
    rb_vgvtr1(m);                           /* OFFSET VCTR */
    LDY(RD(RB_SAVY));
    LDA(RD(AY(RB_HSCRES)));
    STA(RB_TEMP2);
    LDA(RD(AY(RB_HSCRES + 1)));
    STA(RB_TEMP2 + 1);
    LDX(RB_TEMP2 + 1);
    rb_digt1s(m);
    rb_alph_0(m);                           /* TRAILING 0 */
    LDA(RD(RB_SAVY));
    CLC();
    ADC(5);
    TAY();
    CPY(5 * 7);
    if (BCC) goto l10;
l22:
    DEC(RB_HSTMER);
    if (BNE) goto l30;
    rb_initial(m);
    LDA(0xFC);
    AND(RD(RB_GAMODE));
    STA(RB_GAMODE);                         /* CLR TABLE */
    LDA(0);
    STA(RB_OBJKLD + 1);
    LDA(10);
    STA(RB_OBJKLD);
    STA(RB_N_PLNZ);
    rb_nwbnnr(m); return;                   /* CHECK FOR ATTRACT BANNER */
l30:
    return;
}

/* DISPLAY THREE INITIALS */
/* X=FIRST (LEFT) INITIAL (IN INITL) */
/* HSINTL (51E2) */
void rb_hsintl(rb_machine *m)
{
    LDA(2);
    STA(RB_TEMP1);
l10:
    STX(RB_SAVX);
    LDA(RD(AX(RB_INITL)));
    AND(0xFE);
    CMP(0x4E);
    if (BCC) goto l20;
    LDA(0);
l20:
    TAY();
    rb_dspalp(m);                           /* DISPLAY ALPHA */
    LDX(RD(RB_SAVX));
    INX();
    DEC(RB_TEMP1);
    if (BPL) goto l10;
    INC(RB_SAVX);                           /* POINT @ NEXT INITIAL */
    return;
}

/* DISPLAY CHARACTER */
/* Y = CHAR OFFSET ( INTO VGMSGA ) */
/* DSPALP (5201) */
void rb_dspalp(rb_machine *m)
{
    LDX(RD(AY(RB_VGMSGA + 1)));
    LDA(RD(AY(RB_VGMSGA)));
    rb_vgadd2(m); return;
}

/* SET-UP NEW BANNER */
/* NWBNNR (520A) */
void rb_nwbnnr(rb_machine *m)
{
    LDA(RD(RB_GAMODE));
    if (BNE) goto l10;                      /* ONLY ATTRACT */
    LDA(RD(RB_HSCRES + (7 * 5 - 4)));       /* WAIT FOR 7 HIGH SCORES */
    if (BEQ) goto l2;
    BIT(RD(RB_STATUS + 1));                 /* IS POKEY THERE */
    if (BMI) goto l2;
    DEC(RB_S_FLAG);                         /* NO [PLAY WITH VG RESTART] */
l2:
    LDA(0);
    LDX(RB_L_OBJ - 1);
l5:
    STA(ZX(RB_PLSTAT));                     /* CLR PLANE DB */
    DEX();
    if (BPL) goto l5;
    rb_stplne(m);                           /* SET-UP PLANE */
    LDA(0);
    STA(RB_PLSTAT + 2);
    STA(RB_PLSTAT + 3);
    LDA(RB_BNRBIT + 3);
    STA(RB_GAMODE);
l10:
    return;
}

/* DISPLAY BANNER ( ATTRACT SEQUENCE ) */
/* DSPBNR (5231) */
void rb_dspbnr(rb_machine *m)
{
    LDX(RB_N_MOB);
    STX(RB_SAVX + 2);
    rb_plnlbs(m);                           /* POSITION PLANE */
    if (BCS) goto l5;                       /* NOT VISIBLE */
    RB_TAGGED(RB_TAG_PLANE, rb_plane(m));   /* DISPLAY PLANE */
    LDX(RB_N_MOB);
    rb_plnlbs(m);                           /* POSITION BEAM */
    LDA(HI(RB_BANNER));
    LDY(LO(RB_BANNER));
    LDX(RB__BNNR);
    RB_TAGGED(RB_TAG_STATES, rb_loadram(m)); /* BANNER OUTLINE */
    LDA(36);
    rb_message(m);                          /* DISPLAY ' RED BARON ' */
l5:
    CLC();
    LDA(RD(RB_PLSTAT));                     /* UPDATE X POSITION */
    ADC(0xE0);
    STA(RB_PLSTAT);
    LDA(RD(RB_PLSTAT + 1));
    ADC(0xFF);
    STA(RB_PLSTAT + 1);
    SEC();
    SBC(RD(RB_UNIV4X + 1));
    CMP(0xF4);                              /* DESTINATION POSITION */
    if (BPL) goto l10;
    LDA(RD(RB_GAMODE));                     /* CLR BANNER (TO NEW MODE) */
    AND(0xC0);
    STA(RB_GAMODE);
    rb_initial(m); return;
l10:
    return;
}

