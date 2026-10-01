```cpp
class playerent : public dynent, public playerstate
// https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L399
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
    // CE confirmed this: weaponsel appeared at 0x364, only works if name = 260 bytes
    // 0x208 (520)  → name[0]   ← name starts here after 3 bytes padding
    // 0x30C (780)  → name ends (520 + 260 = 780)

    int team;
    // 0x30C (780) → team

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
    // 0x360 (864) → prevweaponsel
    // 0x364 (868) → weaponsel  ← CE confirmed this offset
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
    // POSHIST_SIZE = 7 defined at line 297:
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L297
    //   #define POSHIST_SIZE 7
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

class botent : public playerent
// https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L638
{
    CBot *pBot;
    // 0x408 (1032) → pBot

    playerent *enemy;
    // 0x40C (1036) → enemy

    float targetpitch;
    // 0x410 (1040) → targetpitch

    float targetyaw;
    // 0x414 (1044) → targetyaw
    // botent ends at 0x418 (1048)
};```



now for the dyent


```cpp
class physent
// https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L105
{
    // virtual ~physent() exists → vtable ptr injected at offset 0
    // 0x000 (0) → 4 bytes

    vec o, vel;
    // ── NON-STANDARD SIZE — READ THIS ──
    // vec is a custom struct defined in geom.h
    // source: https://github.com/assaultcube/AC/blob/master/source/src/geom.h#L2
    //   struct vec { float x, y, z; };  → 3 floats × 4 bytes = 12 bytes each
    // 0x004 (4)  → o.x
    // 0x008 (8)  → o.y
    // 0x00C (12) → o.z
    // 0x010 (16) → vel.x
    // 0x014 (20) → vel.y
    // 0x018 (24) → vel.z

    vec deltapos, newpos;
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
    // 0x048 (72) → timeinair

    float radius, eyeheight, maxeyeheight, aboveeye;
    // 0x04C (76) → radius
    // 0x050 (80) → eyeheight
    // 0x054 (84) → maxeyeheight
    // 0x058 (88) → aboveeye

    bool inwater;
    // 0x05C (92) → inwater

    bool onfloor, onladder, jumpnext, jumpd, crouching, crouchedinair, trycrouch, cancollide, stuck, scoping;
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
    // 0x067 (103) → [1 byte padding — 11 bools end at 103, next field int needs 4-byte align, 103 % 4 = 3 → 1 byte pad]

    int lastjump;
    // 0x068 (104) → lastjump

    float lastjumpheight;
    // 0x06C (108) → lastjumpheight

    int lastsplash;
    // 0x070 (112) → lastsplash

    char move, strafe;
    // 0x074 (116) → move
    // 0x075 (117) → strafe

    uchar state, type;
    // 0x076 (118) → state  (CS_ALIVE=0 CS_DEAD=1)
    // 0x077 (119) → type   (ENT_PLAYER=0 ENT_BOT=1)

    float eyeheightvel;
    // 0x078 (120) → eyeheightvel

    int last_pos;
    // 0x07C (124) → last_pos
    // physent ends at 0x080 (128)
};


class dynent : public physent
// https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L156
// physent block sits above starting at offset 0
// dynent own fields start at 0x080 (128)
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
};```



the weapons now 

```cpp
// ═══════════════════════════════════════════
// UNDERSTANDING weapon* POINTERS IN PLAYERENT
// ═══════════════════════════════════════════

// the weapon struct is defined in weapon.h
// source: https://github.com/assaultcube/AC/blob/master/source/src/weapon.h

// a weapon* (with asterisk) means:
//   this field does NOT contain weapon data directly
//   it contains the ADDRESS of a weapon object somewhere on the heap
//   the weapon object itself lives separately in memory

// so when you see:
weapon *weapons[NUMGUNS];
// this is an array of 9 pointers
// each pointer = 4 bytes (32-bit address)
// each address points TO a separate weapon object on the heap
// the weapon objects are not stored inside playerent
// playerent just holds the addresses that find them

// visualized:
//
// playerent object (in heap)
// ┌────────────────────────────┐
// │ ...other fields...         │
// │ weapons[0] = 0x12345678   │──→ knife weapon object (somewhere in heap)
// │ weapons[1] = 0x23456789   │──→ pistol weapon object (somewhere in heap)
// │ weapons[2] = 0x34567890   │──→ carbine weapon object (somewhere in heap)
// │ ...                        │
// │ weaponsel  = 0x23456789   │──→ points to SAME pistol object as weapons[1]
// └────────────────────────────┘   (if pistol is currently selected)

// weaponsel specifically:
weapon *weaponsel;
// this is ONE pointer (4 bytes) that points to whichever weapon is
// currently selected and active. it doesn't store weapon data,
// it just stores the address of the active weapon object.
// when you switch weapons, weaponsel gets updated to point to
// the new weapon object. the weapon objects themselves don't move.

// to get weapon data you have to follow the pointer:
//   playerent base + 0x364 → read 4 bytes → address of weapon object
//   weapon object base + 0xC → read 4 bytes → address of guninfo
//   guninfo base + 0x0 → read bytes → modelname string ("sniper", "pistol" etc)
```

```cpp
// weapon struct layout for reference:
// source: https://github.com/assaultcube/AC/blob/master/source/src/weapon.h
struct weapon
{
    // virtual ~weapon() exists → vtable ptr
    // 0x000 (0) → 4 bytes

    int type;
    // 0x004 (4) → which gun enum value this weapon is

    playerent *owner;
    // 0x008 (8) → pointer back to the playerent that owns this weapon

    const struct guninfo &info;
    // ── NON-STANDARD — READ THIS ──
    // this is a REFERENCE (same as a pointer in memory, 4 bytes)
    // it points to a guninfo struct in a global array called guns[NUMGUNS]
    // guninfo is a static data table, not per-player — all pistol weapons
    // share the same guninfo entry, all snipers share theirs, etc.
    // source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L74
    //   struct guninfo { char modelname[23], title[42]; short sound, reload... };
    //   extern guninfo guns[NUMGUNS];  ← global array in static memory
    // 0x00C (12) → address of this weapon's guninfo entry

    int &ammo, &mag, &gunwait;
    // ── NON-STANDARD — READ THIS ──
    // these are REFERENCES to fields inside playerstate (ammo[], mag[], gunwait[])
    // in memory each reference = 4 bytes (same as pointer)
    // they don't store the ammo count directly
    // they point back into the playerent's own ammo/mag/gunwait arrays
    // 0x010 (16) → reference to ammo[this weapon's gun type]
    // 0x014 (20) → reference to mag[this weapon's gun type]
    // 0x018 (24) → reference to gunwait[this weapon's gun type]

    int shots;
    // 0x01C (28) → shots fired counter

    // everything after this is virtual functions → no data fields
};
```
