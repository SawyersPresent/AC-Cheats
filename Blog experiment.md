# AssaultCube Struct Reference
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h


# ═══════════════════════════════════════════════════════
# OFFSET CALCULATION RULES — READ THIS FIRST
# ═══════════════════════════════════════════════════════

a type must start at an address that is a multiple of its own size

```
bool  = 1 byte → can start anywhere
char  = 1 byte → can start anywhere
short = 2 bytes → must start at even address (divisible by 2)
int   = 4 bytes → must start at address divisible by 4
float = 4 bytes → must start at address divisible by 4
```

how to apply it step by step

```
1. start at offset 0
2. place each field one by one
3. before placing a field ask:
   is my current offset divisible by this field's size?
4. if yes → place it there
5. if no → add padding bytes until it is, then place it
6. add the field's size to get the new current offset
7. repeat
```

quick padding formula

```
current_offset % field_size == 0  →  no padding needed, place it here
current_offset % field_size != 0  →  add (field_size - remainder) bytes of padding

example:
   offset 103, next field is int (4 bytes)
   103 % 4 = 3  →  need (4 - 3) = 1 byte padding
   int goes at 104
```

type sizes reference

```
C++ type        bytes    CE type       notes
─────────────────────────────────────────────────────
bool            1        Byte          0=false 1=true
char            1        Byte          signed -128 to 127
uchar           1        Byte          unsigned 0 to 255
short           2        2 Bytes
int             4        4 Bytes
float           4        Float         positions, angles
double          8        Double
pointer*        4        Pointer       32-bit process
enet_uint32     4        4 Bytes       same as unsigned int
vec             12       3× Float      x y z components
vec[N]          12×N     N× Float
int[N]          4×N      N× 4 Bytes
bool[N]         1×N      N× Byte       pack together no padding
animstate       20       (unknown internals)
string          260      String        typedef char[MAXSTRLEN] — NOT 16 bytes
                                       see NON-STANDARD note under playerent name field
─────────────────────────────────────────────────────
```

vtable ptr rule

```
any class with virtual keyword → 4 bytes injected at offset 0
one ptr per class regardless of how many virtual functions
no virtual keyword anywhere → no vtable ptr, first field is at offset 0
```

inheritance rule

```
parent fields ALWAYS come first in memory
child fields are appended after
left-to-right order for multiple inheritance

playerent : public dynent, public playerstate
→ dynent (includes physent) first
→ playerstate second
→ playerent own fields last

the chain:
physent → dynent → playerstate → playerent → botent
offset 0 → 128  → 232        → 440       → 1032
```

non-standard type rule

```
before assigning a size to ANY non-primitive type
ctrl+F for typedef, struct, class, #define to find actual definition
never assume. the string mistake cost 244 bytes on every field after name.

lesson learned: string name was assumed to be 16 bytes
actual definition in tools.h: typedef char string[260]
everything after name was shifted by 244 bytes until CE confirmed it
```


# ═══════════════════════════════════════════════════════
# PHYSENT — offset 0x000, size 128 bytes
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L105
# this is ALWAYS offset 0. the object base. root of everything.
# ═══════════════════════════════════════════════════════

```cpp
class physent
{
public:
    // virtual ~physent() exists → vtable ptr injected at offset 0
    // 0x000 (0) → vtable ptr (4 bytes)

    vec o, vel;
    // ── NON-STANDARD SIZE — READ THIS ──
    // vec = struct { float x, y, z } = 3 × 4 = 12 bytes
    // source: https://github.com/assaultcube/AC/blob/master/source/src/geom.h#L2
    // 0x004 (4)  → o.x    (float) origin X position
    // 0x008 (8)  → o.y    (float) origin Y position
    // 0x00C (12) → o.z    (float) origin Z position
    // 0x010 (16) → vel.x  (float) velocity X
    // 0x014 (20) → vel.y  (float) velocity Y
    // 0x018 (24) → vel.z  (float) velocity Z

    vec deltapos, newpos;
    // movement interpolation
    // 0x01C (28) → deltapos.x
    // 0x020 (32) → deltapos.y
    // 0x024 (36) → deltapos.z
    // 0x028 (40) → newpos.x
    // 0x02C (44) → newpos.y
    // 0x030 (48) → newpos.z

    float yaw, pitch, roll;
    // 0x034 (52) → yaw
    // 0x038 (56) → pitch
    // 0x03C (60) → roll

    float pitchvel;
    // 0x040 (64) → pitchvel

    float maxspeed;
    // 0x044 (68) → maxspeed

    int timeinair;
    // 0x048 (72) → timeinair — used for fake gravity

    float radius, eyeheight, maxeyeheight, aboveeye;
    // bounding box size
    // 0x04C (76) → radius
    // 0x050 (80) → eyeheight
    // 0x054 (84) → maxeyeheight
    // 0x058 (88) → aboveeye

    bool inwater;
    // 0x05C (92) → inwater

    bool onfloor, onladder, jumpnext, jumpd, crouching, crouchedinair, trycrouch, cancollide, stuck, scoping;
    // 10 bools, 1 byte each, pack together with no padding between them
    // 0x05D (93)  → onfloor
    // 0x05E (94)  → onladder
    // 0x05F (95)  → jumpnext
    // 0x060 (96)  → jumpd
    // 0x061 (97)  → crouching
    // 0x062 (98)  → crouchedinair
    // 0x063 (99)  → trycrouch
    // 0x064 (100) → cancollide
    // 0x065 (101) → stuck
    // 0x066 (102) → scoping
    // 0x067 (103) → [1 byte padding — 11 bools end at 103, next field int
    //                needs 4-byte align, 103 % 4 = 3 → 1 byte pad]

    int lastjump;
    // 0x068 (104) → lastjump

    float lastjumpheight;
    // 0x06C (108) → lastjumpheight — defaults to 200 in constructor

    int lastsplash;
    // 0x070 (112) → lastsplash

    char move, strafe;
    // 0x074 (116) → move    — -1, 0, or 1
    // 0x075 (117) → strafe  — -1, 0, or 1

    uchar state, type;
    // 0x076 (118) → state  — CS_ALIVE=0 CS_DEAD=1 CS_SPAWNING=2 etc
    // 0x077 (119) → type   — ENT_PLAYER=0 ENT_BOT=1 ENT_CAMERA=2

    float eyeheightvel;
    // 0x078 (120) → eyeheightvel

    int last_pos;
    // 0x07C (124) → last_pos
    // physent ends at 0x080 (128)

    // constructor defaults (useful for knowing expected values):
    //   yaw=270, pitch=0, roll=0, pitchvel=0
    //   cancollide=true, stuck=false, scoping=false
    //   lastjump=0, lastjumpheight=200, lastsplash=0
    //   state=CS_ALIVE, last_pos=0

    // virtual functions (no data fields):
    //   virtual ~physent()
    //   virtual void oncollision()
    //   virtual void onmoved(const vec &dist)
};
```


# ═══════════════════════════════════════════════════════
# DYNENT — own fields start 0x080, size 232 bytes total
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L156
# inherits physent. physent block sits above starting at offset 0.
# ═══════════════════════════════════════════════════════

```cpp
class dynent : public physent
{
    bool k_left, k_right, k_up, k_down;
    // 0x080 (128) → k_left
    // 0x081 (129) → k_right
    // 0x082 (130) → k_up
    // 0x083 (131) → k_down

    animstate prev[2], current[2];
    // ── NON-STANDARD SIZE — READ THIS ──
    // animstate is a custom struct — internals unknown from headers alone
    // size confirmed empirically as 20 bytes per instance
    // 4 instances total = 80 bytes
    // 0x084 (132) → prev[0]    20 bytes
    // 0x098 (152) → prev[1]    20 bytes
    // 0x0AC (172) → current[0] 20 bytes
    // 0x0C0 (192) → current[1] 20 bytes

    int lastanimswitchtime[2];
    // 0x0D4 (212) → lastanimswitchtime[0]
    // 0x0D8 (216) → lastanimswitchtime[1]

    void *lastmodel[2];
    // void* = generic pointer, 4 bytes each in 32-bit
    // 0x0DC (220) → lastmodel[0]
    // 0x0E0 (224) → lastmodel[1]

    int lastrendered;
    // 0x0E4 (228) → lastrendered
    // dynent ends at 0x0E8 (232)

    // virtual functions (no data fields):
    //   virtual ~dynent()
};
```


# ═══════════════════════════════════════════════════════
# PLAYERSTATE — own fields start 0x0E8, size 440 bytes total
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h
# inherits nothing. sits after dynent in playerent's memory layout.
# contains per-player stats — ammo, health, gun selection.
# ═══════════════════════════════════════════════════════

```cpp
class playerstate
{
public:
    // virtual ~playerstate() exists → own vtable ptr
    // 0x0E8 (232) → vtable ptr (4 bytes)

    int health, armour;
    // 0x0EC (236) → health  ← CE CONFIRMED — subtract from field address to get base
    // 0x0F0 (240) → armour

    int primary, nextprimary;
    // 0x0F4 (244) → primary      GUN_ASSAULT default
    // 0x0F8 (248) → nextprimary

    int gunselect;
    // 0x0FC (252) → gunselect — which gun is currently selected (index 0-8)

    bool akimbo;
    // 0x100 (256) → akimbo
    // 0x101 (257) → [3 bytes padding — 257 % 4 = 1 → pad to 260]

    int ammo[NUMGUNS], mag[NUMGUNS], gunwait[NUMGUNS];
    // NUMGUNS = 9, each array = 9 × 4 = 36 bytes
    // ammo = total reserve ammo per gun
    // mag  = ammo currently in the magazine
    // gunwait = cooldown timer per gun
    //
    // 0x104 (260) → ammo[0]    GUN_KNIFE
    // 0x108 (264) → ammo[1]    GUN_PISTOL
    // 0x10C (268) → ammo[2]    GUN_CARBINE
    // 0x110 (272) → ammo[3]    GUN_SHOTGUN
    // 0x114 (276) → ammo[4]    GUN_SUBGUN
    // 0x118 (280) → ammo[5]    GUN_SNIPER
    // 0x11C (284) → ammo[6]    GUN_ASSAULT
    // 0x120 (288) → ammo[7]    GUN_GRENADE
    // 0x124 (292) → ammo[8]    GUN_AKIMBO
    //
    // 0x128 (296) → mag[0]     GUN_KNIFE
    // 0x12C (300) → mag[1]     GUN_PISTOL
    // 0x130 (304) → mag[2]     GUN_CARBINE
    // 0x134 (308) → mag[3]     GUN_SHOTGUN
    // 0x138 (312) → mag[4]     GUN_SUBGUN
    // 0x13C (316) → mag[5]     GUN_SNIPER
    // 0x140 (320) → mag[6]     GUN_ASSAULT
    // 0x144 (324) → mag[7]     GUN_GRENADE
    // 0x148 (328) → mag[8]     GUN_AKIMBO
    //
    // 0x14C (332) → gunwait[0] GUN_KNIFE
    // 0x150 (336) → gunwait[1] GUN_PISTOL
    // 0x154 (340) → gunwait[2] GUN_CARBINE
    // 0x158 (344) → gunwait[3] GUN_SHOTGUN
    // 0x15C (348) → gunwait[4] GUN_SUBGUN
    // 0x160 (352) → gunwait[5] GUN_SNIPER
    // 0x164 (356) → gunwait[6] GUN_ASSAULT
    // 0x168 (360) → gunwait[7] GUN_GRENADE
    // 0x16C (364) → gunwait[8] GUN_AKIMBO

    int pstatshots[NUMGUNS], pstatdamage[NUMGUNS];
    // stat tracking per gun — shots fired and damage dealt
    // 0x170 (368) → pstatshots[0..8]   (9 × 4 = 36 bytes)
    // 0x194 (404) → pstatdamage[0..8]  (9 × 4 = 36 bytes)
    // playerstate ends at 0x1B8 (440)

    // virtual functions (no data fields):
    //   virtual ~playerstate()
    //   virtual void spawnstate(int gamemode)
};
```


# ═══════════════════════════════════════════════════════
# PLAYERENT — own fields start 0x1B8, size 1032 bytes total
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L399
# inherits dynent AND playerstate (left to right, in that order)
# memory layout: [physent][dynent][playerstate][playerent own fields]
# ═══════════════════════════════════════════════════════

```cpp
class playerent : public dynent, public playerstate
{
    int curskin, nextskin[2];
    // 0x1B8 (440) → curskin
    // 0x1BC (444) → nextskin[0]
    // 0x1C0 (448) → nextskin[1]

    int clientnum, lastupdate, plag, ping;
    // 0x1C4 (452) → clientnum
    // 0x1C8 (456) → lastupdate
    // 0x1CC (460) → plag
    // 0x1D0 (464) → ping

    enet_uint32 address;
    // 0x1D4 (468) → address

    int lifesequence;
    // 0x1D8 (472) → lifesequence

    int frags, flagscore, deaths, tks;
    // 0x1DC (476) → frags
    // 0x1E0 (480) → flagscore
    // 0x1E4 (484) → deaths
    // 0x1E8 (488) → tks

    int lastaction, lastmove, lastpain, lastvoicecom, lastdeath;
    // 0x1EC (492) → lastaction
    // 0x1F0 (496) → lastmove
    // 0x1F4 (500) → lastpain
    // 0x1F8 (504) → lastvoicecom
    // 0x1FC (508) → lastdeath

    int clientrole;
    // 0x200 (512) → clientrole

    bool attacking;
    // 0x204 (516) → attacking
    // 0x205 (517) → [3 bytes padding — string requires 4-byte alignment]

    string name;
    // ── NON-STANDARD SIZE — READ THIS ──
    // string is NOT std::string and NOT char[16]
    // it is typedef char[MAXSTRLEN] where MAXSTRLEN = 260
    // source: https://github.com/assaultcube/AC/blob/master/source/src/tools.h#L86
    //   #define MAXSTRLEN 260
    //   typedef char string[MAXSTRLEN];
    // MAXNAMELEN = 15 (line 296 entity.h) is only the content limit at runtime
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L296
    // the storage buffer is ALWAYS 260 bytes regardless of content length
    // CE confirmed: weaponsel appeared at 0x364, only works if name = 260 bytes
    // lesson: ctrl+F for typedef before assigning size to any named type
    // 0x208 (520)  → name[0]   ← name starts here after 3 bytes padding
    // 0x30C (780)  → name ends (520 + 260 = 780)

    int team;
    // 0x30C (780) → team  ← CE CONFIRMED 0=TEAM_CLA 1=TEAM_RVSF

    int weaponchanging;
    // 0x310 (784) → weaponchanging

    int nextweapon;
    // 0x314 (788) → nextweapon

    int spectatemode, followplayercn;
    // 0x318 (792) → spectatemode
    // 0x31C (796) → followplayercn

    int eardamagemillis;
    // 0x320 (800) → eardamagemillis

    float maxroll, maxrolleffect, movroll, effroll;
    // 0x324 (804) → maxroll
    // 0x328 (808) → maxrolleffect
    // 0x32C (812) → movroll
    // 0x330 (816) → effroll

    int ffov, scopefov;
    // 0x334 (820) → ffov
    // 0x338 (824) → scopefov

    weapon *weapons[NUMGUNS];
    // ── NON-STANDARD SIZE — READ THIS ──
    // NUMGUNS = 9, each pointer = 4 bytes, total = 36 bytes
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L59
    //   enum { GUN_KNIFE=0, GUN_PISTOL, GUN_CARBINE, GUN_SHOTGUN,
    //          GUN_SUBGUN, GUN_SNIPER, GUN_ASSAULT, GUN_GRENADE, GUN_AKIMBO, NUMGUNS };
    // each entry = pointer (4 bytes) to a separate weapon object on the heap
    // weapon objects are NOT stored inside playerent
    // playerent just holds the addresses that find them
    // all 9 exist in memory from spawn regardless of loadout
    // 0x33C (828) → weapons[0] GUN_KNIFE
    // 0x340 (832) → weapons[1] GUN_PISTOL
    // 0x344 (836) → weapons[2] GUN_CARBINE
    // 0x348 (840) → weapons[3] GUN_SHOTGUN
    // 0x34C (844) → weapons[4] GUN_SUBGUN
    // 0x350 (848) → weapons[5] GUN_SNIPER
    // 0x354 (852) → weapons[6] GUN_ASSAULT
    // 0x358 (856) → weapons[7] GUN_GRENADE
    // 0x35C (860) → weapons[8] GUN_AKIMBO

    weapon *prevweaponsel, *weaponsel, *nextweaponsel, *primweap, *nextprimweap, *lastattackweapon;
    // all pointers to separate weapon objects, not inline data
    // switching weapon = weaponsel updates to point to a different weapon object
    // 0x360 (864) → prevweaponsel
    // 0x364 (868) → weaponsel  ← CE CONFIRMED
    // 0x368 (872) → nextweaponsel
    // 0x36C (876) → primweap
    // 0x370 (880) → nextprimweap
    // 0x374 (884) → lastattackweapon

    poshist history;
    // ── NON-STANDARD SIZE — READ THIS ──
    // poshist is a custom struct defined in entity.h
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L299
    //   struct poshist {
    //       int nextupdate;    → 4 bytes
    //       int curpos;        → 4 bytes
    //       int numpos;        → 4 bytes
    //       vec pos[7];        → 7 × 12 = 84 bytes (vec = 3 floats × 4 bytes)
    //   };                     → total = 96 bytes
    // #define POSHIST_SIZE 7
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L297
    // 0x378 (888)  → history start
    // 0x3D8 (984)  → history end (888 + 96 = 984)

    const char *skin_noteam, *skin_cla, *skin_rvsf;
    // 0x3D8 (984) → skin_noteam
    // 0x3DC (988) → skin_cla
    // 0x3E0 (992) → skin_rvsf

    float deltayaw, deltapitch, newyaw, newpitch;
    // 0x3E4 (996)  → deltayaw
    // 0x3E8 (1000) → deltapitch
    // 0x3EC (1004) → newyaw
    // 0x3F0 (1008) → newpitch

    int smoothmillis;
    // 0x3F4 (1012) → smoothmillis

    vec head;
    // 0x3F8 (1016) → head.x
    // 0x3FC (1020) → head.y
    // 0x400 (1024) → head.z

    bool ignored, muted;
    // 0x404 (1028) → ignored
    // 0x405 (1029) → muted

    bool nocorpse;
    // 0x406 (1030) → nocorpse
    // 0x407 (1031) → [1 byte padding]
    // playerent ends at 0x408 (1032)
};
```


# ═══════════════════════════════════════════════════════
# BOTENT — own fields start 0x408, size 1048 bytes total
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L638
# inherits playerent. same offsets as playerent for everything.
# botent just adds 4 fields at the very end.
# ═══════════════════════════════════════════════════════

```cpp
class botent : public playerent
{
    CBot *pBot;
    // 0x408 (1032) → pBot — points to bot AI class, NULL for remote clients

    playerent *enemy;
    // 0x40C (1036) → enemy — which entity the bot is targeting

    float targetpitch;
    // 0x410 (1040) → targetpitch — direction bot wants to look

    float targetyaw;
    // 0x414 (1044) → targetyaw  — direction bot wants to look
    // botent ends at 0x418 (1048)
};
```


# ═══════════════════════════════════════════════════════
# WEAPON OBJECTS — how they work
# source: https://github.com/assaultcube/AC/blob/master/source/src/weapon.h
# ═══════════════════════════════════════════════════════

weapon* (with asterisk) means pointer — the field contains an ADDRESS of a
weapon object, not the weapon data itself. the weapon object lives separately
on the heap.

all 9 weapon objects exist in memory from the moment the player spawns.
created by weapon::equipplayer() which is called from newplayerent().
weaponsel just changes to point to whichever one is currently active.
switching weapon = weaponsel updates to a different address.
the weapon objects themselves never move.

```
playerent object (in heap)
┌────────────────────────────┐
│ weapons[0] = 0x12345678   │──→ knife weapon object (somewhere in heap)
│ weapons[1] = 0x23456789   │──→ pistol weapon object (somewhere in heap)
│ weapons[5] = 0xFFFF0000   │──→ sniper weapon object (somewhere in heap)
│ ...                        │
│ weaponsel  = 0xFFFF0000   │──→ points to SAME sniper as weapons[5]
└────────────────────────────┘   (if sniper is currently selected)
```

```cpp
// weapon struct layout
// source: https://github.com/assaultcube/AC/blob/master/source/src/weapon.h
struct weapon
{
    // virtual ~weapon() exists → vtable ptr
    // 0x000 (0) → vtable ptr (4 bytes)

    int type;
    // 0x004 (4) → which gun enum value (0=knife 1=pistol ... 8=akimbo)

    playerent *owner;
    // 0x008 (8) → pointer back to the playerent that owns this weapon

    const struct guninfo &info;
    // ── NON-STANDARD — READ THIS ──
    // reference = pointer in memory (4 bytes)
    // points to a guninfo entry in the global guns[NUMGUNS] array
    // guninfo is a static data table — all pistols share one entry,
    // all snipers share one entry etc. lives in ac_client.exe static region.
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L74
    // 0x00C (12) → address of this weapon's guninfo entry

    int &ammo, &mag, &gunwait;
    // ── NON-STANDARD — READ THIS ──
    // references back into playerstate ammo[], mag[], gunwait[] arrays
    // in memory each reference = 4 bytes (same as pointer)
    // they don't store ammo directly — they point back into playerstate
    // when weapon fires: ammo-- → modifies playerstate.ammo[type] directly
    // 0x010 (16) → reference to ammo[this weapon's gun type]
    // 0x014 (20) → reference to mag[this weapon's gun type]
    // 0x018 (24) → reference to gunwait[this weapon's gun type]

    int shots;
    // 0x01C (28) → shots fired counter

    // everything after this is virtual functions → no data fields
};
```

weapon inheritance chain (each adds own fields after weapon base):

```
weapon          ← base, all shared fields
    gun         ← middle layer, most guns use this
        subgun
        sniperrifle  ← adds: bool scoped, int scoped_since
        carbine
        shotgun
        assaultrifle
        pistol
        akimbo   ← adds: int akimboside, akimbomillis, akimbolastaction[2]
    grenades     ← inherits weapon directly, adds throw state fields
    knife        ← inherits weapon directly, no extra fields
```


# ═══════════════════════════════════════════════════════
# GUNINFO — global static array, shared by all players
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L74
# lives in ac_client.exe static region. same address every session.
# one entry per gun type. all players using same gun share same entry.
# ═══════════════════════════════════════════════════════

```cpp
struct guninfo
{
    char modelname[23];   // 0x00 (0)  → "sniper", "pistol" etc
    char title[42];       // 0x17 (23) → "AD-81 SR", "Pistol" etc
    // ── PADDING — READ THIS ──
    // title ends at byte 65 which is ODD
    // shorts need even addresses (65 % 2 = 1 → not even)
    // compiler inserts 1 byte padding before first short
    // [1 byte padding] → 0x41 (65)

    short sound;          // 0x42 (66)
    short reload;         // 0x44 (68)
    short reloadtime;     // 0x46 (70)
    short attackdelay;    // 0x48 (72)
    short damage;         // 0x4A (74) ← damage per shot
    short piercing;       // 0x4C (76)
    short projspeed;      // 0x4E (78)
    short part;           // 0x50 (80)
    short spread;         // 0x52 (82)
    short recoil;         // 0x54 (84)
    short magsize;        // 0x56 (86) ← magazine size
    short mdl_kick_rot;   // 0x58 (88)
    short mdl_kick_back;  // 0x5A (90)
    short recoilincrease; // 0x5C (92)
    short recoilbase;     // 0x5E (94)
    short maxrecoil;      // 0x60 (96)
    short recoilbackfade; // 0x62 (98)
    short pushfactor;     // 0x64 (100)
    bool  isauto;         // 0x66 (102)
    // total = 103 bytes
};

extern guninfo guns[NUMGUNS];
// NUMGUNS = 9 (from enum)
// guns[0] = knife stats
// guns[1] = pistol stats
// guns[5] = sniper stats  ← "sniper" at base + 0
// etc.
```

sniper guninfo values for reference:

```
{ "sniper", "AD-81 SR", S_SNIPER, S_RSNIPER, 1950, 1500, 82, 25, 0, 0, 50, 50, 5, 4, 4, 10, 85, 85, 100, 1, false }
  modelname  title       sound     reload     rtime  adel  dmg prc psp prt spr rec mag mkr mkb ri   rb   mr   rbf  pf  auto
```


# ═══════════════════════════════════════════════════════
# FULL POINTER CHAIN — playerent base to weapon stats
# ═══════════════════════════════════════════════════════

```
playerent base
    ↓ + 0x364           → weaponsel (pointer to active weapon object)
weapon object base
    ↓ + 0x4             → type (int, which gun: 0=knife 5=sniper etc)
    ↓ + 0xC             → info (pointer to guninfo entry in static region)
guninfo entry base
    ↓ + 0x00            → modelname ("sniper")
    ↓ + 0x4A            → damage (short, 2 bytes)
    ↓ + 0x54            → recoil (short, 2 bytes)
    ↓ + 0x56            → magsize (short, 2 bytes)
    ↓ + 0x5E            → recoilbase (short, 2 bytes)
```

in CE pointer chain (bottom to top):

```
ac_client.exe+18AC00   ← static pointer to player1 object
    ↓ +0x364           ← weaponsel in playerent
    ↓ +0xC             ← info in weapon object
    ↓ +0x00            ← modelname in guninfo
→ "sniper"
```


# ═══════════════════════════════════════════════════════
# CONFIRMED STATIC POINTERS
# ═══════════════════════════════════════════════════════

```
ac_client.exe+18AC00 → player1 object base (our player, direct pointer)
ac_client.exe+18AC04 → entity array base (bots and other players)
```

confirmed in assembly:

```asm
ac_client.exe+61489 - mov eax,[ac_client.exe+18AC00] { (00821120) }
ac_client.exe+81AE0 - mov ebx,[ac_client.exe+18AC04] { (234908E8) }
```

confirmed offsets:

```
health:    object base + 0xEC   ← CE CONFIRMED
armour:    object base + 0xF0
type:      object base + 0x077  ← 0=player 1=bot
name:      object base + 0x208  ← string starts here (after 3 bytes padding)
team:      object base + 0x30C  ← CE CONFIRMED 0=CLA 1=RVSF
weaponsel: object base + 0x364  ← CE CONFIRMED
```

entity array pointer chain (for bots):

```
ac_client.exe+18AC04   ← static pointer to entity array
    ↓ +4               ← first slot = bot 1 object base
    ↓ +0xEC            ← health field
→ bot health value
```