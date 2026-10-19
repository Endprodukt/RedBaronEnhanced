/* sound.c - RBSOUN: POKEY SOUND SYSTEM CONTROL
 *
 * Red Baron (Atari, 1981), RBSOUN of the original source as C.
 * Written by tools/mac2c.py from upstream/RBSOUN.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* TABLES OF OFFSET POINTER FOR SOUNDS. (6 BYTES PER SOUND NUMBER) */
/* 71C4 PNTRS: .BYTE TK1-SOUND */
/* 71C5  .BYTE TK2-SOUND */
/* 71C6  .BYTE 0 */
/* 71C7  .BYTE 0 */
/* 71C8  .BYTE 0 */
/* 71C9  .BYTE 0 */
/* 71CA  .BYTE 0 */
/* 71CB  .BYTE 0 */
/* 71CC  .BYTE 0 */
/* 71CD  .BYTE 0 */
/* 71CE  .BYTE 0 */
/* 71CF  .BYTE 0 */
/* 71D0  .BYTE WP5-SOUND */
/* 71D1  .BYTE WP6-SOUND */
/* 71D2  .BYTE 0 */
/* 71D3  .BYTE 0 */
/* 71D4  .BYTE BN1-SOUND */
/* 71D5  .BYTE BN2-SOUND */
/* 71D6  .BYTE 0 */
/* 71D7  .BYTE 0 */
/* 71D8  .BYTE 0 */
/* 71D9  .BYTE 0 */
/* 71DA  .BYTE 0 */
/* 71DB  .BYTE 0 */
/* 71DC  .BYTE 0 */
/* 71DD  .BYTE 0 */
/* 71DE  .BYTE TH3-SOUND */
/* 71DF  .BYTE TH4-SOUND */
/* 71E0  .BYTE 0 */
/* 71E1  .BYTE 0 */
/* 71E2  .BYTE 0 */
/* 71E3  .BYTE 0 */
/* 71E4  .BYTE TP1-SOUND */
/* 71E5  .BYTE TP2-SOUND */
/* 71E6  .BYTE 0 */
/* 71E7  .BYTE 0 */
/* 71E8  .BYTE 0 */
/* 71E9  .BYTE 0 */
/* 71EA  .BYTE 0 */
/* 71EB  .BYTE 0 */
/* AUDIO CONTROL CHANNEL VALUE (1 BYTE PRE SOUND NUMBER) */
/* SOUND TABLES */
/* ALL OF THESE LABELS GO */
/* HERE SO THEY HAVE AN */
/* OFFSET OF 0, WHICH */
/* PREVENTS THE SOUND FROM */
/* AFFECTING THOSE CHANNELS */
/* WHICH HAVE AN OFFSET */
/* OF 0 */
/* 71EC SOUND: .BYTE 0 */
/* A GOOD PLACE FOR THE CHECKSUM */
/* DATA STRUCTURES: */
/* A CHANNEL CONSISTS OF A SERIES OF 4 BYTE SEQUENCES: */
/* BYTE  FUNCTION */
/* 1   STARTING VALUE OF SEQUENCE */
/* 2   # OF FRAMES BEFORE NEXT CHANGE */
/* 3   AMOUNT OF CHANGE */
/* 4   TOTAL NUMBER OF CHANGES (# OF DIFFERENT VALUES-1) */
/* TO STOP A CHANNEL & RETURN TO ITS IDLE STATE, PUT IN AS A */
/* 2 BYTE SEQUENCE X,0 WHERE X WILL BE USED AS THE NEW IDLE VALUE. */
/* POINT TICK */
/* 71ED TK1: .BYTE 30,7,0,4 */
/* 71F1  .BYTE 0,0 */
/* 71F3 TK2: .BYTE 0A4,7,0FF,4 */
/* 71F7  .BYTE 0,0 */
/* BONUS LIFE */
/* 71F9 BN1: .BYTE 06,1,1,30 */
/* 71FD  .BYTE 06,1,1,30 */
/* 7201  .BYTE 06,1,1,30 */
/* 7205  .BYTE 06,1,1,30 */
/* 7209  .BYTE 06,1,1,30 */
/* 720D  .BYTE 06,1,1,30 */
/* 7211  .BYTE 0,0 */
/* 7213 BN2: .BYTE 0A4,2,0,90 */
/* 7217  .BYTE 0,0 */
/* NEW PLANE SOUND */
/* 7219 WP5: .BYTE 54,2,0FF,3C */
/* 721D  .BYTE 54,2,0FF,3C */
/* 7221  .BYTE 54,2,0FF,3C */
/* 7225  .BYTE 0,0 */
/* 7227 WP6: .BYTE 0A4,2,0,0B4 */
/* 722B  .BYTE 0,0 */
/* THREE HUNDRED POINTS SOUND */
/* 722D TH3: .BYTE 79,2,0,10 */
/* 7231  .BYTE 6C,2,0,10 */
/* 7235  .BYTE 60,2,0,10 */
/* 7239  .BYTE 40,2,0,20 */
/* 723D  .BYTE 60,2,0,10 */
/* 7241  .BYTE 40,2,0,20 */
/* 7245  .BYTE 0,0 */
/* 7247 TH4: .BYTE 0A4,2,0,80 */
/* 724B  .BYTE 0,0 */
/* 10 POINT TICK */
/* 724D TP1: .BYTE 38,0A,0,4 */
/* 7251  .BYTE 0,0 */
/* 7253 TP2: .BYTE 0A4,0A,0FF,4 */
/* 7257  .BYTE 0,0 */
/* ACC=SOUND OFFSET */
/* SNDON (7259) */
void rb_sndon(rb_machine *m)
{
    BIT(RD(RB_GAMODE));                     /* ATTRACT MODE? */
    if (BPL) goto l10;                      /* YES */
    CPY(RB_PTLEN + 7);                      /* PAST TABLE? */
    if (BCS) goto l9001;
    LDX(7);
l9002:
    LDA(RD(AY(RB_PNTRS)));
    if (BEQ) goto l9003;                    /* IF POINTER 0 DONT TOUCH THIS CHANNEL */
    STA(AXW(RB_POINT));                     /* IF NOT SET UP POINTER */
    LDA(1);
    STA(AXW(RB_FRAMES));                    /* DUMMY START, NO SOUND */
    STA(AXW(RB_COUNT));                     /* TILL MODSND STORES TO POKEY */
l9003:
    DEY();
    DEX();
    if (BPL) goto l9002;
l9001:
l10:
    return;
}


/* ---- SOUND ROUTINE ---- */

/* CONTINUES A PREVIOUSLY STARTED SOUND */
/* MODSND (7278) */
void rb_modsnd(rb_machine *m)
{
    LDX(7);                                 /* 4 CHANNELS (FREQUENCY, PURE/POLY & VOLUME) 2 BYTES/ */
l10:
    LDY(RD(AX(RB_POINT)));                  /* GET DATA POINTER */
    if (BEQ) goto l40;                      /* NOT ACTIVE CHANNEL */
    DEC(AXW(RB_FRAMES));                    /* CHANGE THIS FRAME */
    if (BNE) goto l30;
    DEC(AXW(RB_COUNT));                     /* DATA CHANGES LEFT? */
    if (BNE) goto l20;                      /* NO -- NONE */
    LDA(RD(AY(RB_SOUND + RB_STVAL)));       /* GET CURRENT VALUE */
    STA(AXW(RB_CURRENT));                   /* SAVE */
    LDA(RD(AY(RB_SOUND + RB_FRCNT)));       /* GET FRAMES OF THIS LEVEL */
    STA(AXW(RB_FRAMES));                    /* UPDATE FRAME NUMBER */
    if (BNE) goto l17;                      /* IF 0, THIS CHANNEL JUST ENDED */
    STA(AXW(RB_POINT));                     /* SET POINTER TO 0 TO IDLE CHANNEL */
    if (BEQ) goto l30;                      /* JUST IDLE CHANNEL (ALWAYS) */
l17:
    LDA(RD(AY(RB_SOUND + RB_NUMBER)));
    STA(AXW(RB_COUNT));                     /* # OF CHANGES */
    LDA(RD(AX(RB_POINT)));
    CLC();
    ADC(4);                                 /* POINT TO NEXT SEQUENCE */
    STA(AXW(RB_POINT));
    goto l30;
l20:
    LDA(RD(AY(RB_SOUND + RB_FRCNT - 4)));   /* FRAME RATE */
    /* THE -4 TO CORRECT FOR THE POINTER BEING ALREADY */
    /* INCREMENTED TO POINT TO THE NEXT BLOCK. */
    STA(AXW(RB_FRAMES));
    LDA(RD(AX(RB_CURRENT)));
    CLC();
    ADC(RD(AY(RB_SOUND + RB_CHANGE - 4)));  /* STEP THRU SEQUENCE */
    STA(AXW(RB_CURRENT));
l30:
    LDA(RD(AX(RB_CURRENT)));                /* MORE THAN 1 PATH HERE SO LOAD CURRENT */
    STA(AXW(RB_POKEY));                     /* OUTPUT */
l40:
    DEX();
    if (BPL) goto l10;
    return;
}

