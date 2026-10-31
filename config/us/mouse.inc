/*
 * TimeSplitters 2, USA release: what src/ts2_mouse.c reads and writes.
 * default.xbe SHA-1 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the
 * disassembly (29 Oct 2026); the hook itself, on look sub_00148A60, is in
 * overrides.inc.
 */
static const Ts2MouseAddrs k_ts2_mouse = {
    /* player_base: [0x00497028] is player 1's record (P), as the debugging
     * tools have it. */
    0x00497028,
    /* current_player: [0x00497100] is the record look and the rest of the
     * player update are working on; look reads and writes only it. */
    0x00497100,
    /* level_id and the first value that is not a playable level, as the
     * debugging tools have them (0x65 measured in the front end; 0x64 is
     * a MapMaker map being played, see config/us/devtools.inc). */
    0x004A46EC, 0x65,
    /* pitch_limit: look clamps P+0x14C to +-[0x00497104] when it is done
     * (measured 40 in Siberia). Look's turn rates are [0x00497138] and
     * [0x0049713C] (measured 160 and 80): it adds rate * P+0x8A8 * ctx+0x324
     * / 3600 * time scale * stick to the angles. */
    0x00497104,
    /* context: the camera/render context ([0x004B2F60]); +0x324 is the
     * field of view look scales its turn rate by, smaller when zoomed. */
    0x004B2F60, 0x324,
};
