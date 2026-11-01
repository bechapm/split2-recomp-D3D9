# Enhancements: what is done and what is next

Tracking list for TimeSplitters 2. Each candidate has an estimate and a
split: **toolkit** work goes in xboxrecomp and every title gets it;
**TS2** work is this repository. The general list, and the reasoning behind
the split, is in `xboxrecomp/docs/technical/xbox-game-enhancements.md`.

Estimates are working time; calendar time will be longer. They date from
30 September 2026.

## Done

| Enhancement | Where | Notes |
| --- | --- | --- |
| Widescreen (any shape) | Both | `aspect_ratio` (launcher row Aspect ratio): 16:9, 16:10, 21:9, 32:9 or the screen's. Camera widened where the game builds it by the toolkit's (4/3)/aspect, so culling agrees; front end, HUD and every other screen laid out by call site, with edge pieces pinned to the picture's edges. CMakeLists.txt declares `recomp_title_widescreen(HOR_PLUS REGISTER 60)` so the launcher offers every shape. `RECOMP_TS2_MENUS=43` brings back the 4:3 front end. |
| Internal resolution | Toolkit | `resolution` = lines (480 to 8640, or `screen`), the width following the shape; `RECOMP_RES_SCALE=1..8` still works |
| Vulkan renderer | Toolkit | The renderer on macOS. On Windows `RECOMP_D3D8_BACKEND=vulkan`, which needs `third_party/dxc` (the DXC GitHub release) for the build |
| Launcher mouse input | Toolkit | Arrow hit boxes match what is drawn (xboxrecomp #24) |
| Variable refresh rate | Toolkit | Borderless fullscreen (Alt+Enter), VRR presents on both renderers; launcher rows Fullscreen and Variable refresh rate (xboxrecomp #29) |
| Frame interpolation | Both | `frame_interp = 2..4` (launcher row): 120+ frames a second shown while the game runs at 60. TS2 names its matrix registers (c60 projection, c64-c75 model-view and bones) in `src/recomp_manual.c`. See below |
| macOS (Apple Silicon) | Both | The toolkit's port; here, `src/main.c` from the portable template, the CMake project and `scripts/build.py` (Ninja). PAL plays to Siberia at 60 fps in a scripted run (13 Oct 2026) |
| Mods that stack | Both | `RECOMP_MODS=a:b:c` (toolkit) with loose files served from inside the paks at run time (TS2, PAL: `files/`, `paks/<pak>/`; `ts2_paks_*`), every mod's `ts2/characters.ini` merged, conflicts settled by order and logged. `tools/ts2mod.py loose/check`, and `ts2kitbash`'s output is a mod. See [modding.md](modding.md) |
| System Link | Both | Windows only. The toolkit emulates the network card and hosts sessions over a LAN or the internet; TS2 needed one seed. See below and [multiplayer.md](multiplayer.md) |

### Frame interpolation leftovers

- Snow and other particles the CPU positions step at 60 among 120.
- A 144 Hz display needs VRR for even pacing; a render thread would serve
  fixed refresh rates (toolkit, 3-4 weeks).
- Not yet checked: the first-person weapon and muzzle flashes while
  firing, guards animating (the bone matrices), the pause menu, a death.
- Measurements and how it works:
  `xboxrecomp/docs/technical/frame-interpolation.md`.

### System Link leftovers

- macOS: the toolkit's network card is Windows-only (toolkit work).
- The USA release: System Link not opened yet; it probably needs its own
  counterpart of PAL's seed `0x0009D530`.
- Playing against xemu or a console (needs the player's `XboxLANKey`), and
  XLink Kai: untested.
- Split screen over System Link: untested.

### Checking a screen's widescreen layout

How to survey a screen: F11 with `RECOMP_TS2_UI_SITES=1` set, then
`d3d8_replay --each-tag` / `--each-tag-hidden` on the capture and
`--place <site>=<placement>` to try a placement before it goes into
`config/<region>/overrides.inc`.

### How the 2D is laid out: anchors

This is the anchor model of any modern UI, retrofitted. An engine's widget says what
it is anchored to, and a web page's box has a flex or grid parent; the layout then
works out the position from the screen's shape. This game's code only says x = 520,
because it knew its screen, so the anchors are written down in
`config/<region>/overrides.inc`. Every position is still calculated, by one factor,
k = (4/3)/aspect, so every shape uses the same data and nothing is kept per
resolution.

```
 what the game drew, 640 wide     where it goes, any shape
 +-----------------+    +------------------------------------------------+
 | rank     radar  |    | rank                                     radar |
 |      panel      |    |                     panel                      |
 | ammo      ammo  |    | ammo                                      ammo |
 +-----------------+    +------------------------------------------------+
                         LEFT            CENTRE, SPREAD             RIGHT
                         <---------------- STRETCH ---------------------->
```

The anchor goes on the largest thing that moves together, and what is inside inherits
it. A site's placement is the first of these (`ts2_ui_place_for`):

| Level | Where | What it is for |
| --- | --- | --- |
| Exception | `k_ts2_ui_places` | One call site unlike the rest, as a full-screen fade or a page's arrow |
| Group | `k_ts2_ui_groups` | The code that draws a whole piece, by address range: a menu panel, the radar, each hand's ammo |
| Area | `ts2_ui_area_enter`, `ts2_ui_area_default` | All of a container, as its default for what is inside: while the pause pages draw (the code drawing them, not the game being paused, so the level around them keeps its anchors), and MapMaker's editor. A group marked `hud` gives way to it, so the gauge the Inventory page borrows from the HUD stays on the page |
| Default | `k_ts2_ui_default` | FILL, full-bleed: what crosses the whole width spans it, the rest stays in proportion. `RECOMP_TS2_UI_DEFAULT` for another |

A formula alone cannot do it: a position does not say what a piece is. The Select
Level panel and the ammo sit in nearly the same 4:3 spot, one to stay by the middle and
one to go to the corner. SPREAD, which scales each draw's 4:3 position with the width,
was tried as the default and cost a hand test: everything of more than one draw came
apart by 1/k, the health bars, the turret's display, and text from its outline, which
is five draws.

Every site gets a batch of its own (`RECOMP_TS2_UI_EACH_SITE=0` puts sites with the
same placement back in one), since the width guess measures a whole draw: two small
hints at opposite sides in one batch read as a wide piece and were stretched, flickering
as the batch's cut moved. Tagging flushes per site either way, so
`RECOMP_TS2_UI_SITES=1` hides a bug of this kind: check without it too.

What a whole widget draws should be one site: wrapping the function that draws it does
that (`sub_000C8F00`, the bars across the top of the HUD, part of which is a helper other
screens share). A security camera's frame is a group that spans the picture, with its
text anchored. Pinning its corners as a nine-slice leaves a seam down each side, since
the tint and scan lines are one piece that stretches and the corners would not, so its
corners are a little wider than they were.

A 3D model in a menu is part of the layout too: the character on Select Character, a
weapon on the Inventory page, the pause pages' ribbons. Each goes through a camera
built for the 4:3 screen, which nothing widens, so it was stretched across the picture
and off its panel. They need no list, since the camera says what they are: one narrower
than the picture was not made for it, where the world's is always as wide as the
picture. While the front end or the pause pages draw (`ts2_ui_models`), such a draw
stays at 4:3 (`XBOX_D3D8_2D_PANEL_3D`, the toolkit's widescreen doc). In a level it is
off, since a split screen's narrow view is the world, and the world stays Hor+.

The squeeze is a viewport, so a piece reaching past the 4:3 edge would stop dead
there. The menu panels' rays do, x -243 to 910 of 640, and they fade out by themselves
further on, so the game turns on the toolkit's overflow
(`xbox_D3D8SetTwoDOverflow`, `xboxrecomp/docs/technical/widescreen-and-resolution.md` 9).

## Candidates

| Enhancement | Toolkit | TS2 | Total | Confidence |
| --- | --- | --- | --- | --- |
| [Modern controller icons](#modern-controller-icons) | 2–3 weeks | ~1 week | 3–4 weeks (first milestone 1.5–2) | High to medium |
| [Native mouselook](#native-mouselook) | 1–2 weeks | 2–4 weeks | 3–6 weeks (+2–3 for mouse menus) | Medium to low |
| [Playable character models](#playable-character-models) | 1–2 days (+2–3 for new characters) | 4.5–7 weeks | 4.5–7 weeks to edit; +3–5 weeks to add characters | Medium to low |

The controller icons need texture replacement by content hash, which is
also the first step for texture mods.

---

### Modern controller icons

Replace the game's Xbox button icons with the matching modern set for the
pad in use: Xbox Series, PlayStation, Switch.

How TS2 draws them: each button is its own 32x32 texture (B, A, Y, X in
`textures/misc/xbstdbutton1-4.xbt`), plus a 128x64 atlas with Start, Back
and arrows. Every menu and dialog prompt is text with an inline icon code,
so about 560 strings in five languages go through those four textures. The
textures are byte-identical in every level pak, so one content hash
identifies each.

Approach: toolkit texture replacement by content hash, one replacement set
per pad family, following the pad used last (SDL3 gives its type). TS2
supplies a small manifest (hash → button, atlas layout).

| Scope | Toolkit | TS2 |
| --- | --- | --- |
| **First milestone:** Xbox Series and PlayStation, chosen by a setting | 4–6 days | 2–3 days |
| Full: live switching, Switch, the atlas, launcher text, follows bindings | 10.5–16 days | 3.5–6 days |
| Optional: per-player icons in split screen | 2–4 days | 1–2 days |
| Optional: controller-picture screens; button names written in text | – | 4–7 days (low confidence) |

To decide: the art (Kenney's Input Prompts are CC0 and cover all three;
CC0 waives copyright, not trademark, so whether glyph art ships in the repo
is a maintainer's call). Icons must follow the button's position and the
player's bindings, not the letter printed on it: a Switch pad's bottom button
reads "B" and acts as the game's A.

### Native mouselook

**Started 29 Oct 2026 (USA release):** the mouse adds to the view angles
directly after the game's own look function, `sub_00148A60`, scaled by the
field of view, with auto lookahead held off, not inverted by the game's
Inverse Look (`ts2_mouse_invert`), and only when the camera is the player's
eye (`src/ts2_mouse.c`, addresses in `config/us/mouse.inc`). While aiming,
the breathing sway (`sub_00140E10`, the angles at +0x188/+0x18C the camera
adds to the view) is held at 0, folded into the view angles as aiming
starts and taken back out as it ends, so the camera does not move at either
(`ts2_steady_aim`). The keys
are PC-shooter defaults on the Default control setup's actions
(`config/input_defaults.json`), the wheel zooms while looking through a scope and changes
weapon otherwise, and Escape pauses in a level and backs out of menus. On
a mounted gun (look's modes 7 and 8) the mouse turns the angles look's stick
turns before look runs, so the gun's arc holds it, and the debugging tools'
free camera turns by it too (`ts2_mouse_turn`). **The menus (30 Oct
2026):** while a menu is up (the front end, or the pause menu) the mouse is a
pointer (the toolkit's `recomp_pointer_want`): moving it onto an item of the
panel with the focus selects it as the panel's own update does (the item's
selected flag, the panel's row and column, the move sound), a click presses
A for it (or steps an option on), the right button is B and the wheel the
D-pad's up and down (`src/ts2_menu_mouse.c`, addresses in
`config/us/menu_mouse.inc`). The boxes are the game's own: each item's box
from its panel's corner, as the panel draws it (`sub_000F2040`). The pause
menu's Options and Quit pages have no boxes: their lines are 3D text on a
moving mesh, placed by the mesh's matrix and by a fit measured from the
picture, since the matrix that lays the text out is gone by the present
(`n_box_3d`). The pointer stays through the front end's gaps between screens
and for as long as the game is paused; a screen with nothing to select (Select
Controller, Enter Name, the Inventory page) takes a click as Start and the
right button as B. A click on an option's value steps it the way of the half
it is on (or of the arrow), laid out as `sub_000F1230` draws it; a click on a
button hint is its button, laid out as `sub_000EFEB0` draws them; a click on a
pause page's arrow (`sub_000D1A00`) is the D-pad's left or right. The hints and
values are measured with the game's own text width (`sub_001AC170`), called
from the present. In widescreen the arrows are pinned to the picture's edges,
outside the ring, and the pointer follows their placement. A click skips a
cutscene: in the front end with no screen up, and in a level while the camera
is not the player's eye and the player is not on a mounted gun
(`src/ts2_mouse.c`), as the button `RECOMP_TS2_SKIP_BUTTON` names. A, the
default, and Start skip the Siberia opening, its fly-over and the Introduction
in Options; B does not. **PC conventions (31 Oct 2026):** a click acts when
the button comes up over what it went down on, the right button does nothing
in a menu, Escape and Backspace go back, Space chooses, the pointer hides
while the keys or a pad drive the menu, and the hints name keys while the
keyboard and mouse are in use (`[Enter]`, `[Esc]`, and the keys bound to Y
and X, from the toolkit's `recomp_bindings_key_name`), a line that grows too
wide stepping down to the game's narrower item font. A hint under the
pointer is lit as the game lights its selected line: the pulsing colour
inside the style's deep blue outline, drawn as `sub_001ACA50` draws its own
(four passes around the line with `[0x0040BB10]` set). The pause pages'
button pictures (`sub_000AA360`) and D-pad picture (`sub_000AB2A0`) are
wrapped: a click on one is that button or way, and with the keys in use a
button's picture is its key's name. A panel's own scroll arrows
(`sub_000F0010`, the Character Select list's) are clickable, and a mirrored
sprite (the pause pages' left arrow) is measured with its corners in order.
**MapMaker (31 Oct 2026, `src/ts2_mapmaker_mouse.c`, addresses in
`config/us/mapmaker.inc`):** in the editor (level 0x66) the pointer is
written into the hand's position (`F+0x30`/`F+0x34`, 16.16) when the mouse
moves, the left button is A while held (place, lasso), the right button B
(delete), the wheel the triggers (zoom), and the pointer past the hand's
limits pushes the left stick (the game's own scrolling). Its screens are the
same hand over hot spots (`S+0x64C`, count `S+0x650`, the id under the hand
in `S+0x648`) or rows the pointer picks by writing that id. In widescreen
every site the placement table does not name is kept in proportion while
the editor runs (`ts2_ui_area_default`), since the grid was stretched and
the hand was not. Map Settings' and Editor Settings' value arrows (sprites
from `0x00119D49`, `0x00119DC7`) pick their row and are the D-pad's left
and right. The Mode question's hot spots are its title, the moving
selection bar, then its lines; only the lines are counted. **The letter
picker** (Enter Name, MapMaker's names, description and briefing; one module,
`sub_001AD350` draws it and so says it is up, state `0x0040BB38 + player *
0x40`) takes the pointer from the menu round it: the cell under it is
written as the picked one (6 by 6 cells, 24 by 20 from the box's corner +
(6, 4)) and a click is A. In a picker Backspace is X (delete) and Space Y
(a space), as in a text box, and MapMaker's "Add  Cancel" line under its
grid (`0x001ADD2C`) steps down to font 0x11 to fit its box with the keys
named. Typing letters on the keyboard is not done: the game's window gets no
text input yet (a toolkit change). Not yet: the PAL release. The plan below
is how it looked before.

The toolkit has no mouse input today.

| Step | Where | Estimate |
| --- | --- | --- |
| Mouse input: raw input, cursor lock in game and release in menus and on alt-tab, sensitivity and invert, mouse buttons and wheel as bindable controls, launcher UI | Toolkit | 1–2 weeks |
| Find TS2's view angles and where stick input becomes turn rate (the in-game camera, `sub_00032DC0` / `0x4B2CA0` in PAL and `sub_00032D90` / `0x4B2F60` in USA, is the starting point) | TS2 | 1–2 weeks |
| Apply the mouse to the view angles directly: pitch limits; auto-aim, look-spring and aim smoothing off for the mouse | TS2 | 3–5 days |
| Edge cases: aim mode, scope zoom, turrets, cutscenes and pause, respawn, split screen (player 1 only) | TS2 | 1–2 weeks |
| Optional: clicking TS2's menus with the mouse | TS2 | 2–3 weeks |

A cheaper option, the mouse as a virtual right stick (1–2 weeks, toolkit
only), does not feel like mouselook: the game's stick acceleration, dead
zone and top turn speed still apply.

### Playable character models

Edit the playable characters in Blender and put them back into the game,
then add new ones to the roster. **How to do it: [modding.md](modding.md)**
(editing, new characters from two existing ones, from Blender, or low-poly
from boxes; adding them to the roster; keeping private ones out of git). Estimated 15 October 2026. The format
research behind it is in
`xboxrecomp/docs/technical/modding-models-textures.md`.

What makes it feasible:

- `.pak` files are uncompressed and community tools already read them;
  `.xbt` textures are plain DXT1/DXT3 behind a 128-byte header.
- A character piece is drawn with at most three matrices (c64-c75, as
  measured for frame interpolation): no large bone palette.
- The XBE names three skeletons: `anim/data/ts2/human_19_bindpose.raw`,
  the rail spider's and the barrel's. Neither of the last two is playable,
  so every playable character probably shares the 19-bone human skeleton
  and every animation (inferred from the strings, not yet checked against
  the models). The decomp names the loader (`bindpose_slot_load`,
  `sub_00012C30`).
- Nobody has written an `.xbr` before; the decomp's model loader is the
  specification.

| Step | Where | Estimate |
| --- | --- | --- |
| A `mods/` folder read before the disc's `.pak` files | Toolkit | **Done:** `RECOMP_MODS` (xboxrecomp `feat/mods-overlay`; macOS checked, Windows not built yet) |
| `.pak` unpack and repack (or `tspak`) | TS2 | **Done:** `tools/ts2pak.py`, all 68 paks round-trip byte for byte |
| Write a character's files into every pak that holds them (see below) | TS2 | **Done:** `tools/ts2mod.py build`, checked in Siberia; and since 15 Oct 2026 at run time instead, from a mod's `files/` (a recolour of the Siberian guards' textures, as loose files, drew in Siberia) |
| **Mods stack:** loose files inside the paks served at run time from every `RECOMP_MODS` folder, new names added to a pak's directory, every mod's `characters.ini` merged | TS2 (+ toolkit `xbox_mods_walk`) | **Done (PAL):** two `ts2kitbash` mods (two characters, one) and a texture mod together: all three characters on Select Character and playable, the recoloured guards in Siberia; each alone, and both orders; a mod's loose files on top of another's whole paks (scripted, macOS) |
| `.xbt` to and from DDS/PNG | TS2 | **Done:** `tools/ts2xbt.py`, all 19,180 textures round-trip |
| The whole `.xbr` format from the game's loader: geometry sections, piece-to-bone binding, the 2–3 matrix blend, materials, LODs | TS2 | **Done:** `tools/ts2xbr.py`, `docs/formats/xbr.md`; every character file on the disc rewrites byte for byte |
| Blender add-on: import `.xbr` and the human skeleton, export back, triangle strips | TS2 | **Done as glTF:** `tools/ts2gltf.py` (`gltf` / `fromgltf`), round-tripped through Blender 5.2; all 314 `chr.pak` models regenerated through glTF drew and animated in an arcade match. The Blender add-on (`tools/blender/ts2_character`) wraps it in File > Import/Export; `graft` combines two characters' pieces, and new textures get ids from 8000 (`docs/formats/xbr.md`, Editing in Blender) |
| Low-detail models (`chrNNlod2.xbr`) and hat and head attachment points | TS2 | 3–5 days |
| Checking in game: animation, hit detection, split screen | TS2 | ~5 days |
| **A character in the spare roster slot**, declared by a mod (`ts2/characters.ini`): name, portrait, description, unlock, stats and voice | TS2 | **Done (PAL):** Agent Static (chr id 70) on Select Character, in matches, in the bot list and the Gallery (15 columns); `docs/formats/roster.md`, "Adding a character (mods)" |
| **Characters added, not swapped:** the roster grown in the XBE before the lift (xboxrecomp XBE patches; `config/pal/xbe_patches.json`): 32 hidden spare slots, the unlock snapshot and the portraits' texture list with them, 78 code sites repointed; `slot = next` in `ts2/characters.ini`, and `tools/ts2kitbash.py` writes that file from its recipe | TS2 | **Done (PAL):** Agent Static, The Bone Ranger and Monkey Business added beside all 126 characters: Select Character, a match each, the Gallery (15 columns); no mod, no change (frame dumps against the unpatched build); `docs/formats/roster.md`, "Room for more characters" |
| More than 35 new characters (new chr ids: the definition, flags and object tables grow too), new characters in story levels | TS2 | 2–3 weeks |
| **To add characters:** System Link refuses players whose mods differ | Toolkit | 2–3 days |

First milestone (about 2.5 weeks): the first four steps, ending in a
hand-edited `.xbr` that still loads and animates. It settles the format
risk before the Blender work. **Reached 15 October 2026:** every human
model in `l_35_ST.pak` with its head piece scaled 5x, written by
`ts2xbr`'s writer and installed with `ts2mod`, drew in Siberia with the
big head and kept animating (scripted run, macOS, frame dumps).

What the format work changed in the plan:

- All 238 human models (155 characters, 83 lod2) share one 19-bone
  skeleton; its joint positions are `fe_anim.pak`
  `human_19_bindpose.raw`, scaled per model. Exceptions: the Barrel Robot
  (13 bones), three story bosses, and one-bone hats and detachable parts.
- No blend weights: a skinned vertex names one of three bones (its
  piece's, the parent's or the child's), so a mesh weight-painted in
  Blender is snapped to one bone per vertex on export.
- `chrinc.pak` holds the character-select previews: the same model with
  its textures inside the file.

Known limits:

- Story enemies are drawn with the same models, so editing a character
  changes it in story mode too. Only a new slot avoids that.
- A level does not draw its characters from `chr.pak`. Every story and
  arcade level pak carries copies of the characters in it (byte-identical
  to `chr.pak`'s, textures under the same global `textures/NNNN.xbt`
  numbers; `chr01.xbr` is in 22 paks, one file is in 24), so a character
  edit is written into each of them. Checked: red textures in `chr.pak`
  alone changed nothing in Siberia; in `l_35_ST.pak` the guards drew red.
- Cutscenes are in-engine but use their own models, skeletons and
  animations (`cs_l_*.pak`, e.g. `anim/data/cs/l_35/spacemarine.xbr` with
  `spacemarine_m0_bind.raw`), so edits do not show in them.

The roster (`docs/formats/roster.md`, PAL): 127 entries of 0x20 bytes at
`0x002BAC60` (chr id, portrait, unlock rule), shown as one 14 x 9 grid of
126 cells; 162 character definitions at `0x002AA060`; which characters a
level loads is a table in the XBE (`0x002CE928`), and a level's pak must
hold their files. Unlocks are worked out from story and medal bits, not
stored per character, so a new character needs no save change. Two ways
to add one:

- Reuse one of the 35 non-playable ids (pedestrians, robots, spare
  copies): give it new files and a roster entry. **Done for one (15 Oct
  2026):** roster entry 126 (Consultant, never shown) is the one spare slot;
  a mod's `ts2/characters.ini` turns it into a chr id with new files, and
  hooks patch the name, portrait and Gallery grid at start-up. A second
  character this way takes over a playable one's slot. The Select Character
  screen is not the 14 x 9 grid: it lists every unlocked roster entry, so it
  needed no change.
- A new id beyond 162: every table is a fixed array with other data after
  it, so either the tables move or the 15 functions that read the roster
  are overridden.

Afterwards:

- [ ] **Cutscenes:** carry an edited character into the cutscene versions:
  export onto each cutscene model's own skeleton, check it in every
  cutscene it appears in. Same `.xbr` tools. About 1–2 weeks for the first
  cutscene, less for each after. Only `cs_l_35.pak` has been looked at.

## Not estimated yet

- **Cutscene skip:** find where in-engine cutscenes start and whether the
  game already has a skip.
- **Texture mods:** naming dumped textures by pak entry; shares the
  replacement work with the controller icons.
- **MSAA and filters:** toolkit work; TS2 only needs exceptions for passes
  that break.
