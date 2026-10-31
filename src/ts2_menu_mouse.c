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
 *     action, its sound, the next screen), or, for an item whose value is
 *     stepped with left and right (an option), steps it on;
 *   - the right button is B (back), and the wheel the D-pad's up and down,
 *     which scrolls a long list.
 *
 * A menu is up when player 1's stack of screens has one on top that runs
 * and is shown, and there is a panel in it with the focus: in the front end
 * any time, in a level only while the game is paused (the pause menu). The
 * front end keeps the pointer through the gaps between its screens, and a
 * screen there with nothing to select takes a click as Start (Select
 * Controller, Enter Name, whose letters are not items). In a level the mouse
 * goes back to the camera when the menu closes. The debugging tools' menu
 * (ts2_mouse_hold) puts all of this aside.
 *
 * The pause menu's lines are 3D text on a moving mesh, with no box of their
 * own: see n_box_3d.
 *
 * In widescreen the menus are drawn in proportion, in the middle 4:3 of the
 * picture (k_ts2_ui_places has every panel and item site as CENTRE), so a
 * pointer over a 16:9 picture is measured from the middle; over a 4:3 one
 * it is the picture.
 *
 * A scripted run's window has no pointer, so RECOMP_TS2_POINTER_SEQ moves
 * one: "FRAME:X:Y" puts it at X, Y in the game's 640x480 menu pixels at
 * that present (counted as ts2_mouse_frame counts them), "FRAME:click",
 * "FRAME:rclick" and "FRAME:wheel:N" press. RECOMP_TS2_MENU_LOG=1 prints
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
} N;

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

    if (A->play_sound && A->sound_on && N_MEM32(A->sound_on) && n_ptr_ok(N_MEM32(A->sounds))) {
        uint32_t top = g_esp, eax = g_eax, ecx = g_ecx, edx = g_edx;

        g_esp = (g_esp - 0x40u) & ~0xFu;
        g_esp -= 4; N_MEM32(g_esp) = 0;
        g_esp -= 4; N_MEM32(g_esp) = N_MEM32(N_MEM32(A->sounds));
        g_esp -= 4; N_MEM32(g_esp) = 0;              /* return address: none */
        A->play_sound();
        g_esp = top;
        g_eax = eax;
        g_ecx = ecx;
        g_edx = edx;
    }
    if (N.log)
        fprintf(stderr, "[TS2-MENU] frame %u: the pointer selects row %d column %d "
                        "(panel %08X item %08X)\n", N.frame, h->row, h->col, p, h->item);
}

/* The log: a screen's panels and items, once each time the screen changes. */
static void n_log_screen(uint32_t s)
{
    uint32_t p, i;
    int n, k, count;

    fprintf(stderr, "[TS2-MENU] frame %u: screen %08X (flags 0x%X, %d on the stack), level %u\n",
            N.frame, s, N_MEM32(s + TS2_MS_FLAGS), N_MEMI(A->frontend + TS2_MF_COUNT),
            N_MEM32(A->level_id));
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
                            "row %d column %d choices %d%s\n",
                    i, N_MEM32(i + TS2_MI_FLAGS), x0, y0, x1, y1, row, col,
                    N_MEMI(i + TS2_MI_CHOICES), N_MEM32(i + TS2_MI_CHILD) ? " (child)" : "");
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

void ts2_menu_frame(void)
{
    RecompPointer ptr;
    uint32_t s, top;
    Hit hit;
    float x, y;
    int over, moved;

    N.frame++;
    if (!A->frontend)
        return;
    if (!N.started) {
        N.started = 1;
        N.log = recomp_config_bool("RECOMP_TS2_MENU_LOG", NULL, 0);
        N.seq = getenv("RECOMP_TS2_POINTER_SEQ");
        fprintf(stderr, "[TS2-MENU] the mouse points at menu items while a menu is up\n");
    }

    s = n_menu();
    /* In the front end the pointer stays through the gaps between screens
     * and on the screens with nothing to select, and in a level it stays
     * as long as the game is paused (the pause menu's Inventory page has no
     * lines to select); otherwise the mouse goes back to the camera. */
    if ((!s && N_MEM32(A->level_id) < A->level_menus && !(A->pause && N_MEMI(A->pause) > 0)) ||
        ts2_mouse_is_held()) {
        if (N.want) {
            recomp_pointer_want(0);
            N.want = 0;
            if (N.log)
                fprintf(stderr, "[TS2-MENU] frame %u: no menu, the mouse is the camera's\n",
                        N.frame);
        }
        N.screen = 0;
        N.wheel = 0;
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
    if (top != N.screen)
        N.hover = 0;
    N.screen = top;

    n_seq();
    if (N.seq_on)
        n_seq_place();
    recomp_pointer_read(&ptr);
    moved = ptr.moved != N.moved;
    N.moved = ptr.moved;
    N.wheel += ptr.wheel;
    if (N.wheel > WHEEL_QUEUE)  N.wheel = WHEEL_QUEUE;
    if (N.wheel < -WHEEL_QUEUE) N.wheel = -WHEEL_QUEUE;

    if (!top) {
        N.wheel = 0;
        return;
    }
    /* Nothing while the screen takes no input (it is opening or closing). */
    if ((N_MEM32(top + TS2_MS_FLAGS) & 2u) || N_MEM8(A->frontend + TS2_MF_LOCK) < 0)
        return;

    /* A screen with nothing to select (Select Controller, Enter Name, a
     * message): the left button is Start, which goes on from them, and the
     * right button B, so the mouse alone gets through them. */
    if (!s) {
        if (N.frame < N.gap_until)
            return;
        if (ptr.pressed & (1u << RECOMP_MOUSE_LEFT)) {
            n_press(XBOX_GAMEPAD_START, 0, 0);
            if (N.log)
                fprintf(stderr, "[TS2-MENU] frame %u: click, nothing to select: Start\n", N.frame);
        } else if (ptr.pressed & (1u << RECOMP_MOUSE_RIGHT)) {
            n_press(0, 0, 1);
            if (N.log)
                fprintf(stderr, "[TS2-MENU] frame %u: right button: B\n", N.frame);
        }
        return;
    }

    over = ptr.inside && (n_to_menu(&ptr, &x, &y), n_hit(s, x, y, &hit));
    /* A click always selects what it is on: the keys may have moved the
     * selection away from the item the pointer last selected. */
    if (over && ((moved && hit.item != N.hover) || (ptr.pressed & (1u << RECOMP_MOUSE_LEFT)))) {
        n_select(&hit);
        N.hover = hit.item;
    }
    if (!over && moved)
        N.hover = 0;

    /* One press at a time, with a gap after it so each is seen. */
    if (N.frame < N.gap_until)
        return;
    if (over && (ptr.pressed & (1u << RECOMP_MOUSE_LEFT))) {
        /* An option steps on (right); anything else is accepted (A). */
        if (N_MEMI(hit.item + TS2_MI_CHOICES) > 0)
            n_press(XBOX_GAMEPAD_DPAD_RIGHT, 0, 0);
        else
            n_press(0, 1, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: click on row %d column %d: %s\n", N.frame,
                    hit.row, hit.col, N_MEMI(hit.item + TS2_MI_CHOICES) > 0 ? "right" : "A");
    } else if (ptr.pressed & (1u << RECOMP_MOUSE_RIGHT)) {
        n_press(0, 0, 1);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: right button: B\n", N.frame);
    } else if (N.wheel) {
        n_press(N.wheel > 0 ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN, 0, 0);
        if (N.log)
            fprintf(stderr, "[TS2-MENU] frame %u: wheel: D-pad %s\n", N.frame,
                    N.wheel > 0 ? "up" : "down");
        N.wheel += N.wheel > 0 ? -1 : 1;
    }
}
