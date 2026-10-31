/*
 * TimeSplitters 2, USA release: what src/ts2_mapmaker_mouse.c reads.
 * default.xbe SHA-1 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the
 * disassembly (31 Oct 2026). Nothing is hooked: the editor runs as it
 * always did, and the mouse writes the hand's position and presses the
 * pad's buttons.
 *
 * The editor: while its level runs and no screen is up (F+0x24 == 0), the
 * level update sub_000DDC10 calls the editor's, sub_00117A50. The hand is
 * at F+0x30, F+0x34 (16.16 screen pixels), moved by the left stick in
 * sub_00086E80 while F+0x18 has bit 2, drawn by sub_000FECD0; with bit 1
 * as well, sub_00086FE0 writes the id of the hot spot under it to S+0x648.
 * A places (held, it lassos), B deletes, Y turns, X copies, the D-pad's
 * left and right change tab and its up and down the floor, the triggers
 * zoom, Start opens the editor's menu.
 */
static const Ts2MapMakerAddrs k_ts2_mapmaker = {
    /* level: MapMaker's editor (a map being played or previewed is 0x64). */
    0x66,
    /* margin_x, margin_y: the hand stops 64 in from the sides and 32 from
     * the top and bottom; the stick past that scrolls. */
    64, 32,
    /* screen_bounds: the hand's limits with a screen up, at 0x004A34A0. */
    0x004A34A0,
    /* zoom_in_right: the right trigger zooms in. */
    1,
};
