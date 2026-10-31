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
 *     game lights the line selected: a hint pulses white to cyan inside the
 *     style's deep blue outline (n_draw_lit), the page arrows pulse, an
 *     option's arrow goes between its own shade and the one it has while
 *     that way is held;
 *   - a click on a pause page's button picture ("Select", "Back") or a
 *     piece of its D-pad picture ("Change page") is that button or way, and
 *     a panel's own scroll arrows (a long list's, the Character Select
 *     list's) scroll it as a page's do;
 *   - while the keyboard and mouse are in use, the hints and pictures name
 *     the keys that do the same ("[Enter] Select", "[Esc] Back"), a line
 *     grown too wide for its panel stepping down to a narrower font as the
 *     game's own fitting does; a pad's press brings the pictures back;
 *   - on the pause menu's Inventory page, the pointer selects the row it
 *     moves onto and a click takes it (the game's Select);
 *   - in MapMaker's editor, src/ts2_mapmaker_mouse.c takes the pointer:
 *     the hand follows it (the tabs' arrows and the prompts are clicked
 *     from here);
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
#include "ts2_mapmaker_mouse.h"
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
#define N_MEM16(a) (*(volatile uint16_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

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
    volatile int  analog_xy;     /* XBOX_BUTTON_X or _Y + 1, or 0 */
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
    int      no_keys;        /* measuring a line as it is, with no keys written in */
    int      fitting;        /* inside a line fitted to its panel (n_key_fit) */
    /* A pause page's prompt text, as drawn this frame: its colours, for the
     * key written in place of its button's picture after it. */
    uint32_t prompt_col, prompt_col2;
    unsigned prompt_frame;
    int      prompt_lit;
    /* How far the key name just drawn runs past its button's picture. */
    float    icon_over;
    unsigned icon_over_frame;
    /* The D-pad's picture being drawn (ts2_menu_dpad): its corner and the
     * ways it offers (up, down, left, right). */
    int      mapmaker_ui;    /* MapMaker's placement was asked for */
    int      dpad_on;
    float    dpad_x, dpad_y;
    int      dpad_way[4];
} N;

/* ── what the menus drew ─────────────────────────────────────── */

/* A Drawn's button for a thing with no slack round it. */
#define TS2_DRAWN_EXACT 0xFF

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
    uint8_t button;          /* DR_HINT: 1 B, 2 A, 3 Y, 4 X */
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

uint32_t ts2_menu_frontend(void)
{
    return A->frontend;
}

uint32_t ts2_menu_level(void)
{
    return A->level_id ? N_MEM32(A->level_id) : 0;
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

/* A sprite's corners as the game passes them, (x0, y0, x1, y1) ints at a,
 * put in order: the game mirrors a sprite by giving its corners the other
 * way round (the pause pages' left arrow, a panel's down arrow), and a box
 * measured from its right to its left holds nothing. */
static void n_corners(uint32_t a, float *x0, float *y0, float *x1, float *y1)
{
    float t;

    *x0 = (float)N_MEMI(a);
    *y0 = (float)N_MEMI(a + 4);
    *x1 = (float)N_MEMI(a + 8);
    *y1 = (float)N_MEMI(a + 12);
    if (*x1 < *x0) { t = *x0; *x0 = *x1; *x1 = t; }
    if (*y1 < *y0) { t = *y0; *y0 = *y1; *y1 = t; }
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
    } else if (ts2_mapmaker_active() && (dir == TS2_ARROW_LEFT || dir == TS2_ARROW_RIGHT)) {
        /* MapMaker's screens of rows (Map Settings, Editor Settings): an
         * arrow beside a row's value. A click picks its row and is the
         * D-pad's left or right, which steps the value (Drawn button 1: a
         * row's arrow, not a page's). */
        n_corners(a + 4, &x0, &y0, &x1, &y1);
        n_drawn_add(DR_PAGE, TS2_PLACE_AUTO, dir == TS2_ARROW_LEFT ? -1 : 1, 1, x0, y0, x1, y1);
        if (n_under(TS2_PLACE_AUTO, x0, y0, x1, y1, 4.0f))
            N_MEM32(a + 20) = n_lit(N_MEM32(a + 20));
        return;
    } else {
        return;
    }
    n_corners(a + 4, &x0, &y0, &x1, &y1);
    n_drawn_add(kind, place, dir, 0, x0, y0, x1, y1);
    if (n_under(place, x0, y0, x1, y1, 4.0f))
        N_MEM32(a + 20) = kind == DR_ARROW ? n_arrow_lit(N_MEM32(a + 20)) : n_lit(N_MEM32(a + 20));
}

/* The D-pad picture's pieces, from its corner (sub_000AB2A0): up, down,
 * left, right, each 16 square. */
static const int8_t k_dpad_piece[4][2] = { { 12, 0 }, { 12, 24 }, { 0, 12 }, { 24, 12 } };

static void n_dpad_box(int k, float *x0, float *y0, float *x1, float *y1)
{
    *x0 = N.dpad_x + (float)k_dpad_piece[k][0];
    *y0 = N.dpad_y + (float)k_dpad_piece[k][1];
    *x1 = *x0 + 16.0f;
    *y1 = *y0 + 16.0f;
}

void ts2_menu_sprite(void)
{
    uint32_t a = g_esp, ret;
    float x0, y0, x1, y1;

    if (!N.want)
        return;
    ret = N_MEM32(a);
    n_corners(a + 4, &x0, &y0, &x1, &y1);
    /* A piece of the D-pad's picture: lit under the pointer when that way
     * does something (ts2_menu_dpad notes it). */
    if (N.dpad_on) {
        float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f, bx0, by0, bx1, by1;
        int k;

        for (k = 0; k < 4; k++) {
            n_dpad_box(k, &bx0, &by0, &bx1, &by1);
            if (N.dpad_way[k] && cx >= bx0 && cx < bx1 && cy >= by0 && cy < by1 &&
                n_under(TS2_PLACE_AUTO, bx0, by0, bx1, by1, 2.0f))
                N_MEM32(a + 0x24) = n_lit(N_MEM32(a + 0x24));
        }
        return;
    }
    /* A panel's scroll arrows (the Character Select list's). */
    if (ret && (ret == A->panel_arrow_up || ret == A->panel_arrow_down)) {
        n_drawn_add(DR_SCROLL, TS2_PLACE_CENTRE,
                    ret == A->panel_arrow_up ? TS2_ARROW_UP : TS2_ARROW_DOWN, 0, x0, y0, x1, y1);
        if (n_under(TS2_PLACE_CENTRE, x0, y0, x1, y1, 4.0f))
            N_MEM32(a + 0x24) = n_lit(N_MEM32(a + 0x24));
        return;
    }
    if (!A->page_draw || ret < A->page_draw || ret >= A->page_draw_end)
        return;
    n_drawn_add(DR_PAGE, TS2_PLACE_AUTO, x0 + x1 < TS2_MENU_W ? -1 : 1, 0, x0, y0, x1, y1);
    if (n_under(TS2_PLACE_AUTO, x0, y0, x1, y1, 6.0f))
        N_MEM32(a + 0x24) = n_lit(N_MEM32(a + 0x24));
}

/* A sprite by id, (float x, float y, id, colour), sized by its texture:
 * MapMaker's tab arrows, the D-pad's left and right as a pause page's. */
void ts2_menu_sprite_id(void)
{
    uint32_t a = g_esp, ret, rec;
    float x0, y0, x1, y1;

    if (!N.want || !A->tab_left)
        return;
    ret = N_MEM32(a);
    if (ret != A->tab_left && ret != A->tab_right)
        return;
    rec = N_MEM32(N_MEM32(A->texture_records)) +
          N_MEM32(A->sprite_textures + 4u * N_MEM32(a + 12)) * 0x34u;
    x0 = N_MEMF(a + 4);
    y0 = N_MEMF(a + 8);
    x1 = x0 + (float)N_MEM16(rec + 0x26);
    y1 = y0 + (float)N_MEM16(rec + 0x2A);
    n_drawn_add(DR_PAGE, TS2_PLACE_AUTO, ret == A->tab_left ? -1 : 1, 0, x0, y0, x1, y1);
    if (n_under(TS2_PLACE_AUTO, x0, y0, x1, y1, 4.0f))
        N_MEM32(a + 16) = n_lit(N_MEM32(a + 16));
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
#define KEY_TEXT_RING 6

/* The key named for the button the game writes as \x0b and digit d: B is
 * Esc and A is Enter in a menu (the conventions: Windows pairs Escape with
 * back and Enter with accept), Y and X the keys bound to them, if any.
 * NULL to keep the pad's picture. */
static const char *n_key_name(int d)
{
    static char y[24], x[24];
    static int read;
    char k[16];

    if (!read) {
        read = 1;
        if (recomp_bindings_key_name(0, "y", k, sizeof k))
            snprintf(y, sizeof y, "[%s]", k);
        if (recomp_bindings_key_name(0, "x", k, sizeof k))
            snprintf(x, sizeof x, "[%s]", k);
    }
    switch (d) {
    case 1: return ts2_mapmaker_b_key() ? ts2_mapmaker_b_key() : "[Esc]";
    case 2: return "[Enter]";
    case 3: return y[0] ? y : NULL;
    case 4: return x[0] ? x : NULL;
    default: return NULL;
    }
}

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

    if (N.no_keys || !N.want || !recomp_input_using_keys() || !n_ptr_ok(str))
        return 0;
    for (k = 0; k < 0x7F && N_MEM8(str + k); k++)
        if (N_MEM8(str + k) == 0x0B && n_key_name(N_MEM8(str + k + 1u) - '0'))
            any = 1;
    if (!any || !(buf = n_key_buffer()))
        return 0;
    for (k = 0; k < 0x7F && N_MEM8(str + k) && o + 20u < KEY_TEXT_LEN; k++) {
        int8_t c = N_MEM8(str + k), d = N_MEM8(str + k + 1u);
        const char *n = c == 0x0B ? n_key_name(d - '0') : NULL;

        if (n) {
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

static uint32_t n_raw_width(uint32_t str)
{
    uint32_t w;

    N.no_keys++;
    w = n_guest_call(A->text_width_raw ? A->text_width_raw : A->text_width, &str, 1);
    N.no_keys--;
    return w < 4096u ? w : 0u;
}

/* A menu item's line with keys written in is wider than the line the game
 * laid out (a key's name against a button's picture), and a narrow panel
 * cuts it off (Select Controller's "Select  Back"). The game fits a line by
 * stepping down through its item fonts, 21 to 19, each a tenth narrower
 * (sub_001C29F0); so is this one, while it is wider than the game's by
 * more than a few pixels. Returns whether the font was changed; *keep holds
 * what to put back. */
static int n_key_fit(uint32_t e0, uint32_t str, uint32_t kt, uint32_t keep[3])
{
    uint32_t ret = N_MEM32(e0), f, wo, wk;
    int k;

    if (N.fitting || !A->font_select || !A->font_now || !A->item_draw ||
        ret < A->item_draw || ret >= A->item_draw_end)
        return 0;
    f = N_MEM32(A->font_now);
    if (f <= TS2_FONT_FIT_MIN || f > TS2_FONT_FIT_MAX)
        return 0;
    wo = n_raw_width(str);
    wk = n_raw_width(kt);
    if (!wo || wk <= wo + 6u)
        return 0;
    for (k = 0; k < 3; k++)
        keep[k] = N_MEM32(A->font_now + 4u * (uint32_t)k);
    while (f > TS2_FONT_FIT_MIN && wk > wo + 6u) {
        f--;
        n_guest_call(A->font_select, &f, 1);
        wk = n_raw_width(kt);
    }
    return 1;
}

/* Calls fn (a lifted text function) with its string argument, slot si on
 * the stack, written with keys for the buttons; the argument is put back
 * after. Leaves the stack as fn does. */
static void n_key_call(void (*fn)(void), int si)
{
    uint32_t e0 = g_esp, str = N_MEM32(e0 + 4u * (uint32_t)si), kt = n_key_text(str);
    uint32_t keep[3];
    int fit, k;

    if (!kt) {
        fn();
        return;
    }
    fit = n_key_fit(e0, str, kt, keep);
    g_esp = e0;
    N_MEM32(e0 + 4u * (uint32_t)si) = kt;
    fn();
    N_MEM32(e0 + 4u * (uint32_t)si) = str;
    if (fit)
        for (k = 0; k < 3; k++)
            N_MEM32(A->font_now + 4u * (uint32_t)k) = keep[k];
}

/* The outline the game draws round a selected line: its style's (deep
 * blue), at the line's own alpha. */
static uint32_t n_outline(uint32_t colour)
{
    uint32_t r = 0, g = 0, b = 80;

    if (A->styles) {
        uint32_t st = A->styles + TS2_STYLE_SIZE;     /* style 1, the panels' */

        r = N_MEM32(st + TS2_STYLE_OUTLINE) & 0xFFu;
        g = N_MEM32(st + TS2_STYLE_OUTLINE + 4) & 0xFFu;
        b = N_MEM32(st + TS2_STYLE_OUTLINE + 8) & 0xFFu;
    }
    return r << 24 | g << 16 | b << 8 | (colour & 0xFFu);
}

/* Draws a line lit as the game draws its selected line: the pulsing colour
 * (already in slot ci) inside the style's outline, which is the line drawn
 * four times around itself, one pixel up and down and two left and right,
 * the button pictures left out (sub_001ACA50's own way, which that form
 * does itself given the outline's colour). The stack is as fn has it; it
 * is left as one call of fn leaves it. */
static void n_draw_lit(void (*fn)(void), int form, int si, int ci)
{
    static const int8_t off[4][2] = { { -2, 0 }, { 0, -1 }, { 2, 0 }, { 0, 1 } };
    uint32_t e0 = g_esp, col = N_MEM32(e0 + 4u * (uint32_t)ci), out = n_outline(col);
    uint32_t keep[6], xp = 0, yp = 0, flag;
    int32_t xi = 0, yi = 0;
    float xf = 0.0f, yf = 0.0f;
    int k, j;

    if (form == TS2_TEXT_AT) {
        N_MEM32(e0 + 4u * (uint32_t)(ci + 1)) = out;
        n_key_call(fn, si);
        return;
    }
    if (!A->outline_flag) {
        n_key_call(fn, si);
        return;
    }
    for (j = 0; j < 6; j++)
        keep[j] = N_MEM32(e0 + 4u + 4u * (uint32_t)j);
    if (form == TS2_TEXT_AT_INT) {
        xp = keep[0];
        yp = keep[1];
        if (!n_ptr_ok(xp) || !n_ptr_ok(yp)) {
            n_key_call(fn, si);
            return;
        }
        xi = N_MEMI(xp);
        yi = N_MEMI(yp);
    } else {
        xf = N_MEMF(e0 + 4);
        yf = N_MEMF(e0 + 8);
    }
    flag = N_MEM32(A->outline_flag);
    for (k = 0; k <= 4; k++) {
        int dx = k < 4 ? off[k][0] : 0, dy = k < 4 ? off[k][1] : 0;

        for (j = 0; j < 6; j++)
            N_MEM32(e0 + 4u + 4u * (uint32_t)j) = keep[j];
        if (form == TS2_TEXT_AT_INT) {
            N_MEMI(xp) = xi + dx;
            N_MEMI(yp) = yi + dy;
        } else {
            N_MEMF(e0 + 4) = xf + (float)dx;
            N_MEMF(e0 + 8) = yf + (float)dy;
        }
        N_MEM32(e0 + 4u * (uint32_t)ci) = k < 4 ? out : col;
        N_MEM32(A->outline_flag) = k < 4 ? 1u : flag;
        g_esp = e0;
        n_key_call(fn, si);
    }
    for (j = 0; j < 6; j++)
        N_MEM32(e0 + 4u + 4u * (uint32_t)j) = keep[j];
    if (form != TS2_TEXT_AT_INT)
        N_MEMF(e0 + 4) = xf, N_MEMF(e0 + 8) = yf;
    g_esp = e0 + 4u;
}

void ts2_key_text(void (*fn)(void), int si)
{
    n_key_call(fn, si);
}

/* A coordinate the game passes as an int or as a float: the pictures'
 * helpers are not all alike, and no screen coordinate is 8192 or more. */
static int n_hint_hot(int button, float y0, float y1);

static float n_arg_coord(uint32_t a)
{
    int32_t v = N_MEMI(a);

    return v > -8192 && v < 8192 ? (float)v : N_MEMF(a);
}

static uint32_t n_f2u(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof u);
    return u;
}

/* A button's picture at (x0, y0)-(x1, y1), icon 0 B, 1 A, 2 Y, 3 X: noted
 * as that button's hint, and drawn by fn as it is (lit_slot, when the
 * picture has one, set while the pointer is over it), or, while the
 * keyboard and mouse drive the menu, as the key's name in its place, in
 * the given colours: ending where the picture ended (align 0), centred on
 * it (1), or starting where it started (2). A button that does nothing now (live 0, drawn dim) is neither
 * noted nor lit. The stack is fn's; it is left as one call of fn leaves
 * it. */
static void n_icon(void (*fn)(void), float x0, float y0, float x1, float y1, int icon,
                   int align, int live, uint32_t lit_slot, uint32_t col, uint32_t col2)
{
    uint32_t a = g_esp, buf, keep[3], args[5], f = TS2_FONT_PROMPTS;
    float h, w, tx, ty;
    int button, k, lit;
    const char *name;

    button = icon == 0 ? 1 : icon == 1 ? 2 : icon == 2 ? 3 : icon == 3 ? 4 : 0;
    name = button && recomp_input_using_keys() ? n_key_name(button) : NULL;
    if (!name || !A->text_outlined || !A->font_select || !A->font_now || !(buf = n_key_buffer())) {
        if (button && live) {
            n_drawn_add(DR_HINT, TS2_PLACE_AUTO, 0, button, x0, y0, x1, y1);
            if (lit_slot && (n_under(TS2_PLACE_AUTO, x0, y0, x1, y1, 2.0f) ||
                             n_hint_hot(button, y0, y1)))
                N_MEM32(a + lit_slot) = 1;
        }
        fn();
        return;
    }
    for (k = 0; name[k] && k < KEY_TEXT_LEN - 1; k++)
        N_MEM8(buf + (uint32_t)k) = name[k];
    N_MEM8(buf + (uint32_t)k) = 0;
    for (k = 0; k < 3; k++)
        keep[k] = N_MEM32(A->font_now + 4u * (uint32_t)k);
    n_guest_call(A->font_select, &f, 1);
    h = n_font_height(TS2_FONT_PROMPTS);
    w = (float)n_raw_width(buf);
    tx = align == 1 ? (x0 + x1 - w) * 0.5f : align == 2 ? x0 : x1 - w;
    ty = y0 + ((y1 - y0) - h) * 0.5f;
    lit = (N.prompt_lit && N.prompt_frame == N.frame && !align) ||
          n_under(TS2_PLACE_AUTO, tx, ty, tx + w, ty + h, 3.0f) || n_hint_hot(button, ty, ty + h);
    if (!live) {
        col = (col & 0xFFFFFF00u) | 0x3Fu;
        col2 = (col2 & 0xFFFFFF00u) | 0x3Fu;
    } else {
        if (lit) {
            col = n_lit(col);
            col2 = n_outline(col);
        }
        n_drawn_add(DR_HINT, TS2_PLACE_AUTO, 0, button, tx, ty, tx + w, ty + h);
    }
    /* Its label, drawn next, goes out by what the name overflows. */
    N.icon_over = align != 1 && w > x1 - x0 ? w - (x1 - x0) : 0.0f;
    N.icon_over_frame = N.frame;
    args[0] = n_f2u(tx);
    args[1] = n_f2u(ty);
    args[2] = buf;
    args[3] = col;
    args[4] = col2;
    n_guest_call(A->text_outlined, args, 5);
    for (k = 0; k < 3; k++)
        N_MEM32(A->font_now + 4u * (uint32_t)k) = keep[k];
    g_esp = a + 4u;                     /* its body not run: its return, as it would */
}

void ts2_menu_icon(void (*fn)(void))
{
    uint32_t a = g_esp, col, col2;
    float x0, y0, x1, y1, t;

    if (!N.want || !A->frontend) {
        fn();
        return;
    }
    x0 = n_arg_coord(a + 4);
    y0 = n_arg_coord(a + 8);
    x1 = n_arg_coord(a + 12);
    y1 = n_arg_coord(a + 16);
    if (x1 < x0) { t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { t = y0; y0 = y1; y1 = t; }
    /* The key in the colours of the prompt's text, drawn just before. */
    if (N.prompt_frame == N.frame) {
        col = N.prompt_col;
        col2 = N.prompt_col2;
    } else {
        uint32_t alpha = (uint32_t)N_MEMI(a + 24) & 0xFFu;

        col = 0x64646400u | alpha;
        col2 = alpha;
    }
    n_icon(fn, x0, y0, x1, y1, N_MEMI(a + 20), 0, 1, 0, col, col2);
}

void ts2_menu_icon_at(void (*fn)(void))
{
    uint32_t a = g_esp;
    float x, y;
    int icon;

    if (!N.want || !A->frontend) {
        fn();
        return;
    }
    x = n_arg_coord(a + 4);
    y = n_arg_coord(a + 8);
    /* A diamond: a name wider than its picture goes out to its own side. */
    icon = N_MEMI(a + 12);
    n_icon(fn, x, y, x + 24.0f, y + 22.0f, icon, icon == 3 ? 0 : icon == 0 ? 2 : 1,
           N_MEMI(a + 16) != 0, 16, 0x6464647Fu, 0x0000007Fu);
}

void ts2_menu_dpad(void (*fn)(void))
{
    uint32_t a = g_esp;
    float x0, y0, x1, y1;
    int k, dim;

    if (!N.want || !A->frontend) {
        fn();
        return;
    }
    N.dpad_x = n_arg_coord(a + 4);
    N.dpad_y = n_arg_coord(a + 8);
    /* The game gives each way an alpha, never 0 while it is drawn: the way
     * that does something is brighter than the one that does not. */
    dim = N_MEMI(a + 12);
    for (k = 1; k < 4; k++)
        if (N_MEMI(a + 12u + 4u * (uint32_t)k) < dim)
            dim = N_MEMI(a + 12u + 4u * (uint32_t)k);
    for (k = 0; k < 4; k++) {
        N.dpad_way[k] = N_MEMI(a + 12u + 4u * (uint32_t)k) > dim;
        if (!N.dpad_way[k])
            continue;
        n_dpad_box(k, &x0, &y0, &x1, &y1);
        /* Up and down scroll (held, as a page's scroll arrow), left and
         * right change page (as a page's arrow): the D-pad's own ways. */
        if (k < 2)
            n_drawn_add(DR_SCROLL, TS2_PLACE_AUTO, k == 0 ? TS2_ARROW_UP : TS2_ARROW_DOWN,
                        TS2_DRAWN_EXACT, x0, y0, x1, y1);
        else
            n_drawn_add(DR_PAGE, TS2_PLACE_AUTO, k == 2 ? -1 : 1, TS2_DRAWN_EXACT,
                        x0, y0, x1, y1);
    }
    N.dpad_on = 1;
    fn();
    N.dpad_on = 0;
}

/* Whether the pointer was over a hint for this button drawn last frame on
 * the same line (y0..y1): a prompt's text and the icon or key drawn beside
 * it are noted apart, and either lights both. */
static int n_hint_hot(int button, float y0, float y1)
{
    int last = g_drawn_cur ^ 1, k;

    for (k = 0; k < g_drawn_n[last]; k++) {
        const Drawn *d = &g_drawn[last][k];

        if (d->kind == DR_HINT && d->button == button && d->y1 > y0 && d->y0 < y1 &&
            n_under(d->place, d->x0, d->y0, d->x1, d->y1, 3.0f))
            return 1;
    }
    return 0;
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
        if (k == 1)
            n_draw_lit(draw, form, si, ci);
        else
            n_key_call(draw, si);
        N_MEM8(str + p1) = end;
        at += w;
    }
    for (j = 0; j < 6; j++)
        N_MEM32(e0 + 4u + 4u * (uint32_t)j) = keep[j];
    g_esp = e0 + 4u;
}

static void n_menu_text(void (*draw)(void), int form, int place)
{
    uint32_t a = g_esp, str, len, k, hot0 = 0, hot1 = 0;
    int si = form == TS2_TEXT_AT_ALIGN ? 4 : 3, ci = si + 1, hot = 0, prompts = 0;
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

    /* The label beside a prompt's key name (n_icon). */
    if (form == TS2_TEXT_AT && N.icon_over_frame == N.frame && N.icon_over > 0.0f &&
        (N_MEM32(a) == A->prompt_label_left || N_MEM32(a) == A->prompt_label_right)) {
        x += N_MEM32(a) == A->prompt_label_left ? -N.icon_over : N.icon_over;
        N_MEMF(a + 4) = x;
    }

    /* A pause page's "Select" or "Back": the text, and its button's icon
     * before it (ts2_menu_icon notes that, or the key in its place). Lit,
     * with the icon, when the pointer is over either. */
    if (form == TS2_TEXT_AT && N_MEM32(a) &&
        (N_MEM32(a) == A->pause_select || N_MEM32(a) == A->pause_back)) {
        int button = N_MEM32(a) == A->pause_select ? 2 : 1;
        float w, x0 = x - 24.0f;

        for (len = 0; len < 0x7F && N_MEM8(str + len); len++)
            ;
        w = n_width_upto(str, len);
        n_drawn_add(DR_HINT, place, 0, button, x0, y - 2.0f, x + w, y + h);
        N.prompt_lit = n_under(place, x0, y - 2.0f, x + w, y + h, 3.0f) ||
                       n_hint_hot(button, y - 2.0f, y + h);
        N.prompt_col = N_MEM32(a + 4u * (uint32_t)ci);
        N.prompt_col2 = N_MEM32(a + 4u * (uint32_t)(ci + 1));
        N.prompt_frame = N.frame;
        if (N.prompt_lit) {
            N_MEM32(a + 4u * (uint32_t)ci) = n_lit(N.prompt_col);
            n_draw_lit(draw, form, si, ci);
            return;
        }
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
    /* Y's and X's hints (\x0b3, \x0b4: Select Level's "View briefing")
     * count in the front end, and in a level on a line of prompts, one with
     * A's or B's too; alone there, the picture is a pad's legend (the pause
     * menu's Controls page), and nothing to click. */
    prompts = !A->level_id || N_MEM32(A->level_id) >= A->level_menus;
    for (k = 0; k + 1 < len; k++)
        if (N_MEM8(str + k) == 0x0B && (N_MEM8(str + k + 1u) == '1' || N_MEM8(str + k + 1u) == '2'))
            prompts = 1;
    /* Each hint runs from a button glyph (\x0b and a digit) to the next. */
    for (k = 0; k + 1 < len; k++) {
        uint32_t e, t;
        int8_t g;
        int button;
        float x0, x1;

        if (N_MEM8(str + k) != 0x0B)
            continue;
        g = N_MEM8(str + k + 1u);
        button = g == '1' ? 1 : g == '2' ? 2 : !prompts ? 0 : g == '3' ? 3 : g == '4' ? 4 : 0;
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
        n_draw_lit(draw, form, si, ci);
        return;
    }
    n_text_parts(draw, form, si, ci, str, len, hot0, hot1, x);
}

/* The fit holds for the whole call: the hints' boxes are measured, and a lit
 * hint drawn, in the font the line is drawn in. */
void ts2_menu_text(void (*draw)(void), int form, int place)
{
    uint32_t a = g_esp, str, kt, keep[3];
    int si = form == TS2_TEXT_AT_ALIGN ? 4 : 3, fit = 0, k;

    if (N.want && A->frontend && A->text_width && !N.fitting) {
        str = N_MEM32(a + 4u * (uint32_t)si);
        if (n_ptr_ok(str) && (kt = n_key_text(str)) != 0)
            fit = n_key_fit(a, str, kt, keep);
    }
    N.fitting += fit;
    n_menu_text(draw, form, place);
    N.fitting -= fit;
    if (fit)
        for (k = 0; k < 3; k++)
            N_MEM32(A->font_now + 4u * (uint32_t)k) = keep[k];
}

/* The thing drawn last frame under the pointer that a click works: a hint,
 * a page's arrow, a scroll arrow (not an option's arrow: the item under it
 * takes that click). */
static const Drawn *n_drawn_under(void)
{
    int last = g_drawn_cur ^ 1, k, pass;

    /* What the pointer is on first, then what it is near. The D-pad's pieces
     * lie closer together than the slack, and a dim piece beside a live one
     * must not click it, so they are only ever what the pointer is on. */
    for (pass = 0; pass < 2; pass++) {
        for (k = 0; k < g_drawn_n[last]; k++) {
            const Drawn *d = &g_drawn[last][k];
            float slack = !pass ? 0.0f : d->kind == DR_HINT ? 4.0f : 6.0f;

            if (pass && d->button == TS2_DRAWN_EXACT)
                continue;
            if (d->kind != DR_ARROW && n_under(d->place, d->x0, d->y0, d->x1, d->y1, slack))
                return d;
        }
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
    N.analog_xy = 0;
    N.buttons = buttons;
    N.analog_a = a;
    N.analog_b = b;
    N.until = N.frame + PRESS_FRAMES;
    N.gap_until = N.until + GAP_FRAMES;
}

void ts2_menu_filter(XBOX_GAMEPAD *pad)
{
    ts2_mapmaker_filter(pad);
    if (!N.want || N.frame >= N.until)
        return;
    pad->wButtons |= N.buttons;
    if (N.analog_a)
        pad->bAnalogButtons[XBOX_BUTTON_A] = 255;
    if (N.analog_b)
        pad->bAnalogButtons[XBOX_BUTTON_B] = 255;
    if (N.analog_xy)
        pad->bAnalogButtons[N.analog_xy - 1] = 255;
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
        N.analog_xy = t->button == 3 ? XBOX_BUTTON_Y + 1 : t->button == 4 ? XBOX_BUTTON_X + 1 : 0;
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on the hint: %s\n", N.frame,
                    t->button == 2 ? "A" : t->button == 1 ? "B" : t->button == 3 ? "Y" : "X");
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
    int over, moved, down, up, in_level, mapmaker;

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
    mapmaker = ts2_mapmaker_active();
    /* MapMaker's editor in widescreen: everything in proportion, so the
     * grid, the hand and the pointer agree (src/recomp_manual.c). */
    if (mapmaker != N.mapmaker_ui) {
        N.mapmaker_ui = mapmaker;
        ts2_ui_area_default(mapmaker ? TS2_PLACE_CENTRE : -1);
    }
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
    /* MapMaker's editor and its screens that are not panel menus: a tab's
     * arrow or a prompt drawn last frame takes a click as in a menu;
     * everything else is the editor's (src/ts2_mapmaker_mouse.c). */
    if (mapmaker && !s) {
        const Drawn *d = ptr.inside ? n_drawn_under() : NULL;
        int took;

        memset(&cur, 0, sizeof cur);
        memset(&hit, 0, sizeof hit);
        if (d && d->kind == DR_PAGE) {
            cur.kind = T_PAGE;
            cur.dir = d->dir;
            /* A row's arrow: the row at its height is picked under it, so
             * the D-pad steps that row's value. */
            if (d->button == 1) {
                cur.row = (int)((d->y0 + d->y1) * 0.5f);
                if (moved || (ptr.pressed & (1u << RECOMP_MOUSE_LEFT)))
                    ts2_mapmaker_pick_row_at(top, (float)cur.row);
            }
        } else if (d && d->kind == DR_HINT) {
            cur.kind = T_HINT;
            cur.button = d->button;
        }
        down = (ptr.pressed & (1u << RECOMP_MOUSE_LEFT)) != 0;
        up = (N.left_held || down) && !(ptr.held & (1u << RECOMP_MOUSE_LEFT));
        N.left_held = (ptr.held & (1u << RECOMP_MOUSE_LEFT)) != 0;
        if (top && ((N_MEM32(top + TS2_MS_FLAGS) & 2u) || N_MEM8(A->frontend + TS2_MF_LOCK) < 0)) {
            N.armed.kind = T_NONE;      /* a screen opening or closing */
            return;
        }
        if (down)
            N.armed = cur;
        took = N.armed.kind != T_NONE;
        if (up) {
            Target armed = N.armed;

            N.armed.kind = T_NONE;
            if (armed.kind != T_NONE && n_same(&armed, &cur) && N.frame >= N.gap_until)
                n_act(&cur, &hit);
        }
        N.wheel = 0;                    /* the editor's own (zoom), read from ptr */
        ts2_mapmaker_frame(top, &ptr, x, y, ptr.inside, moved, took);
        return;
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
