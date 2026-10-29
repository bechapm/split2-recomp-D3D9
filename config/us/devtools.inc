/*
 * TimeSplitters 2, USA release: where src/ts2_devtools.c finds what its
 * tools read and write. default.xbe SHA-1
 * 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. "code" was read from the
 * disassembly; "measured" was seen in scripted Siberia runs (29 Oct 2026).
 */
/* The free camera's portal walk: room_walk(from, from_room, to, 0), cdecl,
 * returns the room holding `to` (code; up to 100 portals). */
extern void sub_0001FDC0(void);

static const Ts2DevAddrs k_ts2_dev = {
    /* player_base: [0x00497028] is the local player records, 0xD3C each;
     * player 1 is the first (code: P = base + sub_00121930(i) * 0xD3C;
     * measured). */
    0x00497028,
    /* level_id: 35 in Siberia, 0x65 in the front end (measured). */
    0x004A46EC, 0x65,
    /* god, infinite_ammo: the developers' switches, in BSS, written only by
     * their own toggles (sub_000553F0, sub_00055740) and a developer button
     * combination. With god set the damage filter sub_00051EC0 ignores
     * damage to a player-controlled character and fall damage is skipped;
     * 0x00356898 is tested where ammo is spent (code). */
    0x00356890, 0x00356898,
    /* unlock_all: sub_001AE6D0 passes every unlock condition when it is set;
     * the override bytes 0x0035F22C[n] are read by sub_000A9700(n):
     * 0 story levels, 1 challenges, 2 arcade league, 3 characters
     * (sub_001AEA10), 4 cheats (sub_001AE9D0); 5 has no reader (code). Their
     * only writers are the developers' button-combination toggles
     * sub_000A9710..sub_000A9840, which also write every unlock into the
     * save when the first profile is named DUFFINS; they are never called
     * from here. */
    0x0035F228, 0x0035F22C, 5,
    /* pause: the frame clock sub_001ADF80 makes ticks [0x0034889C] and the
     * time scale [0x003488A4] from it every frame: 0 running, >0 paused
     * (ticks 0), -1 resuming (one tick, then 0). sub_001AE110(on) is the
     * pause menu's way of setting it (code; measured 0 and 1). */
    0x0040BC58,
    /* context: the camera/render context (code, as in overrides.inc). */
    0x004B2F60,
    /* look_at_return: the camera update sub_00035930 calls look_at
     * (sub_000E67F0) at 0x00036D47 with the view [ctx+0x208], the eye
     * (ctx+0x27C), the forward (ctx+0x2E4, copied to locals) and the up
     * (ctx+0x304), all by value; 0x00036D4C is its return address (code). */
    0x00036D4C,
    sub_0001FDC0,
};
