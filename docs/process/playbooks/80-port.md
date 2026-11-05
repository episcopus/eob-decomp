# 80 The portable build (Phase 7)

**Inputs:** SPEC O5 and the non-goals (section 1): the matched C, a platform layer replacing DOS, video
and sound, SDL3, poolrad's platform layer as the start, the original's behaviour kept with its quirks so
the goldens stay an exact test (the human, 2026-11-04). Whitepaper 2.5 (the graphics drivers' 47 slots),
2.6 (start-up), 2.7 (input and sound). Poolrad's C translation, `../poolrad-decomp/c/` (read only):
`c/include/platform.h` (its only boundary with the machine), `c/platform/` (`sdl_host.c`, `replay.c`,
`null.c`), `c/rtl/`, `c/README.md`; its SPEC 7.3 to 7.5 (the DGROUP mirror, `farptr` and `FP()`), 8.4
(hybrid execution) and `tools/oracle/replay.py`. The human's answers to the decisions, 2026-11-04
(section "Decisions").

**Goal:** `EOB.EXE` built natively from `src/`, unchanged, with SDL3 for the window, the input and the
sound, and every golden session of `EOB.EXE` playing on it with the original's checkpoints, screens,
sound and state.

| Step | State |
|---|---|
| M0 skeleton | done 2026-11-04: `make port` translates `src/E0CF7.C` and `src/E1658.C`, links them with the arena runtime twice, headless (`build/port/eob-port-headless`, no SDL) and with the SDL3 host (`build/port/eob-port`), and the self-test runs them headless on `EOB.EXE`'s own DGROUP (`tests/unit/test_port.py`, fast set) |
| Decisions | the nine answered by the human 2026-11-04, the five follow-ups (A to E) 2026-11-05 (section "Decisions"). Linux and Windows deferred by the human 2026-11-05: macOS only for now. Left for the human: install the newest LLVM (section "What the human installs") |
| M1 tc2c for every C unit | done 2026-11-05 except the LLVM pin, which waits for the human's install: `make port` translates all 41 C units of `EOB.EXE` (609 functions) with none refused, compiles them with `-Werror` and links them; what is still a stub is 63 assembly routines (M3) and 23 library functions that need the machine (section "M1") |
| M2 to M7 | not started (section "Order of work") |

## Why `src/` does not compile natively as it is

The matched C is Turbo C++ 1.00 large-model C, and a host compiler gives the same text another meaning:

| | Turbo C++ `-ml` | clang or gcc on a 64-bit host |
|---|---|---|
| `int`, `unsigned` | 16 bits; arithmetic wraps at 16 bits; `unsigned` stays unsigned in a compare with `int` | 32 bits; a `short` promotes to `int`, so `u < i` and overflow behave otherwise |
| `long` | 32 bits | 64 bits (LP64) |
| every data pointer (`far` or not) | 4 bytes, segment:offset; `p + n` changes the offset only and wraps at 64 KB | 8 bytes, flat |
| structs | packed on bytes (no `-a`); `struct Node` is 3Ch bytes with its far pointers (the 60-byte grid) | other offsets and sizes |
| globals | at fixed DGROUP offsets (the `/*@ ds:XXXX */` annotations), initialised by the EXE's image | wherever the linker puts them |

So `#define far` and a native compile would build, and run wrongly: DGROUP's tables of far pointers
(`gTable1a`, the driver slot table, the menu tables of EOV5) hold 4-byte segment:offset values; the
records the game reads from its files and `EOBDATA.SAV` have 16-bit fields and 4-byte pointers; and
16-bit wraparound is part of the behaviour the goldens test. "Compiles natively unchanged" therefore means:
**the files in `src/` are never edited for the port; the port build translates them**, and every
difference above is resolved by the translation, the same way for every unit.

## The arena (the memory model, decision 1: chosen)

The port keeps poolrad's model (its SPEC 7.3: DGROUP mirrored byte for byte, heap records behind
`farptr` handles, `FP()` into a 1 MB arena) and takes it one step further, because here no C is written
by hand: the arena **is** the real-mode address space.

- `tc_mem`, 1 MB (`port/include/tcport.h`). `EOB.EXE`'s load module is loaded into it at `1000:0000`
  with its 2,504 relocations applied, as DOS and the oracle load it (`tools/oracle/machine.py`
  `LOAD_SEG`; `tc_load_program` in `port/runtime/arena.c`). DGROUP is `312E` there, and its far pointer
  tables already point at the right records.
- A pointer is a `farptr`: a `u32` with the segment in the high word, as Turbo C stores it. `MK_FP`,
  `FP_SEG` and `FP_OFF` are the identity on that representation. Far pointer arithmetic adds to the
  offset only (`tc_fp_add`). A load or store goes through `tc_ld_*` and `tc_st_*` (little-endian, any
  alignment, the second byte of a word at `xxxx:FFFF` at `xxxx:0000`).
- A global is read and written at `TC_MKFP(tc_ds, annotation)`; a far object of another segment
  (`/*@ 0000:3B36 */`) at its segment moved by the load segment.
- A function of `src/` is registered under its original far address (`tc_code_register`), so a far
  function pointer read from the arena (a node's `callback30`, a driver slot, an interrupt vector) finds
  its C function (`tc_code_lookup`).
- Locals and parameters whose address is never taken are ordinary C variables of the exact width.
  Address-taken locals, local arrays and local structs (an `&x` passed to a callee, `union REGS` on the
  stack) get a frame on an arena stack in the original's stack segment (the header's `SS`), as Turbo C
  would give them one (`tc_frame_enter`; M1).

Why this model: every byte the game keeps is where and as the original keeps it, so

1. saves, data files and DGROUP tables need no conversion;
2. the arena can be compared with the oracle's memory directly: the goldens' state digests
   (`Machine.state_digest`, memory `0500h` to `A0000h` in the original's form) apply to the port with
   only the stack and the overlay buffer masked;
3. poolrad's hybrid execution comes for free: the oracle already maps its RAM from a host buffer
   (`uc.mem_map_ptr`, `machine.py`), so a translated function built as a shared library can run on the
   oracle's own memory, called where the original is, for a differential test of one function or one
   unit (M2).

The cost is speed (every access goes through a helper; the game ran on an 8 MHz 286, so this is far from
a concern) and generated code nobody reads: the readable source stays `src/`.

The alternative (decision 1, option B, not taken) was native C with flat pointers and serialisers for
the files. It needs a second, hand-maintained source, and its state cannot be compared with the oracle
byte for byte, so the goldens would test less.

## The translator, `tools/port/tc2c.py` (decision 2: chosen)

`tc2c.py` reads a unit through clang's typed AST and writes C99 for the arena:

- clang parses the unit as C for the **MSP430** target, used for its front end only: `int` 16 bits,
  `long` 32 bits, as Turbo C++. With `-fsigned-char` (no `-K`) and `#pragma pack(1)` (no `-a`)
  (`port/include/tc/prelude.h`), clang applies C's conversion rules with Turbo C's widths, and every
  promotion and usual arithmetic conversion is an explicit node of the AST (`ImplicitCastExpr`). The
  skeleton was built with Apple clang 17.0.0; the reference is to be one pinned LLVM release on every
  platform (decision 2): the newest major release when the human installs it (question B); when Linux and
  Windows come back, one that all three offer.
- `far`, `near` and `huge` are dropped for parsing: in the large model every data pointer is far, so
  tc2c gives every pointer 4 bytes itself and computes every struct layout itself (bytes, no padding,
  unions at offset 0). clang's own sizes (2-byte MSP430 pointers) are never used.
- It writes each value with an exact width (`u8 s8 u16 s16 u32 s32`, `farptr`), each implicit
  conversion as a cast, and `+ - * <<` through `u32` and back to the result type, so the 16-bit
  wraparound is the 8086's and no host overflow is undefined.
- It reuses the matcher's annotation parser (`cmatch.parse_annotations`, imported, never changed) for
  the addresses of globals and functions.
- What it does not handle stops it with the construct and the line (`tc2c: unsupported: X.C:4: a struct
  passed by value (s)`); a unit joins `port/units.txt` only when every construct it uses is handled. A function called but defined by no translated unit is linked as a stub that names itself and
  stops (`build/port/gen/unported.c`), so the worklist is always printed.

The generated C uses GNU statement expressions (compound assignment, `++` and `--` evaluate the address
once); clang and gcc build it, MSVC does not (decision 2: allowed; a Windows build, when it comes, uses clang).

Other ways considered:

- **C++ wrapper types** (`#define far` plus classes `FarPtr<T>` and a 16-bit `Int`): only a lexical
  rewrite, but every pointer of the large model is far, not only those written `far`; an address-taken
  local has no segment:offset; and a class passed through `...` is not an `int`. Each needs a special
  case, so the result is approximate exactly where the goldens look.
- **A hand-written portable copy** (poolrad's way, or Exhume's): two sources that drift, and the matched
  C stops being what runs.

### How each Turbo C construct translates

| Construct in `src/` | In the port | State |
|---|---|---|
| `int`, `unsigned`, `char`, `long`, enums | `s16 u16 s8 u32/s32 s16`, casts from clang's AST | skeleton |
| `T far *p`, `T *p` | `farptr`; loads and stores in the arena | skeleton |
| `p->f`, `s.f`, `a[i]` | offset added to the pointer's offset; `i * size` computed in 16 bits | skeleton |
| `p + n`, `p++` | offset only, wrapping at 64 KB | skeleton |
| `p == q`, `if (p)` | all 32 bits (null is `0000:0000`) | skeleton |
| `p < q` | the offsets only [H]: settle by compiling a probe with Turbo C++ 1.00 (rule 1) | skeleton, [H] |
| far function pointer call | `tc_code_lookup(address)` then the C function | skeleton |
| `p - q` | the offsets' difference over the element size [H] | M1, [H] |
| `MK_FP`, `FP_SEG`, `FP_OFF` | the identity on `farptr` (`TC_MKFP`, `TC_SEG`, `TC_OFF`); in an overlay unit a constant segment is TLINK's segment-table index times 8 (`EOV3.C`'s `MK_FP(0x1d8, 0)`, entry 59), resolved through `build/re/eob.json` | M1 |
| string literals | at the DGROUP offset the original's code uses: `port/literals.json`, recorded by the matcher (`make port-literals`), checked against `src/` on every build and against the original's bytes by the fast tests; a unit the table does not list falls back to a search by content | M1 |
| global definitions with initialisers | the loaded image holds them (until step D); tc2c translates no global definition | M1; step D |
| address-taken locals and parameters, local arrays and structs, struct copies and initialiser lists | a frame on the arena stack, freed on return by the cleanup attribute; struct copy as a byte copy (`tc_copy`); an uninitialised C local starts at 0 (the original's is the stack's leftover) [H] | M1 |
| `switch`, `goto`, `?:`, `,`, `sizeof` | the same in C; `sizeof` and every constant computed by tc2c with Turbo C's sizes | M1 |
| enum constants, typedefs of untagged structs and unions (`E1B82.C`'s `WorkBuffer`) | from the AST | M1 |
| a call whose caller and callee declare the arguments differently (`E14EF.C`'s `UpdateOptionList`: two `int`s where callers push one `long`) | laid out as the caller pushes them, read back as the callee's parameters (`call_args`); a different number of argument bytes stops tc2c | M1 |
| a call to a name no unit defines, whose address is a function another unit defines under another name (`DrawViewRows` and `NoOpAfterMapLoad` at overlay 4's stub entries) | the call goes to the function at that address (`resolve_aliases`) | M1 |
| a value read from a call to a function defined `void` | 0 [H] (the original reads whatever AX held) | M1 |
| the Borland library (`strlen`, `sprintf`, `open`, `int86`, ...) | called as `tcl_<name>`: the runtime (`port/include/tclib.h`, `port/runtime/tclib.c`) has the string, memory, conversion and `sprintf` functions and `getvect`/`setvect` with Turbo C's behaviour; the 23 that need the machine (files, the far heap, `int86`, `exit`, `harderr`) or keep library state in DGROUP (`rand`'s seed) are stubs until M3. A variadic call's arguments are laid out as Turbo C pushes them | M1, M3 |
| `void interrupt (far *)()` vector types | far code pointers; `getvect` and `setvect` on the arena's vector table at `0000:0000` | M1 |
| `/*@flags ... */` | no unit uses `-K`; `-G` and `-v` change only code; `-P` (`E1B82.C`, C++) uses C++ only for `extern "C" { }`, which tc2c blanks before reading the unit as C | M1 |
| the library headers (`dos.h`, `stdio.h`, `alloc.h`, `fcntl.h`, `mem.h`) | the port's own, `port/include/tc/sys/`, with Turbo C++ 1.00's constants and layouts (checked against its headers, not copied) | M1 |

## The overlays as plain code

VROOMM pages overlay code in and out of a buffer; natively there is nothing to page. Overlay units
(`EOV0.C` to `EOV6.C`, `IOV0.C`) translate like resident units and their calls are direct calls. Four
things remain:

1. A far pointer to an overlay function holds its **stub entry** (stub segment, `CD 3F` entry), not its
   code (whitepaper 2.3). tc2c registers each overlay function under its stub entry address, found from
   its `ovlN:` annotation and the stub table, so a far call through such a pointer finds it.
2. The overlay manager (`E1E8B.ASM`) allocates its buffer (`_ovrbuffer`, 02CBh paragraphs) at start-up.
   The port makes the same allocation so the DOS memory chain, which `E1990.ASM` walks, is the
   original's; the buffer's contents are masked in comparisons (they hold whatever overlay was last
   loaded).
3. `EOV3.C`'s `MK_FP(0x1d8, 0)` is a segment-table index (entry 59, `gWallSetCgaMaps`; whitepaper 2.6),
   which the overlay manager's fixups turn into a segment. Translated literally it is segment 01D8h. tc2c
   needs a rule for overlay units: a constant segment the original fixes up is resolved through
   TLINK's segment table (`build/re/eob.json`). Not a source change (open questions).
4. Overlay 5's `gScrollHook` fixups (SPEC O2's accepted difference) are a property of the rebuild's
   link, not of the C; the port registers `MessagePanel` at the address the original stores.

## Platform separation

As in poolrad, where `c/include/platform.h` is "the only boundary between the game and the machine it
runs on", one header separates everything portable from the host, and only one directory sees the host:

| Layer | Where | Uses | Never uses |
|---|---|---|---|
| The game | `build/port/gen/` (tc2c's output from `src/`); the hand-written assembly routines, `port/asm/` (decision 3, M3) | the arena, the runtime's interface | the host, SDL, the wall clock |
| The runtime and the machine | `port/runtime/`: the arena, the loader, the code table; from M3 the PC the game expects (vector table, PIT, ports, INT 33h, INT 10h, DOS) | the arena, `platform.h` | SDL or any other host header |
| The platform | `port/platform/`, behind `port/include/platform.h` | the host's libraries | the arena, the game's types |

The implementations of `platform.h`, as poolrad's three:

- `port/platform/sdl_host.c`: the game you play (window, set 1 scancodes, mouse, wall clock; sound at
  M7), linked into `build/port/eob-port`.
- `port/platform/null.c`: headless, no input, a clock only `platform_sleep_ms` advances; linked into
  `build/port/eob-port-headless`, on which the self-test runs and on which M2's per-function tests build.
- `port/platform/replay.c` (M5): headless; answers from a golden's recording and checks its checkpoints.

Another host (another windowing library, a console, a browser through Emscripten) is one more file in
`port/platform/`. The rules, which the build checks:

1. Only `port/platform/` includes or calls SDL. `make port` runs `port-boundary` first and
   `tests/unit/test_port.py` checks the same; the game and the runtime are compiled once without SDL's
   include path and linked with each platform.
2. `platform.h` passes plain values: pixels and the DAC, scancodes, the mouse in the INT 33h driver's
   range, milliseconds, OPL register writes, file bytes. It never passes the arena or a game type.
3. The machine, not the platform, is what a replay records and plays back: the platform delivers
   hardware events, the machine turns them into interrupts and port reads. So a recording is the same
   whichever host made it, and the machine follows the oracle's own models (`tools/oracle/dos.py`,
   `bios.py`, `input.py`, `adlib.py`, `ega.py`), which already make the goldens deterministic.
4. Portable C99 with the fixed-width types of `tcport.h`, plus GNU statement expressions in the
   generated code (decision 2; poolrad's C11 has no extension); no floating point; files only through
   `platform_file_*`.

Where the two ports differ: Pool of Radiance asks the BIOS for keys and ticks, so poolrad's `platform.h`
has those reads as calls (`platform_key_peek`, `platform_pit_counter`, `platform_timer_set`). Eye of the
Beholder drives its own hardware (whitepaper 2.7): an INT 9 handler reads port 60h, an INT 33h handler
takes the mouse, INT 8 and INT 1Ch handlers run its sound and timers. So here the platform produces
hardware events and consumes hardware output, and the machine layer between it and the game is the PC.
Poolrad's host queues BIOS key codes (INT 16h); this one queues scancodes, because `EOB.EXE` never calls
INT 16h.

### What the machine replaces

What each original mechanism becomes:

| Original | Where in `src/` | Port |
|---|---|---|
| Graphics drivers, 47 slots: 7 imports the game provides, 40 exports (whitepaper 2.5) | `E1C00.ASM` loads `*.OVL` with DOS 4B03h and calls `LinkDriver`; the game calls the slots at `ds:8046` | The machine's 4B03h allocates the block as DOS does (the driver's size in paragraphs) and fills the driver's export table with the code addresses of the port's driver functions, registered at `loadseg:offset` of each export, so the game's slot table holds the far pointers it holds in the oracle. MCGA only (decision 4) |
| Video memory and palette | MCGA draws page 0 at `A000:0000`; DAC through INT 10h AX=1012h and ports 3C8h, 3C9h (`I0921.ASM`); retrace port 3DAh (`E1DE4.ASM`); mode set through `int86` (`E1DDD.C`) | `A000:0000` is arena memory; the machine keeps the DAC; the host presents both. Port 3DAh is a read the replay records |
| Mouse, INT 33h | `E1A72.ASM` (detect, reset, position, handler at `1A72:017A` for mask 1Fh), `E1BF3.ASM` (function 4), the pointer drawn by the game (`E1BBE.ASM`) | The machine is the INT 33h driver: functions 0, 1, 3, 4, 0Ch, with the vector not null so the game finds it; host mouse events become calls of the game's handler with the driver's registers |
| Keyboard, INT 9 | `E1AAD.ASM`: reads port 60h, acknowledges the PIC, its 256-byte ring | Host scancodes become INT 9 deliveries; port 60h returns the scancode; port 20h is accepted |
| Timer, INT 8 and INT 1Ch | `E_TEXT.ASM` (AdLib timer, PIT divisor 0555h; the sample player, 004Bh), `E1948.ASM` (speaker and Tandy), `E1D8F.ASM` (INT 1Ch) | The machine runs the PIT at the programmed divisor and delivers INT 8 through the arena's vector, the chain to the BIOS handler raising INT 1Ch, as in the oracle |
| AdLib, ports 388h and 389h | `E_TEXT.ASM` | `platform_adlib_write`; the SDL host synthesises with ymfm (decision 7, M7); the sound digest compares the register stream |
| PC speaker, Tandy | `E1948.ASM`, ports 42h, 43h, 61h, C0h | `platform_speaker`; Tandy later |
| DOS files | the library's `_open`, `_read` (`E1A47.C`), `LoadFile`, `FileRead` | INT 21h 3Dh to 42h over `platform_file_*`, in a copy of the game's directory (never `eob/` itself) |
| DOS memory | `MemAlloc` (`E19BC.C`), `E1990.ASM`, which walks the MCB chain itself | INT 21h 48h, 49h, 4Ah with MCBs in the arena, laid out as `tools/oracle/dos.py` lays them |
| Console, drives | `E19EA.ASM` (DOS 06h), `E1A19.ASM` (19h, 0Eh) | the machine's INT 21h |

## The assembly units (decision 3: hand-written C)

Nineteen assembly units of `EOB.EXE` need an equivalent (five more in `INTRO.EXE`, `S_TEXT.ASM` in
`START1.EXE`):

| Unit | What | Touches hardware |
|---|---|---|
| `E_TEXT.ASM` | the AdLib driver, two INT 8 handlers, its script interpreter; runs with DS = CS | PIT, 388h/389h |
| `E17A6.ASM` | graphics support: tables, `DrawTile`, the sprite unpackers | EGA ports, INT 10h |
| `E1948.ASM` | PC speaker and Tandy driver, an INT 8 handler | PIT, 61h, C0h |
| `E1990.ASM` | the DOS memory chain | the MCBs |
| `E19EA.ASM`, `E1A19.ASM` | console string, drives | INT 21h |
| `E19FC.ASM`, `E1BA8.ASM`, `E1D8C.ASM`, `E1D9A.ASM` | LCW encoder, far copy, the 24-bit random step, the four unpackers | no |
| `E1A72.ASM`, `E1BF3.ASM`, `E1BB1.ASM`, `E1BBE.ASM` | INT 33h glue, the cursor, the game's own pointer | INT 33h |
| `E1AAD.ASM` | the keyboard driver, INT 9 | 60h, 20h |
| `E1C00.ASM` | the driver loader | INT 21h 4B03h |
| `E1D8F.ASM` | INT 1Ch | the vector table |
| `E1DE4.ASM` | retrace | 3DAh |
| `E1E8B.ASM` | the VROOMM overlay manager | INT 3Fh, INT 21h; natively only its allocation remains |

The human chose **hand-written C**, one file per unit in `port/asm/` (`port/asm/E_TEXT.c` and so on), and
the same for `MCGA.ASM` (decision 4). How:

- Each routine is written from the matched TASM source in `src/`, which stays the reference; its file
  names the routine's original address, and the port registers it there (`tc_code_register`), so far
  pointers to it (driver slots, interrupt vectors, `callback30`) find it.
- A routine takes and returns what the C prototype in `src/` says (`cdecl`, large model). Routines with
  register conventions C cannot state (the four unpackers of `E1D8C.ASM`, `DrawTile`, the INT 33h
  handler at `1A72:017A`, the driver slots) get a small register record in the machine layer, filled
  and read by the caller's side as the original's registers would be.
- Variables an assembly unit keeps in its code segment (`cs:` variables; `E_TEXT.ASM` runs with
  DS = CS) stay in the arena at their original addresses, so the state digests see them.
- Each routine is proven against the original by the per-function test of M2, on the states the
  goldens reach, before a session test relies on it.
- The interrupt handlers (INT 8 in `E_TEXT.ASM` and `E1948.ASM`, INT 9 in `E1AAD.ASM`, INT 1Ch in
  `E1D8F.ASM`) and the waits on the hardware (the retrace in `E1DE4.ASM`) are where hand-written code
  meets the proof's clock; see "The clock with hand-written assembly".

The alternative not taken was `tools/port/asm2c.py`, recompiling the TASM source instruction by
instruction over an emulated register file: exact by construction, but unreadable.

## How the port is proven

The goldens (`tests/sessions/golden/<name>.json`) are keyed by instruction counts: each checkpoint is
"after N instructions of the original". C executes none of the original's instructions, so the port needs
a clock both sides can count. Three choices were offered (decision 5); the human chose the third:

1. **Reads of the outside world** (poolrad's `replay.py`, its SPEC 8.4): each read of the keyboard, the
   clock or a port is recorded with the number of timer interrupts raised by then; a replay returns the
   recorded values and raises the interrupts at reads. For EOB the interrupt handlers also write memory
   the game polls (tick counters, the key ring), so those reads would count too, and Turbo C's `-Z` (load
   suppression) means the compiled code may read a global fewer times than the C text does: the counts
   would differ between the oracle and the port.
2. **Control events**: function entries and loop back edges, counted in the oracle (calls to known
   addresses and taken backward branches in game code) and in the port (tc2c emits a count at each
   function entry and loop head). A spin loop on a timer variable is then exact; whether every C loop is
   one backward branch per iteration in the compiled code is checkable per function against the original.
3. **The original's instruction clock** (chosen): the matched code has the same control flow as the
   original's, block for block, so tc2c can give each basic block of its output the number of
   instructions the original's block executes (mapped through the matcher, which has both). The port
   then runs on the oracle's virtual clock: the timer interrupts fire at the same counts, and the
   goldens are checked as they are, the rebuild's way (`make golden CHECK=1`). An interrupt the oracle
   delivers inside a block is delivered at the end of that block in the port; that is exact unless the
   block itself reads what the handler wrote, which the goldens then show.

### The clock with hand-written assembly (decisions 3 and 5)

Hand-written C runs none of the original's instructions and has no blocks the matcher can map, and the
Borland library (which the port's runtime replaces) has none either. The rule (question A, chosen by the
human 2026-11-05):
**the clock counts only the instructions of the C units.**

- In the oracle: the clock advances on an instruction whose `CS:IP` lies in a C unit's code (resident
  or overlay, from TLINK's segment table, `build/re/eob.json`); instructions in assembly units, in the
  library and in interrupt handlers do not advance it. This is a counter added to the oracle's tools
  (`tools/oracle/`), not a change to the goldens; their instruction-count checkpoints are mapped to it
  once, when the recording file is made.
- In the port: tc2c's blocks add their counts; hand-written routines and the runtime add nothing.
- Each interrupt and input is recorded at its position on this clock, with its order among the others at
  the same position (the recording file, decision 5).
- While assembly or library code runs, the C clock stands still but interrupts still arrive in the
  oracle (a retrace wait spins through many INT 8s). The port's hand-written routine waits on the same
  condition (the tick counter, the retrace bit) and, while it waits, the machine delivers the interrupts
  recorded at that position, in order, until the condition holds. The condition depends only on what
  the handlers write, so the count delivered is the oracle's.
- What this cannot reproduce: an interrupt landing midway through an assembly routine that reads what
  the handler writes. The goldens show it if it happens; none is known.

The alternative not taken was to recompile just the handlers and the hardware waits (`E_TEXT.ASM`,
`E1948.ASM`, `E1AAD.ASM`, `E1D8F.ASM`, `E1DE4.ASM`) with asm2c and hand-write the rest.

Whatever the clock, the proof has two levels:

- **Per function (M2)**: `make port-hybrid` builds the translated units as a shared library;
  `tools/port/hybrid.py` points `tc_mem` at the oracle's RAM, stops a golden at a call of a translated
  function, runs the C on the same memory, runs the original on a copy, and compares memory and the
  return registers. Every function of `src/` can be checked so, on states the goldens actually reach,
  before anything else of the port exists.
- **Per session (M5, M6)**: the port runs a golden headless, inputs from the recording, and its
  checkpoints are compared with the golden's: state digest of the arena (stack and overlay buffer
  masked, the rest as `Machine.canonical` does), the screen (`A000` and the DAC in the form `ega.py`
  digests), and the AdLib register stream. The first checkpoint that differs names the session time;
  the per-function test then finds the function.

The new recording artefacts (per golden: the clock positions of each interrupt and input) are files
beside the goldens, `tests/sessions/golden/<name>.clock.json`; the human approved this change to the
golden format (decision 5). The goldens themselves do not change.

## M1: every C unit (done 2026-11-05)

```
make port              # translates all of port/units.txt; needs build/re/eob.json (make overlays)
make port-literals     # when src/ changes a literal: port/literals.json from the matcher (Turbo C++)
```

- `port/units.txt` lists the 41 C units of `EOB.EXE`; tc2c translates them together (609 functions)
  and refuses none. `ported.h` has one prototype per function (its definition's; for a function no
  unit defines, a variadic declaration first, then one that returns a value); `unported.c` has the
  rest, each a stub that names its address and its kind: 63 assembly routines (`port/asm/`, M3) and 23
  library functions (M3).
- The string literals: `port/literals.json` places all 149 of the 19 units that have any where the
  matcher saw the original's code refer to them. The search by content agrees on 147; the two it got
  wrong (an empty string in `E07A9.C`, `"\r"` in `E122E.C`) are why the table exists.
- The tests (`tests/unit/test_port.py`, fast set): the build and the self-test (every unit
  translated), a probe unit with the M1 constructs translated, compiled and run with Turbo C's results
  (frames, struct copy, a `switch` that falls through, `goto`, 16-bit wraparound before a compare, signed
  `char`) and the runtime library on arena strings, the literal table against `src/` and the original,
  and a struct passed by value refused by name. A mutation that drops the 16-bit wrap fails the probe.
- Not done: the pinned LLVM in `make doctor` (the human installs it; Apple clang 17.0.0 meanwhile).
- Found on the way, for the P6 vocabulary (not changed here, `src/` is under review): two overlay 4
  functions have two names, the callers' and the definition's: `DrawViewRows` (`E0EF3.C`, `EOV3.C`, at the
  stub entry 2116:0034) is `EOV4.C`'s `DrawCompass` (ovl4:1EDE), and `NoOpAfterMapLoad` (`E0ACD.C`,
  `E122E.C`, 2116:0043) is `EOV4.C`'s `PreloadViewOverlay` (ovl4:2039).

## The skeleton (M0)

```
make port
```

- `tools/port/tc2c.py` translates the units of `port/units.txt` into `build/port/gen/` (`E0CF7.c`,
  `E1658.c`, `units.c` (the init table), `ported.h` (prototypes), `unported.c` (the stub for `DrawNode`,
  `16A5:07C3`)). `src/` is read only; the test checks that `git status src` is unchanged.
- `port/port.mk` checks the platform boundary (`port-boundary`), compiles the generated C,
  `port/runtime/arena.c` and `port/main.c` once with no SDL on the include path (`-Wall -Wextra
  -Werror`), and links them twice: with `port/platform/null.c` into `build/port/eob-port-headless`, and,
  when `pkg-config sdl3` finds SDL3 (3.4.18 here), with `port/platform/sdl_host.c` into
  `build/port/eob-port`. Without SDL3 only the headless program is built, and `make port` says so.
- `build/port/eob-port-headless --self-test eob/EOB.EXE` loads the load module into the arena, runs the
  translated `BuildList1` to `BuildList5` (both values of `gFlag`) and checks each list is its table's
  records in order, in DGROUP, ending in null, then `FindNode`, `IsNodeEnabled`, `IsNodeOn` and
  `GetNodeState1` on patterns written into each record; and that `1658:001E` finds `ListAdd` in the code
  table. Three hand mutations of the generated `ListAdd` and `IsNodeOn` (a wrong offset, a wrong return,
  a link left set) each make it fail (the last by its 10 s watchdog, since it makes a list circular). On
  `INTRO.EXE` it fails, as it should: the addresses are `EOB.EXE`'s.
- `build/port/eob-port --window` opens the SDL3 window (320 by 200, 4:3, nearest scaling) and shows a test
  frame through `platform_present`; Escape or 10 s closes it. The headless program refuses `--window`.
- `tests/unit/test_port.py` (fast set, skipped without clang or SDL3): the build and the self-test on the
  headless program, no SDL use outside `port/platform/`, and a unit taking a local's address refused by
  name (at M0; M1 handles it, and the refusal test now uses a struct passed by value).

## Order of work

Every step's check runs on macOS (arm64). Linux and Windows are deferred (the human, 2026-11-05): the
platform separation and the portability rules keep them possible, and nothing is built or checked for
them until the human brings them back.

| Step | Deliverable | Check |
|---|---|---|
| M1 | tc2c handles every C unit of `EOB.EXE` (the arena stack, literals, `switch`, the library calls, the overlay rules); `port/units.txt` lists them all; the pinned LLVM in `make doctor` | `make port`; no unit refused |
| M2 | `make port-hybrid`, `tools/port/hybrid.py`: each translated function against the original on golden states | every function of `src/` |
| M3 | the machine: vector table, PIT, ports, INT 33h, INT 10h, DOS memory, files and 4B03h; the 19 assembly units hand-written in `port/asm/`; the C-unit clock in the oracle and the `.clock.json` recordings | M2's test of each hand-written routine |
| M4 | the MCGA driver, hand-written from `MCGA.ASM` | screens of `boot-title` |
| D | the program's data as C definitions in `src/` (question C): a P6 task after the review of `work/p6-review.md`, proven by `make match` and `make rebuild`; then tc2c places the initialisers and the port's runtime writes the library's own data; the generated image is checked against `EOB.EXE`'s | the generated image equals the original's load module, but for the library data listed |
| M5 | `port/platform/replay.c`: headless replay of `boot-title` to the title menu, from the generated image (step D comes first) | its checkpoints |
| M6 | every `EOB.EXE` golden (`boot-adlib`, `load-walk`, `mouse-party`, `new-party`); then `INTRO.EXE` (the twin map of `make twin` gives its addresses for the shared units) | `make port-golden` in `make check` |
| M7 | live play: real-time pacing from the PIT, AdLib synthesis with ymfm, the launcher's choices (VGA, AdLib, mouse) | a human plays |

## Decisions

The human answered all nine on 2026-11-04:

| # | Decision | Answer |
|---|---|---|
| 1 | Memory model | the arena |
| 2 | Translation | `tc2c.py` over clang's AST; the reference is **one pinned LLVM release** on every platform (not the installed Apple clang); GNU statement expressions allowed |
| 3 | Assembly units and drivers | **hand-written C** (not asm2c) |
| 4 | Graphics | MCGA (VGA) only |
| 5 | The clock for the proof | the original's instruction clock; the golden format grows by one recording file per session (approved) |
| 6 | The initial image | **generated from `src/`**, so the port ships no game bytes (not read from the player's `EOB.EXE`) |
| 7 | The OPL emulator | ymfm (BSD 3-clause); the human installs it, at M7 |
| 8 | Start-up | `EOB.EXE` directly with `3 4 0 homie 3` and the port's own settings; `INTRO.EXE` later |
| 9 | Platforms | macOS, Linux and Windows from the start; **then deferred 2026-11-05: macOS only for now, Linux and Windows later** |

And a note: keep poolrad's platform separation. It is the section "Platform separation" above, now
enforced by the build.

### The follow-up questions (the human, 2026-11-05)

| # | Question | Answer |
|---|---|---|
| A | The clock with hand-written assembly | the clock counts only the instructions of the C units ("The clock with hand-written assembly") |
| B | Which LLVM | the newest major release when it is installed (one all three platforms offer, once Linux and Windows return); `make doctor` checks it |
| C | The data of a generated image | **convert the data first**: the program's data becomes C definitions in `src/` (step D of "Order of work") before the port runs from its own image; the port program never loads `EOB.EXE`. Tests keep reading `eob/` as the oracle does (the self-test, M2's per-function tests on the oracle's memory), since they compare with the original |
| D | Where Linux and Windows build | a Linux container from M1, Windows from M5; **superseded 2026-11-05**: both deferred |
| E | The recording file | `tests/sessions/golden/<name>.clock.json`, beside each golden |

Step D changes `src/`, so it waits for the independent review (`work/p6-review.md`) to finish; M1 to M4
do not need it, since none of them runs the port from its own image.

### What the human installs

Downloads, so the human does them (AGENTS.md rule 5):

- the newest LLVM (Homebrew), and its version is then pinned in `make doctor`;
- at M7, ymfm;
- when Linux and Windows return: a Linux container and a Windows machine or virtual machine, each with
  that LLVM, SDL3 and Python, and the command agents use to run `make port` there.

## Open questions (none needs `src/` changed now)

Everything below is answered in the port's tools or by compiling a probe; none changes a matched file,
which an independent review is reading (`work/p6-review.md`).

- **Far pointer relational compares**: does Turbo C++ 1.00 `-ml` compare offsets only for `<`, or both
  words? [H] in tc2c; compile `p < q` and read the code.
- **`EOV3.C`'s `MK_FP(0x1d8, 0)`**: a segment-table index in overlay code (whitepaper 2.6). tc2c needs
  the rule "a constant segment in an overlay unit that the overlay manager fixes up is resolved through
  the segment table"; the alternative, a named far object in the source, would be a source change and is
  not proposed.
- **The vector table**: E07C8.C and I0892.C use `void interrupt (far *)()` as a type for `getvect` and
  `setvect`; tc2c refuses them until the machine has the vector table at `0000:0000` (M3). The QUIRK in
  `E1C0E.C` and `E19F3.C` (a `strupr` that writes into `0000:0000`) then behaves as the original's,
  since the vector table is arena memory.
- **String literals and initialised definitions**: placed at the original's addresses (the matcher's
  literal map) and checked against the image.
- **`-K`**: the SPEC leaves it undecided; every unit compiled so far has signed `char`. tc2c reads
  `/*@flags */` and would make `char` unsigned for a `-K` unit.
- **Divide errors**: `idiv` overflow and division by zero raise INT 0 on the 8086 (the library's "Divide
  error" exit); the port must do the same where the goldens could reach it. Not seen so far.
- **The high memory wrap**: `TC_LINEAR` wraps at 1 MB as the 8086 does [H for the oracle, which maps 0
  to A0000h and the ROM area; nothing seen addresses above that].
