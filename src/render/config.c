#include "render/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const RB_ACTION_NAME[RB_ACT_COUNT] = {
    "up", "down", "left", "right", "stick_x", "stick_y", "fire", "start", "coin", "menu",
};
static const char *const LANGUAGE_NAME[4] = { "english", "spanish", "french", "german" };
static const char *const COIN_NAME[4] = { "free", "2plays", "1play", "2coins" };
static const char *const INPUT_NAME[2] = { "digital", "analog" };

void rb_config_defaults(rb_config *c)
{
    memset(c, 0, sizeof *c);
    /* (MAME shows 0..520 by 0..400 of the generator's screen for this game:
     * here with the middle at 0,0) */
    c->view_left = -260.0f; c->view_right = 260.0f;
    c->view_bottom = -200.0f; c->view_top = 200.0f;
    c->brightness = 1.0f;
    c->gamma = 0.6f;
    c->floor = 0.15f;
    c->color_cabinet[0] = 110; c->color_cabinet[1] = 190; c->color_cabinet[2] = 255;
    c->volume = 0.8f;
    c->samples = 1;
    c->engine_volume = 0.7f;
    c->smooth = 3;
    c->color = 1;
    c->occlusion = 1;
    c->trees = 1;
    c->clouds = 1;
    c->stick_calibrate = 1;
    c->screens = 1;
    c->cockpit = 1;
    c->default_scores = 1;
    c->attract_demo = 1;
    c->explosions = 1;
    c->language = 0;
    c->bonus = 1;
    c->lives = 3;
    c->coin_mode = 2;
    c->input_mode = RB_INPUT_ANALOG;    /* (the cabinet's stick; keys and switches still work) */
    for (int i = 0; i < RB_ACT_COUNT; i++) snprintf(c->bind[i], RB_BIND_SIZE, "auto");
    c->digital_rate = 4.0f;
    c->digital_reach = 1.0f;
    c->digital_return = 6.0f;
    c->deadzone = 0.08f;
    c->gain = 1.0f;
    c->menu_hold = 1;
}

static int truth(const char *v)
{
    return !strcmp(v, "1") || !strcmp(v, "yes") || !strcmp(v, "on") || !strcmp(v, "true");
}

static int name_index(const char *const *names, int n, const char *v, int fallback)
{
    for (int i = 0; i < n; i++)
        if (!strcmp(names[i], v)) return i;
    return fallback;
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

int rb_config_load(rb_config *c, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *hash = strpbrk(line, "#;");
        if (hash) *hash = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *k = line, *v = eq + 1;
        while (*k == ' ' || *k == '\t') k++;
        char *e = k + strlen(k);
        while (e > k && (e[-1] == ' ' || e[-1] == '\t')) *--e = '\0';
        while (*v == ' ' || *v == '\t') v++;
        e = v + strlen(v);
        while (e > v && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) *--e = '\0';

        if (!strcmp(k, "rom_path")) snprintf(c->rom_path, sizeof c->rom_path, "%s", v);
        else if (!strcmp(k, "view_left")) c->view_left = (float)atof(v);
        else if (!strcmp(k, "view_right")) c->view_right = (float)atof(v);
        else if (!strcmp(k, "view_bottom")) c->view_bottom = (float)atof(v);
        else if (!strcmp(k, "view_top")) c->view_top = (float)atof(v);
        else if (!strcmp(k, "margin_left")) c->margin_left = atoi(v);
        else if (!strcmp(k, "margin_right")) c->margin_right = atoi(v);
        else if (!strcmp(k, "margin_bottom")) c->margin_bottom = atoi(v);
        else if (!strcmp(k, "margin_top")) c->margin_top = atoi(v);
        else if (!strcmp(k, "flip_x")) c->flip_x = truth(v);
        else if (!strcmp(k, "flip_y")) c->flip_y = truth(v);
        else if (!strcmp(k, "brightness")) c->brightness = (float)atof(v);
        else if (!strcmp(k, "gamma")) c->gamma = (float)atof(v);
        else if (!strcmp(k, "floor")) c->floor = (float)atof(v);
        else if (!strcmp(k, "color")) {
            int r, g, b;
            if (sscanf(v, "%d %d %d", &r, &g, &b) == 3) {
                c->color_cabinet[0] = (uint8_t)r; c->color_cabinet[1] = (uint8_t)g; c->color_cabinet[2] = (uint8_t)b;
            }
        }
        else if (!strcmp(k, "volume")) c->volume = (float)atof(v);
        else if (!strcmp(k, "sounds")) c->samples = strcmp(v, "original") != 0 && strcmp(v, "0") != 0;
        else if (!strcmp(k, "engine_volume")) c->engine_volume = (float)atof(v);
        else if (!strcmp(k, "smooth")) c->smooth = truth(v) ? 3 : atoi(v) == 3 ? 3 : 0;
        else if (!strcmp(k, "stick_calibrate")) c->stick_calibrate = truth(v);
        else if (!strcmp(k, "color_lines")) c->color = truth(v);
        else if (!strcmp(k, "occlusion")) c->occlusion = truth(v);
        else if (!strcmp(k, "trees")) c->trees = truth(v);
        else if (!strcmp(k, "clouds")) c->clouds = truth(v);
        else if (!strcmp(k, "screens")) c->screens = truth(v);
        else if (!strcmp(k, "cockpit")) c->cockpit = truth(v);
        else if (!strcmp(k, "default_scores")) c->default_scores = truth(v);
        else if (!strcmp(k, "attract_demo")) c->attract_demo = truth(v);
        else if (!strcmp(k, "explosions")) c->explosions = truth(v);
        else if (!strcmp(k, "language")) c->language = name_index(LANGUAGE_NAME, 4, v, 0);
        else if (!strcmp(k, "bonus")) c->bonus = atoi(v) & 3;
        else if (!strcmp(k, "lives")) c->lives = atoi(v);
        else if (!strcmp(k, "two_play_minimum")) c->two_play_minimum = truth(v);
        else if (!strcmp(k, "self_adjust")) c->self_adjust = truth(v);
        else if (!strcmp(k, "free_play")) { if (truth(v)) c->coin_mode = 0; }
        else if (!strcmp(k, "coin_mode")) c->coin_mode = name_index(COIN_NAME, 4, v, 2);
        else if (!strcmp(k, "controls")) c->input_mode = name_index(INPUT_NAME, 2, v, RB_INPUT_DIGITAL);
        else if (!strcmp(k, "stick_rate") || !strcmp(k, "digital_rate")) c->digital_rate = (float)atof(v);
        else if (!strcmp(k, "digital_reach")) c->digital_reach = (float)atof(v);
        else if (!strcmp(k, "digital_return")) c->digital_return = (float)atof(v);
        else if (!strcmp(k, "deadzone")) c->deadzone = (float)atof(v);
        else if (!strcmp(k, "gain")) c->gain = (float)atof(v);
        else if (!strcmp(k, "invert_y")) c->invert_y = truth(v);
        else if (!strcmp(k, "menu_hold")) c->menu_hold = truth(v);
        else if (!strncmp(k, "bind_", 5)) {
            for (int i = 0; i < RB_ACT_COUNT; i++)
                if (!strcmp(k + 5, RB_ACTION_NAME[i])) snprintf(c->bind[i], RB_BIND_SIZE, "%s", v);
        }
    }
    fclose(f);
    if (c->lives < 2) c->lives = 2;
    if (c->lives > 5) c->lives = 5;
    c->coin_mode &= 3;
    c->digital_rate = clampf(c->digital_rate, 0.5f, 20.0f);
    c->digital_reach = clampf(c->digital_reach, 0.2f, 1.0f);
    c->digital_return = clampf(c->digital_return, 0.0f, 20.0f);
    c->deadzone = clampf(c->deadzone, 0.0f, 0.5f);
    c->gain = clampf(c->gain, 0.5f, 2.0f);
    return 0;
}

int rb_config_save(const rb_config *c, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "# Red Baron Enhanced: the owner's settings (written by the test menu; see the README)\n");
    fprintf(f, "rom_path = %s\n", c->rom_path);
    fprintf(f, "language = %s\nbonus = %d\nlives = %d\ntwo_play_minimum = %d\nself_adjust = %d\ncoin_mode = %s\n",
            LANGUAGE_NAME[c->language & 3], c->bonus & 3, c->lives, c->two_play_minimum, c->self_adjust,
            COIN_NAME[c->coin_mode & 3]);
    fprintf(f, "controls = %s\n", INPUT_NAME[c->input_mode & 1]);
    for (int i = 0; i < RB_ACT_COUNT; i++) fprintf(f, "bind_%s = %s\n", RB_ACTION_NAME[i], c->bind[i]);
    fprintf(f, "digital_rate = %.2f\ndigital_reach = %.2f\ndigital_return = %.2f\ndeadzone = %.2f\ngain = %.2f\n",
            c->digital_rate, c->digital_reach, c->digital_return, c->deadzone, c->gain);
    fprintf(f, "invert_y = %d\nmenu_hold = %d\nstick_calibrate = %d\n", c->invert_y, c->menu_hold, c->stick_calibrate);
    fprintf(f, "smooth = %d\ncolor_lines = %d\nocclusion = %d\ntrees = %d\nclouds = %d\n", c->smooth, c->color,
            c->occlusion, c->trees, c->clouds);
    fprintf(f, "screens = %d\ncockpit = %d\ndefault_scores = %d\nattract_demo = %d\n", c->screens, c->cockpit,
            c->default_scores, c->attract_demo);
    fprintf(f, "explosions = %d\n", c->explosions);
    fprintf(f, "volume = %.2f\nsounds = %s\nengine_volume = %.2f\n", c->volume,
            c->samples ? "samples" : "original", c->engine_volume);
    fprintf(f, "color = %d %d %d\nbrightness = %.2f\ngamma = %.2f\nfloor = %.2f\n", c->color_cabinet[0],
            c->color_cabinet[1], c->color_cabinet[2], c->brightness, c->gamma, c->floor);
    fprintf(f, "view_left = %.1f\nview_right = %.1f\nview_bottom = %.1f\nview_top = %.1f\n",
            c->view_left, c->view_right, c->view_bottom, c->view_top);
    fprintf(f, "margin_left = %d\nmargin_right = %d\nmargin_bottom = %d\nmargin_top = %d\nflip_x = %d\nflip_y = %d\n",
            c->margin_left, c->margin_right, c->margin_bottom, c->margin_top, c->flip_x, c->flip_y);
    fclose(f);
    return 0;
}

uint8_t rb_config_opt0(const rb_config *c)
{
    /* D1,D0: the coin mode, inverted (CMODE is OPT0 inverted: 0 free play,
     * 1 two plays a coin, 2 one, 3 two coins a play); the rest: the
     * mechs' multipliers and bonus coins as shipped. Free play is the
     * port's (config.h): the board is set to one coin, one play. */
    int mode = c->coin_mode & 3;
    if (mode == 0) mode = 2;
    return (uint8_t)(0xfc | (~mode & 3));
}

uint8_t rb_config_opt1(const rb_config *c)
{
    /* D1,D0 language (11 English .. 00 German), D3,D2 bonus (00 none),
     * D5,D4 lives (11: 2 .. 00: 5), D6 low: two plays minimum, D7 low: self adjust */
    unsigned v = (unsigned)(3 - (c->language & 3));
    v |= (unsigned)(c->bonus & 3) << 2;
    v |= (unsigned)(5 - c->lives) << 4;
    if (!c->two_play_minimum) v |= 0x40;
    if (!c->self_adjust) v |= 0x80;
    return (uint8_t)v;
}
