/*
 * ts2_devtools.h -- TimeSplitters 2's debugging tools (src/ts2_devtools.c).
 *
 * The hooks each release's config/<region>/overrides.inc wraps the game's
 * functions with, and the table of where that release keeps what the tools
 * read and write (config/<region>/devtools.inc). Nothing here is an address.
 */
#ifndef SPLIT2_TS2_DEVTOOLS_H
#define SPLIT2_TS2_DEVTOOLS_H

#include <stdint.h>

/* Where one release keeps what the tools use. 0 means not found in that
 * release, and every tool that needs it stays off there. */
typedef struct Ts2DevAddrs {
    uint32_t player_base;     /* dword: player 1's record (P) */
    uint32_t level_id;        /* dword: the level now loaded */
    uint32_t level_menus;     /* the first value that is not a playable level (the front
                               * end, loading, the screens between: this and above) */
    uint32_t god;             /* dword: the developers' god mode (player-controlled characters) */
    uint32_t infinite_ammo;   /* dword: the developers' infinite ammo */
    uint32_t unlock_all;      /* dword: every unlock condition passes */
    uint32_t unlock_override; /* bytes: per-kind unlock overrides, unlock_override_count of them */
    uint32_t unlock_override_count;
    uint32_t pause;           /* int: the game's pause: 0 running, >0 paused, -1 resuming */
    uint32_t ticks;           /* dword: ticks the game runs this frame (the frame clock's) */
    uint32_t time_scale;      /* float: the same, as the time scale */
    uint32_t context;         /* dword: the camera/render context */
    uint32_t look_at_return;  /* the camera update's call of look_at: its return address */
    /* room_walk(float *from, int from_room, float *to, 0): the room that
     * holds `to`, found by walking the portals from `from` in from_room. */
    void (*room_walk)(void);
    /* ray(room, float *start, float *delta, infinite, float *hit, float
     * *normal, int *room_out, class_mask, first_hit, passed): the object hit
     * along start..start+delta (0 for none), the point, the surface normal
     * and the room it ended in; `passed` is filled with what the ray went
     * through on the way (a count, then up to five records of 0x30). */
    void (*ray)(void);
    uint32_t object_list;     /* the model list: 0x24 bytes each, a name pointer first */
    /* The return addresses of the enemies' "can I see my target" rays: the
     * ones invisibility answers "nothing" for when they hit the player. */
    uint32_t sight_returns[8];
    /* give_item(R, item), cdecl: what picking an item up does -- it is owned
     * from then on, its name is shown and the pickup sound plays. */
    void (*give_item)(void);
    /* fill_ammo(R, item), cdecl: the item's ammunition to its maximum. */
    void (*fill_ammo)(void);
    uint32_t item_count;      /* items in the game's item table */
    uint32_t item_names;      /* word: item 0's name, as a text id */
    uint32_t item_name_stride;
    uint32_t text_table;      /* the English text: a string pointer per text id */
    uint32_t text_count;
    /* room_at(float *pos, float up, float half), cdecl: the room that holds
     * `pos`, found from the rooms' bounding boxes and the floor below it, not
     * by a walk; 0 or less for none. */
    void (*room_at)(void);
    uint32_t player_count;    /* dword: how many players; their character indices are 0..n-1 */
    /* What an item needs loaded: the item table (a left and a right weapon
     * type first, -1 for none), the weapon definitions (the first-person
     * and the third-person model first), each model's loaded data in the
     * model list (object_list's entries, at this address), and the two
     * lookups that decide whether the game finds it at all: the resident
     * resources, and the mounted paks' P4CK and P8CK directories (a mod's
     * files among them), each (path), cdecl, 0 for absent; the directory
     * lookups leave the record they found it in at pak_current, which is
     * kept. A model in none of them is not on the disc at all, and giving
     * its item crashed. */
    uint32_t item_table, item_stride;
    uint32_t weapon_defs, weapon_def_stride, weapon_type_count;
    uint32_t model_loaded;
    void (*resident_find)(void);
    void (*pak_find_p4)(void);
    void (*pak_find_p8)(void);
    uint32_t pak_current;
    /* A model loaded mid-level from a file the level did not carry gets no
     * textures: the game binds a model's textures (each material's id
     * replaced by its slot in the loaded textures) only for models in the
     * level's resident data, at level load. These bind them as the game's
     * own run-time binder does: pak_data(path) the file's data, cdecl;
     * material_prepare(list, data); then for each material record (0x10
     * bytes, id first, flags at +0xC, until an id of -1) texture_load(id,
     * rec, rec), or texture_load_alt(...) when the flags have 0x10000000. */
    void (*pak_data)(void);
    void (*material_prepare)(void);
    void (*texture_load)(void);
    void (*texture_load_alt)(void);
    uint32_t model_materials; /* a loaded model: its material list */
    /* The named textures (the HUD's: crosshairs, ammunition icons), a table
     * of 0x10-byte entries, name at +8 and handle at +0xC (after a level
     * load: -1 if the level never asked for it, else loaded). A gun's are
     * its ammunition types' (a weapon definition's +0x10 and +0x18): in the
     * ammunition table (ammo_hud + type * ammo_stride) the first dword is one
     * index, then up to four 0x20-byte entries (until +0 is 0) give two more
     * each at +4 and +8 (-1 none). One the level never asked for drew as a
     * white rectangle over the view. Loaded as a level load does:
     * named_request(i), cdecl, on a handle set to -2, adds its record;
     * rec = named_record(name); named_load(name, [rec+0x10], rec+0x14);
     * the handle is then [rec+0x14]. */
    uint32_t named_textures, named_count;
    void (*named_record)(void);
    void (*named_load)(void);
    void (*named_request)(void);
    uint32_t ammo_hud, ammo_stride, ammo_count;
    /* cloak_on(status), cloak_off(status), cdecl: the game's cloak, with no
     * time limit, on a character's status block. */
    void (*cloak_on)(void);
    void (*cloak_off)(void);
    /* The rooms the game draws (room culling, which the free camera and
     * noclip turn off): the room table, a pointer (room * 0xB0; +4 its
     * portal list: a count, then that many portal numbers), the number of
     * rooms (numbered 1..n), the portal table, a pointer (a pointer per
     * portal; +0 and +4 the two rooms it joins), and the screen's rectangle
     * the game draws into, a pointer (ints: +0 left, +4 right, +0x14 top,
     * +0x18 bottom). */
    uint32_t rooms, room_count, portals, viewport;
    /* Each room's objects (object culling): a count per room at obj_counts
     * + room * 4 and a pointer to that many object pointers at obj_lists +
     * room * 4, the lists the room draw walks (sub_0014E6F0). Before it, the
     * object test (sub_0014E0F0, wrapped) marks each object of a room culled
     * when its box is behind the camera or off its rooms' part of the screen. */
    uint32_t obj_counts, obj_lists;
} Ts2DevAddrs;

/* The player record (P), the character object (C = [P + chr]) and the
 * camera context: the same in every release found so far, so they are here
 * rather than in each table. */
enum {
    TS2_P_INDEX     = 0x000,  /* the character's index: the sweep's first argument */
    TS2_P_POS       = 0x0F0,  /* x, y, z; y is up */
    TS2_P_EYE       = 0x0FC,  /* the eye, made from the position after each move */
    TS2_P_VIEW_DIR  = 0x108,  /* the view direction, a unit vector */
    TS2_P_YAW       = 0x148,  /* degrees; turning right lowers it */
    TS2_P_PITCH     = 0x14C,  /* degrees */
    TS2_P_CHR       = 0x1BC,  /* C */
    TS2_P_ROOM      = 0x87C,  /* the room the player is in */
    TS2_P_ROOM2     = 0x880,  /* the same again: a spawn sets both */
    TS2_O_ROOM      = 0x014,  /* any object: its room */
    TS2_O_MODEL     = 0x008,  /* any object: its index in the model list */
    TS2_O_CLASS     = 0x00C,  /* 2 room, 4 door, 8 character, 0x40 pickup */
    TS2_O_FLAGS     = 0x01C,  /* bit 31: skipped by the ray's object test */
    TS2_C_POS       = 0x058,
    TS2_C_HEALTH    = 0x19C,  /* float */
    TS2_C_RECORD    = 0x124,  /* R: the character's record (items, ammo, armour) */
    TS2_R_ITEM      = 0x0C8,  /* the item in hand */
    TS2_R_OWNED     = 0x984,  /* a dword per item: owned */
    TS2_R_TARGET    = 0x1300, /* an enemy's target: a character object */
    TS2_R_STATUS    = 0x1498, /* its status block: +4 flags */
    TS2_STATUS_CLOAK = 0x10,  /* in those flags: cloaked */
    TS2_CTX_WORLD   = 0x180,  /* the camera's world matrix: right, up, forward, eye rows */
    TS2_CTX_EYE     = 0x27C,  /* the eye the camera update hands the look-at */
    TS2_CTX_ROOM    = 0x294,  /* the room drawing starts from */
    TS2_CTX_FORWARD = 0x2E4,  /* the forward it hands the look-at */
    /* What the visibility pass leaves for the draw: per room, 12 bytes at
     * TS2_CTX_SEEN + room * 12 -- how many portals away it was found (int,
     * -1 not seen), then its rectangle on screen (shorts: left, top, right,
     * bottom) -- and the deepest of those at TS2_CTX_SEEN_DEEPEST. */
    TS2_CTX_SEEN    = 0x358,
    TS2_CTX_SEEN_DEEPEST = 0xCC4,
    TS2_ROOM_STRIDE = 0xB0,   /* the room table */
    TS2_ROOM_PORTALS = 0x004, /* a room: its portal list */
    TS2_ROOMS_MAX   = 200,    /* the room tables' size */
    TS2_SEEN_DEPTH_MAX = 0x4F,/* the draw sorts rooms into 80 depths */
    /* An object in a room's list (code: sub_0014E0F0, sub_0014E6F0,
     * sub_0001D860): the room it is listed in, flags, how many rooms it
     * touches (0: none found, so no rectangle and never drawn). */
    TS2_OBJ_ROOM    = 0x014,
    TS2_OBJ_FLAGS   = 0x01C,
    TS2_OBJ_ROOMS   = 0x0F4,
    TS2_OBJ_CULLED  = 0x800000, /* in TS2_OBJ_FLAGS: not drawn this frame */
    TS2_OBJ_CLIP    = 0x400000, /* in TS2_OBJ_FLAGS: drawn with clipping */
    TS2_OBJ_LISTS   = 224,      /* rooms' lists, and a few of the game's own */
};

/* Once a frame, after the game's present: keys, switches, the readout. */
void ts2_dev_frame(void);

/* The character sweep (wraps the release's sweep(index, from, to, ...),
 * which moves `to` back out of whatever is in the way): for the player,
 * noclip and a teleport put `to` where they want it instead. Every one of
 * the player's movement modes goes through it. */
void ts2_dev_sweep(void (*sweep)(void));

/* The game's frame clock (wraps the release's clock, which decides how many
 * ticks the frame runs): the game's speed scales the count. */
void ts2_dev_clock(void (*clock)(void));

/* The ray test (wraps the release's ray): under invisibility, an enemy's
 * sight ray that reaches the player reaches nothing. */
void ts2_dev_ray(void (*ray)(void));

/* The camera's look-at (wraps the release's look_at(dst, eye xyz,
 * forward xyz, up xyz), all by value): the free camera's eye and direction
 * at the camera update's own call. */
void ts2_dev_look_at(void (*look_at)(void));

/* The visibility pass (wraps the release's, which takes no arguments and
 * decides, from the camera's room and eye, which rooms are drawn and where
 * on screen): with the free camera on it judges from the camera's eye, not
 * the player's, and with room culling off (by default while the free camera
 * or noclip is on) every room is drawn. */
void ts2_dev_visibility(void (*visibility)(void));

/* The object test (wraps the release's per-room object test, cdecl, the
 * room's list as its one argument): with object culling off, an object it
 * marked culled as off screen or behind the camera is drawn anyway. */
void ts2_dev_objects(void (*test)(void));

/* The in-game HUD (wraps the release's HUD draw, which takes no arguments
 * and draws all of it: health and armour, ammo, the radar, the crosshair,
 * messages): not drawn while the tools hide it. */
void ts2_dev_hud(void (*hud)(void));

/* The enemies' awareness of each other and of the players: set(a, b,
 * state) and get(a, b) by character index, and an enemy's check that its
 * target is still one (check(R, refresh)). Under invisibility nothing
 * becomes aware of a player, a player reads as unnoticed, and an enemy
 * whose target is the player has none. */
void ts2_dev_aware_set(void (*set)(void));
void ts2_dev_aware_get(void (*get)(void));
void ts2_dev_target_check(void (*check)(void));

/* The model loader (wraps the release's load(path), which returns the
 * model): a model that is not in the level's resident data -- a gun the
 * tools gave from another level's data -- has its textures bound. */
void ts2_dev_model_load(void (*load)(void));

/* The read of a whole file into a buffer of its own (wraps the release's
 * read(path, ...), which returns the buffer): the model loader's fallback
 * when the file is not held whole in a pak's memory, as a mod's file is
 * not. Inside the model loader, the buffer is the model's data. */
void ts2_dev_file_read(void (*read)(void));

/* What a security camera or wall gun sees (wraps the release's sees(chr),
 * with the remote in esi): under invisibility, not the player. */
void ts2_dev_remote_sees(void (*sees)(void));

/* Whether every gun goes into every level from the game's own data
 * (src/recomp_manual.c): the tools are on and this release can give one. */
int ts2_dev_gives_guns(void);

#endif
