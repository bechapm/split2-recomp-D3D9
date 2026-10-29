# Debugging tools

Tools for setting up a test scene and holding it still: god mode, infinite
ammo, invisibility to enemies, every unlock, stopping the game's clock or
changing its speed, a readout of where you are and what the crosshair is
on, saving and returning to a position, a free camera, noclip and a
wireframe view, all from a menu on screen. They are TimeSplitters 2's own
(`src/ts2_devtools.c`); the toolkit only supplies keys, the overlay's lines
and the pad. USA release only for now, apart from unlock everything, which
PAL has too.

## The menu

**F1**, or both sticks clicked in together (**L3+R3**), opens and closes it.
Up and down (the D-pad, the left stick or the arrow keys) choose a row, **A**
or **Enter** sets it, left and right change a value (the game speed), **B**
or **Esc** closes. The game reads an idle controller while the menu is open.
Choosing the free camera or noclip closes the menu, so the controller drives
them.

```text
TS2 DEV MENU
> Readout                  off
  God mode                 ON
  Infinite ammo            off
  Invisible to enemies     off
  Unlock everything        off
  Game speed               < 1x >
  Stop the clock           off
  One frame forward
  Free camera              off
  Noclip                   off
  Save position
  Back to saved position
  Wireframe                off
A/Enter set  left/right change  B/Esc close
```

## Settings

Every tool starts off. To start with one on, put the setting in the game's
settings file, `%APPDATA%\xboxrecomp\titles\4553000A.conf` on Windows, one
per line, or set the environment variable beside it:

| Setting | Environment variable | What it does |
| --- | --- | --- |
| `ts2_dev = 0` | `RECOMP_TS2_DEV` | No menu and no keys (they are on by default) |
| `ts2_god = 1` | `RECOMP_TS2_GOD` | God mode from the start |
| `ts2_infinite_ammo = 1` | `RECOMP_TS2_INFINITE_AMMO` | Infinite ammo from the start |
| `ts2_invisible = 1` | `RECOMP_TS2_INVISIBLE` | Enemies cannot see you |
| `ts2_unlock_all = 1` | `RECOMP_TS2_UNLOCK_ALL` | Every character, level and cheat |
| `ts2_readout = 1` | `RECOMP_TS2_READOUT` | The readout from the start |
| `ts2_speed = 0.5` | `RECOMP_TS2_SPEED` | The game's speed: 0.25, 0.5, 1, 2 or 4 |
| `ts2_teleport = x,y,z[,yaw,pitch]` | `RECOMP_TS2_TELEPORT` | Where you are put once the first level is playing |
| `ts2_dev_seq = FRAME:ACTION,...` | `RECOMP_TS2_DEV_SEQ` | The keys' actions at given frames (below) |
| | `RECOMP_TS2_POSITIONS` | Where saved positions are kept (default `ts2_positions.txt` beside the executable) |

## Shortcut keys

The menu's items on keys, only while the game's window is in front.

| Key | Tool |
| --- | --- |
| F1 | The menu |
| F2 | God mode |
| F3 | Infinite ammo |
| F4 | Invisible to enemies |
| Home | Unlock everything |
| F5 | Save your position (one per level) |
| F6 | Back to the saved position |
| F7 | Free camera |
| F8 | Noclip |
| F12 or Pause | Stop the game's clock, or start it again |
| Page Down | One frame forward while the clock is stopped |
| End | Wireframe view (every draw as lines; `RECOMP_WIREFRAME=1` starts with it on) |
| - and + | Slower and faster |

F9, F10 and F11 stay the toolkit's: the frame-rate counter, the frame cap and
a frame capture.

**Free camera.** Left stick (or WASD) moves, right stick (or the arrow keys)
looks, right trigger (or Space) rises, left trigger (or Ctrl) sinks, hold A
(or Shift) to go faster and B to go slower. The game reads an idle
controller meanwhile, so you stay where you were. With the clock stopped
you fly around a frozen scene. The view RTX Remix is given follows the free
camera, and the level keeps drawing from the room the camera is in.

**Noclip.** You move with your usual controls, through walls, along the
direction you are looking: look up and push forward to climb. God mode stays
on while noclip is, and for five seconds after, so landing is not a death.

## The readout

Three lines under the frame-rate counter:

```text
pos 12.34 1.80 -40.12  yaw 90 pitch -5  room 17  hp 40
aim 15.10 1.20 -38.00  3.6 away  character ob/chrs/chr47.xbr
GOD AMMO INVISIBLE NOCLIP FREECAM STOPPED
```

The first is where you are (or the free camera is), which way you face, the
room you are in and your health. The second is what the middle of the view is
on, from the game's own line-of-sight test (the one its bullets and enemies
use): the point, how far away, and whether it is a room's wall or floor, a
door, a character or a pickup, with the model it is drawn with. It gives no
texture: under RTX Remix, Remix's own developer menu shows the texture under
the cursor and its hash. The third names the tools that are on. With the
readout on, the log gets the same lines once a second (`[TS2-DEV] frame N:`).

## Scripted runs

A scripted run's window never has the focus, so it cannot press the keys.
`ts2_dev_seq` presses them instead, at frames counted from the first frame
the game presents:

```text
RECOMP_TS2_DEV_SEQ=4200:freeze,4210:freecam,4600:step,4700:freeze,4710:freecam
```

The actions are `readout god ammo invisible unlock save load freecam noclip
freeze step wireframe faster slower menu`. The free camera and noclip read controller 1, so
`RECOMP_INPUT_SEQ` moves them (`<ms>:<control>:<hold ms>`, for example
`90000:lstick_up:2000,93000:rstick_right:500,94000:rt:800`).
`ts2_teleport` and a saved position make a test start from the same spot
every time.

## How each one works

All addresses are the USA release's, in `config/us/devtools.inc`, each with
how it was found.

- **God mode and infinite ammo** are two of the developers' own switches,
  left in the retail game and reachable there only through a button
  combination on several controllers. With god mode set, the damage filter
  ignores damage to player-controlled characters and falls do no damage.
- **Unlock everything** is the developers' unlock switch, which makes every
  unlock condition pass, and their overrides per kind (story levels,
  challenges, arcade league, characters, cheats). The game asks again each
  time a screen is drawn, so nothing is written to the save. (The
  developers' combination also wrote every unlock into the save when the
  first profile was named DUFFINS. That code is never run from here.)
- **Stopping the clock** sets the game's own pause switch, the one its pause
  menu sets, without the menu. The world stops; drawing and the camera go
  on.
- **Game speed** works on the game's frame clock, which decides each frame
  how many whole steps of 1/60 s the game runs (one, or more to catch up
  after a slow frame). The count is scaled after it, with the remainder
  carried: 2x runs two steps a frame, 0.5x one every other frame, so slow
  motion moves in steps (frame interpolation smooths them). The game only
  ever runs whole steps, which is why this is not a way to a higher frame
  rate: frame interpolation is.
- **Invisible** works on the enemies' eyesight: an enemy counts its target
  as seen when its line-of-sight ray reaches it, and those rays (five call
  sites in the AI) reach nothing when they reach you. Bullets are untouched.
  Not yet covered: security cameras, alarms and sensors, and enemies that
  hear gunfire.
- **Noclip and teleport** answer the character sweep, the one function every
  one of the player's movement modes moves through, with the tool's
  position, so the game builds your eye, your character and the rest from
  it as from any step. The room you are in is found with the game's own
  portal walk.
- **The free camera** replaces the eye and direction the camera update hands
  its look-at function, at that one call, and sets the room drawing starts
  from by the same portal walk. The view and everything made from it
  follow, Remix's camera included.
- **The aim line** calls the game's ray test from the eye along the view, past
  your own character.

Not possible: a full save state. The game is native code running on real
threads with graphics objects behind it, so there is no snapshot to restore;
a saved position and `ts2_teleport` are the substitute.
