/* menu.h - Red Baron Enhanced's own test menu (after Battlezone Enhanced's).
 *
 * The cabinet's service key opens it (advmame.rc's "service", F2 if it has
 * none: platform/controls.h), or start and fire held two seconds; the game
 * stands still meanwhile. Pages:
 *
 * The basics only (the rest is in redbaron.ini). Pages:
 *
 *   CONTROLS        an analog or a digital stick and its sensitivity (digital:
 *                   how fast the held stick moves out); what every action is
 *                   bound to (fire on a line, then press the key or button or
 *                   push the stick that is to do it); up and down reversed;
 *                   beside it the test: the stick as the game gets it, the
 *                   analog axes, the pots, fire, start, coin
 *   PICTURE         smooth motion, colours, solid objects, trees, clouds,
 *                   the cockpit, the title and high score pages, brightness
 *   TEST PICTURE    a crosshatch
 *   SOUND           the volume, and a test of the board's sounds
 *   GAME OPTIONS    planes, bonus, language, coins
 *   the high scores back to none
 *
 *   up / down       choose a line
 *   left / right    change its value (a binding: back to the cabinet's own)
 *   fire            change it, open a page, or do what the line says
 *   start           back from a test page
 *   service key     save the settings and go back to the game
 */
#ifndef RB_MENU_H
#define RB_MENU_H

#include "core/machine.h"
#include "platform/controls.h"
#include "render/config.h"

#include <vectorpie/vp.h>

enum {
    RB_MENU_NOTHING = 0,
    RB_MENU_CHANGED,        /* a setting changed: take it over */
    RB_MENU_RESET_SCORES,   /* the high scores back to none */
    RB_MENU_SOUND,          /* play the board's sound rb_menu.sound */
    RB_MENU_CLOSE,          /* leave */
};

/* The sounds of the sound test. */
enum { RB_TEST_GUN = 0, RB_TEST_EXPLOSION, RB_TEST_DIVE, RB_TEST_ENGINE, RB_TEST_CLOSING, RB_TEST_TONE, RB_TEST_COUNT };

typedef struct rb_menu {
    int    open;
    int    page;
    int    item[16];        /* the line chosen on each page */
    int    sound;           /* the sound test's choice (RB_TEST_xxx) */
    int    capture;         /* the action a binding is being taken for, or -1 */
    int    confirm;         /* fire once more resets the high scores */
    int    dir_x, dir_y;    /* where the controls pointed the last time */
    int    settle;          /* after a binding: nothing counts until the stick is let go */
    double repeat_at;       /* when a held direction counts again (seconds) */
} rb_menu;

void rb_menu_open(rb_menu *u);

/* Once a picture while the menu is open, after rb_controls_read, at the
 * time now (seconds). Returns what is to be done (RB_MENU_xxx). */
int  rb_menu_update(rb_menu *u, rb_config *cfg, rb_controls *ctl, double now);
/* m: the machine (the control test shows what the game reads). */
void rb_menu_draw(const rb_menu *u, const rb_config *cfg, const rb_controls *ctl, const rb_machine *m, vp_dvg *d);

#endif
