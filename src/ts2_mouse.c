/*
 * ts2_mouse.c -- TimeSplitters 2 on a mouse and keyboard.
 *
 * Three things, each a fact about this game rather than the toolkit's:
 *
 *   mouse aim: the toolkit's mouse look moves the right stick, and a stick
 *     is a speed. TS2 turns the player by the stick in one function, look
 *     (sub_00148A60 in the USA release), which adds rate * stick to the
 *     view angles in the player record, then clamps the pitch. Here look
 *     runs as it always did, for the pad, and afterwards the mouse's counts
 *     since last time are added to the same two angles directly: so many
 *     degrees a count, whatever the speed, which is what mouse aim is on a
 *     PC. Only for player 1, only when look itself turned the player (not
 *     on a mounted gun, not in a menu, where look is not called or turns
 *     nothing), only when the camera is the player's own eye (look runs
 *     through a level's opening fly-over too), and scaled by the field of
 *     view so a scope zoomed four times turns four times finer. Not
 *     inverted unless ts2_mouse_invert says so: the game's own Inverse
 *     Look is on in a new profile, and a PC shooter's mouse is not.
 *
 *   auto lookahead: with that preference on, look tilts the view back to
 *     level while you run with nothing on the look stick. With a mouse the
 *     look stick is always still, so it would pull against every aim; while
 *     mouse look is on, look's pitch change with a still stick is undone.
 *
 *   steady aim: the camera sways with the player's breathing, all the
 *     time, more after running (the breathing function, sub_00140E10 in
 *     the USA release, sets the two angles the camera adds to the view).
 *     A mouse holds the view still, so the sway is all that moves it, and
 *     it moves the crosshair off what you are aiming at. While player 1
 *     aims (right mouse, or the pad's R3), there is none, and nothing moves
 *     when it stops or starts: the sway of that moment is folded into the
 *     view angles as aiming begins, so the camera stays exactly where it
 *     was, and the sway coming back is taken out of them as aiming ends,
 *     so it picks up from where you are. ts2_steady_aim = 0 keeps it.
 *
 *   one key, two meanings (the toolkit's bindings filter): Escape is bound
 *     to Start, which pauses in a level, but in the front end Start
 *     accepts, so there Escape reads as B (back). And the mouse wheel,
 *     bound to the D-pad's left and right (previous and next weapon), is
 *     its up and down (zoom in and out) while zoomed in through a scope, as
 *     the game's Default controls have the D-pad. Aiming with no scope (or
 *     an item with no crosshair) leaves the wheel changing weapon.
 *
 *   changing weapon ends aim mode, from the wheel, the keys or the pad, so
 *     the next gun does not come up already aimed (straight into a scope).
 *
 * The keys themselves are config/input_defaults.json, the title's own layout
 * (recomp_title_input in CMakeLists.txt).
 *
 * A scripted run's window has no mouse, so RECOMP_TS2_MOUSE_SEQ moves it:
 * "FRAME:DX:DY,..." adds DX, DY counts at those presents (counted from the
 * first), as the window would; it needs mouse look on (RECOMP_MOUSE_LOOK=1).
 * RECOMP_TS2_MOUSE_LOG=1 prints what look did with it.
 *
 * "FRAME:key:NAME:N" holds a button for N presents as the keyboard would
 * send it (dpad_left, dpad_right, dpad_up, dpad_down, rthumb, start).
 *
 * Settings (the title's settings file, or the environment variable):
 *   ts2_mouse_invert = 1    RECOMP_TS2_MOUSE_INVERT   mouse up looks down
 *   ts2_steady_aim = 0      RECOMP_TS2_STEADY_AIM     sway while aiming too
 */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ts2_mouse.h"
#include "recomp/gen/recomp_cpu.h"   /* g_esp */
#include "recomp/ts2_region.h"
#include "input_bindings.h"          /* xboxrecomp: the mouse and the filter */
#include "recomp_config.h"           /* xboxrecomp: RECOMP_TS2_MOUSE_LOG */

/* Where this release keeps what the mouse uses: each release's own table. */
#if defined(TS2_REGION_US)
#include "../config/us/mouse.inc"
#else
/* Not found in the PAL release yet: the keys work, the mouse turns nothing. */
static const Ts2MouseAddrs k_ts2_mouse = { 0 };
#endif

static const Ts2MouseAddrs *const A = &k_ts2_mouse;

extern ptrdiff_t g_xbox_mem_offset;

#define M_MEM32(a) (*(volatile uint32_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define M_MEMF(a)  (*(volatile float *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))
#define M_MEM8(a)  (*(volatile uint8_t *)((uintptr_t)(uint32_t)(a) + g_xbox_mem_offset))

/* Degrees a mouse count turns at the default sensitivity (80): 0.044, the
 * same as the Source engine's 0.022 at sensitivity 2, so a turn right
 * round is 8,200 counts, ten inches on an 800 dpi mouse. Each 20 steps of
 * the setting doubles or halves it (0.0028 at 1, 0.088 at 100). */
#define TS2_MOUSE_DEGREES   0.044
#define TS2_MOUSE_PER_STEP  20.0
/* The field of view the turn rate is measured against: look multiplies by
 * fov / 60, and so does the mouse. */
#define TS2_MOUSE_FOV       60.0f

static struct {
    unsigned frame;         /* presents seen */
    unsigned took;          /* the frame motion was last taken in */
    int      started;
    int      log;
    unsigned logged;
    const char *seq;        /* RECOMP_TS2_MOUSE_SEQ, what is left of it */
    volatile int held;      /* ts2_mouse_hold */
    int      invert;        /* ts2_mouse_invert */
    int      steady;        /* ts2_steady_aim */
    uint32_t still;         /* the record whose sway is held at 0 (aiming) */
    volatile unsigned unaim;    /* a weapon change asked for aimed: the present, or 0 */
    volatile WORD key_buttons;  /* RECOMP_TS2_MOUSE_SEQ's keys, as the keyboard */
    volatile unsigned key_until;
} M;

void ts2_mouse_hold(int on)
{
    M.held = on;
}

/* A guest pointer worth following: low memory, or the contiguous window at
 * 0x80000000 where the game's heap objects (the player records, the camera
 * context) are. */
static int m_ptr_ok(uint32_t a)
{
    return (a >= 0x10000u && a < 0x08000000u) || (a >= 0x80000000u && a < 0x88000000u);
}

static int m_in_level(void)
{
    return A->level_id && M_MEM32(A->level_id) < A->level_menus;
}

static uint32_t m_player1(void)
{
    uint32_t p = A->player_base ? M_MEM32(A->player_base) : 0;

    return m_ptr_ok(p) ? p : 0;
}

/* The camera is player 1's eye: not a cutscene's or a fly-over's camera
 * (nor the debugging tools' free camera). The camera was made last frame,
 * so it trails the eye by a frame's step. */
static int m_camera_is_eye(uint32_t p)
{
    uint32_t ctx = A->context ? M_MEM32(A->context) : 0;
    float d = 0.0f, e;
    int i;

    if (!m_ptr_ok(ctx))
        return 1;                       /* nothing to tell by */
    for (i = 0; i < 3; i++) {
        e = M_MEMF(ctx + TS2_MOUSE_CTX_EYE + 4u * (uint32_t)i) -
            M_MEMF(p + TS2_PM_EYE + 4u * (uint32_t)i);
        d += e * e;
    }
    return d < 1.0f;
}

/* ── the bindings filter ─────────────────────────────────────── */

/* Player 1 is looking through a scope: the flags gain bit 2 (0x8 to 0xE
 * aiming the sniper rifle; 0xA aiming with no scope), the bit that also
 * stops the breathing sway in the game's own breathing function. The field
 * of view cannot tell: the game narrows it for some guns with no scope
 * too (the S47 and laser gun to 40, the SBP90 to 30, measured in the PC
 * release), and a scope's widest step is 30 as well. */
static int m_scoped(uint32_t p)
{
    return M_MEM32(p + TS2_PM_AIMING) && (M_MEM32(p + TS2_PM_FLAGS) & TS2_PF_SCOPE);
}

static void m_filter(unsigned port, XBOX_GAMEPAD *pad, const XBOX_GAMEPAD *from_keyboard)
{
    XBOX_GAMEPAD keys = *from_keyboard;
    const XBOX_GAMEPAD *kbm = &keys;
    uint32_t p;

    if (port != 0)
        return;
    if (M.key_buttons && M.frame < M.key_until) {
        keys.wButtons |= M.key_buttons;     /* RECOMP_TS2_MOUSE_SEQ */
        pad->wButtons |= M.key_buttons;
    }
    if (!m_in_level()) {
        /* The front end: Escape (Start, from the keyboard) backs out. */
        if (A->level_id && (kbm->wButtons & XBOX_GAMEPAD_START)) {
            pad->wButtons &= (WORD)~XBOX_GAMEPAD_START;
            pad->bAnalogButtons[XBOX_BUTTON_B] = 255;
        }
        return;
    }
    /* Changing weapon aimed: noted here, where the request is seen, and
     * acted on in the game's own update (m_unaim_tick). Not when the wheel
     * or keys are zooming a scope. */
    p = m_player1();
    if (p && !M.unaim && M_MEM32(p + TS2_PM_AIMING) &&
        (pad->wButtons & (XBOX_GAMEPAD_DPAD_LEFT | XBOX_GAMEPAD_DPAD_RIGHT)) &&
        !(m_scoped(p) && (kbm->wButtons & (XBOX_GAMEPAD_DPAD_LEFT | XBOX_GAMEPAD_DPAD_RIGHT))))
        M.unaim = M.frame ? M.frame : 1;

    /* Looking through a scope: the wheel (the D-pad's left and right, from
     * the mouse or the keyboard) zooms, as the D-pad's up and down do.
     * Only then: aim mode with a gun that has no scope, or an item with no
     * crosshair at all (the temporal uplink), zooms nothing, and the wheel
     * would stop changing weapon with nothing on screen to say why. */
    if (p && m_scoped(p)) {
        if (kbm->wButtons & XBOX_GAMEPAD_DPAD_LEFT) {
            pad->wButtons &= (WORD)~XBOX_GAMEPAD_DPAD_LEFT;
            pad->wButtons |= XBOX_GAMEPAD_DPAD_UP;
        }
        if (kbm->wButtons & XBOX_GAMEPAD_DPAD_RIGHT) {
            pad->wButtons &= (WORD)~XBOX_GAMEPAD_DPAD_RIGHT;
            pad->wButtons |= XBOX_GAMEPAD_DPAD_DOWN;
        }
    }
}

/* "key:NAME:FRAMES": a button held as the keyboard sends it, through the
 * bindings filter (the toolkit's RECOMP_INPUT_SEQ is a pad added after it,
 * so it never reaches the filter). */
static void m_seq_key(const char *s, char **end)
{
    static const struct { const char *name; WORD bit; } k_keys[] = {
        { "dpad_up", XBOX_GAMEPAD_DPAD_UP },     { "dpad_down", XBOX_GAMEPAD_DPAD_DOWN },
        { "dpad_left", XBOX_GAMEPAD_DPAD_LEFT }, { "dpad_right", XBOX_GAMEPAD_DPAD_RIGHT },
        { "rthumb", XBOX_GAMEPAD_RIGHT_THUMB },  { "start", XBOX_GAMEPAD_START },
    };
    size_t i, n;
    unsigned long frames = 3;

    for (n = 0; s[n] && s[n] != ':' && s[n] != ','; n++)
        ;
    *end = (char *)s + n;
    if (s[n] == ':')
        frames = strtoul(s + n + 1, end, 10);
    for (i = 0; i < sizeof k_keys / sizeof k_keys[0]; i++)
        if (strlen(k_keys[i].name) == n && strncmp(s, k_keys[i].name, n) == 0) {
            M.key_buttons = k_keys[i].bit;
            M.key_until = M.frame + (unsigned)frames;
        }
}

/* RECOMP_TS2_MOUSE_SEQ: the steps due by this frame. */
static void m_seq(void)
{
    while (M.seq && *M.seq) {
        char *end;
        unsigned long at = strtoul(M.seq, &end, 10);
        float dx, dy;

        if (end == M.seq || *end != ':' || at > M.frame)
            return;
        if (strncmp(end + 1, "key:", 4) == 0) {
            m_seq_key(end + 5, &end);
            while (*end && *end != ',')
                end++;
            M.seq = *end ? end + 1 : NULL;
            continue;
        }
        dx = strtof(end + 1, &end);
        dy = *end == ':' ? strtof(end + 1, &end) : 0.0f;
        recomp_mouse_move(dx, dy);
        while (*end && *end != ',')
            end++;
        M.seq = *end ? end + 1 : NULL;
    }
}

void ts2_mouse_frame(void)
{
    M.frame++;
    m_seq();
    if (M.started)
        return;
    M.started = 1;
    recomp_bindings_set_filter(m_filter);
    M.log = recomp_config_bool("RECOMP_TS2_MOUSE_LOG", NULL, 0);
    M.seq = getenv("RECOMP_TS2_MOUSE_SEQ");
    M.invert = recomp_config_bool("RECOMP_TS2_MOUSE_INVERT", "ts2_mouse_invert", 0);
    M.steady = recomp_config_bool("RECOMP_TS2_STEADY_AIM", "ts2_steady_aim", 1);
    if (A->player_base) {
        recomp_mouse_set_direct(1);
        fprintf(stderr, "[TS2-MOUSE] the mouse turns the player directly%s\n",
                recomp_mouse_look() ? "" : " (once mouse look is on)");
    } else {
        fprintf(stderr, "[TS2-MOUSE] this release's look is not known yet: "
                        "the mouse moves the right stick\n");
    }
}

/* ── look ────────────────────────────────────────────────────── */

void ts2_mouse_look(void (*look)(void))
{
    uint32_t p, stick, flags, mode;
    float pitch0 = 0.0f, dx, dy, deg, fov, limit, yaw, pitch;
    int mine, took;

    p = A->current_player ? M_MEM32(A->current_player) : 0;
    stick = M_MEM32(g_esp + 4);             /* [esp] is the return address */
    mine = p && p == m_player1();
    if (mine)
        pitch0 = M_MEMF(p + TS2_PM_PITCH);
    look();
    if (!mine || M.held || !recomp_mouse_look())
        return;

    /* What look turns by itself: the player record, by the look stick.
     * The rest (a mounted gun, the modes it leaves alone) stays the pad's. */
    flags = M_MEM32(p + TS2_PM_FLAGS);
    mode = M_MEM32(p + TS2_PM_MODE);
    if (!(flags & 8) || M_MEM32(p + TS2_PM_HELD) || mode == 6 || mode == 7 ||
        mode == 8 || mode == 9 || !m_camera_is_eye(p))
        return;

    /* Auto lookahead with a still look stick: not with a mouse. */
    pitch = M_MEMF(p + TS2_PM_PITCH);
    if (m_ptr_ok(stick) && M_MEMF(stick + 4) == 0.0f)
        pitch = pitch0;

    /* Motion that waited while nothing took it (a menu, a cutscene, a
     * load) is old: the first look after it starts from still. */
    took = recomp_mouse_take(&dx, &dy);
    if (M.frame - M.took > 2u)
        dx = dy = 0.0f;
    M.took = M.frame;

    deg = (float)(TS2_MOUSE_DEGREES *
                  pow(2.0, (recomp_mouse_sensitivity() - 80) / TS2_MOUSE_PER_STEP));
    fov = A->context && m_ptr_ok(M_MEM32(A->context))
              ? M_MEMF(M_MEM32(A->context) + A->context_fov) : TS2_MOUSE_FOV;
    if (fov > 1.0f && fov < 170.0f)
        deg *= fov / TS2_MOUSE_FOV;

    yaw = M_MEMF(p + TS2_PM_YAW) - dx * deg;      /* turning right lowers it */
    while (yaw < 0.0f)
        yaw += 360.0f;
    while (yaw >= 360.0f)
        yaw -= 360.0f;
    /* Screen y grows down; up on the mouse is up on the stick. */
    pitch += (M.invert ? dy : -dy) * deg;
    limit = A->pitch_limit ? M_MEMF(A->pitch_limit) : 90.0f;
    if (limit > 0.0f && limit <= 90.0f) {
        if (pitch > limit)  pitch = limit;
        if (pitch < -limit) pitch = -limit;
    }
    M_MEMF(p + TS2_PM_YAW) = yaw;
    M_MEMF(p + TS2_PM_PITCH) = pitch;

    if (M.log && M.frame - M.logged >= (took ? 10u : 60u)) {
        M.logged = M.frame;
        fprintf(stderr, "[TS2-MOUSE] frame %u: moved %.0f,%.0f  yaw %.1f pitch %.1f "
                        "(limit %.1f)  fov %.1f  flags 0x%X mode %u aiming %u  "
                        "sway %.2f,%.2f%s\n",
                M.frame, dx, dy, yaw, pitch, limit, fov, flags, mode,
                M_MEM32(p + TS2_PM_AIMING), M_MEMF(p + TS2_PM_SWAY_YAW),
                M_MEMF(p + TS2_PM_SWAY_PITCH), M.still ? " held" : "");
    }
}

/* ── changing weapon ─────────────────────────────────────────── */

/* Aim mode is a toggle by default and the game carries it over a weapon
 * change: aiming (+0x884) is worked out each tick as flags bit 1 (the aim
 * toggle, or the button held with the hold setting) outside modes 4, 7 and
 * 14, so it reads 0 while the weapon changes and comes back on the new one
 * -- the sniper rifle came up looking down its scope. Pressing aim mode's
 * button after the change landed too late (two frames of scope, then the
 * zoom out); before it, during the change, it is unclear what a press does.
 * So once the change asked for has started (flags 0x10), the toggle bit
 * itself is cleared, and the new weapon comes up at the hip. With the hold
 * setting the game sets it again from the button still held. Given up if no
 * change starts in half a second (nothing else to change to). */
static void m_unaim_tick(uint32_t p)
{
    uint32_t flags;

    if (!M.unaim || p != m_player1())
        return;
    flags = M_MEM32(p + TS2_PM_FLAGS);
    if (flags & TS2_PF_CHANGING) {
        M_MEM32(p + TS2_PM_FLAGS) = flags & ~(uint32_t)TS2_PF_AIM;
        M.unaim = 0;
    } else if (M.frame - M.unaim > 30u) {
        M.unaim = 0;
    }
}

/* ── breathing ───────────────────────────────────────────────── */

/* Moves player 1's view angles by a sway, so yaw + sway (what the camera
 * shows) is kept when that sway is taken away (sign 1) or given back (-1). */
static void m_fold_sway(uint32_t p, float sign, float sway_yaw, float sway_pitch)
{
    float yaw = M_MEMF(p + TS2_PM_YAW) + sign * sway_yaw;

    while (yaw < 0.0f)
        yaw += 360.0f;
    while (yaw >= 360.0f)
        yaw -= 360.0f;
    M_MEMF(p + TS2_PM_YAW) = yaw;
    M_MEMF(p + TS2_PM_PITCH) = M_MEMF(p + TS2_PM_PITCH) + sign * sway_pitch;
}

void ts2_mouse_breath(void (*breath)(void))
{
    uint32_t p;
    int still;
    float shown_yaw = 0.0f, shown_pitch = 0.0f;

    /* What the camera showed last: the game's own breathing sets the sway
     * to 0 itself on the first tick through a scope, so it has to be read
     * before it runs. */
    p = A->current_player ? M_MEM32(A->current_player) : 0;
    if (p && p == m_player1()) {
        shown_yaw = M_MEMF(p + TS2_PM_SWAY_YAW);
        shown_pitch = M_MEMF(p + TS2_PM_SWAY_PITCH);
    }
    breath();
    if (p)
        m_unaim_tick(p);
    if (!M.steady || !p || p != m_player1())
        return;

    /* The sway is made afresh each tick from the phases, which run on, so
     * there is nothing to undo: the camera adds whatever is here this tick. */
    still = M_MEM32(p + TS2_PM_AIMING) != 0;
    if (still && M.still != p)          /* aiming: keep the view where it is */
        m_fold_sway(p, 1.0f, shown_yaw, shown_pitch);
    else if (!still && M.still == p)    /* not: the sway resumes from here */
        m_fold_sway(p, -1.0f, M_MEMF(p + TS2_PM_SWAY_YAW), M_MEMF(p + TS2_PM_SWAY_PITCH));
    M.still = still ? p : 0;            /* a new level's record starts afresh */
    if (still) {
        M_MEMF(p + TS2_PM_SWAY_YAW) = 0.0f;
        M_MEMF(p + TS2_PM_SWAY_PITCH) = 0.0f;
    }
}
