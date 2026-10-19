/* glyph.h - the game's own lettering, as lines to place freely.
 *
 * Red Baron draws its letters and digits with vector ROM routines, one per
 * character (VGMSGA: the table of their JSRLs, two bytes each: blank, 0-9,
 * A-Z, and a few marks). For the screens the port adds (render/screens.h,
 * render/cockpit.h) the routines are run once through the vector generator
 * (avg) and kept as lines, so that text can be put anywhere, at any size,
 * slanted or moved letter by letter, and still look like the game's.
 *
 * Sizes are the height of a capital in the screen's units (the picture is
 * -260..260 by -200..200 of them; the game's own score is about 12 high).
 */
#ifndef RB_GLYPH_H
#define RB_GLYPH_H

#include "avg/avg.h"

#include <stdint.h>

enum { RB_TEXT_LEFT = 0, RB_TEXT_CENTER, RB_TEXT_RIGHT };

/* Take the characters from the ROMs in mem (the machine's memory). Once;
 * again only if the ROMs change. */
void rb_glyph_init(const uint8_t *mem);
int  rb_glyph_ready(void);

/* The game's code of an ASCII character (VGMSGA offset; 0 for a blank or one
 * it does not have), and back. */
int  rb_glyph_code(char c);
char rb_glyph_char(int code);

/* The width of s at a size (from the start of the first character to the
 * beam's place after the last). */
float rb_text_width(const char *s, float size);

/* s as lines, its baseline at y; x is its left end, middle or right end
 * (align). slant: x moved by slant * height (0: upright). Returns the lines
 * written (at most max). */
int rb_text(rb_line *out, int max, const char *s, float x, float y, float size, int align, uint8_t intensity,
            uint16_t tag, float slant);

/* One character by its game code at x, y (its start on the baseline),
 * scaled by sx, sy. Returns the lines written. */
int rb_glyph(rb_line *out, int max, int code, float x, float y, float sx, float sy, uint8_t intensity, uint16_t tag,
             float slant);

#endif
