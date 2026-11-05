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
| M2 the per-function test | the machinery done 2026-11-05: `make port-hybrid` (section "M2"); on the five `EOB.EXE` goldens 129 of the 594 functions with an entry are reached, 118 are the same as the original on every call tested (635 calls), none differ, 11 are reached only where they wait for an interrupt (M3). The rest of the check, every function of `src/`, needs sessions that reach them |
| M3 the machine and the assembly units | started 2026-11-05 (section "M3"): `port/asm/` has the four units that touch no hardware, `E1BA8`, `E1D8C`, `E19FC` and `E1D9A` (8 routines), and the interrupt handlers' units `E1D8F`, `E1AAD`, `E1948`, `E_TEXT` with the retrace `E1DE4` (35 routines in all), each the same as the original on every call tested; the C-unit clock in the oracle and the five `.clock.json` recordings ("The interrupts and the C clock"); the machine boots to the oracle's memory byte for byte, and its DOS, INT 10h for MCGA, BIOS time, INT 33h driver and ports (PIC, PIT, 61h, 60h, retrace, DAC, AdLib) answer every one of the 335,055 service calls and port accesses of the five goldens as the oracle does. the machine's wait delivers the recorded interrupts to spinning routines (`port/runtime/irq.c`); 19 library functions on the machine (`port/runtime/tcdos.c`: the files, `int86`, `time`, `rand`; `tcheap.c`: the far heap), each the same as the original's on every call tested; every assembly routine the C calls is in C (63, section "The other units"). Next: M4, the MCGA driver |
| M4 to M7 | not started (section "Order of work") |

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
  alignment; the second byte of a word at `xxxx:FFFF` at the next linear address, as the oracle does,
  where an 8086 would wrap to `xxxx:0000`: found by M3's test of `Unpack0`, section "M3"; a long is
  two words at offsets +0 and +2 of the segment).
- A global is read and written at `TC_MKFP(tc_ds, annotation)`; a far object of another segment
  (`/*@ 0000:3B36 */`) at its segment moved by the load segment.
- A function of `src/` is registered under its original far address (`tc_code_register`), so a far
  function pointer read from the arena (a node's `callback30`, a driver slot, an interrupt vector) finds
  its C function (`tc_code_find`); with none there (code the original loads, such as a graphics driver)
  the call goes to `tc_code_far` with its arguments as Turbo C pushes them, which stops the port and,
  in the hybrid build, runs the original's code.
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
| far function pointer call | `tc_code_find(address)` then the C function; none: `tc_code_far` (the hybrid build runs the original's code there) | skeleton, M2 |
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
| DOS memory | `MemAlloc` (`E19BC.C`), `E1990.ASM`, which walks the MCB chain itself | INT 21h 48h, 49h, 4Ah over a list of blocks, as `tools/oracle/dos.py` keeps it: the oracle writes no MCBs into memory, so neither does the machine (`port/runtime/dos.c`) |
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
  `tools/port/hybrid.py` plays a golden, and at a call of a translated function a second oracle
  machine whose memory is the library's `tc_mem` takes the session's state, runs the original, takes
  the state again, runs the C, and compares memory and the return registers (section "M2"). Every
  function of `src/` can be checked so, on states the goldens actually reach, before anything else of
  the port exists.
- **Per session (M5, M6)**: the port runs a golden headless, inputs from the recording, and its
  checkpoints are compared with the golden's: state digest of the arena (stack and overlay buffer
  masked, the rest as `Machine.canonical` does), the screen (`A000` and the DAC in the form `ega.py`
  digests), and the AdLib register stream. The first checkpoint that differs names the session time;
  the per-function test then finds the function.

The new recording artefacts (per golden: the clock positions of each interrupt and input) are files
beside the goldens, `tests/sessions/golden/<name>.clock.json`; the human approved this change to the
golden format (decision 5). The goldens themselves do not change.

## M3: the machine and the assembly units (started 2026-11-05)

- `port/asm/<unit>.c` holds an assembly unit of `src/` written in C, one routine per routine of the
  TASM source, each marked with its address as `src/` marks it (`/*@ 1D9A:0013 */`). tc2c reads those
  marks: the routine leaves `unported.c`, `ported.h` declares it with the callers' prototype (so the
  compiler checks the hand-written definition against it), `tc2c_init` registers it at its original
  address (far pointers to it find it), and `hybrid.c` gives it a thunk, so the per-function test
  stops at the original routine's entry and compares it as it compares a translated function.
- Where the result depends on how an instruction moves its registers, the C is written over 16-bit
  register variables named as the source names them, with the string instructions of
  `port/asm/x86.h` (`lodsb`, `rep movsw`, `repe cmpsb` and so on, on the arena) and the carry kept by
  hand (`rcl`, `rcr`, `sbb`), so it reads beside the TASM label by label. A unit's `cs:` variables stay
  in the arena at their offsets in its code segment (`CS_VAR`), where the state digests see them.
- Done: `E1BA8` (`Unpack0`, `NormalizePtr`), `E1D8C` (`RandomByte`), `E19FC` (`CompressLcw`), `E1D9A`
  (`Unpack1` to `Unpack4`). The goldens reach three (`NormalizePtr`, `Unpack0`, `Unpack4`: 31 calls,
  all the same); no pinned file uses formats 1 to 3 and the goldens never save a map or draw the
  random step, so `hybrid.Direct` calls a routine on the bench with chosen arguments, the original
  then the C. `tests/unit/test_port.py` (slow set) does so for all eight: the random step in
  sequence, the far copy with overlaps, odd lengths and 64 KB blocks, the encoder and `Unpack4` on its
  output (the original's round trip holds), `Unpack4` on random streams, the dictionary unpackers on
  random code streams, `Unpack3` on well-formed ones. All the same; a copy of `RandomByte` without its
  `cmc` fails every call.
- Found on the way: the oracle reads a word at `xxxx:FFFF` with its second byte at the next linear
  address (Unicorn, tested), not at `xxxx:0000` as an 8086 would; `Unpack0`'s 64 KB blocks straddle it.
  The arena now does as the oracle (`tcport.h`, `TC_NEXT`); a long stays two word accesses at offsets +0
  and +2 of the segment, as the compiled code makes them.
- With these, the five goldens reach 132 functions: 121 the same on every call tested, none
  different, the same 11 waiting for an interrupt.

### The machine (2026-11-05)

```
make port-hybrid && .venv/bin/python tools/port/hybrid.py --shadow [--session NAME]
```

- `port/include/machine.h` and `port/runtime/`: `machine.c` (the vector table, the ROM and its
  service stubs, dispatch; a software interrupt is `machine_int(n, &regs)` with a register record, and
  the vector decides as the CPU's INT does), `dos.c` (INT 21h, 20h, 29h, 2Fh; the PSP and the
  environment; the memory arena; the files over a VFS that keeps writes in memory, as the oracle's),
  `bios.c` (the BIOS data area, INT 11h, 12h, 13h, 1Ah), `video.c` (INT 10h for MCGA: mode set, the
  DAC; mode 13h's memory is the arena at A000:0000), `ports.c` (`machine_in`, `machine_out`: the
  interrupt controller's mask and EOI, the timer's channels 0 and 2 with the next timer interrupt's
  clock, port 61h, the keyboard's data port, the retrace bit and the DAC ports, the AdLib with its
  timers; AdLib writes and speaker changes go to the platform), `mouse.c` (the INT 33h driver and its
  event trampoline in the ROM). The clock is the oracle's: instructions plus what polls charge (a
  retrace poll 40, an AdLib status poll 20); the AdLib's timers are decided in whole instructions
  where the oracle uses float seconds (no floating point in the port), and no poll of the goldens
  falls on a boundary where the two differ. Each is written from the oracle's model
  (`tools/oracle/dos.py`, `vfs.py`, `bios.py`, `ega.py`), function by function, because the goldens
  are the oracle's. A service the oracle has and the goldens never call stops with the oracle's words
  (`machine_unimplemented`); the shadow would report it.
- The host's files: `platform.h` now has what the VFS needs, a file's size and its bytes
  (`platform_file_size`, `platform_file_read`), with `port/platform/files.c` over a directory for both
  programs; the hybrid build answers from the session's VFS.
- The proof, two checks (`tests/unit/test_port.py`, slow set):
  - **boot**: `machine_boot` and the loader on an empty arena give the oracle's memory before it runs,
    every byte of conventional memory and of the ROM (the vector table, the stubs, the default INT 8,
    the BIOS data area, the PSP, the environment, the relocated image);
  - **shadow** (`hybrid.py --shadow`): through each golden, every call of a service the machine has is
    made to the machine just before the oracle's own service answers it, on a copy of the oracle's
    memory with the same registers and pushed flags; the machine's state (files, positions, the arena)
    lives on from call to call, as the oracle's does. Every port access is shadowed the same way.
    Compared: the registers, the flags returned, memory but the INT frame's flags word, the video
    mode, the DAC and A000, the value a port reads, the hardware state (the PIC, the PIT, the next
    timer interrupt, the clock's charges, the AdLib's registers, status and timers, the mouse
    driver), the AdLib writes and speaker changes. What the shadow does not run yet comes from the
    oracle before each call: the interrupts' in-service flags and keyboard byte, the next timer
    interrupt's clock, the mouse's position and buttons. The five goldens make 335,055 calls and
    accesses: 24,966 service calls (20,566 reads; overlay loads of the graphics driver with 4B03h),
    144,585 AdLib status polls, 63,946 retrace polls, 90,793 EOIs, the rest; all the same. A copy
    with three bugs (the current drive, one relocation of a 4B03h load, a retrace poll's charge) is
    caught.
- Not yet: the delivery of the hardware interrupts in the port itself, on the C clock (M5, with tc2c's
  block counts); the library functions on `machine_int` (they still run as the original's code in the
  hybrid build); text output. The interrupts' handlers and the clock are the next section.

### The interrupts and the C clock (2026-11-05)

```
make golden CLOCK=1 [CHECK=1] [NAME=x]          # the C-clock recordings beside the goldens
```

- **Handlers in C.** A routine of `port/asm` marked `/*@ SSSS:OOOO interrupt */` is an interrupt
  handler, `void f(void)`, entered through a vector: tc2c registers it at its address (with no caller
  in C it is declared in `ported.h` as such) and gives it an entry in the per-function table whose
  frame is IRET's (FLAGS, CS, IP). The machine runs whatever a vector or a saved vector points at with
  `machine_call_handler(seg, off, &regs)`: a service stub, the generic IRET, the ROM's default INT 8
  (written in C from its code: the tick count at 40:6C, INT 1Ch, EOI) and IRQ default (EOI), or a
  registered handler; `machine_irq(n)` is the hardware's entry, `machine_int(n)` a software INT's. A
  handler chaining to the vector it replaced (`jmp far`, `pushf; call far`) calls `x86_chain`; its port
  accesses are `machine_port_in`/`machine_port_out` (`x86_inb`, `x86_outb`).
- **On the bench.** In the hybrid build a service, a handler with no C and every port access of a
  routine of `port/asm` run on the bench, whose DOS and devices are the session's (`hybrid.py`:
  `oracle_int` enters the code as the CPU enters an interrupt, `oracle_port` reads or writes the
  oracle's port); the C machine answers the shadow test only. What a call is compared on now includes
  the oracle's clock charges (`extra`): a wait that polls once too often or too few times differs.
  `hybrid.Direct` calls an interrupt handler with chosen registers (`ds=`), and takes the oracle's
  Python state back between the two runs, so a device the original changed is the same for the C.
- **Done**, every handler unit of the plan and the retrace (35 routines of `port/asm` in all now; 28
  assembly routines are still stubs):
  - `E1D8F`: `ReadTickCount`, `InstallTickHandler`, `RemoveTickHandler`, the INT 1Ch handler `TickIsr`
    (1D8F:0055). `E1DE4`: `IsInVerticalRetrace`, `WaitRetrace`.
  - `E1AAD`, the keyboard driver: the readers, `GetKeyState`, install and removal, `QueueInputEvent`,
    `HandleMouseButtons`; the INT 9 handler `KeyboardIsr` (1AAD:07C0, the address the install sets)
    and the bare IRET it gives INT 23h (1AAD:09F5); its near routines are static.
  - `E1948`, the PC speaker and Tandy driver: install, uninstall, `StartSound`, `StopSound`, resume,
    the silence; the INT 8 handler `SoundTimerIsr` at 1948:000A (the vector's address: the symbol's
    first four bytes, at 0006, are the saved vector).
  - `E_TEXT`, the AdLib driver: the accessors at 03A1 to 03FA, the two INT 8 handlers (3E66, 3EE5),
    start, stop, the queue and the tables, `StartSample`; the script interpreter reads its jump table
    (4F94) from the arena and switches on the target address, so a changed entry is still followed and
    an unknown one stops.
- **Their proof.** In the goldens, all the same: 150 functions reached now, 139 the same on every call
  tested, none differ, the same 11 waiting for an interrupt (three calls of each); with 3,000 samples,
  `AdlibTimerIsr`, `QueueSound`, `SetSoundByte`, `GetSoundByte` and `KeyboardIsr` on boot-adlib and
  load-walk, 7,997 calls. `Direct`, through `tests/port_asm/<unit>.py` (slow set): the keyboard
  handler on make, break, E0 and E1 scancodes with the ring full or not, the readers in each record
  mode (the record file written and replayed); every song of `PCGAMESN.DAT` to its end on the speaker
  and on Tandy, synthetic streams over every command byte, random states (26,110 calls, 148 the
  original does not return from: a song looping forever, a divide error the oracle sits on); the
  AdLib driver with `ADLIB.DAT`'s block, the game's sounds and generated scripts over every command,
  the handlers on every pass, all 74 jump-table entries dispatched over the seeds. A planted bug in
  each unit is caught.
- **Not exact, by construction** (each a `QUIRK` comment in the C): the keyboard handler tests its
  lock tables against CH, the interrupted code's CX, which a C handler does not have (it uses 0; it
  matters only with Caps Lock or Num Lock on, which the oracle's BIOS never sets); `WaitKeyEvent` in
  replay mode and `GetDriverTable` above 4 return the caller's AX in the original (0 in the C); the
  AdLib driver's pitch slide pops its own return address when an F-number's low ten bits are zero
  (the C stops there; never reached). `StartSample` polls the timer chip until a count the oracle
  never gives, so the original does not return on the bench; in the port the C clock stands still in
  assembly code, so it needs the machine's wait (below).
- **Found on the way:** the oracle's AdLib hangs off its port handlers only, so the bench never took or
  restored it, and the AdLib register writes were not compared (M2's runs included). `components()`
  now takes the owners of the port handlers; the reruns above compare them. The bench now also
  compares the Tandy and timer logs and the timer and PIC state; `tc2c` regenerates when the build's
  list of `port/asm` files changes, takes a routine's prototype from its own definition when no C unit
  declares it (so `Direct` reaches routines only assembly calls), and leaves `static` routines alone.
- **The wait** (`port/runtime/irq.c`, 2026-11-05). The machine takes the interrupts as a list of
  events in the order they are due, each with its C-clock position and the number of the game's
  accesses made before it (port accesses and service calls; the library's are counted apart, since
  the port's library is its own and makes other calls: an access's CS:IP, or a service's return
  address, in the start-up or the library). It delivers an event once both its counts are reached:
  after each access (`machine_access`, in `machine_port_in`/`out` and a service called through
  `machine_int`), at the C units' block boundaries (M5), and in `machine_wait`, which a hand-written
  routine calls on each turn it spins on memory a handler writes (the three blocking key readers);
  no event due there means the routine would spin forever, and the machine stops. A routine that
  spins on a port (`StartSample` on the timer chip, `WaitRetrace`) needs no call: its accesses
  deliver, and the oracle's instruction count, which the recording gives at the access where it
  changed, moves the port's answer. An interrupt is the controller's in-service flag (the scancode in
  port 60h for IRQ 1, the driver's event and the game's handler in the trampoline's data for the
  mouse) and the handler through its vector; none is delivered inside a handler (the oracle's
  controller would not either, but for a handler that opens interrupts, which the goldens show if it
  matters).
- The recordings now key each interrupt by the access count too (`[position, access]`, ...) and keep
  the library's clock marks apart (`library_times`, `library_accesses`).
- **Proven** by `hybrid.Direct` with `interrupts=True`: the original runs as in a session (the
  oracle's own delivery, the timer by its clock, IRQ 1 for the keys given, at once or after a number
  of instructions), the oracle's C clock records what it delivered and the clock marks, and the C is
  given those; events still due when it returns are delivered after (as the port's next check
  would); an event already due when the routine is entered is delivered before its first statement,
  as the original took it before its first instruction (`tc_hybrid_run`). `tests/port_asm/waits.py`
  (slow set, 83 calls, all the same): the three readers 25 times each over keys, released and special
  keys among them, so a reader turns several times and waits between; `StartSample` 8 times, through
  1 to 2,772 timer interrupts (the sample of the call before still playing at divisor 4Bh, so the C
  `SampleTimerIsr` runs throughout; untestable before: the original never returned on a bench without
  them); and two alterations noticed, the clock marks dropped and a key dropped.
- **Where the bench's oracle takes an interrupt.** With an interrupt pending that it cannot take (IF
  off, a handler in service) the oracle looks again every 100 instructions (`Machine.wait_chunk`; the
  sessions keep 100, so the goldens and recordings are unchanged). The bench looks at every
  instruction instead, so it takes the interrupt at the first instruction with IF set, as the CPU
  does. Two failures showed why. The readers' loop opens interrupts for two instructions in eight
  (`sti`, `jmp`, then `cli`), and a look every 100 lands on two phases of the eight, so 70 of 75
  reader calls stalled. And `StartSample` runs its wait and two AdLib writes under `cli`, then sets up
  the sample right after its `sti`: a look 100 instructions late took the pending timer interrupt
  after those writes, with no access between, which no access count can say; the C, delivering right
  after the access before, did as the CPU would and differed from the late oracle.
- Found on the way: with a block hook installed, Unicorn's `emu_start` runs past its `until` address;
  the bench's C clock counts only while a run with interrupts goes on (`CClock.pause`, `resume`).
- Found on the way: the C's time limit (`LIMIT_MS`, a virtual-time signal that jumps out of the C)
  must never jump while Python runs inside one of the library's callbacks: the jump leaves that
  callback holding Python's lock and the test hangs for good with no CPU used (a first run of
  `waits.py` sat 4.5 hours that way). Every callback (`tc_hybrid_far`, `_oracle_int`, `_oracle_port`,
  `_raise`, `_set_insns`) now marks the time as Python's, saving and restoring the mark so a callback
  that calls the C again cannot clear it; the limit then stops the C when the callback returns, and a
  stop raised by C that a callback called aborts with its message instead of hanging.
- **The C clock in the oracle** (`tools/oracle/cclock.py`, question A): instructions executed in the
  code of the C units of `port/units.txt`, resident (`re/units.toml`) or overlaid (an overlay's code,
  recognised by its hash in `build/re/eob.json` when the manager reads it from `EOB.EXE` into its
  buffer). A block hook adds each translation block's instruction count (capstone, cached) when it
  starts in C code; `Machine.run` now tells `chunk_listeners` after each chunk, and the overshoot of a
  chunk that stopped inside a block is taken back from that block. Over the first 40 million
  instructions of `EOB.EXE` it equals a count taken instruction by instruction (37,158,988 C-unit
  instructions); it costs about 7 s a session.
- **The recordings** (`tests/sessions/golden/<name>.clock.json`, question E), for the five `EOB.EXE`
  goldens (`INTRO.EXE`'s wait for M6): each hardware interrupt delivered at its C-clock position, in
  order (`[position]` for IRQ 0, `[position, "irq1", scancode]`, `[position, "mouse", condition,
  buttons, x, y]`); the C-clock position of each checkpoint; and the oracle's instruction count at
  each port access or service call where it changed since the one before (`[access, insns]`). That
  last list is an addition to the design: the oracle's clock moves by run chunks, which its own
  scheduling decides, and the retrace bit, the AdLib's timers, the timer's next interrupt and the
  time of day read it; no C clock gives those values, so the port's machine sets its clock from the
  recording at each access. `golden.py --clock` plays the session once more with the clock and writes
  the file only if the run is still its golden; `--clock --check` plays it again and compares. Sizes:
  20 to 32 KB, 0.8 MB for boot-adlib and 1.8 MB for load-walk, whose timer runs fast (28,016 and
  60,222 interrupts).
- The proof: `tests/unit/test_port.py` (slow set), `test_the_c_unit_clock` (the two counts over 5
  million instructions; every recording reproduced) and the assembly fuzz test, which now calls the
  handler and the retrace routines.

### The library on the machine (2026-11-05)

```
make port-hybrid && .venv/bin/python tests/port_asm/library.py
```

- `port/runtime/tcdos.c` has the Borland library functions that need the machine or keep the library's
  own data: `rand`, `srand`, `getdisk`, `setdisk`, `time` (with `tzset` and `dostounix`), `int86`,
  `harderr`, and the files: `open`, `close`, `read`, `write`, `lseek`, `filelength`, `remove`. Each
  definition is marked with the library routine's address, `/*@ 0000:XXXX library */`; tc2c reads the
  marks (`--lib`, `LIBRARY_MARK`) and gives each an entry in the per-function test's table, named
  `tcl_<name>`, so `hybrid.Direct` calls the original routine and the C side by side and the goldens'
  test stops at the library's own entries. In the hybrid build the translated callers now call the C
  library, so every caller tested in a golden tests it too.
- **How the behaviour is known.** The library is library (AGENTS.md): nothing is translated from its
  code. Each function is written from its documented behaviour, then the original is called on the
  bench with chosen arguments and its DOS calls, the bytes it writes and its results are recorded
  (scratch observers on `hybrid.Direct` and on the goldens' calls), and the C is made to do the same.
  What was found that way:
  - the library's data: `errno` DG:007D, `_doserrno` DG:879A, the file table `_openfd` DG:8758 (20
    flag words, FFFFh when closed) and `_fmode` DG:8780 just after it, `rand`'s 32-bit state DG:8784,
    `timezone` DG:8818 and `daylight` DG:881C, `tzname`'s two 4-byte buffers DG:FFDC, the handler
    `harderr` installs DG:FFD6 (INT 24h then goes to the library's own handler at 0000:6014);
  - the error exit (`ioerror`): a DOS code 0 to 58h sets `_doserrno` to it and `errno` from an
    89-entry table (derived by `int86` on a handler that returns each code with the carry set); 59h
    and up count as 57h; minus an `errno` of 35 or less sets it, `_doserrno` FFFFh; below that, 57h;
  - `int86` executes its INT with the flags of the library's byte compare of the interrupt number
    against 25h (below it) or 26h (above it), its test for `absread` and `abswrite`, and the caller's
    IF; with the carry back it takes the error exit with AX;
  - `open`: 3Dh with AL from the access bits, then IOCTL 0 (the device bit, 2000h); with `O_CREAT`,
    4300h first: an existing file is opened (error 50h with `O_EXCL`), a missing one created with 3Ch,
    read-only unless the mode has `S_IWRITE`, with `errno` left at 2 by the failed 4300h; the flag word
    is the access less `O_CREAT`, `O_TRUNC` and `O_EXCL`, with 1000h when either of the first two was
    given; `O_TRUNC` writes nothing at the start; no text or binary bit takes `_fmode`'s;
  - the flag word's 0200h is the end of the file: `read` returns 0 without DOS while it is set, and
    `lseek` clears it, even when it fails; a write sets 1000h; `O_APPEND` seeks to the end first; only
    `close` checks a handle against the 20 (error 6, no DOS call), and it marks the handle closed even
    when DOS refuses;
  - `filelength` reads the position, the end, and seeks back.
- Text-mode reads and writes, `int86` with INT 25h or 26h, and a `TZ` in the environment stop with
  the port's words: the game's handles are all binary, and its environment has no `TZ`.
- **Proven** by `tests/port_asm/library.py` (slow set, 891 calls, all the same): `rand` 300 times with
  reseeds; the disk; `time` at two times of day; `int86` for every interrupt number, to handlers that
  return the carry clear or set, through the whole error table and its limits, and through the
  machine's INT 10h and 21h; the files in every access mode, on files that are there and not, every
  whence, the end-of-file bit, the append path, handles past the table. On the five goldens the
  library entries are reached 2,719 times at 200 samples (13 of the 14 functions; `setdisk` is not),
  all the same; with the C library under them, the goldens reach 163 functions, 152 the same on every
  call, none different, the 11 that wait for interrupts as before.
- Found on the way: the bench entered interrupt code the C reaches without setting the CPU's flags
  as the INT leaves them (only the pushed copy), so a handler that returns with `RETF 2` or reads its
  flags saw stale ones (`Bench.oracle_int`). Unicorn takes `INT 6` for its invalid-opcode exception.
- **The far heap** (`port/runtime/tcheap.c`): `farmalloc`, `farfree`, `realloc` (the large model's,
  `farrealloc` with the size as a long), `farcoreleft`, `farheapwalk`, found the same way, on a bench
  that has run the start-up to `main` (the heap the game starts with). What the original does:
  - its words: `_psp` DG:0079, the heap's base DG:0083, the break level DG:0087 and the heap's top
    DG:008B (far pointers, A000:0000 for the top), the program's DOS memory in 1 KB units DG:85C6;
    and in the library's code segment at 0000:70B0 the first block, the last, the rover, DS (written
    by every call) and `realloc`'s size as a long;
  - a block takes (size + 4 + 15) / 16 paragraphs (none for 0 or more than FFFFh), its header at
    seg:0000: the size, then the previous block's segment (the first block's own) in use or 0 free; a
    free block keeps the previous block at +8 and its two ring links at +4 and +6; the pointer is
    seg:0004;
  - the free ring: a freed block goes in just after the rover on the +6 side; an allocation looks at
    the rover, then along +6, and takes the first block big enough, from its high end (the rest stays
    free in place) or whole when it fits exactly (+8 back to +2); a block taken out moves the rover to
    its +4 neighbour; with none, the block comes from the break level, refused past the heap's top;
  - a freed block is marked free first (+8, then 0 at +2) even when it then joins the free block
    before it; it joins a free block after it too, and the link to the block before is written where
    the block keeps it (+8 when free); the last block goes back to the break level, with a free
    block before it;
  - the break level sets DOS's memory block (4Ah, ES the PSP) to (break - PSP) / 40h + 1 KB, no
    further than the heap's top, whenever that count changes;
  - `realloc`: NULL allocates; fewer paragraphs split the block and free the tail (the last block's
    break level just comes down); more allocate a new block, copy the old data (paragraphs * 16 - 4
    bytes) and free the old one, never growing in place; a size of 0 frees and returns what the free
    left in DX:AX, which the game never asks for (the port returns NULL);
  - `farcoreleft` is (top - break) * 16 - 16; `farheapwalk` gives the block after the one given (the
    first for NULL) with its size in bytes and whether it is in use, 2, then 5 at the end.
- **Proven** by `tests/port_asm/heap.py` (slow set): 1,500 random steps of allocations from 0 bytes to
  past the top, frees in random order and `realloc` shrinking, keeping and growing, with
  `farcoreleft` and whole walks: 2,241 calls, all the same; three more seeds of 3,000 steps, about
  20,000 calls, all the same. Three wrong drafts were each caught: a freed block not marked before
  joining the one before it, a header written for the last block's freed tail, the link to the
  block before written at +2 of a free block.
  On the five goldens the heap's entries are reached 774 times at 400 samples, all the same; with the
  whole library in C under them, the goldens reach 168 functions, 157 the same on every call, none
  different, the same 11 waiting for interrupts.
- Still stubs: `chsize`, `exit`, `hardretn`, `heapcheck`; the goldens never call them.

### The other units (2026-11-05)

```
make port-hybrid && .venv/bin/python tests/port_asm/e17a6.py && .venv/bin/python tests/port_asm/pointer.py \
    && .venv/bin/python tests/port_asm/dosunits.py
```

- Every assembly routine the C units call is now in C: `port/asm/` has 63 routines, and `unported.c`
  has no assembly stub left. The nine units of this step, each written from its TASM source label by
  label (three written in parallel, each built apart with `OUT=` and `ASM_C=`):
  - `E17A6.c`, the graphics support: `DirectionFromPoints`, `Distance`, `FindNeighbour`,
    `SetPlaneTables`, `DrawTile` (its CGA, Tandy and MCGA branches), `ExpandNibbles`, the three
    sprite blitters, `BuildMask`, `TranslateImage`. `DrawTile`'s EGA branch stops with its reason: it
    draws through the EGA's sequencer and latches, which the MCGA-only port (decision 4) has no use
    for and the bench cannot compare. An 8-bit `div` that overflows raises INT 0 as `E1948.c` does.
  - `E1A72.c`, `E1BB1.c`, `E1BBE.c`, `E1BF3.c`, the mouse and the game's own pointer: `InitMouse` and
    the INT 33h event handler it installs (1A72:017A), `RemoveMouseHandler`, `IsMouseInRect`, the
    interrupt wrappers (empty: the machine has no IF, the recording places each interrupt where the
    original took it), `SetFarPointer` and `GetFarPointer`, `SetMousePointerShape`, the hide and
    show family with zones and locks, the pointer's moves. The handler takes BX, CX and DX in
    registers; it reads them from the trampoline's event record at F000:3080, which the machine's
    delivery writes, so the record is its register record. The INT 3Fh overlay hook is left out
    (overlays are plain code).
  - `E1990.c` (the DOS memory chain: `CheckMemoryChain`, `ReserveMemoryBelowVideo`,
    `TrimMemoryToVideo`), `E19EA.c` (`PrintString`), `E1A19.c` (`FindLastDrive`,
    `IsLastLogicalDrive`), `E1C00.c` (the graphics driver loader, `RunOverlayRequest`, 4B03h). Two
    QUIRK exits of `E1990.ASM`, where the original returns through the wrong stack words and faults,
    stop with the quirk's words.
- `E1E8B.ASM`, the overlay manager, has no C: natively only its start-up allocation remains (section
  "The overlays as plain code"), which the port's own start-up makes (M5).
- **Proven**: `tests/port_asm/e17a6.py` (2,695 calls), `pointer.py` (1,697), `dosunits.py` (1,390),
  original then C, all the same; each catches planted mistakes (an off-by-one, a "fixed" QUIRK, a
  wrong mask, a missing branch). On the five goldens the new units' routines are reached 1,918 times
  at 200 samples (17 routines: `Distance`, `DirectionFromPoints`, `DrawTile`, `SetPlaneTables`, the
  pointer family, `InitMouse`, the mouse handler, `SetMousePointerShape`, `FindLastDrive`,
  `RunOverlayRequest`), all the same; the goldens now reach 185 functions, 174 the same on every call,
  none different, the 11 that wait for interrupts as before. The routines the goldens do not reach
  (the CGA, EGA and Tandy paths, the memory chain, the drive test) are proven by the direct calls.
- Found on the way, fixed in the shared tools:
  - code the C has no C for answers in registers a prototype cannot state (the graphics driver's
    entry returns BX, its link routine CF): the bench now leaves every register and the flags of
    such a call in `tc_hybrid_far_regs` (`hybrid.h`);
  - the bench maps only memory below A0000h, so the C saw neither the mouse event record (F000:3080)
    nor mode 13h's memory: `Bench.take` copies the record into `tc_mem`, and the per-function test
    gives the C the screen and folds what it wrote back into the oracle's before the screen is
    compared; the hybrid build's delivery writes the record in the arena too;
  - the port had no C for the mouse trampoline (F000:3000): the machine now calls the handler
    registered at F000:3090 (`machine_call_handler`);
  - the machine's DOS lacked INT 21h 06h (`PrintString`'s output): output goes to the console, which
    the machine does not have yet, as for 02h and 09h.

## M2: the per-function test (2026-11-05)

```
make port-hybrid                       # every EOB.EXE golden, 3 calls of each function (about 6 minutes)
make port-hybrid SESSION=load-walk FUNC=DrawViewFrame SAMPLES=10
```

- `make port-hybrid` builds the translation, the runtime and `port/runtime/hybrid.c` with `TC_HYBRID`
  as `build/port/hybrid/libeobhybrid.so`, then runs `tools/port/hybrid.py`. tc2c writes
  `build/port/gen/hybrid.c`: a thunk per function that reads its arguments where the caller pushed them
  (SS:SP+4) and returns its value as DX:AX, and the table of entries (a resident function's address,
  an overlay function's stub entry; the 15 static overlay functions have none). In `unported.c`, under
  `TC_HYBRID`, each routine the port has no C for runs the original's code at its address with the
  arguments laid out as Turbo C pushes them; so do calls through a far pointer with no C behind it
  (the graphics driver's slots).
- Each golden plays on the oracle unchanged. At the first `SAMPLES` calls of each function it reaches,
  a second machine, the bench, takes the session's state (memory, registers, and the oracle's Python
  state: the EGA, DOS and the files, the BIOS, the devices), runs the original to its return, takes
  the same state again and runs the C on it (the library's arena is the bench's memory). No interrupt
  reaches the bench. Nested runs of one Unicorn engine lose the outer run's instruction count (tested:
  the outer run did not stop at its count), so the bench is a separate engine and the session's
  machine is only read; every session is checked against its golden at the end (all five still are).
- Compared: memory, but for the stack below the caller's frame (the C's frames are not the original's,
  and a callee may write its arguments) and for what the overlay manager wrote in the original's run,
  measured there: the writes of its code (CS the segment of the vector the game's INT 3Fh handler
  chains to, ds:FC08) and its reads of an overlay from `EOB.EXE` (11,763 bytes for `EnterMapScreen`;
  TLINK's kind-4 segments are not the manager: two of them lie inside DGROUP); the registers the
  function's type returns (AL, AX, DX:AX); the screen, the AdLib and speaker logs and the files.
- `tests/unit/test_port.py` (slow set): `boot-title` with one call of each function (36 the same, none
  different), then a copy of the library with two functions made wrong (`FindMaskBit` counts one
  more, `SetTextWindow` stores the region plus one): both are reported, the second as
  `ds:811F (gTextRegion), 1 bytes: original 00, C 01`.
- Results on the five goldens of `EOB.EXE` (`build/port/hybrid/report.json`): 129 functions reached,
  118 the same on every call tested (635 calls, in 28 units), none different, none stopped. 11 are
  reached only where the original waits for what an interrupt changes, so they are not tested until
  the bench has the clock (M3): `Main` and `StartupGame` (they do not return), `CalibrateRetrace`
  (the BIOS tick), `StartupAlloc`, `LoadItemData`, `RunMapLoop` and `RunCharacterPanel` (a key, read
  through the keyboard interrupt), `UpdateMonsters`, `UpdateMonsterGroup`, `UpdateHuntingMonster` and
  `StepMonsterToward` (`PlaySoundAtCell` waits for the sound driver's busy countdown, 177D:0135).
- Not done: the other 465 functions with an entry are not reached by the goldens, so "every function
  of `src/`" needs sessions that reach them (combat, spells, the inventory, the camp menus, the other
  levels, the end); and `INTRO.EXE`.

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
