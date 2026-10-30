/*
 * TimeSplitters 2, USA release: what src/ts2_menu_mouse.c reads.
 * default.xbe SHA-1 3ec95fe3ae9e7794d83a5b489ad8583f352e0c18. Read from the
 * disassembly (30 Oct 2026). Nothing is hooked: the menu code runs as it
 * always did, and the pointer reads and sets what it reads and sets.
 */
extern void sub_001C68C0(void);

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
};
