/*
 * ts2_mapmaker_mouse.h -- TimeSplitters 2's MapMaker under the mouse and
 * keyboard (src/ts2_mapmaker_mouse.c).
 *
 * The table of where one release keeps what it reads
 * (config/<region>/mapmaker.inc). Nothing here is an address.
 */
#ifndef SPLIT2_TS2_MAPMAKER_MOUSE_H
#define SPLIT2_TS2_MAPMAKER_MOUSE_H

#include <stdint.h>

#include "input_bindings.h"         /* xboxrecomp: RecompPointer */
#include "xinput_xbox.h"            /* xboxrecomp: XBOX_GAMEPAD */

/* 0 means not found in that release: MapMaker stays the pad's. */
typedef struct Ts2MapMakerAddrs {
    uint32_t level;           /* the level id while MapMaker's editor runs */
    /* The hand's limits on screen while the editor has no screen up, in the
     * game's 640x480 pixels: it stops this far inside each edge, and the
     * stick pushed past them scrolls the map. */
    int      margin_x, margin_y;
    /* dwords, 16.16 as the hand is: its limits with one of the editor's
     * screens up (the palette): top, bottom, left, right. */
    uint32_t screen_bounds;
    /* Which trigger zooms in: 0 the left, 1 the right; the other zooms out. */
    int      zoom_in_right;
    /* The Mode question that opens a new map (Beginner, Advanced): its
     * screen's class, and the dword holding the line picked. */
    uint32_t mode_class, mode_row;
} Ts2MapMakerAddrs;

/* In the menu state F (src/ts2_menu_mouse.h, TS2_MF_*). */
enum {
    TS2_MF_HAND       = 0x018,  /* bits: 1 the hand picks the hot spot under it, 2 the stick moves it */
    TS2_MF_HAND_X     = 0x030,  /* int, 16.16: the hand, in 640x480 pixels */
    TS2_MF_HAND_Y     = 0x034,
    /* A screen's hot spots: the places on it the hand can pick. */
    TS2_MS_HOT_PICKED = 0x648,  /* word: the id of the one picked, as A takes it; 0 none */
    TS2_MS_HOTS       = 0x64C,  /* dword: where they are, TS2_HOT_SIZE each */
    TS2_MS_HOT_COUNT  = 0x650,
    TS2_HOT_SIZE      = 0x18,
    TS2_HOT_X = 0x0, TS2_HOT_Y = 0x2, TS2_HOT_W = 0x4, TS2_HOT_H = 0x6,   /* int16 */
    TS2_HOT_ID = 0x8,           /* word */
    TS2_HOT_FLAGS = 0xA,        /* int16: 1 not a place to pick; 0x80 cannot be picked now */
};

/* From ts2_menu_frame while MapMaker's editor level runs and no panel menu
 * is up: the pointer as read this frame, in menu pixels (x, y; inside says
 * whether it is over the picture), the screen on top (0 for the editor
 * itself), and the left button: down this frame, held now, and whether the
 * press began on something the menu code took (a tab's arrow, a prompt).
 * Returns 1 when MapMaker took the frame. */
int  ts2_mapmaker_frame(uint32_t top, const RecompPointer *ptr, float x, float y, int inside,
                        int moved, int menu_took_press);

/* A frame MapMaker does not take, for a screen the menu pointer runs
 * itself (the letter picker): its presses run out, nothing is held. */
void ts2_mapmaker_idle(void);

/* MapMaker's editor (no screen up) is running: Escape is Start there,
 * which opens its menu. */
int  ts2_mapmaker_editing(void);

/* MapMaker's editor level is running, editor or one of its screens. */
int  ts2_mapmaker_active(void);

/* On a screen of rows, picks the row at height y (menu pixels), as the
 * D-pad picks it: a row's arrow was pointed at. Returns its id, or 0. */
int  ts2_mapmaker_pick_row_at(uint32_t top, float y);

/* From the bindings filter, for controller 1. */
void ts2_mapmaker_filter(XBOX_GAMEPAD *pad);

#endif
