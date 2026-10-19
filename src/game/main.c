/* main.c - MAIN LOOP, SOUNDS, END OF LIFE, GAME INITIALIZATION, SCORE
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- MAIN LOOP ---- */

/* 4800 (not reached) JMP PWRON  ;POWER ON */

/* ---- CHKSUM 2 (4800-4FFF)[FF] ---- */

/* 4803 CHKSM2: .BYTE 05 */
/* START (4804) */
void rb_start(rb_machine *m)
{
    LDA(RD(RB_EAFLG));
    if (BNE) goto l10;                      /* BUSY */
    STA(RB_EAX);
    STA(RB_EABC);
    LDA(RB_EAFILL & 0xFF);
    STA(RB_EASRCE);
    LDA(RB_EAFILL / 0x100);
    STA(RB_EASRCE + 1);                     /* EA SOURCE = HIGH SCORES/INITIALS */
    LDA(6 * 6);
    STA(RB_EACNT);                          /* MAX COUNT */
    LDA(0x20);
    STA(RB_EAFLG);                          /* READ */
l10:
    rb_gminit(m);                           /* INITIALIZE PF */
    LDA(0x20);                              /* HALT FOR FIRST FRAME */
    STA(RB_VGRAM + 3);
    LDA(1);
    STA(RB_VGRAM);                          /* INITIALIZE TO BUFFER 0 */
    LDA(0xE0);
    STA(RB_VGRAM + 1);
    STA(RB_BUFFER);                         /* INITIALIZE BUFFER FLAG (D7 SET) */
    LDA(8);
    STA(RB_ST_LED);                         /* INITIALIZE START LED OFF */
    rb_main(m);                             /* (falls into it) */
}

/* MAIN (4835) */
void rb_main(rb_machine *m)
{
    DEC(RB_S_FLAG);                         /* <>0 (SET CALCULATION LOOP FLAG) */
    LDA(RD(RB_OPT1));
    EOR(0xFF);
    STA(RB_OPTION);                         /* SET OPTION */
    LDA(RD(RB_OPT0));
    EOR(0xFF);                              /* SET COIN MODES */
    STA(RB_CMODE);                          /* SET COIN OPTIONS */
    rb_eaupd(m);
    rb_setdiv(m);
    rb_credit(m);                           /* CHECK RTP MODE */
    rb_strgme(m);                           /* START GAME */
    LDA(RD(RB_GMEND1));
    if (BMI) goto l10;                      /* END OF LIFE */
    LDA(RD(RB_GAMODE));
    if (BPL) goto l5;                       /* ONLY IN PLAY */
    rb_potscl(m);                           /* SCALE POT VALUES */
    rb_newshl(m);                           /* GENERATE SHELLS */
l5:
    LDA(RD(RB_GAMODE));
    LSRA();
    if (BCS) { rb_view_begin(m); return; }  /* FREEZE */
    BIT(RD(RB_GREND));
    if (BVS) { rb_view_begin(m); return; }  /* PLAYER RAN INTO GROUND */
    rb_pfmotn(m);                           /* UPDATE PLAYFIELD */
    BIT(RD(RB_GRMODE));
    if (BVS) { rb_view_begin(m); return; }  /* GROUND SEQUENCE */
    rb_nwplne(m);                           /* GENERATE MOTION OBJECTS */
l10:
    rb_plmotn(m);                           /* UPDATE MOTION OBJECTS (PLANES & DRONES) */
    rb_view_begin(m); return;               /* (the frame's picture: game/frame.h) */
}

/* MAIN_VIEW (4875): the rest of the routine */
void rb_main_view(rb_machine *m)
{
    LDA((RB_VGRAM & 0xFF00) / 0x100);       /* INITIALIZE RAM POINTER */
    LDY((RB_VGRAM + 2) & 0xFF);
    BIT(RD(RB_BUFFER));                     /* BUFFER # ? */
    if (BPL) goto l25;                      /* # 1 ( BECOMES 0 ) */
    LDA(((RB_VGRAM + 0x802) & 0xFF00) / 0x100); /* ELSE WILL BE BUFFER 1 */
l25:
    STA(RB_RAMPTR + 1);
    STY(RB_RAMPTR);
    LDA((RB_NOREST & 0xFF00) / 0x100);
    LDY(RB_NOREST & 0xFF);
    LDX(0x12);
    RB_TAGGED(RB_TAG_COCKPIT, rb_loadram(m));
    rb_pftrig(m);                           /* SET PF SINE/COSINE */
    LDA(RD(RB_GAMODE));
    AND(2);
    if (BNE) goto l40;                      /* HSTB DISPLAY/INPUT */
    LDA(RD(RB_GMEND1));
    ASLA();
    ASLA();
    if (BMI) goto l35;
    rb_trns0(m);
    RB_TAGGED(RB_TAG_LAND, rb_scene(m));    /* DISPLAY PLAYFIELD */
    RB_TAGGED(RB_TAG_GSHELL, rb_grsmtn(m)); /* GROUND SHELLS */
    RB_TAGGED(RB_TAG_PLANE, rb_motobj(m));  /* DISPLAY MOTION OBJECTS */
    RB_TAGGED(RB_TAG_BLIMP, rb_blmotn(m));  /* MOVE, DISPLAY BLIMP */
l35:
    RB_TAGGED(RB_TAG_SHELL, rb_shlmot(m));  /* MOVE, DISPLAY SHELLS (& COLLISION DETECT) */
l40:
    LDA(RD(RB_RAMPTR));
    STA(RB_COPYPT);                         /* SAVE LOCATION OF COPYRIGHT MESSAGE */
    LDA(RD(RB_RAMPTR + 1));
    STA(RB_COPYPT + 1);
    LDA(RB_RCOPYR / 0x100);
    LDY(RB_RCOPYR & 0xFF);
    LDX(0x36);
    RB_TAGGED(RB_TAG_COPYRIGHT, rb_loadram(m)); /* DISPLAY COPYRIGHT */
    RB_TAGGED(RB_TAG_LIVES, rb_dsplif(m));  /* DISPLAY PLAYER LIVES */
    RB_TAGGED(RB_TAG_EOL, rb_eolseq(m));    /* END OF LIFE SEQUENCE */
    RB_TAGGED(RB_TAG_STATES, rb_states(m)); /* DISPLAY STATES (HSTB...) */
    RB_TAGGED(RB_TAG_TEXT, rb_alphas(m));   /* DISPLAY ALPHANUMERICS */
    rb_vghalt(m);                           /* HALT VG (W/ CNTR) */
    rb_sounds(m);
    LDX(2);
l45:
    LDA(RD(AX(RB_PKYRAN)));
    STA(AXW(RB_PKYRAN + 1));
    DEX();
    if (BPL) goto l45;
    LDA(RD(RB_POKEY + 0xA));
    STA(RB_PKYRAN);
    DEC(RB_PKYCNT);
    if (BNE) goto l65;
    LDA(RD(RB_POKEY + 0xA));                /* POKEY RANDOM */
    LDX(3);
l50:
    CMP(RD(AX(RB_PKYRAN)));                 /* CHECK WITH PREVIOUS RANDOMS */
    if (BNE) goto l55;                      /* X > 0 (SHOULD BE PLAY) */
    DEX();
    if (BPL) goto l50;
    /* X=0FF (SHOULD BE ATTRACT/RTP) */
l55:
    TXA();
    EOR(RD(RB_PKYCNT + 1));
    STA(RB_STATUS + 1);                     /* SET POKEY STATUS */
    LDA(0x10);                              /* CHECK POKEY EVERY 10 FRAMES */
    STA(RB_PKYCNT);                         /* RESET COUNTER */
l65:
    rb_random(m);                           /* MAINTAIN PSEUDO RANDOM (POLY COUNTER) */
    rb_percent(m);                          /* GAME PERCENTAGING */
    rb_intwait(m);                          /* WAIT FOR EOF */
    return;
}

/* MAIN2 (4908): the rest of the routine */
void rb_main2(rb_machine *m)
{
    LDA(RD(RB_BUFFER));
    EOR(0x80);
    STA(RB_BUFFER);                         /* SWITCH FLAG (D7) */
    rb_plprop(m);                           /* SWITCH PLANE PROP */
    LDA(0xA4);                              /* SWITCH BUFFER */
    LDY(0x35);
l70:
    EOR(RD(IY(RB_COPYPT)));                 /* CHECK FOR ATARI COPYRIGHT */
    DEY();
    if (BPL) goto l70;
    /* SET STATUS (SHOULD BE 4) */
    LDX(RD(5 * 5 - 4 + RB_HSCRES));
    if (BNE) goto l75;
    LDA(4);
l75:
    EOR(RD(RB_VGRAM + 1));
    STA(RB_VGRAM + 1);
    STA(RB_VGON);                           /* START VG */
    INC(RB_FRAME);
    LDA(RD(RB_LMTIM));
    if (BEQ) goto l85;
    LDA(0xF6);                              /* ALL SOUNDS */
l85:
    STA(RB_SLAMSN);
    LDA(RD(RB_S_VAL));                      /* SHELL NOISE COUNTER */
    if (BEQ) goto l90;
    DEC(RB_S_VAL);                          /* DEC ONCE PER FRAME */
l90:
    return;                                 /* CONTINUE */
}

/* PLPROP (493E) */
void rb_plprop(rb_machine *m)
{
    LDY(RD(RB_PROP_F));                     /* GET PICTURE OFFSET */
    if (BMI) goto l20;                      /* NO PLANE PROP */
    INY();
    INY();
    CPY(6);
    if (BCC) goto l5;
    LDY(0);
l5:
    STY(RB_PROP_F);
    BIT(RD(RB_BUFFER));
    if (BMI) goto l10;
    LDA(RD(AY(RB__PROPS + 6)));             /* GET PICTURE JSRL */
    LDX(RD(AY(RB__PROPS + 7)));
    STA(RB_VGRAM + 0xF94);
    STX(RB_VGRAM + 0xF95);
    if (BNE) goto l20;                      /* ALWAYS */
l10:
    LDA(RD(AY(RB__PROPS)));
    LDX(RD(AY(RB__PROPS + 1)));
    STA(RB_VGRAM + 0x794);                  /* AND STORE IN 'VECTOR' LOCATION */
    STX(RB_VGRAM + 0x795);
l20:
    LDY(RD(RB_PLYPRP));
    INY();
    INY();
    CPY(0xE);
    if (BCC) goto l40;
    LDY(0);
l40:
    BIT(RD(RB_GMEND1));
    if (BMI) goto l60;
    LDA(RD(RB_GAMODE));
    AND(2);
    if (BNE) goto l60;                      /* NOT WITH HST INFO */
    STY(RB_PLYPRP);
    LDA(RD(AY(RB_LNEPRP + 1)));             /* GET JSRL */
    LDX(RD(AY(RB_LNEPRP)));
    BIT(RD(RB_BUFFER));
    if (BMI) goto l50;
    STA(RB_VGRAM + 0x813);
    STX(RB_VGRAM + 0x812);
    return;
l50:
    STA(RB_VGRAM + 0x13);
    STX(RB_VGRAM + 0x12);
l60:
    return;
}

/* WAIT FOR FRAME END (INTERRUPT COUNT SETS FLAG) AND VG HALT */
/* INTWAIT: written by hand, src/game/frame.c */

/* SET DIVIDE COUNT */
/* SETDIV (49A8) */
void rb_setdiv(rb_machine *m)
{
    LDA(RB_D_NBIT);
    STA(RB_DD_NL);                          /* SET DIVIDE BIT(S) */
    LDA(0);
    STA(RB_D_NMLL);                         /* CLR LOWER NUMERATOR 16 BITS */
    STA(RB_D_NMLH);
    return;
}

/* CHECK CREDIT */
/* -IF CREDIT & IN ATTRACT THEN RTP */
/* CREDIT (49B6) */
void rb_credit(rb_machine *m)
{
    LDX(50);                                /* MAX CREDITS @ 50. */
    CPX(RD(RB_CRDT));
    if (BCS) goto l4;
    STX(RB_CRDT);                           /* SET TO MAX */
l4:
    LDA(RD(RB_GAMODE));                     /* ATTRACT? */
    AND(0xC4);
    if (BNE) goto l20;                      /* HSTD INPUT/RTP/PLAY */
    LDA(RD(RB_CRDT));                       /* CREDIT? */
    if (BNE) goto l6;                       /* CREDIT */
    LDA(RD(RB_CMODE));                      /* FREE PLAY ? */
    AND(3);
    if (BNE) goto l10;                      /* NO */
    INC(RB_CRDT);                           /* FP */
    INC(RB_CRDT);
l6:
    CMP(2);
    if (BCS) goto l8;
    LDA(RD(RB_OPTION));
    AND(0x40);
    if (BEQ) goto l8;                       /* NOT 2 PLAY MINIMUM */
    LDA(RD(RB_STARTF));
    if (BEQ) goto l15;                      /* GAME HAS NOT STARTED (NEED 2 CREDITS) */
l8:
    LDA(RD(RB_GAMODE));
    CMP(8);
    if (BCC) goto l7;
    LDA(0);                                 /* CLR 2 COIN MINIMUM STATE (D3) */
l7:
    AND(0x33);                              /* FREEZE, DISPLAY STATUS */
    ORA(RB__STRTP);                         /* SET READY TO PLAY (RTP) */
    STA(RB_GAMODE);
    return;
l10:
    LDA(RD(RB_CNCT));                       /* COINS */
    if (BEQ) goto l20;                      /* NO */
l15:
    LDA(RD(RB_GAMODE));                     /* ELSE 2-COIN MIN */
    ORA(0xB);                               /* D3 SET (AND FREEZE BITS) */
    STA(RB_GAMODE);
    return;
l20:
    STA(RB_STARTF);                         /* CLR START FLAG (IN ATTRACT) */
    return;
}

/* MAINTAIN PLAY MODE SOUNDS */
/* SOUNDS (49FD) */
void rb_sounds(rb_machine *m)
{
    LDA(RD(RB_EXCNTR));                     /* ANY EXPLOSION? */
    if (BEQ) goto l10;                      /* NO */
    DEC(RB_EXCNTR);
    LDA(RD(RB_EXPVAL));                     /* ELSE COUNT DOWN SOUND */
    SEC();
    SBC(0x10);
l10:
    STA(RB_EXPVAL);                         /* TOP NIBBLE EXPLOSION BITS */
    BIT(RD(RB_GAMODE));
    if (BPL) goto l30;                      /* NOT PLAY */
    LDX(RD(RB_POINT + 4));                  /* OTHER SOUNDS ON ? (HIGHER PRIORITY) */
    if (BNE) goto l70;                      /* YES */
    LDA(RD(RB_GMEND1));
    if (BMI) goto l30;                      /* EOL */
    LDA(0xA1);
    STA(RB_POKEY + 5);                      /* TONES/VOLUME */
    STA(RB_POKEY + 7);
    BIT(RD(RB_GRMODE));
    if (BPL) goto l20;
    LDA(RD(RB_PLSTAT + 6));
    AND(0x20);
    if (BNE) goto l60;                      /* PLANE NOT COMING @ PLAYER */
l20:
    LDA(RD(RB_ATGVAL + 1));
    if (BEQ) goto l60;                      /* DEFAULT FREQ. */
    TAX();
    STX(RB_POKEY + 4);                      /* SET FREQUENCY */
    LSRA();
    LSRA();
    LSRA();
    LSRA();
    LSRA();
    STA(RB_TEMP1);
    TXA();
    CLC();
    ADC(RD(RB_TEMP1));                      /* FREQUENCY *(1+1/32) */
    SEC();
    SBC(8);
    STA(RB_POKEY + 6);
    CLC();
    LDA(RD(RB_ATGVAL));                     /* STEP FREQUENCY */
    ADC(RD(RB_ATGDEL));
    STA(RB_ATGVAL);
    LDA(RD(RB_ATGVAL + 1));
    ADC(RD(RB_ATGDEL + 1));
    STA(RB_ATGVAL + 1);
    CMP(0x30);
    if (BCS) goto l70;
l30:
    LDA(0);
    STA(RB_ATGVAL + 1);
    STA(RB_POKEY + 5);                      /* CLR TONES */
    STA(RB_POKEY + 7);
    return;
l60:
    LDX(0xF8);                              /* MAINTAIN BACKGROUND 'HUM' */
    STX(RB_POKEY + 4);
    DEX();
    STX(RB_POKEY + 6);
l70:
    return;
}

/* CHECK FOR PF SOUND CLR ( GROUND MODE ) */
/* PFSCLR (4A6A) */
void rb_pfsclr(rb_machine *m)
{
    CPX(RD(RB_PFSNDF));                     /* PROPER PF MOUNTAIN */
    if (BNE) { rb_pfsx(m); return; }        /* NO */
    rb_pfsoff(m);                           /* (falls into it) */
}

/* PFSOFF (4A6F) */
void rb_pfsoff(rb_machine *m)
{
    BIT(RD(RB_GRMODE));
    if (BPL) { rb_pfsx(m); return; }        /* ONLY IN GROUND MODE */
    LDA(0);
    STA(RB_ATGVAL + 1);                     /* SHUT OFF SOUND */
    rb_pfsx(m);                             /* (falls into it) */
}

/* PFSX (4A77) */
void rb_pfsx(rb_machine *m)
{
    return;
    (void)m;
}

/* END OF LIFE SEQUENCE */
/* EOLSEQ (4A78) */
void rb_eolseq(rb_machine *m)
{
    BIT(RD(RB_GMEND1));
    if (BMI) goto l30;                      /* EOL ( END OF LIFE ) */
    BIT(RD(RB_GREND));
    if (BPL) goto l10;                      /* NOT EOL BY SHELLS/GROUND COLLISION */
    LDA(0);
    LDX(0xC0);
    if (BVC) goto l5;
    LDA(0xF);
    LDX(0xA0);
l5:
    STA(RB_EOGTMR);
    STX(RB_GMEND1);
    LDA(0x82);
    STA(RB_GMEND0);                         /* PLANES FLY AWAY */
    rb_plnzd0(m);                           /* SET Z DELTAS */
    goto l30;                               /* CONTINUE EOL */
l10:
    BIT(RD(RB_GMEND0));
    if (BVC) goto l30;                      /* NO SHELLS FROM BEHIND */
    LDA(RD(RB_PLSTAT + 7));
    LSRA();
    if (BCS) goto l15;
    rb_spray(m);                            /* TIME ENEMY SHELLS W/ RETURNING PLANE */
l15:
    LDA(RD(RB_PLSTAT + 7));
    CMP(0xC);
    if (BNE) goto l25;
    LDA(RD(RB_ENSIDE));                     /* CHECK FOR PLAYER DEATH */
    EOR(RD(RB_PLDELX));
    if (BPL) goto l20;                      /* SORRY WRONG X */
    LDA(RD(RB_PLDELX));
    rb_spabs(m);
    CMP(0x1C);
    if (BCC) goto l20;                      /* PLAYER DIES */
    BIT(RD(RB_BEFLAG));
    if (BPL) goto l17;                      /* FIRST TIME FREE */
    rb_random(m);
    if (BMI) goto l20;                      /* AFTER - ONLY 50/50 */
l17:
    LDA(-1);
    STA(RB_BEFLAG);                         /* SET FLAG */
    if (BMI) goto l30;                      /* ALWAYS */
l20:
    LDA(RD(RB_GMEND0));                     /* EOL BY RETURNING PLANE */
    ORA(1);
    STA(RB_GMEND0);
l25:
    LDA(RD(RB_GMEND0));
    LSRA();
    if (BCC) goto l30;                      /* PLAYER DOES NOT DIE */
    LDA(RD(RB_B_HOLE));
    CMP(0xC);
    if (BCS) goto l30;
    INC(RB_B_HOLE);                         /* STEP BULLET HOLE SEQUENCE */
l30:
    rb_wndshd(m);
    LDA(RD(RB_GMEND1));                     /* EOL? */
    if (BPL) goto l35;                      /* NO */
    LDA(0);
    STA(RB_S_VAL);                          /* CLR SHELL SOUND */
    STA(RB_GUN_ST);                         /* CLR OVERHEATED GUNS */
    STA(RB_PLDELX);                         /* NO PLAYER MOTION */
    STA(RB_PLDELY);
    STA(RB_PLDELY + 1);
    LDA(RD(RB_EOGTMR));
    INC(RB_EOGTMR);
    TAX();
    if (BNE) goto l40;
    LDA(0);
    STA(RB_SPIRAL);                         /* INSURE SPIRAL HDWR IS CHARGED */
    LDA(0x40);                              /* ELSE - SCROLL HORIZON, ROTATE PLAYFIELD */
    ORA(RD(RB_GMEND1));
    STA(RB_GMEND1);                         /* (D6=PF EOL FLAG) */
l35:
    return;
l40:
    CMP(RB__TIME2);
    if (BCC) goto l45;
    goto endlfe;                            /* END OF LIFE SEQUENCE DONE */
l45:
    rb_pftrig(m);
    BIT(RD(RB_GREND));                      /* RAN INTO GROUND */
    if (BVC) goto l50;
    LDA(RD(RB_GREND));
    AND(0x3F);
    TAX();
    LDA(RD(AX(RB_PFOBJ + 6)));
    AND(0xF);
    STA(RB_SAVY + 1);                       /* MOUNTAIN # */
    rb_scene1(m);
    rb_scene2(m);
    goto l60;
l50:
    LDA(RD(RB_GMEND0));
    LSRA();
    if (BCS) goto l60;                      /* DEATH BY OVERHEAD PLANE */
    LDA(RD(RB_GMEND1));
    ASLA();
    ASLA();
    if (BMI) goto l60;                      /* NO DISPLAY DURING STAR SEQUENCE */
    rb_vgcntr(m);
    LDX(3);
l55:
    LDA(RD(AX(RB_BULPOS)));
    STA(ZX(RB_XCOMP));
    DEX();
    if (BPL) goto l55;
    LDA(0);
    STA(RB_VGBRIT);                         /* NO INTENSITY */
    STA(RB_OBJECT + 5);
    TAY();
    rb_trig(m);                             /* NO ROTATION */
    LDA(0xE0);
    STA(RB_OBJECT + 4);
    LDX(RB_XCOMP);
    rb_vgvctr(m);                           /* POSITION VCTR */
    rb_dspgrs(m);                           /* DISPLAY SHELL */
l60:
    LDX(RD(RB_EOGTMR));
    CPX(RB__TIME1);                         /* TIME FOR STARFIELD ? */
    if (BCC) goto l75;                      /* NOT YET */
    if (BNE) goto l70;
    LDA(RD(RB_GMEND1));
    ORA(0x70);
    STA(RB_GMEND1);
    LDA(0);
    STA(RB_SPIRAL);                         /* CLR SPIRAL */
    LDX(5);
l65:
    STA(ZX(RB_PLOBDB));
    DEX();
    if (BPL) goto l65;
    BIT(RD(RB_GAMODE));
    if (BPL) goto endlfe;                   /* NO COLLISION SEQUENCE UNLESS GAME */
    LDA(0xF0);
    STA(RB_EXPVAL);
    LDA(8);
    STA(RB_EXCNTR);                         /* START EXPLOSION SOUND */
    STA(RB_GTIMER + 1);                     /* PLANE EXP TIMER */
l70:
    rb_plcrsh(m); return;
l75:
    LDA(2);
    STA(RB_SPIRAL);
    CLC();
    LDA(RD(RB_I4YPOS));
    ADC(RB__EXSCP & 0xFF);
    TAY();
    LDA(RD(RB_I4YPOS + 1));
    ADC((RB__EXSCP & 0xFF00) / 0x100);
    if (BPL) goto l80;
    LDA(0);
    TAY();
l80:
    STY(RB_I4YPOS);
    STA(RB_I4YPOS + 1);
    LDA(RD(RB_PFROTN));                     /* ROTATE PF */
    ADC(0x70);                              /* CARRY CLR */
    STA(RB_PFROTN);
    if (BCC) goto l85;
    INC(RB_PFROTN + 1);
l85:
    return;
endlfe:
    LDA(RD(RB_SCRTAB + 3));
    if (BNE) goto endlfe_l10;
    STA(RB_GRMODE);                         /* CLR GROUND MODE */
    BIT(RD(RB_GAMODE));
    if (BPL) { rb_initial(m); return; }     /* ATTRACT (RESTART) */
    DEC(RB_LIVES);                          /* TAKE LIFE IN PLAY */
    LDA(RD(RB_LIVES));
    if (BNE) { rb_initial(m); return; }
    LDA(8);
    STA(RB_ST_LED);                         /* CLEAR START LED */
    rb_nw_hsc(m); return;                   /* CHECK FOR HSTB ENTRY */
endlfe_l10:
    return;
}

/* GAME INITIALIZATION */
/* GMINIT (4BB5) */
void rb_gminit(rb_machine *m)
{
    LDX(0);
    STX(RB_PKYCNT);                         /* RESET POKEY CHECK COUNTER */
    STX(RB_POKEY + 0xF);                    /* INIT POKEY */
    STX(RB_MODECT);
    LDA(RD(AX(RB_MCOUNT)));
    STA(RB_NEWCT);
    LDY(7);
    STY(RB_POKEY + 0xF);
    LDA(0);
l10:
    STA(AYW(RB_POKEY));                     /* CLR SOUND REGS */
    DEY();
    if (BPL) goto l10;
    STA(RB_POKEY + 8);
    LDA(RD(RB_GAMODE));
    STA(RB_PKYCNT + 1);                     /* SAVE GAME MODE */
    if (BMI) { rb_initial(m); return; }     /* GAME */
    STX(RB_POKEY + 0xF);                    /* SHUT OFF RANDOM NUMBERS IN ATTRACT/RTP */
    rb_initial(m);                          /* (falls into it) */
}

/* INITIAL (4BDE) */
void rb_initial(rb_machine *m)
{
    LDA(RB_WO_CNT);
    STA(RB_PLSTAT + 7);                     /* DISABLE PLANE */
    rb_plnclr(m);
    LDA(0);
    LDX(RB_MESTM2 - RB_PLDELX);
l10:
    STA(ZX(RB_PLDELX));                     /* CLR PLAYER/TIMING/SOUNDS */
    DEX();
    if (BPL) goto l10;
    STA(RB_BLOBJ + 0x18);                   /* CLR BLIMP */
    LDA(0x10);
    STA(RB_I4YPOS);                         /* INITIALIZE HORIZON Y POSITION */
    LDA(2);
    STA(RB_I4YPOS + 1);
    LDX(RB_N_PFOB);                         /* CLR EOL MOUNTAIN AS WELL */
    LDY(6);
l30:
    LDA(0);
    STA(AXW(RB_PFOBJ));                     /* X OFFSET = 0 */
    STA(AXW(RB_PFOBJ + 2));
    LDA((RB_PF_XZ & 0xFF00) / 0x100);
    STA(AXW(RB_PFOBJ + 3));                 /* X DEPTH */
    LDA(RD(AY(RB_PFOBIZ)));
    STA(AXW(RB_PFOBJ + 4));                 /* Y DEPTH */
    LDA(RD(AY(RB_PFOBIZ + 1)));
    STA(AXW(RB_PFOBJ + 5));                 /* Z MSB */
    CMP(0x11);                              /* CHECK INITIAL POSITION */
    TYA();
    if (BCC) goto l57;
    ORA(0x80);                              /* ON HORIZON */
l57:
    STA(AXW(RB_PFOBJ + 6));
    if (BCS) goto l58;
    LDA(RD(AX(RB_PFOBJ + 4)));
    STA(AXW(RB_PFOBJ + 2));                 /* X DEPTH=Y DEPTH (FREE OBJECT) */
    LDA(RD(AX(RB_PFOBJ + 5)));
    STA(AXW(RB_PFOBJ + 3));
l58:
    LDA(RD(AY(RB_PFOBIZ + 9)));             /* X POSITION MSB */
    STA(AXW(RB_PFOBJ + 1));
    DEY();
    DEY();
    TXA();
    SEC();
    SBC(RB_L_PFOB);
    TAX();
    if (BCS) goto l30;                      /* DO ALL ROUND OBJECTS */
    LDX(RB_NGRSHL);
l60:
    LDA(0);
    STA(AXW(RB_GRSHLS + 5));                /* CLR ALL GROUND SHELLS */
    TXA();
    SEC();
    SBC(RB_GRLSHL);
    TAX();
    if (BCS) goto l60;
    LDY(RB_N_SHL);
l70:
    LDA(0);
    STA(AYW(RB_SHELLS + 7));                /* CLR ALL PLAYER SHELLS */
    TYA();
    SEC();
    SBC(RB_L_SHL);
    TAY();
    if (BCS) goto l70;
    return;
}

/* TABLE OF COUNTS BETWEEN PLANE/GROUND WAVES */
/* 4C59 MCOUNT: .BYTE 4,2,3,2 */
/* 4C5D  .BYTE 1,3,4,2 */
/* GROUND OBJECTS - INITIAL DEPTH */
/* +8             - INITIAL X POS */
/* 4C61 PFOBIZ: .WORD 8200,6E0,3220,0D20 */
/* 4C69  .WORD -0C00,-400,400,0C00 */
/* PLAYER EOL CRASH */
/* PLCRSH (4C71) */
void rb_plcrsh(rb_machine *m)
{
    LDA(4);
    LDY(0xA0);
    rb_vgstat(m);
    rb_trns0(m);                            /* ROTATE ABOUT 0 */
    LDA(RD(RB_EOGTMR));
    CMP(RB__TIME1 + 8);                     /* ONLY STARS */
    if (BCS) goto l5;
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDA(0x80);
    STA(RB_O_DPTH);
    LDA(1);
    STA(RB_O_DPTH + 1);                     /* SCALE FACTOR */
    CLC();
    LDA(RD(RB_GTIMER + 1));
    ADC(6);
    STA(RB_GTIMER + 1);                     /* STEP COUNT */
    LDA(0x40);
    STA(RB_PLSTAT + 6);
    RB_TAGGED(RB_TAG_PLANE, rb_plane(m));   /* DISPLAY EXPLODED PLANE */
    rb_upplx0(m);                           /* UPDATE PIECE ROTATION */
l5:
    LDA(RD(RB_EOGTMR));
    SEC();
    SBC(RB__TIME1 + 4);
    if (BCC) goto l45;
    ASLA();                                 /* STAR PATTERNS */
    TAX();
    STX(RB_SAVX);                           /* ITERATION COUNT (*2 */
    LDX(4);
l10:
    STX(RB_SAVX + 1);
    LDA(RD(ZX(RB_PLOBDB + 1)));             /* START ROTATION */
    LDY(RD(ZX(RB_PLOBDB)));
    rb_trig(m);
    LDA(RD(RB_SAVX + 1));                   /* STAR # */
    CLC();
    ADC(RD(RB_SAVX));                       /* + INTERATION COUNT */
    TAX();
    LDA(RD(AX(RB_PLCRSC)));
    STA(RB_OBJECT + 4);
    LDA(RD(AX(RB_PLCRSC + 1)));
    STA(RB_OBJECT + 5);
    if (BMI) goto l30;                      /* NO STAR THIS COUNT */
    rb_vgcntr(m);
    LDA(RD(RB_SAVX + 1));
    LSRA();
    LSRA();
    LDX(0xA);                               /* ASSUME STAR0 (10. POINTS) */
    LDY(RB__STAR0);
    PHP();
    if (BCC) goto l15;
    INX();                                  /* STAR1 (12. POINTS) */
    LDY(RB__STAR1);
l15:
    rb_plnabz(m);
    LDX(RB__STAR0 * 2);
    PLP();
    PHP();
    if (BCC) goto l20;
    LDX(RB__STAR1 * 2);
l20:
    rb_d_loop(m);                           /* SCALE BY DEPTH */
    LDA(HI(RB_DESTR0));
    LDX(LO(RB_DESTR0));
    PLP();
    if (BCC) goto l25;
    LDA(HI(RB_DESTR1));
    LDX(LO(RB_DESTR1));
l25:
    rb_dcmpa0(m);
l30:
    LDX(RD(RB_SAVX + 1));
    DEX();
    DEX();
    if (BPL) goto l10;
    LDA(RD(RB_PLOBDB));                     /* ROTATE STAR 0 */
    SBC(0x80);
    STA(RB_PLOBDB);
    if (BCS) goto l35;
    DEC(RB_PLOBDB + 1);
l35:
    LDA(RD(RB_PLOBDB + 2));                 /* ROTATE STAR 1 */
    ADC(0x60);
    STA(RB_PLOBDB + 2);
    if (BCC) goto l40;
    INC(RB_PLOBDB + 3);
l40:
    LDA(RD(RB_PLOBDB + 4));                 /* ROTATE STAR 2 */
    SBC(0x70);
    STA(RB_PLOBDB + 4);
    if (BCS) goto l45;
    DEC(RB_PLOBDB + 5);
l45:
    return;
}

/* 4D17 PLCRSC: .WORD -1,-1,8000,2000,800,200,80,20,-1,-1 */
/* INITIALIZE GROUND MODE */
/* INITGR (4D2B) */
void rb_initgr(rb_machine *m)
{
    LDA(2);
    STA(RB_GRNDCT);
    LDA(1);
    STA(RB_GTIMER);                         /* MOUNTAIN OBJECT TIME-OUT COUNTER */
    LDA(0xC0);
    STA(RB_GRMODE);                         /* TO GROUND MODE */
    rb_cintbp(m);                           /* (falls into it) */
}

/* CONDITIONAL BLIMP */
/* BLIMP INITIALIZATION */
/* CINTBP (4D37) */
void rb_cintbp(rb_machine *m)
{
    LDA(RD(RB_BLOBJ + 0x18));               /* ACTIVE BLIMP? */
    if (BMI) { rb_intbp0(m); return; }      /* YES */
    rb_initbp(m);                           /* (falls into it) */
}

/* INITBP (4D3B) */
void rb_initbp(rb_machine *m)
{
    rb_lvlstp(m);                           /* STEP LEVEL FOR EACH BLIMP */
    LDA(0);
    STA(RB_GRDSP);
    STA(RB_BLOBJ + 7);                      /* CLR EXPLOSION TIMER */
    STA(RB_BLOBJ + 2);                      /* Y LSB */
    STA(RB_BLOBJ + 4);                      /* Z LSB */
    STA(RB_BLOBJ + 0x14);                   /* Y ANGLE LSB */
    LDA(2);
    STA(RB_BLOBJ + 3);                      /* Y MSB */
    LDA(0x10);
    STA(RB_BLOBJ + 5);                      /* Z MSB */
    LDX(3);
    LDY(0xC0);
    BIT(RD(RB_PLDELX));
    rb_dpabs(m);                            /* -X SIDE */
    STX(RB_TEMP1);
    CLC();
    TYA();
    ADC(RD(RB_UNIV4X));
    STA(RB_BLOBJ);                          /* X LSB */
    TXA();
    ADC(RD(RB_UNIV4X + 1));
    STA(RB_BLOBJ + 1);                      /* X MSB */
    LDA(-2);
    BIT(RD(RB_TEMP1));
    rb_spabs(m);
    STA(RB_BLOBJ + 0x15);                   /* Y ANGLE MSB */
    LDX(-1);
    LDY(-8);
    BIT(RD(RB_TEMP1));
    rb_dpabs(m);
    STA(RB_BLOBJ + 0xD);                    /* X VEL MSB */
    STY(RB_BLOBJ + 0xC);                    /* LSB */
    LDA(0x80);
    STA(RB_BLOBJ + 6);                      /* IN FLIGHT */
    STA(RB_BLOBJ + 0x18);                   /* ACTIVE */
    rb_intbp0(m);                           /* (falls into it) */
}

/* INTBP0 (4D84) */
void rb_intbp0(rb_machine *m)
{
    return;
    (void)m;
}

/* CLR PLANE NON/EXPLODING DRONES */
/* PLNCLR (4D85) */
void rb_plnclr(rb_machine *m)
{
    LDX(0);
    STX(RB_PLSTAT + 6);                     /* CLR PLANE */
    BIT(RD(RB_P_1ST + 0x18));
    if (BPL) goto l5;                       /* DRONE OBJECT */
    BIT(RD(RB_P_1ST + 6));                  /* BLIMP OBJECT */
    if (BVC) goto l7;                       /* CLR */
    if (BVS) goto l10;                      /* EXPLOSION */
l5:
    LDA(RD(RB_P_1ST + 6));
    AND(4);
    if (BNE) goto l10;                      /* DRONE 1 EXPLOSION */
l7:
    STX(RB_P_1ST + 6);                      /* ELSE CLR */
l10:
    LDA(RD(RB_P_2ST + 6));
    AND(4);
    if (BNE) goto l20;                      /* DRONE 2 EXPLOSION */
    STX(RB_P_2ST + 6);
l20:
    return;
}

/* SPRAY (4DA4) */
void rb_spray(rb_machine *m)
{
    LDY(0xC0);
    BIT(RD(RB_ENSIDE));                     /* CHECK ENEMY X (+ OR -) */
    if (BMI) goto l10;
    LDY(0x80);
l10:
    BIT(RD(RB_GAMODE));
    if (BPL) goto l20;
    INC(RB_S_VAL);                          /* STROBE SOUND FOR ENEMY SPRAY */
l20:
    rb_nwshl0(m); return;                   /* SET ENEMY SHELL */
}

/* SEQUENCE SCREEN BULLET HOLES */
/* WNDSHD (4DB5) */
void rb_wndshd(rb_machine *m)
{
    BIT(RD(RB_GAMODE));
    if (BPL) goto l10;                      /* NOT PLAY */
    LDA(RD(RB_B_HOLE));
    AND(0xE);
    TAX();
    if (BNE) goto eolbh;
l10:
    return;
eolbh:
    LDA((RB_SEQUNA & 0xFF00) / 0x100);
    LDY(RB_SEQUNA & 0xFF);
    BIT(RD(RB_ENSIDE));
    if (BPL) goto eolbh_l10;                /* ENEMY STARTS + X */
    LDA((RB_SEQUNB & 0xFF00) / 0x100);      /* ELSE - X */
    LDY(RB_SEQUNB & 0xFF);
eolbh_l10:
    rb_loadram(m); return;                  /* WIND SCREEN BULLET HOLES */
}

/* DISPLAY PLAYER'S LIVES */
/* DSPLIF (4DD0) */
void rb_dsplif(rb_machine *m)
{
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDA(-500 / 4);
    LDX(352 / 4);                           /* POSITION BEAM (-500,352) */
    LDY(0);                                 /* INTENSITY (OFF) */
    rb_vgvtr(m);
    LDX(RD(RB_LIVES));
    DEX();
    CPX(5);
    if (BMI) { rb_dlives(m); return; }
    LDX(4);
    rb_dlives(m);
    LDA(-160 / 4);
    LDX(-0x20 / 4);
    rb_vgvtr1(m);
    LDA(RD(RB_LIVES));
    SEC();
    SBC(5);
    TAX();
    rb_dlives(m);                           /* (falls into it) */
}

/* DLIVES (4DF5) */
void rb_dlives(rb_machine *m)
{
    STX(RB_SAVX);
l5:
    DEC(RB_SAVX);
    if (BMI) goto l10;
    LDA((RB_LPLANE & 0xFF00) / 0x100);
    LDX(RB_LPLANE & 0xFF);
    rb_vgjsrl(m);                           /* DISPLAY LIFE PLANES */
    goto l5;
l10:
    return;
}

/* COUNT IN SCORE (W/AUDIO) */
/* SCOREM (4E06) */
void rb_scorem(rb_machine *m)
{
    LDX(2);
l10:
    LDA(RD(ZX(RB_SCRTAB + 1)));             /* MOVE SCORES UP IN TABLE */
    if (BNE) goto l20;
    LDY(RD(ZX(RB_SCRTAB)));
    STY(ZX(RB_SCRTAB + 1));
    STA(ZX(RB_SCRTAB));
l20:
    DEX();
    if (BPL) goto l10;
    LDA(RD(RB_POINT + 4));                  /* WAIT FOR OTHER SOUNDS (PLANE ENTRY) */
    ORA(RD(RB_POINT + 2));                  /* (300 PT SOUND) */
    ORA(RD(RB_EXCNTR));                     /* (EXPLOSIONS) */
    if (BNE) goto l30;
    LDA(RD(RB_SCRTAB + 3));                 /* ANY SCORES? */
    if (BEQ) goto l21;                      /* NO */
    DEC(RB_SCRTIM);
    if (BNE) goto l22;
    LDA(RD(RB_BNTIME));                     /* BONUS LIFE SOUND ? */
    if (BEQ) goto l25;                      /* NO */
    DEC(RB_BNTIME);                         /* WAIT FOR SOUND TO FINISH */
l21:
    LDA(RB_STINIT);
    STA(RB_SCRTIM);
l22:
    return;
l25:
    LDX(-1);
    LDA(1);                                 /* ASSUME UNITS */
    LDY(RD(RB_SCRTAB + 3));
    CPY(10);
    LDY(RB_STINIT);
    if (BCC) goto l26;                      /* <10. */
    LDY(RB_STINIT * 2);
    LDX(-10);
    LDA(0x10);
l26:
    STY(RB_SCRTIM);                         /* RESET COUNTER */
    SED();                                  /* ADD POINTS TO SCORE */
    CLC();
    ADC(RD(RB_SCORE));
    STA(RB_SCORE);
    LDA(RD(RB_SCORE + 1));
    ADC(0);
    STA(RB_SCORE + 1);
    CLD();
    CLC();
    TXA();
    ADC(RD(RB_SCRTAB + 3));                 /* DEC SCORE TO ADD */
    STA(RB_SCRTAB + 3);
    LDY(0 * 8 + 7);
    CPX(-1);
    if (BEQ) goto l27;
    LDY(4 * 8 + 7);
l27:
    rb_sndon(m);                            /* SCORE SOUND */
    LDA(RD(RB_OPTION));                     /* CHECK FOR BONUS LIFE */
    AND(0xC);
    LSRA();
    LSRA();
    CLC();
    ADC(RD(RB_EXLIFE));
    TAX();
    LDA(RD(RB_SCORE + 1));
    CMP(RD(AX(RB_BONUSL)));
    if (BCC) goto l30;
    CLC();
    LDA(RD(RB_EXLIFE));
    ADC(4);
    STA(RB_EXLIFE);                         /* EXTRA LIFE */
    LDY(2 * 8 + 7);
    rb_sndon(m);                            /* START BONUS LIFE SOUND */
    LDA(0xC);
    STA(RB_BNTIME);                         /* SET TIMER */
    LDA(30);
    STA(RB_MESTM0);                         /* BONUS LIFE MESSAGE TIMER */
    INC(RB_LIVES);
l30:
    return;
}

/* 4E8C BONUSL: .BYTE 2,4,6,0FF   ;BONUS LIFE LEVELS (0FF=NONE) */
/* 4E90  .BYTE 10,15,20,0FF */
/* 4E94  .BYTE 30,40,50,0FF */
/* 4E98  .BYTE 0FF,0FF,0FF,0FF */
/* MANTAIN PERCENTAGE INFO IN EAROM */
/* PERCENT (4E9C) */
void rb_percent(rb_machine *m)
{
    LDA(RD(RB_PRCTIM));
    ORA(RD(RB_PRCTIM + 1));
    ORA(RD(RB_PRCTIM + 2));
    if (BNE) goto l10;                      /* PRESENT */
    LDA(RD(RB_EAFLG));                      /* EAROM IN USE */
    if (BNE) goto l10;
    LDA(RD(RB_OPTION));
    LSRA();
    LSRA();
    AND(0xF);
    TAX();
    LDA(RD(AX(RB_GTOPTS)));
    LSRA();
    ROR(RB_TEMP1);
    LSRA();
    ROR(RB_TEMP1);
    LSRA();
    ROR(RB_TEMP1);
    STA(RB_PRCTIM + 1);
    LDA(RD(RB_TEMP1));
    AND(0xE0);
    STA(RB_PRCTIM);
l10:
    return;
}

/* EXPECTED GAME TIMES PER OPTIONS */
/* 4EC9 GTOPTS: .BYTE 90.   ;NORM GAME TIME */
/* 4ECA  .BYTE 75. */
/* 4ECB  .BYTE 60. */
/* 4ECC  .BYTE 45. */
/* 4ECD  .BYTE 105. */
/* 4ECE  .BYTE 90. */
/* 4ECF  .BYTE 75. */
/* 4ED0  .BYTE 60. */
/* 4ED1  .BYTE 120. */
/* 4ED2  .BYTE 105. */
/* 4ED3  .BYTE 90. */
/* 4ED4  .BYTE 75. */
/* 4ED5  .BYTE 135. */
/* 4ED6  .BYTE 120. */
/* 4ED7  .BYTE 105. */
/* 4ED8  .BYTE 90. */
