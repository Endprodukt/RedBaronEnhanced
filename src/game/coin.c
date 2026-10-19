/* coin.c - RBCOIN, TCN65: THE COIN ROUTINE
 *
 * Red Baron (Atari, 1981), RBCOIN of the original source as C.
 * Written by tools/mac2c.py from upstream/TCN65.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* INSTRUCTIONS IN BRACKETS( "[" AND "]" ) ARE FOR ILLUSTRATION ONLY, AND ARE NOT */
/* ACTUALLY ASSEMBLED. */
/* MOOLAH (70B7) */
void rb_moolah(rb_machine *m)
{
    LDX(RB_OFFSET * (RB_MECHS - 1));        /* X IS USED TO INDEX FROM RIGHT TO LEFT COIN MECH */
detct:
    LDA(RD(RB_COINA));                      /* GET COIN SWITCHES */
    CPX(RB_OFFSET);                         /* WHICH MECH ARE WE DOING */
    if (BEQ) goto detct_l11;                /* MIDDLE (X=OFFSET) SHIFT TWICE */
    if (BCS) goto detct_l12;                /* RIGHT (X=2*OFFSET) SHIFT ONCE */
    LSRA();                                 /* ELSE LEFT, SHIFT THRICE */
detct_l11:
    LSRA();
detct_l12:
    LSRA();
    LDA(RD(ZX(RB_CNSTT)));
    AND(31);                                /* SHARED INST. SEE BELOW IN BRACKETS [] */
    if (BCS) goto detct_l5;                 /* BRANCH IF INPUT HIGH (COIN ABSENT) */
    /* [AND I,31.]             ISOLATE COIN-ON DOWN-COUNTER, RESET COIN-OFF UP-CTR. */
    if (BEQ) goto detct_l1;                 /* STICK AT 0 (TERMINAL COUNT) */
    CMP(27);                                /* IN FIRST FIVE SAMPLES? */
    if (BCS) goto detct_l10;                /* YES, RUN FAST */
    TAY();                                  /* ELSE SAVE STATUS */
    LDA(RD(RB_INTCT));                      /* CHECK INTERUPT CTR */
    AND(7);                                 /* ARE D0-D2 ALL ONES? */
    CMP(7);                                 /* SET CARRY IF SO */
    TYA();                                  /* STATUS BACK INTO ACC */
    if (BCC) goto detct_l1;                 /* SKIP IF NOT ALL ONES */
detct_l10:
    SBC(1);                                 /* CARRY SET */
detct_l1:
    STA(ZX(RB_CNSTT));                      /* SAVE UPDATED STATUS */
    LDA(RD(RB_LAM));                        /* CHECK SLAM SWITCH */
    AND(RB_LMBIT);
    if (BNE) goto detct_l2;                 /* BRANCH IF BIT HI (SWITCH OFF) */
    LDA(RB_PRST * 8);                       /* ELSE SET PRE-COIN SLAM TIMER */
    STA(RB_LMTIM);                          /* DECR. 8 TIMES/FRAME=PRST FRAMES */
detct_l2:
    LDA(RD(RB_LMTIM));                      /* CHECK PRE-COIN SLAM TIMER */
    if (BEQ) goto detct_l3;                 /* O.K. */
    DEC(RB_LMTIM);                          /* ELSE RUN TIMER */
    LDA(0);
    STA(ZX(RB_CNSTT));                      /* CLEAR COIN STATUS */
    STA(ZX(RB_PSTSL));                      /* CLEAR POST-COIN SLAM-TIMER */
detct_l3:
    CLC();                                  /* DEFAULT "NO COIN DETECTED" */
    LDA(RD(ZX(RB_PSTSL)));                  /* CHECK POST-COIN SLAM-TIMER */
    if (BEQ) goto detct_l8;                 /* EMPTY, PROCEED */
    DEC(ZX(RB_PSTSL));                      /* RUN TIMER */
    if (BNE) goto detct_l8;                 /* NOT DONE, PROCEED */
    SEC();                                  /* WHEN IT BECOMES ZERO, INDICATE A COIN */
    if (BCS) goto detct_l8;                 /* (ALWAYS) */
detct_l5:
    CMP(27);                                /* IS COIN VALID YET (ON FOR >4 SAMPLES) */
    if (BCS) goto detct_l6;                 /* NO, RESET IT */
    LDA(RD(ZX(RB_CNSTT)));                  /* GET STATUS AGAIN */
    ADC(32);                                /* BUMP COIN-OFF UP-CTR. */
    if (BCC) goto detct_l1;                 /* IF IT DIDN'T WRAP, JUST STORE STATUS */
    if (BEQ) goto detct_l6;                 /* IT WRAPPED BUT COIN WAS ON TOO LONG, JUST RESET */
    CLC();                                  /* SET "VALIDITY" AGAIN */
detct_l6:
    LDA(31);                                /* RESET DOWN-COUNTER */
    if (BCS) goto detct_l1;                 /* BRANCH IF COIN TOO LONG OR TOO SHORT */
    STA(ZX(RB_CNSTT));                      /* SAVE RESET STATUS */
    /* [CLC]                   DEFAULT TO "NO COIN" (CARRY IS ALREADY CLEAR) */
    LDA(RD(ZX(RB_PSTSL)));                  /* CHECK HOWIES ASSUMPTION */
    if (BEQ) goto detct_l7;                 /* BRANCH IF $PSTSL VACANT */
    SEC();                                  /* ELSE GIVE CREDIT A LITTLE EARLY */
detct_l7:
    LDA(RB_POST * 4);                       /* /(4 COUNTS/FRAME)="POST" FRAMES */
    STA(ZX(RB_PSTSL));                      /* DELAY ACCEPTANCE FOR POST/60 SEC. */
detct_l8:
    if (BCC) goto detct_l9;
    /* FALL THROUGH TO "MECH-MULTIPLIERS" */
    /* ---- MECH-MULTIPLIERS ---- */
    LDA(0);                                 /* START WITH 0 (TO ADD 1) */
    CPX(RB_OFFSET);                         /* CHECK WHICH MECH */
    if (BCC) goto detct_l85;                /* IF LEFT, ALWAYS ADD 1 */
    if (BEQ) goto detct_l83;                /* IF CENTER, CHECK HALF-MUL */
    LDA(RD(RB_CMODE));
    AND(0xC);
    LSRA();
    LSRA();
    if (BEQ) goto detct_l85;                /* 00-ADD 1 */
    ADC(2);                                 /* ELSE MAP 1,2,3 TO 3,4,5 */
    if (BNE) goto detct_l85;                /* (ALWAYS) */
detct_l83:
    LDA(RD(RB_CMODE));                      /* GET COIN MODE FROM ZERO-PAGE */
    AND(0x10);                              /* ISOLATE HALF-MULTIPLIER */
    if (BEQ) goto detct_l85;
    LDA(1);
detct_l85:
    SEC();
    PHA();
    ADC(RD(RB_BCCNT));                      /* UPDATE BONUS-ADDER COUNTER */
    STA(RB_BCCNT);
    PLA();
    SEC();
    ADC(RD(RB_CNCT));
    STA(RB_CNCT);
    INC(ZX(RB_CCTIM));                      /* "QUEUE" PULSE FOR E.M. COUNTER */
detct_l9:
    DEX();
    if (BMI) goto detct_l120;
    goto detct;
    /* FALL THRU TO BONUS-ADDER */
    /* ---- BONUS-ADDER ---- */
detct_l120:
    LDA(RD(RB_CMODE));
    LSRA();
    LSRA();
    LSRA();
    LSRA();
    LSRA();                                 /* ISOLATE BONUS-ADDER MODE IN BITS 0-2 */
    TAY();
    LDA(RD(RB_BCCNT));
    SEC();
    SBC(RD(AY(RB_MODLO)));                  /* SEE IF ENOUGH UNIT-COINS HAVE ACCUMULATED */
    if (BMI) { rb_extb(m); return; }        /* BRANCH IF NOT */
    STA(RB_BCCNT);                          /* ELSE UPDATE BONUS-ADDER AND... */
    INC(RB_BC);                             /* GIVE ONE OR TWO BONUS UNIT-COINS */
    CPY(3);
    if (BNE) { rb_extb(m); return; }
    INC(RB_BC);                             /* MODE 3 YIELDS 2 BONUS COINS FOR 4 INSERTED */
    if (BNE) { rb_extb(m); return; }        /* BRA */
}

/* 716C $MODLO: .BYTE 7F,2,4,4,5,3,7F,7F   ;7F IS USED TO GENERATE ZERO BONUS COINS */
/* FALL THROUGH TO CONVERT COINS TO CREDITS */

/* ---- COINS TO CREDITS ---- */

/* $EXTB (7174) */
void rb_extb(rb_machine *m)
{
    LDA(RD(RB_CMODE));                      /* GET COIN MODE FROM ZERO-PAGE */
    AND(3);                                 /* ISOLATE COINS-CREDITS OPTION */
    TAY();                                  /* SAVE IT */
    if (BEQ) goto l2;                       /* IF FREE PLAY CMODE=0, DO NOTHING */
    LSRA();                                 /* ELSE FORM PRICE (0,1,1,2) */
    ADC(0);
    EOR(0xFF);
    SEC();
    ADC(RD(RB_CNCT));                       /* ACC <- COINCT-PRICE */
    if (BCS) goto l33;                      /* BRANCH IF NO BORROW */
    /* [CLC] */
    ADC(RD(RB_BC));                         /* ADD IN BONUS COINS-SEE IF THEY HELP */
    if (BMI) goto ext;                      /* BRANCH IF COINCT+BONUS COINS <PRICE */
    STA(RB_BC);                             /* ELSE ACC=UNUSED BONUS COINS */
    LDA(0);                                 /* ACC=NEW $CNCT */
l33:
    CPY(2);                                 /* Y=COIN MODE-COIN MODE 2 OR 3? */
    if (BCS) goto l1;                       /* BRANCH IF MODE 2 OR 3-GIVE 1 CREDIT */
    INC(RB_CRDT);                           /* ELSE GIVE 2 FOR MODE 1 */
l1:
    INC(RB_CRDT);
l2:
    STA(RB_CNCT);                           /* UPDAT COINCT */
    /* FALL THROUGH TO HANDLE E.M. COUNTERS */
    /* ---- ELECTRO-MECH. CTRS ---- */
ext:
    LDA(RD(RB_INTCT));                      /* GET $INTCT */
    LSRA();                                 /* USE LSB FOR PULSE=4 FRAMES */
    if (BCS) goto ext_l99;
    LDY(0);                                 /* START WITH FLAG OF 0 */
    LDX(RB_OFFSET * (RB_EMCTRS - 1));
ext_l1:
    LDA(RD(ZX(RB_CCTIM)));                  /* CHECK TIMER(X) */
    if (BEQ) goto ext_l3;                   /* NEITHER RUNNING NOR PENDING */
    CMP(0x10);                              /* IS IT RUNNING */
    if (BCC) goto ext_l3;                   /* NO, SKIP */
    ADC(0xEF);                              /* ELSE DEC 4 MSB */
    INY();                                  /* SET ON FLAG */
    STA(ZX(RB_CCTIM));
ext_l3:
    DEX();
    if (BPL) goto ext_l1;
    TYA();                                  /* CHECK "ON" FLAG */
    if (BNE) goto ext_l99;                  /* SKIP IF ANY ON */
    /* IF NONE OF THE COUNTERS ARE CURRENTLY ON, WE CHECK TO SEE IF ANY CAN BE */
    /* STARTED. */
    LDX(RB_OFFSET * (RB_EMCTRS - 1));
ext_l4:
    LDA(RD(ZX(RB_CCTIM)));                  /* NEED WE START THIS ONE */
    if (BEQ) goto ext_l5;                   /* NO, NO COUNTS PENDING */
    CLC();
    ADC(0xEF);                              /* SET 4 MSB, DEC 4 LSB */
    STA(ZX(RB_CCTIM));                      /* START TIMER */
    if (BMI) goto ext_l99;                  /* EXIT, SO WE DON'T START MORE */
ext_l5:
    DEX();
    if (BPL) goto ext_l4;
ext_l99:
    return;
}

