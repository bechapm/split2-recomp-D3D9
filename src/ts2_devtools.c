/*
 * ts2_devtools.c -- TimeSplitters 2's debugging tools.
 *
 * For setting up a test scene and holding it still: the developers' own
 * god mode and infinite ammo, every unlock, the game's clock stopped (the
 * world holds still and is still drawn), a readout of where the player is,
 * a saved position to come back to, a free camera, noclip, the game's
 * speed and a wireframe view, all from a menu on the overlay. All of it is
 * TS2's: the toolkit only gives keys, lines on its overlay
 * (xbox_D3D8KeyDown, xbox_D3D8Notice), the pad (xbox_InputWithhold,
 * xbox_InputRead) and the wireframe (xbox_D3D8SetWireframe).
 *
 * The menu: F1, or both sticks clicked together (L3+R3), opens and closes
 * it. Up/down (the D-pad or the arrows) chooses, A or Enter sets, left/right
 * changes a value (the speed), B or Esc closes. The game reads an idle pad
 * while it is open.
 *
 * Every tool starts off. The switches (each a setting in the title's .conf
 * or an environment variable, read once at start-up):
 *
 *   ts2_dev = 0            RECOMP_TS2_DEV          no menu and no keys (on by default)
 *   ts2_god = 1            RECOMP_TS2_GOD          god mode from the start
 *   ts2_infinite_ammo = 1  RECOMP_TS2_INFINITE_AMMO
 *   ts2_unlock_all = 1     RECOMP_TS2_UNLOCK_ALL   every character, level and cheat
 *   ts2_invisible = 1      RECOMP_TS2_INVISIBLE    enemies cannot see the player
 *   ts2_readout = 1        RECOMP_TS2_READOUT      the readout from the start: the
 *                          position, what the middle of the view is on (the
 *                          game's own ray: point, distance, room, door,
 *                          character or pickup and its model) and which
 *                          tools are on
 *   ts2_speed = 0.5        RECOMP_TS2_SPEED        the game's speed: 0.25 0.5 1 2 4
 *   ts2_dev_seq = <frame>:<action>,...  RECOMP_TS2_DEV_SEQ
 *                          the keys' actions at given frames (from the first
 *                          present), for scripted runs: readout god ammo
 *                          unlock save load freecam noclip freeze step
 *                          invisible wireframe faster slower menu hud give
 *                          itemnext itemprev
 *   ts2_hud = 0            RECOMP_TS2_HUD          the game's HUD hidden from the start
 *   ts2_give_item = 11     RECOMP_TS2_GIVE_ITEM    the item "Give item" starts on
 *   ts2_teleport = x,y,z[,yaw,pitch]  RECOMP_TS2_TELEPORT
 *                          where the player is put once the first level is
 *                          playing: a repeatable spot for a scripted run
 *
 * Shortcut keys for the menu's items (only while the game's window is in
 * front):
 *
 *   F1  the menu           F2  god mode        F3  infinite ammo
 *   F4  invisible to enemies                   Home  unlock everything
 *   F5  save position      F6  back to the saved position
 *   F7  free camera        F8  noclip          F12 or Pause: stop the clock
 *   Page Down: one frame forward while the clock is stopped
 *   End  wireframe (the toolkit's, RECOMP_WIREFRAME)
 *   Insert  the game's HUD on and off
 *   - and +  slower and faster
 *
 * Saved positions are kept per level in ts2_positions.txt beside the
 * executable (RECOMP_TS2_POSITIONS names another file), one line each:
 * "<level> <x> <y> <z> <yaw> <pitch>", so a spot survives a restart.
 *
 * What each tool touches, and why that way:
 *
 *   god mode, infinite ammo: two of the developers' own switches, left in
 *     the retail game and reachable there only through a button combination
 *     on several pads. With god set, the damage filter ignores damage to
 *     player-controlled characters and falls do no damage. Written every
 *     frame while on, so a level start that clears them changes nothing.
 *   unlock everything: the developers' unlock switches -- every unlock
 *     condition passes, and the per-kind overrides (story levels,
 *     challenges, arcade league, characters, cheats). Nothing is written to
 *     the save: the unlocks are asked for each time a screen is drawn. (The
 *     developers' combination also wrote them into the save when the first
 *     profile was named DUFFINS; that code is never called from here.)
 *   the clock: the game's own pause switch, which its pause menu sets
 *     (without the menu): the world stops and goes on being drawn, and the
 *     camera still updates, so the free camera flies through a still
 *     scene. A step resumes it for one tick.
 *   speed: the frame clock decides each frame how many whole ticks (1/60 s
 *     each) the game runs, normally 1, more to catch up after a slow frame.
 *     After it, the count is scaled, with the remainder carried: 2x runs two
 *     ticks a frame, 0.5x one every other frame. The game only ever runs
 *     whole ticks, so this is not a way to a higher frame rate.
 *   teleport, noclip: the character sweep, the one function every one of
 *     the player's movement modes moves through, is given the tool's
 *     position as its answer, so the game makes the player's eye, its
 *     character and everything else from it as from any step. The room the
 *     player is in is found as the game's spawn finds it, from the rooms'
 *     bounds and floors (the portal walk where that finds none), so drawing
 *     starts from the right room even through a wall. Noclip keeps god mode on, and for five
 *     seconds after, so landing from a height is not a death.
 *   invisible: an enemy sees its target when its sight ray reaches it;
 *     those rays (and only those: bullets are untouched) reach nothing when
 *     they reach the player. And because awareness, once set, is kept with
 *     no ray at all, the game's awareness of the players is held at
 *     "unaware" (written and read), and an enemy whose target is the player
 *     loses it. Security cameras, alarms and sensors are not covered yet.
 *   free camera: the eye and direction the camera update hands its
 *     look-at, replaced at that one call, and the room drawing starts from
 *     found by the portal walk, so the view, everything made from it and
 *     what graphics tools are given all follow, and rooms are drawn from
 *     where the camera is. The pad (and the keyboard) drive the camera while
 *     it is on, and the game reads a neutral pad.
 *   HUD: the game's HUD draw (all of it: health, ammunition, the radar, the
 *     crosshair, messages) returns at once. The gun in hand stays. A HUD
 *     flash or message waiting meanwhile shows when it is back.
 *   give item: what picking the item up does (the game's own function: its
 *     name shown, the pickup sound), then its ammunition filled and the item
 *     put in hand. A level's pak carries only the guns the level uses, and
 *     the game looks for any other on the disc as a loose file, which is not
 *     there: such an item is marked "not here" and not given.
 */
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ts2_devtools.h"
#include "ts2_mouse.h"
#include "recomp/gen/recomp_cpu.h"   /* g_esp */
#include "recomp/ts2_region.h"

/* Where this release keeps what the tools use: each release's own table. */
#if defined(TS2_REGION_PAL)
#include "../config/pal/devtools.inc"
#elif defined(TS2_REGION_US)
#include "../config/us/devtools.inc"
#else
#error "src/recomp/ts2_region.h names no release this file knows; lift with scripts/build.py"
#endif

/* xboxrecomp: src/kernel, src/config, src/platform, src/d3d, src/input */
extern ptrdiff_t   g_xbox_mem_offset;
extern const char *recomp_config_lookup(const char *env_name, const char *key);
extern int         recomp_config_bool(const char *env_name, const char *key, int fallback);
extern int         host_exe_path(char *buf, size_t bytes);
extern void        host_path_dirname(char *path);
extern int         xbox_D3D8KeyDown(int vk);
extern void        xbox_D3D8Notice(const char *text, unsigned ms);
extern void        xbox_D3D8SetWireframe(int on);
extern int         xbox_D3D8Wireframe(void);
extern void        xbox_InputWithhold(unsigned port, int on);
extern int         xbox_InputRead(unsigned port, uint16_t *buttons, uint8_t analog[8],
                                  int16_t thumbs[4]);

#define DEV_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define DEV_MEMF(a)  (*(volatile float *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define DEV_MEM8(a)  (*(volatile uint8_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define DEV_MEM16(a) (*(volatile uint16_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

/* Windows virtual-key codes (the toolkit translates them off Windows). */
enum {
    VK_F1_ = 0x70, VK_F2_, VK_F3_, VK_F4_, VK_F5_, VK_F6_, VK_F7_, VK_F8_,
    VK_F12_ = 0x7B, VK_PAUSE_ = 0x13, VK_NEXT_ = 0x22, VK_SHIFT_ = 0x10,
    VK_SPACE_ = 0x20, VK_CONTROL_ = 0x11, VK_HOME_ = 0x24, VK_END_ = 0x23,
    VK_RETURN_ = 0x0D, VK_ESCAPE_ = 0x1B, VK_MINUS_ = 0xBD, VK_PLUS_ = 0xBB,
    VK_SUBTRACT_ = 0x6D, VK_ADD_ = 0x6B, VK_INSERT_ = 0x2D,
    VK_LEFT_ = 0x25, VK_UP_, VK_RIGHT_, VK_DOWN_,
};

/* XINPUT wButtons, and the analog buttons' order (A B X Y black white L R). */
enum { PAD_A = 0, PAD_B, PAD_X, PAD_Y, PAD_BLACK, PAD_WHITE, PAD_LT, PAD_RT };
enum {
    PADB_UP = 0x01, PADB_DOWN = 0x02, PADB_LEFT = 0x04, PADB_RIGHT = 0x08,
    PADB_START = 0x10, PADB_BACK = 0x20, PADB_LTHUMB = 0x40, PADB_RTHUMB = 0x80,
};

static const float k_dev_speeds[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
#define DEV_SPEED_NORMAL 2
#define DEV_SPEED_COUNT  ((int)(sizeof k_dev_speeds / sizeof k_dev_speeds[0]))

#define TS2_FRAME_SECONDS (1.0f / 60.0f)   /* the game's logic rate */

static const Ts2DevAddrs *A = &k_ts2_dev;

static struct {
    int  configured, keys, readout;
    int  god, ammo, unlock;            /* what the player asked for */
    int  god_written, ammo_written, unlock_written;
    int  frozen, step, freecam, noclip, invisible;
    int  hud_off;                       /* the game's HUD not drawn */
    int  item;                          /* what "Give item" gives */
    int  god_hold;                      /* frames god stays on after noclip */
    int  speed;                         /* index in k_dev_speeds */
    float speed_carry;                  /* the fraction of a tick not yet run */
    int  menu, menu_row, menu_held, menu_hold_frames;
    uint16_t menu_was;                  /* the menu's controls held last frame */
    int  key_was[256];
    char msg[64];
    int  msg_frames;
    /* teleport: what the next sweeps answer */
    int   tp_sweeps;
    float tp[5];                        /* x y z yaw pitch */
    int   tp_has_angles;
    int   start_tp_pending;
    float start_tp[5];
    int   start_tp_angles;
    /* noclip */
    float nc_pos[3];
    /* free camera */
    float fc_eye[3], fc_fwd_h[3], fc_right[3], fc_pitch;
    float fc_room_eye[3];               /* where fc_room was found */
    int   fc_room;
} D;

/* ── reading the game ──────────────────────────────────────── */

static int dev_ptr_ok(uint32_t p)
{
    return (p >= 0x00010000u && p < 0x04000000u) || (p >= 0x80000000u && p < 0x84000000u);
}

/* A level is playing: its number is a level's, and player 1 has a record,
 * a character and a camera. During boot and the front end the number holds
 * other things, and a tool used then (the readout's ray, a teleport) was
 * working on nothing. */
static uint32_t dev_player(void);
static uint32_t dev_chr(uint32_t p);
static uint32_t dev_context(void);

static int dev_in_level(void)
{
    uint32_t p, ctx, room;

    if (!A->level_id || DEV_MEM32(A->level_id) >= A->level_menus)
        return 0;
    p = dev_player();
    ctx = dev_context();
    if (!p || !dev_chr(p) || !ctx)
        return 0;
    room = DEV_MEM32(ctx + TS2_CTX_ROOM);
    return room < 0x1000u && DEV_MEM32(p + TS2_P_ROOM) < 0x1000u;
}

/* Player 1's record, or 0. */
static uint32_t dev_player(void)
{
    uint32_t base;

    if (!A->player_base)
        return 0;
    base = DEV_MEM32(A->player_base);
    return dev_ptr_ok(base) ? base : 0;
}

static uint32_t dev_chr(uint32_t p)
{
    uint32_t c = p ? DEV_MEM32(p + TS2_P_CHR) : 0;

    return dev_ptr_ok(c) ? c : 0;
}

static uint32_t dev_context(void)
{
    uint32_t ctx = A->context ? DEV_MEM32(A->context) : 0;

    return dev_ptr_ok(ctx) ? ctx : 0;
}

/* A call of a game function from here: its cdecl arguments pushed on the
 * guest stack below the caller's (and below `scratch` bytes kept for the
 * points it reads and writes, at the returned address), a return address of
 * none, and its result. The registers a caller of the hook may still want
 * (the hooked function's result) are kept. */
static uint32_t dev_scratch(uint32_t bytes)
{
    return (g_esp - 0x10u - bytes) & ~0xFu;
}

static uint32_t dev_guest_call(void (*fn)(void), uint32_t below, const uint32_t *args, int n)
{
    uint32_t top = g_esp, eax = g_eax, ecx = g_ecx, edx = g_edx, result;
    int i;

    g_esp = below;
    for (i = n - 1; i >= 0; i--) {
        g_esp -= 4;
        DEV_MEM32(g_esp) = args[i];
    }
    g_esp -= 4;
    DEV_MEM32(g_esp) = 0;               /* return address: none */
    fn();
    result = g_eax;
    g_esp = top;
    g_eax = eax;
    g_ecx = ecx;
    g_edx = edx;
    return result;
}

static void dev_put3(uint32_t at, const float v[3])
{
    int i;

    for (i = 0; i < 3; i++)
        DEV_MEMF(at + 4u * (uint32_t)i) = v[i];
}

/* room_walk(from, from_room, to, 0): the room holding `to`, or -1. */
static int dev_room_walk(const float from[3], int from_room, const float to[3])
{
    uint32_t pts = dev_scratch(0x20), args[4];
    int room;

    if (!A->room_walk || from_room < 0)
        return -1;
    dev_put3(pts, from);
    dev_put3(pts + 0x10, to);
    args[0] = pts;
    args[1] = (uint32_t)from_room;
    args[2] = pts + 0x10;
    args[3] = 0;
    room = (int)dev_guest_call(A->room_walk, pts, args, 4);
    return room >= 0 && room < 0x10000 ? room : -1;
}

/* room_at(pos, 0.1, 0.1), as the player's spawn asks it: the room holding
 * `pos` by the rooms' bounds and floors, or -1. */
static int dev_room_at(const float pos[3])
{
    uint32_t pts = dev_scratch(0x10), args[3];
    int room;
    union { float f; uint32_t u; } tenth = { 0.1f };

    if (!A->room_at)
        return -1;
    dev_put3(pts, pos);
    args[0] = pts;
    args[1] = tenth.u;
    args[2] = tenth.u;
    room = (int)dev_guest_call(A->room_at, pts, args, 3);
    return room > 0 && room < 0x1000 ? room : -1;
}

/* The room holding `to`: by position first, since a walk from `from` crosses
 * no portal through a wall and stays in the old room (then only the sky was
 * drawn); by the walk where the position finds none. */
static int dev_room_find(const float from[3], int from_room, const float to[3])
{
    int room = dev_room_at(to);

    return room >= 0 ? room : dev_room_walk(from, from_room, to);
}

static void dev_say(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(D.msg, sizeof D.msg, fmt, ap);
    va_end(ap);
    D.msg_frames = 120;
    fprintf(stderr, "[TS2-DEV] %s\n", D.msg);
    fflush(stderr);
}

/* A key's press, once. */
static int dev_pressed(int vk)
{
    int down = xbox_D3D8KeyDown(vk);
    int was = D.key_was[vk & 0xFF];

    D.key_was[vk & 0xFF] = down;
    return down && !was;
}

/* ── switches ──────────────────────────────────────────────── */

static void dev_write_switches(void)
{
    int god = D.god || D.noclip || D.god_hold > 0;
    uint32_t i;

    /* Every frame while on; once when turned off, so the game's own use of
     * them is left alone after. */
    if (A->god && (god || D.god_written)) {
        DEV_MEM32(A->god) = god ? 1u : 0u;
        D.god_written = god;
    }
    if (A->infinite_ammo && (D.ammo || D.ammo_written)) {
        DEV_MEM32(A->infinite_ammo) = D.ammo ? 1u : 0u;
        D.ammo_written = D.ammo;
    }
    if (A->unlock_all && (D.unlock || D.unlock_written)) {
        DEV_MEM32(A->unlock_all) = D.unlock ? 1u : 0u;
        for (i = 0; i < A->unlock_override_count; i++)
            DEV_MEM8(A->unlock_override + i) = D.unlock ? 1u : 0u;
        D.unlock_written = D.unlock;
    }
    if (D.god_hold > 0)
        D.god_hold--;
}

/* ── the clock ─────────────────────────────────────────────── */

/* The game's pause switch: >0 paused (ticks 0, time scale 0), -1 resuming
 * (one tick, then the game sets it back to 0). The frame clock reads it at
 * the start of each frame, so a write here takes effect from the next. */
static void dev_freeze(int on)
{
    if (!A->pause || on == D.frozen)
        return;
    if (!on && (int32_t)DEV_MEM32(A->pause) > 0)
        DEV_MEM32(A->pause) = (uint32_t)-1;
    D.frozen = on;
    D.step = 0;
}

/* While stopped, held paused every frame (the pause menu, opened and closed
 * meanwhile, would resume it); a step resumes it for the next frame only. */
static void dev_write_clock(void)
{
    if (!D.frozen)
        return;
    if (D.step) {
        D.step = 0;
        DEV_MEM32(A->pause) = (uint32_t)-1;
        return;
    }
    if ((int32_t)DEV_MEM32(A->pause) <= 0)
        DEV_MEM32(A->pause) = 1;
}

/* clock() is the game's frame clock: it sets the ticks this frame runs and
 * the time scale (the same number, as a float), from the vertical blanks
 * since the last frame and the pause switch. The count is scaled after it,
 * the remainder carried to the next frame. */
void ts2_dev_clock(void (*clock)(void))
{
    uint32_t ticks, run;
    float want;

    clock();
    if (D.speed == DEV_SPEED_NORMAL || !A->ticks || !A->time_scale)
        return;
    ticks = DEV_MEM32(A->ticks);
    if (!ticks || ticks > 8)
        return;                         /* paused, or not a count */
    want = (float)ticks * k_dev_speeds[D.speed] + D.speed_carry;
    run = (uint32_t)want;
    D.speed_carry = want - (float)run;
    if (run > 12)
        run = 12;
    if (g_eax == ticks)
        g_eax = run;
    DEV_MEM32(A->ticks) = run;
    DEV_MEMF(A->time_scale) = (float)run;
}

/* ── saved positions ───────────────────────────────────────── */

static const char *dev_positions_path(void)
{
    static char path[1024];

    if (!path[0]) {
        const char *v = recomp_config_lookup("RECOMP_TS2_POSITIONS", NULL);

        if (v && *v) {
            snprintf(path, sizeof path, "%s", v);
        } else if (host_exe_path(path, sizeof path)) {
            host_path_dirname(path);
            strncat(path, "/ts2_positions.txt", sizeof path - strlen(path) - 1);
        } else {
            snprintf(path, sizeof path, "ts2_positions.txt");
        }
    }
    return path;
}

/* The saved line for this level, or 0. */
static int dev_position_load(uint32_t level, float out[5])
{
    FILE *f = fopen(dev_positions_path(), "r");
    char line[256];
    int found = 0;

    if (!f)
        return 0;
    while (fgets(line, sizeof line, f)) {
        unsigned lv;
        float v[5];

        if (sscanf(line, "%u %f %f %f %f %f", &lv, &v[0], &v[1], &v[2], &v[3], &v[4]) == 6 &&
            lv == level) {
            memcpy(out, v, sizeof v);
            found = 1;                  /* the last one wins */
        }
    }
    fclose(f);
    return found;
}

/* Rewrites the file with this level's line replaced. */
static int dev_position_store(uint32_t level, const float v[5])
{
    const char *path = dev_positions_path();
    static char lines[64][256];
    int n = 0, i;
    FILE *f = fopen(path, "r");

    if (f) {
        while (n < 63 && fgets(lines[n], sizeof lines[n], f)) {
            unsigned lv;

            if (sscanf(lines[n], "%u", &lv) == 1 && lv == level)
                continue;
            n++;
        }
        fclose(f);
    }
    snprintf(lines[n++], sizeof lines[0], "%u %.3f %.3f %.3f %.2f %.2f\n",
             (unsigned)level, v[0], v[1], v[2], v[3], v[4]);
    f = fopen(path, "w");
    if (!f)
        return 0;
    for (i = 0; i < n; i++)
        fputs(lines[i], f);
    fclose(f);
    return 1;
}

static void dev_teleport(const float v[5], int with_angles)
{
    memcpy(D.tp, v, sizeof D.tp);
    D.tp_has_angles = with_angles;
    D.tp_sweeps = 3;                    /* a few, so it settles */
    D.nc_pos[0] = v[0];
    D.nc_pos[1] = v[1];
    D.nc_pos[2] = v[2];
}

/* "x,y,z[,yaw,pitch]" */
static int dev_parse_spot(const char *s, float v[5], int *angles)
{
    int n = sscanf(s, "%f,%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3], &v[4]);

    *angles = n == 5;
    return n == 3 || n == 5;
}

/* ── moving things with the pad ────────────────────────────── */

static float dev_axis(int16_t v)
{
    float f = (float)v / 32767.0f;

    if (f > -0.2f && f < 0.2f)
        return 0.0f;
    return f > 0.0f ? (f - 0.2f) / 0.8f : (f + 0.2f) / 0.8f;
}

static void dev_normalize(float v[3])
{
    float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    if (l > 1e-6f) {
        v[0] /= l;
        v[1] /= l;
        v[2] /= l;
    }
}

/* What the free camera reads: the pad on port 1 (which the game is not
 * reading while it is on) and the keyboard, which the game cannot hear then
 * either: WASD to move, the arrows to turn, Space/Ctrl up and down, Shift
 * faster. Out: move (forward, right, up) and turn (right, up), -1..1. */
static void dev_freecam_input(float move[3], float turn[2], float *speed)
{
    uint16_t buttons = 0;
    uint8_t analog[8] = { 0 };
    int16_t sticks[4] = { 0 };
    int i;

    xbox_InputRead(0, &buttons, analog, sticks);
    move[0] = dev_axis(sticks[1]);
    move[1] = dev_axis(sticks[0]);
    move[2] = (float)analog[PAD_RT] / 255.0f - (float)analog[PAD_LT] / 255.0f;
    turn[0] = dev_axis(sticks[2]);
    turn[1] = dev_axis(sticks[3]);
    *speed = analog[PAD_A] > 64 || xbox_D3D8KeyDown(VK_SHIFT_) ? 4.0f : 1.0f;
    if (analog[PAD_B] > 64)
        *speed = 0.25f;

    if (xbox_D3D8KeyDown('W')) move[0] += 1.0f;
    if (xbox_D3D8KeyDown('S')) move[0] -= 1.0f;
    if (xbox_D3D8KeyDown('D')) move[1] += 1.0f;
    if (xbox_D3D8KeyDown('A')) move[1] -= 1.0f;
    if (xbox_D3D8KeyDown(VK_SPACE_))   move[2] += 1.0f;
    if (xbox_D3D8KeyDown(VK_CONTROL_)) move[2] -= 1.0f;
    if (xbox_D3D8KeyDown(VK_RIGHT_)) turn[0] += 1.0f;
    if (xbox_D3D8KeyDown(VK_LEFT_))  turn[0] -= 1.0f;
    if (xbox_D3D8KeyDown(VK_UP_))    turn[1] += 1.0f;
    if (xbox_D3D8KeyDown(VK_DOWN_))  turn[1] -= 1.0f;
    /* A key bound to the pad (WASD on the left stick) is read twice. */
    for (i = 0; i < 3; i++)
        move[i] = move[i] > 1.0f ? 1.0f : move[i] < -1.0f ? -1.0f : move[i];
    for (i = 0; i < 2; i++)
        turn[i] = turn[i] > 1.0f ? 1.0f : turn[i] < -1.0f ? -1.0f : turn[i];
}

/* Start from the camera as the game left it: its eye, its right row and
 * the forward (the look-at makes the third row -forward, measured: taken as
 * it was, the camera flew backwards), kept as a level forward, a level right and a pitch, and
 * turned in the plane of those two, so the camera never needs to know which
 * way round the game's axes are. Its room is the player's to begin with:
 * in first person the eye is the player's. */
static int dev_freecam_start(uint32_t p)
{
    uint32_t ctx = dev_context();
    uint32_t w;
    float f[3];
    int i;

    if (!ctx)
        return 0;
    w = ctx + TS2_CTX_WORLD;
    for (i = 0; i < 3; i++) {
        D.fc_right[i] = DEV_MEMF(w + 4u * (uint32_t)i);
        f[i] = -DEV_MEMF(w + 0x20u + 4u * (uint32_t)i);   /* the row is -forward */
        D.fc_eye[i] = DEV_MEMF(w + 0x30u + 4u * (uint32_t)i);
        D.fc_room_eye[i] = p ? DEV_MEMF(p + TS2_P_EYE + 4u * (uint32_t)i) : D.fc_eye[i];
    }
    D.fc_room = p ? (int)DEV_MEM32(p + TS2_P_ROOM) : (int)DEV_MEM32(ctx + TS2_CTX_ROOM);
    D.fc_pitch = asinf(f[1] < -1.0f ? -1.0f : f[1] > 1.0f ? 1.0f : f[1]);
    D.fc_fwd_h[0] = f[0];
    D.fc_fwd_h[1] = 0.0f;
    D.fc_fwd_h[2] = f[2];
    D.fc_right[1] = 0.0f;
    dev_normalize(D.fc_fwd_h);
    dev_normalize(D.fc_right);
    return 1;
}

static void dev_freecam_forward(float f[3])
{
    float c = cosf(D.fc_pitch);

    f[0] = D.fc_fwd_h[0] * c;
    f[1] = sinf(D.fc_pitch);
    f[2] = D.fc_fwd_h[2] * c;
}

static void dev_freecam_step(void)
{
    const float turn_rate = 2.2f;       /* radians a second at full stick */
    float move[3], turn[2], speed, f[3], a, c, s, h[3], r[3];
    float step = 6.0f * TS2_FRAME_SECONDS;
    int i;

    dev_freecam_input(move, turn, &speed);
    a = turn[0] * turn_rate * TS2_FRAME_SECONDS;
    if (a != 0.0f) {
        c = cosf(a);
        s = sinf(a);
        for (i = 0; i < 3; i++) {
            h[i] = D.fc_fwd_h[i] * c + D.fc_right[i] * s;
            r[i] = D.fc_right[i] * c - D.fc_fwd_h[i] * s;
        }
        memcpy(D.fc_fwd_h, h, sizeof h);
        memcpy(D.fc_right, r, sizeof r);
        dev_normalize(D.fc_fwd_h);
        dev_normalize(D.fc_right);
    }
    D.fc_pitch += turn[1] * turn_rate * TS2_FRAME_SECONDS;
    if (D.fc_pitch > 1.5f) D.fc_pitch = 1.5f;
    if (D.fc_pitch < -1.5f) D.fc_pitch = -1.5f;

    dev_freecam_forward(f);
    step *= speed;
    for (i = 0; i < 3; i++)
        D.fc_eye[i] += (f[i] * move[0] + D.fc_right[i] * move[1]) * step;
    D.fc_eye[1] += move[2] * step;
}

/* Noclip moves with the game's own controls (the game still turns the
 * player): the left stick along the view direction, pitch included, so
 * looking up and pushing forward climbs. */
static void dev_noclip_step(uint32_t p)
{
    uint16_t buttons = 0;
    uint8_t analog[8] = { 0 };
    int16_t sticks[4] = { 0 };
    uint32_t ctx = dev_context();
    float f[3], r[3], fwd, side, step = 8.0f * TS2_FRAME_SECONDS;
    int i;

    xbox_InputRead(0, &buttons, analog, sticks);
    fwd = dev_axis(sticks[1]);
    side = dev_axis(sticks[0]);
    if (analog[PAD_A] > 64 || xbox_D3D8KeyDown(VK_SHIFT_))
        step *= 4.0f;
    for (i = 0; i < 3; i++) {
        f[i] = DEV_MEMF(p + TS2_P_VIEW_DIR + 4u * (uint32_t)i);
        r[i] = ctx ? DEV_MEMF(ctx + TS2_CTX_WORLD + 4u * (uint32_t)i) : 0.0f;
    }
    r[1] = 0.0f;
    dev_normalize(r);
    for (i = 0; i < 3; i++)
        D.nc_pos[i] += (f[i] * fwd + r[i] * side) * step;
}

/* ── hooks ─────────────────────────────────────────────────── */

/* sweep(index, float *from, float *to, height, radius, out, 1, 0): `to` is
 * corrected in place, and the movement copies it into the player's
 * position. For the player, under noclip or a teleport, it is given the
 * tool's position, and the player's room follows by the portal walk. */
void ts2_dev_sweep(void (*sweep)(void))
{
    uint32_t p, from, to, index;
    float a[3], b[3];
    int i, room;

    if (!(D.noclip || D.tp_sweeps > 0)) {
        sweep();
        return;
    }
    index = DEV_MEM32(g_esp + 4);
    from = DEV_MEM32(g_esp + 8);
    to = DEV_MEM32(g_esp + 12);
    sweep();                            /* g_esp is back at the return address */
    p = dev_player();
    if (!p || index != DEV_MEM32(p + TS2_P_INDEX) || !dev_ptr_ok(from) || !dev_ptr_ok(to))
        return;
    for (i = 0; i < 3; i++) {
        a[i] = DEV_MEMF(from + 4u * (uint32_t)i);
        b[i] = D.tp_sweeps > 0 ? D.tp[i] : D.nc_pos[i];
        DEV_MEMF(to + 4u * (uint32_t)i) = b[i];
    }
    room = dev_room_find(a, (int)DEV_MEM32(p + TS2_P_ROOM), b);
    if (room >= 0) {
        uint32_t c = dev_chr(p);

        /* where the spawn puts it */
        DEV_MEM32(p + TS2_P_ROOM) = (uint32_t)room;
        DEV_MEM32(p + TS2_P_ROOM2) = (uint32_t)room;
        if (c)
            DEV_MEM32(c + TS2_O_ROOM) = (uint32_t)room;
    }
    if (D.tp_sweeps > 0) {
        D.tp_sweeps--;
        if (D.tp_has_angles) {
            DEV_MEMF(p + TS2_P_YAW) = D.tp[3];
            DEV_MEMF(p + TS2_P_PITCH) = D.tp[4];
        }
    }
}

/* ray(room, start, delta, infinite, hit, normal, room_out, mask, first,
 * passed) returns the object hit. An enemy decides it can see its target
 * when its sight ray hits the target itself: under invisibility, those
 * rays (by their return addresses) hit nothing when they hit the player.
 * Bullets and everything else are left alone. */
void ts2_dev_ray(void (*ray)(void))
{
    uint32_t ret, c;
    int i;

    if (!D.invisible) {
        ray();
        return;
    }
    ret = DEV_MEM32(g_esp);
    ray();
    for (i = 0; i < 8 && A->sight_returns[i]; i++)
        if (A->sight_returns[i] == ret) {
            c = dev_chr(dev_player());
            if (c && g_eax == c)
                g_eax = 0;
            return;
        }
}

/* A character index that is a player's. */
static int dev_is_player_index(uint32_t i)
{
    uint32_t n = A->player_count ? DEV_MEM32(A->player_count) : 0;

    return n <= 4 && i < n;
}

/* set(a, b, state) makes a and b aware of each other: under invisibility,
 * a player is made unaware instead (state 0), so no enemy takes it up by
 * any route. */
void ts2_dev_aware_set(void (*set)(void))
{
    if (D.invisible && (dev_is_player_index(DEV_MEM32(g_esp + 4)) ||
                        dev_is_player_index(DEV_MEM32(g_esp + 8))))
        DEV_MEM32(g_esp + 12) = 0;
    set();
}

/* get(a, b): the state, 0 unaware. Under invisibility a player is unnoticed
 * whatever was set before it was turned on. */
void ts2_dev_aware_get(void (*get)(void))
{
    if (D.invisible && (dev_is_player_index(DEV_MEM32(g_esp + 4)) ||
                        dev_is_player_index(DEV_MEM32(g_esp + 8)))) {
        g_eax = 0;
        g_esp += 4;                     /* ret */
        return;
    }
    get();
}

/* check(R, refresh): whether an enemy's target R+0x1300 is still one. Under
 * invisibility an enemy whose target is the player has none, and the game's
 * own check then says so. */
void ts2_dev_target_check(void (*check)(void))
{
    uint32_t r = DEV_MEM32(g_esp + 4), c;

    if (D.invisible && dev_ptr_ok(r)) {
        c = dev_chr(dev_player());
        if (c && DEV_MEM32(r + TS2_R_TARGET) == c)
            DEV_MEM32(r + TS2_R_TARGET) = 0;
    }
    check();
}

/* Binding writes each material's texture slot over its id, in the model's
 * own data. A file held whole in a pak's memory stays there: a level
 * restarted loads the same copy again, ids already replaced, so for those
 * the ids are kept here the first time a record is bound and put back
 * before it is bound again. A file read into a buffer of its own (a mod's)
 * is fresh each load and bound as it is. */
static struct { uint32_t rec, id; } g_dev_mat[2048];
static int g_dev_mat_n;
static uint32_t g_dev_prepared[256];    /* material lists already prepared */
static int g_dev_prepared_n;
static uint32_t g_dev_mat_level = 0xFFFFFFFFu;

/* Another level's paks are other memory: what was kept is forgotten when a
 * different level starts (a restart of the same one keeps its paks). */
static void dev_bind_level(uint32_t level)
{
    if (level != g_dev_mat_level) {
        g_dev_mat_level = level;
        g_dev_mat_n = 0;
        g_dev_prepared_n = 0;
    }
}

/* The buffer the model loader's fallback read the file into, while the
 * loader runs. */
static int      g_dev_model_loading;
static uint32_t g_dev_read_path, g_dev_read_buf;

void ts2_dev_file_read(void (*read)(void))
{
    uint32_t path = DEV_MEM32(g_esp + 4);

    read();
    if (g_dev_model_loading) {
        g_dev_read_path = path;
        g_dev_read_buf = g_eax;
    }
}

static void dev_bind_textures(uint32_t path, uint32_t m)
{
    uint32_t list = DEV_MEM32(m + A->model_materials), data, args[3], rec, id;
    int i, k, prepared = 0, own = 0;

    if (!dev_ptr_ok(list))
        return;
    /* The file held whole in a pak's memory, else the buffer it was read
     * into. */
    data = dev_guest_call(A->pak_data, dev_scratch(0), &path, 1);
    if (!dev_ptr_ok(data) && g_dev_read_path == path) {
        /* A buffer of its own, fresh each load: nothing kept applies to it,
         * and its address may be another model's from before. */
        data = g_dev_read_buf;
        own = 1;
    }
    if (!dev_ptr_ok(data)) {
        fprintf(stderr, "[TS2-DEV] a model loaded mid-level: its data was not found; "
                "its textures are not bound\n");
        return;
    }
    for (i = 0; !own && i < g_dev_prepared_n; i++)
        prepared |= g_dev_prepared[i] == list;
    if (!prepared) {
        args[0] = list;
        args[1] = data;
        dev_guest_call(A->material_prepare, dev_scratch(0), args, 2);
        if (!own && g_dev_prepared_n < (int)(sizeof g_dev_prepared / sizeof g_dev_prepared[0]))
            g_dev_prepared[g_dev_prepared_n++] = list;
    }
    list = DEV_MEM32(m + A->model_materials);
    for (rec = list, i = 0; dev_ptr_ok(rec) && i < 512; rec += 0x10, i++) {
        id = DEV_MEM32(rec);
        for (k = 0; !own && k < g_dev_mat_n && g_dev_mat[k].rec != rec; k++)
            ;
        if (own) {
            /* bound once, as loaded */
        } else if (k < g_dev_mat_n) {
            id = g_dev_mat[k].id;
            DEV_MEM32(rec) = id;
        } else if (id != 0xFFFFFFFFu && g_dev_mat_n < (int)(sizeof g_dev_mat / sizeof g_dev_mat[0])) {
            g_dev_mat[g_dev_mat_n].rec = rec;
            g_dev_mat[g_dev_mat_n].id = id;
            g_dev_mat_n++;
        }
        if (id == 0xFFFFFFFFu)
            break;
        if (id == 0x40000000u)
            continue;
        args[0] = id;
        args[1] = rec;
        args[2] = rec;
        dev_guest_call(DEV_MEM32(rec + 0xC) & 0x10000000u ? A->texture_load_alt : A->texture_load,
                       dev_scratch(0), args, 3);
    }
}

/* load(path) returns the model. One the resident resources do not hold was
 * not bound at level load: while a level plays, with the tools on, its
 * textures are bound here. */
void ts2_dev_model_load(void (*load)(void))
{
    uint32_t path = DEV_MEM32(g_esp + 4), m;

    g_dev_read_path = g_dev_read_buf = 0;
    g_dev_model_loading++;
    load();
    g_dev_model_loading--;
    m = g_eax;
    if (!D.keys || !A->texture_load || !A->material_prepare || !A->pak_data ||
        !dev_ptr_ok(m) || !dev_ptr_ok(path) || !dev_in_level())
        return;
    if (A->resident_find && dev_guest_call(A->resident_find, dev_scratch(0), &path, 1))
        return;
    dev_bind_level(DEV_MEM32(A->level_id));
    dev_bind_textures(path, m);
}

/* look_at(dst, eye x y z, forward x y z, up x y z), all by value: at the
 * camera update's call, the free camera's eye and forward instead, and the
 * room drawing starts from walked to from where it was found last. */
void ts2_dev_look_at(void (*look_at)(void))
{
    uint32_t ctx;
    float f[3];
    int i, room;

    if (!D.freecam || !A->look_at_return || DEV_MEM32(g_esp) != A->look_at_return) {
        look_at();
        return;
    }
    dev_freecam_forward(f);
    for (i = 0; i < 3; i++) {
        DEV_MEMF(g_esp + 0x08u + 4u * (uint32_t)i) = D.fc_eye[i];
        DEV_MEMF(g_esp + 0x14u + 4u * (uint32_t)i) = f[i];
    }
    ctx = dev_context();
    if (ctx) {
        room = dev_room_find(D.fc_room_eye, D.fc_room, D.fc_eye);
        if (room >= 0) {
            D.fc_room = room;
            memcpy(D.fc_room_eye, D.fc_eye, sizeof D.fc_eye);
        }
        DEV_MEM32(ctx + TS2_CTX_ROOM) = (uint32_t)D.fc_room;
    }
    look_at();
}

/* hud() draws the in-game HUD, and takes no arguments: hidden, it returns
 * at once, popping its return address as its own ret would. */
void ts2_dev_hud(void (*hud)(void))
{
    if (D.hud_off) {
        g_esp += 4;                     /* ret */
        return;
    }
    hud();
}

/* ── items ─────────────────────────────────────────────────── */

/* The English text for a text id, into buf: the table's strings are bytes or
 * UTF-16, and either is copied as ASCII. Empty when there is none. */
static const char *dev_text(uint32_t id, char *buf, size_t n)
{
    uint32_t s;
    size_t i;
    int wide;

    buf[0] = 0;
    if (!A->text_table || id >= A->text_count || n < 2)
        return buf;
    s = DEV_MEM32(A->text_table + id * 4u);
    if (!dev_ptr_ok(s))
        return buf;
    wide = DEV_MEM8(s) && !DEV_MEM8(s + 1);
    for (i = 0; i < n - 1; i++) {
        uint32_t ch = wide ? DEV_MEM16(s + 2u * (uint32_t)i) : DEV_MEM8(s + (uint32_t)i);

        if (!ch)
            break;
        buf[i] = ch >= 32 && ch < 127 ? (char)ch : '?';
    }
    buf[i] = 0;
    return buf;
}

static const char *dev_item_name(int item, char *buf, size_t n)
{
    if (A->item_names)
        dev_text(DEV_MEM16(A->item_names + (uint32_t)item * A->item_name_stride), buf, n);
    else
        buf[0] = 0;
    if (!buf[0])
        snprintf(buf, n, "item %d", item);
    return buf;
}

/* Whether model m is there to load: loaded already, resident, or in the
 * level's pak -- the places the game looks before the loose file on the
 * disc, which a gun the level does not carry is not. */
static int dev_model_present(uint32_t m)
{
    uint32_t path;

    if (m >= 2048)
        return 1;                       /* none: nothing to load */
    if (DEV_MEM32(A->model_loaded + m * 0x24u))
        return 1;
    path = DEV_MEM32(A->object_list + m * 0x24u);
    if (!dev_ptr_ok(path))
        return 1;
    if (A->resident_find && dev_guest_call(A->resident_find, dev_scratch(0), &path, 1))
        return 1;
    {
        uint32_t current = A->pak_current ? DEV_MEM32(A->pak_current) : 0;
        int found = (A->pak_find_p4 && dev_guest_call(A->pak_find_p4, dev_scratch(0), &path, 1)) ||
                    (A->pak_find_p8 && dev_guest_call(A->pak_find_p8, dev_scratch(0), &path, 1));

        if (A->pak_current)
            DEV_MEM32(A->pak_current) = current;    /* the game's last lookup */
        return found;
    }
}

/* Whether the level can give the item: each of its weapons' models, first-
 * and third-person, is there to load. 1 for an item with no weapon, and
 * where this release's tables are not known. */
static int dev_item_givable(int item)
{
    uint32_t d;
    int32_t type;
    int side;

    if (!A->item_table || !A->weapon_defs || !A->model_loaded || !A->object_list)
        return 1;
    for (side = 0; side < 2; side++) {
        type = (int32_t)DEV_MEM32(A->item_table + (uint32_t)item * A->item_stride + 4u * (uint32_t)side);
        if (type < 0 || (uint32_t)type >= A->weapon_type_count)
            continue;
        d = A->weapon_defs + (uint32_t)type * A->weapon_def_stride;
        if (!dev_model_present(DEV_MEM32(d)) || !dev_model_present(DEV_MEM32(d + 4)))
            return 0;
    }
    return 1;
}

/* Player 1's character record (R), or 0. */
static uint32_t dev_record(uint32_t p)
{
    uint32_t c = dev_chr(p), r = c ? DEV_MEM32(c + TS2_C_RECORD) : 0;

    return dev_ptr_ok(r) ? r : 0;
}

/* One HUD texture, by its index in the named table: loaded now, as a level
 * load would have, when the level never asked for it. */
static void dev_load_named(uint32_t i)
{
    uint32_t e, name, rec, args[3];
    int32_t h;
    int added = 0;

    if (i >= A->named_count)
        return;
    e = A->named_textures + i * 0x10u;
    if ((int32_t)DEV_MEM32(e + 0xC) >= 0)
        return;                         /* loaded */
    name = DEV_MEM32(e + 8);
    if (!dev_ptr_ok(name))
        return;
    rec = dev_guest_call(A->named_record, dev_scratch(0), &name, 1);
    if (!dev_ptr_ok(rec)) {
        /* No record: ask for it as the level start does, which adds one. */
        DEV_MEM32(e + 0xC) = (uint32_t)-2;
        args[0] = i;
        dev_guest_call(A->named_request, dev_scratch(0), args, 1);
        rec = dev_guest_call(A->named_record, dev_scratch(0), &name, 1);
        if (!dev_ptr_ok(rec)) {
            DEV_MEM32(e + 0xC) = (uint32_t)-1;
            return;
        }
        /* A new record's handle is 0 (the table is zero-filled), which is a
         * slot: it is loaded whatever it says. */
        DEV_MEM32(rec + 0x14) = (uint32_t)-1;
        added = 1;
    }
    h = (int32_t)DEV_MEM32(rec + 0x14);
    if (h < 0 || added) {
        args[0] = name;
        args[1] = DEV_MEM32(rec + 0x10);
        args[2] = rec + 0x14;
        dev_guest_call(A->named_load, dev_scratch(0), args, 3);
        h = (int32_t)DEV_MEM32(rec + 0x14);
    }
    DEV_MEM32(e + 0xC) = h >= 0 ? (uint32_t)h : (uint32_t)-1;
}

/* The HUD textures of an item's guns: their ammunition types' entries, as
 * the game's own weapon set-up names them. A gun the level carries has
 * them; one given from another level's data drew a white rectangle. */
static void dev_load_item_hud(int item)
{
    uint32_t d, a, base, e;
    int32_t type, t;
    int side, ammo, k;

    if (!A->named_textures || !A->named_record || !A->named_load || !A->named_request ||
        !A->ammo_hud || !A->item_table || !A->weapon_defs)
        return;
    for (side = 0; side < 2; side++) {
        type = (int32_t)DEV_MEM32(A->item_table + (uint32_t)item * A->item_stride + 4u * (uint32_t)side);
        if (type < 0 || (uint32_t)type >= A->weapon_type_count)
            continue;
        d = A->weapon_defs + (uint32_t)type * A->weapon_def_stride;
        for (ammo = 0; ammo < 2; ammo++) {
            t = (int32_t)DEV_MEM32(d + 0x10u + 8u * (uint32_t)ammo);
            if (t <= 0 || (uint32_t)t >= A->ammo_count)
                continue;
            base = A->ammo_hud + (uint32_t)t * A->ammo_stride;
            dev_load_named(DEV_MEM32(base));
            for (k = 0; k < 4; k++) {
                e = base + 4u + 0x20u * (uint32_t)k;
                if (!DEV_MEM32(e))
                    break;
                for (a = 4; a <= 8; a += 4)
                    if ((int32_t)DEV_MEM32(e + a) >= 0)
                        dev_load_named(DEV_MEM32(e + a));
            }
        }
    }
}

/* The chosen item, as picking it up gives it (its name shown, the pickup
 * sound), its ammunition full, and in hand. */
static void dev_give(uint32_t p)
{
    uint32_t r = dev_record(p), args[2];
    char name[40];

    dev_item_name(D.item, name, sizeof name);
    if (!A->give_item) {
        dev_say("give item: not found in this release");
        return;
    }
    if (!r || !dev_in_level()) {
        dev_say("give item: not in a level");
        return;
    }
    if (!dev_item_givable(D.item)) {
        dev_say("%s is not in this level", name);
        return;
    }
    args[0] = r;
    args[1] = (uint32_t)D.item;
    dev_guest_call(A->give_item, dev_scratch(0), args, 2);
    if (A->fill_ammo)
        dev_guest_call(A->fill_ammo, dev_scratch(0), args, 2);
    DEV_MEM32(r + TS2_R_ITEM) = (uint32_t)D.item;
    dev_load_item_hud(D.item);
    dev_say("gave %s (%d)", name, D.item);
}

/* ── once a frame ──────────────────────────────────────────── */

/* What a key (or a step of ts2_dev_seq) does. */
enum {
    ACT_READOUT, ACT_GOD, ACT_AMMO, ACT_UNLOCK, ACT_SAVE, ACT_LOAD,
    ACT_FREECAM, ACT_NOCLIP, ACT_FREEZE, ACT_STEP, ACT_INVISIBLE, ACT_WIREFRAME,
    ACT_FASTER, ACT_SLOWER, ACT_MENU, ACT_HUD, ACT_GIVE, ACT_ITEM_NEXT, ACT_ITEM_PREV,
    ACT_COUNT
};

static const char *const k_dev_actions[ACT_COUNT] = {
    "readout", "god", "ammo", "unlock", "save", "load",
    "freecam", "noclip", "freeze", "step", "invisible", "wireframe",
    "faster", "slower", "menu", "hud", "give", "itemnext", "itemprev",
};

static void dev_action(int act, uint32_t p)
{
    float v[5];

    switch (act) {
    case ACT_READOUT:
        D.readout = !D.readout;
        dev_say("readout %s", D.readout ? "on" : "off");
        break;
    case ACT_GOD:
        D.god = !D.god;
        dev_say(A->god ? "god mode %s" : "god mode: not found in this release",
                D.god ? "ON" : "off");
        break;
    case ACT_AMMO:
        D.ammo = !D.ammo;
        dev_say(A->infinite_ammo ? "infinite ammo %s" : "infinite ammo: not found",
                D.ammo ? "ON" : "off");
        break;
    case ACT_UNLOCK:
        D.unlock = !D.unlock;
        dev_say(A->unlock_all ? "unlock everything %s" : "unlock everything: not found",
                D.unlock ? "ON" : "off");
        break;
    case ACT_SAVE:
        if (p && dev_in_level()) {
            v[0] = DEV_MEMF(p + TS2_P_POS);
            v[1] = DEV_MEMF(p + TS2_P_POS + 4);
            v[2] = DEV_MEMF(p + TS2_P_POS + 8);
            v[3] = DEV_MEMF(p + TS2_P_YAW);
            v[4] = DEV_MEMF(p + TS2_P_PITCH);
            if (dev_position_store(DEV_MEM32(A->level_id), v))
                dev_say("position saved (level %u)", (unsigned)DEV_MEM32(A->level_id));
            else
                dev_say("could not write %s", dev_positions_path());
        } else {
            dev_say("save position: not in a level");
        }
        break;
    case ACT_LOAD:
        if (p && dev_in_level() && dev_position_load(DEV_MEM32(A->level_id), v)) {
            dev_teleport(v, 1);
            dev_say("back to the saved position");
        } else {
            dev_say("no saved position for this level");
        }
        break;
    case ACT_FREECAM:
        if (D.freecam) {
            D.freecam = 0;
            dev_say("free camera off");
        } else if (!A->look_at_return) {
            dev_say("free camera: not found in this release");
        } else if (dev_in_level() && dev_freecam_start(p)) {
            D.freecam = 1;
            dev_say("free camera ON");
        }
        break;
    case ACT_NOCLIP:
        if (D.noclip) {
            D.noclip = 0;
            D.god_hold = 300;
            dev_say("noclip off");
        } else if (p && dev_in_level()) {
            D.nc_pos[0] = DEV_MEMF(p + TS2_P_POS);
            D.nc_pos[1] = DEV_MEMF(p + TS2_P_POS + 4);
            D.nc_pos[2] = DEV_MEMF(p + TS2_P_POS + 8);
            D.noclip = 1;
            dev_say("noclip ON");
        }
        break;
    case ACT_FREEZE:
        if (!A->pause) {
            dev_say("stop the clock: not found in this release");
        } else {
            dev_freeze(!D.frozen);
            dev_say(D.frozen ? "clock STOPPED (Page Down: one frame)" : "clock running");
        }
        break;
    case ACT_STEP:
        if (D.frozen)
            D.step = 1;
        break;
    case ACT_FASTER:
    case ACT_SLOWER:
        if (!A->ticks) {
            dev_say("game speed: not found in this release");
            break;
        }
        D.speed += act == ACT_FASTER ? 1 : -1;
        if (D.speed < 0) D.speed = 0;
        if (D.speed >= DEV_SPEED_COUNT) D.speed = DEV_SPEED_COUNT - 1;
        D.speed_carry = 0.0f;
        dev_say("game speed %gx", (double)k_dev_speeds[D.speed]);
        break;
    case ACT_MENU:
        D.menu = !D.menu;
        D.menu_was = 0xFFFF;            /* what opened it is not a press in it */
        break;
    case ACT_WIREFRAME:
        xbox_D3D8SetWireframe(!xbox_D3D8Wireframe());
        dev_say("wireframe %s", xbox_D3D8Wireframe() ? "ON" : "off");
        break;
    case ACT_HUD:
        D.hud_off = !D.hud_off;
        dev_say("HUD %s", D.hud_off ? "hidden" : "shown");
        break;
    case ACT_GIVE:
        dev_give(p);
        break;
    case ACT_ITEM_NEXT:
    case ACT_ITEM_PREV:
        if (A->item_count) {
            int n = (int)A->item_count;

            D.item = (D.item + (act == ACT_ITEM_NEXT ? 1 : n - 1)) % n;
        }
        break;
    case ACT_INVISIBLE:
        D.invisible = !D.invisible;
        dev_say(A->sight_returns[0] ? "invisible to enemies %s" : "invisible: not found",
                D.invisible ? "ON" : "off");
        break;
    }
}

static void dev_keys(uint32_t p)
{
    static const struct { int vk, act; } keys[] = {
        { VK_F1_, ACT_MENU },    { VK_F2_, ACT_GOD },     { VK_F3_, ACT_AMMO },
        { VK_F4_, ACT_INVISIBLE }, { VK_HOME_, ACT_UNLOCK },
        { VK_F5_, ACT_SAVE },    { VK_F6_, ACT_LOAD },
        { VK_F7_, ACT_FREECAM }, { VK_F8_, ACT_NOCLIP },  { VK_F12_, ACT_FREEZE },
        { VK_PAUSE_, ACT_FREEZE }, { VK_NEXT_, ACT_STEP }, { VK_END_, ACT_WIREFRAME },
        { VK_MINUS_, ACT_SLOWER }, { VK_SUBTRACT_, ACT_SLOWER },
        { VK_PLUS_, ACT_FASTER },  { VK_ADD_, ACT_FASTER }, { VK_INSERT_, ACT_HUD },
    };
    size_t i;

    for (i = 0; i < sizeof keys / sizeof keys[0]; i++)
        if (dev_pressed(keys[i].vk))
            dev_action(keys[i].act, p);
}

/* ts2_dev_seq = "<frame>:<action>,...": the keys' actions at given frames,
 * counted from the first present, for scripted runs (which cannot press a
 * key: their window is never in front). */
static struct { unsigned long frame; int act; } g_dev_seq[64];
static int g_dev_seq_n, g_dev_seq_next;
static unsigned long g_dev_frames;

static void dev_seq_parse(const char *s)
{
    while (s && *s && g_dev_seq_n < 64) {
        char name[16];
        unsigned long frame;
        int len = 0, a;

        if (sscanf(s, "%lu:%15[a-z]%n", &frame, name, &len) < 2 || !len)
            break;
        for (a = 0; a < ACT_COUNT; a++)
            if (!strcmp(name, k_dev_actions[a]))
                break;
        if (a < ACT_COUNT) {
            g_dev_seq[g_dev_seq_n].frame = frame;
            g_dev_seq[g_dev_seq_n].act = a;
            g_dev_seq_n++;
        } else {
            fprintf(stderr, "[TS2-DEV] ts2_dev_seq: no action \"%s\"\n", name);
        }
        s += len;
        if (*s == ',')
            s++;
    }
}

static void dev_seq_run(uint32_t p)
{
    while (g_dev_seq_next < g_dev_seq_n && g_dev_seq[g_dev_seq_next].frame <= g_dev_frames) {
        fprintf(stderr, "[TS2-DEV] frame %lu: %s\n", g_dev_frames,
                k_dev_actions[g_dev_seq[g_dev_seq_next].act]);
        dev_action(g_dev_seq[g_dev_seq_next].act, p);
        g_dev_seq_next++;
    }
}

static void dev_configure(void)
{
    const char *v;

    D.configured = 1;
    D.keys = recomp_config_bool("RECOMP_TS2_DEV", "ts2_dev", 1);
    D.speed = DEV_SPEED_NORMAL;
    v = recomp_config_lookup("RECOMP_TS2_SPEED", "ts2_speed");
    if (v && *v) {
        double want = atof(v);
        int i;

        for (i = 0; i < DEV_SPEED_COUNT; i++)
            if (want > 0.0 && (float)want <= k_dev_speeds[i] * 1.01f) {
                D.speed = i;
                break;
            }
    }
    D.god = recomp_config_bool("RECOMP_TS2_GOD", "ts2_god", 0);
    D.ammo = recomp_config_bool("RECOMP_TS2_INFINITE_AMMO", "ts2_infinite_ammo", 0);
    D.unlock = recomp_config_bool("RECOMP_TS2_UNLOCK_ALL", "ts2_unlock_all", 0);
    D.readout = recomp_config_bool("RECOMP_TS2_READOUT", "ts2_readout", 0);
    D.invisible = recomp_config_bool("RECOMP_TS2_INVISIBLE", "ts2_invisible", 0);
    D.hud_off = !recomp_config_bool("RECOMP_TS2_HUD", "ts2_hud", 1);
    v = recomp_config_lookup("RECOMP_TS2_GIVE_ITEM", "ts2_give_item");
    if (v && *v && A->item_count)
        D.item = (int)((unsigned)atoi(v) % A->item_count);
    dev_seq_parse(recomp_config_lookup("RECOMP_TS2_DEV_SEQ", "ts2_dev_seq"));
    v = recomp_config_lookup("RECOMP_TS2_TELEPORT", "ts2_teleport");
    if (v && *v)
        D.start_tp_pending = dev_parse_spot(v, D.start_tp, &D.start_tp_angles);
    fprintf(stderr, "[TS2-DEV] menu and keys %s (F1 or L3+R3: the menu), god %d, "
            "infinite ammo %d, unlock all %d, invisible %d, readout %d, HUD %d, speed %gx%s "
            "(src/ts2_devtools.c, docs/devtools.md)\n",
            D.keys ? "on" : "off (ts2_dev = 0)", D.god, D.ammo, D.unlock, D.invisible,
            D.readout, !D.hud_off, (double)k_dev_speeds[D.speed],
            D.start_tp_pending ? ", teleport at the first level" : "");
}

/* What the middle of the view is on: the game's own ray, from just ahead of
 * the eye (the free camera's when it is on) 1000 units along the view.
 * Writes a line, or nothing without a ray. */
static void dev_aim(char *out, size_t n)
{
    uint32_t ctx = dev_context(), pts, args[10], obj, cls, model, name, self, flags;
    float eye[3], dir[3], hit[3];
    char model_name[40];
    int i, room;

    out[0] = 0;
    if (!A->ray || !ctx || !dev_in_level())
        return;
    if (D.freecam) {
        memcpy(eye, D.fc_eye, sizeof eye);
        dev_freecam_forward(dir);
        room = D.fc_room;
    } else {
        for (i = 0; i < 3; i++) {
            eye[i] = DEV_MEMF(ctx + TS2_CTX_EYE + 4u * (uint32_t)i);
            dir[i] = DEV_MEMF(ctx + TS2_CTX_FORWARD + 4u * (uint32_t)i);
        }
        room = (int)DEV_MEM32(ctx + TS2_CTX_ROOM);
    }
    dev_normalize(dir);
    /* The ray's object test skips an object whose flags (+0x1C) have bit 31
     * set (sub_00154DF0). In first person the eye is inside the player's
     * own character, and every cast from it, or from past its radius, hit
     * that: it is marked skipped for this one call. Not under the free
     * camera, which may look at the player. */
    self = D.freecam ? 0 : dev_chr(dev_player());
    pts = dev_scratch(0x150);
    for (i = 0; i < 3; i++)
        dir[i] *= 1000.0f;
    dev_put3(pts, eye);
    dev_put3(pts + 0x10, dir);
    hit[0] = hit[1] = hit[2] = 0.0f;
    dev_put3(pts + 0x20, hit);
    DEV_MEM32(pts + 0x40) = (uint32_t)-1;
    args[0] = (uint32_t)room;
    args[1] = pts;                      /* start */
    args[2] = pts + 0x10;               /* delta */
    args[3] = 0;                        /* the segment, not a line */
    args[4] = pts + 0x20;               /* hit */
    args[5] = pts + 0x30;               /* normal */
    args[6] = pts + 0x40;               /* room it ended in */
    args[7] = 0;                        /* every class */
    args[8] = 0;                        /* the nearest hit */
    /* What the ray passed through (a fence) goes into a list: a count and
     * up to five records of 0x30. Zeroed scratch, never an object. */
    for (i = 0; i < 0x100; i += 4)
        DEV_MEM32(pts + 0x50 + (uint32_t)i) = 0;
    args[9] = pts + 0x50;
    flags = self ? DEV_MEM32(self + TS2_O_FLAGS) : 0;
    if (self)
        DEV_MEM32(self + TS2_O_FLAGS) = flags | 0x80000000u;
    obj = dev_guest_call(A->ray, pts, args, 10);
    if (self)
        DEV_MEM32(self + TS2_O_FLAGS) = flags;
    for (i = 0; i < 3; i++)
        hit[i] = DEV_MEMF(pts + 0x20 + 4u * (uint32_t)i);
    if (!dev_ptr_ok(obj)) {
        snprintf(out, n, "aim: nothing within 1000");
        return;
    }
    cls = DEV_MEM32(obj + TS2_O_CLASS);
    model = DEV_MEM32(obj + TS2_O_MODEL);
    model_name[0] = 0;
    if (A->object_list && model < 2048) {
        name = DEV_MEM32(A->object_list + model * 0x24u);
        for (i = 0; dev_ptr_ok(name) && i < (int)sizeof model_name - 1; i++) {
            char ch = (char)DEV_MEM8(name + (uint32_t)i);

            if (ch < 32 || ch > 126)
                break;
            model_name[i] = ch;
            model_name[i + 1] = 0;
        }
    }
    snprintf(out, n, "aim %.2f %.2f %.2f  %.1f away  %s %s",
             hit[0], hit[1], hit[2],
             sqrtf((hit[0] - eye[0]) * (hit[0] - eye[0]) + (hit[1] - eye[1]) * (hit[1] - eye[1]) +
                   (hit[2] - eye[2]) * (hit[2] - eye[2])),
             cls == 2 ? "room" : cls == 4 ? "door" : cls == 8 ? "character" :
             cls == 0x40 ? "pickup" : "object",
             cls == 2 ? "" : model_name);
    if (cls != 2 && cls != 4 && cls != 8 && cls != 0x40) {
        size_t k = strlen(out);
        snprintf(out + k, n - k, " (class 0x%X)", (unsigned)cls);
    }
}

/* ── the menu ──────────────────────────────────────────────── */

/* Its rows: what each says, and what A does on it (left/right too, for a
 * row with a value). */
static const struct { const char *label; int act, left, right; } k_dev_menu[] = {
    { "Readout",                  ACT_READOUT,   -1, -1 },
    { "God mode",                 ACT_GOD,       -1, -1 },
    { "Infinite ammo",            ACT_AMMO,      -1, -1 },
    { "Invisible to enemies",     ACT_INVISIBLE, -1, -1 },
    { "Unlock everything",        ACT_UNLOCK,    -1, -1 },
    { "Game speed",               ACT_FASTER,    ACT_SLOWER, ACT_FASTER },
    { "Stop the clock",           ACT_FREEZE,    -1, -1 },
    { "One frame forward",        ACT_STEP,      -1, -1 },
    { "Free camera",              ACT_FREECAM,   -1, -1 },
    { "Noclip",                   ACT_NOCLIP,    -1, -1 },
    { "Save position",            ACT_SAVE,      -1, -1 },
    { "Back to saved position",   ACT_LOAD,      -1, -1 },
    { "Wireframe",                ACT_WIREFRAME, -1, -1 },
    { "HUD",                      ACT_HUD,       -1, -1 },
    { "Give item",                ACT_GIVE,      ACT_ITEM_PREV, ACT_ITEM_NEXT },
};
#define DEV_MENU_ROWS ((int)(sizeof k_dev_menu / sizeof k_dev_menu[0]))

static const char *dev_menu_value(int row, char *buf, size_t n)
{
    int on = -1;

    switch (k_dev_menu[row].act) {
    case ACT_READOUT:   on = D.readout; break;
    case ACT_GOD:       on = D.god; break;
    case ACT_AMMO:      on = D.ammo; break;
    case ACT_INVISIBLE: on = D.invisible; break;
    case ACT_UNLOCK:    on = D.unlock; break;
    case ACT_FREEZE:    on = D.frozen; break;
    case ACT_FREECAM:   on = D.freecam; break;
    case ACT_NOCLIP:    on = D.noclip; break;
    case ACT_WIREFRAME: on = xbox_D3D8Wireframe(); break;
    case ACT_HUD:       on = !D.hud_off; break;
    case ACT_GIVE: {
        char name[32];
        uint32_t r = dev_record(dev_player());
        int in = r && dev_in_level();

        snprintf(buf, n, "< %s >%s", dev_item_name(D.item, name, sizeof name),
                 !in ? "" : !dev_item_givable(D.item) ? " (not here)" :
                 DEV_MEM32(r + TS2_R_OWNED + 4u * (uint32_t)D.item) ? " (have)" : "");
        return buf;
    }
    case ACT_FASTER:
        snprintf(buf, n, "< %gx >", (double)k_dev_speeds[D.speed]);
        return buf;
    default:
        return "";
    }
    return on ? "ON" : "off";
}

/* The pad (the keyboard's arrows, Enter and Esc too) while the menu is
 * open: a press acts once, and a held direction repeats after a moment. */
static void dev_menu_input(uint32_t p)
{
    uint16_t buttons = 0, now, pressed;
    uint8_t analog[8] = { 0 };
    int16_t sticks[4] = { 0 };
    int row = D.menu_row;

    xbox_InputRead(0, &buttons, analog, sticks);
    now = buttons & (PADB_UP | PADB_DOWN | PADB_LEFT | PADB_RIGHT | PADB_START | PADB_BACK);
    if (analog[PAD_A] > 64 || xbox_D3D8KeyDown(VK_RETURN_)) now |= 0x100;
    if (analog[PAD_B] > 64 || xbox_D3D8KeyDown(VK_ESCAPE_)) now |= 0x200;
    if (xbox_D3D8KeyDown(VK_UP_))    now |= PADB_UP;
    if (xbox_D3D8KeyDown(VK_DOWN_))  now |= PADB_DOWN;
    if (xbox_D3D8KeyDown(VK_LEFT_))  now |= PADB_LEFT;
    if (xbox_D3D8KeyDown(VK_RIGHT_)) now |= PADB_RIGHT;
    if (sticks[1] > 16000)  now |= PADB_UP;
    if (sticks[1] < -16000) now |= PADB_DOWN;

    pressed = (uint16_t)(now & ~D.menu_was);
    /* a held direction: again after 18 frames, then every 6 */
    if (now & (PADB_UP | PADB_DOWN | PADB_LEFT | PADB_RIGHT) & D.menu_was) {
        if (++D.menu_hold_frames >= 18 && (D.menu_hold_frames - 18) % 6 == 0)
            pressed |= now & (PADB_UP | PADB_DOWN | PADB_LEFT | PADB_RIGHT);
    } else {
        D.menu_hold_frames = 0;
    }
    D.menu_was = now;

    if (pressed & PADB_UP)
        row = (row + DEV_MENU_ROWS - 1) % DEV_MENU_ROWS;
    if (pressed & PADB_DOWN)
        row = (row + 1) % DEV_MENU_ROWS;
    D.menu_row = row;
    if ((pressed & PADB_LEFT) && k_dev_menu[row].left >= 0)
        dev_action(k_dev_menu[row].left, p);
    if ((pressed & PADB_RIGHT) && k_dev_menu[row].right >= 0)
        dev_action(k_dev_menu[row].right, p);
    if (pressed & (0x100 | PADB_START)) {
        int act = k_dev_menu[row].act;

        dev_action(act, p);
        /* The free camera and noclip are driven by the pad: close the menu
         * so it reaches them. */
        if ((act == ACT_FREECAM && D.freecam) || (act == ACT_NOCLIP && D.noclip))
            dev_action(ACT_MENU, p);
    }
    if (pressed & (0x200 | PADB_BACK))
        dev_action(ACT_MENU, p);
}

static size_t dev_menu_text(char *out, size_t n, int footer)
{
    size_t k;
    int i;
    char buf[64];

    k = (size_t)snprintf(out, n, "TS2 DEV MENU\n");
    for (i = 0; i < DEV_MENU_ROWS && k < n; i++)
        k += (size_t)snprintf(out + k, n - k, "%s %-24s %s\n", i == D.menu_row ? ">" : " ",
                              k_dev_menu[i].label, dev_menu_value(i, buf, sizeof buf));
    if (k < n && footer)
        k += (size_t)snprintf(out + k, n - k, "A/Enter set  left/right change  B/Esc close");
    else if (k > 0 && k <= n && out[k - 1] == '\n')
        out[--k] = 0;                   /* the message takes the footer's line */
    return k < n ? k : n - 1;
}

/* The overlay's line: the readout, then the last thing a key did. */
static void dev_show(uint32_t p)
{
    char line[1200];
    size_t n = 0;

    line[0] = 0;
    if (D.menu) {
        n = dev_menu_text(line, sizeof line, D.msg_frames <= 0);
    } else if (D.readout && p && dev_in_level()) {
        uint32_t c = dev_chr(p);
        const float *at = D.freecam ? D.fc_eye : NULL;
        float pos[3];
        int i;

        for (i = 0; i < 3; i++)
            pos[i] = at ? at[i] : DEV_MEMF(p + TS2_P_POS + 4u * (uint32_t)i);
        n = (size_t)snprintf(line, sizeof line, "%s %.2f %.2f %.2f  yaw %.0f pitch %.0f  "
                             "room %d  hp %.0f\n", at ? "cam" : "pos", pos[0], pos[1], pos[2],
                             DEV_MEMF(p + TS2_P_YAW), DEV_MEMF(p + TS2_P_PITCH),
                             at ? D.fc_room : (int)DEV_MEM32(p + TS2_P_ROOM),
                             c ? DEV_MEMF(c + TS2_C_HEALTH) : 0.0f);
        if (n < sizeof line)
            dev_aim(line + n, sizeof line - n);
        n = strlen(line);
        if (n < sizeof line)
            n += (size_t)snprintf(line + n, sizeof line - n, "%s%s%s%s%s%s%s",
                                  D.god || D.noclip ? "\nGOD " : "\n",
                                  D.ammo ? "AMMO " : "", D.invisible ? "INVISIBLE " : "",
                                  D.noclip ? "NOCLIP " : "", D.freecam ? "FREECAM " : "",
                                  D.hud_off ? "NOHUD " : "", D.frozen ? "STOPPED" : "");
    } else if (D.readout) {
        n = (size_t)snprintf(line, sizeof line, "level %u",
                             A->level_id ? (unsigned)DEV_MEM32(A->level_id) : 0u);
    }
    if (n >= sizeof line)
        n = sizeof line - 1;
    if (D.msg_frames > 0) {
        D.msg_frames--;
        snprintf(line + n, sizeof line - n, "%s%s", n ? "\n" : "", D.msg);
    }
    if (line[0])
        xbox_D3D8Notice(line, 250);
    /* The readout in the log too, once a second, for scripted runs. */
    if (D.readout && !D.menu && line[0] && g_dev_frames % 60 == 0) {
        char *nl;

        while ((nl = strchr(line, '\n')) != NULL)
            *nl = '|';
        fprintf(stderr, "[TS2-DEV] frame %lu: %s\n", g_dev_frames, line);
    }
}

/* The game reads a neutral pad while the menu or the free camera has it,
 * and after the menu closes until the controls that closed it are let go:
 * a B, Start or Esc still held would reach the game (Esc is Start in a
 * level with keyboard bindings, which opens the pause menu). */
static void dev_withhold(void)
{
    static int held = -1;
    int want = D.menu || D.freecam;

    if (!want && held == 1) {
        uint16_t buttons = 0;
        uint8_t analog[8] = { 0 };

        xbox_InputRead(0, &buttons, analog, NULL);
        want = (buttons & (PADB_START | PADB_BACK | PADB_LTHUMB | PADB_RTHUMB)) ||
               analog[PAD_A] > 64 || analog[PAD_B] > 64 ||
               xbox_D3D8KeyDown(VK_RETURN_) || xbox_D3D8KeyDown(VK_ESCAPE_);
    }
    if (want != held) {
        xbox_InputWithhold(0, want);
        ts2_mouse_hold(want);           /* nor does the mouse turn the player */
        held = want;
    }
}

void ts2_dev_frame(void)
{
    uint32_t p;

    if (!D.configured)
        dev_configure();
    p = dev_player();
    g_dev_frames++;
    if (D.keys) {
        uint16_t buttons = 0;
        static int thumbs_were;
        int thumbs;

        /* L3+R3 opens and closes the menu from the pad. */
        xbox_InputRead(0, &buttons, NULL, NULL);
        thumbs = (buttons & (PADB_LTHUMB | PADB_RTHUMB)) == (PADB_LTHUMB | PADB_RTHUMB);
        if (thumbs && !thumbs_were)
            dev_action(ACT_MENU, p);
        thumbs_were = thumbs;
        dev_keys(p);
        if (D.menu)
            dev_menu_input(p);
    }
    dev_seq_run(p);

    /* A level gone (the front end, a load): its tools go with it. */
    if (!dev_in_level() || !p) {
        D.freecam = 0;
        D.noclip = 0;
        D.tp_sweeps = 0;
    } else if (D.start_tp_pending) {
        D.start_tp_pending = 0;
        dev_teleport(D.start_tp, D.start_tp_angles);
        dev_say("teleported (ts2_teleport)");
    }

    dev_write_switches();
    dev_write_clock();
    if (D.freecam && !D.menu)
        dev_freecam_step();
    if (D.noclip && p && !D.menu)
        dev_noclip_step(p);
    dev_withhold();
    dev_show(p);
}
