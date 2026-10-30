# TimeSplitters 2 under RTX Remix

What is known about running this recomp under RTX Remix and authoring for it
in the RTX Remix Toolkit. The renderer side (the D3D9 backend and what it tells
Remix) is general and documented in the toolkit:
[d3d9-remix.md](../xboxrecomp/docs/technical/d3d9-remix.md) and
[d3d9-parity.md](../xboxrecomp/docs/technical/d3d9-parity.md). This page is
only about TimeSplitters 2.

## Running

`RECOMP_D3D8_BACKEND=d3d9`, with Remix's `d3d9.dll` and runtime beside the
executable. Measured with Remix 1.6.1.

`rtx.conf` beside the executable:

```
rtx.uiTextures = 0xA2F17C4A8F3E3CEA, 0xC1DD87A9D3601983
rtx.minimizeBlasMerging = True
```

The first marks the recomp's 2D as UI. The second keeps the first-person
weapons and gadgets in place while the camera turns: their meshes are under
Remix's merge threshold, and the merged structure kept them where an earlier
frame had put them (missing, shifted or mirrored).

## Capturing for the Toolkit

Remix's capture needs `rtx.enableReplacementAssets = False`. A capture holds
the world in metres, Y up, every mesh in its place, and the camera. A Hospital
capture is about 5 MB; a Siberia one about 20 MB, plus a 64 MB sky probe.

## Hashes

Stable across runs, across a level restart, for first-person weapons wherever
they are drawn, and for characters while they animate (the Siberia intro: the
same 80 parts at different poses). Characters come through as 60 to 80 rigid
parts, not skinned meshes: TS2 animates in its vertex programs, which Remix
reads by vertex capture.

What does change hash, and is better replaced by texture:

- Geometry rebuilt every frame, textured with `0x2CEDA9EB716F8AF1` or
  `0xAFD5CC08D69BF90D`.
- Screens that animate by swapping textures (Siberia): each frame's texture
  is stable, so every frame needs replacing.

## Lighting

TS2 has no light sources Remix can use: its lighting is painted on, as
alpha-blended overlay textures, and baked into vertex colours (which matter
little under Remix). The per-draw ambient colour and two nearby point lights
the game hands its vertex programs are fill lights relative to each object,
not lamps. Real lights are to be placed in the Toolkit, using the captures,
where the lamp fixtures and their textures sit at their real positions.

The painted light comes off with `rtx.ignoreTextures`. Overlay textures found
so far, each tested alone:

| Texture | Where | What it paints |
|---|---|---|
| `0xA73D3632365DB83B` | Hospital | light pools (arches) on walls and floor |
| `0x994A27C470F49A92` | Hospital | lamp halos; a whole-wall shading layer in the corridor |
| `0xBFC1A327B8342884` | Hospital | light strips on floor and ceiling beams |
| `0x039ECF99995C1202` | Hospital | light cones under the wall lamps |
| `0xED7ECAF1D0FF6128` | Hospital | glow around the wall lamps |
| `0xDBD4E50D17FA9CAB` | Hospital ward | the light pool around a wall lamp |
| `0x87A7565156FAA8BE` | Hospital ward | a light patch across a bed |
| `0x7D7BE638AE0A6DE6` | Training Ground | darkening and colour on the bunker walls (shading, more than light) |
| `0x6CEA4B2DB68D770A` | Training Ground | a shadow patch on the ground |
| `0xA89A78CFADE9DA65` | Training Ground | light strips along floor edges |

Alpha-blended textures seen with no visible effect yet: `0x31875C465EF98FF5`,
`0x6431131A6E19C425` (a small streak), `0x2CEDA9EB716F8AF1`,
`0xA005BBB927E81283`. Not drawn in any frame tested: `0xA26C34DC7E95E55F`,
`0x5AD7B1DF8996DA68`, `0x5704892543EEEA1D`, `0xC724EA3C919BE0DB`.

## Effects

No Remix texture category reproduces the original look of these, so they are
for Remix's particle system or replacement materials:

| Texture | What |
|---|---|
| `0xC66178159CAC85A6` | spawn and teleport shield and beams (white volumes under Remix) |
| `0x171AEF0AF28D0347` | the shield's glow sprites |
| `0x2F932195103CB701`, `0x381B4B1E13894A11` | bullet tracers (hard white streaks) |

## Not a problem

- The orange and blue crescents after being hit are the game's own
  damage-direction indicator; D3D11 draws them too.
- A weapon or a level that is black under Remix at night or indoors is unlit,
  not missing.
