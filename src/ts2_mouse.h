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
    TS2_PM_MODE     = 0x158,  /* 7, 8 (mounted guns), 6 and 9: below */
    TS2_PM_AIMING   = 0x884,  /* non-zero in aim mode */
    TS2_PM_HELD     = 0x998,  /* non-zero: look does nothing at all */
    TS2_MOUSE_CTX_EYE = 0x27C, /* the camera context: the eye the camera was made from (TS2_CTX_EYE in ts2_devtools.h) */
};

/* TS2_PM_MODE: what look does with the stick. */
enum {
    TS2_MODE_MOUNT_CLAMPED = 7, /* turns P, then holds it within the gun's arc */
    TS2_MODE_MOUNT         = 8, /* turns the gun itself, not P */
};

/* A mounted gun, as look finds it: from P's character (P+0x1BC, then +0x124)
 * the object it is on (+0x1444), and that object's gun (+0x124). */
enum {
    TS2_PM_CHARACTER = 0x1BC,  /* P: the character object */
    TS2_OBJ_DATA     = 0x124,  /* an object: its own record */
    TS2_CHR_MOUNT    = 0x1444, /* the character's record: the object it is on */
    TS2_OBJ_TYPE     = 0x008,  /* an object: its type */
    TS2_GUN_FLAGS    = 0x008,  /* the gun: bit 1 without bit 10, it does not turn */
    TS2_GUN_KIND     = 0x00C,  /* 0 and 8: look leaves it alone */
    TS2_GUN_PITCH    = 0x018,  /* degrees; the stick's up lowers it */
    TS2_GUN_YAW      = 0x020,  /* degrees, -180..180; turning right lowers it */
    TS2_GUN_PITCH_MIN = 0x030, TS2_GUN_PITCH_MAX = 0x034,
    TS2_GUN_YAW_MIN  = 0x038, TS2_GUN_YAW_MAX  = 0x03C,
    TS2_OBJ_TYPE_FREE_GUN = 0x239, /* turns whatever its flags say */
};

/* TS2_PM_FLAGS bits. */
enum {
    TS2_PF_AIM      = 0x002,  /* aim mode: the toggle, or the button held (0x8 to 0xA) */
    TS2_PF_SCOPE    = 0x004,  /* looking through a scope (no breathing sway) */
    TS2_PF_AIM_KEPT = 0x010,  /* aim mode kept while the weapon changes, put back after (0x18) */
};

/* While held (the debugging tools' menu or free camera is up and the game
 * reads a neutral pad), look turns nothing by the mouse and leaves its
 * motion for whoever takes it (recomp_mouse_take). */
void ts2_mouse_hold(int on);
int  ts2_mouse_is_held(void);

/* Once a frame, after the game's present. */
void ts2_mouse_frame(void);

/* look (wraps the release's look(stick, move, out), cdecl, which turns the
 * record at current_player by the stick): for player 1, the mouse's motion
 * is then added to the view angles directly (on a mounted gun, to the gun's
 * angles first, so look's own limits hold them). */
void ts2_mouse_look(void (*look)(void));

/* The mouse's motion since last taken, in degrees (right, up) at the
 * player's sensitivity and the game's field of view, as the player turns:
 * for a camera of the debugging tools while ts2_mouse_hold is on. 0 if
 * nothing moved or mouse look is off. */
int ts2_mouse_turn(float *right, float *up);

/* breathing (wraps the release's breathing function, which sets the sway
 * the camera adds to the record at current_player's view): while player 1
 * aims, the sway is held at 0 without moving the view. */
void ts2_mouse_breath(void (*breath)(void));

#endif
