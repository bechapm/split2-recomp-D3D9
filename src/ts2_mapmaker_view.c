/*
 * ts2_mapmaker_view.c -- MapMaker's editor as wide as the picture.
 *
 * The editor is a 2D view of a flat map. The game says where the view is on
 * the screen (a rectangle in pixels, 0,0 640x480) and on the map (its left
 * and right, its span), and from the two a scale, map units a pixel. A
 * wider picture should show more of the map at that same scale, so a cell
 * stays square and the size it was, not stretch or fill in proportion.
 *
 * Only what asks how far the view reaches depends on its width: the grid's
 * lines, the pieces culled at its edges, and the hand, which stops 64 pixels
 * inside it and scrolls the map past that. Turning a map place into a pixel
 * does not: moving the rectangle's left edge out by n pixels and the view's
 * left edge out by n pixels' worth of map cancels. So the game's own values
 * stay as they are, and two things, the editor's update (sub_00117A50) and
 * its draw (sub_001132D0), see the view widened on the way in and put back
 * on the way out:
 *
 *   - everything that sets the view (a new map, centring on the map, going
 *     to a room, each writing a span of its own) runs at the game's width;
 *   - about 30 of the editor's other screens (the palette, the menus, Map
 *     Settings) lay themselves out from the same rectangle, and would slide
 *     left by what it grew. Neither function reaches them, by direct call or
 *     by anything indirect in the editor's code (checked over the call graph
 *     of the disassembly, 1 Nov 2026).
 *
 * The map itself never changes: its limits are its own, 40 cells each way
 * from the middle, a piece can be put only inside them, and the view's middle
 * stops at their edge (sub_00113EE0, sub_001146F0). Nothing that saves a map
 * reads the view, so a map made at 21:9 is the same map at 4:3.
 *
 * Two things that are not the view's reach need help. The game's box draw
 * clips to the screen's rectangle, a different one, read by every screen too:
 * it is widened only while the grid draws. And the zoom's limits are numbers
 * in the code, 60 and 8 cells across at any width, which on a wider view
 * would mean bigger cells and a shorter view at both ends: scaled at their
 * four call sites, they stay the scale they were.
 *
 * What the mouse needs is the hand's reach (ts2_mapmaker_view_span): it runs
 * outside those two functions and sees the game's values.
 *
 * RECOMP_TS2_MAPMAKER_WIDE=0 leaves the view at 4:3 for comparison. The hooks
 * attach in config/<region>/overrides.inc; the addresses are in mapmaker.inc.
 */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "ts2_mapmaker_mouse.h"
#include "recomp_config.h"           /* xboxrecomp: recomp_config_bool */
#include "recomp/gen/recomp_cpu.h"   /* g_esp */

extern ptrdiff_t g_xbox_mem_offset;

/* src/recomp_manual.c: how many times wider than 4:3 the 2D's area is. */
extern float ts2_ui_wide(void);

#define V_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define V_MEMI(a)  (*(volatile int32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

#define GAME_W 640

static struct {
    int      in;                /* the view is widened now */
    int      x, w;              /* the rectangle as the game has it */
    int32_t  span, pad;         /* the span widened, and the map added at each side */
    uint32_t screen;            /* the screen rectangle widened now, or 0 */
    int      screen_pad;
} V;

/* Pixels added at each side, 0 when nothing is: to an even width, rounded up
 * so the grid reaches the picture's edge. */
static int view_pad(int w)
{
    static float wide;
    int full;

    if (wide == 0.0f) {
        wide = recomp_config_bool("RECOMP_TS2_MAPMAKER_WIDE", NULL, 1) ? ts2_ui_wide() : 1.0f;
        fprintf(stderr, "[TS2-MAPMAKER] the editor's view is %.3f times 4:3 "
                "(RECOMP_TS2_MAPMAKER_WIDE)\n", wide);
    }
    if (wide <= 1.0f)
        return 0;
    full = (int)ceilf((float)w * wide * 0.5f) * 2;
    return full > w ? (full - w) / 2 : 0;
}

void ts2_mm_view_enter(void)
{
    const Ts2MapMakerAddrs *a = ts2_mapmaker_addrs();
    int pad;
    int32_t span;

    if (V.in || !a->view.w)
        return;
    V.x = V_MEMI(a->view.x);
    V.w = V_MEMI(a->view.w);
    pad = view_pad(V.w);
    if (V.w <= 0 || !pad)
        return;
    span = V_MEMI(a->view.span);
    V.pad = (int32_t)((int64_t)span * pad / V.w);
    V.span = span + 2 * V.pad;
    V.in = 1;
    V_MEMI(a->view.x) = V.x - pad;
    V_MEMI(a->view.w) = V.w + 2 * pad;
    V_MEMI(a->view.left) -= V.pad;
    V_MEMI(a->view.right) += V.pad;
    V_MEMI(a->view.span) = V.span;
}

void ts2_mm_view_leave(void)
{
    const Ts2MapMakerAddrs *a = ts2_mapmaker_addrs();
    int32_t span;

    if (!V.in)
        return;
    V.in = 0;
    span = V_MEMI(a->view.span);
    /* Unchanged, the same number comes off that went on, so nothing creeps
     * frame by frame. A zoom wrote a new span at the wider width. */
    if (span != V.span)
        V.pad = (span - (int32_t)((int64_t)span * V.w / V_MEMI(a->view.w))) / 2;
    V_MEMI(a->view.left) += V.pad;
    V_MEMI(a->view.right) -= V.pad;
    V_MEMI(a->view.span) = span - 2 * V.pad;
    V_MEMI(a->view.x) = V.x;
    V_MEMI(a->view.w) = V.w;
}

void ts2_mm_screen_enter(void)
{
    const Ts2MapMakerAddrs *a = ts2_mapmaker_addrs();

    if (!V.in || V.screen)
        return;
    V.screen = V_MEM32(a->screen);
    V.screen_pad = view_pad(V.w);
    V_MEMI(V.screen) -= V.screen_pad;
    V_MEMI(V.screen + 4u) += V.screen_pad;
}

void ts2_mm_screen_leave(void)
{
    if (!V.screen)
        return;
    V_MEMI(V.screen) += V.screen_pad;
    V_MEMI(V.screen + 4u) -= V.screen_pad;
    V.screen = 0;
}

/* At the entry of the fixed-point helper the zoom's limits go through: the
 * return address is at [esp], the arguments above it. arg is where this
 * helper has the limit. */
void ts2_mm_zoom_limit(int arg)
{
    const Ts2MapMakerAddrs *a = ts2_mapmaker_addrs();
    uint32_t back;
    int k;

    if (!V.in)
        return;
    back = V_MEM32(g_esp);
    for (k = 0; k < 4; k++)
        if (a->zoom_limits[k] == back) {
            int32_t limit = V_MEMI(g_esp + (uint32_t)arg);

            V_MEMI(g_esp + (uint32_t)arg) =
                (int32_t)((int64_t)limit * (V.w + 2 * view_pad(V.w)) / V.w);
            return;
        }
}

void ts2_mapmaker_view_span(float *left, float *right)
{
    const Ts2MapMakerAddrs *a = ts2_mapmaker_addrs();
    int pad = a->view.w ? view_pad(GAME_W) : 0;

    *left = (float)-pad;
    *right = (float)(GAME_W + pad);
}
