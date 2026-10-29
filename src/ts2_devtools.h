/*
 * ts2_devtools.h -- TimeSplitters 2's debugging tools (src/ts2_devtools.c).
 *
 * The hooks each release's config/<region>/overrides.inc wraps the game's
 * functions with, and the table of where that release keeps what the tools
 * read and write (config/<region>/devtools.inc). Nothing here is an address.
 */
#ifndef SPLIT2_TS2_DEVTOOLS_H
#define SPLIT2_TS2_DEVTOOLS_H

#include <stdint.h>

/* Where one release keeps what the tools use. 0 means not found in that
 * release, and every tool that needs it stays off there. */
typedef struct Ts2DevAddrs {
    uint32_t player_base;     /* dword: player 1's record (P) */
    uint32_t level_id;        /* dword: the level now loaded */
    uint32_t level_front_end; /* its value in the front end */
    uint32_t god;             /* dword: the developers' god mode (player-controlled characters) */
    uint32_t infinite_ammo;   /* dword: the developers' infinite ammo */
    uint32_t unlock_all;      /* dword: every unlock condition passes */
    uint32_t unlock_override; /* bytes: per-kind unlock overrides, unlock_override_count of them */
    uint32_t unlock_override_count;
    uint32_t pause;           /* int: the game's pause: 0 running, >0 paused, -1 resuming */
    uint32_t context;         /* dword: the camera/render context */
    uint32_t look_at_return;  /* the camera update's call of look_at: its return address */
    /* room_walk(float *from, int from_room, float *to, 0): the room that
     * holds `to`, found by walking the portals from `from` in from_room. */
    void (*room_walk)(void);
} Ts2DevAddrs;

/* The player record (P), the character object (C = [P + chr]) and the
 * camera context: the same in every release found so far, so they are here
 * rather than in each table. */
enum {
    TS2_P_INDEX     = 0x000,  /* the character's index: the sweep's first argument */
    TS2_P_POS       = 0x0F0,  /* x, y, z; y is up */
    TS2_P_EYE       = 0x0FC,  /* the eye, made from the position after each move */
    TS2_P_VIEW_DIR  = 0x108,  /* the view direction, a unit vector */
    TS2_P_YAW       = 0x148,  /* degrees; turning right lowers it */
    TS2_P_PITCH     = 0x14C,  /* degrees */
    TS2_P_CHR       = 0x1BC,  /* C */
    TS2_P_ROOM      = 0x87C,  /* the room the player is in */
    TS2_C_POS       = 0x058,
    TS2_C_HEALTH    = 0x19C,  /* float */
    TS2_CTX_WORLD   = 0x180,  /* the camera's world matrix: right, up, forward, eye rows */
    TS2_CTX_ROOM    = 0x294,  /* the room drawing starts from */
};

/* Once a frame, after the game's present: keys, switches, the readout. */
void ts2_dev_frame(void);

/* The character sweep (wraps the release's sweep(index, from, to, ...),
 * which moves `to` back out of whatever is in the way): for the player,
 * noclip and a teleport put `to` where they want it instead. Every one of
 * the player's movement modes goes through it. */
void ts2_dev_sweep(void (*sweep)(void));

/* The camera's look-at (wraps the release's look_at(dst, eye xyz,
 * forward xyz, up xyz), all by value): the free camera's eye and direction
 * at the camera update's own call. */
void ts2_dev_look_at(void (*look_at)(void));

#endif
