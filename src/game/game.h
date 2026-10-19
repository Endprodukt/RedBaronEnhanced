/* game.h - the routines of the Red Baron program (written by tools/mac2c.py).
 *
 * rb_xxx is the routine XXX of the source; the address is where it stands in
 * the ROMs. The registers go in and out through m->r, as on the 6502.
 */
#ifndef RB_GAME_H
#define RB_GAME_H

#include "core/machine.h"


/* main.c */
void rb_start(rb_machine *m);                  /* 4804 */
void rb_main(rb_machine *m);                   /* 4835 */
void rb_plprop(rb_machine *m);                 /* 493E */
void rb_setdiv(rb_machine *m);                 /* 49A8 */
void rb_credit(rb_machine *m);                 /* 49B6 */
void rb_sounds(rb_machine *m);                 /* 49FD */
void rb_pfsclr(rb_machine *m);                 /* 4A6A */
void rb_pfsoff(rb_machine *m);                 /* 4A6F */
void rb_pfsx(rb_machine *m);                   /* 4A77 */
void rb_eolseq(rb_machine *m);                 /* 4A78 */
void rb_gminit(rb_machine *m);                 /* 4BB5 */
void rb_initial(rb_machine *m);                /* 4BDE */
void rb_plcrsh(rb_machine *m);                 /* 4C71 */
void rb_initgr(rb_machine *m);                 /* 4D2B */
void rb_cintbp(rb_machine *m);                 /* 4D37 */
void rb_initbp(rb_machine *m);                 /* 4D3B */
void rb_intbp0(rb_machine *m);                 /* 4D84 */
void rb_plnclr(rb_machine *m);                 /* 4D85 */
void rb_spray(rb_machine *m);                  /* 4DA4 */
void rb_wndshd(rb_machine *m);                 /* 4DB5 */
void rb_dsplif(rb_machine *m);                 /* 4DD0 */
void rb_dlives(rb_machine *m);                 /* 4DF5 */
void rb_scorem(rb_machine *m);                 /* 4E06 */
void rb_percent(rb_machine *m);                /* 4E9C */

/* states.c */
void rb_states(rb_machine *m);                 /* 4ED9 */
void rb_nw_hsc(rb_machine *m);                 /* 4FEB */
void rb_div32(rb_machine *m);                  /* 513E */
void rb_letchk(rb_machine *m);                 /* 5153 */
void rb_hstbl(rb_machine *m);                  /* 5178 */
void rb_hsintl(rb_machine *m);                 /* 51E2 */
void rb_dspalp(rb_machine *m);                 /* 5201 */
void rb_nwbnnr(rb_machine *m);                 /* 520A */
void rb_dspbnr(rb_machine *m);                 /* 5231 */

/* objects.c */
void rb_newshl(rb_machine *m);                 /* 526E */
void rb_nwshl0(rb_machine *m);                 /* 5281 */
void rb_nwshl1(rb_machine *m);                 /* 5290 */
void rb_setshl(rb_machine *m);                 /* 529C */
void rb_nwplne(rb_machine *m);                 /* 5314 */
void rb_stplne(rb_machine *m);                 /* 5354 */
void rb_plnzd(rb_machine *m);                  /* 543F */
void rb_plnzd0(rb_machine *m);                 /* 5445 */
void rb_plnznd(rb_machine *m);                 /* 549A */
void rb_lvlstp(rb_machine *m);                 /* 54C5 */
void rb_nwenme(rb_machine *m);                 /* 54F6 */
void rb_plmotn(rb_machine *m);                 /* 5534 */
void rb_updpln(rb_machine *m);                 /* 5580 */
void rb_updmob(rb_machine *m);                 /* 55CE */
void rb_plonsn(rb_machine *m);                 /* 57C1 */
void rb_upplex(rb_machine *m);                 /* 5870 */
void rb_upplx0(rb_machine *m);                 /* 58B5 */
void rb_upplx1(rb_machine *m);                 /* 58C3 */
void rb_plnscr(rb_machine *m);                 /* 58FF */
void rb_drnscr(rb_machine *m);                 /* 5907 */
void rb_nwscre(rb_machine *m);                 /* 590E */
void rb_updrne(rb_machine *m);                 /* 5919 */
void rb_plnxcg(rb_machine *m);                 /* 59BA */
void rb_pfmotn(rb_machine *m);                 /* 59CA */
void rb_dischk(rb_machine *m);                 /* 5C44 */
void rb_frepar(rb_machine *m);                 /* 5C73 */
void rb_frdrne(rb_machine *m);                 /* 5C80 */
void rb_formtn(rb_machine *m);                 /* 5CA3 */

/* ground.c */
void rb_grdisp(rb_machine *m);                 /* 5CC2 */
void rb_grsmtn(rb_machine *m);                 /* 5E18 */
void rb_grcoln(rb_machine *m);                 /* 5EFD */
void rb_plycol(rb_machine *m);                 /* 5FFE */
void rb_pcolx(rb_machine *m);                  /* 604C */
void rb_pcolny(rb_machine *m);                 /* 6063 */
void rb_shlaun(rb_machine *m);                 /* 6074 */
void rb_shlaux(rb_machine *m);                 /* 607C */
void rb_shlau0(rb_machine *m);                 /* 607D */
void rb_grsran(rb_machine *m);                 /* 612A */
void rb_blmotn(rb_machine *m);                 /* 6193 */
void rb_blexpl(rb_machine *m);                 /* 6292 */
void rb_wrapit(rb_machine *m);                 /* 6303 */
void rb_dspgrs(rb_machine *m);                 /* 631A */

/* display.c */
void rb_scene(rb_machine *m);                  /* 634E */
void rb_scene0(rb_machine *m);                 /* 646C */
void rb_scene1(rb_machine *m);                 /* 648B */
void rb_scene2(rb_machine *m);                 /* 64C3 */
void rb_pfdspl(rb_machine *m);                 /* 64D3 */
void rb_pfxpos(rb_machine *m);                 /* 6576 */
void rb_motobj(rb_machine *m);                 /* 65D7 */
void rb_plnshl(rb_machine *m);                 /* 665E */
void rb_plnlbs(rb_machine *m);                 /* 6671 */
void rb_plnlb0(rb_machine *m);                 /* 66A0 */
void rb_dsppln(rb_machine *m);                 /* 66C1 */
void rb_dspdrn(rb_machine *m);                 /* 66CE */
void rb_plane(rb_machine *m);                  /* 66DB */
void rb_drone(rb_machine *m);                  /* 673A */
void rb_drnpic(rb_machine *m);                 /* 676B */
void rb_pltest(rb_machine *m);                 /* 6798 */
void rb_pldecd(rb_machine *m);                 /* 67EA */
void rb_plnprp(rb_machine *m);                 /* 6809 */
void rb_plnexp(rb_machine *m);                 /* 683A */
void rb_drnexp(rb_machine *m);                 /* 68B3 */
void rb_shlmot(rb_machine *m);                 /* 68FA */
void rb_pstshl(rb_machine *m);                 /* 6919 */
void rb_dspshl(rb_machine *m);                 /* 6937 */

/* alpha.c */
void rb_alphas(rb_machine *m);                 /* 698F */
void rb_alph_0(rb_machine *m);                 /* 6B0E */
void rb_drntst(rb_machine *m);                 /* 6B15 */

/* collide.c */
void rb_cdsset(rb_machine *m);                 /* 6B47 */
void rb_shcdck(rb_machine *m);                 /* 6BCE */
void rb_minmax(rb_machine *m);                 /* 6CF7 */
void rb_mnmx0(rb_machine *m);                  /* 6CF9 */
void rb_colstp(rb_machine *m);                 /* 6D62 */
void rb_potscl(rb_machine *m);                 /* 6DC5 */
void rb_strgme(rb_machine *m);                 /* 6E81 */

/* math.c */
void rb_pftrig(rb_machine *m);                 /* 6EC7 */
void rb_trig(rb_machine *m);                   /* 6ECB */
void rb_t_sine(rb_machine *m);                 /* 6F09 */
void rb_multpl(rb_machine *m);                 /* 6F3F */
void rb_trns0(rb_machine *m);                  /* 6F6F */
void rb_mrsab0(rb_machine *m);                 /* 6F7A */
void rb_mrslt0(rb_machine *m);                 /* 6F7D */
void rb_mrsult(rb_machine *m);                 /* 6F80 */
void rb_ddivit(rb_machine *m);                 /* 6FA2 */
void rb_ddiv10(rb_machine *m);                 /* 6FA8 */
void rb_dpabs(rb_machine *m);                  /* 6FB8 */
void rb_d_comp(rb_machine *m);                 /* 6FBA */
void rb_dpcmp0(rb_machine *m);                 /* 6FC6 */
void rb_spabs(rb_machine *m);                  /* 6FC8 */
void rb_comp(rb_machine *m);                   /* 6FCA */
void rb_comp_0(rb_machine *m);                 /* 6FCF */
void rb_divby4(rb_machine *m);                 /* 6FD0 */
void rb_divby2(rb_machine *m);                 /* 6FD5 */
void rb_multb4(rb_machine *m);                 /* 6FDB */
void rb_random(rb_machine *m);                 /* 6FE2 */
void rb_rand2(rb_machine *m);                  /* 6FE5 */
void rb_rand(rb_machine *m);                   /* 6FE8 */

/* coin.c */
void rb_moolah(rb_machine *m);                 /* 70B7 */
void rb_extb(rb_machine *m);                   /* 7174 */

/* sound.c */
void rb_sndon(rb_machine *m);                  /* 7259 */
void rb_modsnd(rb_machine *m);                 /* 7278 */

/* grnd.c */
void rb_nmi(rb_machine *m);                    /* 72C8 */
void rb_positp(rb_machine *m);                 /* 73DC */
void rb_posith(rb_machine *m);                 /* 73EE */
void rb_setbm(rb_machine *m);                  /* 747A */
void rb_placbm(rb_machine *m);                 /* 748D */
void rb_plac_0(rb_machine *m);                 /* 749A */
void rb_setgrs(rb_machine *m);                 /* 749B */
void rb_project(rb_machine *m);                /* 74B3 */
void rb_plnabz(rb_machine *m);                 /* 755F */
void rb_yaxis(rb_machine *m);                  /* 75E0 */
void rb_xaxis(rb_machine *m);                  /* 762A */
void rb_d_loop(rb_machine *m);                 /* 7674 */
void rb_dcmpa0(rb_machine *m);                 /* 76CD */
void rb_decmap(rb_machine *m);                 /* 76D7 */

/* vgut.c */
void rb_vgrtsl(rb_machine *m);                 /* 7856 */
void rb_vghalt(rb_machine *m);                 /* 785A */
void rb_vghal1(rb_machine *m);                 /* 785F */
void rb_vghexz(rb_machine *m);                 /* 7866 */
void rb_vgjmp1(rb_machine *m);                 /* 788B */
void rb_vgjsrl(rb_machine *m);                 /* 7897 */
void rb_vgstat(rb_machine *m);                 /* 78A0 */
void rb_vgcntr(rb_machine *m);                 /* 78A7 */
void rb_vgadd2(rb_machine *m);                 /* 78AB */
void rb_vgadd(rb_machine *m);                  /* 78B3 */
void rb_vgsca1(rb_machine *m);                 /* 78BE */
void rb_vgdot(rb_machine *m);                  /* 78C7 */
void rb_vgvtr(rb_machine *m);                  /* 78CB */
void rb_vgvtr1(rb_machine *m);                 /* 78CD */
void rb_vgvtr2(rb_machine *m);                 /* 78E8 */
void rb_vgvctr(rb_machine *m);                 /* 78EA */
void rb_vgwai1(rb_machine *m);                 /* 7904 */

/* earom.c */
void rb_eaupd(rb_machine *m);                  /* 7909 */
void rb_stearom(rb_machine *m);                /* 79D0 */

/* int.c */
void rb_message(rb_machine *m);                /* 7A02 */
void rb_loadram(rb_machine *m);                /* 7A66 */
void rb_digt1s(rb_machine *m);                 /* 7A7F */
void rb_digtys(rb_machine *m);                 /* 7A81 */
void rb_pwron(rb_machine *m);                  /* 7AA5 */
void rb_decimal(rb_machine *m);                /* 7AED */

/* cut out of a routine */
void rb_main_view(rb_machine *m);              /* 4875 */
void rb_main2(rb_machine *m);                  /* 4908 */

#include "game/frame.h"

#endif
