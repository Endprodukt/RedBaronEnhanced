/* vgut.c - VGUT: VECTOR GENERATOR UTILITIES
 *
 * Red Baron (Atari, 1981), VGUT of the original source as C.
 * Written by tools/mac2c.py from upstream/VGUT.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"

/* VGRTSL - ADD RTSL TO VECTOR LIST */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* USES A,Y,(VGLIST,VGLIST+1) */
/* VGRTSL (7856) */
void rb_vgrtsl(rb_machine *m)
{
    LDA(0xC0);                              /* DXXX IS RTSL */
    if (BNE) { rb_vghal1(m); return; }      /* ALWASY */
    rb_vghalt(m);                           /* (falls into it) */
}


/* ---- VGHALT - CENTER AND HALT VECTOR GENERATOR ---- */

/* VHALT - CENTER AND HALT VECTOR GENERATOR */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* USES A,X,Y,(VGLIST,VGLIST+1) */
/* VGHALT (785A) */
void rb_vghalt(rb_machine *m)
{
    rb_vgcntr(m);                           /* CENTER FIRST */
    LDA(0x20);                              /* BXXX IS HALT */
    rb_vghal1(m);                           /* (falls into it) */
}

/* VGHAL1 (785F) */
void rb_vghal1(rb_machine *m)
{
    LDY(0);
    STA(IYW(RB_VGLIST));
    rb_vgwai1(m); return;                   /* ADD LAST BYTE */
}


/* ---- VGHEXZ - DISPLAY DIGIT WITH ZERO SUPPRESSION ---- */

/* VGHEXZ - DISPLAY DIGIT WITH ZERO SUPPRESSION */
/* THIS ROUTINE WILL DISPLAY A DIGIT USING THE DEFAULT CHARACTER SIZE. */
/* NO ATTEMPT IS MADE TO USE THE VARIABLE VGSIZE. */
/* ENTRY (A) = LOWER 4 BITS TO BE DISPLAYED */
/* (C) = CARRY CLEAR IF NO ZERO SUPPRESSION */
/* EXIT (C) = CARRY CLEARED IF NON-ZERO DIGIT DISPLAYED */
/* USES A,X,Y,(VGLIST,VGLIST+1) */
/* VGHEXZ (7866) */
void rb_vghexz(rb_machine *m)
{
    if (BCC) goto vghex;                    /* IF NO ZERO SUPPRESSION */
    AND(0xF);
    if (BEQ) goto vghex1;                   /* LEAVE C SET */
    /* ---- VGHEX - DISPLAY DIGIT ---- */
    /* VGHEX - DISPLAY DIGIT */
    /* THIS ROUTINE WILL DISPLAY A DIGIT USING THE DEFAULT CHARACTER SIZE. */
    /* NO ATTEMPT IS MADE TO USE THE VARIABLE VGSIZE. */
    /* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
    /* (A) = LOWER 4 BITS TO BE DISPLAYED */
    /* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
    /* (C) = CARRY IS CLEAR */
    /* USES A,X,Y,(VGLIST,VGLIST+1) */
vghex:
    AND(0xF);
    CLC();
    ADC(1);                                 /* CLEARS C BIT */
vghex1:
    PHP();                                  /* SAVE C FLAG */
    ASLA();
    LDY(0);
    TAX();
    LDA(RD(AX(RB_VGMSGA)));
    STA(IYW(RB_VGLIST));
    LDA(RD(AX(RB_VGMSGA + 1)));             /* COPY JSRL TO CHARACTER ROUTINE */
    INY();
    STA(IYW(RB_VGLIST));
    rb_vgadd(m);                            /* UPDATE VECTOR LIST POINTER */
    PLP();                                  /* RESTORE C FLAG */
    return;
    /* ---- VGJMPL - ADD JMPL TO VECTOR LIST ---- */
    /* VGJMPL - ADD JMPL TO VECTOR LIST */
    /* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
    /* (A) = MSB OF ADDRESS */
    /* (X) = LSB OF ADDRESS */
    /* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
    /* USES A,Y,(VGLIST,VGLIST+1) */
    LSRA();
    AND(0xF);                               /* BASE ADDRESS IS RELATIVE TO ZERO */
    ORA(0xE0);
    rb_vgjmp1(m);                           /* (falls into it) */
}

/* VGJMP1 (788B) */
void rb_vgjmp1(rb_machine *m)
{
    LDY(1);
    STA(IYW(RB_VGLIST));                    /* SAVE MSB + OPCODE */
    DEY();
    TXA();
    RORA();
    STA(IYW(RB_VGLIST));                    /* LSB OF ADDRESS */
    INY();
    if (BNE) { rb_vgadd(m); return; }       /* UPDATE VECTOR POINTER */
    rb_vgjsrl(m);                           /* (falls into it) */
}


/* ---- VGJSRL - ADD JSRL TO VECTOR LIST ---- */

/* VGJSRL - ADD JSRL TO VECTOR LIST */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* (A) = MSB OF ADDRESS */
/* (X) = LSB OF ADDRESS */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* USES A,Y,(VGLIST,VGLIST+1) */
/* VGJSRL (7897) */
void rb_vgjsrl(rb_machine *m)
{
    LSRA();
    AND(0xF);                               /* BASE ADDRESS IS RELATIVE */
    ORA(0xA0);
    if (BNE) { rb_vgjmp1(m); return; }      /* MOVE INTO VECTOR LIST */
    /* ---- VGSTAT - SET VECTOR GENERATOR STATUS ---- */
    /* VGSTAT - SET VECTOR GENERATOR STATUS */
    /* ENTRY (A)=HI/LOW AND IN/OUT FLAGS AND ENABLE (0,1,2,3,OR 4) */
    /* (Y)=VECTOR BRIGHTNESS (0,10,20,...,F0) */
    /* (VGLIST,VGLIST+1)=VECTOR LIST ADDRESS */
    /* EXIT (VGLIST,VGLIST+1)=NEW VECTOR LIST ADDRESS */
    /* USES A,X,Y,(VGLIST,VGLIST+1) */
    LDY(RD(RB_VGBRIT));
    rb_vgstat(m);                           /* (falls into it) */
}

/* VGSTAT (78A0) */
void rb_vgstat(rb_machine *m)
{
    ORA(0x60);
    TAX();                                  /* OPCODE + LIMIT BITS */
    TYA();
    rb_vgadd2(m); return;                   /* ADD 2 BYTES TO VECTOR LIST */
}


/* ---- VGCNTR - CENTER BEAM IN MIDDLE OF SCREEN ---- */

/* VGCNTR - CENTER BEAM IN MIDDLE OF SCREEN */
/* ENTRY (VGLIST,VGLIST+1)=VECTOR LIST ADDRESS */
/* USES A,X,Y,(VGLIST,VGLIST+1) */
/* VGCNTR (78A7) */
void rb_vgcntr(rb_machine *m)
{
    LDA(0x40);                              /* TIMER + SCALE */
    LDX(0x80);                              /* OPCODE */
    rb_vgadd2(m);                           /* (falls into it) */
}

/* JMP VGADD2 */

/* ---- VGADD2 - ADD 2 WORDS TO VECTOR LIST ---- */

/* VGADD2 - ADD 2 WORDS TO VECTOR LIST */
/* ENTRY (A)=FIRST BYTE */
/* (X)=SECOND BYTE */
/* EXIT (VGLIST,VGLIST+1)=NEW VECTOR LIST POINTER */
/* USES A,Y,(VGLIST,VGLIST+1) */
/* VGADD2 (78AB) */
void rb_vgadd2(rb_machine *m)
{
    LDY(0);
    STA(IYW(RB_VGLIST));                    /* LSB BYTE */
    INY();
    TXA();
    STA(IYW(RB_VGLIST));                    /* MSB BYTE */
    rb_vgadd(m);                            /* (falls into it) */
}

/* JMP VGADD  ;UPDATE VECTOR LIST POINTER */

/* ---- VGADD - ADD Y+1 TO VECTOR ADDRESS ---- */

/* VGADD - ADD Y+1 TO VECTOR LIST ADDRESS */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* (Y) = VALUE+1 TO BE ADDED TO VECTOR LIST */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* USES A,(VGLIST,VGLIST+1) */
/* VGADD (78B3) */
void rb_vgadd(rb_machine *m)
{
    TYA();                                  /* ADD 1+(Y) TO VGLIST */
    SEC();
    ADC(RD(RB_VGLIST));
    STA(RB_VGLIST);
    if (BCC) goto l10;
    INC(RB_VGLIST + 1);
l10:
    return;
}


/* ---- VGSCAL-SET VECTOR GENERATOR SCALE ---- */

/* VGSCAL-SET VECTOR GENERATOR SCALE */
/* ENTRY (Y)=LINEAR SCALE FACTOR (0=FULL SIZE, FF=1/256 SIZE) */
/* (A)=POWER OF 2 SCALE FACTOR (0=FULL SIZE,1=1/2 SIZE,...) */
/* (VGLIST,VGLIST+1)=VECTOR LIST ADDRESS */
/* EXIT (VGLIST,VGLIST+1)=NEW VECTOR LIST ADDRESS */
/* USES A,X,Y(VGLIST,VGLIST+1) */
/* VGSCA1 (78BE) */
void rb_vgsca1(rb_machine *m)
{
    LDY(0);                                 /* USE FULL SIZE */
    ORA(0x70);                              /* OPCODE + SCALE SIZE */
    TAX();
    TYA();
    rb_vgadd2(m); return;                   /* ADD 2 BYTES TO VECTOR LIST */
}


/* ---- VGDOT - DRAW A DOT AT THE CURRENT POSITION ---- */

/* VGDOT - DRAW A DOT AT CURRENT POSITION */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* (A) = INTENSITY (0 = OFF, EQ = MAX, 20 = INCREMENT) */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* (VGBRIT) = NEW INTENSITY */
/* USES A,Y,(VGLIST,VGLIST+1) */
/* VGDOT (78C7) */
void rb_vgdot(rb_machine *m)
{
    TAY();
    LDA(0);
    TAX();
    rb_vgvtr(m);                            /* (falls into it) */
}

/* JMP VGVTR */

/* ---- VGVTR - SHORT FORM VGVCTR CALL ---- */

/* VGVTR - SHORT FORM VGVCTR CALL */
/* ENTRY (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* (A) = CHANGE IN X/4 (-80 TO +7F) */
/* (X) = CHANGE IN Y/4 (-80 TO +7F) */
/* (Y) = VECTOR BRIGHTNESS (0,10,20,...,F0) */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* (VGBRIT)=NEW VECTOR BRIGHTNESS */
/* USES A,X,Y,(VGLIST,VGLIST+1),(XCOMP,XCOMP+3) */
/* VGVTR (78CB) */
void rb_vgvtr(rb_machine *m)
{
    STY(RB_VGBRIT);
    rb_vgvtr1(m);                           /* (falls into it) */
}

/* VGVTR1 (78CD) */
void rb_vgvtr1(rb_machine *m)
{
    LDY(0);
    ASLA();
    if (BCC) goto l10;                      /* SIGN EXTEND */
    DEY();                                  /* Y=-1 */
l10:
    STY(RB_XCOMP + 1);                      /* WITH SCALE=0 A=1 MEANS=DOTS ON XY */
    ASLA();
    ROL(RB_XCOMP + 1);
    STA(RB_XCOMP);
    TXA();                                  /* WRITE Y VALUE TO XCOMP+2,XCOMP+3 */
    ASLA();
    LDY(0);
    if (BCC) goto l20;                      /* SIGN EXTEND */
    DEY();                                  /* Y=-1 */
l20:
    STY(RB_XCOMP + 3);
    ASLA();
    ROL(RB_XCOMP + 3);
    STA(RB_XCOMP + 2);
    rb_vgvtr2(m);                           /* (falls into it) */
}

/* VGVTR2 (78E8) */
void rb_vgvtr2(rb_machine *m)
{
    LDX(RB_XCOMP);
    rb_vgvctr(m);                           /* (falls into it) */
}

/* JMP VGVCTR  ;GENERATE VECTOR */

/* ---- VGVCTR - ADD VECTOR TO VECTOR LIST ---- */

/* VGVCTR - ADD VECTOR TO VECTOR LIST */
/* NOTE: IF THE NUMBERS GIVEN ARE MORE THAN */
/* 15 BITS IN SIGNIFICANCE THEN AN */
/* INCORRECT VECTOR WILL BE GENERATED */
/* ENTRY (X) =4 ZERO PAGE LOCN.  CONTAINING (X LSB,X MSB,Y MSB,Y LSB) */
/* (VGLIST,VGLIST+1) = VECTOR LIST ADDRESS */
/* (VGBRIT)=VECTOR BRIGHTNESS (ONLY THE UPPER 3 BITS ARE USED) */
/* EXIT (VGLIST,VGLIST+1) = NEW VECTOR LIST ADDRESS */
/* USES A,X,Y,(VGLIST,VGLIST+1) */
/* VGVCTR (78EA) */
void rb_vgvctr(rb_machine *m)
{
    LDY(0);
    LDA(RD(ZX(2)));                         /* Y LSB */
    STA(IYW(RB_VGLIST));
    LDA(RD(ZX(3)));                         /* Y MSB */
    AND(0x1F);                              /* CLEAR SIGN EXTENSION */
    INY();
    STA(IYW(RB_VGLIST));
    LDA(RD(ZX(0)));                         /* X LSB */
    INY();
    STA(IYW(RB_VGLIST));
    LDA(RD(ZX(1)));                         /* X MSB */
    EOR(RD(RB_VGBRIT));
    AND(0x1F);                              /* CLEAR SIGN EXTENSION */
    EOR(RD(RB_VGBRIT));                     /* COMBINE UPPER 3 BITS OF VGBRIT WITH X MSB */
    rb_vgwai1(m);                           /* (falls into it) */
}

/* VGWAI1 (7904) */
void rb_vgwai1(rb_machine *m)
{
    INY();
    STA(IYW(RB_VGLIST));                    /* SET INTENSITY */
    if (BNE) { rb_vgadd(m); return; }       /* ALWAYS - UPDATE VGLIST POINTER */
}

