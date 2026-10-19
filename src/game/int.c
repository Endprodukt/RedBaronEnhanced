/* int.c - RBINT: MESSAGES, DIGITS, POWER ON
 *
 * Red Baron (Atari, 1981), RBINT of the original source as C.
 * Written by tools/mac2c.py from upstream/RBINT.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* ATTRACT BANNER OUTLINE */
/* 79E2 BANNER: .WORD 0&^H1FFF,<0*^H2000>+<40.&^H1FFF> */
/* 79E6  .WORD ^H4000+<3*^H20>+<28./2&^H1F>+<16.*^H80&^H1F00> */
/* 79E8  .WORD ^H4000+<0*^H20>+<-28./2&^H1F>+<-16.*^H80&^H1F00> */
/* 79EA  .WORD ^H4000+<3*^H20>+<28./2&^H1F>+<-16.*^H80&^H1F00> */
/* 79EC  .WORD 32.&^H1FFF,<5*^H2000>+<0&^H1FFF> */
/* 79F0  .WORD 0&^H1FFF,<5*^H2000>+<288.&^H1FFF> */
/* 79F4  .WORD -16.&^H1FFF,<5*^H2000>+<-40.&^H1FFF> */
/* 79F8  .WORD -16.&^H1FFF,<5*^H2000>+<40.&^H1FFF> */
/* 79FC  .WORD 0&^H1FFF,<5*^H2000>+<-288.&^H1FFF> */
/* 7A00  .WORD ^H4000+<0*^H20>+<20./2&^H1F>+<4*^H80&^H1F00> */
/* GENERATE MESSAGE */
/* ENTRY  EXIT */
/* ACC MESSAGE BLITZED */
/* X DC  BLITZED */
/* Y DC  BLITZED */
/* MESSAGE (7A02) */
void rb_message(rb_machine *m)
{
    STA(RB_TEMP1);
    LDA(RD(RB_LANG));
    ASLA();
    TAX();
    LDA(RD(RB_TEMP1));
    ADC(RD(AX(RB_MSGS)));
    STA(RB_POINTR);
    LDA(0);
    TAY();
    ADC(RD(AX(RB_MSGS + 1)));
    STA(RB_POINTR + 1);
    LDA(RD(IY(RB_POINTR)));
    TAX();
    INY();
    LDA(RD(IY(RB_POINTR)));
    STA(RB_POINTR + 1);
    STX(RB_POINTR);
    LDY(1);
    LDA(RD(IY(RB_POINTR)));
    if (BEQ) goto stmess_l10;
    PHA();
    rb_vgcntr(m);                           /* CENTER BEAM */
    LDY(0);
    LDA(RD(IY(RB_POINTR)));
    TAX();
    PLA();
    rb_vgvtr(m);                            /* OFFSET VCTR */
stmess_l10:
    LDA(RD(RB_TEMP1));
    CMP(38);
    if (BCC) goto stmess_l20;
    LDA(2);
    rb_vgsca1(m);
stmess_l20:
    LDY(2);
    /* JMP DSPLAY  ;DISPLAY ALPHA STRING */
    /* ALPHANUMERIC STRING DISPLAY */
    /* ENTRY - POINTR = ALPHA STRING (.ASCVG) */
    /* RAMPTR = ADDR. IN VGRAM FOR LOADING CHAR JSRL'S */
    /* ASSUMES CHARACTER SIZE IS DEFAULT */
    /* ALL CHARACTERS IN STRING ARE DISPLAYED ( ,1-9,A-Z) */
    /* # OF CHARACTER <=255. */
    /* EXIT - RAMPTR = ADDR. NEXT BYTE IN VGRAM */
dsplay:
    LDA(RD(IY(RB_POINTR)));
    PHP();                                  /* SAVE SIGN */
    INY();
    STY(RB_TEMP1 + 1);                      /* SAVE POINTR OFFSET */
    AND(0x7F);
    TAY();
    LDA(RD(AY(RB_VGMSGA)));                 /* INSERT CHAR. JSRL INTO */
    LDX(RD(AY(RB_VGMSGA + 1)));
    rb_vgadd2(m);
    LDY(RD(RB_TEMP1 + 1));
    PLP();                                  /* RESTORE SIGN OFF .ASCVG BYTE */
    if (BPL) goto dsplay;                   /* DO NEXT CHARACTER */
    /* EXIT W/RAMPTR = NEXT BYTE IN VG RAM LIST */
    LDA(RD(RB_TEMP1));
    CMP(38);
    if (BCC) goto dsplay_l10;
    LDA(1);
    rb_vgsca1(m); return;
dsplay_l10:
    return;
    /* MESSAGE TAKES MESSAGE # (IN ACC) AND PUTS IN VG RAM */
    /* X = LABS # (OFFSET IN MSSGLBS) */
    /* CENTER & BEAM POSITION INSERTED IN VG RAM FOLLOWED */
    /* BY MESSAGE STRING (OFFSET IN ACC) */
    /* ALL REGS USED; LODRM4, SCRLD2, & DSPLAY ROUTINES CALLED */
    /* LOADRAM MOVES BYTE FROM (RAMLD) TO */
    /* (RAMPTR), X=# OF BYTES, Y,A = (LO),(HI) ADDR. TO LOAD FROM, AT EXIT */
    /* RAMLD & RAMPTR POINT @ NEXT BYTE */
    LDX(8);                                 /* STANDARD = 8 BYTES */
    rb_loadram(m);                          /* (falls into it) */
}

/* LOADRAM (7A66) */
void rb_loadram(rb_machine *m)
{
    STA(RB_POINTR + 1);                     /* A,Y=HI,LO ADDR. OF SOURCE */
    STY(RB_POINTR);
    TXA();                                  /* ASSUMES RAMLD, RAMPTR ARE CORRECT */
    TAY();
    DEY();
ldram_l10:
    LDA(RD(IY(RB_POINTR)));                 /* MOVE BYTES FROM RAMLD TO RAMPTR */
    STA(IYW(RB_RAMPTR));                    /* # OF BYTES =[X] */
    DEY();
    if (BPL) goto ldram_l10;
    TXA();
    CLC();
    ADC(RD(RB_RAMPTR));                     /* UPDATE RAMPTR */
    STA(RB_RAMPTR);
    if (BCC) goto ldram_l20;
    INC(RB_RAMPTR + 1);
ldram_l20:
    return;                                 /* EXIT RAMPTR=NEXT FREE BYTE */
}


/* ---- DISPLAY DIGITS ---- */

/* DIGITS - DISPLAY 2(Y+1) DIGIT NUMBERS */
/* ENTRY (C) = CARRY SET FOR ZERO SUPPRESSION */
/* (X) = ADDRESS OF (Y) ZERO PAGE LOCATIONS CONTAINING MSB OF NUMBER (LSB TO MSB) */
/* (Y) = NUMBER OF ZERO PAGE LOCATIONS TO USE (0 TO 255). */
/* DIGT1S (7A7F) */
void rb_digt1s(rb_machine *m)
{
    LDY(1);
    rb_digtys(m);                           /* (falls into it) */
}

/* DIGTYS (7A81) */
void rb_digtys(rb_machine *m)
{
    SEC();
    STY(RB_TEMP1 + 1);
digits_l10:
    PHP();
    STX(RB_TEMP1);                          /* SAVE DIGIT ZP OFFSET */
    LDA(RD(ZX(0)));
    LSRA();
    LSRA();
    LSRA();
    LSRA();
    PLP();
    rb_vghexz(m);                           /* FIRST DIGIT */
    LDA(RD(RB_TEMP1 + 1));
    if (BNE) goto digits_l20;
    CLC();                                  /* DISPLAY LAST DIGIT (EVEN 0) */
digits_l20:
    LDX(RD(RB_TEMP1));
    LDA(RD(ZX(0)));
    rb_vghexz(m);                           /* SECOND DIGIT */
    LDX(RD(RB_TEMP1));
    DEX();
    DEC(RB_TEMP1 + 1);
    if (BPL) goto digits_l10;               /* LOOP FOR EACH SET OF DIGITS */
    return;
}


/* ---- POWER ON & SELF TEST ROUTINES ---- */

/* PWRON (7AA5) */
void rb_pwron(rb_machine *m)
{
    LDX(RB_INITSTK);
    /* TXS: the stack is C's */             /* INITIALIZE STACK */
    LDA(0);
    CLD();                                  /* CLEAR DECIMAL MODE */
    STA(RB_POKEY + 0xF);
    TAX();
stest_l10:
    STA(ZX(0));                             /* CLEAR ZERO PAGE */
    STA(AXW(0x100));                        /* CLEAR PAGE 1 */
    STA(AXW(0x200));
    STA(AXW(0x300));
    STA(RB_WATCH);
    INX();
    if (BNE) goto stest_l10;
    STA(RB_VGRST);
    STA(RB_NSERST);
    LDA(7);
    STA(RB_POKEY + 0xF);
    LDX(7);
    LDA(0);
stest_l20:
    STA(AXW(RB_POKEY));
    DEX();
    if (BPL) goto stest_l20;
    STA(RB_POKEY + 8);
    LDA(RD(RB_TESTSW));                     /* D4=NOT (SELF TEST) */
    AND(RB_ST_BIT);
    if (BNE) goto pwrint;                   /* CONTINUE POWER ON */
    rb_not_ported(m, "RAMTST"); return;     /* CONTINUE SELF TEST */
pwrint:
    LDA(0xFF);
    STA(RB_POTMIN);                         /* INITIALIZE POT. MIN.=0FF */
    STA(RB_POTMIN + 1);
    STA(RB_POTFLG);
    rb_start(m); return;                    /* GO TO ATTRACT */
}

/* CONVERTS HEX INTO DECIMAL - 1 BYTE HEX IN ACC */
/* RESULT IN NUMB2,NUMB1 */
/* ROUTINE FOR FAST 8-BIT BINARY TO 3 DIGIT CONVERSION */
/* INPUT: A=MSB'S */
/* OUTPUT: A=LSD'S, X=MSD'S */
/* 774. CYCLES WORST (AND BEST) CASE FOR 16. BITS */
/* DECIMAL (7AED) */
void rb_decimal(rb_machine *m)
{
    STA(RB_TEMP1);                          /* TEMP1+1,TEMP1 = BINARY WORD */
    LDY(7);                                 /* BIT COUNT */
    LDA(0);
    STA(RB_NUMB1);                          /* CLR DECIMAL BYTES */
    STA(RB_NUMB1 + 1);
    SED();                                  /* SET DECIMAL MODE */
l10:
    ASL(RB_TEMP1);
    LDA(RD(RB_NUMB1));
    ADC(RD(RB_NUMB1));
    STA(RB_NUMB1);                          /* AND ADD INTO DECIMAL BYTES */
    LDA(RD(RB_NUMB1 + 1));
    ADC(RD(RB_NUMB1 + 1));                  /* TOP BYTE */
    STA(RB_NUMB1 + 1);
    DEY();
    if (BPL) goto l10;
    LDX(RD(RB_NUMB1 + 1));
    LDA(RD(RB_NUMB1));
    CLD();                                  /* CLEAR DECIMAL FLAG */
    return;
}


/* ---- ZERO-PAGE TEST ---- */

