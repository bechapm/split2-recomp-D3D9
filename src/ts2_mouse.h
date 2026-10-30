/*
 * ts2_mouse.h -- TimeSplitters 2 on a mouse and keyboard (src/ts2_mouse.c).
 *
 * The hook each release's config/<region>/overrides.inc wraps the game's
 * look function with, and the table of where that release keeps what it
 * reads and writes (config/<region>/mouse.inc). Nothing here is an address.
 */
#ifndef SPLIT2_TS2_MOUSE_H
#define SPLIT2_TS2_MOUSE_H

#include <stdint.h>

/* Where one release keeps what the mouse uses. 0 means not found in that
 * release; the mouse then turns nothing there (the keys still work). */
typedef struct Ts2MouseAddrs {
    uint32_t player_base;     /* dword: player 1's record (P) */
    uint32_t current_player;  /* dword: the record the player update is on */
    uint32_t level_id;        /* dword: the level now loaded */
    uint32_t level_menus;     /* the first value that is not a playable level */
    uint32_t pitch_limit;     /* float: look keeps the pitch within +- this */
    uint32_t context;         /* dword: the camera/render context */
    uint32_t context_fov;     /* float at context + this: the field of view */
} Ts2MouseAddrs;

/* The player record's view angles, as the debugging tools have them, and
 * what look reads beside them. The same in every release found so far. */
enum {
    TS2_PM_EYE      = 0x0FC,  /* the eye, made from the position after each move */
    TS2_PM_YAW      = 0x148,  /* degrees, 0..360; turning right lowers it */
    TS2_PM_PITCH    = 0x14C,  /* degrees; up is positive (measured: -40 shows the floor) */
    TS2_PM_FLAGS    = 0x00C,  /* bit 3: turns by look; bit 2: a scope's view; others slow it */
    TS2_PM_SWAY_YAW   = 0x188, /* breathing: degrees the camera adds to the yaw */
    TS2_PM_SWAY_PITCH = 0x18C, /* and to the pitch */
    TS2_PM_MODE     = 0x158,  /* 6, 8 (a mounted gun) and 9: look does not turn P */
    TS2_PM_AIMING   = 0x884,  /* non-zero in aim mode */
    TS2_PM_HELD     = 0x998,  /* non-zero: look does nothing at all */
    TS2_MOUSE_CTX_EYE = 0x27C, /* the camera context: the eye the camera was made from (TS2_CTX_EYE in ts2_devtools.h) */
};

/* TS2_PM_FLAGS bits. */
enum {
    TS2_PF_SCOPE    = 0x004,  /* looking through a scope (no breathing sway) */
    TS2_PF_CHANGING = 0x010,  /* changing weapon (0x18 measured; aiming reads 0 meanwhile) */
};

/* While held (the debugging tools' menu or free camera is up and the game
 * reads a neutral pad), look turns nothing by the mouse and leaves its
 * motion for whoever takes it (recomp_mouse_take). */
void ts2_mouse_hold(int on);

/* Once a frame, after the game's present. */
void ts2_mouse_frame(void);

/* look (wraps the release's look(stick, move, out), cdecl, which turns the
 * record at current_player by the stick): for player 1, the mouse's motion
 * is then added to the view angles directly. */
void ts2_mouse_look(void (*look)(void));

/* breathing (wraps the release's breathing function, which sets the sway
 * the camera adds to the record at current_player's view): while player 1
 * aims, the sway is held at 0 without moving the view. */
void ts2_mouse_breath(void (*breath)(void));

#endif
