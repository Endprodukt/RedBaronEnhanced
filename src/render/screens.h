/* screens.h - the port's own title, high score table and initials entry,
 * and a free play that lets the attract mode run (settings `screens`,
 * `coin_mode = free`).
 *
 * Nothing of the game changes: these are other pictures of the game's own
 * states. Its pages are known by GAMODE (RBARON.MAC):
 *
 *   00010011  the attract banner: the plane towing RED BARON   -> the title
 *   0X000011  the high score table                             -> the table
 *   XX000111  the initials after a game                        -> the entry
 *
 * On these pages the game's own lines of the page (tag STATES, and the
 * banner's plane) are left out and the port's drawn instead; what it shows
 * besides (the score line on top, INSERT COINS and what a coin buys,
 * CREDITS, the copyright) stays, INSERT COINS and its line moved down out of
 * the way. The entry reads what the game keeps (LETR: the place in the table
 * and the letter being chosen; INITL: the letters; TIMER: the time left);
 * the stick and fire work as on the cabinet.
 *
 * Everything is drawn as lines in the screen's units (render/glyph.h for
 * the lettering), so the occlusion, the colours and rbshot's pictures work
 * as for the game's own lines.
 */
#ifndef RB_SCREENS_H
#define RB_SCREENS_H

#include "avg/avg.h"

#include <stdint.h>

enum { RB_PAGE_GAME = 0, RB_PAGE_TITLE, RB_PAGE_SCORES, RB_PAGE_ENTRY };

typedef struct rb_screens {
    int    page;            /* the page of the last picture */
    double t0;              /* when it began (the game's clock) */
    double elapsed;         /* >= 0: how long the page has shown, instead (rbshot) */
} rb_screens;

typedef struct rb_screens_opt {
    int screens;            /* the title, the table and the entry */
    int free_play;          /* FREE PLAY, PRESS START instead of INSERT COINS and the credits */
    int language;           /* 0 English, 1 Spanish, 2 French, 3 German (the game's own) */
    int cockpit;            /* the cockpit covers the bottom (render/cockpit.h): the texts
                             * there (CREDITS, free play's) go up, under INSERT COINS */
} rb_screens_opt;

void rb_screens_init(rb_screens *s);

/* The page the game shows (RB_PAGE_xxx), from its memory. */
int rb_screens_page(const uint8_t *mem);

/* The picture's lines (in, n) as they are to be shown: the game's lines of
 * a page left out or moved, the port's added. mem: the machine's memory (the
 * game's state); t: the game's clock in seconds (the animations). Writes
 * out (at most max); returns how many. */
int rb_screens_apply(rb_screens *s, const uint8_t *mem, double t, const rb_screens_opt *o, const rb_line *in, int n,
                     rb_line *out, int max);

/* The iron cross of the title as lines (also for the table): centre x, y,
 * arm length r, turned by angle (radians). Returns the lines written. */
int rb_screens_cross(rb_line *out, int max, float x, float y, float r, float angle, uint8_t intensity);

#endif
