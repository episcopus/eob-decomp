# Eye of the Beholder (DOS) Decompilation: Process Specification

| | |
|---|---|
| Version | 0.2 (2026-11-04: the graphics drivers in scope, the rebuild's accepted differences, the port's host and quirk policy) |
| Target | Westwood / SSI *Eye of the Beholder*, DOS, the files in `eob/` (hashes in `re/inputs.toml`) |
| Output | C source in `src/` that the original compiler turns into the original's machine code, a rebuild of the game from it, a living design doc in `docs/design/` |
| Model | `poolrad-decomp` (SPEC 0.8, ADR-002): same proof ladder, same agent process, adapted to C |

This document says **what** we build, **how** we prove it is right, and **how agents do the work**. `AGENTS.md` is the agent operating manual; playbooks are in `docs/process/playbooks/`; what we learn about the game goes in `docs/design/WHITEPAPER.md`.

---

## 1. Outcomes

| ID | Outcome | Measure of done |
|---|---|---|
| O1 | C source for the whole game | Every routine of `EOB.EXE`, `INTRO.EXE` and `START1.EXE` outside the Borland runtime library is C (or assembly where the original was assembly) that the pinned toolchain turns into the original's instructions (L2); and the four graphics drivers `CGA.OVL`, `EGA.OVL`, `MCGA.OVL`, `TGA.OVL`, as TASM source that assembles and links to each file byte for byte (added by the human, 2026-11-04) |
| O2 | A rebuild | `make rebuild` compiles and links `src/` into programs whose load modules, relocation tables and FBOV overlay data are identical to the originals, and the rebuilt programs play the golden sessions identically in the oracle. Accepted differences (the human, 2026-11-03 and 2026-11-04): the link date, the order of the overlay stub entries (it follows the original function names, which are not known), and in `EOB.EXE` overlay 5's three fixups of `gScrollHook` |
| O3 | A design doc | `docs/design/WHITEPAPER.md` covers every subsystem with evidence-tagged claims |
| O4 | Readable source | Real names for modules, functions, globals, parameters, constants and types, with evidence; a comment on every module and function |
| O5 | A portable build | The matched C, with a platform layer replacing DOS, video and sound, built natively (Phase 7): SDL3, starting from poolrad's platform layer (branch `track-c-hybrid`), proven by the golden sessions (the human, 2026-11-04) |

Non-goals for now: bug fixes or enhancements to game behaviour. The port keeps the original's behaviour, its quirks included (the `QUIRK` comments in `src/`), so the goldens stay an exact test; fixes belong to a separate, later build behind a switch (the human, 2026-11-04).

---

## 2. Pinned inputs

Every tool works on exactly the files in `re/inputs.toml` (`make inputs`). **The copy is the unofficial fan patch "1.9"** (`eob/version.nfo`, release 2006-07-11), built on the official 1.7, not the retail release. Decided by the human on 2026-10-31: work from these files.

| File | Bytes | Notes |
|---|---:|---|
| `EOB.EXE` | 264,480 | The game. Turbo C++, TLINK 3.0, not packed. 201,552-byte load module, 2,504 relocations, then 52,688 bytes of FBOV overlay data. File date 1991-04-29 |
| `INTRO.EXE` | 88,032 | The introduction. Same toolchain; 72,160-byte load module, 12,288 bytes of FBOV data |
| `START1.EXE` | 8,652 | The setup menu (graphics, sound, mouse); runs `INTRO.EXE`. Says "Eye of the Beholder Version 1.9": **edited by the patch** |
| `CGA.OVL`, `EGA.OVL`, `MCGA.OVL`, `TGA.OVL` | 10,594 to 13,271 | Graphics drivers loaded by the game (MZ files without an entry point) |
| `EOBDATA1.PAK` to `EOBDATA6.PAK` | | Westwood PAK archives, 193 files; dated 2006, so rewritten by the patch |
| `FONT6.FNT`, `FONT8.FNT` | | Fonts |
| `version.nfo` | | The patch's release notes |

Not pinned: `LEVELS.TMP` and `EOBDATA.SAV`, which the game writes while it runs (saving, changing level). Tools that need them copy them first.

Consequences of the patched copy:

- What the patch changed is not known yet. The data changes (items, maps, NPCs) matter to the whitepaper, not to matching; code changes matter to matching, since a hex edit may give bytes no compiler wrote. Such a byte is recorded as a patch site (`re/patches.toml`, Phase 1), and the rebuild applies it after linking, as poolrad's DOS port applies its patch series.
- If a retail copy becomes available, diffing it against `eob/` identifies the patch sites directly. That is a human decision (rule 6 of `AGENTS.md`).

**Pinned toolchain** (2026-10-31, whitepaper 2.1). The programs were built with **Turbo C++ 1.00** (May 1990) and its TLINK 3.0. Turbo C++ 1.01 (February 1991) was compared and eliminated. Both print the same "Copyright (c) 1990" banner and TLINK 3.01 writes the same 30h version byte, so only the code decides. Installed by `make tc-install` from the archives in `build/toolchain/dist/`, checked by `make tc-check` against `tools/re/tc_toolchain.toml`; never committed.

| File (release `tc100`) | Bytes | SHA-256 |
|---|---:|---|
| `BIN/TCC.EXE` | 458,560 | `1d08bb9fbd575782fee50391573e2e0c73747b823c7d8cbe9f7b2f13f751e7d4` |
| `BIN/TLINK.EXE` | 53,414 | `9e6e32c0efa26010748aba9fa2de75f7fd693fc7dc69ed97229795f160b9ed51` |
| `LIB/C0L.OBJ` | 2,236 | `b51e146481a6764978b27a31e537a88ab2fb2f79da50b6a263833c6a644fbff1` |
| `LIB/CL.LIB` | 196,096 | `cf45945fa4017ae11b321fc56a17af547123b050e653e9f940af0040dd41d0f0` |
| `LIB/OVERLAY.LIB` | 14,336 | `d4b73dceb1a675d5579ff91c5b524a09a5d30ec034d8db503bfa0a6851d198aa` |

The assembler is provisionally **TASM 2.0** (`tasm20`, May 1990): it shipped with the same TLINK (identical hash) as Turbo C++ 1.00. [H] until an assembly module is matched; TASM 2.01 is installed for that comparison.

---

## 3. What the binaries are (verified facts that shape the approach)

Evidence is in the whitepaper, section 2.

1. **Language: Borland Turbo C++ 1.00.** All three programs carry "Turbo C++ - Copyright 1990 Borland Intl." and the Borland C runtime strings; the EXE headers carry TLINK's signature. The startup code of `EOB.EXE` is 1.00's `C0L.OBJ` (all 16 differing instructions are address fields), and its runtime modules are 1.00's (`make rtl-compare`, whitepaper 2.1).
2. **Switches: `-ml -O -Z`** (large model, jump optimisation, register load suppression), with the defaults for the rest (register variables, 8086 code, no stack check, optimise for size); proven by matching two whole modules (whitepaper 2.1.1). `-K`, `-a` and `-d` are not decided yet.
3. **Segments.** The FBOV header points to TLINK's segment table: 99 entries in `EOB.EXE` (58 code, 22 data, 7 overlay stubs, 12 of a fourth kind), 54 in `INTRO.EXE`. Segments are byte aligned; the table gives the exact boundary of every code segment, and with Borland's one code segment per source file, of every module outside segment 0. DGROUP is paragraph `212E` in `EOB.EXE`; 127,435 bytes of resident code in 53 non-empty segments.
4. **Overlays (VROOMM).** 7 overlaid segments in `EOB.EXE` (125 entries, 50,641 bytes of code) and 1 in `INTRO.EXE` (22 entries, 11,741 bytes). The stub layout is Turbo Pascal's; the overlay data is appended to the EXE instead of a separate `.OVR` file. `make overlays` validates both.
5. **No packing.** The load modules are analysed as they are.
6. **Assembly.** Some code is hand-written: a segment with 36 `iret` and 77 `out` (interrupt handlers, timer, sound, video), near-return code, and probably the four graphics drivers (in scope since 2026-11-04, O1). Assembly is matched as assembly (TASM source), as poolrad's sound driver.
7. **Data.** The 6 PAK archives parse with one layout and rebuild byte for byte (`make pak`).

---

## 4. Approach

### 4.1 ADR-001: matching C with the original toolchain (2026-10-31)

Decided from the poolrad experience (its ADR-002 replaced behaviour-matching C after four days):

1. **Proof by identity.** `make match` compiles our C with the compiler that built the game and compares instructions; the compiler, not a test, says the source is the program.
2. **One pass to C.** Unlike poolrad (Pascal first, C translation later), the original language is C, so the matched source is the C we want.
3. **Independence of functions.** A function is proven on its own: callees need only prototypes, globals only declarations.
4. **The runtime is the compiler's.** The Borland runtime library (`CL.LIB` or the model's equivalent, `C0x.OBJ`) is linked, never decompiled.
5. **A real rebuild** when everything matches, compared byte for byte (O2).

Known differences from Turbo Pascal (the first two settled by the spike, section 4.2):

- Register variables (SI, DI) are chosen by the compiler from the declarations, and their order changes the code.
- Optimisation switches (`-O`, `-Z`, `-G`, `-r`) and the model (`-ml`, `-mm`) change the code; they are found per module, as poolrad found `$O+` per unit.
- String literals go to the module's part of `_DATA`, not before the function: matching compares them by content and module, and the rebuild needs the module order of DGROUP.
- `switch` statements compile to jump tables in the code segment.

### 4.2 The gate before building the matcher: passed 2026-10-31

Phase 1 had a **go/no-go spike**: three to five functions of one small C module matched by hand with the pinned compiler; if register allocation or switches could not be reproduced, the approach would be reconsidered before a full matcher was written.

**Result: go.** Two whole modules match, 8 functions and 1,078 bytes: `src/E0CF7.C` (6 functions, list building over far pointer tables) and `src/E07A9.C` (start-up allocations; a routine with string literals, a 14-byte struct table, a signed char field, a far function pointer call and an overlay call). Register allocation came out right without `register` declarations; the switches were found by elimination (whitepaper 2.1.1). Three wrong variants are rejected. The spike matcher grew into `make match` (`tools/match/cmatch.py`, 2026-10-31): it links our unit twice, the second time with data and code shifted, so every address field moves and every constant stays; bytes that do not move must equal the original's, and each field must name the same global, string, function (by annotation, or in the runtime library by `build/re/rtl_compare.json`) or the same distance into the function. It handles C units (per-file switches), assembly units (TASM 2.0), overlay code (segment fixups through the segment table), jump tables, and partly decompiled units (siblings as stubs, near calls checked by name). 16 functions in 8 units are at L2; three negative tests (a constant, a global, a literal) and one for assembly are rejected (playbook 30).

**The matcher (`make match`, `tools/match/cmatch.py`, 2026-10-31)** links our unit twice, the second time with data shifted by 111h and code by 113h, so every address field moves and every constant stays. Bytes that do not move must equal the original's; each field must name the same global, string literal, function (by annotation, or in the runtime library through `build/re/rtl_compare.json`) or the same distance into the function. It handles C units (per-file switches), assembly units (TASM 2.0), overlay code (segment fixups through the segment table), jump tables, and partly decompiled units (siblings as stubs, near calls checked by name). 16 functions in 8 units are at L2; negative tests (a constant, a global, a literal, an assembly jump) are rejected. Playbook 30 describes the annotations.

---

## 5. Repository layout

```
AGENTS.md              agent operating manual; CLAUDE.md imports it
Makefile               every command
eob/                   original game files (pinned, read-only)
docs/spec/SPEC.md      this document
docs/setup/            environment setup; SETUP-tc.md for the compiler
docs/process/playbooks step-by-step procedures
docs/design/           WHITEPAPER.md (how the game works)
docs/reference/        prior art and licence rules
re/                    reverse-engineering database (text): inputs.toml now; units, symbols, data names from Phase 1
tools/setup/           doctor.sh, inputs.py, build-emu2.sh
tools/re/              mzutil.py, fbov.py, pak.py
tests/unit/            pytest
work/status.md         where the project stands
build/                 generated, ignored; build/toolchain/ holds the compiler (never committed)
```

---

## 6. Verification ladder (as in poolrad)

| Level | Name | Evidence |
|---|---|---|
| L0 | Drafted | The function is C in its module and the module compiles with the pinned toolchain |
| L2 | Matched | `make match`: instructions identical, every address field names the same thing (global, function, literal, segment) |
| L3 | Rebuilt | The function's module is byte-identical in `make rebuild`, and the rebuilt programs pass the goldens |

Only tools write levels. The oracle (Phase 2) is a Unicorn PC in virtual time, as poolrad's, extended for what EOB needs that Pool of Radiance did not: VGA mode 13h, the mouse (INT 33h), AdLib (port 388h), and a game that runs in real time from the timer interrupt, so recordings must replay timer ticks exactly.

---

## 7. Agentic workflow

As poolrad's SPEC section 9 (roles, packets, locks, worktrees, integrator, reviewer independence), brought over when Phase 3 starts. Until then work is done in one session at a time on `main`.

---

## 10. Phases

| Phase | Deliverable | Exit check |
|---|---|---|
| P0 Bootstrap | Pinned inputs, environment check, overlay and segment manifest, PAK reader | `make doctor` and `make p0-check` pass. Done 2026-10-31 |
| P1 Map and toolchain | Done 2026-10-31. Turbo C++ release pinned by runtime comparison (1.00), compiler switches and memory model, `re/units.toml` (module per code segment), function enumeration, runtime library identified, assembly modules flagged, patch sites found, the go/no-go spike | spike passes; `make p1-check` |
| P2 Oracle and data | Unicorn PC with VGA, mouse, AdLib, timer; goldens; codecs for CPS, CMP, VCN/VMP, MAZ, INF, DAT | goldens repeatable; codec round trips. Done 2026-11-01: `make data` decodes every pinned file (the compression formats 1 and 4 checked byte for byte against the game's own routines run in the oracle; DAT, fonts, item tables, saves in `eobfmt.py`; the save round trip, and the game's own `LEVELS.TMP` equal to the save's level states); the oracle runs `EOB.EXE` and `INTRO.EXE` (which loads `EOB.EXE` itself) with keyboard, mouse and AdLib; 7 golden sessions, repeatable and independent of the checkout. The meaning of INF bytecode and VCN tiles comes with the units that use them (P4) |
| P3 Match harness | `make match` for C modules and TASM modules (done 2026-10-31, section 4.2); the agent process (from poolrad: claims, workers, integration, review) | negative fixtures rejected. Done 2026-11-01: `make match-gate` (every unit matches, every L2 function still proven, 11 negative fixtures rejected with a difference); `make claim`, `packet`, `skeleton`, `handoff`, `integrate` (through `make check`), `review-pass`, `worker`, a pre-commit guard (playbooks 30, 70, 90); rehearsed end to end in a throwaway clone |
| P4 Match | every function at L2 | status |
| P5 Rebuild | identical programs | `make rebuild`, goldens on the rebuild |
| P6 Consistency | one vocabulary across modules | review |
| P7 Portable build | platform layer, native build | goldens |
