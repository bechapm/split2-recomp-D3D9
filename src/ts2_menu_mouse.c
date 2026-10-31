/*
 * ts2_menu_mouse.c -- TimeSplitters 2's menus under the mouse.
 *
 * The game's menus are panels of items, each item with its box on screen
 * (in 640x480 pixels, from the panel's corner) and each panel with its
 * selected row and column; the panel update moves the selection by the
 * pad, plays the move sound, keeps the selected item scrolled into view,
 * and runs the item's action when A or Start is pressed. All of that is the
 * game's, and stays the game's. Here, while a menu is up:
 *
 *   - the toolkit is asked for a pointer (recomp_pointer_want): the window
 *     gives the mouse back from mouse look and shows the arrow;
 *   - moving the pointer onto an item of the panel that has the focus
 *     selects it, the way the panel update does -- the selected flag moved
 *     from the old item to the new, the row and column set, the move sound
 *     played. Only when the pointer moves, so the keys and the pad still
 *     move the selection while it rests; only an item wholly in view, so
 *     the panel never scrolls under a resting pointer;
 *   - a click on an item presses A for it (the game's own accept: its
 *     action, its sound, the next screen). A click acts when the button
 *     comes up over what it went down on, as on a PC (letting go somewhere
 *     else does nothing); the press selects the line at once. On an item whose value is
 *     stepped with left and right (an option) a click on the value's left
 *     half or the arrow before it steps it back, on its right half or the
 *     arrow after it on; on its name, A if it has an action, else on;
 *   - a click on a button hint ("A Select  B Back", under a panel or in
 *     it, or a pause page's "Select") is that button; on a pause menu
 *     page's arrows, the D-pad's left or right, which changes page; on a
 *     page's scroll arrow, the D-pad's up or down for as long as it is
 *     held, which scrolls it;
 *   - the hint, option arrow or page arrow under the pointer is lit as the
 *     game lights the line selected: the hint and the page arrows pulse
 *     white to cyan, an option's arrow between its own shade and the one
 *     it has while that way is held;
 *   - on the pause menu's Inventory page, the pointer selects the row it
 *     moves onto and a click takes it (the game's Select);
 *   - the wheel is the D-pad's up and down, which scrolls a long list;
 *     the right button does nothing (Esc, Backspace and the Back hint go
 *     back, as the conventions most PC games and the platform guidelines
 *     follow have it).
 *
 * A menu is up when player 1's stack of screens has one on top that runs
 * and is shown, and there is a panel in it with the focus: in the front end
 * any time, in a level only while the game is paused (the pause menu). The
 * front end keeps the pointer through the gaps between its screens, and a
 * screen there with nothing to select takes a click as Start (Select
 * Controller, Enter Name, whose letters are not items). The pause menu is
 * left only by the keys that leave it and by its own Continue: a click on
 * nothing does nothing there (Start or B on a page closes the menu), and on
 * a screen opened from a page Esc is B, which goes back one step. In a level the
 * mouse goes back to the camera when the menu closes. The debugging tools'
 * menu (ts2_mouse_hold) puts all of this aside.
 *
 * The pause menu's lines are 3D text on a moving mesh, with no box of their
 * own: see n_box_3d.
 *
 * Where an option's value is, is the game's own layout, measured with its
 * own text width (sub_001AC170), called here. Where the hints and the
 * arrows are is what the game drew last frame: the drawing hooks below see
 * each one go by (with the placement of the site that drew it) and note it.
 *
 * A cutscene is skipped by a click: in the front end, a click with no
 * screen up; in a level, src/ts2_mouse.c (where the left button is fire).
 * RECOMP_TS2_SKIP_BUTTON says which pad button that is (a, b, start, back):
 * A by default. A and Start skip a cutscene and a level's fly-over, B does not.
 *
 * In widescreen the menus are drawn in proportion, in the middle 4:3 of the
 * picture (k_ts2_ui_places has every panel and item site as CENTRE), so a
 * pointer over a 16:9 picture is measured from the middle; over a 4:3 one
 * it is the picture. What the hooks note is measured the way its site is
 * placed (the hints under a panel go to the left edge, LEFT).
 *
 * A scripted run's window has no pointer, so RECOMP_TS2_POINTER_SEQ moves
 * one: "FRAME:X:Y" puts it at X, Y in the game's 640x480 menu pixels at
 * that present (counted as ts2_mouse_frame counts them), "FRAME:click",
 * "FRAME:rclick" and "FRAME:wheel:N" press, "FRAME:lhold:N" holds the
 * left button for N presents (as fire, where no menu is up). RECOMP_TS2_MENU_LOG=1 prints
 * each screen's panels and items as the pointer finds them, and what it did.
 * The toolkit's RECOMP_INPUT_SEQ takes the pad over and so skips the bindings
 * filter that a click's press goes through: with it, set
 * RECOMP_INPUT_SEQ_PAD=1 to test clicks.
 */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ts2_menu_mouse.h"
#include "ts2_mouse.h"               /* ts2_mouse_is_held */
#include "recomp/gen/recomp_cpu.h"   /* g_esp */
#include "recomp/ts2_region.h"
#include "input_bindings.h"          /* xboxrecomp: the pointer */
#include "recomp_config.h"           /* xboxrecomp: RECOMP_TS2_MENU_LOG */

/* xboxrecomp src/d3d/d3d8_xbox.h: the shape of the picture shown. */
extern int xbox_D3D8PictureShape(unsigned int *width, unsigned int *height);

#if defined(TS2_REGION_US)
#include "../config/us/menu_mouse.inc"
#else
/* Not found in the PAL release yet: the menus stay the pad's and the keys'. */
static const Ts2MenuAddrs k_ts2_menu = { 0 };
#endif

static const Ts2MenuAddrs *const A = &k_ts2_menu;

extern ptrdiff_t g_xbox_mem_offset;

#define N_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define N_MEMI(a)  (*(volatile int32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define N_MEMF(a)  (*(volatile float *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define N_MEM8(a)  (*(volatile int8_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

/* The game's menus are laid out on a 640x480 screen. */
#define TS2_MENU_W 640.0f
#define TS2_MENU_H 480.0f

/* Presses made for the pointer, in presents: long enough for a game that
 * reads the pad once a frame to see the press and then the release. */
#define PRESS_FRAMES 3
#define GAP_FRAMES   2
#define WHEEL_QUEUE  4

/* What the pointer is over. */
typedef struct Hit {
    uint32_t panel, item;
    int row, col;
} Hit;

/* What a click is on, noted when the left button goes down and compared
 * when it comes up (n_target, n_same). */
enum { T_NONE, T_PAGE, T_SCROLL, T_HINT, T_INVENTORY, T_EMPTY, T_ITEM };

typedef struct Target {
    int      kind;
    int      dir;            /* T_PAGE: -1, 1; T_SCROLL: TS2_ARROW_*; T_ITEM: the option side */
    int      button;         /* T_HINT: 1 B, 2 A */
    int      row;            /* T_INVENTORY */
    uint32_t item;           /* T_ITEM */
} Target;

static struct {
    unsigned frame;
    int      started;
    int      log;
    int      want;           /* asked the toolkit for a pointer */
    unsigned moved;          /* the pointer's last position count */
    uint32_t screen;         /* the screen last seen, for the log */
    uint32_t hover;          /* the item last selected by the pointer */
    /* Presses for the bindings filter: which, and until when. */
    volatile WORD buttons;
    volatile int  analog_a, analog_b;
    volatile unsigned until;
    unsigned gap_until;
    int      wheel;          /* notches waiting: + up, - down */
    const char *seq;         /* RECOMP_TS2_POINTER_SEQ, what is left of it */
    float    seq_x, seq_y;   /* the scripted pointer, in menu pixels */
    int      seq_on;
    unsigned seq_hold_until; /* "lhold": the left button held until this present */
    WORD     skip;           /* RECOMP_TS2_SKIP_BUTTON */
    int      skip_analog;
    int      skip_read;
    /* The pointer as the drawing hooks measure what they draw against it:
     * in menu pixels, as the middle 4:3 (px), the left edge (pxl), the
     * right edge (pxr) and the whole picture (pxs) place it. */
    int      pin;
    float    px, pxl, pxr, pxs, py;
    WORD     scroll_held;    /* a scroll arrow held down: the D-pad way it scrolls */
    int      players;        /* local players, this frame */
    int      left_held;      /* the left button was down at the last read */
    Target   armed;          /* what the left button went down on */
    int      esc_backs;      /* a pause menu screen opened from a page is up */
} N;

/* ── what the menus drew ─────────────────────────────────────── */

/* Each frame the drawing hooks (ts2_menu_text, ts2_menu_arrow,
 * ts2_menu_sprite) note where the hints and arrows went, in the game's
 * 640x480 pixels and with their call site's placement, and light the one
 * under the pointer as they draw it. The frame after, a click is measured
 * against what was drawn: whatever the screen, whichever layout. */
enum { DR_HINT, DR_ARROW, DR_SCROLL, DR_PAGE };

typedef struct Drawn {
    uint8_t kind;
    int8_t  place;           /* TS2_PLACE_* */
    int8_t  dir;             /* DR_ARROW, DR_SCROLL: TS2_ARROW_*; DR_PAGE: -1 before, 1 after */
    uint8_t button;          /* DR_HINT: 1 B, 2 A */
    float   x0, y0, x1, y1;
} Drawn;

#define DRAWN_MAX 48
static Drawn g_drawn[2][DRAWN_MAX];
static int   g_drawn_n[2];
static int   g_drawn_cur;        /* being drawn; the other is the frame before */

static int n_ptr_ok(uint32_t a)
{
    return (a >= 0x10000u && a < 0x08000000u) || (a >= 0x80000000u && a < 0x88000000u);
}

/* ── the menus ───────────────────────────────────────────────── */

/* Player 1's screen on top of the stack, when it runs and is shown. */
static uint32_t n_top_screen(void)
{
    int32_t count;
    uint32_t s;

    if (!A->frontend)
        return 0;
    count = N_MEMI(A->frontend + TS2_MF_COUNT);
    if (count < 1 || count > 3)
        return 0;
    s = N_MEM32(A->frontend + TS2_MF_SCREENS + 4u * (uint32_t)(count - 1));
    if (!n_ptr_ok(s) || (N_MEM32(s + TS2_MS_FLAGS) & 5u) != 5u)
        return 0;
    return s;
}

static int n_panel_live(uint32_t p)
{
    uint32_t f = N_MEM32(p + TS2_MP_FLAGS);

    return (f & (TS2_MP_FOCUS | TS2_MP_SHOWN)) == (TS2_MP_FOCUS | TS2_MP_SHOWN) &&
           N_MEMI(p + TS2_MP_ROW) >= 0;
}

/* A menu the pointer can work: a screen up, and a panel in it with the
 * focus and a selection. In a level, only the pause menu. */
static uint32_t n_menu(void)
{
    uint32_t s, p;
    int n;

    if (!A->frontend || !A->level_id)
        return 0;
    if (N_MEM32(A->level_id) < A->level_menus && !(A->pause && N_MEMI(A->pause) > 0))
        return 0;
    s = n_top_screen();
    if (!s)
        return 0;
    for (p = N_MEM32(s + TS2_MS_PANELS), n = 0; n_ptr_ok(p) && n < 32;
         p = N_MEM32(p + TS2_MP_NEXT), n++)
        if (n_panel_live(p))
            return s;
    return 0;
}

/* The item the panel shows at (row, column): sub_000ED700's walk. */
static uint32_t n_item_at(uint32_t p, int row, int col)
{
    uint32_t e = N_MEM32(p + TS2_MP_ITEMS), x;
    int32_t look;

    for (; row > 0 && e; row--)
        e = N_MEM32(e + TS2_MI_NEXT_ROW);
    for (; col > 0 && e; col--) {
        x = N_MEM32(e + TS2_MI_NEXT_COL);
        if (x)
            e = x;
    }
    if (!e || !n_ptr_ok(e))
        return 0;
    for (look = N_MEMI(e + TS2_MI_LOOK); look > 0 && e; look--)
        e = N_MEM32(e + TS2_MI_OPTION);
    return n_ptr_ok(e) ? e : 0;
}

/* Where item i is in the panel's rows and columns; 0 if it is in none. */
static int n_row_col(uint32_t p, uint32_t item, int *row, int *col)
{
    uint32_t r = N_MEM32(p + TS2_MP_ITEMS), x;
    int rows = N_MEMI(p + TS2_MP_ROWS), ri, ci;

    for (ri = 0; n_ptr_ok(r) && ri < rows && ri < 256; ri++, r = N_MEM32(r + TS2_MI_NEXT_ROW))
        for (x = r, ci = 0; n_ptr_ok(x) && ci < 64; x = N_MEM32(x + TS2_MI_NEXT_COL), ci++)
            if (x == item) {
                *row = ri;
                *col = ci;
                return 1;
            }
    return 0;
}

/* The panel's visible part, as its drawing clips it (sub_000F2040). */
static void n_clip(uint32_t p, float *l, float *t, float *r, float *b)
{
    int32_t x = N_MEMI(p + TS2_MP_X), y = N_MEMI(p + TS2_MP_Y);
    int32_t cx = N_MEMI(p + TS2_MP_CLIP_X), cy = N_MEMI(p + TS2_MP_CLIP_Y);
    int32_t r1 = cx + N_MEMI(p + TS2_MP_CLIP_W), r2 = x + N_MEMI(p + TS2_MP_W);
    int32_t b1 = cy + N_MEMI(p + TS2_MP_CLIP_H), b2 = y + N_MEMI(p + TS2_MP_H);

    *l = (float)(cx > x ? cx : x);
    *t = (float)(cy > y ? cy : y);
    *r = (float)(r1 < r2 ? r1 : r2);
    *b = (float)(b1 < b2 ? b1 : b2);
}

/* An item of a 3D menu (the pause menu) has no box: it is a line of text
 * laid on a mesh that moves, each corner of each letter going through the
 * mesh's matrix and the viewport (sub_000ED060, sub_001ACD90, sub_001AB880).
 * The line's place on the mesh comes from a matrix in force only while the
 * text is laid out, which is gone by the present, so it is measured instead:
 * mesh y = N_3D_Y0 + N_3D_Y_SCALE * the item's height fits all six lines of
 * the pause menu's Options page to a pixel. The mesh's own matrix is read
 * each frame, for where it has moved to. */
#define N_3D_Y0      (-0.0289f)
#define N_3D_Y_SCALE 0.797f
#define N_3D_HALF_W  90.0f          /* menu pixels; the longest line is about this */
#define N_3D_HALF_H  24.0f          /* lines are 52 apart at the closest */

static int n_box_3d(uint32_t i, float *x0, float *y0, float *x1, float *y1)
{
    uint32_t c = N_MEM32(i + TS2_MI_CHILD), vp = A->viewport ? N_MEM32(A->viewport) : 0;
    uint32_t m = c + TS2_MC_MATRIX;
    float y = N_3D_Y0 + N_3D_Y_SCALE * N_MEMF(i + TS2_MI_Y_3D);
    float z = N_MEMF(A->frontend + TS2_MF_TEXT_Z);
    float w = y * N_MEMF(m + 0x1C) + z * N_MEMF(m + 0x2C) + N_MEMF(m + 0x3C);
    float sx, sy;

    if (!n_ptr_ok(vp) || !n_ptr_ok(c) || !(w > 0.0001f))
        return 0;
    sx = (float)N_MEMI(vp + TS2_VP_CENTRE_X) + (float)N_MEMI(vp + TS2_VP_HALF_W) *
         (y * N_MEMF(m + 0x10) + z * N_MEMF(m + 0x20) + N_MEMF(m + 0x30)) / w;
    sy = (float)N_MEMI(vp + TS2_VP_CENTRE_Y) - (float)N_MEMI(vp + TS2_VP_HALF_H) *
         (y * N_MEMF(m + 0x14) + z * N_MEMF(m + 0x24) + N_MEMF(m + 0x34)) / w;
    *x0 = sx - N_3D_HALF_W;
    *x1 = sx + N_3D_HALF_W;
    *y0 = sy - N_3D_HALF_H;
    *y1 = sy + N_3D_HALF_H;
    return 1;
}

/* An item's box on screen, as the panel draws it. */
static void n_box(uint32_t p, uint32_t i, float *x0, float *y0, float *x1, float *y1)
{
    float y = (float)(N_MEMI(p + TS2_MP_Y) + N_MEMI(i + TS2_MI_Y));

    if (N_MEM32(i + TS2_MI_CHILD) && n_box_3d(i, x0, y0, x1, y1))
        return;
    if (N_MEM32(i + TS2_MI_FLAGS) & TS2_MI_SCROLLS)
        y += (float)N_MEMI(p + TS2_MP_SCROLL) - N_MEMF(p + TS2_MP_OFFSET);
    *x0 = (float)(N_MEMI(p + TS2_MP_X) + N_MEMI(i + TS2_MI_X));
    *y0 = y;
    *x1 = *x0 + (float)N_MEMI(i + TS2_MI_W);
    *y1 = y + (float)N_MEMI(i + TS2_MI_H);
}

static uint32_t n_option_value(uint32_t p, uint32_t i, float *right);

/* The selectable item under (x, y), in menu pixels, wholly in view. */
static int n_hit(uint32_t s, float x, float y, Hit *h)
{
    uint32_t p, i;
    int n, k, count;

    for (p = N_MEM32(s + TS2_MS_PANELS), n = 0; n_ptr_ok(p) && n < 32;
         p = N_MEM32(p + TS2_MP_NEXT), n++) {
        float cl, ct, cr, cb;

        if (!n_panel_live(p))
            continue;
        n_clip(p, &cl, &ct, &cr, &cb);
        count = N_MEMI(p + TS2_MP_ITEM_COUNT);
        for (i = N_MEM32(p + TS2_MP_ITEMS), k = 0; n_ptr_ok(i) && k < count && k < 512;
             i = N_MEM32(i + TS2_MI_NEXT), k++) {
            float x0, y0, x1, y1;

            if (N_MEM32(i + TS2_MI_FLAGS) & TS2_MI_DISABLED)
                continue;
            n_box(p, i, &x0, &y0, &x1, &y1);
            {
                float right;

                /* An option's right arrow may end past its box. */
                if (n_option_value(p, i, &right) && right + 6.0f > x1)
                    x1 = right + 6.0f;
            }
            if (x < x0 || x >= x1 || y < y0 || y >= y1)
                continue;
            /* The panel clips what it draws itself; a 3D menu's text is
             * drawn outside it (its panel is 0 high). */
            if (!N_MEM32(i + TS2_MI_CHILD) &&
                (x < cl || x >= cr || y < ct || y >= cb ||
                 y0 < ct - 1.0f || y1 > cb + 1.0f))
                continue;               /* partly scrolled away, or not in view */
            if (!n_row_col(p, i, &h->row, &h->col))
                continue;
            h->panel = p;
            h->item = i;
            return 1;
        }
    }
    return 0;
}

/* Calls a lifted function of the game's, cdecl, from the present: on a
 * stack of its own below the guest's, its registers put back after.
 * Returns eax. */
static uint32_t n_guest_call(void (*fn)(void), const uint32_t *args, int n)
{
    uint32_t top = g_esp, eax = g_eax, ecx = g_ecx, edx = g_edx, ret;
    int k;

    g_esp = (g_esp - 0x40u) & ~0xFu;
    for (k = n - 1; k >= 0; k--) {
        g_esp -= 4;
        N_MEM32(g_esp) = args[k];
    }
    g_esp -= 4;
    N_MEM32(g_esp) = 0;                         /* return address: none */
    fn();
    ret = g_eax;
    g_esp = top;
    g_eax = eax;
    g_ecx = ecx;
    g_edx = edx;
    return ret;
}

/* A font's line height, in menu pixels, as sub_001AB100 has it. */
static float n_font_height(uint32_t font)
{
    uint32_t desc, set;
    int32_t h;

    if (!A->fonts || font >= 64u)
        return 0.0f;
    desc = A->fonts + font * 0x1Cu;
    set = A->glyph_sets + (N_MEM32(desc + TS2_FONT_GLYPHS) << 5);
    h = N_MEMI(set + TS2_GLYPHS_H);
    if (N_MEM32(A->text_flags) & 1u)
        h >>= 1;
    return (float)h * N_MEMF(desc + TS2_FONT_SCALE_Y);
}

/* How wide the game draws a string in a font: its own measure, with the
 * font it had selected put back after. -1 when it cannot be asked. */
static int n_text_width(uint32_t font, uint32_t str)
{
    uint32_t keep[3];
    int k, w;

    if (!A->font_select || !A->text_width || !A->font_now || font >= 64u || !n_ptr_ok(str))
        return -1;
    for (k = 0; k < 3; k++)
        keep[k] = N_MEM32(A->font_now + 4u * (uint32_t)k);
    n_guest_call(A->font_select, &font, 1);
    w = (int)n_guest_call(A->text_width, &str, 1);
    for (k = 0; k < 3; k++)
        N_MEM32(A->font_now + 4u * (uint32_t)k) = keep[k];
    return w >= 0 && w < 4096 ? w : -1;
}

/* An option's value now, and where its right arrow ends (x, menu pixels),
 * as sub_000F1230 draws it; 0 when the item is not an option. */
static uint32_t n_option_value(uint32_t p, uint32_t i, float *right)
{
    uint32_t c;
    int32_t k;

    if (N_MEMI(i + TS2_MI_CHOICES) <= 0 || (N_MEM32(i + TS2_MI_FLAGS) & TS2_MI_FIXED) ||
        N_MEM32(i + TS2_MI_CHILD))
        return 0;
    c = N_MEM32(i + TS2_MI_CHOICE_LIST);
    for (k = N_MEMI(i + TS2_MI_CHOICE); k > 0 && n_ptr_ok(c); k--)
        c = N_MEM32(c + TS2_CH_NEXT);
    if (!n_ptr_ok(c))
        return 0;
    *right = (float)(N_MEMI(p + TS2_MP_X) + N_MEMI(i + TS2_MI_X) + N_MEMI(c + TS2_CH_X));
    return c;
}

/* Which way a click at x steps an option: -1 on the value's left half or
 * the arrow before it, 1 on its right half or the arrow after it, 0 on
 * neither (its name). sub_000F1230 draws the right arrow aw wide ending at
 * the value's x, the text ending 4 before the arrow, and the left arrow
 * ending 4 before the text, aw the font's height less 4. */
static int n_option_side(uint32_t p, uint32_t i, float x)
{
    float right, aw, mid, left;
    uint32_t c = n_option_value(p, i, &right);
    int w;

    if (!c)
        return 0;
    aw = (float)(int)(n_font_height(N_MEM32(i + TS2_MI_FONT)) - 4.0f);
    w = n_text_width(N_MEM32(i + TS2_MI_FONT), N_MEM32(c + TS2_CH_TEXT));
    if (w < 0 || aw < 0.0f)
        return 0;
    mid = right - aw - 4.0f - (float)w * 0.5f;
    left = right - aw - 4.0f - (float)w - 4.0f - aw;
    if (x >= mid && x <= right + 6.0f)
        return 1;
    if (x >= left - 6.0f && x < mid)
        return -1;
    return 0;
}

/* The sound the menus make when the selection moves (sub_000F1900). */
static void n_move_sound(void)
{
    if (A->play_sound && A->sound_on && N_MEM32(A->sound_on) && n_ptr_ok(N_MEM32(A->sounds))) {
        uint32_t args[2];

        args[0] = N_MEM32(N_MEM32(A->sounds));
        args[1] = 0;
        n_guest_call(A->play_sound, args, 2);
    }
}

/* ── the drawing hooks ───────────────────────────────────────── */

static void n_drawn_add(int kind, int place, int dir, int button,
                        float x0, float y0, float x1, float y1)
{
    Drawn *d;

    if (g_drawn_n[g_drawn_cur] >= DRAWN_MAX)
        return;
    d = &g_drawn[g_drawn_cur][g_drawn_n[g_drawn_cur]++];
    d->kind = (uint8_t)kind;
    d->place = (int8_t)place;
    d->dir = (int8_t)dir;
    d->button = (uint8_t)button;
    d->x0 = x0;
    d->y0 = y0;
    d->x1 = x1;
    d->y1 = y1;
}

/* Whether the pointer is over a box drawn with this placement, slack
 * pixels around it. A site with no placement of its own is put where the
 * renderer's width guess puts it -- the middle 4:3 or the whole picture --
 * so either will do. */
static int n_under(int place, float x0, float y0, float x1, float y1, float slack)
{
    float x;

    if (!N.pin || N.py < y0 - slack || N.py >= y1 + slack)
        return 0;
    switch (place) {
    case TS2_PLACE_OFF:
    case TS2_PLACE_CENTRE:  x = N.px;  break;
    case TS2_PLACE_LEFT:    x = N.pxl; break;
    case TS2_PLACE_RIGHT:   x = N.pxr; break;
    case TS2_PLACE_STRETCH: x = N.pxs; break;
    default:
        return (N.px >= x0 - slack && N.px < x1 + slack) ||
               (N.pxs >= x0 - slack && N.pxs < x1 + slack);
    }
    return x >= x0 - slack && x < x1 + slack;
}

/* The game's pulse: its sine table at its menu clock, -1..1, as the
 * selected line reads it (sub_000F1230). */
static float n_wave(void)
{
    float t;
    uint32_t i;

    if (!A->sine)
        return 0.0f;
    t = N_MEMF(A->frontend + TS2_MF_TIME);
    if (!(t >= 0.0f && t < 1.0e6f))
        return 0.0f;
    i = (uint32_t)(int32_t)(t * 6.2831855f * 325.94931f + 0.5f) & 0x7FFu;
    return N_MEMF(A->sine + 4u * i);
}

static uint32_t n_rgb(uint32_t c)
{
    return (N_MEM32(c) & 0xFFu) << 16 | (N_MEM32(c + 4) & 0xFFu) << 8 | (N_MEM32(c + 8) & 0xFFu);
}

/* A colour (r, g, b, a bytes, high to low) lit the way the game lights the
 * line selected: red pulsing between 51 and 127 over the style's green and
 * blue, white to cyan and back once a second. Its alpha is kept. */
static uint32_t n_lit(uint32_t colour)
{
    float s = n_wave();
    uint32_t r = (uint32_t)(int)(((s + 1.0f) * 0.3f + 0.4f) * 127.0f) & 0xFFu;
    uint32_t g = 120, b = 120;

    if (A->styles) {
        uint32_t st = A->styles + TS2_STYLE_SIZE;     /* style 1, the panels' */

        g = N_MEM32(st + TS2_STYLE_LIT + 4) & 0xFFu;
        b = N_MEM32(st + TS2_STYLE_LIT + 8) & 0xFFu;
    }
    return r << 24 | g << 16 | b << 8 | (colour & 0xFFu);
}

/* An option's arrow lit: its own shade and the shade it has while that way
 * is held, in turn on the same pulse. Its style is the one whose arrow
 * shade it is drawn in. */
static uint32_t n_arrow_lit(uint32_t colour)
{
    int s, k, j;

    for (s = 0; A->styles && s < TS2_STYLES; s++) {
        uint32_t st = A->styles + (uint32_t)s * TS2_STYLE_SIZE;

        for (k = 0; k < 2; k++) {
            uint32_t c = st + (k ? TS2_STYLE_ARROW_DIM : TS2_STYLE_ARROW);
            uint32_t lit = st + TS2_STYLE_ARROW_LIT, out = 0;
            float w = (n_wave() + 1.0f) * 0.5f;

            if (n_rgb(c) != (colour >> 8) || n_rgb(c) == n_rgb(lit))
                continue;
            for (j = 0; j < 3; j++) {
                float from = (float)(N_MEM32(c + 4u * (uint32_t)j) & 0xFFu);
                float to = (float)(N_MEM32(lit + 4u * (uint32_t)j) & 0xFFu);

                out = out << 8 | ((uint32_t)(int)(from + (to - from) * w) & 0xFFu);
            }
            return out << 8 | (colour & 0xFFu);
        }
    }
    return n_lit(colour);
}

void ts2_menu_arrow(int dir)
{
    uint32_t a = g_esp, ret;
    float x0, y0, x1, y1;
    int kind, place;

    if (!N.want || !A->frontend)
        return;
    ret = N_MEM32(a);
    if (ret >= A->item_draw && ret < A->item_draw_end) {
        kind = DR_ARROW;
        place = TS2_PLACE_CENTRE;            /* beside its value, in the panel */
    } else if (ret >= A->scroll_draw && ret < A->scroll_draw_end) {
        kind = DR_SCROLL;
        place = TS2_PLACE_AUTO;
    } else {
        return;
    }
    x0 = (float)N_MEMI(a + 4);
    y0 = (float)N_MEMI(a + 8);
    x1 = (float)N_MEMI(a + 12);
    y1 = (float)N_MEMI(a + 16);
    n_drawn_add(kind, place, dir, 0, x0, y0, x1, y1);
    if (n_under(place, x0, y0, x1, y1, 4.0f))
        N_MEM32(a + 20) = kind == DR_ARROW ? n_arrow_lit(N_MEM32(a + 20)) : n_lit(N_MEM32(a + 20));
}

void ts2_menu_sprite(void)
{
    uint32_t a = g_esp, ret;
    float x0, y0, x1, y1;

    if (!N.want || !A->page_draw)
        return;
    ret = N_MEM32(a);
    if (ret < A->page_draw || ret >= A->page_draw_end)
        return;
    x0 = (float)N_MEMI(a + 4);
    y0 = (float)N_MEMI(a + 8);
    x1 = (float)N_MEMI(a + 12);
    y1 = (float)N_MEMI(a + 16);
    n_drawn_add(DR_PAGE, TS2_PLACE_AUTO, x0 + x1 < TS2_MENU_W ? -1 : 1, 0, x0, y0, x1, y1);
    if (n_under(TS2_PLACE_AUTO, x0, y0, x1, y1, 6.0f))
        N_MEM32(a + 0x24) = n_lit(N_MEM32(a + 0x24));
}

/* How wide the first n bytes of a string draw in the font selected now. */
/* ── keys in the hints, while the keyboard and mouse are in use ─────── */

/* The game writes a pad button in its text as \x0b and a digit, drawn as
 * the button's picture: 2 is A, 1 is B. On a PC the prompt names the key
 * the player is using instead (the conventions write-up: Steam's and
 * Apple's guidelines, and every PC port review, ask for prompts that match
 * the device in use). While a menu is up and the keyboard and mouse have
 * it (recomp_input_using_keys), each text call and the game's own measure
 * of text (sub_001AC170, so its hint bar lays the line out at the new
 * width) get a copy of the line with those two written as the keys that
 * do the same in a menu: Enter for A, Esc for B. The other digits, and
 * text in a level outside the pause menu, keep the pad's pictures. */
#define KEY_TEXT_LEN  256
#define KEY_TEXT_RING 4
static const char *const k_key_names[3] = { NULL, "[Esc]", "[Enter]" };

extern uint32_t xbox_HeapAlloc(uint32_t size, uint32_t alignment);

static uint32_t n_key_buffer(void)
{
    static uint32_t ring;
    static int next;

    if (!ring)
        ring = xbox_HeapAlloc(KEY_TEXT_LEN * KEY_TEXT_RING, 16);
    if (!ring)
        return 0;
    next = (next + 1) % KEY_TEXT_RING;      /* a call inside a call gets its own */
    return ring + (uint32_t)next * KEY_TEXT_LEN;
}

/* str with A and B written as keys, in guest memory; 0 when it has neither
 * or nothing is to be changed. */
static uint32_t n_key_text(uint32_t str)
{
    uint32_t k, o = 0, buf;
    int any = 0;

    if (!N.want || !recomp_input_using_keys() || !n_ptr_ok(str))
        return 0;
    for (k = 0; k < 0x7F && N_MEM8(str + k); k++)
        if (N_MEM8(str + k) == 0x0B && (N_MEM8(str + k + 1u) == '1' || N_MEM8(str + k + 1u) == '2'))
            any = 1;
    if (!any || !(buf = n_key_buffer()))
        return 0;
    for (k = 0; k < 0x7F && N_MEM8(str + k) && o + 9u < KEY_TEXT_LEN; k++) {
        int8_t c = N_MEM8(str + k), d = N_MEM8(str + k + 1u);

        if (c == 0x0B && (d == '1' || d == '2')) {
            const char *n = k_key_names[d - '0'];

            while (*n)
                N_MEM8(buf + o++) = *n++;
            k++;
            continue;
        }
        N_MEM8(buf + o++) = c;
    }
    N_MEM8(buf + o) = 0;
    return buf;
}

/* Calls fn (a lifted text function) with its string argument, slot si on
 * the stack, written with keys for A and B; the argument is put back after.
 * Leaves the stack as fn does. */
static void n_key_call(void (*fn)(void), int si)
{
    uint32_t e0 = g_esp, str = N_MEM32(e0 + 4u * (uint32_t)si), kt = n_key_text(str);

    if (!kt) {
        fn();
        return;
    }
    N_MEM32(e0 + 4u * (uint32_t)si) = kt;
    fn();
    N_MEM32(e0 + 4u * (uint32_t)si) = str;
}

void ts2_key_text(void (*fn)(void), int si)
{
    n_key_call(fn, si);
}

static float n_width_upto(uint32_t str, uint32_t n)
{
    int8_t keep;
    uint32_t w;

    if (!n || !A->text_width)
        return 0.0f;
    keep = N_MEM8(str + n);
    N_MEM8(str + n) = 0;
    w = n_guest_call(A->text_width, &str, 1);
    N_MEM8(str + n) = keep;
    return w < 4096u ? (float)w : 0.0f;
}

/* Draws a line in three parts -- before the lit hint, the hint, after it --
 * each a call of the text function on that part, the hint in n_lit. The
 * arguments are put back before each call; the line is cut by a 0 written
 * after the part and taken away after. Leaves the stack as one call does. */
static void n_text_parts(void (*draw)(void), int form, int si, int ci, uint32_t str,
                         uint32_t len, uint32_t b0, uint32_t b1, float x)
{
    uint32_t e0 = g_esp, keep[6], cut[3][2];
    float at = x;
    int k, j;

    cut[0][0] = 0;  cut[0][1] = b0;
    cut[1][0] = b0; cut[1][1] = b1;
    cut[2][0] = b1; cut[2][1] = len;
    for (j = 0; j < 6; j++)
        keep[j] = N_MEM32(e0 + 4u + 4u * (uint32_t)j);
    for (k = 0; k < 3; k++) {
        uint32_t p0 = cut[k][0], p1 = cut[k][1];
        int8_t end;
        float w;

        if (p1 <= p0)
            continue;
        for (j = 0; j < 6; j++)
            N_MEM32(e0 + 4u + 4u * (uint32_t)j) = keep[j];
        w = n_width_upto(str + p0, p1 - p0);
        end = N_MEM8(str + p1);
        N_MEM8(str + p1) = 0;
        N_MEM32(e0 + 4u * (uint32_t)si) = str + p0;
        if (k == 1)
            N_MEM32(e0 + 4u * (uint32_t)ci) = n_lit(keep[ci - 1]);
        if (form != TS2_TEXT_AT_INT)
            N_MEMF(e0 + 4u) = at;               /* the int form moves its own x */
        g_esp = e0;
        n_key_call(draw, si);
        N_MEM8(str + p1) = end;
        at += w;
    }
    for (j = 0; j < 6; j++)
        N_MEM32(e0 + 4u + 4u * (uint32_t)j) = keep[j];
    g_esp = e0 + 4u;
}

void ts2_menu_text(void (*draw)(void), int form, int place)
{
    uint32_t a = g_esp, str, len, k, hot0 = 0, hot1 = 0;
    int si = form == TS2_TEXT_AT_ALIGN ? 4 : 3, ci = si + 1, hot = 0;
    float x, y, h;

    if (!N.want || !A->frontend || !A->text_width) {
        draw();
        return;
    }
    str = N_MEM32(a + 4u * (uint32_t)si);
    if (!n_ptr_ok(str)) {
        n_key_call(draw, si);
        return;
    }
    if (form == TS2_TEXT_AT_INT) {
        uint32_t xp = N_MEM32(a + 4), yp = N_MEM32(a + 8);

        if (!n_ptr_ok(xp) || !n_ptr_ok(yp)) {
            n_key_call(draw, si);
            return;
        }
        x = (float)N_MEMI(xp);
        y = (float)N_MEMI(yp);
    } else {
        x = N_MEMF(a + 4);
        y = N_MEMF(a + 8);
    }
    h = n_font_height(N_MEM32(A->font_now));

    /* A pause page's "Select": the text, and its A icon before it. */
    if (form == TS2_TEXT_AT && A->pause_select && N_MEM32(a) == A->pause_select) {
        float w, x0 = x - 24.0f;

        for (len = 0; len < 0x7F && N_MEM8(str + len); len++)
            ;
        w = n_width_upto(str, len);
        n_drawn_add(DR_HINT, place, 0, 2, x0, y - 2.0f, x + w, y + h);
        if (n_under(place, x0, y - 2.0f, x + w, y + h, 3.0f))
            N_MEM32(a + 4u * (uint32_t)ci) = n_lit(N_MEM32(a + 4u * (uint32_t)ci));
        n_key_call(draw, si);
        return;
    }
    /* A centred or right-aligned line is not a hint line. */
    if (form == TS2_TEXT_AT_ALIGN && N_MEMI(a + 12) != 0) {
        n_key_call(draw, si);
        return;
    }
    for (len = 0; len < 0x7F && N_MEM8(str + len); len++)
        ;
    /* Each hint runs from a button glyph (\x0b and a digit) to the next. */
    for (k = 0; k + 1 < len; k++) {
        uint32_t e, t;
        int8_t g;
        int button;
        float x0, x1;

        if (N_MEM8(str + k) != 0x0B)
            continue;
        g = N_MEM8(str + k + 1u);
        button = g == '1' ? 1 : g == '2' ? 2 : 0;
        for (e = k + 2u; e < len && N_MEM8(str + e) != 0x0B; e++)
            ;
        if (button) {
            for (t = e; t > k + 2u && N_MEM8(str + t - 1u) == ' '; t--)
                ;
            x0 = x + n_width_upto(str, k);
            x1 = x + n_width_upto(str, t);
            n_drawn_add(DR_HINT, place, 0, button, x0, y, x1, y + h);
            if (!hot && n_under(place, x0, y, x1, y + h, 4.0f)) {
                hot = 1;
                hot0 = k;
                hot1 = e;
            }
        }
        k = e - 1u;
    }
    if (!hot) {
        n_key_call(draw, si);
        return;
    }
    if (hot0 == 0 && hot1 >= len) {
        N_MEM32(a + 4u * (uint32_t)ci) = n_lit(N_MEM32(a + 4u * (uint32_t)ci));
        n_key_call(draw, si);
        return;
    }
    n_text_parts(draw, form, si, ci, str, len, hot0, hot1, x);
}

/* The thing drawn last frame under the pointer that a click works: a hint,
 * a page's arrow, a scroll arrow (not an option's arrow: the item under it
 * takes that click). */
static const Drawn *n_drawn_under(void)
{
    int last = g_drawn_cur ^ 1, k;

    for (k = 0; k < g_drawn_n[last]; k++) {
        const Drawn *d = &g_drawn[last][k];

        if (d->kind != DR_ARROW &&
            n_under(d->place, d->x0, d->y0, d->x1, d->y1, d->kind == DR_HINT ? 4.0f : 6.0f))
            return d;
    }
    return NULL;
}

/* ── the pause menu's Inventory page ─────────────────────────── */

/* The row of the Inventory list under (x, y), wholly in view, or -1: the
 * rows as sub_000D7840 draws them for one player. */
static int n_inventory_row(uint32_t s, float x, float y)
{
    uint32_t f = A->frontend, p = N_MEM32(s + TS2_MS_PANELS), cls = N_MEM32(s + TS2_MS_CLASS);
    int count = N_MEMI(f + TS2_MF_INV_COUNT), row;
    float h, top, py0, py1, px0;

    if (!A->inventory || !n_ptr_ok(p) || !n_ptr_ok(cls) || N_MEM32(cls + 4) != A->inventory ||
        N.players != 1 || count <= 0 || count > TS2_INV_MAX)
        return -1;
    h = (float)(int)n_font_height(TS2_INV_FONT);
    if (h < 4.0f)
        return -1;
    px0 = (float)N_MEMI(p + TS2_MP_X);
    py0 = (float)N_MEMI(p + TS2_MP_Y);
    py1 = py0 + (float)N_MEMI(p + TS2_MP_H);
    top = (float)(int)(py0 - N_MEMF(p + TS2_MP_OFFSET));
    if (x < px0 + TS2_INV_X0 || x >= px0 + TS2_INV_X1 || y < py0 || y >= py1)
        return -1;
    row = (int)floorf((y - top) / h);
    if (row < 0 || row >= count)
        return -1;
    if (top + (float)row * h < py0 - 2.0f || top + (float)(row + 1) * h > py1 + 2.0f)
        return -1;
    return row;
}

/* Select a row of the Inventory as its update does when the D-pad moves it:
 * the id, and the move sound. */
static void n_inventory_select(int row)
{
    uint32_t f = A->frontend;
    int32_t id = N_MEMI(f + TS2_MF_INV_IDS + 4u * (uint32_t)row);

    if (N_MEMI(f + TS2_MF_INV_SEL) == id)
        return;
    N_MEMI(f + TS2_MF_INV_SEL) = id;
    n_move_sound();
    if (N.log)
        fprintf(stderr, "[TS2-MENU] frame %u: the pointer selects inventory row %d (id %d)\n",
                N.frame, row, id);
}

/* Select (row, column) in the panel as its update does when the pad moves
 * the selection (sub_000F1900), sound and all. The update keeps the new
 * item scrolled into view by itself, every frame. */
static void n_select(const Hit *h)
{
    uint32_t p = h->panel, old, now;
    int row = N_MEMI(p + TS2_MP_ROW), col = N_MEMI(p + TS2_MP_COLUMN);

    if (row == h->row && col == h->col)
        return;
    now = n_item_at(p, h->row, h->col);
    if (!now || (N_MEM32(now + TS2_MI_FLAGS) & TS2_MI_DISABLED))
        return;
    old = n_item_at(p, row, col);
    if (old)
        N_MEM32(old + TS2_MI_FLAGS) &= ~(uint32_t)TS2_MI_SELECTED;
    N_MEMI(p + TS2_MP_ROW) = h->row;
    N_MEMI(p + TS2_MP_COLUMN) = h->col;
    N_MEM32(now + TS2_MI_FLAGS) |= TS2_MI_SELECTED;
    n_move_sound();
    if (N.log)
        fprintf(stderr, "[TS2-MENU] frame %u: the pointer selects row %d column %d "
                        "(panel %08X item %08X)\n", N.frame, h->row, h->col, p, h->item);
}

/* The log: what was drawn last frame that the pointer can work, when that
 * changes. */
static void n_log_drawn(void)
{
    static int said = -1;
    int last = g_drawn_cur ^ 1, k;

    if (g_drawn_n[last] == said)
        return;
    said = g_drawn_n[last];
    fprintf(stderr, "[TS2-MENU] frame %u: %d drawn to work\n", N.frame, said);
    for (k = 0; k < g_drawn_n[last]; k++) {
        static const char *const kinds[] = { "hint", "option arrow", "scroll arrow", "page arrow" };
        const Drawn *d = &g_drawn[last][k];

        fprintf(stderr, "[TS2-MENU]   %s (placement %d, way %d, button %d) %.0f,%.0f-%.0f,%.0f\n",
                kinds[d->kind], d->place, d->dir, d->button, d->x0, d->y0, d->x1, d->y1);
    }
}

/* The log: a screen's panels and items, once each time the screen changes. */
static void n_log_screen(uint32_t s)
{
    uint32_t p, i, cls = N_MEM32(s + TS2_MS_CLASS);
    int n, k, count;

    fprintf(stderr, "[TS2-MENU] frame %u: screen %08X (flags 0x%X, %d on the stack, update %08X), "
                    "level %u, at %d,%d, left %08X right %08X\n",
            N.frame, s, N_MEM32(s + TS2_MS_FLAGS), N_MEMI(A->frontend + TS2_MF_COUNT),
            n_ptr_ok(cls) ? N_MEM32(cls + 4) : 0, N_MEM32(A->level_id),
            N_MEMI(s + TS2_MS_X), N_MEMI(s + TS2_MS_Y),
            N_MEM32(s + TS2_MS_LEFT), N_MEM32(s + TS2_MS_RIGHT));

    for (p = N_MEM32(s + TS2_MS_PANELS), n = 0; n_ptr_ok(p) && n < 32;
         p = N_MEM32(p + TS2_MP_NEXT), n++) {
        float cl, ct, cr, cb;

        n_clip(p, &cl, &ct, &cr, &cb);
        fprintf(stderr, "[TS2-MENU]   panel %08X flags 0x%X at %d,%d size %dx%d, visible "
                        "%.0f,%.0f-%.0f,%.0f, %d items, %d rows, row %d column %d\n",
                p, N_MEM32(p + TS2_MP_FLAGS), N_MEMI(p + TS2_MP_X), N_MEMI(p + TS2_MP_Y),
                N_MEMI(p + TS2_MP_W), N_MEMI(p + TS2_MP_H), cl, ct, cr, cb,
                N_MEMI(p + TS2_MP_ITEM_COUNT), N_MEMI(p + TS2_MP_ROWS),
                N_MEMI(p + TS2_MP_ROW), N_MEMI(p + TS2_MP_COLUMN));
        count = N_MEMI(p + TS2_MP_ITEM_COUNT);
        for (i = N_MEM32(p + TS2_MP_ITEMS), k = 0; n_ptr_ok(i) && k < count && k < 64;
             i = N_MEM32(i + TS2_MI_NEXT), k++) {
            float x0, y0, x1, y1;
            int row = -1, col = -1;

            n_box(p, i, &x0, &y0, &x1, &y1);
            n_row_col(p, i, &row, &col);
            fprintf(stderr, "[TS2-MENU]     item %08X flags 0x%X box %.0f,%.0f-%.0f,%.0f "
                            "row %d column %d choices %d action %08X%s\n",
                    i, N_MEM32(i + TS2_MI_FLAGS), x0, y0, x1, y1, row, col,
                    N_MEMI(i + TS2_MI_CHOICES), N_MEM32(i + TS2_MI_ACTION),
                    N_MEM32(i + TS2_MI_CHILD) ? " (child)" : "");
        }
    }
}

/* ── presses ─────────────────────────────────────────────────── */

static void n_press(WORD buttons, int a, int b)
{
    N.buttons = buttons;
    N.analog_a = a;
    N.analog_b = b;
    N.until = N.frame + PRESS_FRAMES;
    N.gap_until = N.until + GAP_FRAMES;
}

void ts2_menu_filter(XBOX_GAMEPAD *pad)
{
    if (!N.want || N.frame >= N.until)
        return;
    pad->wButtons |= N.buttons;
    if (N.analog_a)
        pad->bAnalogButtons[XBOX_BUTTON_A] = 255;
    if (N.analog_b)
        pad->bAnalogButtons[XBOX_BUTTON_B] = 255;
}

WORD ts2_skip_button(int *analog)
{
    if (!N.skip_read) {
        const char *v = recomp_config_lookup("RECOMP_TS2_SKIP_BUTTON", "ts2_skip_button");

        N.skip_read = 1;
        N.skip = 0;
        N.skip_analog = 1;                      /* A */
        if (v && !strcmp(v, "b"))
            N.skip_analog = 2;
        else if (v && !strcmp(v, "start"))
            N.skip = XBOX_GAMEPAD_START, N.skip_analog = 0;
        else if (v && !strcmp(v, "back"))
            N.skip = XBOX_GAMEPAD_BACK, N.skip_analog = 0;
    }
    *analog = N.skip_analog;
    return N.skip;
}

/* ── the scripted pointer ────────────────────────────────────── */

/* Menu pixels to the picture, the inverse of n_to_menu. */
static void n_seq_place(void)
{
    unsigned int sw = 0, sh = 0;
    float aspect = 4.0f / 3.0f, u;

    if (xbox_D3D8PictureShape(&sw, &sh) && sw && sh)
        aspect = (float)sw / (float)sh;
    u = aspect > 4.0f / 3.0f + 0.01f
            ? (N.seq_x - TS2_MENU_W / 2.0f) / (TS2_MENU_H * aspect) + 0.5f
            : N.seq_x / TS2_MENU_W;
    recomp_pointer_set(1, u, N.seq_y / TS2_MENU_H, aspect);
}

static void n_seq(void)
{
    while (N.seq && *N.seq) {
        char *end;
        unsigned long at = strtoul(N.seq, &end, 10);

        if (end == N.seq || *end != ':' || at > N.frame)
            return;
        end++;
        if (strncmp(end, "click", 5) == 0) {
            recomp_mouse_button(RECOMP_MOUSE_LEFT, 1);
            recomp_mouse_button(RECOMP_MOUSE_LEFT, 0);
        } else if (strncmp(end, "lhold:", 6) == 0) {
            recomp_mouse_button(RECOMP_MOUSE_LEFT, 1);
            N.seq_hold_until = N.frame + (unsigned)strtoul(end + 6, &end, 10);
        } else if (strncmp(end, "rclick", 6) == 0) {
            recomp_mouse_button(RECOMP_MOUSE_RIGHT, 1);
            recomp_mouse_button(RECOMP_MOUSE_RIGHT, 0);
        } else if (strncmp(end, "wheel:", 6) == 0) {
            recomp_mouse_wheel(strtof(end + 6, NULL));
        } else {
            N.seq_x = strtof(end, &end);
            N.seq_y = *end == ':' ? strtof(end + 1, &end) : N.seq_y;
            N.seq_on = 1;
        }
        while (*end && *end != ',')
            end++;
        N.seq = *end ? end + 1 : NULL;
    }
}

/* ── each frame ──────────────────────────────────────────────── */

/* The pointer, in the menus' 640x480 pixels. */
static void n_to_menu(const RecompPointer *ptr, float *x, float *y)
{
    float aspect = ptr->aspect > 0.1f ? ptr->aspect : 4.0f / 3.0f;

    if (aspect > 4.0f / 3.0f + 0.01f)
        *x = TS2_MENU_W / 2.0f + (ptr->x - 0.5f) * TS2_MENU_H * aspect;
    else
        *x = ptr->x * TS2_MENU_W;
    *y = ptr->y * TS2_MENU_H;
}

/* x for what is drawn at the left edge in widescreen (LEFT), and at the
 * right edge (RIGHT). */
static float n_to_menu_left(const RecompPointer *ptr)
{
    float aspect = ptr->aspect > 0.1f ? ptr->aspect : 4.0f / 3.0f;

    return aspect > 4.0f / 3.0f + 0.01f ? ptr->x * TS2_MENU_H * aspect : ptr->x * TS2_MENU_W;
}

static float n_to_menu_right(const RecompPointer *ptr)
{
    float aspect = ptr->aspect > 0.1f ? ptr->aspect : 4.0f / 3.0f;

    return aspect > 4.0f / 3.0f + 0.01f ? TS2_MENU_W - (1.0f - ptr->x) * TS2_MENU_H * aspect
                                        : ptr->x * TS2_MENU_W;
}

static void n_press_skip(const char *where)
{
    int analog;
    WORD b = ts2_skip_button(&analog);

    n_press(b, analog == 1, analog == 2);
    if (N.log)
        fprintf(stderr, "[TS2-MENU] frame %u: click, %s: skip\n", N.frame, where);
}

/* What the pointer is on at (x, y), in the order a click takes it: a hint,
 * a page's arrow or a scroll arrow drawn last frame, an Inventory row, a
 * line of the menu (with the side of an option's value it is on), or, on a
 * front-end screen with nothing to select, the screen itself. *over says
 * whether a line is under it, with *hit what it is, for hovering. */
static void n_target(uint32_t s, uint32_t top, float x, float y, int in_level,
                     Target *t, Hit *hit, int *over)
{
    const Drawn *d = n_drawn_under();
    float right;

    memset(t, 0, sizeof *t);
    *over = s && n_hit(s, x, y, hit);
    if (d && d->kind == DR_PAGE) {
        t->kind = T_PAGE;
        t->dir = d->dir;
    } else if (d && d->kind == DR_SCROLL) {
        t->kind = T_SCROLL;
        t->dir = d->dir;
    } else if (d && d->kind == DR_HINT) {
        t->kind = T_HINT;
        t->button = d->button;
    } else if ((t->row = n_inventory_row(top, x, y)) >= 0) {
        t->kind = T_INVENTORY;
    } else if (*over) {
        t->kind = T_ITEM;
        t->item = hit->item;
        if (n_option_value(hit->panel, hit->item, &right)) {
            t->dir = n_option_side(hit->panel, hit->item, x);
            if (!t->dir && !N_MEM32(hit->item + TS2_MI_ACTION))
                t->dir = 1;
        }
    } else if (!s && !in_level) {
        t->kind = T_EMPTY;
    }
}

static int n_same(const Target *a, const Target *b)
{
    return a->kind == b->kind && a->dir == b->dir && a->button == b->button &&
           a->row == b->row && a->item == b->item;
}

/* A click on t: the pad press it stands for. */
static void n_act(const Target *t, const Hit *hit)
{
    switch (t->kind) {
    case T_PAGE:
        /* The D-pad's left or right, which changes page. */
        n_press(t->dir < 0 ? XBOX_GAMEPAD_DPAD_LEFT : XBOX_GAMEPAD_DPAD_RIGHT, 0, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on the page arrow: %s\n", N.frame,
                    t->dir < 0 ? "left" : "right");
        break;
    case T_HINT:
        n_press(0, t->button == 2, t->button == 1);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on the hint: %s\n", N.frame,
                    t->button == 2 ? "A" : "B");
        break;
    case T_INVENTORY:
        /* A: the game's own Select, which also closes the pause menu. */
        n_press(0, 1, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on inventory row %d: A\n", N.frame,
                    t->row);
        break;
    case T_EMPTY:
        /* A front-end screen with nothing to select (Press Start, Select
         * Controller, Enter Name, a message): Start goes on from it, so the
         * mouse alone gets through. Not in the pause menu, where Start
         * would close it. */
        n_press(XBOX_GAMEPAD_START, 0, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click, nothing to select: Start\n", N.frame);
        break;
    case T_ITEM:
        /* An option steps the way the click is on its value; on its name,
         * A if it has an action, else on. Anything else is accepted (A). */
        if (t->dir)
            n_press(t->dir < 0 ? XBOX_GAMEPAD_DPAD_LEFT : XBOX_GAMEPAD_DPAD_RIGHT, 0, 0);
        else
            n_press(0, 1, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on row %d column %d: %s\n", N.frame,
                    hit->row, hit->col, t->dir < 0 ? "left" : t->dir > 0 ? "right" : "A");
        break;
    default:
        break;
    }
}

int ts2_menu_esc_backs(void)
{
    return N.want && N.esc_backs;
}

void ts2_menu_frame(void)
{
    RecompPointer ptr;
    uint32_t s, top;
    Hit hit;
    Target cur;
    float x = 0.0f, y = 0.0f;
    int over, moved, down, up, in_level;

    N.frame++;
    /* What was drawn this frame is measured against from now on, and the
     * next frame is noted afresh. */
    g_drawn_cur ^= 1;
    g_drawn_n[g_drawn_cur] = 0;
    N.pin = 0;
    if (!A->frontend)
        return;
    if (!N.started) {
        N.started = 1;
        N.log = recomp_config_bool("RECOMP_TS2_MENU_LOG", NULL, 0);
        N.seq = getenv("RECOMP_TS2_POINTER_SEQ");
        fprintf(stderr, "[TS2-MENU] the mouse points at menu items while a menu is up\n");
    }
    /* The script runs whatever is up: "lhold" is fire in a cutscene. */
    n_seq();
    if (N.seq_hold_until && N.frame >= N.seq_hold_until) {
        recomp_mouse_button(RECOMP_MOUSE_LEFT, 0);
        N.seq_hold_until = 0;
    }

    in_level = N_MEM32(A->level_id) < A->level_menus;
    s = n_menu();
    /* In the front end the pointer stays through the gaps between screens
     * and on the screens with nothing to select, and in a level it stays
     * as long as the game is paused (the pause menu's pages that have no
     * lines to select); otherwise the mouse goes back to the camera. */
    if ((!s && in_level && !(A->pause && N_MEMI(A->pause) > 0)) || ts2_mouse_is_held()) {
        if (N.want) {
            recomp_pointer_want(0);
            N.want = 0;
            if (N.log)
                fprintf(stderr, "[TS2-MENU] frame %u: no menu, the mouse is the camera's\n",
                        N.frame);
        }
        N.screen = 0;
        N.wheel = 0;
        N.scroll_held = 0;
        N.esc_backs = 0;
        N.left_held = 0;
        N.armed.kind = T_NONE;
        return;
    }
    if (!N.want) {
        recomp_pointer_want(1);
        N.want = 1;
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: a menu is up, the mouse points\n", N.frame);
    }
    top = s ? s : n_top_screen();
    if (N.log && top && top != N.screen)
        n_log_screen(top);
    if (N.log)
        n_log_drawn();
    if (top != N.screen)
        N.hover = 0;
    N.screen = top;
    N.players = A->players ? (int)n_guest_call(A->players, NULL, 0) : 1;
    /* In the pause menu, a screen opened from a page (its stack holds two):
     * there Esc (Start, from the keyboard) goes back one step as B does,
     * where Start would close the whole menu (src/ts2_mouse.c). */
    N.esc_backs = in_level && N_MEMI(A->frontend + TS2_MF_COUNT) >= 2;

    if (N.seq_on)
        n_seq_place();
    recomp_pointer_read(&ptr);
    moved = ptr.moved != N.moved;
    N.moved = ptr.moved;
    N.wheel += ptr.wheel;
    if (N.wheel > WHEEL_QUEUE)  N.wheel = WHEEL_QUEUE;
    if (N.wheel < -WHEEL_QUEUE) N.wheel = -WHEEL_QUEUE;

    if (ptr.inside) {
        n_to_menu(&ptr, &x, &y);
        N.pin = 1;
        N.px = x;
        N.py = y;
        N.pxl = n_to_menu_left(&ptr);
        N.pxr = n_to_menu_right(&ptr);
        N.pxs = ptr.x * TS2_MENU_W;
    }
    if (!top) {
        /* The front end with no screen up: a cutscene, or between two. A
         * skip is taken on the press, as fire is in a level. */
        N.wheel = 0;
        N.left_held = (ptr.held & (1u << RECOMP_MOUSE_LEFT)) != 0;
        N.armed.kind = T_NONE;
        if ((ptr.pressed & (1u << RECOMP_MOUSE_LEFT)) && N.frame >= N.gap_until && !in_level)
            n_press_skip("no screen up");
        return;
    }
    /* Nothing while the screen takes no input (it is opening or closing),
     * and a click begun before it is forgotten. */
    if ((N_MEM32(top + TS2_MS_FLAGS) & 2u) || N_MEM8(A->frontend + TS2_MF_LOCK) < 0) {
        N.scroll_held = 0;
        N.left_held = (ptr.held & (1u << RECOMP_MOUSE_LEFT)) != 0;
        N.armed.kind = T_NONE;
        return;
    }

    /* A scroll arrow scrolls for as long as the button is held on it. */
    if (N.scroll_held) {
        const Drawn *d = ptr.inside ? n_drawn_under() : NULL;

        if ((ptr.held & (1u << RECOMP_MOUSE_LEFT)) && d && d->kind == DR_SCROLL) {
            N.left_held = 1;
            N.buttons = N.scroll_held;
            N.analog_a = N.analog_b = 0;
            N.until = N.frame + PRESS_FRAMES;
            N.gap_until = N.until;
            return;
        }
        N.scroll_held = 0;
        N.until = N.frame;
        N.gap_until = N.frame + GAP_FRAMES;
    }

    /* A click acts when the left button comes up over what it went down
     * on, as a click does on a PC (Xbox Accessibility Guideline 107, WCAG
     * 2.5.2): pressing on the wrong line and sliding off before letting go
     * does nothing. A press selects the line it is on at once, so the
     * selection shows what will be taken. A scroll arrow is the exception:
     * it scrolls from the press and for as long as the button is held. */
    over = 0;
    if (ptr.inside)
        n_target(s, top, x, y, in_level, &cur, &hit, &over);
    else
        memset(&cur, 0, sizeof cur);
    down = (ptr.pressed & (1u << RECOMP_MOUSE_LEFT)) != 0;
    up = (N.left_held || down) && !(ptr.held & (1u << RECOMP_MOUSE_LEFT));
    N.left_held = (ptr.held & (1u << RECOMP_MOUSE_LEFT)) != 0;

    /* The Inventory page: its rows are not items. The pointer selects the
     * row it moves onto or presses on. */
    if (cur.kind == T_INVENTORY && (moved || down))
        n_inventory_select(cur.row);
    /* A press always selects what it is on: the keys may have moved the
     * selection away from the item the pointer last selected. */
    if (over && ((moved && hit.item != N.hover) || down)) {
        n_select(&hit);
        N.hover = hit.item;
    }
    if (!over && moved)
        N.hover = 0;

    if (down) {
        N.armed = cur;
        if (cur.kind == T_SCROLL && N.frame >= N.gap_until) {
            N.scroll_held = cur.dir == TS2_ARROW_UP ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN;
            n_press(N.scroll_held, 0, 0);
            if (N.log)
                fprintf(stderr, "[TS2-MENU] frame %u: press on the scroll arrow: %s\n", N.frame,
                        cur.dir == TS2_ARROW_UP ? "up" : "down");
            N.armed.kind = T_NONE;
            return;
        }
    }
    if (up) {
        Target armed = N.armed;

        N.armed.kind = T_NONE;
        if (armed.kind != T_NONE && armed.kind != T_SCROLL && n_same(&armed, &cur) &&
            N.frame >= N.gap_until) {
            n_act(&cur, &hit);
            return;
        }
        if (armed.kind != T_NONE && N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: let go away from what was pressed: nothing\n",
                    N.frame);
    }

    /* The right button does nothing in a menu: no guideline gives it a
     * meaning there, and Esc, Backspace and the clickable Back hint go back
     * (the user's decision, after the conventions write-up). The wheel is
     * the D-pad's up and down: it moves the selection, or scrolls a page
     * with nothing to select; in the front end's screens with nothing to
     * select it does nothing. One press at a time, with a gap after it so
     * each is seen. */
    if (N.frame < N.gap_until || !N.wheel)
        return;
    if (!s && !in_level) {
        N.wheel = 0;
        return;
    }
    n_press(N.wheel > 0 ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN, 0, 0);
    if (N.log)
        fprintf(stderr, "[TS2-MENU] frame %u: wheel: D-pad %s\n", N.frame,
                N.wheel > 0 ? "up" : "down");
    N.wheel += N.wheel > 0 ? -1 : 1;
}
