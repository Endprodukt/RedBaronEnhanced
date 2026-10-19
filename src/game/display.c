/* display.c - DISPLAY: PLAYFIELD, PLANE MO, EXPLOSIONS, SHELLS
 *
 * Red Baron (Atari, 1981), RBARON of the original source as C.
 * Written by tools/mac2c.py from upstream/RBARON.MAC: change the translator or its
 * tables, not this file. The routines, labels and comments are the original's;
 * one C statement stands for one instruction (src/game/m6502.h).
 */

#include "game/game.h"
#include "game/m6502.h"


/* ---- DISPLAY ---- */


/* ---- PLAYFIELD ---- */

/* SCENE (634E) */
void rb_scene(rb_machine *m)
{
    LDY(0);
    STY(RB_FLAG);
    STY(RB_FLAG + 1);
    STY(RB_OBJECT + 2);                     /* Y POSITION */
    STY(RB_OBJECT + 3);
    STY(RB_OBJECT + 4);
    LDA(RB_HORZ / 0x100);
    STA(RB_OBJECT + 5);
    LDY(RD(RB_PFXSCR));                     /* LEFT EDGE X COOR (*4) */
    LDX(RD(RB_PFXSCR + 1));
    rb_d_comp(m);
    AND(7);
    SEC();
    SBC(0x10);
    STY(RB_TEMP2);
    STY(RB_OBJECT);
    STA(RB_OBJECT + 1);                     /* X POSITION = -(EDGE OF SCREEN - OFFSET) */
    STA(RB_TEMP2 + 1);                      /* TEMP2,TEMP2+1= X COOR */
    LDX(RD(RB_PFXSCR + 1));
    LDA(RD(RB_PFXSCR));
    if (BNE) goto l10;
    DEX();
l10:
    TXA();
    CLC();
    ADC(8);
    AND(0x18);
    STA(RB_SAVX + 1);
    LSRA();
    LSRA();
    STA(RB_TEMP1);
    LSRA();
    ADC(RD(RB_SAVX + 1));                   /* CARRY CLR */
    ADC(RD(RB_TEMP1));
    STA(RB_SAVX + 1);                       /* SAVX=DB OFFSET (PFOBJ 11. BYTES/) */
    LDA(3);
    STA(RB_SAVX + 2);                       /* SAVX+2=COUNTER */
    rb_posith(m);                           /* ADD HORIZN ADJ */
    rb_placbm(m);
    LDX(RD(RB_SAVX + 1));                   /* SEGMENT OFFSET */
l15:
    STX(RB_SAVX + 1);                       /* PF OBJECT DB OFFSET */
    LDA(RD(AX(RB_PFOBJ + 6)));              /* SEGMENT STILL IN PLACE */
    if (BPL) goto l20;                      /* NO (DISPLAY STRAIGHT LINE) */
    AND(0xF);
    STA(RB_SAVY + 1);
    LDA(RD(AX(RB_PFOBJ + 2)));
    STA(RB_OBJECT + 4);
    LDA(RD(AX(RB_PFOBJ + 3)));
    STA(RB_OBJECT + 5);
    LDA(RD(RB_TEMP2));
    STA(RB_OBJECT);                         /* X CENTER OF OBJECT (LSB) */
    CLC();
    LDA(RD(RB_TEMP2 + 1));
    ADC((RB_PF_SGX & 0xFF00) / 0x100);
    STA(RB_OBJECT + 1);                     /* X CENTER (MSB) */
    LDY(RB_PF_BRT);
    STY(RB_V_BRIT);                         /* INTENSITY */
    LDA(4);
    rb_vgstat(m);                           /* GET INTENSITY REGISTER */
    rb_posith(m);                           /* ADD HORIZN ADJ */
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PFOBJ + 4)));              /* Y DEPTH (ON HORIZON) */
    STA(RB_OBJECT + 6);
    LDA(RD(AX(RB_PFOBJ + 5)));
    STA(RB_OBJECT + 7);
    RB_NOTED(RB_OBJ_MOUNTAIN, RB_TAGGED(RB_TAG_MOUNTAIN, rb_pfdspl(m))); /* DISPLAY OBJECT */
    goto l30;
l20:
    LDX(3);
l22:
    LDA(RD(ZX(RB_XPOS)));
    STA(ZX(RB_XPOINT));
    DEX();
    if (BPL) goto l22;
    LDX(RD(RB_SAVX + 1));
    LDY(RB_PF_BRT);                         /* HORIZON INTENSITY */
    LDA(RD(AX(RB_PFOBJ + 3)));
    CMP(0xF);
    if (BCC) goto l25;
    LDY(0);
    CMP(0x10);
    if (BCS) goto l25;
    LDA(RD(AX(RB_PFOBJ + 2)));
    LSRA();
    EOR(0x60);
    AND(0x60);
    TAY();
l25:
    LDA(4);
    rb_vgstat(m);                           /* SET INTENSITY REG */
    RB_TAGGED(RB_TAG_HORIZON, rb_scene0(m)); /* CONSTRUCT HORIZON (256. ALONG X-AXIS) */
l30:
    CLC();
    LDA(RD(RB_TEMP2 + 1));
    ADC((RB_PF_QTX & 0xFF00) / 0x100);
    STA(RB_TEMP2 + 1);
    LDX(1);
l32:
    LDA(RD(ZX(RB_TEMP2)));
    STA(ZX(RB_OBJECT));
    LDA(0);
    STA(ZX(RB_OBJECT + 2));
    DEX();
    if (BPL) goto l32;
    STA(RB_OBJECT + 4);
    LDA(RB_HORZ / 0x100);
    STA(RB_OBJECT + 5);
    rb_posith(m);                           /* ADD HORIZN ADJ */
    LDA(RD(RB_SAVX + 1));
    CLC();
    ADC(RB_L_PFOB);
    CMP(RB_N_PFOB + 1);
    if (BCC) goto l35;
    LDA(0);
l35:
    TAX();
    DEC(RB_SAVX + 2);
    if (BMI) goto l40;
    goto l15;
l40:
    LDX(RB_N_PFOB);
l50:
    LDA(RD(AX(RB_PFOBJ + 6)));              /* PF OBJECT SCROLLING */
    if (BMI) goto l60;                      /* NO */
    AND(0xF);
    STA(RB_SAVY + 1);                       /* PF OBJECT # */
    LDA(RD(AX(RB_PFOBJ + 7)));
    if (BMI) goto l60;                      /* GROUND SEQ INVISIBLE */
    AND(0xF);
    STA(RB_TEMP3 + 8);
    RB_TAGGED(RB_TAG_MOUNTAIN, rb_scene1(m)); /* DISPLAY PF OBJECT */
    RB_NOTED(RB_OBJ_MOUNTAIN, RB_TAGGED(RB_TAG_MOUNTAIN, rb_scene2(m)));
    BIT(RD(RB_GRMODE));
    if (BPL) goto l55;
    LDX(RD(RB_SAVX + 1));
    RB_TAGGED(RB_TAG_GROUND, rb_grdisp(m)); /* DISPLAY GROUND OBJECTS */
l55:
    LDX(RD(RB_SAVX + 1));
l60:
    TXA();
    SEC();
    SBC(RB_L_PFOB);
    TAX();
    if (BCS) goto l50;
    BIT(RD(RB_GRMODE));
    if (BPL) goto l70;
    LDA(RD(RB_FLAG + 1));
    if (BEQ) goto l65;
    BIT(RD(RB_FLAG));
    if (BMI) goto l70;
l65:
    INC(RB_GRDSP);
l70:
    return;
}

/* SCENE0 (646C) */
void rb_scene0(rb_machine *m)
{
    CLC();
    LDA(RD(RB_TEMP2));
    STA(RB_OBJECT);
    LDA(RD(RB_TEMP2 + 1));
    ADC((RB_PF_QTX & 0xFF00) / 0x100);
    STA(RB_OBJECT + 1);
    rb_posith(m);                           /* ADD HORIZN ADJ */
    LDX(3);
l10:
    LDA(RD(ZX(RB_XPOS)));
    STA(AXW(RB_DB_TRP + 2));
    DEX();
    if (BPL) goto l10;
    LDX(RB_H_MAP & 0xFF);
    LDA((RB_H_MAP & 0xFF00) / 0x100);
    rb_decmap(m); return;                   /* DISPLAY VCTR */
}

/* SCENE1 (648B) */
void rb_scene1(rb_machine *m)
{
    LDA(RD(AX(RB_PFOBJ + 3)));
    ASLA();
    ASLA();
    ASLA();
    if (BPL) goto l10;
    LDA(0x70);
l10:
    AND(0xF0);
    STA(RB_V_BRIT);
    LDA(RB__PFOBJ);
    SEC();
    SBC(RD(RB_V_BRIT));
    AND(0xF0);
    STX(RB_SAVX + 1);                       /* SAVE OFFSET */
    STA(RB_V_BRIT);                         /* INTENSITY SET TO DEPTH */
    LDY(0);
l20:
    LDA(RD(AX(RB_PFOBJ)));
    STA(AYW(RB_OBJECT));                    /* ESTABLISH POSITION */
    LDA(0);
    STA(AYW(RB_OBJECT + 2));                /* ALTITUDE=0 */
    LDA(RD(AX(RB_PFOBJ + 2)));
    STA(AYW(RB_OBJECT + 4));
    INX();
    INY();
    CPY(2);
    if (BCC) goto l20;
    rb_trns0(m);                            /* ROTATE ABOUT 0,0 */
    rb_posith(m); return;                   /* POSITION BEAM */
}

/* PLAYFIELD OBJECT DISPLAY */
/* SCENE2 (64C3) */
void rb_scene2(rb_machine *m)
{
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PFOBJ + 4)));
    STA(RB_OBJECT + 4);
    STA(RB_OBJECT + 6);
    LDA(RD(AX(RB_PFOBJ + 5)));
    STA(RB_OBJECT + 5);
    STA(RB_OBJECT + 7);
    rb_pfdspl(m);                           /* (falls into it) */
}

/* PFDSPL (64D3) */
void rb_pfdspl(rb_machine *m)
{
    LDY(RD(RB_SAVY + 1));                   /* OBJECT OFFSET */
    LDX(RD(AY(RB_SSEGS)));                  /* OBJECT POINT TABLE ADDR */
    LDA(RD(AY(RB_SSEGS + 1)));              /* ADDR MSB */
    PHA();
    TYA();
    LSRA();
    TAY();
    LDA(RD(AY(RB__SSEG)));
    TAY();
    PLA();
    rb_project(m);                          /* PROJECT POINTS */
    LDA(RD(RB_SAVY + 1));                   /* OBJECT #*2 */
    ASLA();
    ASLA();
    STA(RB_SAVY + 1);                       /* OBJECT #*8 */
    LDX(3);
    STX(RB_TEMP4);                          /* LOOP COUNT */
l10:
    LDA(RD(RB_SAVY + 1));
    LSRA();
    TAX();
    LDY(RD(AX(RB_PFOPOS)));                 /* LEFT EDGE */
    LDA(0);
    STA(RB_TEMP1);
    rb_pfxpos(m);                           /* CALCULATE POSITION */
    if (BCC) goto l20;                      /* ON SCREEN */
    LDA(RD(RB_SAVY + 1));
    LSRA();
    TAX();
    LDY(RD(AX(RB_PFOPOS + 0x10)));          /* RIGHT EDGE */
    rb_pfxpos(m);                           /* CALCULATE POSITION */
    if (BCS) goto l35;                      /* OFF SCREEN */
    DEC(RB_TEMP1);
l20:
    LDA(0);
    STA(RB_VGBRIT);                         /* VCTR 0 INTENSITY */
    rb_vgcntr(m);                           /* CENTER BEAM */
    rb_vgvtr2(m);                           /* POSITION BEAM */
    LDY(RD(RB_V_BRIT));
    LDA(4);
    rb_vgstat(m);
    LDY(RD(RB_SAVY + 1));
    LDA(RD(AY(RB_SCMAP + 1)));              /* DISPLAY BRIGHTER SEGMENTS */
    LDX(RD(AY(RB_SCMAP)));
    BIT(RD(RB_TEMP1));
    if (BPL) goto l28;
    LDX(RD(AY(RB_SCMAP + 0x20)));
    LDA(RD(AY(RB_SCMAP + 0x21)));
l28:
    rb_decmap(m);
    LDA(RD(RB_OBJECT + 4));
    CMP(1);
    LDA(RD(RB_OBJECT + 5));
    SBC(2);
    if (BCS) goto l29;
    BIT(RD(RB_GREND));
    if (BMI) goto l29;                      /* ALREADY EOL */
    rb_plycol(m);
    if (BCS) goto l29;
    LDA(RD(RB_SAVX + 1));                   /* MOUNTAIN OFFSET OF CRASH */
    ORA(0xC0);
    STA(RB_GREND);                          /* D6=GROUND COLLISION */
l29:
    LDA(RD(RB_V_BRIT));
    SEC();
    SBC(0x30);
    if (BCS) goto l30;
    LDA(0x20);
l30:
    TAY();
    LDA(4);
    rb_vgstat(m);                           /* SET INTENSITY REGISTER */
    LDY(RD(RB_SAVY + 1));
    LDA(RD(AY(RB_SCLMAP + 1)));             /* DISPLAY DIM SEGMENTS */
    if (BMI) goto l35;                      /* NO DIM LINES FOR THIS PIECE */
    LDX(RD(AY(RB_SCLMAP)));
    rb_decmap(m);
l35:
    INC(RB_SAVY + 1);                       /* NEXT PIECE */
    INC(RB_SAVY + 1);
    DEC(RB_TEMP4);
    if (BMI) goto l40;
    goto l10;
l40:
    return;
}

/* ENTRY EXIT */
/* ACC DC XCOMP+1 */
/* X DC 0FE */
/* Y POINTY POINTY-4 */
/* PFXPOS (6576) */
void rb_pfxpos(rb_machine *m)
{
    LDX(2);                                 /* Y OFFSET */
l10:
    CLC();
    LDA(RD(AY(RB_DB_TRP)));
    STA(AXW(RB_DB_TRP + 0xF2));             /* IN CASE OF CD */
    STA(ZX(RB_XPOINT));
    ADC(RD(ZX(RB_XPOS)));
    STA(ZX(RB_XCOMP));
    LDA(RD(AY(RB_DB_TRP + 1)));
    STA(AXW(RB_DB_TRP + 0xF3));
    STA(ZX(RB_XPOINT + 1));
    ADC(RD(ZX(RB_XPOS + 1)));
    STA(ZX(RB_XCOMP + 1));
    DEY();
    DEY();
    DEX();
    DEX();
    if (BPL) goto l10;
    /* JMP DSTNCE ;CONTINUE TO DSTNCE */
    /* DISTANCE CALCULATION XCOMP = X POSITION */
    /* YCOMP = Y POSITION */
    LDY(RD(RB_XCOMP));
    LDX(RD(RB_XCOMP + 1));
    rb_dpabs(m);                            /* ABSOLUTE OF X POS */
    STY(RB_RESULT);
    STX(RB_RESULT + 1);
    LDY(RD(RB_YCOMP));
    LDX(RD(RB_YCOMP + 1));
    rb_dpabs(m);                            /* ABSOLUTE OF Y POS */
    CPY(RD(RB_RESULT));
    SBC(RD(RB_RESULT + 1));
    LDX(2);                                 /* ASSUME Y < X */
    if (BCC) goto dstnce_l10;
    LDX(0);                                 /* X < Y */
dstnce_l10:
    LDA(RD(ZX(RB_XCOMP + 1)));              /* 1/2 * SMALLER */
    CMP(0x80);
    RORA();
    TAY();
    LDA(RD(ZX(RB_XCOMP)));
    RORA();
    CPX(1);
    LDX(0);                                 /* ASSUME X > Y */
    if (BCS) goto dstnce_l20;
    LDX(2);                                 /* Y > X */
dstnce_l20:
    CLC();
    ADC(RD(ZX(RB_XCOMP)));                  /* + LARGER */
    STA(RB_RESULT);
    TYA();
    ADC(RD(ZX(RB_XCOMP + 1)));
    LDY(RD(RB_RESULT));
    TAX();
    rb_dpabs(m);                            /* ABSOLUTE */
    CPY(0x80);
    SBC(2);                                 /* COMPARE TO SCREEN EDGE */
    return;
}


/* ---- PLANE MO ---- */

/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X DC   BLITZED */
/* Y DC   BLITZED */
/* MOTOBJ (65D7) */
void rb_motobj(rb_machine *m)
{
    LDX(RB_N_MOB);
l10:
    STX(RB_SAVX + 2);
    TXA();
    if (BNE) goto l15;                      /* DRONE/PLANE */
    LDA(RD(ZX(RB_PLOBDB + 0x18)));
    if (BMI) goto l50;                      /* BLIMP */
l15:
    LDA(RD(ZX(RB_PLOBDB + 6)));
    BIT(RD(RB_BIT1));                       /* PARALLEL? */
    if (BEQ) goto l20;                      /* NO */
    CLC();
    LDA(RD(RB_PLSTAT));
    ADC(RD(ZX(RB_PLOBDB)));                 /* DRONE X,Y IS OFFSET FROM */
    STA(RB_OBJECT);                         /* PLANE */
    LDA(RD(RB_PLSTAT + 1));
    ADC(RD(ZX(RB_PLOBDB + 1)));
    STA(RB_OBJECT + 1);
    CLC();
    LDA(RD(RB_PLSTAT + 2));
    ADC(RD(ZX(RB_PLOBDB + 2)));
    STA(RB_OBJECT + 2);
    LDA(RD(RB_PLSTAT + 3));
    ADC(RD(ZX(RB_PLOBDB + 3)));
    rb_plnlb0(m);                           /* CONSTRUCT LABS */
    if (BCC) goto l30;                      /* VISIBLE */
    if (BCS) goto l50;                      /* NOT VISIBLE */
l20:
    AND(0xC5);                              /* FL EX X X X EX X FL */
    if (BEQ) goto l50;                      /* PLANE/DRONE NOT PRESENT */
    rb_plnlbs(m);                           /* CONSTRUCT LABS */
    if (BCS) goto l50;                      /* NOT VISIBLE */
l30:
    LDX(RD(RB_SAVX + 2));
    LDA(RD(ZX(RB_PLOBDB + 6)));
    if (BMI) goto l47;                      /* PLANE FLIGHT */
    BIT(RD(RB_BIT6));
    if (BNE) goto l49;                      /* EXPLOSION */
    BIT(RD(RB_BIT2));
    if (BNE) goto l42;                      /* DRONE EXPLOSION */
    BIT(RD(RB_DRNSFL));
    if (BPL) goto l35;
    CPX(RD(RB_DRNSET));
    if (BNE) goto l35;
    rb_plnshl(m);
l35:
    RB_TAGGED(RB_TAG_DRONE, rb_drone(m));   /* DRONE FLIGHT */
    goto l50;
l42:
    RB_TAGGED(RB_TAG_DRONE, rb_drnexp(m));  /* DRONE EXPLOSION */
    goto l50;
l47:
    LDX(3);
l48:
    LDA(RD(AX(RB_PFOBJL)));
    STA(ZX(RB_XCOMP));
    DEX();
    if (BPL) goto l48;
    BIT(RD(RB_GMEND0));
    if (BMI) goto l49;                      /* RETURNING PLANE */
    LDX(RD(RB_SAVX + 2));
    rb_plnshl(m);
l49:
    rb_plane(m);
l50:
    LDA(RD(RB_SAVX + 2));
    SEC();
    SBC(RB_L_OBJ);
    TAX();
    if (BCC) goto l60;
    goto l10;
l60:
    LDA(0);
    STA(RB_DRNFLG);
    return;
}

/* CHECK PLANE FIRE */
/* PLNSHL (665E) */
void rb_plnshl(rb_machine *m)
{
    LDA(RD(ZX(RB_PLOBDB + 6)));
    AND(8);
    if (BEQ) goto l10;                      /* PLANE DOES NOT SHOOT */
    LDA(0x80);
    STA(RB_SHMODE);                         /* FAST SHELLS */
    LDA(RD(RB_FRAME));
    LSRA();
    if (BCS) goto l10;                      /* EVERY OTHER FRAME */
    rb_shlau0(m); return;                   /* FIRE SHELLS */
l10:
    return;
}

/* X=PLANE OFFSET */
/* SET PLANE POSITION (BY CENTER W/VCTR) CC=VISIBLE PLANE */
/* PLNLBS (6671) */
void rb_plnlbs(rb_machine *m)
{
    LDA(RD(ZX(RB_PLOBDB + 0x19)));          /* POSITION DEPTH */
    STA(RB_OBJECT + 4);
    LDA(RD(ZX(RB_PLOBDB + 0x1A)));
    STA(RB_OBJECT + 5);
    LDA(RD(ZX(RB_PLOBDB + 5)));
    ASLA();
    ASLA();
    ASLA();
    AND(RB__MOTOBJ);
    EOR(RB__MOTOBJ);                        /* SET BRIGHTNESS BY DEPTH */
    CMP(0x60);
    if (BCS) goto l10;
    LDA(0x60);
l10:
    STA(RB_V_BRIT);
    TAY();
    LDA(4);
    rb_vgstat(m);
    LDX(RD(RB_SAVX + 2));
    LDA(RD(ZX(RB_PLOBDB)));                 /* X POSITION (LSB) */
    STA(RB_OBJECT);
    LDA(RD(ZX(RB_PLOBDB + 1)));             /* X POSITION (MSB) */
    STA(RB_OBJECT + 1);
    LDA(RD(ZX(RB_PLOBDB + 2)));             /* Y POSITION (LSB) */
    STA(RB_OBJECT + 2);
    LDA(RD(ZX(RB_PLOBDB + 3)));             /* Y POSITION (MSB) */
    rb_plnlb0(m);                           /* (falls into it) */
}

/* PLNLB0 (66A0) */
void rb_plnlb0(rb_machine *m)
{
    STA(RB_OBJECT + 3);
    rb_pftrig(m);                           /* PF ROTATION */
    rb_positp(m);
    rb_setbm(m);
    LDX(RD(RB_SAVX + 2));
    LDA(RD(ZX(RB_PLOBDB + 4)));
    STA(RB_O_DPTH);
    LDA(RD(ZX(RB_PLOBDB + 5)));
    STA(RB_O_DPTH + 1);
    LDY(3);
l10:
    LDA(RD(AY(RB_XPOS)));
    STA(ZX(RB_PLOBDB + 11));
    DEX();
    DEY();
    if (BPL) goto l10;
    return;
}

/* DISPLAY FULL PLANE */
/* DSPPLN (66C1) */
void rb_dsppln(rb_machine *m)
{
    LDX(0x40);                              /* BUFFER 0(2640) */
    LDA(0x26);
    BIT(RD(RB_BUFFER));
    if (BPL) goto l10;
    LDA(0x2E);                              /* BUFFER 1(2E40) */
l10:
    rb_vgjsrl(m); return;
}

/* DISPLAY DRONE */
/* DSPDRN (66CE) */
void rb_dspdrn(rb_machine *m)
{
    LDX(0x30);
    LDA(0x25);
    BIT(RD(RB_BUFFER));                     /* BUFFER 0(2530) */
    if (BPL) goto l10;
    LDA(0x2D);                              /* BUFFER 1(2D30) */
l10:
    rb_vgjsrl(m); return;                   /* STORE JSRL TO DRONE */
}

/* ROTATE BIPLANE (ON ALL AXES) AND PROJECT ONTO XY MONITOR */
/* DB.PLN = Z LOCATION (S7654321) */
/* DB.PLN+1= X LOCATION (S7654321) */
/* DB.PLN+2= Y LOCATION (S7654321) */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X DC   BLITZED */
/* Y DC   BLITZED */
/* PLANE (66DB) */
void rb_plane(rb_machine *m)
{
    rb_dsppln(m);
    LDA(RD(RB_RAMPTR));
    STA(RB_TEMP4);                          /* SAVE VG POINTER */
    LDA(RD(RB_RAMPTR + 1));
    STA(RB_TEMP4 + 1);
    LDA(0xA0);
    STA(RB_RAMPTR);
    LDA(RB_VGRAM / 0x100 + 7);
    BIT(RD(RB_BUFFER));
    if (BPL) goto l10;
    LDA(RB_VGRAM / 0x100 + 0xF);
l10:
    STA(RB_RAMPTR + 1);                     /* PROPS START AT SET ADDRESS (BY BUFFER) */
    RB_TAGGED(RB_TAG_PROP, rb_plnprp(m));
    LDA(0x40);
    STA(RB_RAMPTR);
    LDA(RB_VGRAM / 0x100 + 6);              /* ASSUME BUFFER 0 */
    BIT(RD(RB_BUFFER));
    if (BPL) goto l20;
    LDA(RB_VGRAM / 0x100 + 0xE);            /* ELSE BUFFER 1 */
l20:
    STA(RB_RAMPTR + 1);
    BIT(RD(RB_PLSTAT + 6));
    if (BVC) goto l30;
    LDA(RD(RB_GTIMER + 1));
    if (BEQ) goto l30;
    rb_plnexp(m);
    LDA(RD(RB_PROP_F));
    ORA(0x80);                              /* NO PLANE, PROP */
    STA(RB_PROP_F);
    goto l40;
l30:
    LDX(0);                                 /* FULL PLANE */
    rb_pltest(m);
    RB_NOTED(RB_OBJ_PLANE, rb_pldecd(m));
    LDY(RD(RB_V_BRIT));
    LDA(4);
    rb_vgstat(m);
    LDA(4);
    TAY();
    rb_vgstat(m);                           /* 2 BYTES FOR PROP */
l40:
    rb_vgrtsl(m);
    LDA(RD(RB_TEMP4));
    STA(RB_RAMPTR);
    LDA(RD(RB_TEMP4 + 1));
    STA(RB_RAMPTR + 1);
    return;
}

/* DRONE (673A) */
void rb_drone(rb_machine *m)
{
    BIT(RD(RB_DRNFLG));
    if (BMI) goto l40;                      /* DRONE PICTURE DB IN USE */
    DEC(RB_DRNFLG);                         /* SET (=0FF) */
    LDA(RD(RB_RAMPTR));
    STA(RB_TEMP4);
    LDA(RD(RB_RAMPTR + 1));
    STA(RB_TEMP4 + 1);
    LDX(0x30);
    LDA(RB_VGRAM / 0x100 + 5);
    BIT(RD(RB_BUFFER));
    if (BPL) goto l10;
    LDA(RB_VGRAM / 0x100 + 0xD);
l10:
    STA(RB_RAMPTR + 1);                     /* DRONE PICTURE DB POINTER */
    STX(RB_RAMPTR);
    rb_drnpic(m);
    rb_vgrtsl(m);
    LDA(RD(RB_TEMP4));
    STA(RB_RAMPTR);                         /* RESTORE RAM POINTER */
    LDA(RD(RB_TEMP4 + 1));
    STA(RB_RAMPTR + 1);
l35:
    rb_dspdrn(m); return;                   /* DISPLAY DRONE (JSRL TO PICTURE, DB) */
l40:
    BIT(RD(RB_DRNSFL));
    if (BPL) goto l35;                      /* NO GROWTH */
    rb_drnpic(m);                           /* (falls into it) */
}

/* CONTINUE INTO DRNPIC */
/* DRNPIC (676B) */
void rb_drnpic(rb_machine *m)
{
    LDA(RD(RB_PLSTAT + 6));                 /* PLANE ROTATED */
    AND(0x10);
    if (BEQ) goto l20;                      /* YES */
    LDA(RB_DROFFZ & 0xFF);                  /* ELSE SMALL SCALE PLANE */
    STA(RB_DD_DML);
    LDA((RB_DROFFZ & 0xFF00) / 0x100);
    STA(RB_DD_DMM);
    rb_drntst(m);                           /* SCALE DOWN PLANE POINTS */
    LDX(RB_DB_MAP & 0xFF);
    LDA((RB_DB_MAP & 0xFF00) / 0x100);
    rb_dcmpa0(m); return;                   /* DECODE POINTS TO GRAPH */
l20:
    LDY(RB__DRPNT - 3);
    LDX(0);
    rb_plnabz(m);                           /* 2-D DRONE POINTS */
    LDX((RB__DRPNT - 3) * 2);
    rb_d_loop(m);                           /* SCALE */
    LDX(RB_DB_MAR & 0xFF);
    LDA((RB_DB_MAR & 0xFF00) / 0x100);
    rb_dcmpa0(m); return;
}

/* ROTATE & PROJECT POINTS (POINTP FORMAT) */
/* X=SOURCE OFFSET */
/* PLTEST (6798) */
void rb_pltest(rb_machine *m)
{
    STX(RB_SAVX + 1);
    CLC();                                  /* Z AXIS ROTATION */
    LDA(RD(RB_PLSTAT + 0x12));
    ADC(RD(RB_PFROTN));
    TAY();
    LDA(RD(RB_PLSTAT + 0x13));
    ADC(RD(RB_PFROTN + 1));
    rb_trig(m);
    LDX(RD(RB_SAVX + 1));
    LDY(RD(AX(RB_PPNTS)));
    rb_plnabz(m);
    LDA(RD(RB_PLSTAT + 6));
    AND(0x10);
    if (BNE) goto l5;                       /* UNTIL FULL ROTATION NEED Y AXIS ROTATION */
    LDA(RD(RB_GAMODE));
    AND(RB_BNRBIT);                         /* CHECK FOR BANNER DISPLAY */
    if (BEQ) goto l10;                      /* NOT BANNER */
l5:
    LDA(RD(RB_PLSTAT + 0x15));              /* Y AXIS ROTATION */
    LDY(RD(RB_PLSTAT + 0x14));
    rb_trig(m);
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PPNTS)));
    ASLA();
    TAX();
    rb_yaxis(m);
    goto l15;
l10:
    LDA(RD(RB_PLSTAT + 0x17));              /* X AXIS ROTATION */
    LDY(RD(RB_PLSTAT + 0x16));
    rb_trig(m);
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PPNTS)));
    ASLA();
    TAX();
    rb_xaxis(m);
l15:
    LDX(RD(RB_SAVX + 1));
    LDA(RD(AX(RB_PPNTS)));
    ASLA();
    TAX();
    rb_d_loop(m); return;
}

/* DECODE PLANE POINTS (FULL 3D) */
/* PLDECD (67EA) */
void rb_pldecd(rb_machine *m)
{
    LDX(RB_DB_MAP & 0xFF);
    LDA((RB_DB_MAP & 0xFF00) / 0x100);
    rb_dcmpa0(m);
    LDA(RD(RB_V_BRIT));
    STA(RB_TEMP2);
    SEC();
    SBC(0x60);
    if (BCS) goto l10;
    LDA(0);
l10:
    TAY();                                  /* ADD LIGHTER LINES */
    LDA(4);
    rb_vgstat(m);
    LDX(RB_DB_LNS & 0xFF);
    LDA((RB_DB_LNS & 0xFF00) / 0x100);
    rb_decmap(m); return;
}

/* CONSTRUCT PLANE PROP PICTURES */
/* PLNPRP (6809) */
void rb_plnprp(rb_machine *m)
{
    LDX(1);
    rb_pltest(m);                           /* CONSTRUCT PROP POINTS */
    LDX(3);
l5:
    LDA(RD(AX(RB_DB_TRP + 2)));
    STA(ZX(RB_XPOINT));
    STA(ZX(RB_TEMP3));                      /* SAVE ORIGIN POINT */
    DEX();
    if (BPL) goto l5;
    LDY(4);
l10:
    STY(RB_SAVY + 1);                       /* PICTURE OFFSET */
    LDA(RD(AY(RB_PROPS + 1)));
    LDX(RD(AY(RB_PROPS)));
    rb_decmap(m);                           /* DECODE ONE PROP BLADE PAIR */
    rb_vgrtsl(m);                           /* CONSTRUCT RTSL (VG) */
    LDX(3);
l15:
    LDA(RD(ZX(RB_TEMP3)));
    STA(ZX(RB_XPOINT));                     /* RESET ORIGIN FOR NEXT PAIR */
    DEX();
    if (BPL) goto l15;
    LDY(RD(RB_SAVY + 1));
    DEY();
    DEY();
    if (BPL) goto l10;                      /* DO ALL THREE BLADE PAIRS */
    return;
}


/* ---- EXPLOSIONS ---- */

/* DISPLAY PLANE EXPLOSION PIECES */
/* ENTRY   EXIT */
/* ACC DC   BLITZED */
/* X DC   BLITZED */
/* Y DC   BLITZED */
/* USES - SAVY+2,PLCDLT(S),GTIMER,XCOMP,+1,YCOMP,+1,VGBRIT,P.STAT(S),PLPCLN(S) */
/* PLPCL2(S),PLPCDE(S) */
/* CALLS - MULTPL,VGVCTR,TRIG,PLNABZ,YAXIS,XAXIS,D.LOOP,DECMAP */
/* PLNEXP (683A) */
void rb_plnexp(rb_machine *m)
{
    LDY(6);                                 /* 4 PIECES */
l10:
    STY(RB_SAVY + 2);
    LDX(RD(AY(RB_PLCDLT)));                 /* X DELTA */
    LDA(RD(RB_GTIMER + 1));                 /* INTERVAL TIMER */
    rb_multpl(m);
    STX(RB_XCOMP + 1);                      /* VCTR X */
    STY(RB_XCOMP);
    LDY(RD(RB_SAVY + 2));
    LDX(RD(AY(RB_PLCDLT + 1)));             /* Y DELTA */
    LDA(RD(RB_GTIMER + 1));                 /* TIMER */
    rb_multpl(m);
    STX(RB_YCOMP + 1);                      /* VCTR Y */
    STY(RB_YCOMP);
    LDA(0);
    STA(RB_VGBRIT);                         /* NO INTENSITY */
    rb_vgvtr2(m);                           /* CONSTRUCT VCTR */
    CLC();
    LDA(RD(RB_PFROTN));
    ADC(RD(RB_PLSTAT + 0x12));
    TAY();
    LDA(RD(RB_PFROTN + 1));
    ADC(RD(RB_PLSTAT + 0x13));
    rb_trig(m);                             /* ANGLE ABOUT Z */
    LDA(RD(RB_SAVY + 2));
    LSRA();
    TAX();
    LDA(RD(AX(RB_PLPCLN)));
    TAY();
    TXA();
    CLC();
    ADC(3);
    TAX();
    rb_plnabz(m);                           /* ROTATE ABOUT Z */
    LDA(RD(RB_PLSTAT + 0x17));
    LDY(RD(RB_PLSTAT + 0x16));
    rb_trig(m);                             /* ANGLE ABOUT X */
    LDA(RD(RB_SAVY + 2));
    LSRA();
    TAX();
    LDA(RD(AX(RB_PLPCL2)));
    TAX();
    rb_xaxis(m);                            /* ROTATE ABOUT X */
    LDA(RD(RB_SAVY + 2));
    LSRA();
    TAX();
    LDA(RD(AX(RB_PLPCL2)));
    TAX();
    rb_d_loop(m);                           /* PROJECT BY DEPTH */
    LDY(RD(RB_SAVY + 2));
    LDA(RD(AY(RB_PLPCDE + 1)));
    LDX(RD(AY(RB_PLPCDE)));
    RB_NOTED(RB_OBJ_PIECES, rb_dcmpa0(m));  /* DECODE POINTS */
    LDY(RD(RB_SAVY + 2));
    DEY();
    DEY();
    if (BPL) goto l10;
    return;
}

/* PLANE EXPLOSION PIECES DELTAS (X,Y) */
/* 68AB PLCDLT: .BYTE 0,14 */
/* 68AD  .BYTE 14,-2 */
/* 68AF  .BYTE -28,0 */
/* 68B1  .BYTE 14,-16 */
/* DRONE EXPLOSION ROUTINE */
/* DRNEXP (68B3) */
void rb_drnexp(rb_machine *m)
{
    INC(ZX(RB_PLOBDB + 7));
    LDA(RD(ZX(RB_PLOBDB + 7)));
    CMP(0x10);
    if (BCS) goto l20;                      /* FINISHED W/ EXPLOSION */
    STA(RB_TEMP1);
    LDY(8);
l10:
    STY(RB_SAVY);
    LDX(RD(AY(RB_DPCDEL)));
    LDA(RD(RB_TEMP1));
    rb_multpl(m);
    STX(RB_XCOMP + 1);
    STY(RB_XCOMP);
    LDY(RD(RB_SAVY));
    LDX(RD(AY(RB_DPCDEL + 1)));
    LDA(RD(RB_TEMP1));
    rb_multpl(m);
    STX(RB_YCOMP + 1);
    STY(RB_YCOMP);
    LDA(0);
    STA(RB_VGBRIT);
    rb_vgvtr2(m);
    LDY(RD(RB_SAVY));
    LDA(RD(AY(RB_DPCTBL)));
    LDX(RD(AY(RB_DPCTBL + 1)));
    rb_vgadd2(m);
    LDY(RD(RB_SAVY));
    DEY();
    DEY();
    if (BPL) goto l10;
    return;
l20:
    LDA(0);
    STA(ZX(RB_PLOBDB + 6));                 /* CLR DRONE (& EXPLOSION) */
    return;
}


/* ---- CHKSUM 6 (6800-6FFF) [FF] ---- */

/* 68F9 CHKSM6: .BYTE 6C */

/* ---- SHELLS ---- */

/* UPDATE SHELL POSITION, DISPLAY AND CHECK FOR COLLISIONS */
/* SHLMOT (68FA) */
void rb_shlmot(rb_machine *m)
{
    rb_cdsset(m);                           /* SET COLLISION DETECT FRAMES */
    CLC();
    rb_dspshl(m);                           /* DISPLAY SHELLS */
    rb_pstshl(m);                           /* UPDATE SHELL POSITION */
    SEC();
    rb_dspshl(m);
    rb_pstshl(m);
    CLC();
    rb_dspshl(m);
    rb_pstshl(m);
    SEC();
    rb_dspshl(m);
    rb_pstshl(m); return;
}

/* ENTRY   EXIT */
/* ACC */
/* X */
/* Y */
/* PSTSHL (6919) */
void rb_pstshl(rb_machine *m)
{
    LDX(RB_N_SHL);                          /* POINT @ LAST SHELL */
l10:
    LDA(RD(AX(RB_SHELLS + 7)));
    if (BPL) goto l20;
    INC(AXW(RB_SHELLS + 5));                /* UPDATE Z (DEPTH) */
    LDA(RD(AX(RB_SHELLS + 5)));
    CMP(RB_S_MAXZ & 0xFF);                  /* MAX SHELL DEPTH ? */
    if (BCC) goto l20;                      /* NO */
    LDA(0);                                 /* ELSE CLEAR SHELL */
    STA(AXW(RB_SHELLS + 7));                /* FLAG CLEARED (BIT D7) */
l20:
    TXA();
    SEC();
    SBC(RB_L_SHL);
    TAX();
    if (BCS) goto l10;
    return;
}

/* DSPSHL (6937) */
void rb_dspshl(rb_machine *m)
{
    LDX(0);
    if (BCC) goto l5;
    DEX();
l5:
    STX(RB_FLAG);
    rb_pftrig(m);                           /* SET SINE/COSINE */
    LDX(RB_N_SHL);
l10:
    LDA(RD(AX(RB_SHELLS + 7)));
    if (BPL) goto l20;                      /* NOT ACTIVE */
    STX(RB_SAVX);
    LDA(RD(AX(RB_SHELLS + 5)));
    ASLA();
    ASLA();
    ASLA();
    EOR(0xF0);
    AND(0xF0);
    TAY();                                  /* DEPTH DEFINES INTENSITY */
    LDA(4);
    rb_vgstat(m);
    LDY(RD(RB_SAVX));
    LDX(0);
l15:
    LDA(RD(AY(RB_SHELLS)));
    STA(ZX(RB_OBJECT));
    INY();
    INX();
    CPX(6);
    if (BCC) goto l15;
    rb_positp(m);                           /* CALCULATE POSITION */
    BIT(RD(RB_FLAG));
    if (BMI) goto l16;
    rb_setbm(m);                            /* SET BEAM POSITION */
    if (BCS) goto l17;                      /* BEAM OFF SCREEN */
    LDA(0x20);
    rb_vgdot(m);                            /* DISPLAY DOT (Z REG) */
l16:
    LDX(RD(RB_SAVX));
    LDA(RD(AX(RB_SHELLS + 7)));
    ASLA();
    if (BMI) goto l17;                      /* ENEMY SHELL */
    rb_shcdck(m);                           /* CHECK FOR SHELL COLLISIONS */
l17:
    LDX(RD(RB_SAVX));
l20:
    TXA();
    SEC();
    SBC(RB_L_SHL);                          /* DO NEXT SHELL */
    TAX();
    if (BCS) goto l10;
    return;
}

