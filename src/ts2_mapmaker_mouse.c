/*
 * ts2_mapmaker_mouse.c -- TimeSplitters 2's MapMaker under the mouse.
 *
 * MapMaker's editor is not a menu of panels: it is a grid with a hand over
 * it, moved by the left stick, and the game does everything else from the
 * pad (A places a piece, held it lassos; B deletes; Y turns, X copies; the
 * D-pad picks the tab and the floor; the triggers zoom; Start opens the
 * editor's menu). Its screens (the palette of pieces, the menus) are the
 * same hand over hot spots, or a table of rows picked by id. All of that
 * stays the game's. Here, while the editor's level runs:
 *
 *   - the hand is where the pointer is: the pointer's place is written
 *     into the hand's own position (F+0x30, F+0x34) when the mouse moves,
 *     so the stick and the keys still move it while the mouse rests, and
 *     while the wheel zooms, since a zoom moves the hand with the map. It
 *     stops where the stick stops it, and the pointer past that edge
 *     pushes the stick that way, which scrolls the map as the stick does
 *     (an editor's edge scrolling);
 *   - the left button is A for as long as it is held: a click places, a
 *     drag lassos, as A does. The right button is B, which deletes (the
 *     usual pair in a grid editor: left puts, right takes away). The wheel
 *     zooms, as the triggers do. The middle button does nothing;
 *   - on a screen of the editor's with the hand (the palette), the hand
 *     follows the pointer the same way and a click is A on what it is
 *     over; on a screen of rows (the menus), the row under the pointer is
 *     picked as the D-pad would pick it, and a click on it is A;
 *   - Escape is Start in the editor, which opens its menu
 *     (src/ts2_mouse.c), so a prompt names B's key as Backspace there.
 *
 * A tab's arrows and the prompts are noted where they are drawn and
 * clicked through the menu pointer (src/ts2_menu_mouse.c), which calls
 * this first while the editor's level runs.
 */
#include <stdio.h>
#include <string.h>

#include "ts2_mapmaker_mouse.h"
#include "ts2_menu_mouse.h"
#include "recomp/ts2_region.h"
#include "recomp_config.h"           /* xboxrecomp: recomp_config_bool */

#if defined(TS2_REGION_US)
#include "../config/us/mapmaker.inc"
#else
static const Ts2MapMakerAddrs k_ts2_mapmaker = { 0 };
#endif

/* The menu state's address is the menu pointer's (config/<region>/menu_mouse.inc). */
extern uint32_t ts2_menu_frontend(void);
extern uint32_t ts2_menu_level(void);

extern ptrdiff_t g_xbox_mem_offset;

#define M_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define M_MEMI(a)  (*(volatile int32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define M_MEM16(a) (*(volatile uint16_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

static const Ts2MapMakerAddrs *const MA = &k_ts2_mapmaker;

#define PRESS_FRAMES 3
#define GAP_FRAMES   2
#define ZOOM_FRAMES  6          /* a notch of the wheel holds a trigger this long */
#define EDGE_FULL    40.0f      /* menu pixels past the hand's limit for the stick's full push */

static struct {
    unsigned frame;
    int      log;
    uint32_t logged;            /* the class of the screen last written to the log */
    int      a_held;            /* the left button went down on the grid and is held */
    int      armed;             /* a screen: the hot spot id the left button went down on */
    int      left_was;
    /* Presses for the bindings filter. */
    volatile WORD buttons;
    volatile int  an_a, an_b;
    volatile unsigned until;
    unsigned gap_until;
    volatile int  trigger;      /* XBOX_BUTTON_LTRIGGER / RTRIGGER held until zoom_until, or -1 */
    volatile unsigned zoom_until;
    volatile int  stick_x, stick_y;   /* the edge's push, -32767..32767 */
    int      wheel;
} MM = { .trigger = -1 };

static int mm_ptr_ok(uint32_t a)
{
    return (a >= 0x10000u && a < 0x08000000u) || (a >= 0x80000000u && a < 0x88000000u);
}

int ts2_mapmaker_active(void)
{
    return MA->level && ts2_menu_frontend() && ts2_menu_level() == MA->level;
}

int ts2_mapmaker_editing(void)
{
    return ts2_mapmaker_active() && M_MEMI(ts2_menu_frontend() + 0x24) == 0;
}

const char *ts2_mapmaker_b_key(void)
{
    static char name[24];
    static int read;
    char k[16];

    if (!read) {
        read = 1;
        if (recomp_bindings_key_name(0, "b", k, sizeof k))
            snprintf(name, sizeof name, "[%s]", k);
    }
    return name[0] && ts2_mapmaker_editing() ? name : NULL;
}

static void mm_press(WORD buttons, int a, int b)
{
    MM.buttons = buttons;
    MM.an_a = a;
    MM.an_b = b;
    MM.until = MM.frame + PRESS_FRAMES;
    MM.gap_until = MM.until + GAP_FRAMES;
}

void ts2_mapmaker_filter(XBOX_GAMEPAD *pad)
{
    if (!ts2_mapmaker_active()) {
        MM.a_held = 0;
        MM.stick_x = MM.stick_y = 0;
        return;
    }
    if (MM.frame < MM.until) {
        pad->wButtons |= MM.buttons;
        if (MM.an_a)
            pad->bAnalogButtons[XBOX_BUTTON_A] = 255;
        if (MM.an_b)
            pad->bAnalogButtons[XBOX_BUTTON_B] = 255;
    }
    if (MM.a_held)
        pad->bAnalogButtons[XBOX_BUTTON_A] = 255;
    if (MM.trigger >= 0 && MM.frame < MM.zoom_until)
        pad->bAnalogButtons[MM.trigger] = 255;
    if (MM.stick_x)
        pad->sThumbLX = (SHORT)MM.stick_x;
    if (MM.stick_y)
        pad->sThumbLY = (SHORT)MM.stick_y;
}

static float mm_clamp(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* The hand's limits now, in menu pixels: the editor's margins, or a
 * screen's own. */
static void mm_bounds(uint32_t top, float *l, float *t, float *r, float *b)
{
    *l = (float)MA->margin_x;
    *t = (float)MA->margin_y;
    *r = 640.0f - (float)MA->margin_x;
    *b = 480.0f - (float)MA->margin_y;
    if (top && MA->screen_bounds) {
        float st = (float)M_MEMI(MA->screen_bounds) / 65536.0f;
        float sb = (float)M_MEMI(MA->screen_bounds + 4u) / 65536.0f;
        float sl = (float)M_MEMI(MA->screen_bounds + 8u) / 65536.0f;
        float sr = (float)M_MEMI(MA->screen_bounds + 12u) / 65536.0f;

        if (sr > sl + 8.0f && sb > st + 8.0f) {
            *l = sl;
            *t = st;
            *r = sr;
            *b = sb;
        }
    }
}

static void mm_hand_to(uint32_t f, float x, float y)
{
    M_MEMI(f + TS2_MF_HAND_X) = (int32_t)(x * 65536.0f);
    M_MEMI(f + TS2_MF_HAND_Y) = (int32_t)(y * 65536.0f);
}

/* The hot spot of a screen of rows under (x, y): its id, or 0, which is no
 * spot to the game too. Its corners are the hand's: from the viewport, not
 * the screen. */
static int mm_hot_at(uint32_t s, float x, float y)
{
    int n = M_MEMI(s + TS2_MS_HOT_COUNT), k;
    uint32_t hots = M_MEM32(s + TS2_MS_HOTS);
    float right = 0.0f;

    if (n <= 0 || n > 256)
        return 0;
    /* A spot is only as wide as its row's text, and the bar over the row
     * runs across them all: the column's width is what is seen. */
    for (k = 0; k < n; k++) {
        uint32_t h = hots + (uint32_t)k * TS2_HOT_SIZE;
        float r = (float)M_MEM16(h + TS2_HOT_X) + (float)M_MEM16(h + TS2_HOT_W);

        if (r > right)
            right = r;
    }
    for (k = 0; k < n; k++) {
        uint32_t h = hots + (uint32_t)k * TS2_HOT_SIZE;
        float hx = (float)M_MEM16(h + TS2_HOT_X), hy = (float)M_MEM16(h + TS2_HOT_Y);
        float hh = (float)M_MEM16(h + TS2_HOT_H);

        if (M_MEM16(h + TS2_HOT_FLAGS) & 0x81)
            continue;
        if (x >= hx && x < right && y >= hy && y < hy + hh)
            return (int)M_MEM16(h + TS2_HOT_ID);
    }
    return 0;
}

/* The line of the Mode question under (x, y), from 0, or -1: after its title
 * each hot spot is a line, or the same line again. */
static int mm_mode_at(uint32_t s, float x, float y)
{
    int n = M_MEMI(s + TS2_MS_HOT_COUNT), k, row = -1;
    uint32_t hots = M_MEM32(s + TS2_MS_HOTS), y_was = 0xFFFFFFFFu;

    if (n <= 1 || n > 16)
        return -1;
    for (k = 1; k < n; k++) {
        uint32_t h = hots + (uint32_t)k * TS2_HOT_SIZE;
        float hx = (float)M_MEM16(h + TS2_HOT_X), hy = (float)M_MEM16(h + TS2_HOT_Y);

        if (M_MEM16(h + TS2_HOT_Y) != y_was) {
            y_was = M_MEM16(h + TS2_HOT_Y);
            row++;
        }
        if (x >= hx && x < hx + (float)M_MEM16(h + TS2_HOT_W) && y >= hy &&
            y < hy + (float)M_MEM16(h + TS2_HOT_H))
            return row;
    }
    return -1;
}

int ts2_mapmaker_frame(uint32_t top, const RecompPointer *ptr, float x, float y, int inside,
                       int moved, int menu_took_press)
{
    uint32_t f = ts2_menu_frontend(), hand;
    int left_down, left_held, left_up, right_down;
    float l, t, r, b;

    MM.frame++;
    MM.stick_x = MM.stick_y = 0;
    if (!ts2_mapmaker_active() || !f) {
        MM.a_held = 0;
        return 0;
    }
    if (MM.frame == 1)
        MM.log = recomp_config_bool("RECOMP_TS2_MENU_LOG", NULL, 0);
    hand = M_MEM32(f + TS2_MF_HAND);
    left_down = (ptr->pressed & (1u << RECOMP_MOUSE_LEFT)) != 0;
    left_held = (ptr->held & (1u << RECOMP_MOUSE_LEFT)) != 0;
    left_up = (MM.left_was || left_down) && !left_held;
    MM.left_was = left_held;
    right_down = (ptr->pressed & (1u << RECOMP_MOUSE_RIGHT)) != 0;
    MM.wheel += ptr->wheel;
    if (MM.wheel > 4) MM.wheel = 4;
    if (MM.wheel < -4) MM.wheel = -4;
    mm_bounds(top, &l, &t, &r, &b);

    if (!top) {
        /* The editor. The hand follows the pointer, inside its limits. */
        if (inside && (moved || MM.frame < MM.zoom_until + GAP_FRAMES) && (hand & 2u))
            mm_hand_to(f, mm_clamp(x, l, r), mm_clamp(y, t, b));
        /* Past a limit, the stick pushes that way: the map scrolls under
         * the hand, as with the stick. Not while a key or the pad moves
         * the hand (the pointer is hidden then: inside is 0). */
        if (inside && (hand & 2u)) {
            if (x < l)
                MM.stick_x = -(int)(32767.0f * mm_clamp((l - x) / EDGE_FULL, 0.3f, 1.0f));
            else if (x > r)
                MM.stick_x = (int)(32767.0f * mm_clamp((x - r) / EDGE_FULL, 0.3f, 1.0f));
            if (y < t)
                MM.stick_y = (int)(32767.0f * mm_clamp((t - y) / EDGE_FULL, 0.3f, 1.0f));
            else if (y > b)
                MM.stick_y = -(int)(32767.0f * mm_clamp((y - b) / EDGE_FULL, 0.3f, 1.0f));
        }
        /* Left: A for as long as it is held, from a press on the grid. A
         * click over within one frame is still a press of A. */
        if (left_down && !menu_took_press && inside) {
            if (left_held)
                MM.a_held = 1;
            else if (MM.frame >= MM.gap_until)
                mm_press(0, 1, 0);
            if (MM.log)
                fprintf(stderr, "[TS2-MAPMAKER] frame %u: left button on the grid at %.0f,%.0f: A held\n",
                        MM.frame, x, y);
        }
        if (!left_held)
            MM.a_held = 0;
        /* Right: B, which deletes. */
        if (right_down && inside && MM.frame >= MM.gap_until) {
            mm_press(0, 0, 1);
            if (MM.log)
                fprintf(stderr, "[TS2-MAPMAKER] frame %u: right button: B\n", MM.frame);
        }
        /* The wheel zooms: away from the player in, toward out. */
        if (MM.wheel && MM.frame >= MM.zoom_until + GAP_FRAMES) {
            int in = MM.wheel > 0;

            MM.trigger = (in == !!MA->zoom_in_right) ? XBOX_BUTTON_RTRIGGER : XBOX_BUTTON_LTRIGGER;
            MM.zoom_until = MM.frame + ZOOM_FRAMES;
            MM.wheel += in ? -1 : 1;
            if (MM.log)
                fprintf(stderr, "[TS2-MAPMAKER] frame %u: wheel: zoom %s\n", MM.frame,
                        in ? "in" : "out");
        }
        return 1;
    }

    MM.a_held = 0;
    if (MM.log && (M_MEM32(top + 4u) != MM.logged || left_down)) {
        int n = M_MEMI(top + TS2_MS_HOT_COUNT), k;

        MM.logged = M_MEM32(top + 4u);
        fprintf(stderr, "[TS2-MAPMAKER] frame %u: screen %08X class %08X, pointer %.0f,%.0f in %d "
                "moved %d, hand bits %u at %.0f,%.0f, limits %.0f,%.0f-%.0f,%.0f, %d hot spots, "
                "picked %u\n", MM.frame, top, M_MEM32(top + 4u), x, y, inside, moved, hand & 3u,
                (float)M_MEMI(f + TS2_MF_HAND_X) / 65536.0f,
                (float)M_MEMI(f + TS2_MF_HAND_Y) / 65536.0f, l, t, r, b, n,
                (unsigned)M_MEM16(top + TS2_MS_HOT_PICKED));
        for (k = 0; k < n && k < 32; k++) {
            uint32_t h = M_MEM32(top + TS2_MS_HOTS) + (uint32_t)k * TS2_HOT_SIZE;

            fprintf(stderr, "[TS2-MAPMAKER]   hot %d at %u,%u size %ux%u id %u flags 0x%X\n", k,
                    (unsigned)M_MEM16(h + TS2_HOT_X), (unsigned)M_MEM16(h + TS2_HOT_Y),
                    (unsigned)M_MEM16(h + TS2_HOT_W), (unsigned)M_MEM16(h + TS2_HOT_H),
                    (unsigned)M_MEM16(h + TS2_HOT_ID), (unsigned)M_MEM16(h + TS2_HOT_FLAGS));
        }
    }
    if (MA->mode_class && M_MEM32(top + 4u) == MA->mode_class) {
        /* The Mode question: the line under the pointer is picked as the
         * D-pad picks it, and a click on it is A. */
        int row = inside ? mm_mode_at(top, x, y) : -1;

        if (row >= 0 && (moved || left_down))
            M_MEMI(MA->mode_row) = row;
        if (left_down && !menu_took_press)
            MM.armed = row + 1;
        if (left_up) {
            if (MM.armed && MM.armed == row + 1 && MM.frame >= MM.gap_until)
                mm_press(0, 1, 0);
            MM.armed = 0;
        }
        return 1;
    }
    if (hand & 2u) {
        /* A screen with the hand (the palette): the hand follows the
         * pointer, the game picks what it is over (hot spots, F+0x18 bit 1),
         * and a click is A on it. */
        int picked;

        if (inside && moved)
            mm_hand_to(f, mm_clamp(x, l, r), mm_clamp(y, t, b));
        /* With no hit test (bit 1 clear) the hand is a cursor over the whole
         * screen, and a click is A wherever it is. */
        picked = (hand & 1u) ? M_MEM16(top + TS2_MS_HOT_PICKED) : 1;
        if (left_down && !menu_took_press && inside)
            MM.armed = picked;
        if (left_up) {
            if (MM.armed && MM.armed == picked && MM.frame >= MM.gap_until) {
                mm_press(0, 1, 0);
                if (MM.log)
                    fprintf(stderr, "[TS2-MAPMAKER] frame %u: click with the hand on %d: A\n",
                            MM.frame, MM.armed);
            }
            MM.armed = 0;
        }
    } else if (M_MEMI(top + TS2_MS_HOT_COUNT) > 0) {
        /* A screen of rows: the row under the pointer is picked as the
         * D-pad picks it; a click on it is A. */
        int hot = inside ? mm_hot_at(top, x, y) : 0;

        if (hot && moved && M_MEM16(top + TS2_MS_HOT_PICKED) != hot) {
            M_MEM16(top + TS2_MS_HOT_PICKED) = (uint16_t)hot;
            if (MM.log)
                fprintf(stderr, "[TS2-MAPMAKER] frame %u: the pointer picks row %d\n", MM.frame,
                        hot);
        }
        if (left_down && !menu_took_press) {
            MM.armed = hot;
            if (hot)
                M_MEM16(top + TS2_MS_HOT_PICKED) = (uint16_t)hot;
        }
        if (left_up) {
            if (MM.armed && MM.armed == hot && MM.frame >= MM.gap_until) {
                mm_press(0, 1, 0);
                if (MM.log)
                    fprintf(stderr, "[TS2-MAPMAKER] frame %u: click on row %d: A\n", MM.frame, hot);
            }
            MM.armed = 0;
        }
    } else {
        MM.armed = 0;
        return 0;                       /* a screen the menu pointer knows, or nothing */
    }
    /* On a screen the wheel is the D-pad's up and down; the right button
     * does nothing, as in a menu. */
    if (MM.wheel && MM.frame >= MM.gap_until) {
        mm_press(MM.wheel > 0 ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN, 0, 0);
        MM.wheel += MM.wheel > 0 ? -1 : 1;
    }
    return 1;
}
