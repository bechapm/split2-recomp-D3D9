/*
 * ts2_devtools.c -- TimeSplitters 2's debugging tools.
 *
 * For setting up a test scene and holding it still: the developers' own
 * god mode and infinite ammo, every unlock, the game's clock stopped (the
 * world holds still and is still drawn), a readout of where the player is,
 * a saved position to come back to, a free camera and noclip. All of it is
 * TS2's: the toolkit only gives keys, a line on its overlay
 * (xbox_D3D8KeyDown, xbox_D3D8Notice) and the pad (xbox_InputWithhold,
 * xbox_InputRead).
 *
 * Off unless asked for. The switches (each a setting in the title's .conf
 * or an environment variable, read once at start-up):
 *
 *   ts2_dev = 1            RECOMP_TS2_DEV          the keys below
 *   ts2_god = 1            RECOMP_TS2_GOD          god mode from the start
 *   ts2_infinite_ammo = 1  RECOMP_TS2_INFINITE_AMMO
 *   ts2_unlock_all = 1     RECOMP_TS2_UNLOCK_ALL   every character, level and cheat
 *   ts2_readout = 1        RECOMP_TS2_READOUT      the position line from the start
 *   ts2_teleport = x,y,z[,yaw,pitch]  RECOMP_TS2_TELEPORT
 *                          where the player is put once the first level is
 *                          playing: a repeatable spot for a scripted run
 *
 * The keys (with ts2_dev, only while the game's window is in front):
 *
 *   F1  readout on/off     F2  god mode        F3  infinite ammo
 *   F4  unlock everything  F5  save position   F6  back to the saved position
 *   F7  free camera        F8  noclip          F12 or Pause: stop the clock
 *   Page Down: one frame forward while the clock is stopped
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
 *   teleport, noclip: the character sweep, the one function every one of
 *     the player's movement modes moves through, is given the tool's
 *     position as its answer, so the game makes the player's eye, its
 *     character and everything else from it as from any step. The room the
 *     player is in is found by the game's own portal walk, so drawing
 *     starts from the right room. Noclip keeps god mode on, and for five
 *     seconds after, so landing from a height is not a death.
 *   free camera: the eye and direction the camera update hands its
 *     look-at, replaced at that one call, and the room drawing starts from
 *     found by the portal walk, so the view, everything made from it and
 *     the camera RTX Remix is given all follow, and rooms are drawn from
 *     where the camera is. The pad (and the keyboard) drive the camera while
 *     it is on, and the game reads a neutral pad.
 */
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ts2_devtools.h"
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
extern void        xbox_InputWithhold(unsigned port, int on);
extern int         xbox_InputRead(unsigned port, uint16_t *buttons, uint8_t analog[8],
                                  int16_t thumbs[4]);

#define DEV_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define DEV_MEMF(a)  (*(volatile float *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define DEV_MEM8(a)  (*(volatile uint8_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

/* Windows virtual-key codes (the toolkit translates them off Windows). */
enum {
    VK_F1_ = 0x70, VK_F2_, VK_F3_, VK_F4_, VK_F5_, VK_F6_, VK_F7_, VK_F8_,
    VK_F12_ = 0x7B, VK_PAUSE_ = 0x13, VK_NEXT_ = 0x22, VK_SHIFT_ = 0x10,
    VK_SPACE_ = 0x20, VK_CONTROL_ = 0x11,
    VK_LEFT_ = 0x25, VK_UP_, VK_RIGHT_, VK_DOWN_,
};

/* XINPUT wButtons, and the analog buttons' order (A B X Y black white L R). */
enum { PAD_A = 0, PAD_B, PAD_X, PAD_Y, PAD_BLACK, PAD_WHITE, PAD_LT, PAD_RT };

#define TS2_FRAME_SECONDS (1.0f / 60.0f)   /* the game's logic rate */

static const Ts2DevAddrs *A = &k_ts2_dev;

static struct {
    int  configured, keys, readout;
    int  god, ammo, unlock;            /* what the player asked for */
    int  god_written, ammo_written, unlock_written;
    int  frozen, step, freecam, noclip;
    int  god_hold;                      /* frames god stays on after noclip */
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

static int dev_in_level(void)
{
    return A->level_id && DEV_MEM32(A->level_id) != A->level_front_end;
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

/* room_walk(from, from_room, to, 0), called on the guest stack below the
 * caller's: the two points go just under it, the arguments under those.
 * The room holding `to`, or -1. The registers a caller of the hook may
 * still want (the hooked function's result) are kept. */
static int dev_room_walk(const float from[3], int from_room, const float to[3])
{
    uint32_t top = g_esp, pts = top - 0x40, eax = g_eax, ecx = g_ecx, edx = g_edx;
    int i, room;

    if (!A->room_walk || from_room < 0)
        return -1;
    for (i = 0; i < 3; i++) {
        DEV_MEMF(pts + 4u * (uint32_t)i) = from[i];
        DEV_MEMF(pts + 0x10u + 4u * (uint32_t)i) = to[i];
    }
    g_esp = pts;
    g_esp -= 4; DEV_MEM32(g_esp) = 0;
    g_esp -= 4; DEV_MEM32(g_esp) = pts + 0x10u;
    g_esp -= 4; DEV_MEM32(g_esp) = (uint32_t)from_room;
    g_esp -= 4; DEV_MEM32(g_esp) = pts;
    g_esp -= 4; DEV_MEM32(g_esp) = 0;   /* return address: none */
    A->room_walk();
    room = (int)g_eax;
    g_esp = top;
    g_eax = eax;
    g_ecx = ecx;
    g_edx = edx;
    return room >= 0 && room < 0x10000 ? room : -1;
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
}

/* Start from the camera as the game left it: its eye, and its right and
 * forward rows, kept as a level forward, a level right and a pitch, and
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
        f[i] = DEV_MEMF(w + 0x20u + 4u * (uint32_t)i);
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
    room = dev_room_walk(a, (int)DEV_MEM32(p + TS2_P_ROOM), b);
    if (room >= 0)
        DEV_MEM32(p + TS2_P_ROOM) = (uint32_t)room;
    if (D.tp_sweeps > 0) {
        D.tp_sweeps--;
        if (D.tp_has_angles) {
            DEV_MEMF(p + TS2_P_YAW) = D.tp[3];
            DEV_MEMF(p + TS2_P_PITCH) = D.tp[4];
        }
    }
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
        room = dev_room_walk(D.fc_room_eye, D.fc_room, D.fc_eye);
        if (room >= 0) {
            D.fc_room = room;
            memcpy(D.fc_room_eye, D.fc_eye, sizeof D.fc_eye);
        }
        DEV_MEM32(ctx + TS2_CTX_ROOM) = (uint32_t)D.fc_room;
    }
    look_at();
}

/* ── once a frame ──────────────────────────────────────────── */

static void dev_configure(void)
{
    const char *v;

    D.configured = 1;
    D.keys = recomp_config_bool("RECOMP_TS2_DEV", "ts2_dev", 0);
    D.god = recomp_config_bool("RECOMP_TS2_GOD", "ts2_god", 0);
    D.ammo = recomp_config_bool("RECOMP_TS2_INFINITE_AMMO", "ts2_infinite_ammo", 0);
    D.unlock = recomp_config_bool("RECOMP_TS2_UNLOCK_ALL", "ts2_unlock_all", 0);
    D.readout = recomp_config_bool("RECOMP_TS2_READOUT", "ts2_readout", 0);
    v = recomp_config_lookup("RECOMP_TS2_TELEPORT", "ts2_teleport");
    if (v && *v)
        D.start_tp_pending = dev_parse_spot(v, D.start_tp, &D.start_tp_angles);
    if (D.keys || D.god || D.ammo || D.unlock || D.readout || D.start_tp_pending)
        fprintf(stderr, "[TS2-DEV] keys %s, god %d, infinite ammo %d, unlock all %d, "
                "readout %d%s (src/ts2_devtools.c)\n",
                D.keys ? "on (F1-F8, F12)" : "off", D.god, D.ammo, D.unlock, D.readout,
                D.start_tp_pending ? ", teleport at the first level" : "");
}

static void dev_keys(uint32_t p)
{
    float v[5];

    if (dev_pressed(VK_F1_)) {
        D.readout = !D.readout;
        dev_say("readout %s", D.readout ? "on" : "off");
    }
    if (dev_pressed(VK_F2_)) {
        D.god = !D.god;
        dev_say(A->god ? "god mode %s" : "god mode: not found in this release",
                D.god ? "ON" : "off");
    }
    if (dev_pressed(VK_F3_)) {
        D.ammo = !D.ammo;
        dev_say(A->infinite_ammo ? "infinite ammo %s" : "infinite ammo: not found",
                D.ammo ? "ON" : "off");
    }
    if (dev_pressed(VK_F4_)) {
        D.unlock = !D.unlock;
        dev_say(A->unlock_all ? "unlock everything %s" : "unlock everything: not found",
                D.unlock ? "ON" : "off");
    }
    if (dev_pressed(VK_F5_)) {
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
    }
    if (dev_pressed(VK_F6_)) {
        if (p && dev_in_level() && dev_position_load(DEV_MEM32(A->level_id), v)) {
            dev_teleport(v, 1);
            dev_say("back to the saved position");
        } else {
            dev_say("no saved position for this level");
        }
    }
    if (dev_pressed(VK_F7_)) {
        if (D.freecam) {
            D.freecam = 0;
            xbox_InputWithhold(0, 0);
            dev_say("free camera off");
        } else if (!A->look_at_return) {
            dev_say("free camera: not found in this release");
        } else if (dev_in_level() && dev_freecam_start(p)) {
            D.freecam = 1;
            xbox_InputWithhold(0, 1);
            dev_say("free camera ON");
        }
    }
    if (dev_pressed(VK_F8_)) {
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
    }
    if (dev_pressed(VK_F12_) | dev_pressed(VK_PAUSE_)) {
        if (!A->pause)
            dev_say("stop the clock: not found in this release");
        else {
            dev_freeze(!D.frozen);
            dev_say(D.frozen ? "clock STOPPED (Page Down: one frame)" : "clock running");
        }
    }
    if (dev_pressed(VK_NEXT_) && D.frozen)
        D.step = 1;
}

/* The overlay's line: the readout, then the last thing a key did. */
static void dev_show(uint32_t p)
{
    char line[96];
    size_t n = 0;

    line[0] = 0;
    if (D.readout && p && dev_in_level()) {
        uint32_t c = dev_chr(p);
        const float *at = D.freecam ? D.fc_eye : NULL;
        float pos[3];
        int i;

        for (i = 0; i < 3; i++)
            pos[i] = at ? at[i] : DEV_MEMF(p + TS2_P_POS + 4u * (uint32_t)i);
        n = (size_t)snprintf(line, sizeof line, "%s %.2f %.2f %.2f  yaw %.0f pitch %.0f  "
                             "room %d  hp %.0f", at ? "cam" : "pos", pos[0], pos[1], pos[2],
                             DEV_MEMF(p + TS2_P_YAW), DEV_MEMF(p + TS2_P_PITCH),
                             at ? D.fc_room : (int)DEV_MEM32(p + TS2_P_ROOM),
                             c ? DEV_MEMF(c + TS2_C_HEALTH) : 0.0f);
    } else if (D.readout) {
        n = (size_t)snprintf(line, sizeof line, "level %u",
                             A->level_id ? (unsigned)DEV_MEM32(A->level_id) : 0u);
    }
    if (n >= sizeof line)
        n = sizeof line - 1;
    if (D.msg_frames > 0) {
        D.msg_frames--;
        snprintf(line + n, sizeof line - n, "%s%s", n ? "  |  " : "", D.msg);
    }
    if (line[0])
        xbox_D3D8Notice(line, 250);
}

void ts2_dev_frame(void)
{
    uint32_t p;

    if (!D.configured)
        dev_configure();
    p = dev_player();
    if (D.keys)
        dev_keys(p);

    /* A level gone (the front end, a load): its tools go with it. */
    if (!dev_in_level() || !p) {
        if (D.freecam) {
            D.freecam = 0;
            xbox_InputWithhold(0, 0);
        }
        D.noclip = 0;
        D.tp_sweeps = 0;
    } else if (D.start_tp_pending) {
        D.start_tp_pending = 0;
        dev_teleport(D.start_tp, D.start_tp_angles);
        dev_say("teleported (ts2_teleport)");
    }

    dev_write_switches();
    dev_write_clock();
    if (D.freecam)
        dev_freecam_step();
    if (D.noclip && p)
        dev_noclip_step(p);
    dev_show(p);
}
