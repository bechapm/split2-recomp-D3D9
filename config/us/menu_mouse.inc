/*
 * TimeSplitters 2, USA release: what src/ts2_menu_mouse.c reads.
 * default.xbe SHA-1 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the
 * disassembly (30 Oct 2026). Nothing is hooked: the menu code runs as it
 * always did, and the pointer reads and sets what it reads and sets.
 */
extern void sub_001C68C0(void);
extern void sub_001AB030(void);
extern void sub_001AC170(void);
extern void sub_00121900(void);

static const Ts2MenuAddrs k_ts2_menu = {
    /* frontend: four menu states of 0x7AC bytes from 0x004AE680, one per
     * local player, cleared and set up by sub_000875D0; [0x004AE668] points
     * at the one being worked on and is left at player 1's. The screens
     * there are run by sub_000870E0: those with flags 1 and 4 each frame. */
    0x004AE680,
    /* level_id and the first value that is not a playable level, as the
     * debugging tools and the mouse have them. */
    0x004A46EC, 0x64,
    /* pause: [0x0040BC58], 1 while the game is paused (the pause menu's
     * sub_001AE110 sets it), as the debugging tools have it. */
    0x0040BC58,
    /* sound_on, sounds: the panel update (sub_000F1900) plays [[0x004AD688]]
     * through sub_001C68C0(sound, 0) when it moves the selection, if
     * [0x004AD684] is set. */
    0x004AD684, 0x004AD688,
    sub_001C68C0,
    /* viewport: [0x00483F28], read by the 3D text (sub_001ACD90) to put its
     * projected anchor on screen. */
    0x00483F28,
    /* fonts, glyph_sets, font_now, text_flags: sub_001AB030(font) selects
     * font n: [0x0040BB24] = n, [0x0040BB2C] = 0x00345350 + n * 0x1C, and
     * [0x0040BB28] = 0x003452D0 + 0x20 * that font's first dword. Text is
     * half height while [0x0048D764] bit 0 is set (sub_001AB100). */
    0x00345350, 0x003452D0, 0x0040BB24, 0x0048D764,
    /* font_select, text_width: sub_001AC170(string) is the width the text
     * draws in the font selected, in pixels; the button hints
     * (sub_000EFEB0) and an option's value (sub_000F1230) are laid out by
     * it. */
    sub_001AB030, sub_001AC170,
    /* page_prev, page_next: what a pause menu page screen keeps for the
     * D-pad's left and right (S+0x628, S+0x62C, set by sub_000D5240 and the
     * like): the page before (sub_000CC010) and after (sub_000CC0B0). Its
     * arrows are drawn by sub_000D1A00. */
    0x000D19C0, 0x000D19E0,
    /* styles, sine: sub_000F1230 colours an item by its style, the block at
     * 0x002D1538 + style * 0xC0 (dwords r, g, b, a). The line selected is
     * (pulse, [+0x44], [+0x48], [+0x4C]), pulse = ((sin + 1) * 0.3 + 0.4) * 127
     * with sin = [0x00488260 + 4 * (int(clock * 2048 + 0.5) & 0x7FF)], the
     * clock [F + 0x29C] (0x000F136C); an option's arrows are [+0x90] on the
     * selected line and [+0xA0] while that way is held (0x000F1676). */
    0x002D1538, 0x00488260,
    /* item_draw: sub_000F1230, the item drawing, which draws an option's
     * arrows. scroll_draw: sub_000CCCB0, a pause page's scroll arrows (from
     * the Inventory, Status and Briefing drawings, sub_000D7840,
     * sub_000D8F30, sub_000D92D0). page_draw: sub_000D1A00, its page arrows,
     * each through sub_001B75A0. pause_select: the return from the
     * sub_001ACA50 call in sub_000CD030 that draws a pause page's "Select"
     * (text 2123), right-aligned, its A icon drawn after it to its left. */
    0x000F1230, 0x000F18F2,
    0x000CCCB0, 0x000CCEB4,
    0x000D1A00, 0x000D1DD0,
    0x000CD10F,
    /* players: sub_00121900, the number of local players (the pause pages
     * lay themselves out for it). inventory: sub_000D8120, the Inventory
     * page's update (its screen's class names it at +4, as sub_000870E0 calls it):
     * it moves [F+0x30C] through the ids at [F+0x31C] with the D-pad's up
     * and down (F+0x18 bits 4, 8), plays the move sound, and on A or Start
     * takes the item selected and closes the pause menu (sub_000D2520). */
    sub_00121900, 0x000D8120,
};
