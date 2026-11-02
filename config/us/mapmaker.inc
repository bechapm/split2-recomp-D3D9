/*
 * TimeSplitters 2, USA release: what src/ts2_mapmaker_mouse.c and
 * src/ts2_mapmaker_view.c read. default.xbe SHA-1
 * 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the disassembly (31
 * Oct 2026). The mouse hooks nothing: the editor runs as it always did, and
 * the mouse writes the hand's position and presses the pad's buttons. The
 * wider view's hooks are in overrides.inc.
 *
 * The editor: while its level runs and no screen is up (F+0x24 == 0), the
 * level update sub_000DDC10 calls the editor's, sub_00117A50. The hand is
 * at F+0x30, F+0x34 (16.16 screen pixels), moved by the left stick in
 * sub_00086E80 while F+0x18 has bit 2, drawn by sub_000FECD0; with bit 1
 * as well, sub_00086FE0 writes the id of the hot spot under it to S+0x648.
 * A places (held, it lassos), B deletes, Y turns, X copies, the D-pad's
 * left and right change tab and its up and down the floor, the triggers
 * zoom, Start opens the editor's menu. The editor clamps the hand each
 * frame (0x001193E1) and writes it itself while zooming, about the map
 * point under it (0x00119648).
 */
static const Ts2MapMakerAddrs k_ts2_mapmaker = {
    /* level: MapMaker's editor (a map being played or previewed is 0x64). */
    0x66,
    /* margin_x, margin_y: the hand stops 64 in from the sides and 32 from
     * the top and bottom; the stick past that scrolls. */
    64, 32,
    /* screen_bounds: the hand's limits with a screen up: top, bottom, left,
     * right, 16.16, as 0x001188FB reads them (the palette, sub_00108FA0:
     * 60, 384, 91, 548). */
    0x004A34A0,
    /* zoom_in_right: the right trigger (held bit 2, 0x00348C10) scales the
     * view by 1.02 a frame and the left (bit 1) by 0.98, from 60 cells
     * across to 8 (0x001193C3). */
    1,
    /* mode_class, mode_row: the Mode question, class 0x002D2C48 (update
     * sub_000FFBC0): the D-pad's up and down move [0x00398E24] between 0 and
     * 1, A takes it. Its hot spots (sub_000FFA50) only say where the title
     * and the lines are, all flagged 1, and its hand limits are all 0. */
    0x002D2C48, 0x00398E24,
    /* lists: Load and Save Map's list of files, class 0x002D37D0 (draw
     * sub_0011C440, bar 0x82 to 0x211), and its list of disks, 0x002D3A2C
     * (sub_0011C780, bar 0x9B to 0x1EE). */
    { { 0x002D37D0, 130, 529 }, { 0x002D3A2C, 155, 494 } },
    /* view: x and w set by sub_0010F910 from the screen rectangle and only
     * there; span, left and right from it, sub_0010F7C0, a room's view in
     * sub_00108FA0 and sub_0010C140, and the zoom. Each cell is 0x200000 of
     * the map, so the span 0x2000000 is 16 cells across 640 pixels. */
    { 0x004A2BD8, 0x004A2AA4, 0x004A2AA0, 0x004A2A98, 0x004A2A80 },
    /* screen: left +0, right +4; sub_001B7450, the box draw, clips to them. */
    0x00483F28,
    /* zoom_limits: 0x7800000 (60 cells) and 0x1000000 (8), divided into the
     * view's width at 0x001194C9 and 0x001194DE (sub_0010F4C0, second
     * argument) and multiplied at 0x00119540 and 0x00119566 (sub_0010F4B0,
     * first argument). */
    { 0x001194CE, 0x001194E3, 0x00119545, 0x0011956B },
};
