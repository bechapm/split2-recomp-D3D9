/*
 * TimeSplitters 2, USA release: what src/ts2_menu_mouse.c reads.
 * default.xbe SHA-1 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the
 * disassembly (30 Oct 2026). Nothing is hooked: the menu code runs as it
 * always did, and the pointer reads and sets what it reads and sets.
 */
extern void sub_001C68C0(void);
extern void sub_001AB030(void);
extern void sub_001AC170(void);

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
};
