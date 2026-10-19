/* tags.h - what a line of the picture is.
 *
 * The drawing routines of the game are called with a tag (tools/mac2c.py,
 * TAG_AT: RB_TAGGED around the call); the machine keeps the tag next to
 * every word of vector RAM written meanwhile (rb_machine.vgtag), and the
 * vector generator hands it on with each line (rb_line.tag). Colour
 * (render/color.h) and occlusion (render/solid.h) go by it. Lines of the
 * vector ROM's own pictures (characters, the player's propeller) carry the
 * tag of the instruction that called them.
 */
#ifndef RB_TAGS_H
#define RB_TAGS_H

enum {
    RB_TAG_NONE = 0,        /* not tagged: the beam's moves, what is not drawn by a routine below */
    RB_TAG_COCKPIT,         /* the player's own plane: the propeller in front, the guns (NOREST) */
    RB_TAG_LAND,            /* SCENE, where it draws itself */
    RB_TAG_HORIZON,         /* the horizon between the mountains */
    RB_TAG_MOUNTAIN,        /* the mountains */
    RB_TAG_GROUND,          /* what stands on the ground (ground sequence) */
    RB_TAG_GSHELL,          /* the guns on the ground and their shells */
    RB_TAG_PLANE,           /* the enemy plane */
    RB_TAG_PROP,            /* its propeller */
    RB_TAG_DRONE,           /* the drones */
    RB_TAG_EXPLOSION,       /* a plane or a drone in pieces */
    RB_TAG_BLIMP,           /* the blimp */
    RB_TAG_SHELL,           /* shells and bullets in the air */
    RB_TAG_COPYRIGHT,
    RB_TAG_LIVES,           /* the planes left, top left */
    RB_TAG_EOL,             /* the end of a life: bullet holes, the crash, the stars */
    RB_TAG_STATES,          /* the high score pages, the initials, the banner */
    RB_TAG_TEXT,            /* ALPHAS, where it draws itself */
    RB_TAG_SCORE,           /* the player's score */
    RB_TAG_VALUE,           /* what the plane in sight is worth */
    RB_TAG_HISCORE,         /* the best score and its initials */
    RB_TAG_LABEL,           /* the words SCORE, VALUE, HIGH SCORE */
    RB_TAG_WARN,            /* the guns are overheated */
    RB_TAG_CREDITS,         /* CREDITS and their number */
    RB_TAG_ATTRACT,         /* what a coin buys, two plays minimum */
    RB_TAG_INSERT,          /* INSERT COINS, PRESS START, FREE PLAY */
    RB_TAG_BONUS,           /* BONUS LIFE, 300 POINTS, BEHIND YOU */
    RB_TAG_DECO,            /* the additions: trees, clouds (render/deco.h) */
    /* the port's own screens (render/screens.h) */
    RB_TAG_LOGO_CROSS,      /* the iron cross of the title */
    RB_TAG_LOGO_RIM,        /* its rim, the circle round it */
    RB_TAG_LOGO_EAGLE,      /* the eagle's head */
    RB_TAG_LOGO_RED,        /* the stripes: red */
    RB_TAG_LOGO_GOLD,       /*   gold */
    RB_TAG_LOGO_TEXT,       /* the lettering "Red Baron" */
    RB_TAG_SCREEN_HEAD,     /* headings: HIGH SCORES, NEW HIGH SCORE */
    RB_TAG_SCREEN_TEXT,     /* the table, hints */
    RB_TAG_SCREEN_HI,       /* what is chosen or new */
    RB_TAG_FREE,            /* FREE PLAY, PRESS START */
    /* the cockpit (render/cockpit.h) */
    RB_TAG_GUN,             /* the two machine guns */
    RB_TAG_DASH,            /* the dashboard */
    RB_TAG_STRUT,           /* the struts to the upper wing */
    RB_TAG_NOSE,            /* the engine's cowling */
    RB_TAG_PROPELLER,       /* the player's propeller (the cockpit's) */
    RB_TAG_WIRE,            /* the bracing wires */
    RB_TAG_WINDSCREEN,      /* the small windscreen */
    RB_TAG_COAMING,         /* the cockpit's padded rim (not drawn any more) */
    RB_TAG_GUNHOT,          /* the guns' jackets, hot */
    RB_TAG_BLAST,           /* explosions (render/fx.h): the flash, */
    RB_TAG_FIRE,            /*   the fire, */
    RB_TAG_EMBER,           /*   the embers */
    RB_TAG_ROUNDEL_BLUE,    /* the enemy's roundels (render/fx.h): the blue ring, */
    RB_TAG_ROUNDEL_RED,     /*   the red middle */
    RB_TAG_GAUGE,           /* the instruments' faces */
    RB_TAG_NEEDLE,          /* their needles */
    RB_TAG_FLASH,           /* the guns' muzzle flash */
    RB_TAG_COUNT
};

/* The objects whose points DECODE notes (rb_machine.note_obj, and the
 * model of a line: rb_line.model). */
enum {
    RB_OBJ_NONE = 0,
    RB_OBJ_PLANE = 1,       /* a whole biplane (PLDECD: DB.MAP and DB.LNS) */
    RB_OBJ_PIECES = 2,      /* the pieces of one (PLNEXP) */
    RB_OBJ_MOUNTAIN = 3,    /* a mountain (PFDSPL: four pieces of landscape) */
    RB_OBJ_GROUND = 4,      /* 4..7: an object on the ground, by its type (PFLOB): */
    RB_OBJ_TENT = 4,        /*   a tent */
    RB_OBJ_TREE = 5,        /*   a tree */
    RB_OBJ_GUN = 6,         /*   a gun */
    RB_OBJ_BUNKER = 7,      /*   a bunker */
    RB_OBJ_DECO_TREE = 8,   /* the additions (render/deco.h): trees on the ground */
    RB_OBJ_DECO_FIR = 9,    /*   firs */
    RB_OBJ_CLOUD = 10,      /*   clouds */
    RB_OBJ_BLIMP = 11,      /* the blimp, its lines' part in p0 (render/color.h: rb_color_mark) */
    RB_OBJ_MOUNTAIN_SEG = 32,   /* 32..35: a mountain drawn, by its piece of landscape (SSEGS) */
};

#endif
