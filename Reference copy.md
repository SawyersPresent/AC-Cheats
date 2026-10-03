# Reverse Engineering Notes — AssaultCube
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h

---

# ═══════════════════════════════════════════════════════
# METHODOLOGY — HOW TO FIGURE OUT ANYTHING ON YOUR OWN
# ═══════════════════════════════════════════════════════

this is the thinking process. not just the steps. the actual way to reason
through any unknown value, any unknown offset, any unknown pointer from zero.


## phase 1 — find the value

pick something you can control and observe.
health is ideal because you can change it on demand by taking damage.
kill count works too. position works. anything you can make go up or down.

```
open CE, attach to ac_client.exe
scan for the current value (exact value, 4 bytes)
change it in game (take damage, get a kill, move)
scan for the new value
repeat until one address remains
```

if you get stuck with too many results:
   change the value multiple times between scans
   each scan eliminates everything that didn't change to your exact value
   if results still won't narrow down, try "decreased by" or "increased by"
   instead of exact value — useful when you can't control the exact number

that one remaining address = the field address for this session.
write it down. this is your starting point for everything else.


## phase 2 — find the offset

the offset is the distance between the object base and your field.
you need it to calculate the object base, and to verify other fields later.

two ways to find it:

### way 1 — read it from the assembly (most reliable)

```
right click the field address in CE
→ find what accesses this address
trigger the behaviour in game (take damage, get a kill etc)
instructions appear in the box
```

look at the instructions. find one that looks like:

```asm
mov reg, [some_register + 000000XX]
push [some_register + 000000XX]
cmp dword ptr [some_register + 000000XX], 00
```

the `XX` after the `+` is your offset in hex.

example for health:

```asm
0045E3EB - mov ecx, [ebx + 000000EC]
```

offset = `EC`. that is how far health is from the object base.

### way 2 — infer it from behaviour

if you see a compare against zero while debugging health:

```asm
cmp dword ptr [esi + 000000EC], 00
```

that is the game checking if you are dead (health == 0). you know the
field is health just from the context. EC is confirmed as the offset.

if you see a subtract instruction touching your field:

```asm
sub [ebx + 000000EC], esi
```

that is the game applying damage. same confirmation.

always ask: what behaviour would logically trigger this instruction?
if the answer matches what you did in game to trigger the breakpoint → confirmed.


## phase 3 — calculate the object base

```
open Calculator → Programmer mode → HEX selected
field address - offset = object base
```

example:

```
0082120C - EC = 00821120
```

`00821120` = object base this session. this is the start of the entire
player object in heap memory. all offsets for all fields are measured from here.

sanity check the math:
   object base + offset should give you back the field address
   00821120 + EC = 0082120C ✓


## phase 4 — find the static pointer

the object base changes every session. you need something permanent.
the static pointer is an address inside `ac_client.exe` that always holds
the current object base. find it once, use it forever.

### method 1 — scan for it (start here)

```
in CE, new scan
value type: 4 bytes
scan type:  exact value
value:      00821120    (your object base this session)
hex checkbox ticked
first scan
```

look through results for one with `ac_client.exe+XXXXXX` in the address column.
that address lives inside the executable — static region, never moves.

write down that `ac_client.exe+XXXXXX`. that is your static pointer.

### method 2 — trace through assembly (more powerful, works without source)

from the "find what accesses" list, double click an instruction.
disassembler opens at that line.
scroll UP from there.

you are looking for the line that LOADED the register that holds the object base.
it will look like:

```asm
mov reg, [ac_client.exe+XXXXXX]
```

brackets around an `ac_client.exe+` address = reading a global variable =
that IS your static pointer.

dead end — if near the top of the function you see:

```asm
mov ebx, ecx
mov ebx, edx
```

the object was passed in as a parameter. not loaded here.
try a different instruction from the access list.

what to look for in the disassembler:

```
pattern 1: mov reg, [ac_client.exe+XXXXXX]   ← loading from static region
           THIS IS THE STATIC POINTER

pattern 2: mov reg, [reg + offset]            ← reading a field from struct
           the offset tells you where in the object

pattern 3: mov reg, [reg + reg*4]             ← indexing into an array
           the second reg is the index, *4 scales by pointer size
```

when you find the static pointer confirm it makes sense:
   does the value it currently holds match your object base?
   example: ac_client.exe+18AC00 currently holds 00821120 ✓


## phase 5 — verify the static pointer survives restart

```
in CE add a new entry:
   address: ac_client.exe+XXXXXX    ← your static pointer
   offset:  XX                      ← your field offset
   type:    4 bytes
   tick pointer checkbox
```

the entry should show the correct field value right now.
close the game. reopen it. if it still shows the correct value → confirmed.
the static pointer is permanent. save it.


## phase 6 — validate the object base with multiple fields

once you have the object base, verify it is actually correct before trusting it.
pick 3-4 fields you can independently confirm and check them all.

```
object base + 0xEC  → health       (should match current health)
object base + 0x077 → type         (should be 0 for player, 1 for bot)
object base + 0x208 → name         (should show name string in memory view)
object base + 0x004 → position X   (should change as entity moves)
```

if all four match → base confirmed. if one fails → your offset calculation
has an error somewhere. go back to phase 2.

why this works: the same object base works for every field because all offsets
are distances from offset 0 of the same object. the source code defines these
distances. they never change. only the base address changes between sessions.


## phase 7 — finding new fields with a confirmed static pointer

once you have a working static pointer you can explore any field without
scanning from scratch. the pointer already gives you the object base every
session. you just add different offsets.

process for any new field:

```
1. look at the source code for the field you want
2. calculate its offset from the object base (count bytes through the struct)
3. in CE: static pointer + new offset
4. verify the value makes sense for the current game state
5. change the value in game and watch CE update → confirmed
```

example — finding armour:

```
look at source: health is at 0x0EC, armour is the next int after it
0x0EC + 4 = 0x0F0
static pointer + offset 0xF0 → should show current armour value
put on armour in game → value changes ✓
```

example — finding position:

```
look at source: o.x is at offset 0x004 from physent base
static pointer + offset 0x4 → should show a float (your X coordinate)
move in game → value changes ✓
```


## phase 8 — thinking through things that don't work

when something behaves unexpectedly, ask these questions in order:

**the value doesn't appear / shows garbage:**
```
→ wrong type? (reading int when it's a float, or vice versa)
→ wrong offset? (recount bytes from source, check for non-standard types)
→ wrong pointer? (verify static pointer still holds correct object base)
```

**you write a value but it snaps back immediately:**
```
→ the game is overwriting it every frame
→ health does this — the game manages it constantly
→ use CE freeze (lock checkbox) to keep writing your value faster
→ or find the code that writes to it and patch it
→ kill count doesn't do this because nothing writes to it constantly
```

**offset works on bot but not player or vice versa:**
```
→ check if they use the same static pointer or different ones
→ player1 → ac_client.exe+18AC00 (direct pointer)
→ bots    → ac_client.exe+18AC04 + 4 (array, need first slot)
→ the offsets inside the object are the same — inheritance is identical
→ the entry point into memory is different
```

**the value is there but wrong size:**
```
→ check source for the actual type (int vs short vs bool)
→ shorts are 2 bytes, bools are 1 byte, ints are 4 bytes
→ CE reads wrong type = wrong value even at correct address
```

**static pointer gives different base after restart:**
```
→ that is expected — the object base changes every session
→ the static pointer itself should not change
→ if the static pointer address changed, you found a heap address not a static one
→ real static pointers are in the ac_client.exe range: 0x004xxxxx to 0x006xxxxx
```

**non-standard type throws off all offsets after it:**
```
→ you assumed wrong size for a type (the string = 260 mistake)
→ ctrl+F in source for typedef, struct, class, #define before assigning sizes
→ CE verify: if your calculated offset doesn't match what CE finds, one of
  your types has the wrong size. binary search — which field is the first
  wrong one? the non-standard type is right before it.
```


## the mental model — always think in these terms

```
static pointer          → permanent, lives in executable, your anchor
    ↓ dereference
object base             → temporary, heap, different every session
    ↓ + offset
field address           → temporary, heap, different every session
    ↓ read bytes
value                   → what you actually want
```

every layer below the static pointer changes every session.
every offset stays the same because it comes from source code.
the static pointer is the only thing you save permanently.

when you find something new, always verify it survives a restart before
trusting it. if it doesn't survive a restart, you saved a heap address
instead of a static pointer.


---

# ═══════════════════════════════════════════════════════
# THE CORE WORKFLOW — FROM VALUE TO STATIC POINTER
# ═══════════════════════════════════════════════════════

the goal is simple. find a value in memory → find its offset → find the
object base → find the static pointer that survives restarts.


## step 1 — find the value

open CE, attach to ac_client.exe. health starts at 100 so:

```
scan type: exact value
value type: 4 bytes
value:      100
first scan
```

thousands of results. too many. go in game and take damage. scan for the
new health value. take damage again. scan again. keep going until one address
remains. that address is the field address — the raw address of health right now.

example this session: `0082120C`


## step 2 — find the offset

the offset is the distance between the start of the player object and the
health field. if you already know it from source code its `EC`.

if you dont know it yet:

```
right click the field address in CE
→ find what accesses this address
go take damage in game
instructions appear in the box
```

they look like this:

```asm
00461628 - FF B7 EC000000     - push [edi+000000EC]
0045E3EB - 8B 8B EC000000     - mov ecx,[ebx+000000EC]
0047D1A8 - 83 BE EC000000 00  - cmp dword ptr [esi+000000EC],00
```

the number after the `+` in the brackets is the offset. `EC` in all three
cases. that is how far health lives from the start of the object.

you can also infer it from behaviour. if you see:

```asm
ac_client.exe+7D1A8 - 83 BE EC000000 00 - cmp dword ptr [esi+000000EC],00
```

a compare against zero while debugging health = the game checking if you are
dead. `EC` is the offset even without knowing it from the source.


## step 3 — find the object base

open Calculator → programmer mode → HEX selected.

```
field address - offset = object base
0082120C      - EC     = 00821120
```

`00821120` is the object base this session. it changes every session because
it lives on the heap. useless to save on its own.


## step 4 — find the static pointer

### METHOD 1 — scan for it (easier, start here)

in CE do a new scan:

```
value type: 4 bytes
scan type:  exact value
value:      00821120
hex checkbox ticked
```

look through the results for one showing `ac_client.exe+XXXXXX` in the address
column. that one lives in the static region — same address every session.

![scanning for object base to find static pointer](image-6.png)

example found: `ac_client.exe+18AC00`


### METHOD 2 — trace backwards through assembly (more powerful)

from the "find what accesses" list, double click an instruction to open
the disassembler at that exact location. then scroll UP.

you are looking for one specific line:

```asm
mov reg, [ac_client.exe+XXXXXX]
```

brackets around an `ac_client.exe+` address = reading from the static region =
that IS the static pointer. write down the `XXXXXX` part.

example of what finding it looks like:

```asm
ac_client.exe+61489 - A1 00AC5800 - mov eax,[ac_client.exe+18AC00] { (00821120) }
```

see `00821120` in the curly braces? that is the object base we calculated.
`ac_client.exe+18AC00` is holding it. that is the static pointer.

**dead end signal** — if near the top of the function you see:

```asm
mov ebx, ecx
mov ebx, edx
```

the object was passed in as a parameter. the static pointer is NOT in this
function. close it and try a different instruction from the access list.

---

then I set up a breakpoint on the instruction to find:

```asm
ac_client.exe+61628 - FF B7 EC000000 - push [edi+000000EC]
```

so were pushing the edi register with the offset EC. if we dont know EC is
health we can infer it based on the behaviour and the instruction — for example
if were debugging health and we see a compare assembly thats comparing a
register to null which means its the game checking if we are dead or not.

example:

```asm
ac_client.exe+7D1A8 - 83 BE EC000000 00 - cmp dword ptr [esi+000000EC],00
```

so based on that address if we set a breakpoint on that assembly and trigger
this behaviour then we can find the value.

![breakpoint hit showing register value = object base](image-5.png)

so we found the base which is `00821120`. to test this we saw with the initial
assembly instruction that it was this base address + the offset EC.

we test this by doing `00821120 + 0xEC` and we can see if this actually
is the same number as our health or not.

so now to recover everything we take the base address `00821120` and we search
for this hex value.

![scanning CE for object base value showing ac_client.exe+ result](image-6.png)


## step 5 — verify it

in CE add a new entry:

```
address: ac_client.exe+18AC00
offset:  EC
type:    4 bytes
tick pointer checkbox
```

should show health value.


## step 6 — confirm it survives restart

close the game. reopen it. if the CE entry still shows health correctly →
confirmed. the static pointer is permanent.

now we save this then we close the game and boot it back up to see if its
actually reliable or not. in this case the one that was the most reliable
and booted up first was `ac_client.exe+18AC00`. now to test this we just take
this address then we make it a pointer, we add the offset we actually know.


---

# ═══════════════════════════════════════════════════════
# THE CHAIN IN PLAIN ENGLISH
# ═══════════════════════════════════════════════════════

so it starts with the following:

find the value → use the value to find its offset → use the offset to find
its base address → use the base address to find the static global pointer

written out with addresses:

```
find the value           → 0082120C    (health address this session)
     ↓ - EC (offset)
find the object base     → 00821120    (start of player object this session)
     ↓ scan for it
find the static pointer  → ac_client.exe+18AC00   (never changes)
```

CE reads it as:

```
go to ac_client.exe+18AC00
    ↓ read 4 bytes → get 00821120 (object base, changes every session)
add EC to that
    ↓ → get 0082120C (health field address)
read 4 bytes there
    ↓ → get health value
```


---

# ═══════════════════════════════════════════════════════
# TERMINOLOGY — THREE THINGS PEOPLE CALL "BASE"
# ═══════════════════════════════════════════════════════

```
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
```

the chain written out with correct terminology:

```
static pointer          object base        field address    value
ac_client.exe+17BA60 → 27136A90        →  27136B7C      →  100
(never changes)         (heap, changes)    (heap, changes)  (health)
```

in CE when you add an address manually with pointer ticked:

```
address box = static pointer    ← ac_client.exe+17BA60
offset box  = field offset      ← EC
CE reads: static pointer → object base → object base + EC → value
```

when CE's struct dissect shows offsets:

```
those are distances from the object base
not from the static pointer
not from the field address
```


---

# ═══════════════════════════════════════════════════════
# MEMORY REGIONS
# ═══════════════════════════════════════════════════════

```
static   → ac_client.exe+offset → global vars, never moves, same every run
heap     → where new/malloc puts objects, randomized every session (ASLR)
stack    → function calls, local vars, changes constantly
```

playerent objects live on the heap which is why the base address changes
every session. global variables like g_player and g_other_players live in
the static region which is why `ac_client.exe+offset` never changes. the
static pointer is the anchor into the heap.

why you CANT just save the object base directly:

```
session 1:  player object base = 0x00939E40
session 2:  player object base = 0x27136A90
session 3:  player object base = 0x04F2A100
```

the offsets never change because they come from source code.
the object base changes every session because it lives on the heap.
this is why you need the static pointer chain:

```
static pointer        ← lives in executable, same every run
    ↓ read it
object base           ← different every run, lives on heap
    ↓ + offset
field value           ← health, name, position, etc.
```

CE follows this chain automatically when you tick the pointer checkbox.


---

# ═══════════════════════════════════════════════════════
# HEX AND ADDRESS MATH
# ═══════════════════════════════════════════════════════

every address you see in CE, disassemblers, memory view → hex.
hex digits are 0-9 and A-F. if you see letters in an address that's why.
`0x` prefix just makes it explicit. `0x1F5A19B4` and `1F5A19B4` are the same.

when doing math on addresses always use Calculator in Programmer mode with
HEX selected. decimal will reject the letters and give you garbage.

subtracting an offset from an address:

```
health_address - 0xEC = object base
```

you're asking "health is 236 bytes forward from the start, so where is the
start?" going backwards by the offset lands you at offset 0 = the object base.

why subtracting 0xEC from health gives you the object base:

```
offset 0:   physent block     ← object base (this is what you want)
offset 128: dynent block
offset 232: playerstate block ← health lives in here at +4 from playerstate start
offset 440: playerent own fields
```

health is at `0xEC` (236) from that base. subtract that from any health
address and you always land at offset 0 = the object base = the physent
vtable pointer. this is why the same subtraction works for players AND bots.
same inheritance chain, same layout, same offsets. just different object
base addresses.

pointer arithmetic in C — these two lines are the same thing:

```c
SOMETHING* x = my_array[INDEX];
something* x = *(my_array+index);
```

the `*4` is implicit in C. its just the data type. when you add a number to
a pointer in C it gets multiplied by the size of the datatype its pointed to.


---

# ═══════════════════════════════════════════════════════
# ASSEMBLY READING — THREE QUESTIONS
# ═══════════════════════════════════════════════════════

when you open disassembly cold, ask three questions about every line:

**1. what does this instruction DO?**

```
mov  = copy a value
cmp  = compare two values (sets flags, doesn't store)
push = save to stack
sub  = subtract
add  = add
test = AND two values, check if result is zero
jmp/je/jne/jng = jump somewhere based on flags from cmp
```

**2. are there brackets?**

```
[register]         = go to that address and read what's there (dereference)
[register+offset]  = go to address+offset and read (struct field access)
[register+reg*4]   = go to address+(reg×4) and read (array index)
no brackets        = use the value directly, don't dereference
```

**3. what region is the value in?**

```
0x004xxxxx to 0x006xxxxx = inside ac_client.exe = static = global variable
0x001xxxxx = stack = temporary, ignore
everything else = heap = dynamic object
```

the three patterns to recognize in any game:

```asm
mov reg, [ac_client.exe+XXXXXX]   ← load global (static, never changes)
                                     THIS IS THE STATIC POINTER
mov reg, [reg+esi*4]              ← index into array (heap, changes every session)
mov reg, [reg+offset]             ← read field from object
```


---

# ═══════════════════════════════════════════════════════
# VERIFICATION CHECKLIST
# ═══════════════════════════════════════════════════════

found an address? subtract the offset to get base, then verify:

```
base + 0xEC  → should show health value
base + 0x077 → should show 0 (player) or 1 (bot)
base + 0x208 → should show name as string in memory view
base + 0x004 → should show X position (Float, changes as entity moves)
```

if all four check out → base confirmed.

CE struct guesser is a starting point only. it guesses types wrong constantly.
always cross reference against source code. the source is the ground truth.


---

# ═══════════════════════════════════════════════════════
# FINDING THE BOT
# ═══════════════════════════════════════════════════════

same methodology as our player but the static pointer leads to an array:

```
ac_client.exe+18AC04 → entity array
array[0] = null
array[1] = bot 1 object base  ← +4 offset from array start
array[2] = bot 2 object base
```

confirmed in assembly:

```asm
ac_client.exe+81AE0 - 8B 1D 04AC5800 - mov ebx,[ac_client.exe+18AC04] { (234908E8) }
```

![CE pointer chain showing entity list with offset 4 and name offset 205](image-3.png)

so now when we add the initial offset being 4 (the order matters its bottom
first and up last.) and when I change that 4 offset I change things within
that list. the second newest offset is 205 which is the offset for the name.
we have the entity list finally at last.

confirmed bot base this session: `1F5A18C8`

verified:

```
1F5A18C8 + 0xEC  = 1F5A19B4  → bot health ✓
1F5A18C8 + 0x077 = 1F5A193F  → type = 1 (ENT_BOT) ✓
1F5A18C8 + 0x205 = 1F5A1ACD  → bot name as string ✓
```

![CE showing type = 1 confirming ENT_BOT](image-1.png)

![CE memory view showing bot name string](image.png)

strings in C are known as C-style strings. they end in `00` which is a null
byte which indicates the end of the string.

![memory view showing string ending in null bytes](image-2.png)

botent class inherits from playerent so all offsets are the same. the only
difference is the type field — bots show 1, players show 0.

```cpp
class botent : public playerent
{
public:
    CBot *pBot;       // only used if this is a bot on host, NULL otherwise
    playerent *enemy; // monster wants to kill this entity
    float targetpitch;
    float targetyaw;
    botent() : pBot(NULL), enemy(NULL) { type = ENT_BOT; }
};
```

note from clientgame.cpp:
https://github.com/assaultcube/AC/blob/master/source/src/clientgame.cpp#L24

```cpp
playerent *player1 = newplayerent();   // our client — separate object
vector<playerent *> players;           // other clients — stored in array
```

our player is created as a separate object and NOT stored with the others.


---

# ═══════════════════════════════════════════════════════
# TYPE SIZES REFERENCE
# ═══════════════════════════════════════════════════════

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
                                       source: tools.h#L86
                                       #define MAXSTRLEN 260
                                       typedef char string[MAXSTRLEN]
                                       MAXNAMELEN=15 is content limit only
─────────────────────────────────────────────────────
padding rule:
  after bools before int/float → pad to next multiple of 4
  103 % 4 = 3 → 1 byte padding
  257 % 4 = 1 → 3 bytes padding

vtable ptr rule:
  any class with virtual keyword → 4 bytes at offset 0
  one ptr per class regardless of how many virtual functions

inheritance rule:
  inherited block ALWAYS comes before own fields
  playerent : public dynent, public playerstate
  → dynent (includes physent) first
  → playerstate second
  → playerent own fields last

lesson on non-standard types:
  before assigning a size to ANY non-primitive type
  ctrl+F for typedef, struct, class, #define to find actual definition
  never assume. the string mistake cost us 244 bytes on every field after name.
```


---

# ═══════════════════════════════════════════════════════
# STRUCT OFFSET MAPS
# ═══════════════════════════════════════════════════════

all offsets are measured from the object base (offset 0 = physent vtable ptr).
inheritance stacks in order: physent → dynent → playerstate → playerent → botent.

source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h


## physent (offset 0x000, size 128 bytes)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L105

```
0x000 (0)   → vtable ptr (4 bytes) — virtual keyword present
0x004 (4)   → o.x        (float)   — position X
0x008 (8)   → o.y        (float)   — position Y
0x00C (12)  → o.z        (float)   — position Z
0x010 (16)  → vel.x      (float)   — velocity X
0x014 (20)  → vel.y      (float)   — velocity Y
0x018 (24)  → vel.z      (float)   — velocity Z
0x01C (28)  → deltapos.x (float)
0x020 (32)  → deltapos.y (float)
0x024 (36)  → deltapos.z (float)
0x028 (40)  → newpos.x   (float)
0x02C (44)  → newpos.y   (float)
0x030 (48)  → newpos.z   (float)
0x034 (52)  → yaw        (float)
0x038 (56)  → pitch      (float)
0x03C (60)  → roll       (float)
0x040 (64)  → pitchvel   (float)
0x044 (68)  → maxspeed   (float)   — 24 for player
0x048 (72)  → timeinair  (int)
0x04C (76)  → radius     (float)
0x050 (80)  → eyeheight  (float)
0x054 (84)  → maxeyeheight (float)
0x058 (88)  → aboveeye   (float)
0x05C (92)  → inwater    (bool)
0x05D (93)  → onfloor    (bool)
0x05E (94)  → onladder   (bool)
0x05F (95)  → jumpnext   (bool)
0x060 (96)  → jumpd      (bool)
0x061 (97)  → crouching  (bool)
0x062 (98)  → crouchedinair (bool)
0x063 (99)  → trycrouch  (bool)
0x064 (100) → cancollide (bool)
0x065 (101) → stuck      (bool)
0x066 (102) → scoping    (bool)
0x067 (103) → [1 byte padding — 103 % 4 = 3 → pad to 104]
0x068 (104) → lastjump      (int)
0x06C (108) → lastjumpheight (float)
0x070 (112) → lastsplash    (int)
0x074 (116) → move          (char)
0x075 (117) → strafe        (char)
0x076 (118) → state         (uchar)  — CS_ALIVE=0 CS_DEAD=1
0x077 (119) → type          (uchar)  — ENT_PLAYER=0 ENT_BOT=1
0x078 (120) → eyeheightvel  (float)
0x07C (124) → last_pos      (int)
physent ends at 0x080 (128)
```


## dynent : physent (own fields start 0x080, size 232 bytes total)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L156

```
0x080 (128) → k_left   (bool)
0x081 (129) → k_right  (bool)
0x082 (130) → k_up     (bool)
0x083 (131) → k_down   (bool)
0x084 (132) → prev[0]     (animstate — 20 bytes)
0x098 (152) → prev[1]     (animstate — 20 bytes)
0x0AC (172) → current[0]  (animstate — 20 bytes)
0x0C0 (192) → current[1]  (animstate — 20 bytes)
0x0D4 (212) → lastanimswitchtime[0] (int)
0x0D8 (216) → lastanimswitchtime[1] (int)
0x0DC (220) → lastmodel[0] (void* — 4 bytes)
0x0E0 (224) → lastmodel[1] (void* — 4 bytes)
0x0E4 (228) → lastrendered (int)
dynent ends at 0x0E8 (232)
```


## playerstate (own vtable at 0x0E8, size 440 bytes total)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h

```
0x0E8 (232) → vtable ptr (4 bytes) — playerstate has its own virtual functions
0x0EC (236) → health     (int)  ← CE CONFIRMED — subtract from field address to get base
0x0F0 (240) → armour     (int)
0x0F4 (244) → primary    (int)  — GUN_ASSAULT default
0x0F8 (248) → nextprimary (int)
0x0FC (252) → gunselect  (int)
0x100 (256) → akimbo     (bool)
0x101 (257) → [3 bytes padding — 257 % 4 = 1 → pad to 260]
0x104 (260) → ammo[0]    (int)  — GUN_KNIFE
0x108 (264) → ammo[1]    (int)  — GUN_PISTOL
0x10C (268) → ammo[2]    (int)  — GUN_CARBINE
0x110 (272) → ammo[3]    (int)  — GUN_SHOTGUN
0x114 (276) → ammo[4]    (int)  — GUN_SUBGUN
0x118 (280) → ammo[5]    (int)  — GUN_SNIPER
0x11C (284) → ammo[6]    (int)  — GUN_ASSAULT
0x120 (288) → ammo[7]    (int)  — GUN_GRENADE
0x124 (292) → ammo[8]    (int)  — GUN_AKIMBO
0x128 (296) → mag[0]     (int)  — GUN_KNIFE
0x12C (300) → mag[1]     (int)  — GUN_PISTOL
0x130 (304) → mag[2]     (int)  — GUN_CARBINE
0x134 (308) → mag[3]     (int)  — GUN_SHOTGUN
0x138 (312) → mag[4]     (int)  — GUN_SUBGUN
0x13C (316) → mag[5]     (int)  — GUN_SNIPER
0x140 (320) → mag[6]     (int)  — GUN_ASSAULT
0x144 (324) → mag[7]     (int)  — GUN_GRENADE
0x148 (328) → mag[8]     (int)  — GUN_AKIMBO
0x14C (332) → gunwait[0] (int)  — GUN_KNIFE
0x150 (336) → gunwait[1] (int)  — GUN_PISTOL
0x154 (340) → gunwait[2] (int)  — GUN_CARBINE
0x158 (344) → gunwait[3] (int)  — GUN_SHOTGUN
0x15C (348) → gunwait[4] (int)  — GUN_SUBGUN
0x160 (352) → gunwait[5] (int)  — GUN_SNIPER
0x164 (356) → gunwait[6] (int)  — GUN_ASSAULT
0x168 (360) → gunwait[7] (int)  — GUN_GRENADE
0x16C (364) → gunwait[8] (int)  — GUN_AKIMBO
0x170 (368) → pstatshots[0..8]  (int[9] — 36 bytes)
0x194 (404) → pstatdamage[0..8] (int[9] — 36 bytes)
playerstate ends at 0x1B8 (440)
```


## playerent : dynent, playerstate (own fields start 0x1B8, size 1032 bytes total)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L399

```
0x1B8 (440) → curskin      (int)
0x1BC (444) → nextskin[0]  (int)
0x1C0 (448) → nextskin[1]  (int)
0x1C4 (452) → clientnum    (int)
0x1C8 (456) → lastupdate   (int)
0x1CC (460) → plag         (int)
0x1D0 (464) → ping         (int)
0x1D4 (468) → address      (enet_uint32 = 4 bytes)
0x1D8 (472) → lifesequence (int)
0x1DC (476) → frags        (int)
0x1E0 (480) → flagscore    (int)
0x1E4 (484) → deaths       (int)
0x1E8 (488) → tks          (int)
0x1EC (492) → lastaction   (int)
0x1F0 (496) → lastmove     (int)
0x1F4 (500) → lastpain     (int)
0x1F8 (504) → lastvoicecom (int)
0x1FC (508) → lastdeath    (int)
0x200 (512) → clientrole   (int)
0x204 (516) → attacking    (bool)
0x205 (517) → [3 bytes padding — string requires 4-byte alignment]

0x208 (520) → name         (string = char[260])
   ── NON-STANDARD SIZE — READ THIS ──
   string is NOT std::string and NOT char[16]
   typedef char string[MAXSTRLEN] where MAXSTRLEN = 260
   source: https://github.com/assaultcube/AC/blob/master/source/src/tools.h#L86
     #define MAXSTRLEN 260
     typedef char string[MAXSTRLEN];
   MAXNAMELEN = 15 is only the content limit at runtime
   storage is ALWAYS 260 bytes regardless of content length
   CE confirmed: weaponsel at 0x364 only works if name = 260 bytes
   lesson: ctrl+F for typedef before assigning size to any named type
   name ends at 0x30C (520 + 260 = 780)

0x30C (780) → team         (int)  ← CE CONFIRMED 0=TEAM_CLA 1=TEAM_RVSF
0x310 (784) → weaponchanging (int)
0x314 (788) → nextweapon   (int)
0x318 (792) → spectatemode (int)
0x31C (796) → followplayercn (int)
0x320 (800) → eardamagemillis (int)
0x324 (804) → maxroll      (float)
0x328 (808) → maxrolleffect (float)
0x32C (812) → movroll      (float)
0x330 (816) → effroll      (float)
0x334 (820) → ffov         (int)
0x338 (824) → scopefov     (int)

0x33C (828) → weapons[0]   (weapon* — pointer to knife object)
0x340 (832) → weapons[1]   (weapon* — pointer to pistol object)
0x344 (836) → weapons[2]   (weapon* — pointer to carbine object)
0x348 (840) → weapons[3]   (weapon* — pointer to shotgun object)
0x34C (844) → weapons[4]   (weapon* — pointer to smg object)
0x350 (848) → weapons[5]   (weapon* — pointer to sniper object)
0x354 (852) → weapons[6]   (weapon* — pointer to assault object)
0x358 (856) → weapons[7]   (weapon* — pointer to grenade object)
0x35C (860) → weapons[8]   (weapon* — pointer to akimbo object)
   ── NOTE ──
   NUMGUNS = 9, each entry is a POINTER (4 bytes) to a separate heap object
   source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L59
   all 9 weapon objects exist in memory from spawn regardless of what is equipped

0x360 (864) → prevweaponsel   (weapon*)
0x364 (868) → weaponsel       (weapon*) ← CE CONFIRMED
0x368 (872) → nextweaponsel   (weapon*)
0x36C (876) → primweap        (weapon*)
0x370 (880) → nextprimweap    (weapon*)
0x374 (884) → lastattackweapon (weapon*)
   these are all separate weapon pointers to separate heap objects
   weaponsel changes when you switch weapon — points to whichever is active

0x378 (888) → history         (poshist — 96 bytes)
   ── NON-STANDARD SIZE — READ THIS ──
   source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L299
     struct poshist {
         int nextupdate;  → 4 bytes
         int curpos;      → 4 bytes
         int numpos;      → 4 bytes
         vec pos[7];      → 7 × 12 = 84 bytes
     };                   → total = 96 bytes
   #define POSHIST_SIZE 7
   source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L297
   history ends at 0x3D8 (888 + 96 = 984)

0x3D8 (984)  → skin_noteam   (const char* — pointer, 4 bytes)
0x3DC (988)  → skin_cla      (const char*)
0x3E0 (992)  → skin_rvsf     (const char*)
0x3E4 (996)  → deltayaw      (float)
0x3E8 (1000) → deltapitch    (float)
0x3EC (1004) → newyaw        (float)
0x3F0 (1008) → newpitch      (float)
0x3F4 (1012) → smoothmillis  (int)
0x3F8 (1016) → head.x        (float)
0x3FC (1020) → head.y        (float)
0x400 (1024) → head.z        (float)
0x404 (1028) → ignored       (bool)
0x405 (1029) → muted         (bool)
0x406 (1030) → nocorpse      (bool)
0x407 (1031) → [1 byte padding]
playerent ends at 0x408 (1032)
```


## botent : playerent (own fields start 0x408, size 1048 bytes total)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L638

```
0x408 (1032) → pBot        (CBot* — pointer)
0x40C (1036) → enemy       (playerent* — pointer)
0x410 (1040) → targetpitch (float)
0x414 (1044) → targetyaw   (float)
botent ends at 0x418 (1048)
```


## weapon struct
# source: https://github.com/assaultcube/AC/blob/master/source/src/weapon.h

```
0x000 (0)  → vtable ptr   (4 bytes)
0x004 (4)  → type         (int)     — gun enum 0=knife 1=pistol 5=sniper etc
0x008 (8)  → owner        (playerent* — pointer back to owning player)
0x00C (12) → info         (guninfo& — reference = pointer to guninfo, 4 bytes)
               ── NOTE ──
               points to a global static array guns[NUMGUNS] in ac_client.exe
               all players with same gun share the same guninfo entry
               source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L74
0x010 (16) → &ammo        (int& — reference into playerstate ammo[], 4 bytes)
0x014 (20) → &mag         (int& — reference into playerstate mag[], 4 bytes)
0x018 (24) → &gunwait     (int& — reference into playerstate gunwait[], 4 bytes)
0x01C (28) → shots        (int)
```


## guninfo struct (global static array, shared by all players)
# source: https://github.com/assaultcube/AC/blob/master/source/src/entity.h#L74

```
0x00 (0)  → modelname[23]  (char[23])  — "sniper", "pistol" etc
0x17 (23) → title[42]      (char[42])  — display name
0x41 (65) → sound          (short)
0x43 (67) → reload         (short)
0x45 (69) → reloadtime     (short)
0x47 (71) → attackdelay    (short)
0x49 (73) → damage         (short)     ← damage per shot
0x4B (75) → piercing       (short)
0x4D (77) → projspeed      (short)
0x4F (79) → part           (short)
0x51 (81) → spread         (short)
0x53 (83) → recoil         (short)
0x55 (85) → magsize        (short)     ← magazine size
0x57      → mdl_kick_rot   (short)
0x59      → mdl_kick_back  (short)
0x5B      → recoilincrease (short)
0x5D      → recoilbase     (short)
0x5F      → maxrecoil      (short)
0x61      → recoilbackfade (short)
0x63      → pushfactor     (short)
0x65      → isauto         (bool)
```

full pointer chain from playerent base to weapon stats:

```
playerent base + 0x364  → weaponsel (pointer to weapon object)
weapon object  + 0x4    → type (which gun: 0=knife 5=sniper etc)
weapon object  + 0xC    → info (pointer to guninfo entry)
guninfo        + 0x00   → modelname ("sniper" "pistol" etc)
guninfo        + 0x49   → damage (short, 2 bytes)
guninfo        + 0x53   → recoil (short, 2 bytes)
guninfo        + 0x55   → magsize (short, 2 bytes)
```

in CE (bottom to top):

```
ac_client.exe+18AC04   ← entity array static pointer
    ↓ +4               ← first entity = bot 1 base
    ↓ +0x364           ← weaponsel in playerent
    ↓ +0xC             ← info in weapon object
    ↓ +0x0             ← modelname in guninfo
→ "sniper"
```

---

# ═══════════════════════════════════════════════════════
# CONFIRMED STATIC POINTERS FOR ASSAULTCUBE
# ═══════════════════════════════════════════════════════

```
ac_client.exe+18AC00 → player1 object base (our player)
ac_client.exe+18AC04 → entity array base (bots and other players)
```

confirmed in assembly:

```asm
ac_client.exe+61489 - A1 00AC5800 - mov eax,[ac_client.exe+18AC00] { (00821120) }
ac_client.exe+81AE0 - 8B 1D 04AC5800 - mov ebx,[ac_client.exe+18AC04] { (234908E8) }
```

confirmed offsets:

```
health offset from object base: EC
name offset from object base:   208
type offset from object base:   077  (0=player 1=bot)
position X from object base:    004
weaponsel from object base:     364  (CE confirmed)
team from object base:          30C  (CE confirmed, 0=CLA 1=RVSF)
```