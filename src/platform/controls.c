#include "platform/controls.h"

#include "core/rb_symbols.h"

#include <SDL.h>
#include <vectorpie/vp_rc.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>



enum { B_AUTO = 0, B_NONE, B_KEY, B_BUTTON, B_AXIS, B_HAT };

static const char *const HAT_NAME[4] = { "up", "right", "down", "left" };
static const uint8_t HAT_BIT[4] = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT };

/* --- bindings as text ------------------------------------------------------------ */

static void parse_bind(const char *spec, rb_bind *b)
{
    int joy, k;
    char word[16];
    memset(b, 0, sizeof *b);
    b->type = B_AUTO;
    if (!strcmp(spec, "none")) {
        b->type = B_NONE;
    } else if (!strncmp(spec, "key ", 4)) {
        SDL_Scancode sc = SDL_SCANCODE_UNKNOWN;
        if (spec[4] == '#') sc = (SDL_Scancode)atoi(spec + 5);
        else sc = SDL_GetScancodeFromName(spec + 4);
        if (sc > SDL_SCANCODE_UNKNOWN && sc < SDL_NUM_SCANCODES) {
            b->type = B_KEY;
            b->index = (int16_t)sc;
        }
    } else if (sscanf(spec, "joy%d button %d", &joy, &k) == 2 && joy >= 1 && joy <= RB_MAX_JOYSTICKS) {
        b->type = B_BUTTON;
        b->joy = (uint8_t)(joy - 1);
        b->index = (int16_t)k;
    } else if (sscanf(spec, "joy%d axis %d %1s", &joy, &k, word) == 3 && joy >= 1 && joy <= RB_MAX_JOYSTICKS) {
        b->type = B_AXIS;
        b->joy = (uint8_t)(joy - 1);
        b->index = (int16_t)k;
        b->dir = word[0] == '-' ? -1 : 1;
    } else if (sscanf(spec, "joy%d hat %d %15s", &joy, &k, word) == 3 && joy >= 1 && joy <= RB_MAX_JOYSTICKS) {
        for (int i = 0; i < 4; i++)
            if (!strcmp(word, HAT_NAME[i])) {
                b->type = B_HAT;
                b->joy = (uint8_t)(joy - 1);
                b->index = (int16_t)k;
                b->dir = (int8_t)HAT_BIT[i];
            }
    }
}

/* --- joysticks ------------------------------------------------------------------- */

static void open_joysticks(rb_controls *c)
{
    /* (SDL counts its opens: these are the SDK's joysticks once more) */
    for (int i = 0; i < c->joys; i++)
        if (c->joy[i]) SDL_JoystickClose((SDL_Joystick *)c->joy[i]);
    c->joys = SDL_NumJoysticks();
    if (c->joys > RB_MAX_JOYSTICKS) c->joys = RB_MAX_JOYSTICKS;
    if (c->joys < 0) c->joys = 0;
    for (int i = 0; i < c->joys; i++) c->joy[i] = SDL_JoystickOpen(i);
}

static float axis_of(const rb_controls *c, int joy, int axis)
{
    if (joy >= c->joys || !c->joy[joy]) return 0.0f;
    return (float)SDL_JoystickGetAxis((SDL_Joystick *)c->joy[joy], axis) / 32767.0f;
}

int rb_controls_joysticks(const rb_controls *c)
{
    return c->joys;
}

const char *rb_controls_joy_name(const rb_controls *c, int j)
{
    if (j < 0 || j >= c->joys || !c->joy[j]) return "";
    const char *n = SDL_JoystickName((SDL_Joystick *)c->joy[j]);
    return n ? n : "";
}

int rb_controls_joy_axes(const rb_controls *c, int j, float *out, int max)
{
    if (j < 0 || j >= c->joys || !c->joy[j]) return 0;
    int n = SDL_JoystickNumAxes((SDL_Joystick *)c->joy[j]);
    if (n > max) n = max;
    for (int a = 0; a < n; a++) out[a] = axis_of(c, j, a);
    return n;
}

int rb_controls_joy_buttons(const rb_controls *c, int j, uint8_t *out, int max)
{
    if (j < 0 || j >= c->joys || !c->joy[j]) return 0;
    SDL_Joystick *joy = (SDL_Joystick *)c->joy[j];
    int n = SDL_JoystickNumButtons(joy);
    if (n > max) n = max;
    for (int b = 0; b < n; b++) out[b] = (uint8_t)SDL_JoystickGetButton(joy, b);
    return n;
}

/* --- reading --------------------------------------------------------------------- */

/* A binding of the program's own: 0..1 (an axis: -1..1, the way that
 * counts positive). */
static float bound(const rb_controls *c, const rb_bind *b, const Uint8 *keys)
{
    SDL_Joystick *j = b->joy < c->joys ? (SDL_Joystick *)c->joy[b->joy] : NULL;
    switch (b->type) {
    case B_KEY:    return keys[b->index] ? 1.0f : 0.0f;
    case B_BUTTON: return j && SDL_JoystickGetButton(j, b->index) ? 1.0f : 0.0f;
    case B_HAT:    return j && (SDL_JoystickGetHat(j, b->index) & (uint8_t)b->dir) ? 1.0f : 0.0f;
    case B_AXIS:   return axis_of(c, b->joy, b->index) * (float)b->dir;
    default:       return 0.0f;
    }
}

/* A switch: the binding, or the cabinet's own control. */
static int held(const rb_controls *c, vp_input *in, const Uint8 *keys, int act)
{
    const rb_bind *b = &c->bind[act];
    if (b->type != B_AUTO) return bound(c, b, keys) > 0.5f;
    switch (act) {
    case RB_ACT_UP:    return vp_input_held(in, "p1_up");
    case RB_ACT_DOWN:  return vp_input_held(in, "p1_down");
    case RB_ACT_LEFT:  return vp_input_held(in, "p1_left");
    case RB_ACT_RIGHT: return vp_input_held(in, "p1_right");
    case RB_ACT_FIRE:  return vp_input_held(in, "p1_button1");
    case RB_ACT_START: return vp_input_held(in, "start1");
    case RB_ACT_COIN:  return vp_input_held(in, "coin1");
    case RB_ACT_MENU:  return c->service_rc ? vp_input_held(in, "service") : keys[c->service_key];
    default:           return 0;
    }
}

/* An axis of the stick, -1..1 (right and down positive), as read. */
static float stick_axis(const rb_controls *c, vp_input *in, const Uint8 *keys, int act)
{
    const rb_bind *b = &c->bind[act];
    if (b->type == B_AXIS) return bound(c, b, keys);
    if (b->type != B_AUTO) return 0.0f;
    return vp_input_axis(in, act == RB_ACT_STICK_X ? "p1_stickx" : "p1_sticky");
}

static float shape(float v, float zone, float gain)
{
    float a = fabsf(v);
    if (a <= zone) return 0.0f;
    a = (a - zone) / (1.0f - zone) * gain;
    if (a > 1.0f) a = 1.0f;
    return v < 0.0f ? -a : a;
}

/* The digital stick: toward target at rate while held, back at ret when let go. */
static float approach(float v, int dir, float reach, float rate, float ret, double dt)
{
    if (dir) {
        float target = (float)dir * reach, step = rate * (float)dt;
        /* (pushed the other way: it leaves the far side at once to the middle) */
        if ((target > 0.0f && v < 0.0f) || (target < 0.0f && v > 0.0f)) v = 0.0f;
        if (v < target) v = fminf(target, v + step);
        else if (v > target) v = fmaxf(target, v - step);
        return v;
    }
    if (ret <= 0.0f) return 0.0f;
    float step = ret * (float)dt;
    if (v > 0.0f) return fmaxf(0.0f, v - step);
    return fminf(0.0f, v + step);
}

/* The service key: AdvanceMAME's action "service" if advmame.rc binds it
 * (VectorPie's CONTROLS settings and AdvanceMAME's own menu write there),
 * else F2, as in MAME. */
static void find_service_key(rb_controls *c)
{
    c->service_rc = 0;
    c->service_key = SDL_SCANCODE_F2;
    snprintf(c->service_name, sizeof c->service_name, "F2");
    char path[1024];
    vp_rc *rc = NULL;
    if (vp_rc_default_path(path, sizeof path) != VP_OK || vp_rc_load(path, &rc) != VP_OK) return;
    const char *v = vp_rc_get(rc, "input_map[service]");
    if (v && *v && strcmp(v, "auto") != 0) {
        c->service_rc = 1;
        /* its name: the first key it names, else SERVICE */
        const char *k = strstr(v, "keyboard[");
        const char *comma = k ? strchr(k, ',') : NULL;
        const char *end = comma ? strchr(comma, ']') : NULL;
        if (comma && end && end - comma - 1 > 0 && end - comma - 1 < (int)sizeof c->service_name) {
            int n = (int)(end - comma - 1);
            for (int i = 0; i < n; i++) c->service_name[i] = (char)toupper((unsigned char)comma[1 + i]);
            c->service_name[n] = '\0';
        } else {
            snprintf(c->service_name, sizeof c->service_name, "SERVICE");
        }
    }
    vp_rc_free(rc);
}

const char *rb_controls_service_name(const rb_controls *c)
{
    return c->service_name;
}

void rb_controls_init(rb_controls *c, const rb_config *cfg)
{
    memset(c, 0, sizeof *c);
    c->cap_action = -1;
    find_service_key(c);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    open_joysticks(c);
    rb_controls_configure(c, cfg, NULL);
}

void rb_controls_close(rb_controls *c)
{
    for (int i = 0; i < c->joys; i++)
        if (c->joy[i]) SDL_JoystickClose((SDL_Joystick *)c->joy[i]);
    c->joys = 0;
}

void rb_controls_configure(rb_controls *c, const rb_config *cfg, vp_input *in)
{
    c->mode = cfg->input_mode;
    c->deadzone = cfg->deadzone;
    c->gain = cfg->gain;
    c->rate = cfg->digital_rate;
    c->reach = cfg->digital_reach;
    c->ret = cfg->digital_return;
    c->invert_y = cfg->invert_y;
    c->menu_hold = cfg->menu_hold;
    for (int i = 0; i < RB_ACT_COUNT; i++) parse_bind(cfg->bind[i], &c->bind[i]);
    /* (the dead zone is ours: the SDK's own is set to nothing) */
    if (in) vp_input_set_deadzone(in, 0.0f);
}

void rb_controls_read(rb_controls *c, vp_input *in, double dt)
{
    if (SDL_NumJoysticks() != c->joys && SDL_NumJoysticks() <= RB_MAX_JOYSTICKS) open_joysticks(c);
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (dt > 0.1) dt = 0.1;

    int up = held(c, in, keys, RB_ACT_UP), down = held(c, in, keys, RB_ACT_DOWN);
    int left = held(c, in, keys, RB_ACT_LEFT), right = held(c, in, keys, RB_ACT_RIGHT);
    int fire = held(c, in, keys, RB_ACT_FIRE), start = held(c, in, keys, RB_ACT_START);
    if (up && down) up = down = 0;
    if (left && right) left = right = 0;
    c->dx = right - left;
    c->dy = down - up;
    c->ax = stick_axis(c, in, keys, RB_ACT_STICK_X);
    c->ay = stick_axis(c, in, keys, RB_ACT_STICK_Y);
    c->fire = fire;
    c->start = start;

    /* The stick's place. */
    float sx = c->mode == RB_INPUT_ANALOG ? shape(c->ax, c->deadzone, c->gain) : 0.0f;
    float sy = c->mode == RB_INPUT_ANALOG ? shape(c->ay, c->deadzone, c->gain) : 0.0f;
    float nx = approach(c->x, c->dx, c->reach, c->rate, c->ret, dt);
    float ny = approach(c->y, c->dy, c->reach, c->rate, c->ret, dt);
    c->x = sx != 0.0f ? sx : nx;
    c->y = sy != 0.0f ? sy : ny;

    /* For a menu: the stick and the arrow keys, fire, start, Enter, space. */
    rb_nav nav = { 0, 0, 0 };
    if (keys[SDL_SCANCODE_UP] || up || c->ay < -0.5f) nav.y = -1;
    else if (keys[SDL_SCANCODE_DOWN] || down || c->ay > 0.5f) nav.y = 1;
    if (keys[SDL_SCANCODE_LEFT] || left || c->ax < -0.5f) nav.x = -1;
    else if (keys[SDL_SCANCODE_RIGHT] || right || c->ax > 0.5f) nav.x = 1;
    int fire_now = fire || start || keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE];
    nav.fire = fire_now && !c->fire_was;
    c->fire_was = fire_now;
    c->nav = nav;

    /* the service key, and what the owner bound besides */
    int menu_now = (c->service_rc ? vp_input_held(in, "service") : keys[c->service_key]) ||
                   (c->bind[RB_ACT_MENU].type != B_AUTO && held(c, in, keys, RB_ACT_MENU));
    /* A cabinet without a key for it: start and fire held together. */
    if (c->menu_hold && fire && start) {
        if (!c->hold_since) c->hold_since = SDL_GetTicks() | 1u;
        else if (SDL_GetTicks() - c->hold_since >= 2000u) menu_now = 1;
    } else {
        c->hold_since = 0;
    }
    c->menu = menu_now && !c->menu_was;
    c->menu_was = menu_now;

    uint8_t in4 = RB_IN4_START;                     /* (start is low when pressed) */
    if (start) in4 &= (uint8_t)~RB_IN4_START;
    if (fire) in4 |= RB_IN4_FIRE;
    c->in4 = in4;

    int coin = held(c, in, keys, RB_ACT_COIN);
    c->coin = coin;
    if (coin && !c->coin_was && c->coin_pulse[0] == 0) c->coin_pulse[0] = RB_COIN_PULSE_NMIS;
    c->coin_was = coin;
    if (vp_input_pressed(in, "coin2") && c->coin_pulse[1] == 0) c->coin_pulse[1] = RB_COIN_PULSE_NMIS;
}

void rb_controls_tick(rb_controls *c, rb_machine *m)
{
    uint8_t in0 = 0x3f;                             /* nothing closed (active low) */
    if (c->coin_pulse[0] > 0) { in0 &= (uint8_t)~RB_IN0_COIN_R; c->coin_pulse[0]--; }
    if (c->coin_pulse[1] > 0) { in0 &= (uint8_t)~RB_IN0_COIN_M; c->coin_pulse[1]--; }
    m->in.in0 = in0;
    m->in.in4 = c->in4;
    float x = c->x, y = c->invert_y ? -c->y : c->y;
    /* The initials (GAMODE xxxx0111): LETCHK changes the letter while a pot
     * is in the top or bottom eighth of its range, and looks at both. A yoke
     * gets there unsteadily, and askew: the game is given only the axis
     * pushed more, and that all the way or not at all. */
    if ((m->mem[RB_GAMODE] & 0x0f) == 7) {
        float ax = fabsf(x), ay = fabsf(y);
        if (ay >= ax) {
            x = 0.0f;
            y = ay > 0.45f ? (y > 0.0f ? 1.0f : -1.0f) : 0.0f;
        } else {
            y = 0.0f;
            x = ax > 0.45f ? (x > 0.0f ? 1.0f : -1.0f) : 0.0f;
        }
    }
    /* (MAME's range for the pots: 64..192) */
    m->in.pot_x = (uint8_t)lroundf(128.0f + 64.0f * fmaxf(-1.0f, fminf(1.0f, x)));
    m->in.pot_y = (uint8_t)lroundf(128.0f + 64.0f * fmaxf(-1.0f, fminf(1.0f, y)));
}

/* --- a new binding --------------------------------------------------------------- */

void rb_controls_capture_begin(rb_controls *c, int action)
{
    c->cap_action = action;
    c->cap_armed = 0;
    c->cap_still = 0;
    for (int j = 0; j < RB_MAX_JOYSTICKS; j++)
        for (int a = 0; a < RB_MAX_AXES; a++)
            c->cap_rest[j][a] = j < c->joys && c->joy[j] ? SDL_JoystickGetAxis((SDL_Joystick *)c->joy[j], a) : 0;
}

int rb_controls_capture_poll(rb_controls *c, char *spec, size_t size)
{
    if (c->cap_action < 0) return -1;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    int analog = c->cap_action == RB_ACT_STICK_X || c->cap_action == RB_ACT_STICK_Y;
    int found = 0, moving = 0;
    spec[0] = '\0';

    if (keys[SDL_SCANCODE_ESCAPE]) {
        c->cap_action = -1;
        return -1;
    }
    for (int sc = 1; sc < SDL_NUM_SCANCODES && !found; sc++) {
        if (!keys[sc] || (!c->service_rc && sc == c->service_key)) continue;
        found = 1;
        if (analog) continue;               /* (a key is no axis: only keeps the wait from arming) */
        const char *name = SDL_GetScancodeName((SDL_Scancode)sc);
        if (name && *name) snprintf(spec, size, "key %s", name);
        else snprintf(spec, size, "key #%d", sc);
    }
    for (int j = 0; j < c->joys && !found; j++) {
        SDL_Joystick *joy = (SDL_Joystick *)c->joy[j];
        if (!joy) continue;
        int axes = SDL_JoystickNumAxes(joy);
        if (axes > RB_MAX_AXES) axes = RB_MAX_AXES;
        for (int a = 0; a < axes && !found; a++) {
            int v = SDL_JoystickGetAxis(joy, a) - c->cap_rest[j][a];
            if (v > 3000 || v < -3000) moving = 1;
            if (v > 19000 || v < -19000) {
                found = 1;
                snprintf(spec, size, "joy%d axis %d %c", j + 1, a, v > 0 ? '+' : '-');
            }
        }
        if (analog) continue;
        for (int b = 0; b < SDL_JoystickNumButtons(joy) && !found; b++)
            if (SDL_JoystickGetButton(joy, b)) {
                found = 1;
                snprintf(spec, size, "joy%d button %d", j + 1, b);
            }
        for (int h = 0; h < SDL_JoystickNumHats(joy) && !found; h++) {
            uint8_t v = SDL_JoystickGetHat(joy, h);
            for (int i = 0; i < 4 && !found; i++)
                if (v & HAT_BIT[i]) {
                    found = 1;
                    snprintf(spec, size, "joy%d hat %d %s", j + 1, h, HAT_NAME[i]);
                }
        }
    }
    if (!c->cap_armed) {
        /* What opened the line (fire, a stick still held over) has to be
         * let go first. */
        if (found || moving) {
            int action = c->cap_action;
            rb_controls_capture_begin(c, action);
        } else if (++c->cap_still >= 12) {
            c->cap_armed = 1;
        }
        return 0;
    }
    if (!found || !spec[0]) return 0;
    c->cap_action = -1;
    return 1;
}
