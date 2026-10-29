/*
 * TimeSplitters 2, USA release: where src/ts2_devtools.c finds what its
 * tools read and write. default.xbe SHA-1
 * 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. "code" was read from the
 * disassembly; "measured" was seen in scripted Siberia runs (29 Oct 2026).
 */
/* The free camera's portal walk: room_walk(from, from_room, to, 0), cdecl,
 * returns the room holding `to` (code; up to 100 portals). */
extern void sub_0001FDC0(void);
/* The ray test sub_00020160 (47 callers; a wrapper of the core
 * sub_0001FE80, which walks the rooms through their portals and tests each
 * with sub_00154DF0). Its own wrapper in overrides.inc is what is called. */
extern void sub_00020160(void);
/* give_item sub_000B5E90(R, item) and fill_ammo sub_000AFF50(R, item),
 * below. */
extern void sub_000B5E90(void);
extern void sub_000AFF50(void);
/* room_at sub_0011FF10, below. */
extern void sub_0011FF10(void);
/* resident_find sub_001495C0 and pak_find sub_00086360, below. */
extern void sub_001495C0(void);
extern void sub_00086360(void);

static const Ts2DevAddrs k_ts2_dev = {
    /* player_base: [0x00497028] is the local player records, 0xD3C each;
     * player 1 is the first (code: P = base + sub_00121930(i) * 0xD3C;
     * measured). */
    0x00497028,
    /* level_id: 35 in Siberia (measured); 0x64 and up are the front end
     * and the screens around it (0x65 measured in the front end; the
     * per-viewport draw sub_000DE610 switches on 0x65-0x69, and code
     * compares against 0x64 and 0x6A). */
    0x004A46EC, 0x64,
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
    /* ticks and time scale, which the frame clock sub_001ADF80 writes every
     * frame (1..3 and the same as a float running, 0 paused; measured). */
    0x0034889C, 0x003488A4,
    /* context: the camera/render context (code, as in overrides.inc). */
    0x004B2F60,
    /* look_at_return: the camera update sub_00035930 calls look_at
     * (sub_000E67F0) at 0x00036D47 with the view [ctx+0x208], the eye
     * (ctx+0x27C), the forward (ctx+0x2E4, copied to locals) and the up
     * (ctx+0x304), all by value; 0x00036D4C is its return address (code). */
    0x00036D4C,
    sub_0001FDC0,
    sub_00020160,
    /* object_list: 1324 entries of 0x24, a pointer to the ob/... name first;
     * an object's model index is at +8 (measured: chr47.xbr for Ilsa). */
    0x002D89CC,
    /* sight_returns: the AI perception update sub_00065340 runs
     * sub_00080590 (story) or sub_0006B450 (otherwise), which cast the ray
     * from the enemy's room at its target (R+0x1300) and count the target
     * seen when the ray's object is the target (cmp eax,edi at 0x00080761,
     * 0x00080A4C, 0x0006B650; cmp eax,[esi+0x1300] at 0x000808A7; test at
     * 0x0006B7C3). These are those calls' return addresses (code). */
    { 0x00080761, 0x0008089E, 0x00080A49, 0x0006B64D, 0x0006B7BE },
    /* give_item: sub_000B5E90(R, item) sets the owned flag R+0x984+item*4,
     * lets sub_000B3010 decide the automatic switch (the player's setting;
     * a switch writes R+0xC8), shows the item's name and plays the pickup
     * sound. The weapon pickup sub_0013DD30 calls it after giving the
     * weapon's two ammunition kinds with sub_000B04D0 (code). */
    sub_000B5E90,
    /* fill_ammo: sub_000AFF50(R, item) sets the item's ammunition pools to
     * the maximum in the ammunition table 0x2C1678 (code). */
    sub_000AFF50,
    /* item_count, item_names, item_name_stride: the item table at 0x2C9BF0
     * has 42 entries of 0x24; each item's text id is the word at
     * 0x2CA1D0 + item * 0x48 (code; measured: 36 Temporal Uplink, 11
     * Sniper Rifle). */
    42, 0x002CA1D0, 0x48,
    /* text_table, text_count: the English strings, a pointer per text id
     * (the language table 0x00335EA8's English entry; 2770 ids; code). */
    0x00335EC0, 2770,
    /* room_at: sub_0011FF10(pos, up, half), 35 callers. It lists the rooms
     * whose bounding boxes (the room table [0x3534D4] + room * 0xB0, min at
     * +0x24, max at +0x30) overlap a box round `pos` (sub_0001E550), keeps
     * the one with the highest floor below the point (sub_0011FC50), falls
     * back to a portal walk from the last room it found, and returns the
     * room (>0) or <=0 for none. The player's spawn sub_00142C90 calls it
     * at 0x0014301D with up = half = 0.1 and stores the room in P+0x87C,
     * P+0x880 and C+0x14 (code). */
    sub_0011FF10,
    /* player_count: the players are character indices 0..[0x4A46F0]-1; the
     * awareness table is sized from it (code). */
    0x004A46F0,
    /* item_table, item_stride: 0x2C9BF0, 42 of 0x24; +0 and +4 are the
     * left and right weapon types, -1 for none (code; measured: item 11
     * Sniper Rifle is type 6, 13 -> 8, 14 -> 9, 19 Rocket Launcher -> 13). */
    0x002C9BF0, 0x24,
    /* weapon_defs, stride, count: 0x2C3468 + type * 0x240, the table the
     * weapon pickup sub_0013DD30 reads; +0 is the first-person model
     * (*_ph.xbr), +4 the third-person one (*_cl.xbr) (measured: type 6 is
     * 993/994 sniperrifle, 13 is 1013/1014 rocketlauncher). The count is
     * only a bound on what is read. */
    0x002C3468, 0x240, 64,
    /* model_loaded: the model list's entries are 0x2D89C0 + m * 0x24 (the
     * path at +0xC is object_list above); +4 is the loaded model, 0 until
     * first use and cleared at each level start (sub_00151CC0). The loader
     * sub_0014BA20 fills it with sub_0012C820(path), which tries the
     * resident table (sub_001495C0), then the mounted pak (sub_00086360),
     * then the loose file on the disc (sub_00086970), where a gun the level
     * does not carry crashed (code; measured with the Rocket Launcher in
     * Siberia). */
    0x002D89C4,
    sub_001495C0,
    sub_00086360,
};
