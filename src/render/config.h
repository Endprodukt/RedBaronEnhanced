/* config.h - the program's settings (redbaron.ini: "name = value" lines,
 * '#' or ';' starts a comment). The package's file gives the defaults, the
 * one in the save folder the owner's own; the test menu (platform/menu.h)
 * changes them and writes the owner's file. */
#ifndef RB_CONFIG_H
#define RB_CONFIG_H

#include <stdint.h>

/* What can be bound to a control (platform/controls.h). */
enum {
    RB_ACT_UP = 0, RB_ACT_DOWN, RB_ACT_LEFT, RB_ACT_RIGHT,     /* the stick as switches */
    RB_ACT_STICK_X, RB_ACT_STICK_Y,                             /* the stick's (or yoke's) two axes */
    RB_ACT_FIRE, RB_ACT_START, RB_ACT_COIN, RB_ACT_MENU,
    RB_ACT_COUNT
};
#define RB_BIND_SIZE 40

/* How the plane is flown. */
enum { RB_INPUT_DIGITAL = 0, RB_INPUT_ANALOG };

typedef struct rb_config {
    char    rom_path[512];      /* a folder or zip to look for the ROMs in first */

    /* the picture: what of the vector generator's screen is shown (its
     * units, the middle at 0,0), and where on the board (0..4095) */
    float   view_left, view_right, view_bottom, view_top;
    int     margin_left, margin_right, margin_bottom, margin_top;
    int     flip_x, flip_y;
    float   brightness;         /* 1: as it is */
    float   gamma;              /* of the 15 intensities */
    float   floor;              /* the dimmest line's share of full brightness */
    uint8_t color_cabinet[3];   /* the cabinet's tube is white behind a blue sheet ("color") */

    float   volume;             /* 0..1 */
    int     samples;            /* 1: the sounds as recordings (sound/samples.h); 0: the board's circuits */
    float   engine_volume;      /* the recorded engine's loudness, 0..1 */

    /* additions */
    int     smooth;             /* 3: pictures between the frames, computed from the game's state
                                 * (render/subframe.h); 0: each frame's picture as on the cabinet */
    int     color;              /* 1: a colour for every line (render/color.h); 0: the cabinet's one */
    int     occlusion;          /* 1: what is nearer hides what is behind it (render/solid.h) */
    int     trees, clouds;      /* the additions to the world (render/deco.h) */
    int     stick_calibrate;    /* the program's own calibration of the stick set at power on
                                 * (rb_stick_calibrate) instead of learnt from the player's moves */
    int     screens;            /* the port's title, high score table and initials entry (render/screens.h) */
    int     cockpit;            /* the bezel's cockpit drawn: the guns, the instruments (render/cockpit.h) */
    int     default_scores;     /* an empty high score table filled with the aces of 1917 (main.c) */
    int     attract_demo;       /* real gameplay in the attract mode, by an autopilot (render/demo.h) */
    int     explosions;         /* a fireball and sparks where things blow up (render/fx.h) */

    /* the cabinet's option switches, as the program reads them */
    int     language;           /* 0 English, 1 Spanish, 2 French, 3 German */
    int     bonus;              /* 0 none, 1: 6000/20000/50000, 2: 4000/15000/40000, 3: 2000/10000/30000 */
    int     lives;              /* 2..5 */
    int     two_play_minimum;
    int     self_adjust;        /* the game adjusts its difficulty to the average game time */
    int     coin_mode;          /* 0 free play, 1: 1 coin 2 plays, 2: 1 coin 1 play, 3: 2 coins 1 play.
                                 * Free play is the port's: the attract mode runs, start gives the
                                 * game its coin (main.c); the board's own free play (it stands at
                                 * PRESS START for ever) is not used */

    /* the controls (platform/controls.h) */
    int     input_mode;         /* RB_INPUT_xxx */
    char    bind[RB_ACT_COUNT][RB_BIND_SIZE];   /* "auto", "none", "key Up", "joy1 button 0", ... */
    float   digital_rate;       /* a digital stick: full deflection reached in 1/rate seconds */
    float   digital_reach;      /* how far it goes held: 0.2..1 of the stick's travel */
    float   digital_return;     /* let go: back to the middle in 1/return seconds (0: at once) */
    float   deadzone;           /* an analog stick: the middle that counts as nothing, 0..0.5 */
    float   gain;               /* an analog stick: how far a push goes, 0.5..2 */
    int     invert_y;           /* pull to climb (1) or push to climb (0, as the cabinet) */
    int     menu_hold;          /* start and fire held for two seconds open the test menu too */
} rb_config;

void rb_config_defaults(rb_config *c);
/* Reads what the file has over what c holds. 0, or -1 if there is no file. */
int  rb_config_load(rb_config *c, const char *path);
/* Writes all of it. 0, or -1. */
int  rb_config_save(const rb_config *c, const char *path);

/* The option switch banks at 0A00 and 0C00 (as the board presents them:
 * the program inverts them). */
uint8_t rb_config_opt0(const rb_config *c);
uint8_t rb_config_opt1(const rb_config *c);

/* The names of the actions in the file (bind_<name>). */
extern const char *const RB_ACTION_NAME[RB_ACT_COUNT];

#endif
