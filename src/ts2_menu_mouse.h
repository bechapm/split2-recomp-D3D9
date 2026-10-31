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
    uint32_t viewport;        /* dword: points at the game's viewport (TS2_VP_*) */
    /* The button hints and an option's value are measured as the game
     * measures text: in a font of its font table, by its own width. */
    uint32_t fonts;           /* the font table (TS2_FONT_*), 0x1C a font */
    uint32_t glyph_sets;      /* the glyph sets a font names (TS2_GLYPHS_*), 0x20 each */
    uint32_t font_now;        /* the font selected and its two table pointers: 3 dwords */
    uint32_t text_flags;      /* dword: bit 0, text at half height */
    void   (*font_select)(void); /* select(font), cdecl */
    void   (*text_width)(void);  /* width(string) in eax, in the font selected, cdecl */
    /* The pause menu's pages: the handlers a page screen has for the
     * D-pad's left and right (TS2_MS_LEFT, TS2_MS_RIGHT). */
    uint32_t page_prev, page_next;
} Ts2MenuAddrs;

/* The menu state F (one per local player), a screen S, a panel P and an
 * item I, as the game's own menu code reads them (USA disassembly,
 * 30 Oct 2026; the screen update is sub_000F2570, the panel's sub_000F1900,
 * its drawing sub_000F2040). */
enum {
    TS2_MF_SCREENS    = 0x008,  /* S*[]: the stack of open screens, bottom first */
    TS2_MF_LOCK       = 0x01C,  /* byte: negative while input is held (a transition) */
    TS2_MF_COUNT      = 0x024,  /* int: screens on the stack */
    TS2_MF_TEXT_Z     = 0x2D4,  /* float: a 3D menu's text, its depth */

    TS2_MS_FLAGS      = 0x008,  /* 1|4: running and shown; 2: takes no input */
    TS2_MS_X          = 0x01C,  /* int: a screen in a level: where it is on screen */
    TS2_MS_Y          = 0x020,
    TS2_MS_PANELS     = 0x068,  /* P*: the first panel */
    TS2_MS_LEFT       = 0x628,  /* handler(S) for the D-pad's left, held (sub_000F2570) */
    TS2_MS_RIGHT      = 0x62C,  /* and its right */

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
    TS2_MP_HINT_Y     = 0x184,  /* int: the button hints' line (sub_000EFEB0) */
    TS2_MP_HINTS      = 0x188,  /* int: how many hints */
    TS2_MP_HINT_TEXT  = 0x18C,  /* char[0x80] each: "\x0b2 Select", "\x0b1 Back" */
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
    TS2_MI_FONT       = 0x01C,  /* int: its font */
    TS2_MI_FLAGS      = 0x028,  /* 8: cannot be selected; 0x10: selected; 0x100: scrolls */
    TS2_MI_ACTION     = 0x030,  /* what A does on it, or 0 */
    TS2_MI_CHOICES    = 0x040,  /* int: values to step through with left and right */
    TS2_MI_CHOICE     = 0x044,  /* int: the value now */
    TS2_MI_CHOICE_LIST = 0x048, /* the values (TS2_CH_*), linked */
    TS2_MI_CHILD      = 0x05C,  /* non-zero: text in a 3D menu, not on the panel */
    TS2_MI_Y_3D       = 0x060,  /* float: such an item's height in the 3D menu */
    TS2_MI_X          = 0x064,  /* int: the item's box, from the panel's corner */
    TS2_MI_Y          = 0x068,
    TS2_MI_W          = 0x06C,
    TS2_MI_H          = 0x070,
    TS2_MI_LOOK       = 0x074,  /* int: which of its looks is shown (TS2_MI_OPTION) */
};

/* The game's viewport, as its 3D text reads it (sub_001AB880), and the mesh
 * a 3D menu's text lies on. */
enum {
    TS2_VP_CENTRE_X = 0x08, TS2_VP_HALF_W = 0x10,
    TS2_VP_CENTRE_Y = 0x1C, TS2_VP_HALF_H = 0x24,
    TS2_MC_MATRIX   = 0x2F0,    /* float[16]: the mesh's space to the screen */
};

/* An option's value, as its item draws it (sub_000F1230), and a font. */
enum {
    TS2_CH_TEXT      = 0x04,    /* char*: the value's text */
    TS2_CH_X         = 0x0C,    /* int: where its right arrow ends, from the item's box */
    TS2_CH_NEXT      = 0x18,    /* the next value */
    TS2_FONT_GLYPHS  = 0x00,    /* int: its glyph set */
    TS2_FONT_SCALE_Y = 0x0C,    /* float: its height, against the glyphs' */
    TS2_GLYPHS_H     = 0x10,    /* int: the glyphs' height */
    TS2_FONT_HINTS   = 0x14,    /* the button hints' font */
};

enum {
    TS2_MP_FOCUS = 0x2, TS2_MP_SHOWN = 0x4, TS2_MP_CLOSING = 0x20, TS2_MP_HIDDEN = 0x80,
    TS2_MP_NO_EMPTY_A = 0x100,  /* no A hint on an item with no action */
    TS2_MI_FIXED = 0x1,         /* its value cannot be stepped, nor A pressed on it */
    TS2_MI_DISABLED = 0x8, TS2_MI_SELECTED = 0x10, TS2_MI_SCROLLS = 0x100,
};

/* Once a frame, after the game's present (from ts2_mouse_frame): whether a
 * menu is up, and what the pointer did over it. */
void ts2_menu_frame(void);

/* From the bindings filter, for controller 1: the presses a click, the
 * right button or the wheel stand for, added to what the pad reads. */
void ts2_menu_filter(XBOX_GAMEPAD *pad);

/* The button that skips a cutscene, as a pad press: a button bit or, for A
 * and B, *analog set to which (1 A, 2 B). RECOMP_TS2_SKIP_BUTTON picks it. */
WORD ts2_skip_button(int *analog);

#endif
