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
| M3 the machine and the assembly units | started 2026-11-05 (section "M3"): `port/asm/` has the four units that touch no hardware, `E1BA8`, `E1D8C`, `E19FC` and `E1D9A` (8 routines), and the interrupt handlers' units `E1D8F`, `E1AAD`, `E1948`, `E_TEXT` with the retrace `E1DE4` (35 routines in all), each the same as the original on every call tested; the C-unit clock in the oracle and the five `.clock.json` recordings ("The interrupts and the C clock"); the machine boots to the oracle's memory byte for byte, and its DOS, INT 10h for MCGA, BIOS time, INT 33h driver and ports (PIC, PIT, 61h, 60h, retrace, DAC, AdLib) answer every one of the 335,055 service calls and port accesses of the five goldens as the oracle does. the machine's wait delivers the recorded interrupts to spinning routines (`port/runtime/irq.c`); 19 library functions on the machine (`port/runtime/tcdos.c`: the files, `int86`, `time`, `rand`; `tcheap.c`: the far heap), each the same as the original's on every call tested; every assembly routine the C calls is in C (63, section "The other units") |
| M4 the MCGA driver | done 2026-11-05 (section "M4"): `MCGA.OVL` in C (`port/drivers/`, 40 exports), entered with its argument bytes, the same as the original on every direct call and every golden call tested. Next: step D, then M5 |
| D the program's data as C | done 2026-11-06 (section "Step D"): D0 the data map (`re/datamap.toml`, 34 modules); D1 the tools; D2 every module's data as definitions in `src/`, the data of all 34 modules proven by `make match`, `make rebuild` of both programs as before; D3 tc2c writes the port's image from them (`make port-image`: 64,446 bytes the same as EOB.EXE's). Left: the naming round for the 126 address names D2 kept (100 are E0CF7's screen-list nodes), with its independent review |
| M5 the replay | done 2026-11-07 (section "Step M5"): `make port-replay` plays `boot-title` headless from the port's own image, all four checkpoints the oracle's (the state view and the golden's screens); the interrupts and checkpoints keyed by the game's accesses (the human, 2026-11-07); the program's start in `port/start.c`; the assembly units' code-segment data in the image (`port/asmdata.json`) |
| M6 the other goldens | started 2026-11-07 (section "Step M6"): `boot-title`, `boot-adlib`, `new-party`, `mouse-party` and `new-character` replay as the oracle at every checkpoint under the relaxed check (the human, 2026-11-07); interrupts also placed by the library's file calls; Westwood's far data in `src/far/FARDATA.ASM`; `DispatchKeyWait`'s result fixed in `src/E16A5.C`. Left: `load-walk` (needs interrupts placed inside computation), `make port-golden` in `make check`; `INTRO.EXE` plays in live play (section "Playtesting") |
| M7 live play | started 2026-11-07 (section "Step M7"): `make port-play` plays the game in a window from the port's own image (keyboard, mouse, the PIT's ticks from the wall clock); headless with scripted keys it reaches character generation and the dungeon. Left: sound (ymfm, which the human installs), and a human playing it |

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
- The replay (M5) is not a platform: the machine plays a recording back (rule 3), so it is `port/replay.c`, beside `main.c`, linked into the headless program (`eob-port-headless --replay`).

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
| Graphics drivers, 47 slots: 7 imports the game provides, 40 exports (whitepaper 2.5) | `E1C00.ASM` loads `*.OVL` with DOS 4B03h and calls `LinkDriver`; the game calls the slots at `ds:8046` | The machine's 4B03h loads the driver's image where the game asks and registers the driver in C (`port/drivers`) at `loadseg:offset` of each routine; `LinkDriver` in C fills the game's slots, so they hold the far pointers they hold in the oracle (section "M4"). MCGA only (decision 4) |
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

## Step D: the program's data as C (planned 2026-11-06)

Question C's answer: before the port runs from its own image, the program's data becomes C
definitions in `src/`, so the port program never loads `EOB.EXE`. P6 is signed off (2026-11-06), so
`src/` is open for it.

**What there is.** `EOB.EXE`'s own initialised data is ds:0090 to ds:84B2 (33,826 bytes, 969
relocations: far pointers in tables), between the start-up code's `_DATA` (to ds:008F, the
rebuild's map) and the library's. Its uninitialised data (_BSS, ds:8A6C up) is zero at start: those globals stay
`extern` and `make rebuild` keeps reserving them; the port clears the arena. The rebuild fills the
initialised data today with the original's bytes, in 18 fillers cut by the relocation runs; that cut
is good enough for bytes but not for definitions. E0A72's far-pointer table at ds:3406 points at
ds:337A to 33ED, below the cut the rebuild puts at ds:3406, and ds:36B0 to 36BB above its string pool
is E0ACD's `gHandSplitY`, so a unit's data is not the rebuild's piece.

**Where a unit's data is.** Turbo C writes a module's `_DATA` as its initialised globals and the
templates of its initialised locals in source order, then its string pool, word aligned, and TLINK
lays the modules out in link order. So each unit's data ends with its pool (the matcher's literal map,
`port/literals.json`: E08E6's ends at ds:3378, E0A72's runs ds:36A4 to 36AE) and starts after the
previous unit's (and its pad byte). Units with no literal (E0CF7, the data-only modules) are
bounded by the other anchors: the relocation runs, the alignment pads and the names (D0 below).

| Sub-step | Deliverable | Check |
|---|---|---|
| D0 | the data map: each unit's exact range of initialised data, with the evidence for each boundary (pool, template, relocation run, pad, reader), written by a tool and kept in `re/` | it covers ds:0090 to 84B2 with no gap or overlap; every boundary settled by an anchor or free (no relocation inside it); the names a module's span holds that it does not declare listed |
| D1 | the tools: `make match` compiles a unit's definitions, places its `_DATA` at the map's address and compares every byte and relocation with the original (code's references to the unit's own data resolve by the annotation); `make rebuild` links a unit's own data where the unit defines it and a filler where it does not yet; `make data-skeleton UNIT=` drafts the definitions from the bytes, typed by the declarations, a far pointer by the name at its target | a negative fixture per kind of defect (a byte, a relocation, the order); `make match-gate`, `make rebuild` as before |
| D2 | per unit: the externs of its range become definitions in address order, the unnamed bytes between them named from their readers (playbook 65's rules, `make rename`), the far-pointer tables written as tables of names; claimable and reviewed like a lift (playbooks 30, 70) | `make match` with the data; `make rebuild` identical but for the documented differences, with one filler fewer |
| D3 | tc2c translates the definitions (each initialiser written at its annotated arena address before `main`), the runtime writes the library's data (C0's, the RTL's: `_streams`, the ctype table, the heap and overlay variables), and the port starts from its own image | the generated image equals `EOB.EXE`'s load module but for the library data listed |

**D0, the data map (done 2026-11-06; two fixes from D1, below).** `re/datamap.toml` gives each of the 34 modules with
initialised data its span, in link order, covering ds:0090 to 84B2 with no gap; `make datamap`
shows how each boundary is placed and `make datamap WRITE=1` writes the file
(`tools/rebuild/datamap.py`; `tests/unit/test_datamap.py` checks the coverage, that a free
boundary has no relocation inside, and that the file is what the tool writes). What places a
boundary:

- **A pool closes its module.** Turbo C writes a module's `_DATA` as its globals and templates,
  then its string literals, so the next module starts at the end of the pool, rounded up to even.
  The literal map (`port/literals.json`) knows only the literals the matcher saw referenced, so a
  pool goes on over the strings the unit declares `extern` right after it (E122E's "INVENT" at
  ds:5BA1).
- **The relocation table, by module.** TLINK writes it module by module, and Turbo C writes a
  module's data fixups downward, one object-file record (about 1 KB) at a time, so a run of data
  relocations after a unit's code relocations is that unit's, or a code-less module's after it.
  Every C unit's run falls; the driver slot table's (ds:802C-8046) rises, as TASM writes them (checked
  in D1 by assembling a test file), so it is an assembled data-only module's, not E1AAA's (C).
- **The link order.** The resident units by address; a **data-only module** (`dSSSS` in the map)
  at each of the four empty code segments (07C8:0001, 177D:000A, 1AAD:000C, 1B82:0009), before the
  unit that starts there; and the seven overlays after E177D, in number order (their pools, EOV1's
  at ds:645B to EOV6's at 7F46, lie after E177D's, and the one run of relocations after E177D's code
  falls into one record per overlay). The rebuild's own order (EOV0 to EOV2 before E14EF) works for
  its fillers but is not the original's.
- **Names.** A name only one unit declares places that unit when it is one of a cluster of three or
  more, or next to the unit's other anchors, and the link order allows it; a name alone is weak (the
  driver's slots at ds:802A are declared one by one by their callers). A name alone does place a
  module the map took to have no data when it lies between a pool's end and the next module's first
  name or far pointer and only that module, which comes between them in link order, declares it
  (E14EF's `gHoverSelectsOption`, ds:5D1A, after E1414's pool and before E1547's names); the
  boundary after it is then free.

Of the 33 boundaries, 17 are settled (a pool's end), 3 settled [H] (a pool's end with modules
between that the map takes to have no data), 2 touching (two anchors meet), and 12 free. A free
boundary has no relocation inside it, so the image cannot tell where it is: any even cut gives the
same bytes and the same relocation order, and `make rebuild` proves D2's data either way. The map
places a free boundary by a default rule (the stretch goes to the module before, unless the module
after declares names in it) and D2 may move it for readability. Each module's `notes` list the names
in its span that it does not declare: shared data other units read, or a boundary to move.

The largest data-only module is the one after E07A9: ds:009E to 3056, 12,216 bytes and 448
relocations (the region table, the panels, the file table, the game's tables). D2 creates the four
data-only files. ds:552F (E0DE8's `gLauncherAmmoType`) lies inside E0D1C's string at ds:550E: a
table read from a base below its start, as ds:FB34 was; E0DE8's D2 settles it.

**D1, the tools (done 2026-11-06).**

- **How the toolchain writes data, settled by compiling and assembling test files.** Turbo C++ 1.00
  writes a module's `_DATA` as its initialised globals and the templates of its initialised locals
  in source order, with no padding between them, then its string pool; so a definition goes in the
  file before the function whose template follows it (EOV5.C's `gMenuRowX` after 00A1's templates).
  It writes a module's data fixups downward; TASM 2.0 writes them upward (`dd` fixups at +2, +6, +12,
  +16 come out in that order), which D0 had as [H].
- **`make match` proves a unit's data** when its file has the line `/*@data */` (`; @data` in
  assembly). A definition (`signed char gSaveMaxLevel[6] = { 17, ... }; /*@ ds:341E */`) is then left
  out of the stub, and the unit's `_DATA` is compared with the original's at the unit's span in
  `re/datamap.toml`: the same length (but a zero pad byte to the next even offset), every
  definition at its annotation (a `static` is refused: its place cannot be checked), every byte the
  same in links A and B the original's, a far pointer naming the same function (an overlay function
  as its stub entry, a library function by `rtl_compare.json`) or the same data (the unit's own by
  its place in it, an extern by its annotation), a near offset naming the same data, the same
  relocated words, and the same relocation order. The code's references to the unit's own data are
  resolved by their place in it. A file of no functions is a data-only module (`src/D1AAD.ASM`,
  the map's `d1aad`). Without the marker nothing changes: EOV5.C's one definition is still compared
  by its content. `make match-gate` counts the modules whose data it proved.
- **`make data-skeleton UNIT=`** (`tools/rebuild/dataskel.py`) drafts a module's data from the
  original's bytes, cut at every name `src/` declares in the span (typed by the C declaration, the
  unit's own first), at the literals and templates the unit's code owns (left to the functions, with
  a comment where they fall), at runs of far pointers and at unnamed pointer targets (`g33BB`, typed
  by the table that points at it: E0A72's `long g34D0[12]`). A far pointer is written by the name at
  its target, and the declarations the file lacks are listed. The draft is C, or TASM source when
  the module's relocations rise (`--c` forces C). What it cannot type (a struct) is bytes with a
  comment. A draft is stable: drafted again from a file that holds it, it is the same text.
- **`make rebuild`** links a module whose file has the marker with its own data, literals included,
  at its span, and fills the rest as before; a data-only file (`src/DSSSS.C` or `.ASM`) goes before
  the unit that starts at its empty code segment, and brings that segment itself.
- **The pilots**, to prove the tools: `src/E0A72.C` with the draft unedited (821 bytes, 26
  definitions, 12 relocations, then its pad byte) and `src/D1AAD.ASM`, the driver slot table as an
  assembled data-only module (194 bytes, 36 definitions, 7 relocations). Both match; `make rebuild`
  gives the same result as before them (the overlay stub entries and the link date differ, the
  relocation table is in the original's order). Their naming is D2's: the names other files
  declare are used, the rest are named by their address.
- **Negative fixtures**, one per kind of defect: a byte (`e0a72-data-byte.C`), a far pointer to the
  wrong table (`e0a72-data-pointer.C`), a pointer written as a number, without its relocation
  (`e0a72-data-relocation.C`), two definitions in the wrong order (`e0a72-data-place.C`), and d1aad in
  C (`d1aad-data-order.C`: every byte and pointer right, the relocation order downward).
- **Two fixes to D0.** The map's name scan read `/*@ ds:XXXX */` only and missed 21 annotations with
  text after the address; reading them gives E14EF, E16A5 and E1A72 data of their own (34 modules,
  E07C8 from ds:3056, E1C0E from ds:8196). And a free boundary could fall between a far pointer's
  offset and its relocated segment word (d1b82's at ds:8106, now ds:8104). `tests/unit/test_datamap.py`
  checks both.
- **Found for D2.** E07A9's `SetMouseItem` passes `""` where the original passes ds:0090, and matched
  because the byte there is 0; the data show a 4-byte global {0, 55h, 55h, FFh} at ds:0090, before
  " taken.\r" at ds:0094. Defined, and passed instead of the literal, E07A9 matches whole with its
  data (checked on a copy); its D2 makes the change and corrects the QUIRK comment. Strings a unit
  declares `extern` after its pool (E122E's "INVENT") are literals of its code, so its D2 writes them
  as literals.

**D2, every module's data (done 2026-11-06).** Each of the 34 modules' data is written in its unit's
file, after the `/*@data */` line (`; @data` in assembly), or in a data-only file (`src/D07C8.C`,
`src/D1B82.C`, `src/D1AAD.ASM`), and `make match` proves it: the gate counts the data of 34 modules.
`make rebuild` of EOB.EXE gives what it gave before D2 (27 stub entries in far pointers, 232 in the
stub table, 3 link-date bytes; the relocation table in the original's order), with no filler left but
the 16 pad bytes, and INTRO.EXE as before. What the modules taught, each settled by `make match`:

- **The pool is in source order and never merged.** A string the original's code reaches at one
  address from two places, or one before the module's last global, is a named array, and the code
  passes its name (E07C8's gFont8Name, gFontErrorText, gReinstallText; EOV2's 30 answer words). An
  empty literal the code resolved to a global's first byte is that global: E07A9's and E0D1C's
  4-byte CGA rows {0, 55h, 55h, FFh} (gCgaIconRemap, gCgaItemRemap). A table's strings are its
  initialiser's literals and fall in the pool where the table is in the source (gOverlayFiles,
  gErrorText, the 371 strings of d07c8's tables, EOV5's menus), so a table is placed by where its
  strings fall.
- **Templates are data at their function's place.** A definition whose bytes follow a function's
  local template goes after that function (E0ACD after ClickFloorItem, EOV0 after PickPortrait, EOV1
  after ScribeScrolls, E122E after DispatchWallHandler, EOV3 after LoadLevel, EOV5 after
  DrawCharPortrait and TileAnimation). D0's free boundary between eov5 and eov6 was CampLoop's
  template at ds:7F40, which eov6's code loads: the map's `template` boundary (datamap.py TEMPLATES).
- **One name per address, inside a definition too.** A name other units declared inside another
  unit's definition became the definition's own expression: the moving door's three names are
  gEffects[12]'s fields (macros in EOB.H), E0EF3 reads a spell's sound as gSpells[i].sound,
  gLauncherAmmoType is E0DE8's 8-byte table at ds:5536 read as [type - 7] (the code's ds:552F base
  inside E0D1C's pool). One stays: EOV5's gPoolNameBase (ds:71A7, gCharPool + 2 - 0xF3) is a folded
  base the matcher can only prove as a name, so EOV4's bytes there carry it.
- **The library's own variables.** E07C8 defines the overlay manager's _ovrbuffer (02CBh at
  ds:3088) itself, as the original does, so TLINK links no OVRBUFF; written as bytes, the library's
  default came in and moved every library variable by 2.
- **E1948's driver segment** (SNDDATA, ds:88A0 to 8A6C) holds the original's values: the PC speaker
  and Tandy note tables, 96 words each.
- The tools grew with it: `make data-skeleton` types structs and unions by their layout
  (`tools/rebuild/clayout.py`), declares the unit's functions a table names, and writes a table's
  pool strings as literals; the map reads the declarations and the literal table as D0 had them
  (commit 1e6507c), so it stays reproducible; a twin's whole data (INTRO.EXE) goes where the twin
  proof maps its span; the FILE:line citations follow the lines D2 moved.
- Not done: the 126 items D2 left with address names (E0CF7's 100 screen-list nodes, d07c8's
  g1123, g117D and g1733, the drivers' slot table at ds:802A, the overlay loader's words at ds:818A
  to 8195, others) are named from their readers in a naming round with an independent review, as P6
  was. No byte changes with it.

**D3, the port's image (done 2026-11-06).** tc2c lays out each module's data as Turbo C does: from the
start of its span, its file-scope definitions and its functions' templates (initialised local arrays
and structs) in source order, then its pool, the literals in source order (a literal is in the pool
when it decays to a pointer; one that initialises a char array, a variable or a struct's field, is
those bytes). Every definition must fall at its annotation, every literal at its address in
`port/literals.json`, and the pool must end at the span's end, or tc2c stops; the check found two
errors on the way (an empty string placed on the previous string's NUL, and struct fields' names
counted as pool literals). Each unit's `tc2c_data_<unit>` writes the bytes with `tc_put` and then its
far pointers, which need the load segment; `tools/port/asmdata.py` reads the assembly modules' `db`,
`dw` and `dd` (and E1948's segment) for `tc2c_data_asm`; the runtime writes the library variables its
own functions keep in DGROUP (`tc_library_data`: _openfd, _fmode, rand's seed, tzset's values). `make
port-image` runs `eob-port-headless --data-image` on an empty arena and compares DGROUP with EOB.EXE's
loaded at the same segment: the program's data, E1948's segment, the uninitialised data and the
runtime's variables, 64,446 bytes, all the same (in the fast tests). Not generated, and read by no
function of the port: the rest of the start-up code's data (ds:0000 to 008F, Borland's copyright
string among it) and of the library's (ds:84B2 to 88A0); a golden's state digest that covers them is
M5's to settle. `INTRO.EXE` follows at M6.

## Step M5: the replay (done 2026-11-07)

`make port-replay` (`tools/port/replay.py`) plays `boot-title` headless on the port, started from its
own image (step D: no byte of `EOB.EXE` is loaded), and its four checkpoints are the oracle's: the
port's view of the state the same, and the screens the golden's own image hashes. A test in the slow
set runs it (`tests/unit/test_port.py`, `test_the_port_replays_boot_title`).

### The replay's clock (the human, 2026-11-07)

The goldens' checkpoints and the recordings' interrupts are placed on the C-unit clock (decision A).
The port counts no machine instructions, and counting them exactly would mean tc2c knowing, for every
path through 609 functions, the original's instruction count. The human chose instead (2026-11-07):
**the replay keys the interrupts and the checkpoints by the game's accesses.** The recordings keep
their C-clock positions, which the oracle's tools still use, but the port does not read them.

- An access is a port access or a service call of the game's code, as `tools/oracle/cclock.py` counts
  them (the start-up code's, the library's and now the overlay manager's apart: the port's overlays are
  plain code and never read the program file). Each interrupt is recorded after so many accesses.
- The port delivers an interrupt lazily (`machine_replay`, `port/runtime/irq.c`): every one recorded
  after the last access before the next access is made; otherwise the next one when the game waits for
  it. A hand-written routine that spins on what a handler writes waits as before (`machine_wait`). A
  loop of `src/` waits when its last turn left the arena as it found it, made no access and read a byte
  a handler writes: tc2c gives every loop a `tc_loop` and calls `tc_spin` before each test of its
  condition, and at each label (a backward `goto`); the arena keeps a fingerprint of its contents (the
  sum of a 64-bit mix of address and value over the bytes stores changed) and counts the loads of a
  byte a handler wrote (`tcport.h`: the handlers' stores mark the bytes, and the bytes a wait can read
  before any handler has run are marked at the start, `port/start.c`). A fingerprint, not a count of
  changing stores: the class menu's loop (`PickClass`, ovl0:207A) calls `IsMouseInRect` on every turn,
  which raises the pointer's busy flag and lowers it again (E1A72.ASM, `inc` then `dec`), two changes
  that leave the arena as it was; counted, the loop never waited, and in live play it spun with the
  keys, the mouse and the clock undelivered (found in M7, guarded by the golden `new-character`).
- Eager delivery at every loop test does not work: each timer interrupt's handler ends with an EOI, an
  access, which makes the next interrupt due at once, so a drawing loop would take a whole wait's
  interrupts before the wait begins.
- A checkpoint is recorded with its place, how many interrupts were delivered and how many accesses
  made before it, and the port's view where the port takes it: just before the next interrupt or the
  next access (`tests/sessions/golden/<name>.clock.json`, `port`). The oracle also takes the view at
  the checkpoint itself and the two are the same for every checkpoint of the five `EOB.EXE` goldens:
  each is at a wait (`port_not_at_a_wait` is empty in every recording).
- The library's accesses in the port (`machine_library_int`: `tcdos.c`, `tcheap.c`) are counted with
  the library's, and the oracle's instruction count at them (`library_times`) is set from their
  ordinal. `int86` is the exception: the Borland library runs its `INT` from a stub on the stack, so
  the oracle counts it with the game's, and so does the port.

### The program's start (`port/start.c`)

What the start-up code (C0L), the library's initialisers and the overlay manager leave before `main`
was read off the oracle at `main`'s entry: every service call with its registers, and the memory that
differs from the loaded image. The port writes the start-up's variables its runtime reads (`_psp`, the
version, the heap's base, break level and top), makes the same DOS calls through the machine (counted
as the library's), allocates the three heap blocks the original has at `main` (the overlay buffer,
2CC0h bytes: `_ovrbuffer`'s 02CBh paragraphs and one more; a stream's 200h buffer; the environment
array), keeps handle 5 taken as the overlay manager keeps the program file open (`dos_reserve_handle`:
the file is not read), sets INT 0 and INT 3Fh where C0 and the overlay manager set them, and builds
`argv` at the top of the stack segment. Where an access is the original's but the port has no reason to
make it (the overlay manager's file reads), only its place in the count is kept.

### The view the port is compared on (`tools/port/state.py`)

The golden's state digest covers the registers and all of memory, code included; the port has neither
the code nor the original's stack. The view is conventional memory from 0500h to A0000h without: the
load module's code; the start-up code's and the library's data, but for the variables the port's
runtime keeps there; the stack segment, and E1C00's SS:SP saved in DGROUP across the driver's EXEC;
the overlay buffer. The screen is compared apart, rendered as the golden's screenshots are, so a `shot`
is compared with the golden's own image hash.

### The assembly units' data in their code segments (`port/asmdata.json`)

The hand-written routines read their unit's tables where the original has them, in its code segment
(`E1AAD`'s key tables, `E_TEXT`'s AdLib tables): the first replay went wrong at the first key, which
translated to 0. These bytes are now in the port's image too, from `src/`: `make port-asmdata`
(`tools/port/asmlist.py`) assembles each unit with the matcher's switches and a listing, takes each
`db`, `dw` and `dd` line of its code segment at its offset (the unit's code begins at its start in
`re/units.toml`), checks each run against `EOB.EXE`, and writes `port/asmdata.json`, which tc2c reads
(committed, as `port/literals.json`, so `make port` needs no assembler): 32 runs, 17,493 bytes, in 18
units. `make port-image` checks them and that the port writes no other byte below DGROUP.

### Left for M6

- The other goldens: the mouse callback's and the sound drivers' bytes to mark at the start
  (`port/start.c`, `hot_seed`); a session's `save` and `clock` options in `replay.py`; the AdLib
  register stream compared.
- The code segments are masked in the view: the assembly units' variables there could be compared now
  that the port writes their initial values.
- `make port-golden` in `make check`, then `INTRO.EXE`.
- The per-function test (`make port-hybrid`) still delivers its bench's interrupts after each access
  (`machine_replay` clear), as M2 and M3 left it.

## Playtesting (the human, 2026-11-08)

The way to find what stops a player is to play, and the port can play at thousands of times real speed.
`make port-playtest` runs the headless program in live play, on a clock only its own sleeps move: the whole
suite takes seconds. It replaced instruction and cycle counting as the check that the port is playable: goldens
compared at checkpoints stay (M5, M6) for what they prove, but they are no longer the gate for the
next player-visible feature, and a golden that does not replay (`load-walk`, `camp`) does not hold anything up.

| Layer | What it does | How it fails |
|---|---|---|
| The build's count | `tc2c`'s "still unported" list must be empty | the playtest refuses to start |
| Scripts, `tests/playtest/*.play` | one for each thing a player does: intro, title, load, walk, camp, spells, fight, save and reload | an error stop, a hang, a screen marked `changed` that did not change, an unexpected Exit to DOS |
| The monkey, `MONKEY=n` | random play from the save and from a new party: the buttons, the keys, anywhere on the screen | the same; a failure names its seed (`--monkey-seed`) |
| Coverage, `COV=1` | an instrumented build; `build/port/playtest/never-run.txt` lists the functions no run reached | not a failure: the list of what to script next |

Each script writes a contact sheet, `build/port/playtest/<script>.png`, to be looked at: passing says nothing
stopped, only eyes say the screens are right. A new feature, or a bug a player found, gets a script first. Slow
tests run the suite (`test_the_playtest_passes`), so `make check` holds it.

### Saves, the application, and the intro

- `make port` also builds `build/port/Eye of the Beholder.app` (tools/port/app.py), with SDL3 and the game's
  files inside. The program finds them by its own place; a double click, or `build/port/eob-port` with no
  arguments, plays the intro and then the game. `make port-play` is the same.
- What the game writes lands in the player's directory when it closes the file (`platform_file_write`;
  macOS: ~/Library/Application Support/Eye of the Beholder). It is read before the game's own, and the
  app ships eob/'s EOBDATA.SAV as the first game to load. Replays and tests name a scratch one or none.
- The intro: tools/port/introsrc.py hands tc2c a copy of src/I0892.C and src/IOV0.C in which what INTRO.EXE
  shares with EOB.EXE (23 units, proven by `make twin`) is addressed as the game's copy, and the rest sits in a
  segment of its own (TC_INTRO_SEG), filled from INTRO.EXE (port/intro.c). `execl` (tcl_execl) restarts the
  machine and runs `Main` with the command tail the intro built. `INTRO.EXE` needs no goldens of its own: it is checked
  by `intro.play` and `intro-skip.play`.

## Step M6: the other goldens (started 2026-11-07)

`make port-replay` plays `boot-title`, `boot-adlib`, `new-party`, `mouse-party` and `new-character`
headless from the port's own image, and every checkpoint of the five is the oracle's under the relaxed
check below (`boot-title` and `boot-adlib` exactly, screens included; the others but for the
tick-driven animations). `load-walk` does not replay yet ("What load-walk needs"); `INTRO.EXE` is not
started.

### The library's file calls place interrupts too

After the title the game loads the character panel's files: thousands of the library's file reads and
no access of its own (each access in that stretch is an interrupt handler's EOI). Keyed by accesses
alone, every interrupt of the stretch waited in the port for the next wait, where it was delivered at
once, while in the original it came during the loading; a routine that reads the tick counter there
then saw an older count. So each interrupt is also recorded with the number of the library's file
calls made before it (INT 21h with AH 3Ch to 44h, `interrupt_files` in the recording,
`tools/oracle/cclock.py`), and the port delivers it once both counts are reached, before the next
access or the next file call (`machine_library_file`, `port/runtime/irq.c`). The port's runtime library
makes the same file calls as the Borland library, one for one (compared on `boot-title`, 3,075, and `new-party`, 4,265);
its other calls differ (its `setvect` makes no DOS call, its heap grows in other steps), which is why
only the file calls count. The overlay manager's calls are left out on both sides, the library's
`_chmod` it runs through the library's overlay module OVRHALT (0000:5028 to 0000:5098, by
`build/re/rtl_compare.json`) included.

### Finding where a replay leaves the oracle

`tools/port/replay.py --probe-file F,...` compares the port's view and the oracle's just before the
library's file call F, and `--probe-access A,...` just before access A: at either the two are at the
same instruction with the same interrupts delivered, so any difference is the port's. (`--probe K`, at
interrupt K, compares only where both are at a wait.) `TC_TRACE=file` on `eob-port-headless --replay`
writes the port's accesses, file calls and interrupts in the form of the oracle's.

### What the replays found and fixed

- **Westwood's far data was not in `src/`.** The load module has 15 far data segments of Westwood's,
  1F1D to 20AB (about 8 KB: the panels' rectangles and icon definitions, the button table, the camp
  strings, the item names, the cell bytes, the CGA sheet maps), which `src/` declares `extern far` at
  their addresses and `make rebuild` generated from the original's bytes. Step D converted DGROUP only,
  so the port read zeros there (`BuildIconTable` made none of its 10 cells). They are now a module of
  `src/`, `src/far/FARDATA.ASM` (bytes as drafted from the original, each object named and its users
  listed): `make rebuild` links it in place of the generated module, with the same output, tc2c writes
  it into the image and `make port-image` compares it with `EOB.EXE`'s. It lies in `src/far/`, which
  `make match` does not read: the matcher proves DGROUP data only, and extending it was not needed.
- **`DispatchKeyWait` returned nothing in C.** `src/E16A5.C` defined it `void`, but the original leaves
  `DispatchNodeEvent`'s result in AX and its callers read it (`WaitKeyOrPause`, EOV1, E0ACD); tc2c gave
  them 0, so a click on a character box did nothing. It now returns that result (the same bytes,
  `make match`), and tc2c refuses a `void` function's call whose value is used instead of giving 0.
- **Part of `E_TEXT`'s data was missing from the image.** A long comment that TASM's listing wraps onto
  the next line began with "end", which `tools/port/asmlist.py` took for the `END` directive, so the
  AdLib command table at 0000:4F94 and part of `E1990`'s data were left out; the directives are now
  matched in upper case, as `src/` writes them (`port/asmdata.json`: 48 runs, 24,848 bytes with the far
  data, each the same as `EOB.EXE`'s).
- **The AdLib busy byte.** `PlaySoundAtCell` waits on `gBusyCountdown` (0000:3B36), which the AdLib
  timer handler counts down; it is now among the bytes marked handler-written at the start
  (`port/start.c`), so the wait is seen.

### The relaxed check (the human, 2026-11-07)

Placing every interrupt exactly where the original took it would need a clock inside computation (the
loop turns, or the instruction count); the human chose instead to relax the session check: the port's
view leaves out, on both sides, the state that only says when ticks landed, each item found by a
replay and named in `tools/port/state.py` (`timing_masks`, `frame_outline`, `box_rect`): the selection
frame's step and next tick and its outline on the screen; the member box animation's frame, next tick,
cell in page 2 and box on the screen; the MCGA driver's line colour, kept inside its code; the pointer's
save-under buffer; and where a mouse button's release lands against the game's reading of the ring
(the last event read, the button changes not yet read). Everything else is compared, the far data
included; a `shot` checkpoint is reported against the golden's own screenshot as well ("the golden's
screenshot too", or "but its tick-driven animations").

### What `load-walk` needs

With the AdLib on, the timer interrupts come at 874 Hz, and many come while the C code computes with no
access and no file call. At its 37,469th interrupt the original's AdLib tick ran before the C code
queued a sound and found nothing to play; the port delivers that interrupt at the next wait, after the
sound is queued, so its tick plays it, the accesses part and the replay cannot go on (it stops at a
wait at access 61,403, the next interrupt being due after 61,432). No mask covers this: it needs the
interrupts placed inside computation, by the loop turns (the oracle counting each C function's loop
back edges, found by dominators, since Turbo C's switch dispatch also jumps backward) or by the
instruction count. Not started; the human's call.

## Step M7: live play (started 2026-11-07)

`make port-play` (`tools/port/play.py`) opens the game in a window (`build/port/eob-port --play DIR`,
SDL3), from the port's own image, in a game directory made of the pinned files of `eob/` (with
`SAVE=tests/sessions/fixtures/EOBDATA.SAV` a saved game in it). The launcher's choices are VGA, the
mouse unless `--no-mouse`, no sound unless `--adlib` (the game's own arguments, " 3 4 1 homie 3").
The port's DOS keeps the game's writes in memory: a game saved in the window is gone when it closes.

The program is `build/port/eob-port` of the checkout `make port-play` runs in (the main checkout's
`<your clone>/build/port/eob-port`; a worktree has its own `build/`), and the game
directory `build/port/play/game` beside it. It says where it plays from when it starts and how it
ended when it stops. `make port` builds only what changed: tc2c and `make overlays` write a file only
when its text changes, and the objects depend on the generated files, not on the translation's stamp.

### How the live machine runs (`port/live.c`)

The program is the replay's (`port/start.c`, then `Main`), and the machine delivers the interrupts the
same lazy way, before an access or a file call or at a wait; only their source differs. Where the
machine would deliver one it asks (`machine_more`, `port/runtime/irq.c`), and `live.c` pushes what has
happened since: IRQ 0 at the rate the game programmed into the PIT, from the wall clock (the machine's
clock, which the PIT counter, the retrace bit and the time of day read, is the wall clock's
microseconds; one timer interrupt waits at a time, as on the PC, and a backlog over 0.1 s is dropped);
IRQ 1 for each scancode the host gives; the INT 33h driver's event for a mouse move or button, when the
game has a handler for it. Every 16 ms it presents A000:0000 through the DAC. At a wait with nothing
due it sleeps until the next tick; a long computation still looks at the clock every 64K loop turns
(`tc_live_poll`), and delivers there what has come, as a PC's interrupt comes between any two
instructions, so no loop can keep the keyboard, the mouse and the clock from the game. The window's close ends the program; the game's own exit is DOS 4Ch, through the
library's `exit`, now in the runtime (`port/runtime/tcdos.c`).

### Checked so far

- In the window (SDL3, macOS): the title menu at 38 s, the same screen as headless.
- Headless, whose clock only the program's sleeps move, so a run is the same every time, with the
  null platform's scripted scancodes (`TC_NULL_INPUT`) and a snapshot (`TC_PLAY_SNAPSHOT`): keypad 2
  and Enter at the title menu reach character generation, the golden's own screenshot but for the
  selection frame's outline (`test_live_play_reaches_character_generation`, slow set); Enter on "Load
  game in progress" with the saved game reaches the dungeon with the party, and turning and stepping
  work (the game answers "You can't go that way" at a wall).
- In the window, character creation hung at the class menu (the human, 2026-11-07): its loop never
  turned idle (the fingerprint above) and made no access, so nothing was delivered. Headless with
  scripted keys it now goes through race, class and alignment to the rolled abilities, the original's
  own (`new-character`).
- In the window, the Camp button stopped the game (the human, 2026-11-07): the camp menu calls the
  library's `heapcheck` before each of its menus, and it was one of three library routines the runtime
  did not have yet (the build's last line said "still unported: 3 library"). All three are in the
  runtime now, and the build says "still unported: 0 library, 0 assembly, 0 C": `heapcheck`
  (`tcheap.c`) and `chsize` (`tcdos.c`) agree with the original's at every call on the bench (`heap.py`:
  780 calls on healthy heaps and on heaps with one header word changed; `library.py`: 15 calls,
  shorter, longer, read-only and closed handles); `hardretn` stops with a message, since only the
  critical error handler calls it and the port's DOS raises no critical error. The null platform's
  scripted input takes mouse events too (`MS:Mx.y.b`, the screen's pixel and the buttons): the saved
  game, then a click on Camp, is the golden `camp`'s screenshot pixel for pixel
  (`test_live_play_opens_the_camp_menu`). `camp` does not replay on the port yet: after the saved game
  is loaded its state differs (the random generator's seed among it), as `load-walk`'s does.
- Once, the first time the window opened, the game ended through `exit` within 45 s (the library's
  `exit` was not in the runtime then); it did not happen again and why it chose to end is not known.

### Left

- Sound: the AdLib's register writes do not reach the host yet (`platform_adlib_write`), and the host
  has no OPL emulator: ymfm (decision 7), which the human installs.
- A human plays it (M7's check): the mouse, the menus, combat and spells, saving.

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
| M5 | `port/replay.c` and `port/start.c` (at the program's level beside `main.c`, not in `port/platform/`: the replay is the machine's, rule 3, and its dumps read the arena): headless replay of `boot-title` to the title menu, from the generated image (step D comes first) | its checkpoints (`make port-replay`) |
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
| (M5) | The replay's clock (2026-11-07) | **by the game's accesses**: the interrupts and the checkpoints are placed by how many accesses the game made, delivered lazily, at the next access or when the game waits; the port counts no instructions (section "Step M5") |

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
