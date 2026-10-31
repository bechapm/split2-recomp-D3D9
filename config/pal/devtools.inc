/*
 * TimeSplitters 2, PAL release: where src/ts2_devtools.c finds what its
 * tools read and write. default.xbe SHA-1
 * 2809eb147385723eaa90c425be89ee4db033fc5a. Only the unlock switches are
 * known here (docs/formats/roster.md); every other tool stays off in this
 * release until its addresses are found (the USA table,
 * config/us/devtools.inc, says what each one is).
 */
static const Ts2DevAddrs k_ts2_dev = {
    0,                  /* player records */
    0, 0,               /* level id, first non-level value */
    0, 0,               /* god, infinite ammo */
    /* unlock_all and the override bytes, read by sub_001AEB10 and
     * sub_000A9740(n) (code; docs/formats/roster.md). */
    0x0035F668, 0x0035F66C, 5,
    0,                  /* pause */
    0, 0,               /* ticks, time scale */
    0,                  /* context */
    0,                  /* look_at */
    0,                  /* room walk */
    0, 0,               /* ray, object list */
    { 0 },              /* sight rays */
    0, 0,               /* give item, fill ammo */
    0, 0, 0,            /* item names */
    0, 0,               /* English text */
    0, 0,               /* room by position, player count */
    0, 0, 0, 0, 0, 0,   /* items' models */
    0, 0, 0, 0,         /* their lookups */
    0, 0, 0, 0, 0,      /* binding a model's textures */
    0, 0, 0, 0, 0,      /* the HUD's named textures */
    0, 0, 0,            /* and a gun's among them */
    0, 0,               /* the game's cloak */
    0, 0, 0, 0,         /* rooms, portals and the screen (room culling) */
};
