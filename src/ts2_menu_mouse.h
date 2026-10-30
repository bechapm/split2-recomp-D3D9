/*
 * ts2_menu_mouse.h -- TimeSplitters 2's menus under the mouse
 * (src/ts2_menu_mouse.c).
 *
 * The table of where one release keeps its menus (config/<region>/menu_mouse.inc).
 * Nothing here is an address.
 */
#ifndef SPLIT2_TS2_MENU_MOUSE_H
#define SPLIT2_TS2_MENU_MOUSE_H

#include <stdint.h>

#include "xinput_xbox.h"            /* xboxrecomp: XBOX_GAMEPAD */

/* Where one release keeps what the menu pointer reads. 0 means not found
 * in that release; the menus then stay the pad's and the keyboard's. */
typedef struct Ts2MenuAddrs {
    uint32_t frontend;        /* player 1's menu state (F): its stack of screens */
    uint32_t level_id;        /* dword: the level now loaded */
    uint32_t level_menus;     /* the first value that is not a playable level */
    uint32_t pause;           /* int: the game's pause: 0 running, >0 paused */
    uint32_t sound_on;        /* dword: non-zero when the menus make sounds */
    uint32_t sounds;          /* dword: the menus' sound set; [0] is the move */
    void   (*play_sound)(void); /* play(sound, 0), cdecl */
} Ts2MenuAddrs;

/* The menu state F (one per local player), a screen S, a panel P and an
 * item I, as the game's own menu code reads them (USA disassembly,
 * 30 Oct 2026; the screen update is sub_000F2570, the panel's sub_000F1900,
 * its drawing sub_000F2040). */
enum {
    TS2_MF_SCREENS    = 0x008,  /* S*[]: the stack of open screens, bottom first */
    TS2_MF_LOCK       = 0x01C,  /* byte: negative while input is held (a transition) */
    TS2_MF_COUNT      = 0x024,  /* int: screens on the stack */

    TS2_MS_FLAGS      = 0x008,  /* 1|4: running and shown; 2: takes no input */
    TS2_MS_PANELS     = 0x068,  /* P*: the first panel */

    TS2_MP_FLAGS      = 0x00C,  /* 2: has the focus (reads the pad); 4: shown */
    TS2_MP_W          = 0x010,  /* int: the panel's width */
    TS2_MP_H          = 0x020,  /* int: and height */
    TS2_MP_SCROLL     = 0x024,  /* int: where a scrolling list starts, with */
    TS2_MP_X          = 0x034,  /* int: the panel's left on screen, this frame */
    TS2_MP_Y          = 0x038,  /* int: and top */
    TS2_MP_CLIP_X     = 0x03C,  /* int: the visible part: left, top, width, height */
    TS2_MP_CLIP_Y     = 0x040,
    TS2_MP_CLIP_W     = 0x044,
    TS2_MP_CLIP_H     = 0x048,
    TS2_MP_OFFSET     = 0x054,  /* float: the list's scroll, against TS2_MP_SCROLL */
    TS2_MP_ITEMS      = 0x38C,  /* I*: the first item; rows go on through +8 */
    TS2_MP_ITEM_COUNT = 0x390,  /* int: items in the list through +4 */
    TS2_MP_ROWS       = 0x394,  /* int: rows */
    TS2_MP_ROW        = 0x398,  /* int: the selected row, -1 for none */
    TS2_MP_COLUMN     = 0x39C,  /* int: the selected column in it */
    TS2_MP_NEXT       = 0x3A4,  /* P*: the screen's next panel */

    TS2_MI_NEXT       = 0x004,  /* I*: every item, in order */
    TS2_MI_NEXT_ROW   = 0x008,  /* I*: the first item of the next row */
    TS2_MI_NEXT_COL   = 0x00C,  /* I*: the next item along the row */
    TS2_MI_OPTION     = 0x010,  /* I*: the item's next look (TS2_MI_LOOK of them) */
    TS2_MI_FLAGS      = 0x028,  /* 8: cannot be selected; 0x10: selected; 0x100: scrolls */
    TS2_MI_CHOICES    = 0x040,  /* int: values to step through with left and right */
    TS2_MI_CHILD      = 0x05C,  /* non-zero: drawn by something else, not a row */
    TS2_MI_X          = 0x064,  /* int: the item's box, from the panel's corner */
    TS2_MI_Y          = 0x068,
    TS2_MI_W          = 0x06C,
    TS2_MI_H          = 0x070,
    TS2_MI_LOOK       = 0x074,  /* int: which of its looks is shown (TS2_MI_OPTION) */
};

enum {
    TS2_MP_FOCUS = 0x2, TS2_MP_SHOWN = 0x4,
    TS2_MI_DISABLED = 0x8, TS2_MI_SELECTED = 0x10, TS2_MI_SCROLLS = 0x100,
};

/* Once a frame, after the game's present (from ts2_mouse_frame): whether a
 * menu is up, and what the pointer did over it. */
void ts2_menu_frame(void);

/* From the bindings filter, for controller 1: the presses a click, the
 * right button or the wheel stand for, added to what the pad reads. */
void ts2_menu_filter(XBOX_GAMEPAD *pad);

#endif
