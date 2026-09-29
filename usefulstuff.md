https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L59

# FOR REFERENCE

the full workflow from zero

1. scan for a known value (health = 100)
2. damage/heal to narrow down to one address → confirmed field address
3. subtract field offset from address → object base (changes every session)
4. verify base by checking other known offsets
5. scan for base address as 4 bytes hex
6. look for ac_client.exe+ result → static pointer (never changes)
7. OR trace backwards through assembly from field access to find static pointer
8. save as pointer chain in cheat table:
      ac_client.exe+XXXXXX → pointer → +offset → field value
9. verify survives game restart → done

for other players/bots:
   same process but the static pointer leads to an array
   array[0] = null, array[1] = first entity base, array[2] = second, etc.
   count stored separately at another static address

```CPP
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
string/char[]   1×N      String        name = 16 bytes
─────────────────────────────────────────────────────
padding rule:
  after bools before int/float → pad to next multiple of 4
  103 % 4 = 3 → 1 byte padding
  257 % 4 = 1 → 3 bytes padding
  533 % 4 = 1 → 3 bytes padding
─────────────────────────────────────────────────────
vtable ptr rule:
  any class with virtual keyword → 4 bytes at offset 0
  one ptr per class regardless of how many virtual functions
─────────────────────────────────────────────────────
inheritance rule:
  inherited block ALWAYS comes before own fields
  left-to-right order for multiple inheritance
  playerent : public dynent, public playerstate
  → dynent (includes physent) first
  → playerstate second
  → playerent own fields last
```


terminology: three things people call "base" that are NOT the same

   field address (or raw address)
      the actual memory address where a specific value lives right now
      example: 27136B7C  ← where health bytes are stored this session
      changes every session, useless to save on its own

   object base (or entity base, or instance base)
      the start of the playerent/botent object in heap memory
      offset 0 of the entire inheritance block = start of physent vtable ptr
      example: 27136A90  ← this session's player object start
      changes every session, but all offsets are measured from here
      how you get it: field address - field offset (27136B7C - 0xEC = 27136A90)

   static pointer (or base pointer, or global pointer)
      the address inside ac_client.exe that HOLDS the object base
      lives in the executable's data section, never changes between sessions
      example: ac_client.exe+17BA60  ← always contains current object base
      how you get it: scan for object base as 4 bytes hex, find the ac_client.exe+ result

the chain written out with correct terminology:

   static pointer          object base        field address    value
   ac_client.exe+17BA60 → 27136A90        →  27136B7C      →  100
   (never changes)         (heap, changes)    (heap, changes)  (health)

in CE:
   when you add an address manually with pointer ticked:
      address box = static pointer    ← ac_client.exe+17BA60
      offset box  = field offset      ← EC
      CE reads: static pointer → object base → object base + EC → value

   when CE's struct dissect shows offsets:
      those are distances from the object base
      not from the static pointer
      not from the field address


to usually find our own player object what we can / need to do would be to have our health and damage ourselves and then do decrease by and see what keeps changing, and then we have the health and etc. 

interms of finding ourselves and the entire struct, remember that the tool for creating data structuers cheat engine guesses it so it could or could not be accurate, this is  a psosibility, some attributes just get missed completely or they assume theyre 4 bytes instead of 1 and etc. etc.


why pointers and addresses are always hex

   every address you see in CE, disassemblers, memory view → hex.
   hex digits are 0-9 and A-F. if you see letters in an address that's why.
   0x prefix just makes it explicit. 0x1F5A19B4 and 1F5A19B4 are the same thing.

   when doing math on addresses always use Calculator in Programmer mode with HEX selected.
   decimal will reject the letters and give you garbage.

   subtracting an offset from an address:
      health_address - 0xEC = object base
      you're asking "health is 236 bytes forward from the start, so where is the start?"
      going backwards by the offset lands you at offset 0 = the object base.


```cpp
class physent
{
public:
    vec o, vel;                         // origin, velocity
    vec deltapos, newpos;                       // movement interpolation
    float yaw, pitch, roll;             // used as vec in one place
    float pitchvel;
    float maxspeed;                     // cubes per second, 24 for player
    int timeinair;                      // used for fake gravity
    float radius, eyeheight, maxeyeheight, aboveeye;  // bounding box size
    bool inwater;
    bool onfloor, onladder, jumpnext, jumpd, crouching, crouchedinair, trycrouch, cancollide, stuck, scoping;
    int lastjump;
    float lastjumpheight;
    int lastsplash;
    char move, strafe;
    uchar state, type;
    float eyeheightvel;
    int last_pos;
```

so now lets count the bytes of the entire size of this. for vec this link is relevant https://github.com/assaultcube/AC/blob/master/source/src/geom.h#L2-L9

```cpp
class physent
{
    // virtual → vtable ptr
    // 0x000 (0) → 4 bytes

    vec o, vel;
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
    // 0x067 (103) → [1 byte padding]

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
    // 0x076 (118) → state
    // 0x077 (119) → type

    float eyeheightvel;
    // 0x078 (120) → eyeheightvel

    int last_pos;
    // 0x07C (124) → last_pos
    // physent ends at 0x080 (128)
};

class dynent : public physent
{
    bool k_left, k_right, k_up, k_down;
    // 0x080 (128) → k_left
    // 0x081 (129) → k_right
    // 0x082 (130) → k_up
    // 0x083 (131) → k_down

    animstate prev[2], current[2];
    // 0x084 (132) → prev[0]     20 bytes
    // 0x098 (152) → prev[1]     20 bytes
    // 0x0AC (172) → current[0]  20 bytes
    // 0x0C0 (192) → current[1]  20 bytes

    int lastanimswitchtime[2];
    // 0x0D4 (212) → lastanimswitchtime[0]
    // 0x0D8 (216) → lastanimswitchtime[1]

    void *lastmodel[2];
    // 0x0DC (220) → lastmodel[0]
    // 0x0E0 (224) → lastmodel[1]

    int lastrendered;
    // 0x0E4 (228) → lastrendered
    // dynent ends at 0x0E8 (232)
};

class playerstate
{
    // virtual → own vtable ptr
    // 0x0E8 (232) → 4 bytes

    int health, armour;
    // 0x0EC (236) → health
    // 0x0F0 (240) → armour

    int primary, nextprimary;
    // 0x0F4 (244) → primary
    // 0x0F8 (248) → nextprimary

    int gunselect;
    // 0x0FC (252) → gunselect

    bool akimbo;
    // 0x100 (256) → akimbo
    // 0x101 (257) → [3 bytes padding]

    int ammo[NUMGUNS];
    // 0x104 (260) → ammo[0] GUN_KNIFE
    // 0x108 (264) → ammo[1] GUN_PISTOL
    // 0x10C (268) → ammo[2] GUN_CARBINE
    // 0x110 (272) → ammo[3] GUN_SHOTGUN
    // 0x114 (276) → ammo[4] GUN_SUBGUN
    // 0x118 (280) → ammo[5] GUN_SNIPER
    // 0x11C (284) → ammo[6] GUN_ASSAULT
    // 0x120 (288) → ammo[7] GUN_GRENADE
    // 0x124 (292) → ammo[8] GUN_AKIMBO

    int mag[NUMGUNS];
    // 0x128 (296) → mag[0] GUN_KNIFE
    // 0x12C (300) → mag[1] GUN_PISTOL
    // 0x130 (304) → mag[2] GUN_CARBINE
    // 0x134 (308) → mag[3] GUN_SHOTGUN
    // 0x138 (312) → mag[4] GUN_SUBGUN
    // 0x13C (316) → mag[5] GUN_SNIPER
    // 0x140 (320) → mag[6] GUN_ASSAULT
    // 0x144 (324) → mag[7] GUN_GRENADE
    // 0x148 (328) → mag[8] GUN_AKIMBO

    int gunwait[NUMGUNS];
    // 0x14C (332) → gunwait[0]
    // 0x150 (336) → gunwait[1]
    // 0x154 (340) → gunwait[2]
    // 0x158 (344) → gunwait[3]
    // 0x15C (348) → gunwait[4]
    // 0x160 (352) → gunwait[5]
    // 0x164 (356) → gunwait[6]
    // 0x168 (360) → gunwait[7]
    // 0x16C (364) → gunwait[8]

    int pstatshots[NUMGUNS];
    // 0x170 (368) → pstatshots[0]
    // 0x174 (372) → pstatshots[1]
    // 0x178 (376) → pstatshots[2]
    // 0x17C (380) → pstatshots[3]
    // 0x180 (384) → pstatshots[4]
    // 0x184 (388) → pstatshots[5]
    // 0x188 (392) → pstatshots[6]
    // 0x18C (396) → pstatshots[7]
    // 0x190 (400) → pstatshots[8]

    int pstatdamage[NUMGUNS];
    // 0x194 (404) → pstatdamage[0]
    // 0x198 (408) → pstatdamage[1]
    // 0x19C (412) → pstatdamage[2]
    // 0x1A0 (416) → pstatdamage[3]
    // 0x1A4 (420) → pstatdamage[4]
    // 0x1A8 (424) → pstatdamage[5]
    // 0x1AC (428) → pstatdamage[6]
    // 0x1B0 (432) → pstatdamage[7]
    // 0x1B4 (436) → pstatdamage[8]
    // playerstate ends at 0x1B8 (440)
};

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

    string name;
    // 0x205 (517) → name[16]
    // 0x215 (533) → [3 bytes padding]

    int team;
    // 0x218 (536) → team

    int weaponchanging;
    // 0x21C (540) → weaponchanging

    int nextweapon;
    // 0x220 (544) → nextweapon

    int spectatemode, followplayercn;
    // 0x224 (548) → spectatemode
    // 0x228 (552) → followplayercn

    int eardamagemillis;
    // 0x22C (556) → eardamagemillis

    float maxroll, maxrolleffect, movroll, effroll;
    // 0x230 (560) → maxroll
    // 0x234 (564) → maxrolleffect
    // 0x238 (568) → movroll
    // 0x23C (572) → effroll

    int ffov, scopefov;
    // 0x240 (576) → ffov
    // 0x244 (580) → scopefov

    weapon *weapons[NUMGUNS];
    // 0x248 (584) → weapons[0]
    // 0x24C (588) → weapons[1]
    // 0x250 (592) → weapons[2]
    // 0x254 (596) → weapons[3]
    // 0x258 (600) → weapons[4]
    // 0x25C (604) → weapons[5]
    // 0x260 (608) → weapons[6]
    // 0x264 (612) → weapons[7]
    // 0x268 (616) → weapons[8]

    weapon *prevweaponsel, *weaponsel, *nextweaponsel, *primweap, *nextprimweap, *lastattackweapon;
    // 0x26C (620) → prevweaponsel
    // 0x270 (624) → weaponsel
    // 0x274 (628) → nextweaponsel
    // 0x278 (632) → primweap
    // 0x27C (636) → nextprimweap
    // 0x280 (640) → lastattackweapon

    poshist history;
    // 0x284 (644) → history (96 bytes total)

    const char *skin_noteam, *skin_cla, *skin_rvsf;
    // 0x2E4 (740) → skin_noteam
    // 0x2E8 (744) → skin_cla
    // 0x2EC (748) → skin_rvsf

    float deltayaw, deltapitch, newyaw, newpitch;
    // 0x2F0 (752) → deltayaw
    // 0x2F4 (756) → deltapitch
    // 0x2F8 (760) → newyaw
    // 0x2FC (764) → newpitch

    int smoothmillis;
    // 0x300 (768) → smoothmillis

    vec head;
    // 0x304 (772) → head.x
    // 0x308 (776) → head.y
    // 0x30C (780) → head.z

    bool ignored, muted;
    // 0x310 (784) → ignored
    // 0x311 (785) → muted

    bool nocorpse;
    // 0x312 (786) → nocorpse
    // 0x313 (787) → [1 byte padding]
    // playerent ends at 0x314 (788)
};

class botent : public playerent
{
    CBot *pBot;
    // 0x314 (788) → pBot

    playerent *enemy;
    // 0x318 (792) → enemy

    float targetpitch;
    // 0x31C (796) → targetpitch

    float targetyaw;
    // 0x320 (800) → targetyaw
    // botent ends at 0x324 (804)
};
```


why subtracting 0xEC from health gives you the object base and not something weird

   the inheritance chain stacks in memory like this:
      offset 0:   physent block     ← object base (this is what you want)
      offset 128: dynent block
      offset 232: playerstate block ← health lives in here at +4 from playerstate start
      offset 440: playerent own fields

   physent is always first because it's the root parent. every class that inherits
   from physent starts with the physent block at offset 0. so the object base of any
   playerent, botent, dynent IS the start of the physent block.

   health is at 0xEC (236) from that base. subtract that from any health address
   and you always land at offset 0 = the object base = the physent vtable pointer.

   this is why the same subtraction works for players AND bots. same inheritance
   chain, same layout, same offsets. just different object base addresses.


# Time to work.

So now we have the ability to finally be able to fucking validate small dynamic / initial base addresses using these offsites.

verification checklist: always do this when you find a new base

found an address? subtract the offset to get base, then verify (specific to this game in this case):
   base + 0xEC  → should show health value
   base + 0x077 → should show 0 (player) or 1 (bot)
   base + 0x205 → should show name as string in memory view
   base + 0x004 → should show X position (Float, changes as entity moves)

if all four check out → base confirmed.

CE struct guesser is a starting point only. it guesses types wrong constantly.
always cross reference against source code. the source is the ground truth.


memory regions: why this matters

three regions:
- static (ac_client.exe+offset) → global vars, never moves, same every run
- heap → where new/malloc puts objects, randomized every session (ASLR)
- stack → function calls, local vars, changes constantly


Regarding the heap specifically for the entity list in this case its held up in the heap. So we cant really walk it up or down, its all in different areas. furthremore here is an imagge example.:

excalidraw



playerent objects live on the heap which is why the base address changes every session.
global variables like g_player and g_other_players live in the static region which is
why ac_client.exe+offset never changes. the static pointer is the anchor into the heap.


why you CANT just save the object base directly

   every time the game starts, new/malloc gives the playerent object
   a different heap address. ASLR randomizes where heap memory lands.

   session 1:  player object base = 0x00939E40
   session 2:  player object base = 0x27136A90
   session 3:  player object base = 0x04F2A100

   the offsets never change because they come from source code.
   the object base changes every session because it lives on the heap.
   this is why you need the static pointer chain:

      static pointer        ← lives in executable, same every run
          ↓ read it
      object base           ← different every run, lives on heap
          ↓ + offset
      field value           ← health, name, position, etc.

   CE follows this chain automatically when you tick the pointer checkbox.


this is how we found ourselves but now we need to find the bot

```cpp
class CBot;

class botent : public playerent  // its inheriting from playernt, its GG for the bot lil vro
{
public:
    // Added by Rick
    CBot *pBot; // Only used if this is a bot, points to the bot class if we are the host,
                // for other clients its NULL
    // End add by Rick

    playerent *enemy;                      // monster wants to kill this entity
    // Added by Rick: targetpitch
    float targetpitch;                    // monster wants to look in this direction
    // End add
    float targetyaw;                    // monster wants to look in this direction

    botent() : pBot(NULL), enemy(NULL) { type = ENT_BOT; }
    ~botent() { }

    int deaths() { return lifesequence; }
};
#endif //#ifndef STANDALONE
```

so what we can do is find a object that points to the class but... how can I find that class? another thing would be to do the same thing that we did for ourselves where we continiously damage the bot

i did it and founnd the address oif that specific bots health it was 

```xml
<?xml version="1.0" encoding="utf-8"?>
<CheatTable>
  <CheatEntries>
    <CheatEntry>
      <ID>18</ID>
      <Description>"CONFIRMED BOT HEALTH"</Description>
      <LastState Value="100" RealAddress="1F5A19B4"/>
      <VariableType>4 Bytes</VariableType>
      <Address>1F5A19B4</Address>
    </CheatEntry>
  </CheatEntries>
</CheatTable>
```

so that minus 0xEC = `1F5A18C8` now to verify it I add offsets to this address to try and vverify it.

1F5A18C8 + 0xEC  = 1F5A19B4  → should show bot health (whatever you shot it to)
1F5A18C8 + 0x077 = 1F5A193F  → should show 1 (ENT_BOT)
1F5A18C8 + 0x205 = 1F5A1ACD  → should show bot name as string


![alt text](image-1.png)

![alt text](image.png)

it works!


strings in C are known as c style string, they end in 00 which is a null byte which indicates its a string.

![alt text](image-2.png)

so for that assembly it basically


the thing about pointers the bit the video was missing that, in C these 2 lines are equivelant (the last two). 

```c
SOMETHING* X = my_array[INDEX];
and
something* x = *(my_array+index);
```

are the same fucking thing. the *4 is implicit in C, its just the data type. when you add a number to the pointer in C it gets multiplied by the size of the datatype its pointed to.


arrays values are stored on the heap, allows us to allocate memory at runtime, thereforce due to ASLR it becomes randomized and we dont know where its going to be. its really good because global_entities kind of have to be present because think of the use function of a game.



this was the right one?


```c
ac_client.exe+81AE0 - 8B 1D 04AC5800        - mov ebx,[ac_client.exe+18AC04] { (234908E8) }
```


okay so now that we have this finally. 


![alt text](image-3.png)


so now when we add the initial offset being 4 (the order matters its bottom first and up last.) and when I change that 4 offset I change things within that list. the second newest offset is 205 which is the offset for the name. we have the entity list finally at last.


assembly reading methodology: how to figure out unknown addresses

1. find a field you know (health, name, position)
2. "find what accesses this address" in CE
3. look at the instruction → [register + offset] tells you the offset from base
4. ask: what loaded that register? scroll up in disassembler
5. keep asking "where did this come from" until you hit [ac_client.exe+XXXXXX]
6. that static address = global variable = your permanent pointer

the pattern is always:
   mov reg, [ac_client.exe+XXXXXX]   ← load global (static, never changes)
   mov reg, [reg+esi*4]              ← index into array (heap, changes every session)
   mov reg, [reg+offset]             ← read field from object

recognizing these three patterns lets you reverse any game.

reading assembly cold, how to not get lost

   when you open disassembly and don't know what anything means, ask these
   questions in order:

   1. what does this instruction DO?
      mov  = copy a value
      cmp  = compare two values (sets flags, doesn't store)
      jmp/je/jng = jump somewhere based on flags
      push = save to stack
      sub  = subtract
      add  = add
      test = AND two values, check if result is zero

   2. are there brackets?
      [register]         = go to that address and read what's there (dereference)
      [register+offset]  = go to address+offset and read (struct field access)
      [register+reg*4]   = go to address+(reg×4) and read (array index)
      no brackets        = use the value directly, don't dereference

   3. what region is the value in?
      0x004xxxxx to 0x006xxxxx = inside ac_client.exe = static = global variable
      0x001xxxxx = stack = temporary, ignore
      everything else = heap = dynamic object

   those three questions get you 80% of the way through any function.

To validate, grab that address and then work with it based on the code itself, in this case we add the initial offset of 4 and then add to it 205 to see if it actually gives us the value that we want. remember always read the source code and understand the behaviour.