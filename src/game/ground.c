/* ground.c - GROUND SEQUENCE
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- GROUND SEQUENCE ---- */

/* GROUND SEQUENCE */
/* GRDISP (5CC2) */
void rb_grdisp(rb_machine *m)
{
    LDA(RD(AX(RB_PFOBJ + 8)));
    ORA(RD(AX(RB_PFOBJ + 9)));
    ORA(RD(AX(RB_PFOBJ + 0xA)));
    AND(0xC0);
    if (BNE) goto l5;
    return;
l5:
    STA(RB_FLAG + 1);                       /* SET (<>0) */
    rb_scene1(m);                           /* POSITION BEAM (X SAVED IN SAVX+1) */
    LDY(2);
l10:
    STY(RB_SAVY + 2);
    TYA();
    CLC();
    ADC(RD(RB_SAVX + 1));
    STA(RB_SAVX + 2);
    TAX();
    LDA(RD(AX(RB_PFOBJ + 8)));
    if (BMI) goto l15;                      /* OBJECT PRESENT */
    goto l60;                               /* NO OBJECT */
l15:
    LDA(RD(RB_OBJECT + 4));
    STA(RB_DD_DML);
    LDA(RD(RB_OBJECT + 5));
    STA(RB_DD_DMM);
    LDX(RD(RB_SAVX + 1));
    LDA(RD(RB_SAVY + 2));
    ASLA();
    ASLA();
    STA(RB_TEMP2 + 1);                      /* GROUND OBJECT # * 4 */
    LDA(RD(AX(RB_PFOBJ + 6)));
    AND(0xF);
    STA(RB_TEMP1);
    ASLA();
    ADC(RD(RB_TEMP1));                      /* CARRY CLR */
    ASLA();
    STA(RB_TEMP3);
    ADC(RD(RB_TEMP2 + 1));                  /* CARRY CLR */
    STA(RB_TEMP2 + 1);
    TAY();
    LDX(RD(AY(RB_PFOFFS + 1)));
    LDA(RD(AY(RB_PFOFFS)));
    TAY();
    rb_ddivit(m);
    STX(RB_MM_EM);
    STY(RB_MM_EL);
    LDY(RD(RB_TEMP2 + 1));
    LDX(RD(AY(RB_PFOFFS + 3)));
    LDA(RD(AY(RB_PFOFFS + 2)));
    TAY();
    rb_ddivit(m);
    STX(RB_MM_FM);
    STY(RB_MM_FL);
    LDA(HI(RB_PFPNT0));
    LDX(LO(RB_PFPNT0));
    LDY(0);
    STY(RB_VGBRIT);                         /* NO INTENSITY */
    rb_project(m);                          /* PROJECT CENTER (0,0) */
    LDA(RD(RB_SAVY + 2));
    CLC();
    ADC(RD(RB_TEMP3 + 8));
    TAY();
    LDA(RD(AY(RB_PFOBJN)));                 /* GET OBJECT # */
    STA(RB_TEMP2);                          /* TEMP2=OBJECT # */
    TAX();
    LDA(RD(RB_V_BRIT));
    CPX(4);
    if (BCC) goto l24;
    ADC(0x1F);                              /* ADD 20 */
l24:
    TAY();
    LDA(4);
    rb_vgstat(m);                           /* SET INTENSITY */
    LDY(4);
    rb_pfxpos(m);                           /* DETERMINE POSITION */
    if (BCS) goto l26;
    rb_vgcntr(m);                           /* CENTER BEAM */
    rb_vgvtr2(m);                           /* POSITION BEAM */
    CLC();
    DEC(RB_FLAG);                           /* VISIBLE OBJECT */
l26:
    LDX(RD(RB_SAVX + 2));
    LDA(RD(AX(RB_PFOBJ + 8)));
    AND(0x40);
    if (BNE) goto l40;                      /* EXPLOSION */
    if (BCS) goto l27;                      /* NOT ON SCREEN */
    LDY(RD(RB_TEMP2));
    LDX(RD(AY(RB_PFLOB)));                  /* OBJECT POINT LIST */
    LDA(RD(AY(RB_PFLOB + 1)));
    PHA();
    TYA();
    LSRA();
    TAY();
    LDA(RD(AY(RB__PFLOB)));
    TAY();
    PLA();
    rb_project(m);                          /* PROJECT OBJECT POINTS */
    LDY(RD(RB_TEMP2));
    LDA(RD(AY(RB_PFODEC + 1)));
    LDX(RD(AY(RB_PFODEC)));
    RB_NOTED(RB_OBJ_GROUND, rb_decmap(m));  /* DECODE POINTS */
    rb_grcoln(m);                           /* CHECK FOR SHELL TO GROUND OBJ COLLISION */
l27:
    LDA(RD(RB_TEMP2));
    CMP(4);
    if (BCC) goto l35;                      /* PASSIVE GROUND OBJECTS */
    LDA(RD(RB_SAVY + 2));
    ASLA();
    ASLA();
    ADC(RD(RB_TEMP3));                      /* CARRY CLR */
    TAY();
    LDA(RD(AY(RB_PFOBLN)));
    ADC(RD(RB_UNIV4X));                     /* CARRY CLR */
    STA(RB_XCOMP);
    LDA(RD(AY(RB_PFOBLN + 1)));
    ADC(RD(RB_UNIV4X + 1));
    STA(RB_XCOMP + 1);
    LDA(RD(AY(RB_PFOBLN + 2)));
    STA(RB_XCOMP + 2);
    LDA(RD(AY(RB_PFOBLN + 3)));
    STA(RB_XCOMP + 3);
    rb_shlaun(m);                           /* LAUNCH SHELL AT PLAYER */
l35:
    goto l60;
l40:
    if (BCS) goto l35;                      /* NOT ON SCREEN */
    LDX(RD(RB_SAVX + 2));
    LDA(RD(AX(RB_PFOBJ + 8)));
    AND(0xF);
    STA(RB_TEMP1);
    TAY();
    LDA(RD(AY(RB_PFEXSZ)));                 /* PF EXPL SCALE SIZE */
    if (BMI) goto l45;
    rb_vgsca1(m);
    LDA(RB_EXCRCL / 0x100);
    LDX(RB_EXCRCL & 0xFF);
    RB_TAGGED(RB_TAG_EXPLOSION, rb_vgjsrl(m));
l45:
    LDY(RD(RB_TEMP1));
    LDA(RD(AY(RB_PFEXSZ + 6)));             /* PF EXPL SCALE SIZE */
    if (BMI) goto l50;
    rb_vgsca1(m);
    LDA(RB_EXCRCL / 0x100);
    LDX(RB_EXCRCL & 0xFF);
    RB_TAGGED(RB_TAG_EXPLOSION, rb_vgjsrl(m));
l50:
    LDA(1);
    rb_vgsca1(m);                           /* RESTORE PROPER SCALE */
    LDX(RD(RB_SAVX + 2));
    INC(AXW(RB_PFOBJ + 8));
    LDA(RD(AX(RB_PFOBJ + 8)));
    CMP(0xC6);
    if (BCC) goto l60;
    LDA(0);
    STA(AXW(RB_PFOBJ + 8));
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PFOBJ + 8)));
    ORA(RD(AX(RB_PFOBJ + 9)));
    ORA(RD(AX(RB_PFOBJ + 0xA)));
    AND(0xC0);
    if (BNE) goto l60;
    rb_pfsclr(m);
l60:
    LDY(RD(RB_SAVY + 2));
    DEY();
    if (BMI) goto l65;
    goto l10;
l65:
    return;
}

/* GROUND SHELL MOTION ROUTINE ( INCLUDE DISPLAY & PLAYER CD ) */
/* ENTRY   EXIT */
/* ACC DC   DC */
/* X DC   DC */
/* Y DC   DC */
/* USES - GRSHLS,OBJECT,SAVX+1,POINTR(+1),GREND,XPOS,BULPOS,TEMP1(+1) */
/* CALLS - POSITP,SETGRS,DSPGRS */
/* GRSMTN (5E18) */
void rb_grsmtn(rb_machine *m)
{
    LDX(RB_NGRSHL);                         /* GROUND SHELLS */
l5:
    STX(RB_SAVX + 1);
    LDA(RD(AX(RB_GRSHLS + 5)));             /* SHELL IN FLIGHT? */
    if (BMI) goto l10;
    goto l55;                               /* NO */
l10:
    LDY(0);
l15:
    LDA(RD(AX(RB_GRSHLS)));
    STA(AYW(RB_OBJECT));
    INX();
    INY();
    CPY(6);
    if (BCC) goto l15;
    AND(0x3F);                              /* ACC = OBJECT+5 */
    STA(RB_OBJECT + 5);                     /* D7=STATUS BIT */
    rb_positp(m);                           /* ADD HORIZN ADJ */
    rb_setgrs(m);
    if (BCC) goto l20;
    LDY(RD(RB_OBJECT + 4));
    LDA(RD(RB_OBJECT + 5));
    CPY(1);
    SBC(3);
    if (BCS) goto l40;
    LDX(RD(RB_SAVX + 1));
    goto l52;
l20:
    rb_dspgrs(m);
    LDA(HI(RB_H_MAP));
    STA(RB_POINTR + 1);
    LDX(LO(RB_H_MAP));
    STX(RB_POINTR);
    LDA(RD(RB_OBJECT + 4));
    CMP(0);
    LDA(RD(RB_OBJECT + 5));
    SBC(1);
    if (BCS) goto l40;
    BIT(RD(RB_GREND));
    if (BMI) goto l40;                      /* EOL */
    LDA(0x80);
    STA(RB_GREND);                          /* SHELL CD */
    LDX(3);
l30:
    LDA(RD(ZX(RB_XPOS)));
    STA(AXW(RB_BULPOS));
    DEX();
    if (BPL) goto l30;
l40:
    LDX(RD(RB_SAVX + 1));                   /* SHELL OFFSET */
    LDA(RD(AX(RB_GRSHLS + 5)));             /* Z POS MSB */
    STA(RB_TEMP1);
    AND(0x3F);
    STA(RB_TEMP1 + 1);
    if (BEQ) goto l52;
    CLC();
    LDA(RD(AX(RB_GRSHLS + 0xB)));           /* STEP X POSITION */
    ADC(RD(AX(RB_GRSHLS + 6)));
    STA(AXW(RB_GRSHLS + 0xB));              /* FRACTIONAL */
    LDA(RD(AX(RB_GRSHLS + 7)));
    ADC(RD(AX(RB_GRSHLS)));
    STA(AXW(RB_GRSHLS));
    LDA(RD(AX(RB_GRSHLS + 0xA)));
    ADC(RD(AX(RB_GRSHLS + 1)));             /* MSB OF POSITION */
    STA(AXW(RB_GRSHLS + 1));
    CLC();
    LDA(RD(AX(RB_GRSHLS + 0xD)));           /* STEP Y POSITION */
    ADC(RD(AX(RB_GRSHLS + 8)));
    STA(AXW(RB_GRSHLS + 0xD));
    LDA(RD(AX(RB_GRSHLS + 9)));
    ADC(RD(AX(RB_GRSHLS + 2)));
    STA(AXW(RB_GRSHLS + 2));
    LDA(RD(AX(RB_GRSHLS + 0xC)));
    ADC(RD(AX(RB_GRSHLS + 3)));             /* MSB Y POSITION */
    STA(AXW(RB_GRSHLS + 3));
    LDA(-1);
    BIT(RD(RB_TEMP1));
    if (BVC) goto l45;
    ASLA();
l45:
    CLC();
    ADC(RD(RB_TEMP1 + 1));
    STA(RB_TEMP1 + 1);
    PHP();
    LDA(RD(AX(RB_GRSHLS + 5)));
    AND(0xC0);
    ORA(RD(RB_TEMP1 + 1));
    STA(AXW(RB_GRSHLS + 5));
    PLP();
    if (BMI) goto l50;
    if (BNE) goto l55;
l50:
    LDA(0x80);
    STA(AXW(RB_GRSHLS + 5));
    LDA(0xE0);
    STA(AXW(RB_GRSHLS + 4));
    if (BNE) goto l55;                      /* ALWAYS */
l52:
    LDA(0);
    STA(AXW(RB_GRSHLS + 5));                /* CLR SHELL */
l55:
    TXA();
    SEC();
    SBC(RB_GRLSHL);
    TAX();
    if (BCC) goto l60;
    goto l5;                                /* DO ALL SHELLS */
l60:
    return;
}

/* 5EF1 PFEXSZ: .BYTE 5,4,3,2,-1,-1 */
/* 5EF7  .BYTE -1,-1,5,4,3,2 */
/* GRCOLN (5EFD) */
void rb_grcoln(rb_machine *m)
{
    LDY(RB_N_SHL);
l1:
    STY(RB_SAVY);                           /* SHELL OFFSET */
    LDA(RD(AY(RB_SHELLS + 7)));
    if (BPL) goto l4;                       /* SHELL NOT ACTIVE */
    ASLA();
    if (BMI) goto l4;                       /* ENEMY SHELL */
    SEC();
    LDX(RD(AY(RB_SHELLS + 5)));             /* SHELL DEPTH */
    LDA(RD(AY(RB_SHELLS + 4)));
    SBC(RD(RB_OBJECT + 4));                 /* DIFF WITH OBJECT DEPTH */
    TAY();
    TXA();
    SBC(RD(RB_OBJECT + 5));
    TAX();
    rb_dpabs(m);                            /* ABSOLUTE OF DEPTH DIFF */
    CPY(0x40);
    SBC(2);
    LDY(RD(RB_SAVY));
    if (BCC) goto l5;
l4:
    goto l40;                               /* NO COLLISION (Z AXIS) */
l5:
    SEC();
    LDA(RD(AY(RB_SHELLS)));                 /* SHELL X POS */
    SBC(RD(RB_UNIV4X));                     /* - UNIVERSE X POS */
    STA(RB_TEMP1);
    LDA(RD(AY(RB_SHELLS + 1)));             /* MSB X POS */
    SBC(RD(RB_UNIV4X + 1));
    TAX();
    LDA(RD(RB_TEMP1));                      /* ACTUAL X POS */
    SEC();
    SBC(RD(RB_OBJECT));                     /* - OBJECT CENTER X POS */
    STA(RB_XCOMP);                          /* XCOMP = SHELL X - OBJECT X */
    TXA();                                  /* MSB X POS */
    SBC(RD(RB_OBJECT + 1));
    STA(RB_XCOMP + 1);                      /* MSB */
    SEC();
    LDA(RD(AY(RB_SHELLS + 2)));
    SBC(RD(RB_OBJECT + 2));
    STA(RB_YCOMP);                          /* YCOMP=OBJECT-SHELL(YPOS) */
    LDA(RD(AY(RB_SHELLS + 3)));
    SBC(RD(RB_OBJECT + 3));
    STA(RB_YCOMP + 1);
    LDA(RD(RB_SAVY + 2));
    ASLA();
    ASLA();
    ASLA();
    STA(RB_TEMP1);
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PFOBJ + 6)));
    AND(0xF);
    STA(RB_TEMP1 + 1);
    ASLA();
    ADC(RD(RB_TEMP1 + 1));                  /* CARRY CLR */
    ASLA();
    ASLA();
    ADC(RD(RB_TEMP1));                      /* CARRY CLR */
    TAY();
    INY();
    INY();
    LDX(2);                                 /* START WITH Y */
l10:
    LDA(RD(ZX(RB_XCOMP)));
    CMP(RD(AY(RB_PFOCOL)));
    LDA(RD(ZX(RB_XCOMP + 1)));
    SBC(RD(AY(RB_PFOCOL + 1)));
    if (BMI) goto l40;                      /* LESS THAN MIN */
    LDA(RD(AY(RB_PFOCOL + 4)));
    CMP(RD(ZX(RB_XCOMP)));
    LDA(RD(AY(RB_PFOCOL + 5)));
    SBC(RD(ZX(RB_XCOMP + 1)));
    if (BMI) goto l40;                      /* MORE THAN MAX */
    DEY();
    DEY();
    DEX();
    DEX();
    if (BPL) goto l10;                      /* REPEAT FOR X */
    LDA(RD(RB_TEMP2));                      /* POINTS BY OBJECT TYPE */
    LSRA();
    TAY();
    LDA(RD(AY(RB_PFSCRE)));                 /* PF OBJECT SCORES */
    rb_nwscre(m);                           /* GIVE SCORE */
    rb_lvlstp(m);
    LDA(0xF0);
    STA(RB_EXPVAL);
    LDA(7);
    STA(RB_EXCNTR);
    LDY(RD(RB_SAVY));
    LDA(0);
    STA(AYW(RB_SHELLS + 7));                /* CLEAR SHELL */
    LDX(RD(RB_SAVX + 2));
    LDA(RD(AX(RB_PFOBJ + 8)));              /* EXPLOSION */
    ORA(0x40);
    STA(AXW(RB_PFOBJ + 8));
l40:
    LDA(RD(RB_SAVY));                       /* SHELL OFFSET */
    SEC();
    SBC(RB_L_SHL);
    TAY();
    if (BCC) goto l45;
    goto l1;
l45:
    return;
}

/* 5FBA PFOBJN: .BYTE 0,2,6 */
/* 5FBD  .BYTE 0,0,6 */
/* 5FC0  .BYTE 4,2,6 */
/* 5FC3  .BYTE 4,4,6 */
/* 5FC6 PFOBLN: .WORD -96.*8,27.*8 */
/* 5FCA  .WORD 56.*8,3*8 */
/* 5FCE  .WORD 72.*8,3*8 */
/* 5FD2  .WORD -120.*8,3*8 */
/* 5FD6  .WORD 24.*8,19.*8 */
/* 5FDA  .WORD 40.*8,19.*8 */
/* 5FDE  .WORD -8*8,3*8 */
/* 5FE2  .WORD 8*8,3*8 */
/* 5FE6  .WORD 40.*8,19.*8 */
/* 5FEA  .WORD -104.*8,19.*8 */
/* 5FEE  .WORD 72.*8,3*8 */
/* 5FF2  .WORD 88.*8,3*8 */
/* 5FF6 PFOBJL: .WORD 0*8,8*8 */
/* 5FFA PFSCRE: .BYTE 10.,10.,25.,25.   ;PF OBJECT SCORE VALUES */
/* STANDARD PLAYER COLLISION DETECT */
/* XPOS,+1,YPOS,+1=CENTER OF OBJECT (SCREEN POSITION) */
/* POINTR,+1=TABLE OF DECODE POINT OFFSETS (FOR DECMAP) */
/* CS=COLLISION */
/* PLYCOL (5FFE) */
void rb_plycol(rb_machine *m)
{
    LDY(0);
l10:
    LDA(RD(IY(RB_POINTR)));
    STY(RB_SAVY);
    CMP(0xFF);
    if (BEQ) goto l30;                      /* NO MORE POINTS */
    AND(0xFE);
    TAX();
    STX(RB_SAVX);                           /* POINT OFFSET */
    rb_pcolx(m);                            /* CHECK X CD */
    if (BCC) goto l12;                      /* X W/I WINDOW */
    LDA(RD(RB_TEMP1));
    STA(RB_TEMP1 + 1);
    LDX(0xF0);
    rb_pcolx(m);                            /* CHECK PREV X */
    if (BCC) goto l12;
    LDA(RD(RB_TEMP1));
    EOR(RD(RB_TEMP1 + 1));
    if (BPL) goto l20;                      /* BOTH X SAME SIGN */
l12:
    LDX(RD(RB_SAVX));
    rb_pcolny(m);                           /* NORMALIZE Y */
    if (BPL) goto l15;
    LDX(0xF0);
    rb_pcolny(m);                           /* NORMALIZE PREV Y */
    if (BMI) goto l20;                      /* NO Y CD */
l15:
    CLC();                                  /* GROUND COLLISION */
    return;
l20:
    LDA(RD(RB_SAVX));
    CLC();
    ADC(3);
    TAX();
    LDY(3);
l25:
    LDA(RD(AX(RB_DB_TRP + 2)));
    STA(AYW(RB_DB_TRP + 0xF2));             /* PREV=CURRENT */
    DEX();
    DEY();
    if (BPL) goto l25;
    LDY(RD(RB_SAVY));
    INY();
    if (BPL) goto l10;                      /* ALWAYS */
l30:
    SEC();
    return;
}

/* PLAYER COLLISION DETECT X AXIS CHECK */
/* PCOLX (604C) */
void rb_pcolx(rb_machine *m)
{
    CLC();
    LDA(RD(AX(RB_DB_TRP + 2)));             /* X LSB */
    ADC(RD(RB_XPOS));
    TAY();
    LDA(RD(AX(RB_DB_TRP + 3)));             /* X MSB */
    ADC(RD(RB_XPOS + 1));
    TAX();
    STA(RB_TEMP1);
    rb_dpabs(m);                            /* ABSOLUTE */
    CPY(RB_PCDX & 0xFF);
    SBC(RB_PCDX / 0x100);
    return;
}

/* NORMALIZE Y VALUE TO SCREEN POSITION (POSITIVE) */
/* PCOLNY (6063) */
void rb_pcolny(rb_machine *m)
{
    CLC();                                  /* X=OFFSET */
    LDA(RD(AX(RB_DB_TRP + 4)));
    ADC(RD(RB_YPOS));                       /* SCREEN POSITION CENTER OF OBJECT */
    TAY();
    LDA(RD(AX(RB_DB_TRP + 5)));
    ADC(RD(RB_YPOS + 1));
    CPY(RB_PCDY & 0xFF);
    SBC(RB_PCDY / 0x100);
    return;
}

/* MODIFIES-GRSHLS,SAVX,TEMP1.+1,TEMP2 */
/* OBJECT..+5 POSITION OF OBJECT */
/* XCOMP,+1;YCOMP,+1 OFFSET FOR GUN BARREL */
/* XPOS,+1 SCREEN X POSITION OF OBJECT */
/* SHLAUN (6074) */
void rb_shlaun(rb_machine *m)
{
    LDA(RD(RB_FRAME));
    AND(3);                                 /* 1 OUT OF 4 FRAMES */
    STA(RB_SHMODE);
    if (BEQ) { rb_shlau0(m); return; }
    rb_shlaux(m);                           /* (falls into it) */
}

/* SHLAUX (607C) */
void rb_shlaux(rb_machine *m)
{
    return;
    (void)m;
}

/* SHLAU0 (607D) */
void rb_shlau0(rb_machine *m)
{
    LDY(RD(RB_XPOS));                       /* X POSITION LSB */
    LDX(RD(RB_XPOS + 1));                   /* MSB */
    rb_dpabs(m);
    CPY(1);
    SBC(3);
    if (BCS) { rb_shlaux(m); return; }      /* X OFF SCREEN */
    LDX(RD(RB_GMLEVL));
    DEX();
    DEX();
    if (BMI) { rb_shlaux(m); return; }      /* NO GROUND SHELLS @ LOWER LEVELS */
    LDY(RD(AX(RB_GRSLVL)));                 /* GET NUMBER OF SHELLS (OFFSET) */
shlau1:
    LDA(RD(AY(RB_GRSHLS + 5)));
    if (BPL) goto shlau1_l10;
    goto shlau2;                            /* IN USE */
shlau1_l10:
    LDA(0x10);                              /* 16 BIT DIVIDE */
    STA(RB_DD_NL);
    LDA(0);
    STA(RB_TEMP2);
    STA(AYW(RB_GRSHLS + 0xB));              /* FRACTIONAL POSITION */
    STA(AYW(RB_GRSHLS + 0xD));
    LDA(RD(RB_OBJECT + 4));
    STA(AYW(RB_GRSHLS + 4));
    LDA(RD(RB_SHMODE));
    LSRA();
    AND(0x40);
    ORA(RD(RB_OBJECT + 5));
    ORA(0x80);                              /* D7 = 1 SHELL IN FLIGHT */
    STA(AYW(RB_GRSHLS + 5));
    LDX(RD(RB_OBJECT + 5));
    INX();
    LDA(0xFF);
    BIT(RD(RB_SHMODE));
    if (BPL) goto shlau1_l15;
    LDA(0xFE);
shlau1_l15:
    STA(RB_TEMP1);
    TXA();
    AND(RD(RB_TEMP1));
    BIT(RD(RB_SHMODE));
    if (BPL) goto shlau1_l40;
    rb_divby2(m);
shlau1_l40:
    STA(RB_DD_DMM);
    LDA(RD(RB_TEMP2));
    STA(RB_DD_DML);
    BIT(RD(RB_SHMODE));                     /* SHELL MODE */
    if (BPL) goto shlau1_l45;
    ASL(RB_PLYRDL);
    ROL(RB_PLYRDL + 1);
    ASL(RB_PLYRDL + 2);
    ROL(RB_PLYRDL + 3);
shlau1_l45:
    INY();
    INY();                                  /* POINT @ Y VALUES */
    LDX(2);
shlau1_l50:
    STX(RB_SAVX);                           /* UNIVERSE VALUE OFFSET */
    CLC();
    LDA(RD(ZX(RB_OBJECT)));                 /* LAUNCHING VALUE OFFSET */
    ADC(RD(ZX(RB_XCOMP)));                  /* GUN OFFSET IN OBJECT */
    STA(AYW(RB_GRSHLS));                    /* SHELL POSITION */
    STA(RB_TEMP1);
    LDA(RD(ZX(RB_OBJECT + 1)));             /* MSB LAUNCH */
    ADC(RD(ZX(RB_XCOMP + 1)));
    STA(AYW(RB_GRSHLS + 1));                /* MSB SHELL POSITION */
    STA(RB_TEMP1 + 1);
    SEC();
    LDA(RD(ZX(RB_UNIV4X)));                 /* ESTABLISH SHELL DELTA TO PLAYER */
    SBC(RD(RB_TEMP1));
    STA(RB_D_NMLH);
    LDA(RD(ZX(RB_UNIV4X + 1)));
    LDX(0);
    SBC(RD(RB_TEMP1 + 1));                  /* MSB OF DELTA */
    rb_grsran(m);                           /* DIVIDE BY DEPTH OF SHELL ( + RANDOM OFFSET) */
    DEY();
    DEY();
    LDX(RD(RB_SAVX));
    DEX();
    DEX();
    if (BPL) goto shlau1_l50;
    rb_setdiv(m); return;                   /* RESET DIVIDE REGISTERS (DEFAULT) */
shlau2:
    TYA();
    SEC();
    SBC(RB_GRLSHL);
    TAY();
    if (BCC) goto shlau3;
    goto shlau1;
shlau3:
    return;                                 /* NO SHELLS AVAILABLE */
}

/* GRSRAN (612A) */
void rb_grsran(rb_machine *m)
{
    if (BPL) goto l5;
    DEX();                                  /* X = 0FF */
l5:
    STA(RB_D_NMHL);
    STX(RB_D_NMHH);                         /* MSB */
    STA(RB_D_SZXD);                         /* START DIVIDE */
    MATSTT();
    LDA(RD(RB_YLOW));                       /* MIDDLE */
    STA(RB_TEMP1);
    LDA(RD(RB_YHIGH));                      /* MSB */
    STA(RB_TEMP1 + 1);
    STA(RB_D_DYPL);                         /* DISPLAY LOW QUOTIENT */
    LDA(RD(RB_YHIGH));                      /* LSB */
    STA(RB_TEMP2);
    TYA();
    LSRA();
    LSRA();
    if (BCS) goto l30;                      /* ODD GROUND SHELLS */
    rb_random(m);
    CLC();
    ADC(RD(RB_TEMP2));                      /* TO DELTA (LSB) */
    STA(AYW(RB_GRSHLS + 6));                /* DELTA LSB */
    LDX(0);
    PHP();                                  /* SAVE CARRY */
    LDA(RD(RB_POLYH));
    AND(0x1F);
    CMP(0x10);
    if (BCC) goto l20;
    DEX();
    ORA(0xF0);
l20:
    PLP();                                  /* RESET CARRY */
    ADC(RD(RB_TEMP1));
    STA(AYW(RB_GRSHLS + 7));                /* DELTA MIDDLE */
    TXA();
    ADC(RD(RB_TEMP1 + 1));
    STA(AYW(RB_GRSHLS + 0xA));              /* DELTA MSB */
    return;
l30:
    LDA(RD(RB_TEMP2));
    STA(AYW(RB_GRSHLS + 6));                /* LSB */
    LDX(RD(RB_SAVX));
    LDA(RD(AX(RB_PLYRDL)));
    CLC();
    ADC(RD(RB_TEMP1));                      /* COMPENSATE FOR PLAYER'S MOTION */
    STA(AYW(RB_GRSHLS + 7));                /* MIDDLE */
    LDA(RD(AX(RB_PLYRDL + 1)));
    ADC(RD(RB_TEMP1 + 1));
    STA(AYW(RB_GRSHLS + 0xA));              /* MSB */
    return;
}

/* 618F GRSLVL: .BYTE 0,GRLSHL */
/* 6191  .BYTE GRLSHL*3,NGRSHL */
/* BLIMP MOTION */
/* BLMOTN (6193) */
void rb_blmotn(rb_machine *m)
{
    LDA(RD(RB_BLOBJ + 0x18));               /* STATUS */
    if (BMI) goto l5;
l2:
    goto l45;                               /* NOT IN FLIGHT */
l5:
    rb_pftrig(m);
    LDX(5);
l10:
    LDA(RD(ZX(RB_BLOBJ)));                  /* SET OBJECT PARAMS */
    STA(ZX(RB_OBJECT));
    DEX();
    if (BPL) goto l10;
    rb_positp(m);                           /* CALCULATE POSITION */
    rb_setbm(m);                            /* SET BEAM */
    LDX(3);
l15:
    LDA(RD(ZX(RB_XPOS)));
    STA(ZX(RB_BLOBJ + 8));                  /* SAVE POSITION */
    DEX();
    if (BPL) goto l15;
    if (BCS) goto l2;                       /* NOT VISIBLE */
    BIT(RD(RB_BLOBJ + 6));
    if (BVC) goto l30;
    LDX(RD(RB_BLOBJ + 7));
    LDY(RB__BLEX);
    rb_blexpl(m);
    CLC();
    LDA(RD(RB_BLOBJ + 7));
    ADC(3);
    STA(RB_BLOBJ + 7);
    CMP(0x20);
    if (BCC) goto l25;
    goto l40;
l25:
    return;
l30:
    LDY(0xC0);
    LDA(4);
    rb_vgstat(m);
    LDX(7);                                 /* BLOBJ OBJECT */
    LDY(RB__BLMP);
    rb_plnabz(m);                           /* ROTATE ABOUT Z */
    LDY(RD(RB_BLOBJ + 0x14));
    TYA();
    ORA(RD(RB_BLOBJ + 0x15));
    if (BEQ) goto l32;                      /* NO ROTATION */
    LDA(RD(RB_BLOBJ + 0x15));
    rb_trig(m);                             /* ANGLE ABOUT Y */
    LDX(RB__BLMP * 2);
    rb_yaxis(m);                            /* ROTATE ABOUT Y */
l32:
    LDA(RD(RB_DB_TRP + (35 * 6 + 3)));      /* BLIMP GUN POINT */
    STA(RB_TEMP1);
    LDA(RD(RB_DB_TRP + (35 * 6 + 2)));
    rb_multb4(m);
    STA(RB_XCOMP);
    LDA(RD(RB_TEMP1));
    STA(RB_XCOMP + 1);
    LDA(RD(RB_DB_TRP + (35 * 6 + 5)));
    STA(RB_TEMP1);
    LDA(RD(RB_DB_TRP + (35 * 6 + 4)));
    rb_multb4(m);
    STA(RB_YCOMP);
    LDA(RD(RB_TEMP1));
    STA(RB_YCOMP + 1);
    rb_shlaun(m);                           /* LAUNCH SHELL @ PLAYER */
    LDX(RB__BLMP * 2);
    rb_d_loop(m);                           /* SCALE BY DEPTH */
    LDA(HI(RB_DBLIMP));
    LDX(LO(RB_DBLIMP));
    rb_dcmpa0(m);                           /* DECODE POINTS */
    CLC();
    LDA(RD(RB_BLOBJ));                      /* UPDATE X POSITION */
    ADC(RD(RB_BLOBJ + 0xC));
    STA(RB_BLOBJ);
    TAY();
    LDA(RD(RB_BLOBJ + 1));                  /* MSB */
    ADC(RD(RB_BLOBJ + 0xD));
    STA(RB_BLOBJ + 1);
    TAX();
    SEC();
    TYA();
    SBC(RD(RB_UNIV4X));
    TAY();
    TXA();
    SBC(RD(RB_UNIV4X + 1));
    rb_wrapit(m);
    if (BCC) goto l35;
    CLC();
    TYA();
    ADC(RD(RB_UNIV4X));
    STA(RB_BLOBJ);
    TXA();
    ADC(RD(RB_UNIV4X + 1));
    STA(RB_BLOBJ + 1);
l35:
    CLC();
    LDA(RD(RB_BLOBJ + 4));                  /* UPDATE Z POSITON */
    ADC(-0x80);
    STA(RB_BLOBJ + 4);
    LDA(RD(RB_BLOBJ + 5));                  /* MSB */
    ADC(-1);
    STA(RB_BLOBJ + 5);
    CMP(1);
    if (BPL) goto l55;
l40:
    LDA(0);                                 /* CLR BLOBJ */
    STA(RB_GRDSP);
    STA(RB_BLOBJ + 0x18);
    STA(RB_BLOBJ + 6);                      /* NO DRONE */
l45:
    BIT(RD(RB_GMEND0));
    if (BMI) goto l50;
    BIT(RD(RB_GRMODE));
    if (BPL) goto l50;                      /* ONLY IN GROUND MODE */
    LDA(RD(RB_GRDSP));
    CMP(0x11);
    if (BCC) goto l50;
    rb_initbp(m); return;
l50:
    return;
l55:
    LDA(RD(RB_BLOBJ + 0x14));
    TAY();
    ORA(RD(RB_BLOBJ + 0x15));
    if (BEQ) goto l60;
    LDX(RD(RB_BLOBJ + 0x15));
    if (BMI) goto l65;
    TYA();
    ADC(-0x28);
    STA(RB_BLOBJ + 0x14);
    if (BCS) goto l60;
    DEC(RB_BLOBJ + 0x15);
l60:
    return;
l65:
    TYA();
    ADC(0x28);
    STA(RB_BLOBJ + 0x14);
    if (BCC) goto l70;
    INC(RB_BLOBJ + 0x15);
l70:
    return;
}

/* BLEXPL (6292) */
void rb_blexpl(rb_machine *m)
{
    STX(RB_TEMP1);
l10:
    STY(RB_SAVY);
    LDX(RD(AY(RB_BLEXVC)));
    LDA(RD(RB_TEMP1));
    rb_multpl(m);
    STX(RB_XCOMP + 1);                      /* VCTR X */
    STY(RB_XCOMP);
    LDY(RD(RB_SAVY));
    LDX(RD(AY(RB_BLEXVC + 1)));
    LDA(RD(RB_TEMP1));
    rb_multpl(m);
    STX(RB_YCOMP + 1);                      /* VCTR Y */
    STY(RB_YCOMP);
    LDA(0);
    STA(RB_VGBRIT);                         /* NO INTENSITY */
    rb_vgvtr2(m);                           /* POSITION VCTR */
    LDY(RD(RB_SAVY));
    LDX(RD(AY(RB_BLEXPC + 1)));
    LDA(RD(AY(RB_BLEXPC)));
    LDY(0xC0);
    rb_vgvtr(m);                            /* PICTURE VCTR */
    LDY(RD(RB_SAVY));
    DEY();
    DEY();
    if (BPL) goto l10;
    return;
}

/* 62CB BLEXVC: .BYTE 0,-4,2,0,2,-4,-4,0 */
/* 62D3  .BYTE 0,-2,-2,0,-4,0,0,2 */
/* 62DB  .BYTE -2,2,2,2,0,2,2,0 */
/* 62E3  .BYTE 2,-2,0,-2 */
/* 62E7 BLEXPC: .BYTE 1,4,4,0,2,-2,0,4 */
/* 62EF  .BYTE -2,2,-2,4,3,1,2,0 */
/* 62F7  .BYTE 1,1,1,2,1,-3,-3,-3 */
/* 62FF  .BYTE -1,-3,-1,2 */
/* WRAPIT (6303) */
void rb_wrapit(rb_machine *m)
{
    TAX();
    STA(RB_TEMP1);                          /* SAVE X SIGN */
    rb_dpabs(m);                            /* ABSOLUTE X POSITION */
    CPY(1);
    SBC(0xC);
    if (BCC) goto l10;                      /* < OFF SCREEN LIMIT */
    LDX((-0xC01 & 0xFF00) / 0x100);
    LDY(-0xC01 & 0xFF);
    BIT(RD(RB_TEMP1));                      /* CHANGE SIDES (CROSSED LIMIT BY MOTION) */
    rb_dpabs(m);
    SEC();                                  /* LIMIT CROSS */
l10:
    return;
}

/* DSPGRS (631A) */
void rb_dspgrs(rb_machine *m)
{
    LDY(0xC0);
    LDA(4);
    rb_vgstat(m);                           /* INTENSITY */
    rb_trns0(m);                            /* ROTATE ABOUT 0 */
    LDA(RD(RB_OBJECT + 5));
    LSRA();
    if (BCS) goto l10;                      /* ALTERNATE SHELL PICTURES */
    LDA(HI(RB_BULLT0));
    LDX(LO(RB_BULLT0));
    if (BCC) goto l20;                      /* ALWAYS */
l10:
    LDA(HI(RB_BULLT1));
    LDX(LO(RB_BULLT1));
l20:
    STA(RB_POINTR + 1);
    STX(RB_POINTR);
    LDY(0x17);                              /* 4 POINTS */
l30:
    LDA(RD(IY(RB_POINTR)));
    STA(AYW(RB_DB_TRP));                    /* SET POINTS IN RAM */
    DEY();
    if (BPL) goto l30;
    LDX(0x12);
    rb_d_loop(m);
    LDA(HI(RB_BULLDE));
    LDX(LO(RB_BULLDE));
    rb_dcmpa0(m); return;                   /* DECODE POINTS */
}


/* ---- CHKSUM 5 (6000-67FF) [FF] ---- */

/* 634D CHKSM5: .BYTE 64 */
