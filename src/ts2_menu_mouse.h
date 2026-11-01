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
    /* How the menus light what is selected: the item styles (TS2_STYLE_*,
     * TS2_STYLE_SIZE each, 4 of them) and the sine table the selected line's
     * pulse is read from (2048 floats, one turn), by the menu state's clock
     * (TS2_MF_TIME). */
    uint32_t styles, sine;
    /* Who draws an arrow or a sprite, by the range its return address is
     * in: an option's arrows (the item drawing), a pause page's scroll
     * arrows, its page arrows; and the call that draws a pause page's
     * "Select" (its A icon beside it). */
    uint32_t item_draw, item_draw_end;
    uint32_t scroll_draw, scroll_draw_end;
    uint32_t page_draw, page_draw_end;
    uint32_t pause_select;
    void   (*players)(void);  /* local players, in eax */
    uint32_t inventory;       /* the pause menu's Inventory page: its screen update */
    /* A panel's own scroll arrows, drawn when its list runs past it: the
     * sprites' return addresses, up and down. */
    uint32_t panel_arrow_up, panel_arrow_down;
    /* The call that draws a pause page's "Back", as pause_select its "Select". */
    uint32_t pause_back;
    /* dword: 1 while a line is drawn as one of its outline passes, which
     * leave out the button pictures. */
    uint32_t outline_flag;
    /* text(float x, float y, text, colour, outline colour), cdecl: the form
     * that draws its own outline. */
    void   (*text_outlined)(void);
    /* width(string), the game's measure as it is (text_width, above, names
     * the keys in a line while they drive a menu). */
    void   (*text_width_raw)(void);
    /* MapMaker's tab bar: the sprites' return addresses of its arrows,
     * left and right (the D-pad's left and right change tab). They are
     * sprites by id: dword array sprite id to texture, and the dword holding
     * the texture records, 0x34 each, width at +0x26 and height at +0x2A. */
    uint32_t tab_left, tab_right;
    uint32_t sprite_textures, texture_records;
    /* The prompt text calls drawn just after the picture at the left and
     * right of MapMaker's diamond: their return addresses. */
    uint32_t prompt_label_left, prompt_label_right;
    /* MapMaker's screens of rows (Map Settings, Editor Settings): the
     * sprites' return addresses of a row's arrows, left and right, drawn
     * beside its value (the D-pad's left and right on the row picked step
     * it). */
    uint32_t row_arrow_left, row_arrow_right;
    /* The letter picker (Enter Name, MapMaker's names and texts): the
     * states, TS2_KB_SIZE a player (TS2_KB_*), and the return address of
     * the "Add  Cancel" line MapMaker's draw under its grid, which is
     * fitted to its box while it names keys. */
    uint32_t picker_states;
    uint32_t picker_hint;
} Ts2MenuAddrs;

/* The menu state F (one per local player), a screen S, a panel P and an
 * item I, as the game's own menu code reads them (USA disassembly,
 * 30 Oct 2026; the screen update is sub_000F2570, the panel's sub_000F1900,
 * its drawing sub_000F2040). */
enum {
    TS2_MF_SCREENS    = 0x008,  /* S*[]: the stack of open screens, bottom first */
    TS2_MF_LOCK       = 0x01C,  /* byte: negative while input is held (a transition) */
    TS2_MF_COUNT      = 0x024,  /* int: screens on the stack */
    TS2_MF_TIME       = 0x29C,  /* float: the menus' clock, in seconds; the pulses run on it */
    TS2_MF_TEXT_Z     = 0x2D4,  /* float: a 3D menu's text, its depth */
    /* The Inventory page's list, remade by its update each frame
     * (sub_000D8120): the item selected (an id), how many, their ids. */
    TS2_MF_INV_SEL    = 0x30C,
    TS2_MF_INV_COUNT  = 0x318,
    TS2_MF_INV_IDS    = 0x31C,  /* int[20] */

    TS2_MS_CLASS      = 0x004,  /* its class: +4 its update (sub_000870E0) */

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
    TS2_FONT_PROMPTS = 5,       /* a pause page's prompts ("Select", "Back") */
    /* The item fonts the game steps down through to fit a line (21, 20,
     * 19: sub_001C29F0): one glyph set, each 0.1 narrower. */
    TS2_FONT_FIT_MIN = 19, TS2_FONT_FIT_MAX = 21,
    /* The fonts the letter picker's "Add  Cancel" line steps down through:
     * it is drawn in 0x14 (0x13, 0x10 in two languages), each smaller one
     * narrower. */
    TS2_FONT_PICKER_MIN = 0x10, TS2_FONT_PICKER_MAX = 0x14,
};

/* A player's letter picker (sub_001AD350 draws it, sub_001ACE70 runs it):
 * a grid of 6 by 6 cells, each 24 wide and 20 high (15 with three or more
 * players), from (x + 6, y + 4). SPACE, DEL and END are two cells wide,
 * any of their columns picking them. A or Start acts on the cell picked. */
enum {
    TS2_KB_X      = 0x00,       /* int: the box, from the viewport's corner */
    TS2_KB_Y      = 0x04,
    TS2_KB_LAYOUT = 0x1C,       /* char*: the cells' characters, row * 6 + column */
    TS2_KB_COLUMN = 0x28,       /* int: the cell picked */
    TS2_KB_ROW    = 0x2C,
    TS2_KB_MODE   = 0x30,       /* int: 1 the front end, 2 with three or more players, 5 MapMaker */
    TS2_KB_SIZE   = 0x40,
};

/* An item style (sub_000F1230 colours an item by its style): r, g, b, a
 * dwords, 0..127. */
enum {
    TS2_STYLE_SIZE      = 0xC0,
    TS2_STYLE_LIT       = 0x40,   /* the selected line: g, b (r is the pulse) */
    TS2_STYLE_OUTLINE   = 0x50,   /* its outline, drawn round it (r, g, b, a) */
    TS2_STYLE_ARROW_DIM = 0x80,   /* an option's arrows, on a line not selected */
    TS2_STYLE_ARROW     = 0x90,   /* on the selected line */
    TS2_STYLE_ARROW_LIT = 0xA0,   /* that way held */
    TS2_STYLES          = 4,
};

/* The Inventory page's rows (sub_000D7840, one player): from the panel's
 * left + 10, a line of font 8 each from its top less its scroll, the one
 * selected boxed from 5 before to 200 after. */
enum { TS2_INV_FONT = 8, TS2_INV_X0 = 5, TS2_INV_X1 = 210, TS2_INV_MAX = 20 };

/* Placements, as recomp_manual.c's TS2_UI_* has them. */
enum { TS2_PLACE_OFF = -1, TS2_PLACE_AUTO = 0, TS2_PLACE_STRETCH, TS2_PLACE_CENTRE,
       TS2_PLACE_LEFT, TS2_PLACE_RIGHT, TS2_PLACE_SIDE, TS2_PLACE_EDGE,
       TS2_PLACE_SPREAD };

enum {
    TS2_MP_FOCUS = 0x2, TS2_MP_SHOWN = 0x4, TS2_MP_CLOSING = 0x20, TS2_MP_HIDDEN = 0x80,
    TS2_MP_NO_EMPTY_A = 0x100,  /* no A hint on an item with no action */
    TS2_MI_FIXED = 0x1,         /* its value cannot be stepped, nor A pressed on it */
    TS2_MI_DISABLED = 0x8, TS2_MI_SELECTED = 0x10, TS2_MI_SCROLLS = 0x100,
};

/* Once a frame, after the game's present (from ts2_mouse_frame): whether a
 * menu is up, and what the pointer did over it. */
void ts2_menu_frame(void);

/* From the bindings filter, for controller 1: the presses a click or the
 * wheel stands for, added to what the pad reads. */
void ts2_menu_filter(XBOX_GAMEPAD *pad);

/* For src/ts2_mapmaker_mouse.c: the menu state F (player 1's), and the
 * level running now; 0 when not found. */
uint32_t ts2_menu_frontend(void);
uint32_t ts2_menu_level(void);

/* Whether Esc should go back one step as B does, rather than be Start: a
 * pause menu screen opened from one of its pages is up (where Start would
 * close the whole menu). For src/ts2_mouse.c's bindings filter. */
int ts2_menu_esc_backs(void);

/* Whether player 1's letter picker is up (Enter Name, MapMaker's names and
 * texts): its keys are a text field's there (src/ts2_mouse.c). */
int ts2_menu_picker_up(void);

/* MapMaker (src/ts2_mapmaker_mouse.c): the key a prompt names for B while
 * MapMaker is up, where Esc opens the editor's menu ("[Backspace]"), or
 * NULL elsewhere. */
const char *ts2_mapmaker_b_key(void);

/* The button that skips a cutscene, as a pad press: a button bit or, for A
 * and B, *analog set to which (1 A, 2 B). RECOMP_TS2_SKIP_BUTTON picks it. */
WORD ts2_skip_button(int *analog);

#endif
