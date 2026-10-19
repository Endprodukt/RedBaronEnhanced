/* earom.c - RBROM: EAROM INTERFACE
 *
 * Red Baron (Atari, 1981), RBROM of the original source as C.
 * Written by tools/mac2c.py from upstream/RBROM.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* NOT (C1),C2 FUNCTION */
/* 0 0 READ MODE */
/* 0 1 UNDEFINED */
/* 1 0 WRITE MODE */
/* 1 1 ERASE MODE */
/* EAUPD (7909) */
void rb_eaupd(rb_machine *m)
{
    LDA(RD(RB_EAFLG));                      /* ANY ACTIVITY? */
    if (BEQ) goto l10;                      /* NO. DESELECT EA ROM AND EXIT */
    if (BPL) goto l20;                      /* R/W */
    LDX(RD(RB_EAX));                        /* ERASE.GET ADDRESS */
    LDA(RB_EAC1 + RB_EAC2);                 /* SELECT ERASE MODE */
    STA(RB_EACTL);                          /* DE-SELECT */
    STA(AXW(RB_EADAL));                     /* STORE ADDRESS */
    LDA(RB_EAC1 + RB_EAC2 + RB_EACE);
    DEC(RB_EAX);                            /* NEXT @ */
    DEC(RB_EABC);                           /* COUNT */
    if (BNE) goto l10;                      /* CONTINUE */
    LDX(0x40);                              /* SELECT WRITE MODE */
    STX(RB_EAFLG);
l5:
    INC(RB_EAX);                            /* NEXT ADDRESS */
l10:
    STA(RB_EACTL);                          /* DO EA CONTROL */
    return;                                 /* EXIT */
l20:
    LDY(RD(RB_EABC));                       /* BYTE COUNTER */
    if (BNE) goto l25;                      /* NOT FIRST */
    STY(RB_EACS);                           /* RESET CHECKSUM */
l25:
    ASLA();                                 /* MODE?(CARRY CLEARED) */
    if (BPL) goto l50;                      /* READ */
    CPY(5);                                 /* CHECK COUNT */
    if (BCC) goto l30;                      /* 5 BYTES/CHECKSUM */
    LDA(0xFF);
    STA(RB_EABC);                           /* RESET BYTE COUNT */
    LDA(RD(RB_EASRCE));
    ADC(4);                                 /* ADD 5 (CARRY SET) */
    STA(RB_EASRCE);                         /* NEXT SET OF BYTES */
    LDA(RD(RB_EACS));                       /* CHECKSUM */
    goto l40;
    /* PROTECTION FOR EAROM FIRST BYTE */
l30:
    LDX(RD(RB_EAX));                        /* FIRST BYTE ALWAYS ZERO */
    if (BNE) goto l35;
    TXA();
    if (BEQ) goto l40;                      /* ALWAYS */
l35:
    LDA(RD(IY(RB_EASRCE)));                 /* SOURCE BYTE */
    ADC(RD(RB_EACS));                       /* CARRY CLR */
    STA(RB_EACS);                           /* ADD TO CHECKSUM */
    LDA(RD(IY(RB_EASRCE)));
l40:
    INC(RB_EABC);                           /* BUMP BYTE COUNT */
    LDX(RD(RB_EAX));                        /* @INTO EA ROM TO PUT DATA */
    STA(AXW(RB_EADAL));                     /* SET UP ADDRESS/DATA */
    LDA(RB_EAC1);                           /* SELECT WRITE MODE */
    STA(RB_EACTL);
    LDA(RB_EAC1 + RB_EACE);
l45:
    INX();
    CPX(RD(RB_EACNT));                      /* LAST ONE? */
    if (BCC) goto l5;                       /* NO */
    LDX(0);                                 /* YES. RESET ACTIVITY FLAG */
    STX(RB_EAFLG);                          /* DONE */
    if (BEQ) goto l10;                      /* (ALWAYS) */
l50:
    LDX(RD(RB_EAX));
    LDA(RB_EACE);
    STA(AXW(RB_EADAL));                     /* SELECT @ */
    STA(RB_EACTL);                          /* SELECT READ MODE */
    LDA(RB_EACE + RB_EACK);
    STA(RB_EACTL);
    LDA(RB_EACE);
    STA(RB_EACTL);
    LDX(0);                                 /* NEED A 0 LATER */
    CPY(5);
    if (BCC) goto l65;                      /* 5 BYTES/ENTRY */
    STX(RB_EABC);                           /* RESET BYTE COUNT */
    LDA(RD(RB_EAIN));                       /* READ ROM DATA */
    STX(RB_EACTL);                          /* DE-SELECT CHIP */
    EOR(RD(RB_EACS));                       /* MATCH CHECKSUM? */
    if (BEQ) goto l60;                      /* YES */
    LDA(0);
    LDY(5);
l52:
    STA(IYW(RB_EASRCE));                    /* CLR LOCATIONS */
    DEY();
    if (BPL) goto l52;
    INC(RB_EABAD);                          /* SET BAD FLAG */
    LDA(RD(RB_EASRCE));
    CMP(LO(RB_HSCRES));
    if (BCS) goto l55;
    ADC(5);                                 /* ADD 5 CARRY CLR TO PERCENTAGING FIELDS */
    STA(RB_EASRCE);
l55:
    LDX(RD(RB_EAX));
    goto l45;
l60:
    LDA(RD(RB_EASRCE));
    ADC(4);                                 /* ADD 5 (CARRY SET) */
    STA(RB_EASRCE);
    if (BCC) goto l55;                      /* ALWAYS <256. */
l65:
    LDA(RD(RB_EAIN));                       /* GET ROM DATA */
    STX(RB_EACTL);                          /* DE-SELECT CHIP */
    /* PROTECTION AGAINST FIRST BYTE OF EAROM */
    LDX(RD(RB_EAX));
    if (BNE) goto l70;
    TXA();                                  /* FIRST BYTE ALWAYS ZERO */
l70:
    STA(IYW(RB_EASRCE));                    /* PASS IT */
    ADC(RD(RB_EACS));                       /* ACCUMULATE CHECKSUM */
    STA(RB_EACS);
    INC(RB_EABC);                           /* NEXT BYTE */
    LDA(0);                                 /* DE-SELECT */
    if (BEQ) goto l55;                      /* (ALWAYS) */
    rb_stearom(m);                          /* (falls into it) */
}

/* STEAROM (79D0) */
void rb_stearom(rb_machine *m)
{
    LDY(RD(RB_EAFLG));                      /* ALREADY WORKING? */
    if (BNE) goto l20;                      /* YEP. NEVER MIND */
    SEC();
    ROR(RB_EAFLG);                          /* SET BIT 7. (STARTS SEQUENCE) */
    LDA(RD(RB_EABAD));                      /* ANY BAD ENTRIES? */
    if (BEQ) goto l20;                      /* NO */
    LDA(0);
    STA(RB_EABAD);                          /* RESET BAD FLAG */
l20:
    return;
}

