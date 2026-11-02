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
/* resident_find sub_001495C0 and the pak lookups sub_000862A0 (P4CK) and
 * sub_00086040 (P8CK), below; the lookups are k_ts2_paks's too. */
extern void sub_001495C0(void);
extern void sub_000862A0(void);
extern void sub_00086040(void);
/* pak_data, material_prepare, texture_load and texture_load_alt, below. */
extern void sub_00086360(void);
extern void sub_00129460(void);
extern void sub_001B7E60(void);
extern void sub_001B7F70(void);
/* named_record and named_load, below. */
extern void sub_00149790(void);
extern void sub_001B7D80(void);
extern void sub_000A9BA0(void);

/* cloak_on, cloak_off, below. */
extern void sub_000459C0(void);
extern void sub_00045A10(void);

static const Ts2DevAddrs k_ts2_dev = {
    /* player_base: [0x00497028] is the local player records, 0xD3C each;
     * player 1 is the first (code: P = base + sub_00121930(i) * 0xD3C;
     * measured). */
    0x00497028,
    /* level_id: 35 in Siberia (measured); above 0x64 are the front end
     * and the screens around it (0x65 measured in the front end; the
     * per-viewport draw sub_000DE610 switches on 0x65-0x69, and code
     * compares against 0x64 and 0x6A). 0x64 itself is a MapMaker map being
     * played or previewed, a level like any other: the game's own test for
     * a menu level is level > 0x64 (round 6 reading, 31 Oct 2026; MapMaker's
     * editor is 0x66). */
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
     * resident table (sub_001495C0), then a pak read into memory
     * (sub_00086360, which answers only for those), then a read by name
     * through the mounted paks (sub_00086970), which goes to the disc as a
     * loose file when no pak has the name: where a gun the level does not
     * carry crashed (code; measured with the Rocket Launcher in Siberia).
     * So "can it be found" is the resident table or the mounted paks'
     * directories, which a mod's files are added to: the P4CK and P8CK
     * lookups sub_000862A0 and sub_00086040, which leave the record at
     * 0x3585F0 (k_ts2_paks, overrides.inc; code). */
    0x002D89C4,
    sub_001495C0,
    sub_000862A0, sub_00086040, 0x003585F0,
    /* The model loader sub_0012C820(path) relocates a model and keeps its
     * material list at M+0xC but binds no textures: the requests
     * (sub_00149CA0) come only from level-load code, and the level-load pass
     * sub_00149D20 loads them. A model loaded mid-level that the level's
     * resident data lacks kept its raw texture ids (rocketlauncher_ph
     * 0x5BB-0x5C1, read as slots out of range of the loaded table
     * [0x348A2C], capacity 0x400) and drew untextured (measured). The
     * run-time binder sub_0012C7C0 does: data = sub_00086360(path) (the
     * file's data in a pak read into memory), sub_00129460([M+0xC], data),
     * then for each record sub_001B7E60(id, rec, rec), or sub_001B7F70 when
     * rec+0xC has 0x10000000, skipping id 0x40000000; each writes the
     * texture's slot over the id (code; measured). */
    sub_00086360, sub_00129460, sub_001B7E60, sub_001B7F70, 0x0C,
    /* named_textures, named_count: 0x2BF390 + i * 0x10, 548 entries, name
     * at +8 ("host0:textures/misc/<x>.raw"), handle at +0xC. sub_000A9C20
     * sets every handle to -2 at level start and asks for the level's set;
     * sub_000A9BA0(i) asks for one (only on -2: handle -1, its parameters
     * at 0x35FF20, a resident record added by sub_00149FA0); the level-load
     * pass sub_00149810 loads each record with sub_001B7D80(name,
     * [rec+0x10], rec+0x14), and sub_000A9D10 then writes sub_00149790
     * (name)->+0x14, or -1 with no record, into every handle. sub_000A9AA0(i)
     * gives the HUD the handle. (code; measured: in Siberia ammo_laser 43,
     * hudtanklaser 78 and the rest of the Lasergun's, Minigun's, Plasma
     * Autorifle's and Homing Launcher's are -1 with no record, 378 of the
     * 548 in all; the resident table has room for 700 records, 353 used.) */
    0x002BF390, 548,
    sub_00149790, sub_001B7D80, sub_000A9BA0,
    /* ammo_hud, ammo_stride, ammo_count: a weapon's are asked for by
     * sub_000B0010(type), for its ammunition types (def +0x10, +0x18):
     * [0x2C167C + t * 0xD0], then +4 and +8 (when not -1) of up to four
     * 0x20-byte entries from 0x2C1680 + t * 0xD0 (until +0 is 0) (code;
     * measured: minigun ammunition 3 -> 38, 67, 68; laser 10 -> 43, 76, 78;
     * plasma 11 -> 45, 67, 68; homing 14 -> 47; rockets 13 -> 46, 75). The
     * count is only a bound on what is read. */
    0x002C167C, 0xD0, 32,
    /* cloak_on, cloak_off: the game's cloak, on the status block at R+0x1498.
     * The yellow orb of arcade matches is sub_00047A20 (from the power-up
     * pickup sub_0013D850, type 0): bit 0x10 of block+4, and 30 seconds in
     * block+0x28. sub_000459C0 is the same with no time limit, which is how
     * the AI cloaks itself (sub_00074980, sub_00075160, sub_000814B0), and
     * sub_00045A10 fades it out (code). */
    sub_000459C0, sub_00045A10,
    /* rooms, room_count, portals, viewport: the visibility pass sub_0001F450
     * (overrides.inc) starts from the room at ctx+0x294 and walks the
     * portals of each room ([0x3534D4] + room * 0xB0, +4 the list) through
     * the portal table [0x3534D8] (a portal: +0, +4 its two rooms) for rooms
     * 1..[0x4B33DC] (sub_0001E9B0, sub_0001EAB0), clipping each portal on
     * screen to the rectangle of [0x483F28] and of the room it is seen from
     * (sub_0001EF00, sub_0001EBE0). The draw sub_0001DA30 then draws the
     * rooms found by depth, each where its rectangle overlaps the screen's,
     * and a room's objects where their boxes do (sub_0001D860,
     * sub_0014E0F0) (code). */
    0x003534D4, 0x004B33DC, 0x003534D8, 0x00483F28,
    /* each room's objects: counts, list pointers (code: sub_0014E0F0 and
     * sub_0014E6F0 index both by room; sub_0014A1C0 adds an object to its
     * room's list, sub_0014A230 moves it) */
    0x00496920, 0x00496CA0,
};
