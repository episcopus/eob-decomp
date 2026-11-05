# Eye of the Beholder: how the program works

A living document. Every claim cites its evidence (an address, a tool, a test). Claims not yet proven on our files are tagged [H].

**How to read it.** Section 1 says which copy of the game the project works on. Section 2 describes the three programs, how they were built (the compiler, its switches, the linker, the overlays, the graphics drivers) and how they start. Section 3 is the architecture of the running game: its units, memory, files, interrupts and main loop. Section 4 describes the data files. Section 5 explains each game system (the party, items, the maze, scripts, monsters, combat, magic, the camp, the interface, saving, the 3D view, the screen, sound, the intro and the clocks). Section 6 lists what is still open. The section numbers are stable: source comments and tools cite them.

**Tags.** **[V]** verified (the evidence is cited), **[I]** inference (the reasoning is given), **[H]** hypothesis or a lead from outside sources, not yet proven on our bytes, **[?]** open question. Addresses are written `1B82:00C7` (segment:offset in the load module), `ovl2:09CC` (overlay 2) and `ds:8115` (the data segment); a name in backticks is a function or global in `src/`. `make handbook` typesets this document as a PDF with diagrams (tools/handbook/README.md).

## 1. The copy we work on

The files in `eob/` are the unofficial fan patch "1.9" (`eob/version.nfo`: "based on Demo version 1.0", release 2006-07-11; fixes to items, maps and joining NPCs). [H] Third-party listings describe 1.9 as built on the official 1.7 and credit a fan known as EOBFan. Evidence of the patch in the files:

- `START1.EXE` contains "Eye of the Beholder Version 1.9" (`strings`), so it was edited.
- The six PAK archives are dated 2006-07-11; their directory terminators differ (section 4.1), which suggests they were rewritten by more than one tool.
- `EOB.EXE` and `INTRO.EXE` keep their 1991 dates; whether their bytes were edited is not known. All three programs contain the string "homie", whose origin is not known.

## 2. The programs

### 2.1 Toolchain

- "Turbo C++ - Copyright 1990 Borland Intl." in all three programs; Borland C runtime messages ("Divide error", "Abnormal program termination"; `START1.EXE` also "Null pointer assignment"). `strings`.
- EXE header bytes `FB 30 6A 72` at `1Eh`: the TLINK signature, version 30h (TLINK 3.0). `tools/re/mzutil.py` (`tlink`), `tests/unit/test_fbov.py`.
- **The release is Turbo C++ 1.00** (May 1990), large model, not 1.01 (February 1991). Both releases print "Version 1.0x Copyright (c) 1990", and TLINK 3.0 (with 1.00) and 3.01 (with 1.01) both write version byte 30h, so neither the banner nor the header decides. The code does:
  - **Startup.** A program linked with 1.00's `C0L.OBJ` and `CL.LIB` has the same first 524 bytes as `EOB.EXE` except 16 instructions, all address fields: the DGROUP paragraph, DS offsets of runtime variables, the init table segments, call targets (`main` is called at `07C8:0001`). 1.01's startup is 522 bytes and differs in 287 bytes. Scratch probe, 2026-10-31.
  - **Runtime modules.** `make rtl-compare` (`tools/re/rtl_compare.py`) links every module of `CL.LIB` and `OVERLAY.LIB` for each release and looks for each in `EOB.EXE`, instruction by instruction with address fields ignored (direct far call targets included since 2026-10-31; the first run compared them as bytes and found fewer). 1.00: 66 modules found; 1.01: 59, a subset. `C0.ASM`, `FARHEAP`, `FBRK`, `FCORELFT`, `FSEEK`, `IOERROR` and `SETVBUF` are found only as 1.00's. Modules that keep data in their code are not decoded by that comparison; scored by equal bytes instead (above 85%), `FFLUSH`, `FLUSHALL`, `TIMECVT`, `XFFLUSH`, `INT86`, `TZSET` and `VPRINTER` fit only 1.00's version, and none fits only 1.01's. `INTRO.EXE`: 64 of 1.00's modules found.
- TASM 2.0 came with the same `TLINK.EXE` as Turbo C++ 1.00 (identical SHA-256), TASM 2.01 with 1.01's. [H] The assembly modules were assembled by TASM 2.0.

<!-- FIG prog_toolchain -->

### 2.1.1 Compiler switches

Proven by matching two whole modules, `e0cf7` (6 functions, 591 bytes) and `e07a9` (2 functions, 487 bytes), with `tools/match/cmatch.py` (`make match`), 2026-10-31:

| Switch | Evidence |
|---|---|
| `-ml` large model | far calls and far data pointers; the large-model runtime |
| `-O` jump optimisation | needed: with `-Z` alone `ListInit` keeps a second `les bx`; `-O` also merges the common tails of `if (gGraphicsChoice) f(a); else f(b);` in `SetMouseItem` |
| `-Z` register load suppression | needed: neither module matches without it; `xor ax,ax; push ax` is reused for the next zero argument |
| `-r` register variables (default) | `-r-` breaks both modules (SI holds the loop counter) |
| no `-G` (optimise for size, default) | `-G` breaks both modules |
| no `-1` (8086 code, default) | `-1` breaks both modules; `shl` by 1 twice, `mov cl,4; shl ax,cl` |
| no `-N` (no stack check, default) | `-N` breaks both modules |
| `-K`, `-a`, `-d` | not decided: these modules use no plain `char`, no structure layout that `-a` changes and no repeated string |

#### Switches differ by unit

`e1b82` (`src/E1B82.C`) is compiled with `-G` (optimise for speed): it cleans the stack after calls with `inc sp; inc sp` and `add sp,4` where the `-O -Z` units use `pop cx`. Its four functions match with `-ml -O -Z -G -P`, three of them with `-ml -O -Z -G` and none without `-G`. The small units `e1a16` and `e1a63` show the same `-G` pattern. A source file names its switches with `/*@flags ... */` (playbook 30). `eov0` (`src/EOV0.C`) is compiled with `-v` (source-level debugging): its `RunCharacterPanel` (ovl0:28F9) keeps two sibling calls to `ovl0:23EC` apart that `-Z` alone cross-jumps, and `make match` needs `-ml -O -Z -v` for it (2026-11-02).

An idiom found on `eov0`: when a function keeps a *parameter* in DI and a computed *local* in SI while both are live across calls, the local is marked `register` (`DrawPanelIcon`, ovl0:0128: `register int cellRow;` puts the cell row in SI and the parameter in DI; without `register` the parameter takes SI). Turbo C promotes an explicit `register` local over a parameter (2026-11-02).

#### The C and C++ front ends differ

`e1b82`'s `Unpack` (1B82:00C7) sign-extends its format byte to long and then switches on the low word (`mov al,es:[bx]; cbw; cwd; mov bx,ax; cmp bx,4; jbe; shl bx,1; jmp cs:[bx+20d]`). `TCC`'s C front end folds `(int)(long)char` to `char` (leaving `cbw` without `cwd`) and never turns a `long` switch into a jump table, while its C++ front end (`-P`) keeps the `cwd` and emits exactly these bytes. So a unit that shows a `long` conversion before a jump table is C++ (`-P`, names then mangled, so the file wraps its definitions in `extern "C"` for the matcher). The same front end can also fold `FP_SEG(p)` on a far-pointer local to `push dx` where the C front end reloads the segment; the original 1B82 pushes the memory word, so the file keeps the block's two halves in a union.

Idioms settled by matching (playbook 30 has the full list): an assignment tested and the result unused (`mov [g],ax; or ax,ax`) is an `if (g = x) ;` with an empty body (`SetUnpackBuffer`, 1B82:0217); two `continue` tests merged into one `||` keep ES:BX loaded (`UpdateMonsters`, 08E6:0CC8); a `switch`'s case bodies are laid out in source order, so the jump table gives the order of the `case` labels.

`0ACD:091C` and `14EF:0274` keep identical case tails that the matcher's build merges (draft notes in `re/symbols.toml`). That merge is not `-O`: it is TCC's **internal assembler**, which cross-jumps a later basic block into an earlier identical one, while the original was assembled by **TASM 2.0 with SMART off**, which does not (found 2026-11-01 on `e14ef`: with `TCC -B` via TASM the whole unit matches, 6 of 6, 1410 bytes; `14EF:0274` 789 bytes). SMART off is also what fixes `e19ec`'s immediates (`or bx,1` is `0x81` in the original, `0x83` under TASM SMART, a TASM directive, not a listed switch). Open: a unit whose bytes need that inversion with `-c` cannot always take `-B` instead (TASM keeps TCC -S's forward-jump form). TCC -S inverts a conditional jump it estimates as more than 127 bytes from its target, and the estimate can exceed the built distance when the block holds a long argument computation (`e0d1c` `DrawRowList`, 0D1C:0465: with `-B`, `jne then; jmp else` where the original has `je`; probes 2026-11-01). `e0d1c` `FindBestUndeadTurner` is the mirror case: `-c` cross-jumps its two identical `bestVal = ...; best = who;` tails (6 bytes short) where the original keeps both, and `-B` keeps them apart but inverts the first `continue`, so neither flag set reproduces it; with `-c` 24 of 26 functions match, with `-B` 22 of 26.

No game code uses floating point: no emulator interrupts (`INT 34h` to `3Dh`) in any code segment. The switches are `cflags` in `tools/re/tc_toolchain.toml`; a module that needs others records them (`match_flags` in `re/symbols.toml`).

Three negative tests confirm the matcher rejects a near miss: a loop bound changed from 52 to 53, the wrong table passed, and an extra store (scratch probe, 2026-10-31).

<!-- FIG prog_matcher -->


#### Struct copies are array initialisers

[V] (2026-11-01, escalations eov6 and e0d1c). All 15 calls of `F_SCOPY@` (0000:78C9) push the destination's segment before its offset (`push ss; lea ax,[bp-14h]; push ax`), then the source, then `mov cx,n`. Turbo C++ 1.00 compiles a local *array* initialised from a constant list (`char counters[6] = { 0, 0, 0, 0, 0, 0 };`, the initialiser kept as an unnamed block in DGROUP) exactly so, and a struct initialised or assigned with the offset first. The source is an array, not another compiler: Turbo C++ 1.01, Turbo C 2.0 and 2.01, Borland C++ 2.0 and Turbo C++ 3.0 were installed and tested on the way, and all agree with 1.00 on both forms, so the pin stays. (The lead came from a research pass; CampLoop's copy then matched.) Likewise `mov ax,..; and ax,K; or ax,0; je` is Turbo C++ 1.00's bit test of a `long` (verified), not a different code generator. TASM 1.0 and 1.01 assemble e14ef like TASM 2.0 `/jNOSMART` but break other units compiled with `-B`, so TASM 2.0 stays.

#### Some units were compiled through the assembler (superseded)

(escalation e14ef, 2026-11-01; superseded 2026-11-02 by the next paragraph: the units matched this way match with `-v`, without the assembler). Turbo C++'s own code generation (`-c`) merges identical jump tails; in `UpdateOptionList` (14EF:0274) the original keeps four case tails apart, and only `TCC -S` followed by TASM gives those bytes: the whole unit e14ef matches (6 of 6, 1,410 bytes) that way and not otherwise. That is what `TCC -B` does, which a file with inline `asm` gets automatically. The assembler encoded instructions without TASM 2.0's SMART forms (with SMART, `or bx,1` becomes `83` where the original has `81`), as TASM 2.0 `/jNOSMART` does, or an earlier TASM [H], and in two passes (`/m2`): one pass pads a forward jump that turns out short with a NOP (`EB xx 90`), which the original does not have (e1414 matches whole, 19 functions, 3,511 bytes, only with `/m2`). Of the 24 C units matched on 2026-11-01, all but E07C8 also match this way, so the switch is per file: `/*@flags ... -B */`. It does not explain `0ACD:091C`.
#### The game modules were compiled with debug information

[V] (2026-11-02, escalations eov4, eov5, e122e, e0d1c). The original keeps identical code tails apart (case tails, `continue` tails, two equal stores) and, in the same functions, sizes every jump exactly. TCC's own code generation (`-c`) sizes jumps but merges the tails; `-B` keeps the tails but sizes jumps conservatively (a short `jne` 124 bytes from its target becomes `je over; jmp`), and TASM writes `xchg ax,bx` as `93` where the original has TCC's `87 D8`. With `-v` (source debugging; `-y`, line numbers, gives the same bytes) TCC does both: it does not merge code from different source lines and still sizes jumps itself. Rewording a tail does not help: the merge is on machine code. Evidence: every function that matched only under `-B` matches under `-v` (e14ef, e1414, e082c, e08e6, e0acd, e16a5, eov4); the functions that refused `-B` (CloseDoor 122E:01D9, IdentifyItemsOp 122E:1CAD, DrawRowList 0D1C:0454, SurrenderPanel ovl5:10A6, DrawSlotMonsters's `87 D8`) match under it too; three drafts no build reproduced match (ToggleWallTypeOp 122E:0A16, DrawCharPortrait ovl5:00A1, FindBestUndeadTurner); and every C unit of src/ that matches still matches with `-v` added, except e1de7. The two-file explanation (a unit made of a `-B` file and a `-c` file) fails, because in e122e and eov4 the two kinds of function alternate by address. The exception fits a pattern: e1de7 is one of the shared library modules (`-G`, the units INTRO.EXE links too), and its DrawTextLine needs its tail merged, so the library was built without debug information. The debug information stays in the OBJ files and never reaches the EXE (TLINK without `/v`), so only the code shows it. E0BD2 and EOV0 had found `-v` earlier for the same reason.

### 2.1.2 What segment 0 holds

Segment `0000` (`_TEXT`, 31,386 bytes) starts with `C0L` (524 bytes), then 19,996 bytes that are not library code (`0000:020C` to `0000:5028`), then the runtime library from `0000:5028` (`OVRHALT`, the first module found) to the end, 10,866 bytes. The 20 KB are one assembly module, Westwood's AdLib sound driver, which `src/E_TEXT.ASM` reproduces byte for byte (`make match`, the whole unit, TASM 2.0 `/mx`): code and data in one segment that runs with DS = CS. [V] Its parts, in address order: the pitch curve tables and the script flag (`020C` to `03A0`), five far routines the game calls (`03AA` to `03FA`: the block's address, a byte write and a byte read in a sound record, a sound's data pointer, and `03FA`, which adds `0416h` to the first 300 words of the block), the 32C8h data block (`0416` to `36DD`; `LoadSoundDriver` (177D:000A) reads `adlib.dat` or `pcgamesn.dat` over it, so the EXE's bytes there are an image that is overwritten), the tables (`3944`: the entry table of eleven routines, fourteen level curves), the variables (`3B36`), ten voice records of 43h bytes (`3B93`), the voice, operator and F-number tables (`3E31`), the two INT 08h handlers (`3E66` the AdLib timer, divided from a PIT divisor of 0555h into a driver tick every 0Ch passes and a BIOS chain every 30h; `3EE5` the 4-bit delta sample player, PIT divisor 004Bh), the driver (`3F77` to `4F83`: start, stop, tick, the sound queue, the per-voice script interpreter `418F` and its register helpers) and the interpreter's 74-entry command jump table (`4F94`). The PC speaker and Tandy driver is the separate unit `E1948.ASM` (segment `1948`); units `e_c0`, `e_text`, `e_lib` in `re/units.toml`. `make map` took `03FA` to `3F76` for one function and the table `4F94` for part of `SoundRandom`; `re/symbols.toml` lists the parts one by one. `INTRO.EXE` has the same three parts, its library from `0000:5400`.

### 2.1.3 Modules

`make map` (playbook 10) writes `re/units.toml` and `re/symbols.toml`:

| Program | Units | By kind ([H] until matched) | Functions |
|---|---:|---|---:|
| `EOB.EXE` | 62 | 30 C, 25 assembly, 5 unknown, startup, library (7 of the units are overlays) | 885 |
| `INTRO.EXE` | 33 | 8 C, 19 assembly, 4 unknown, startup, library; 23 units have the same code as an `EOB.EXE` unit | 250 |
| `START1.EXE` | 1 | not mapped yet | |

Matched (L2): `e0cf7` (6 functions) and `e07a9` (2 functions), whole units.

### 2.2 EOB.EXE layout

- Entry `0000:0000` is Borland's startup (`c0`): `mov dx,212Eh` then DOS version (`int 21h`, `ah=30h`) and `mov ds,dx`. DGROUP is paragraph `212E`. Stack `312D:0080`.
- The FBOV header (section 2.3) gives the file offset of TLINK's segment table, 144,400 (load module offset `20C10h`), 99 entries of 8 bytes: frame paragraph, end offset in that frame (exclusive; `FFFF` for no extent), kind, flags. `make overlays` writes it to `build/re/eob.json`.
- **Segments are byte aligned**: each starts where the previous one ends, inside the paragraph that holds that end, so the frame paragraph is the start rounded down. `main` is at `07C8:0001`, right after the `retf` at `07C8:0000` that ends the module framed at `07A9`; that module starts at `07A9:000A`. Some entries have zero length. (Phase 0 first read the second field as size - 1; corrected 2026-10-31, `tests/unit/test_cmatch.py`.)
- Kinds: 1 code (58 entries, 53 not empty, 127,435 bytes), 0 data (DGROUP among them, `212E`, ending at `212E:FFE4`), 3 overlay stub (7), 4 not established (12 entries). The flags take values 0 to 15; meaning not established.
- Segment `0000` is 31,386 bytes with 136 near returns: the startup, about 20 KB of assembly, and the runtime library (section 2.1.2). Every other non-empty code segment is one Westwood module: the runtime library is all in segment 0. Segment `17A6` (6,694 bytes) has 36 `iret`, 77 `out` and `int 10h`: [H] hand-written interrupt and hardware code.

<!-- FIG prog_exe_layout -->

### 2.3 Overlays (VROOMM)

The overlay data follows the load module in the EXE (`tools/re/fbov.py`):

| Offset | Field |
|---|---|
| +00 | "FBOV" |
| +04 | u32 bytes after this 16-byte header (52,672 in `EOB.EXE`, 12,272 in `INTRO.EXE`) |
| +08 | u32 file offset of the segment table |
| +0C | u32 segment table entries (99, 54) |
| +10 | one record per overlaid segment: code, u16 fixup offsets, zero padding to the next record |

Overlay stub segments in the load module have Turbo Pascal's layout (poolrad whitepaper, `tpovl.py`): `CD 3F` and a u16 the tools do not read at +0, u32 record offset (from +10) at +4, u16 code bytes at +8, u16 fixup bytes at +A, u16 entry count at +C, u16 link at +E, 16 runtime bytes zero on disk, then from +20h 5-byte entries `CD 3F`, u16 code offset, 0 (`tools/re/fbov.py:53`).

| Program | Stub paragraphs | Entries | Code bytes |
|---|---|---:|---:|
| `EOB.EXE` | `20F6 2102 2106 210C 2116 211E 212A` | 125 | 50,641 |
| `INTRO.EXE` | `0E96` | 22 | 11,741 |

#### Segment fixups in overlay code

Segment fixups in overlay code hold a byte offset into the segment table (index times 8), not a paragraph: overlay 2 calls `FileOpen` as `0188:00DB`, and entry 49 (188h / 8) of the table is `1C0E`, where the resident code calls it as `1C0E:00DB`. `tools/match/cmatch.py` translates them (`Original.seg`). Overlay code compiles with the resident units' switches: `ovl2:0110` and `ovl2:09CC` match without `-Y` (`src/EOV2.C`).

The padding between records is 0 to 42 bytes in `EOB.EXE` and 169 bytes after `INTRO.EXE`'s one record; the rule is not established. Fixup offsets within a record come in runs that descend within each 1 KB of code (not explained yet).

### 2.4 INTRO.EXE and START1.EXE

- `INTRO.EXE`: 54 segments, 34 code (59,286 bytes). It names the same data files as `EOB.EXE` (`eobdata*.pak`, `WESTWOOD.COL`, `SSI.CMP`, the graphics drivers): [H] the two programs share library modules.
- `START1.EXE`: one small program without overlays or a segment table: the setup menu (CGA, Tandy, EGA, VGA; AdLib or Sound Blaster, PC speaker, Tandy speaker), then runs `INTRO.EXE`. **Small model**: its first 193 instructions are Turbo C++ 1.00's `C0S.OBJ`, the 18 that differ all DGROUP offsets (scratch probe, 2026-10-31). Being edited by the patch ("Version 1.9", section 1), it may hold bytes no compiler wrote.

### 2.5 Graphics drivers

`CGA.OVL`, `EGA.OVL`, `MCGA.OVL`, `TGA.OVL`: MZ files with 40 relocations each and entry `0000:0000`, loaded by the game with DOS function 4B03h (load overlay: an image relocated to a segment of the caller's choosing, no PSP); the VGA mode loads `MCGA.OVL` (oracle trace, section 2.6). `EOB.EXE` also names `vga.ovl`, which is not in the copy.

#### Each driver is one assembly module, linked alone

[V] (`make rebuild PROGRAM=MCGA.OVL`, `tools/rebuild/driver.py`, 2026-11-04). `src/drivers/MCGA.ASM`, `CGA.ASM`, `EGA.ASM` and `TGA.ASM`, assembled by TASM 2.0 `/mx` (one pass) and linked with no start-up code, library or stack, give the whole files, header included: 11,832, 13,271, 10,594 and 11,440 bytes (EGA and TGA 2026-11-04). The headers are TLINK's (the signature `01 00 FB 30 6A 72` at 1Ch, a 512-byte header, SS:SP 0000:0000 from a module with no stack segment). What the matching showed about the source: every jump that fits in a byte is a short one with no NOP after it, so the source wrote `jmp short` (TASM's one pass pads a plain forward `jmp`); the register to register `xchg` and `test` have their operands in the order that puts the first one in the ModRM reg field; segment overrides on string instructions and `xlat` are written out. The 40 relocations are the segment words of the export table (cs:001F, `dd Routine`). `CGA.OVL`'s header asks for one paragraph more than its image (minalloc 1): TLINK writes that for uninitialised bytes after the last initialised one, which nothing reads, so their number (1 to 16 give the same file) is not known. `make match` does not prove a driver: it proves functions of `EOB.EXE` and `INTRO.EXE` linked with C0L, by `re/units.toml`, and a driver is a program of its own, so it is built and compared whole like `START1.EXE` (no level is recorded in `re/symbols.toml`).

#### The driver interface

[V] (`src/drivers/MCGA.ASM`, the callers in `src/`). The image starts with `jmp DriverEntry`. `RunOverlayRequest` (1C00:0055) loads the driver and calls segment:0000 far (1C00:0088); DriverEntry returns BX = the driver's size in paragraphs (the block is shrunk to it, 1C00:00A8) and AX = the offset of LinkDriver, which the game calls with the far pointer of its slot table, the request's words +4 and +6 (ds:8125 = 212E:802A, 1C00:00C0). The driver has the same table at cs:0003: 47 far pointers (the count is the word at cs:00BF). LinkDriver walks both: where the game's slot is not zero, the pointer is copied into the driver (an import); where it is zero, the driver's pointer is written into the game's slot (an export). The game's image holds seven pointers and forty zeros, so slots 0 to 6 are imports and slots 7 to 46 exports, the game's far pointer at ds:8046 + 4 * i being export i. LinkDriver returns carry set if a driver export slot is zero. All four drivers have the same slots; export 34 (the line offsets), 38 and 39 are data, the rest far routines.

| Slot | ds: | Game name | MCGA | CGA | EGA | TGA | What (MCGA.OVL; the others where they differ) |
|---:|---|---|---|---|---|---|---|
| 0 | 802A | import | | | | | ds:FE4E, the far pointer to the current font (gCurFont) |
| 1 | 802E | import | | | | | ds:FC0C, the eight page segments (gPageSegs) |
| 2 | 8032 | import | | | | | ds:8115, the page the driver draws to (gDrawPage) |
| 3 | 8036 | import | | | | | 19BC:0007 MemAlloc(size low, size high, flags), dx:ax the block |
| 4 | 803A | import | | | | | 19BC:0137 MemFree |
| 5 | 803E | import | | | | | 1A72:033A GetFarPointer |
| 6 | 8042 | import | | | | | 1A72:02FE SetFarPointer (only EGA.OVL calls it: LinkDriver hands the game its register shadow, cs:00C1) |
| 7 | 8046 | gCopyPage | 04F8 | 0707 | 053B | 0614 | CopyPage: copy a whole page, segment to segment (CGA: an art page to CGA pixels through the dither tables, third argument the colour map) |
| 8 | 804A | gRestoreRectToPage | 0516 | 07C2 | 057F | 065A | RestoreRectToPage: RestoreRect on the page gDrawPage names |
| 9 | 804E | gRestoreRect | 0536 | 07E2 | 059F | 067A | RestoreRect: (col, y, cols, rows, buf): a saved rectangle back to page 0 |
| 10 | 8052 | gCopyRegion | 05D4 | 0886 | 06C7 | 071D | CopyRegion: (srcCol, srcY, dstCol, dstY, cols, rows, srcPage, dstPage): a rectangle between pages |
| 11 | 8056 | gClearPage | 0693 | 095B | 07FB | 07F2 | ClearPage: fill the draw page with colour 0 |
| 12 | 805A | gDrawCell8x8 | 06AF | 0977 | 0819 | 080E | DrawCell8x8: (src, col, y): an 8 by 8 cell to the draw page |
| 13 | 805E | gDrawIndexedTile | 0717 | 09E5 | 08EE | 087A | DrawIndexedTile: (tile, col, y): a tile of the set SetTileSet names, through its colour row (CGA, EGA, TGA: does nothing) |
| 14 | 8062 | gDrawLine | 07E3 | 0AB3 | 08F3 | 0948 | DrawLine: (x1, y1, x2, y2, colour): a line, clipped to SetLineClip's rectangle |
| 15 | 8066 | gXorRect | 2617 | 2A6A | 2234 | 24DE | XorRect: (x1, y1, x2, y2, colour): XOR colour into a rectangle |
| 16 | 806A | gFillRect | 09F5 | 0E56 | 0BB8 | 0C69 | FillRect: (x1, y1, x2, y2, colour): fill a rectangle |
| 17 | 806E | (none) | 0A97 | 0FA8 | 0D15 | 0D97 | ReadCell8x8: (page, dst, col, y): an 8 by 8 cell of a page into dst; not called |
| 18 | 8072 | gGetPixel | 0B56 | 106E | 0E19 | 0E59 | GetPixel: (x, y): the draw page's pixel |
| 19 | 8076 | (none) | 0B8F | 10C1 | 0ECC | 0EA5 | MirrorCell16: (src, dst): mirror a 16 by 16 cell (EGA: does nothing); not called |
| 20 | 807A | (none) | 0C5C | 1185 | 0FEC | 0F79 | SetTileSet: (pixels, colourRows, tileMap): the tile set (CGA, EGA, TGA: does nothing); not called |
| 21 | 807E | gSaveRectFromPage | 1ABD | 1CE0 | 1889 | 1A01 | SaveRectFromPage: SaveRect from the page gDrawPage names |
| 22 | 8082 | gGetSpriteCellRle | 0C8D | 11B8 | 0FF1 | 0FAC | GetSpriteCellRle: (col, y, cols, rows, map): cut a cell from the page after the draw page into MemAlloc'd memory, runs of 0 coded (CGA, EGA, TGA: the same routine as export 16) |
| 23 | 8086 | gGetSpriteCell | 0DD9 | 11B8 | 0FF1 | 0FAC | GetSpriteCell: (col, y, cols, rows, map): cut a 4-bit cell with a 16-colour table (CGA: CGA pixels and a mask, through the map; EGA: (mask, colour) pairs for each 8-pixel group; TGA: the pixels packed two a byte) |
| 24 | 808A | (none) | 0F56 | 12EC | 1237 | 104F | DrawMaskedCell16: (maskCell, cell, sheet, col, y): a 16 by 16 cell through a mask cell (CGA, EGA, TGA: does nothing); no caller in `EOB.EXE` or `INTRO.EXE` (a byte search for the far call through the slot found none, 2026-11-05; 5.13) |
| 25 | 808E | gCopyToPlanar | 0FCD | 1365 | 123C | 10C8 | CopyToPlanar: (src, dstSeg, bytes): 4-bit pixels, two a byte, to the EGA planes (MCGA, CGA, TGA: does nothing) |
| 26 | 8092 | (none) | 0FD2 | 136A | 1309 | 10CD | SwapPageWithScreen: (page): exchange a page with page 0 (EGA: shows the page, INT 10h function 5, and exchanges the two page segments); not called |
| 27 | 8096 | gDrawShapeMaskedFlipped | 1006 | 139E | 1355 | 1103 | DrawShapeMaskedFlipped: DrawShapeMasked mirrored (CGA, EGA, TGA: does nothing); no caller (5.12) |
| 28 | 809A | gDrawShapeMasked | 12F0 | 1689 | 135A | 13EF | DrawShapeMasked: (page, shape, x, y, depth): a shape behind the scenery planes of page 4 (CGA, EGA, TGA: does nothing); no caller (5.12) |
| 29 | 809E | gPutPixel | 15DA | 1974 | 135F | 16DB | PutPixel: (x, y, colour): set a pixel |
| 30 | 80A2 | gRestoreDrawState | 1612 | 19CF | 13BE | 1732 | RestoreDrawState: put back the region SaveDrawState kept (EGA: and the latches and the registers) |
| 31 | 80A6 | gDrawShapeFlipped | 1639 | 19F6 | 1583 | 1759 | DrawShapeFlipped: DrawShape mirrored |
| 32 | 80AA | gSaveDrawState | 1A7A | 1C9D | 1746 | 19BE | SaveDrawState: keep the region and set it to the whole screen (EGA: (regs), and keep the registers and the latches) |
| 33 | 80AE | gSaveRect | 1ADC | 1CFF | 18A8 | 1A20 | SaveRect: (col, y, cols, rows, buf): a rectangle of page 0 into buf; returns its bytes |
| 34 | 80B2 | (none) | 1B9D | 1DC2 | 19A4 | 1AE2 | SetLineClip: (left, top, right, bottom): DrawLine's clip rectangle, in pixels; not called |
| 35 | 80B6 | gSetRegion | 1BC0 | 1DE5 | 19C7 | 1B05 | SetRegion: (left, right, top, bottom): the shape draws' region, in columns and lines |
| 36 | 80BA | gSetPalette | 1BE3 | 1E08 | 19EA | 1B28 | SetPalette: (rgb): load the 256 DAC registers, INT 10h AX=1012h (CGA: does nothing; EGA: the attribute controller's 16 palette registers; TGA: the video array's 16) |
| 37 | 80BE | (none) | 1C03 | 1E2A | 1A3D | 1B58 | SetTileFormat: (width, height, a, b): the tile size in cells (CGA, EGA, TGA: does nothing); not called |
| 38 | 80C2 | gDrawShape | 1CAB | 1ED3 | 1A42 | 1C02 | DrawShape: (page, shape, x, y): draw a shape, clipped to the region |
| 39 | 80C6 | gSaveRectSize | 1FF1 | 2149 | 1BC4 | 1D54 | SaveRectSize: (cols, rows): the bytes SaveRect needs |
| 40 | 80CA | gDrawText | 2543 | 2958 | 2149 | 23E0 | DrawText: (text, x, y, colour, background): a string in the current font |
| 41 | 80CE | gRowOffsetTableOff | 0328 | 0321 | 03A9 | 042E | gRowOffsets: data: the offset of each screen line |
| 42 | 80D2 | gSetRemapTable | 277B | 2C38 | 2569 | 2A6D | SetRemapTable: (colours): the remapped draws' colours |
| 43 | 80D6 | gDrawShapeRemapped | 279C | 2F9E | 23E2 | 2705 | DrawShapeRemapped: (page, shape, x, y): a 4-bit shape through the remap table |
| 44 | 80DA | gDrawShapeFlippedRemapped | 298A | 2CE6 | 25A5 | 28A3 | DrawShapeFlippedRemapped: DrawShapeRemapped mirrored |
| 45 | 80DE | (none) | 00F6 | 00F9 | 02A1 | 020A | gNibbleSwap: data: a byte with its pixels in the other order (MCGA and TGA nibbles, CGA pixel pairs, EGA bits) |
| 46 | 80E2 | gRemapTable | 030C | 01F9 | 018F | 010A | gRemapTable: data: the remap table SetRemapTable fills |

"Not called": no far call through the slot in `EOB.EXE` or `INTRO.EXE` (a search for `FF 1E` and the slot's offset, 2026-11-04). Coordinates are pixels, or 8-pixel columns where the table says col; a page number goes through gPageSegs (`PageSeg`, MCGA.OVL 0B3F), the draw routines draw to gDrawPage's page (`DrawPageSeg`, 0B1D), and the sprite cutters read the page after it (0AFA). The shape draws clip to the region SetRegion sets (cs:00CA), which SaveDrawState and RestoreDrawState keep and restore around each pointer redraw. MCGA shapes have two formats by their first byte: 0, rows of 8-bit pixels with 0 and a count for a transparent run, or else 4-bit pixels with a 16-byte colour table of their own (DrawShape, 1CAB); the remapped draws use the driver's remap table instead.

#### CGA.OVL

CGA.OVL keeps the same routines in the CGA format (two bits a pixel, 80 bytes a line, the odd lines at B800:2000). Seven exports return at once there, each followed by its old body as dead code: DrawIndexedTile, SetTileSet, DrawMaskedCell16, DrawShapeMasked and its mirror, SetPalette and SetTileFormat. Its cells (GetSpriteCell, used for exports 15 and 16) are CGA pixels followed by a transparency mask, and DrawShape draws that format.

#### EGA.OVL

[V] (`src/drivers/EGA.ASM`, 2026-11-04) draws to BIOS mode 0Dh's four bit planes at A000h (40 bytes a line in each, a page 8000 bytes of address space, the pages 200h paragraphs apart), with the same routines in a planar form. Its state between calls is write mode 2 with the graphics controller's index at the bit mask: a draw writes the bit mask (port 3CFh) and then a colour into a byte it has read first (the read loads the latches). Every register write goes through a 16-byte shadow at cs:00C1 (the two index registers, the five sequencer and the nine graphics controller registers), and LinkDriver, after the exchange, calls the game's SetFarPointer (import slot 6) with that pointer, the only driver that calls it: the game keeps it in gEgaRegShadowOff (ds:8172) and passes it to SaveDrawState, which keeps a copy at cs:00D1, stores the latches at A000:FFFF (a write with the bit mask 0) and sets the state the draws expect; RestoreDrawState reads A000:FFFF back and writes each register whose kept value differs. The pointer redraws (1BBE) bracket themselves with the two, so a pointer drawn in the middle of a draw leaves that draw's registers as they were. CopyToPlanar is real here (the wall textures, LoadWallTextures). GetSpriteCell's cells are, after the 4-byte header, one (mask, colour) pair for each colour present in each 8-pixel group, or (0, 0), the last colour byte of a group, a row and the cell flagged by bits 7, 6 and 5; DrawShape and its flipped and remapped forms write each pair as a bit mask and a colour, and gRemapTable (cs:018F) is 256 bytes, the 16 colours repeated with each flag nibble. Seven exports return at once; six of them have no body after them (DrawIndexedTile, SetTileSet, DrawMaskedCell16, the two masked shape draws, SetTileFormat) and MirrorCell16 (0ECC) is followed by its old body as dead code. SetPalette loads the attribute controller's 16 palette registers, colours above 7 moved to 10h to 17h. SwapPageWithScreen is a page flip (INT 10h function 5) instead of a copy.

#### TGA.OVL

[V] (`src/drivers/TGA.ASM`, 2026-11-04) is the Tandy choice's driver [H: the second name of gOverlayFiles, ds:3074, as Tandy is graphics choice 1]: 16 colours, four bits a pixel, two a byte (the left one high), 160 bytes a line, line y in bank y AND 3 (2000h apart) at row y SHR 2. Its code is CGA.OVL's for that format, at other offsets, with the same six stubs followed by their old bodies as dead code (SetPalette is real here: the video array's palette registers 10h to 1Fh through ports 3DAh and 3DEh) and CGA.OVL's two-bit pixel masks left unread in the image (cs:05BE). Its cells are the art's pixels packed two a byte after the 4-byte header, with no mask: colour 0 is transparent, the draws keeping the screen's nibble under a 0 nibble through a 256-byte table (gKeepMasks, cs:030A); the shape draws place a shape on a whole byte (x rounded down to even). gRemapTable (cs:010A) is 256 bytes, both pixels of a byte remapped.

#### The CGA colour maps

[V] (CGA.OVL 20E6, 2026-11-04). The pointer the game passes as CopyPage's two extra words (19EC:002A) and as GetSpriteCell's last argument is a 32-byte colour map: CGA.OVL's SetDitherTables (20E6) reads map[c] and map[16 + c], the two CGA colours of colour c, and builds the two pixel-pair tables CopyPage and GetSpriteCell convert 16-colour pixels through: on even lines map[hi] SHL 2 OR map[16 + lo], on odd lines map[16 + hi] SHL 2 OR map[lo], a checkerboard of the two colours. The other drivers do not read the pointer. This settles the names gCgaMapDefault (ds:113F), gCgaMapPlayfld (2069:0000), gCgaMapInvent (2067:0000), gCgaMapItemL1, gCgaMapItemS1, gCgaMapThrown and gCgaMapItemIcn (20A3:0000 to 20A9:0000): each is 32 bytes of values 0 to 3 in that layout, and the art they are passed with is drawn through them in CGA mode. CGA.OVL holds a map of its own at 07A0 that nothing reads.

### 2.6 Start-up and command line

`START1.EXE` asks for graphics, sound and the mouse and runs `spawnl(.., "INTRO.EXE", "INTRO.EXE", graphics, sound, mouse, "homie", NULL)` (START1 0000:0447), each choice a one-character string ('0' + index). `INTRO.EXE` passes them on to `EOB.EXE` with a fifth argument. `main` (07C8:0001):

| Argument | Use |
|---|---|
| `argv[1][0]` | graphics: `ds:8102`, reduced by '1' in 07C8:0199 to 1 Tandy (the MCB check `CheckMCB` runs), 3 VGA (clears A000:0000 with `memset`, palette `eobpal.col`), then the mode number from the table at `ds:84A2` through `int86` (1DDD:002D) |
| `argv[2][0]` | sound: `ds:B892` = digit - '1'; `ds:80E6` = sound on unless '4' |
| `argv[3][0]` | mouse: digit - '0' |
| `argv[4]` | must be "homie" (`strcmp`), or the program ends at once: the reason for the string found in Phase 0 |
| `argv[5]` | `atol`: `ds:84A0`, the video mode restored on exit |
| `argv[6]` | `atol`: `gMemTopSeg` (`src/E07C8.C:311`); `INTRO.EXE` passes "0" |

Without enough memory `main` prints "Not enough memory to run program." through `PrintString` (19EA:0000, `src/E19EA.ASM`).

`INTRO.EXE` does not exec `EOB.EXE` [V] (oracle, sessions `intro-play` and `intro-skip`). When the intro ends (after 147 s of virtual time) or on ESC it writes the command tail " 3 4 0 homie 3 0" (its own four arguments, the video mode to restore, and a sixth argument "0", which `main` reads with `atol` as `gMemTopSeg`, `src/E07C8.C:311` [V]) into its own PSP, fills the PSP's two FCBs with DOS 29h, loads `EOB.EXE` with DOS 4B03h (load overlay) at segment 1000h, the same place DOS loads it, and jumps to it. So `EOB.EXE` addresses are the same whether it runs alone or after the intro, and the title menu belongs to `EOB.EXE`.

#### INTRO.EXE shares its library with EOB.EXE

[V] (`make twin`, 2026-11-02). 23 of its 33 units are EOB units again: the same object module, every instruction and length equal, only addresses moved, each EOB address to one INTRO address (e1948's data block 4 bytes later, its code one byte earlier). They are the sound, graphics, input, file and memory routines (e1948 to e1e8b, the C ones e19bc, e19ec, e1a29, e1a47, e1c0e, e1de7 included), 164 functions proven with their twins' sources. INTRO.EXE's own code is its overlay (22 functions), i0892 (its out-of-memory exit at 0892:08C8 among them, compiled `-G` like e1b82), segment 0 (i_text) and four small units; i0aed is e1b82 in another version (it differs at +0204) and i0923 is three bytes shorter than e1990.

#### START1.EXE is small model

[V] (2026-11-03). Its 8,140-byte load module has 3 relocations, all the DGROUP paragraph 01AA loaded by the start-up code (0000:0001), and no far call: one code segment, near calls (`-ms`, C0S and CS.LIB). `rtl_compare.py --model s` finds 38 Turbo C++ 1.00 library modules in it (31 of 1.01's); the library runs from ATEXIT at 0682h to the end of the code at 1AA0h (OVRHP, "found" at 0574h, is one byte of code and matched by chance; review of s_text, 2026-11-03). With no overlays TLINK wrote no segment table, so the map is three units: the start-up code (0000 to 0239h, the size of the small model's C0.ASM), Westwood's code (s_text, 0239h to 0682h, 1,097 bytes, 15 functions in two modules: a C module, the menu at 0000:0239 that ends in `execl` of INTRO.EXE and a string printer at 0000:0474, and an assembly module of video probes (0496 to 05F5) and console and mouse routines (05F6 to 0681), src/S_TEXT.C and src/S_TEXT.ASM) and the library. Its strings show the patch's edit ("Eye of the Beholder Version 1.9"); a byte the patch changed in code is a patch site (SPEC 2), not a source form.

#### START1.EXE rebuilds identical

[V] (`make rebuild`, 2026-11-03): src/S_TEXT.C, src/S_TEXT.ASM and a data module generated from the original's DGROUP, linked by TLINK 3.0 between C0S and CS.LIB, give the whole file, all 8,652 bytes, header included. The original link order is the start-up code, the C module (0239h), the assembly module (0496h), the library; the program's data is ds:00A8 to 0340 (after C0.ASM's 0A8h bytes, before the library's, which the probe link finds shifted by 0298h) and its uninitialised data ds:052C to 053A, past the end of the image. The C module's code is byte aligned: it follows the start-up code at the odd 0239h.

#### INTRO.EXE rebuilds but for 42 bytes

[V] (`make rebuild PROGRAM=INTRO.EXE`, 2026-11-03): the load image, the relocation table (806 entries, in order) and the overlay are the original's except the link date and the overlay stub order. What the rebuild showed about how it was built (playbook 50, items 10 to 17): the initialised data was owned by modules, among them three data-only modules (empty code segments in TLINK's table at 0892:0000, 0A19:0000 and 0AED:000D, their data's relocations right after the module before them); I0892.C and E1C0E.C held data of their own; uninitialised globals sit in their modules' `_BSS` (ds:28A0 to 2E30) before the library's, not communal; the program defines OVERLAY.LIB's `_ovrbuffer` itself (00C7h paragraphs, ds:1A28); E1948.ASM's code segment is byte aligned and its `SND` data segment byte aligned in class DATA (frame 110C, its data from ds:26DA); the `db 0` that src/ keeps at the start of E1BBE.ASM, I0CDA.ASM and E1D9A.ASM is a word-alignment pad (the segment table records each segment's start after alignment). TLINK wrote the output name ("intro.exe") and the date of the link into the overlay data: **29 April 1991**. The stub order: TLINK numbers an overlay module's stub entries in the reverse order of its public definitions, and Turbo C++ 1.00 writes a module's publics in the order of its symbol hash of their names, not of definition (tested: six functions defined in three orders give one order), so the stub table encodes the original names of IOV0's 22 functions, not recovered.

#### EOB.EXE rebuilds but for the stub order, the link date and one hook

[V] (`make rebuild PROGRAM=EOB.EXE`, 2026-11-03): the load image (201,552 bytes), all 2,504 relocations in the original's order, every overlay's code to its length and every overlay fixup but three are the original's. What differs: the link date (29 April 1991, as INTRO.EXE); the stub entries, which follow the original function names (the stub tables, 26 far pointers in the image, far calls in five overlays); and three fixups in overlay 5, where `gScrollHook` is set to its own MessagePanel through the stub, as Turbo C++ writes only for a function of another module [H]. What the rebuild showed about how it was built (playbook 50, items 18 to 27): the overlay modules were linked among the resident ones, EOV0 and EOV1 after E1414 and EOV3, EOV5 and EOV6 after E177D (their data and its relocations say so; EOV2 and EOV4, with no data, go between their neighbours, as TLINK numbers overlays in link order); 15 overlay functions were static (no stub entry: five of overlay 0's, ten of overlay 1's); several overlay modules owned initialised globals between their templates (E122E's gLaunchPositions, EOV5's menu tables of far pointers and gYesNoMenu), so src/'s externs there are definitions in the original; the program defines `_ovrbuffer` (02CBh paragraphs, ds:3088); E07C8.C's gArgv is the library's `_argv`; E_TEXT.ASM's byte at 0000:03A0 is the far object overlay 0 reads (now `gScriptFlag` in both files); a segment in an overlay's code is TLINK's segment-table index times 8, so EOV3.C's `MK_FP(0x1d8, 0)` is gWallSetCgaMaps (entry 59).

### 2.7 Input and sound

- **Keyboard: the game's own INT 9 handler.** `EOB.EXE` never calls INT 16h; its handler (assembly unit `e1aad`) reads scancodes from port 60h (1AAD:080C) and acknowledges the interrupt controller itself. It buffers keys: keys typed before the title menu exists are applied once it does (oracle, `tests/sessions/boot-title.keys`).
- **Mouse: an INT 33h event handler.** The game (1A72:0106) looks at the INT 33h vector and decides there is no driver if it is null or points at an `IRET`; otherwise it resets the driver (function 0, expects FFFFh), reads the position (function 3) and treats x above A0h as a 640-wide range (doubling its own x), sets the position (4), installs a handler at 1A72:017A for moves and both buttons (function 0Ch, mask 1Fh) and shows the pointer (1). The game draws its own pointer.
- **Arrow keys move without turning**: UP forward, LEFT and RIGHT one square sideways (`tests/sessions/load-walk.keys`).
- **AdLib**: with sound '1' the game does not detect the card; it sets waveform select (register 01h = 20h), clears the CSM and rhythm registers and puts every operator at full attenuation (40h to 55h = 3Fh): 21 writes before the title menu. In the dungeon it makes 5,334 register writes in the golden session `load-walk` (`tests/sessions/golden/load-walk.json`, `adlib_writes`; section 5.14).
- **Saved game**: "Load game in progress" reads `EOBDATA.SAV` and writes `LEVELS.TMP` (the levels visited, kept while playing).

**In the oracle** (`make oracle-run`, `tools/oracle/run.py`, arguments `3 4 0 homie 3`), `EOB.EXE` sets mode 13h, loads `MCGA.OVL`, both fonts and four PAKs, runs the overlay manager, and shows its title menu ("Load game in progress", "Start a new party", "Exit to DOS") after 30 million instructions; two runs end in the same state digest (`tests/unit/test_oracle.py`).

## 3. Program architecture

How the running game is organised: its units and overlays, memory, files, interrupts, clocks and main loop. Sections 2.x say how the programs were built and started; this section says how they run.

### 3.1 The game in one page

Eye of the Beholder is a real-time, first-person dungeon crawl on a square grid. This page says only what the code shows, and the rest of the chapter takes each piece apart.

**The screen.** The picture is 320 by 200 pixels. The first-person view is 128 by 96 pixels at (24, 8). Six party boxes of 63 by 50 pixels stand at the right (x = 184 and 256, y = 2, 54 and 106). A message window runs along the bottom (text window 7, 272 by 18 pixels at (8, 179)). Six small arrows, 21 by 16 pixels, sit under the view at the left. [V] Node list 1b (`ds:506A`) and the region table (`ds:009F`); section 5.10 draws them.

**The party.** The player makes four characters before the first step and may end with six: `gChars` holds six 243-byte records (`ds:F580`), the camp menu refuses to drop a member when only four are left ("You cannot have less than four characters."), and overlay 5 has the panels that add members (`AddPartyMember`, `ovl5:070E`). [V] A character picks one of 12 race-and-sex entries (six races), one of 15 classes (single, double and triple), one of 9 alignments, and rolls six abilities by summing four 1d6 and dropping the lowest (`RollAbilityScore`, `ovl0:1177`). [V] The tables are at `ds:11AD`, `ds:11DD` and `ds:1240`.

**The world.** There are twelve levels (`gCurLevel`, 1 to 12). Each is a 32 by 32 grid of 1,024 cells; the party stands in one cell and faces one of four directions (0 north, 1 east, 2 south, 3 west). One key press moves one cell forward, back or sideways, or turns 90 degrees. [V] `gMaze` is 1,024 records of 9 bytes at `ds:B894`; the step offsets are -32, +1, +32 and -1 (`ClickFloorItem`, `src/E0ACD.C:517`). The levels use five wall sets, named by the data files: `BRICK` (levels 1 to 3), `BLUE` (4 to 6), `DROW` (7 to 9), `GREEN` (10, 11) and `XANATHA` (12). [V] Decoded from the headers of the twelve `LEVELn.INF` files.

**Time.** The game is real time with no turn mode. A 18.2 Hz clock drives a table of 16 timed effects: a thrown item moves every 3 ticks (0.16 s), a monster group every 15 to 35 ticks (0.8 to 1.9 s, set by the level), a member's food drops one point every 1,080 ticks (59 s) from at most 100. The clock that counts game time stops while the camp menu, a level load or a spell animation runs, so those never cost the party time. [V] Section 3.6 and the next page.

**What else.** Monsters are 28-byte records, 30 to a level; items are 14-byte records, 500 in all; spells come from two lists (mage and cleric) of five spell levels and are learned or prayed for by resting; the level data carries scripts that open doors, move the party, spawn monsters and talk to non-player characters. The game ends in one of three ways: the party dies ("All of your party has been defeated."), the player quits, or the winner panel prints a password built from the party's names (`WinnerPanel`, `ovl5:160A`). [V]

#### 3.1.1 The top-level structure

The program is one loop. `main` sets the machine up; the title menu, which runs inside the start-up, returns when a game is loaded or a party is made; then `RunMapLoop` runs for the rest of the session and only leaves through `exit`. [V] `Main` (07C8:0001), `StartupGame` (07C8:0199), `StartupAlloc` (07A9:000A), `LoadItemData` (ovl2:09CC), `LoadGamePanel` (ovl2:0846), `RunMapLoop` (0ACD:0E8A). Section 3.2 follows the chain.

<!-- FIG arc_states -->

Everything the player sees after the title menu is the map screen. What changes is the node list that `gPanelMode` and `gFlag` select: list 1 (mode 0, the map), 2 (mode 1, the inventory of one member), 3 (mode 2, the character sheet), list 4 while a member is picked up to be swapped, list 5 while a spell waits for its target, and the "a" variants of lists 1 to 3 while a spell panel covers the lower view (`gFlag` = 1). [V] `BuildList1` to `BuildList5` (`src/E0CF7.C`), `OpenSpellPanel` (0EF3:21E0). The camp menu, the event panels and level changes are calls made from the loop, not states of their own.

#### 3.1.2 What a tick is, and what a pass does

Three clocks exist and they must not be confused. [V]

- The **PC tick** is IRQ0 at 18.2 Hz. Its handler (`INT 1Ch`, 1D8F:0055) adds 1 to several counters (section 3.6).
- The **game clock** `gGameTicks` (`ds:8492`) is one of those counters, but it only counts while bit 1 of `gTimerEnableFlags` (`ds:849E`) is set. All timed effects and the level timer are measured on it.
- A **pass** is one turn of the `for (;;)` in `RunMapLoop`. It is not timed: `RunMapLoop` itself has no wait and its speed is the speed of the machine, except that `DrawViewFrame` holds for `gFrameWaitTicks` after an attack or hit flash and the AdLib `PlaySoundAtCell` waits for the driver's busy count.

One pass does six things in this order (`src/E0ACD.C:925`):

<!-- FIG arc_tick -->

1. `PollMapInput`: `DispatchKeyWait` takes one event from the key ring if one waits (it never blocks), matches it against the node list and runs the node's callback; then `HandleMapKey` reads the same key for the keyboard-only commands (section 5.10). The callbacks do the real work: `MoveForward`, `ClickFloorItem` and the rest.
2. `RunEffects` (1414:04E7): for each of the 16 entries of `gEffects` (`ds:5BA8`) that is armed and whose `expiry` is before `gGameTicks`, call its handler with its argument and, if the handler left it armed, set `expiry = gGameTicks + period`. The expiry is taken from the current tick, not the old expiry, so a late effect does not run twice to catch up. It ends with `TickLevelScriptTimer`.
3. `DrawViewFrame(1)` (overlay 4), only if some action set `gViewDirty`.
4. The ambient sound: on levels 1 to 3 only, when `gNextRandomSoundTick` has passed, plan the next one (1d10 + 3 times 18 ticks away) and play sound 1Eh at a cell 0 to 11 away from the party.
5. `PlaySoundDefault(0)`: because `PlaySoundAtCell` plays the sound recorded by the previous call and records the new one, this call is what actually starts the sound the callbacks asked for during the pass (section 5.14 notes the one-behind quirk).
6. `TurnUndead` (0D1C:0A43): on levels 2 and 7 only, if the cell ahead holds monsters, the best turner in the party (a cleric or paladin who holds a holy symbol) rolls 1d20 against each undead type there, with no key press.

The timed effects are where the game's rhythm lives. The 16 entries are: `MoveDDEffects` (thrown items and spell missiles in flight, period 3), `StepDoor` (a moving door, 5), `BlinkSelected` (9), `CheckFarTable` (animated walls, 10), `UpdateMonsterGroup` four times (20 initially), `RandomiseMonsterOffsets` (25), `RunCharEffects` six times, one per member (50), and `DecayMemberState` (1,080). [V] Decoded from the image; the periods are in ticks. Monsters are not updated per pass: `UpdateMonsters(kind)` (08E6:0CC8) runs the monsters of one *update group* when that group's effect fires, and each group has its own phase. `LoadLevel` reads up to two (index, period) pairs from the level file and stores each as the period of effects 2k+2 and 2k+3; it starts effect 2k+2 one period ahead and 2k+3 half a period ahead. [V] `src/EOV3.C:576-587`. So a level has two monster speeds, each in two phases half a period apart; a monster's group is the file's group times 2 plus its slot's parity (`InitMonster`). The decoded periods:

| Level | Wall set | First monster kind (period) | Second monster kind (period) | Step timer fires every |
|---|---|---|---|---|
| 1 | brick | kobold (25) | leech (35) | none |
| 2 | brick | zombie (35) | skeleton (25) | 80 steps |
| 3 | brick | kuotoa (35) | flind (25) | 100 |
| 4 | blue | spider (15) | none (25) | 150 |
| 5 | blue | dwarf (25) | spider (15) | 50 |
| 6 | blue | kenku (25) | mage (25) | 50 |
| 7 | drow | drowelf (25) | skelwar (25) | 50 |
| 8 | drow | drider (25) | hellhnd (15) | 50 |
| 9 | drow | rust (35) | disbeast (15) | 80 |
| 10 | green | shindia (25) | mantis (15) | 80 |
| 11 | green | xorn (35) | mflayer (25) | 50 |
| 12 | xanatha | xanath (15) | golem (35) | 50 |

The names are the strings in `LEVELn.INF` (the file names of the monster pictures); that pair k belongs to monster kind k is [I] from the fact that the spider, listed first on level 4 and second on level 5, gets 15 both times. The last column is the level timer (`gLevelTimerMode` 2): `TickLevelScriptTimer` (08E6:1410) counts the party's steps (`MoveParty` adds one per step, 122E:0001) and, when the count reaches the level's number, runs the level's global script (`RunCellScript(0, 0x20)`), kills the first monster more than 3 cells away if all 29 slots are in use, and resets the count. [V] No level uses the other two timer modes (1: by clock, 3: both); the code for them is dead in this copy. [I]

A step is itself a small sequence (`MoveParty`, 122E:0001): wake monsters near the old cell, set the new cell, run the old cell's script with trigger 2 (leaving) and the new cell's with trigger 1 (entering), wake monsters near the new cell, count the step, set `gViewDirty`, print a newline in the message window (so the messages scroll with each step) and test the projectiles. [V]

### 3.2 The run from START1 to the game loop

Sections 2.4 and 2.6 cover `START1.EXE`, `INTRO.EXE` and the arguments; by the time `EOB.EXE` runs, the C start-up code (`C0L`, 524 bytes) has set DS to DGROUP (212E), run the runtime's init table, built `argv` and called `main` at 07C8:0001. [V] The figure lists what the game does from there.

<!-- FIG arc_startup -->

**Stage 2, `Main`.** It stores the three digits (`gGraphicsChoice`, `gSoundCard`, `gSoundOn`, and the mouse digit in a local), refuses to run unless `argv[4]` is "homie", saves the mode to restore (`argv[5]`), saves the INT 3Fh vector in `gSavedInt3F` and puts its own hook there (`Int3FHandler`, 1A72:0006). It then checks `LargestFreeBlock() >= 65,000` (the largest single page it will ask for) and prints "Not enough memory to run program." if not. Last it sets the two hooks `gExitHook` (07C8:061D, a stub that returns 1) and `gAllocFailHook` (07C8:0625, `AllocationFailure`), and installs the keyboard, the hard-error handler and the tick handler. [V] `src/E07C8.C:217-268`.

**Stage 3, `StartupGame`.** The graphics digit is reduced to 0 CGA, 1 Tandy, 2 EGA, 3 VGA (MCGA.OVL), 4 for a `vga.ovl` this copy does not have. The Tandy path checks the DOS memory chain ("Memory chain corrupt!"), reserves 64 KB under the video segment and reads a second number from `argv[6]`; the VGA path clears A000:0000. Then, in order: `SetVideoMode`, `InitMouse` (INT 33h), `RunOverlayRequest` loads the driver file (an `.OVL`, not a VROOMM overlay; section 2.5) and fills the function table at `ds:802A`, the page table `gPageSegs` is built (section 3.4), `FONT8.FNT` and `FONT6.FNT` are loaded (the error message always names `Font8.fnt`, a small bug), the palette buffer is allocated (16 bytes for Tandy and EGA, 768 for the two VGA choices, none for CGA), `InitOverlayCache` builds the file table (section 3.5), `CalibrateRetrace` samples the retrace bit, `srand(time(NULL))` seeds the dice (section 5.16), `SetTextWindow(0)` selects the full-screen window, and the sound card is started (`StartAdlibDriver`, or `SoundDriverInstall` for the speaker and Tandy). [V] `src/E07C8.C:285-434`.

**Stages 4 and 5.** Back in `Main` the palette is loaded (`eobpal.col` for the two VGA-class choices), `gKeyboardOptions` is set to 1 (the key driver's "auto-repeat allowed" bit [I], section 3.6), the text colours are 0Fh and 0Ch, and the mouse is disabled when no driver answered or the argument says so. `StartupAlloc` makes five far allocations (section 3.4), takes three page buffers, loads the sound driver (`LoadSoundDriver`), gives the unpacker its work buffer (`SetUnpackBuffer(gPageSegs[7], 0xFC00)`) and calls `LoadItemData`.

**Stage 6, the title menu.** `LoadItemData` (ovl2:09CC) is not only the loader of `ITEM.DAT`: it sets the pointer image, clears the shape tables and gives each of the six characters a 1,300-byte portrait buffer (`InitGameTables`), reads `ITEM.DAT` and `ITEMTYPE.DAT`, sets the interface colours, draws the `INTRO` picture and calls `LoadGamePanel`. [V] `src/EOV2.C:439`. `LoadGamePanel` shows three entries from the table at `ds:65DE` and loops on `UpdateOptionList` until one is chosen:

| Entry | Text on screen | What happens |
|---|---|---|
| 0 | LOAD GAME IN PROGRESS | `gGameLoadState` = 0FFh, `LoadGame`; on success `gGameLoadState` = 0FEh and the panel returns; on failure "Unable to load your saved game!" and the menu again |
| 1 | START A NEW PARTY | the 8x8 font, `RunCharacterPanel` (ovl0:28F9), then the panel returns with `gGameLoadState` = 0 |
| 2 | EXIT TO DOS | `ShutdownGame`, `exit(0)` |

**Stage 7.** After the title menu `StartupAlloc` loads the rest of the art (`LoadDecorateIcons`: the item sheets, `DECORATE`, `ITEMRMP`) and calls `RunMapLoop`. `EnterMapScreen` (0ACD:0F72) tells the two entries apart by `gGameLoadState`: zero means a new party and `ResetMapScreen` loads level 1 and puts the party in cell 0x1EA (490: row 15, column 10) facing north; nonzero means a loaded game, so the level in `gCurLevel` is loaded with `LoadLevel`. Either way it then loads the `PLAYFLD` and `INVENT` sheets, redraws the six characters and selects text window 7. [V] `src/E0ACD.C:957-992`.

**The way out.** `ShutdownGame` (07C8:058A) undoes the set-up in the other order: the sound card off, the exit hook, `RemoveMouseHandler`, `ShutdownOverlayCache` (frees every cached file copy and returns to the start drive), `RemoveTickHandler`, `RemoveKeyboard`, `SetVideoMode(7)` (index 7 means "the mode `argv[5]` gave"), the INT 3Fh vector put back, `TrimMemoryToVideo` on the Tandy path, and `exit(0)`. An allocation failure ends the same way after "Allocation failure, returning to DOS!" and a key press. [V] `src/E07C8.C:441-495`.

### 3.3 Code organisation

`EOB.EXE` has 62 units (section 2.1.3): 7 are overlays, 2 are the Borland start-up code and runtime library, and 53 are the game's resident code, drivers and services. Each unit is one Turbo C++ module or one TASM module; each has a segment of its own except the three that share segment 0000 (start-up code, AdLib driver and library). `src/` names a unit's file after its segment paragraph (`E0ACD.C` is the unit at 0ACD:0008). The table below gives all 62 by address, size and role.

| Id | File | Start | Bytes | Fns | Role |
|---|---|---|---:|---:|---|
| `e_c0` | (runtime) | `0000:0000` | 524 | 7 | Borland start-up code (C0L): DOS version, DS, init tables, argv, calls main |
| `e_text` | `src/E_TEXT.ASM` | `0000:020C` | 19,996 | 36 | Westwood AdLib sound driver, code and data in one segment (section 2.1.2) |
| `e_lib` | (runtime) | `0000:5028` | 10,866 | 123 | Borland C runtime library |
| `e07a9` | `src/E07A9.C` | `07A9:000A` | 487 | 2 | StartupAlloc, SetMouseItem |
| `e07c8` | `src/E07C8.C` | `07C8:0001` | 1,610 | 5 | main, StartupGame, ShutdownGame, exit and allocation-failure hooks |
| `e082c` | `src/E082C.C` | `082C:000B` | 2,976 | 28 | shared helpers: field editor, RollDice, WaitTicks, screen images, panels |
| `e08e6` | `src/E08E6.C` | `08E6:000B` | 6,331 | 36 | monsters: update dispatch, behaviours, hits, spawning, level script timer |
| `e0a72` | `src/E0A72.C` | `0A72:0006` | 1,458 | 15 | character accessors, class tables, hit-point rolls, strength and dexterity bonuses |
| `e0acd` | `src/E0ACD.C` | `0ACD:0008` | 4,182 | 35 | map screen action handlers (move, turn, portraits, items), RunMapLoop |
| `e0bd2` | `src/E0BD2.C` | `0BD2:000E` | 4,681 | 22 | inventory and item use, per-cell item lists |
| `e0cf7` | `src/E0CF7.C` | `0CF7:0007` | 591 | 6 | builds the node lists (BuildList1 to 5) |
| `e0d1c` | `src/E0D1C.C` | `0D1C:0006` | 3,265 | 26 | party status, member selection, turn undead, item icons, framed panels |
| `e0de8` | `src/E0DE8.C` | `0DE8:0007` | 4,279 | 20 | combat rolls, damage, healing, experience, armour class |
| `e0ef3` | `src/E0EF3.C` | `0EF3:000E` | 13,219 | 104 | spells and timed character effects, spell panel, particle bursts |
| `e122e` | `src/E122E.C` | `122E:0001` | 7,775 | 40 | cell script interpreter, MoveParty, wall handlers |
| `e1414` | `src/E1414.C` | `1414:0000` | 3,511 | 19 | timed effect scheduler, projectiles, character effects, food decay |
| `e14ef` | `src/E14EF.C` | `14EF:0007` | 1,410 | 6 | option lists (title menu, character creation lists) |
| `e1547` | `src/E1547.C` | `1547:0009` | 4,362 | 8 | character information screen (RedrawMember) |
| `e1658` | `src/E1658.C` | `1658:0003` | 1,238 | 12 | node list operations: add, find, toggle, enable |
| `e16a5` | `src/E16A5.C` | `16A5:0009` | 3,457 | 5 | node event dispatcher and DrawNode |
| `e177d` | `src/E177D.C` | `177D:000A` | 646 | 4 | sound data loader, PlaySound family |
| `e17a6` | `src/E17A6.ASM` | `17A6:0000` | 6,694 | 4 | graphics helpers: tile blitter, sprite unpackers, Distance, translation tables |
| `e1948` | `src/E1948.ASM` | `1948:0006` | 1,147 | 13 | PC speaker and Tandy sound driver |
| `e1990` | `src/E1990.ASM` | `1990:0001` | 710 | 5 | DOS memory control blocks (the Tandy memory reserve) |
| `e19bc` | `src/E19BC.C` | `19BC:0007` | 728 | 5 | far heap: MemAlloc, MemFree, MemRealloc, LargestFreeBlock |
| `e19ea` | `src/E19EA.ASM` | `19EA:0000` | 34 | 1 | PrintString (DOS console output) |
| `e19ec` | `src/E19EC.C` | `19EC:0002` | 124 | 3 | page table lookups: GetPageBuffer, GetPageVideo, CopyPage |
| `e19f3` | `src/E19F3.C` | `19F3:000E` | 137 | 1 | CopyRegion, a rectangle clipping front end |
| `e19fc` | `src/E19FC.ASM` | `19FC:0007` | 420 | 1 | CompressLcw, the LCW encoder |
| `e1a16` | `src/E1A16.C` | `1A16:000B` | 43 | 2 | WaitFrames, WaitFrame |
| `e1a19` | `src/E1A19.ASM` | `1A19:0006` | 102 | 2 | drive probes for the file search |
| `e1a1f` | `src/E1A1F.C` | `1A1F:000C` | 163 | 1 | DrawBox |
| `e1a29` | `src/E1A29.C` | `1A29:000F` | 470 | 7 | region front ends (DrawShape and kin), SelectRegion |
| `e1a47` | `src/E1A47.C` | `1A47:0005` | 447 | 3 | font loader: LoadFont, SetFont |
| `e1a63` | `src/E1A63.C` | `1A63:0004` | 42 | 2 | DOS critical error (INT 24h) handler |
| `e1a65` | `src/E1A65.C` | `1A65:000E` | 200 | 1 | CalibrateRetrace |
| `e1a72` | `src/E1A72.ASM` | `1A72:0006` | 841 | 8 | mouse driver glue, the INT 3Fh hook, interrupt enable and disable |
| `e1aa6` | `src/E1AA6.C` | `1AA6:000F` | 54 | 1 | PrintInt |
| `e1aaa` | `src/E1AAA.C` | `1AAA:0005` | 55 | 1 | RandomRange |
| `e1aad` | `src/E1AAD.ASM` | `1AAD:000C` | 3,405 | 19 | keyboard driver: INT 9, key ring, key queries, input record and replay |
| `e1b82` | `src/E1B82.C` | `1B82:0009` | 547 | 4 | LoadFile, LoadPacked, Unpack, SetUnpackBuffer |
| `e1ba4` | `src/E1BA4.C` | `1BA4:000C` | 57 | 1 | PrintLong |
| `e1ba8` | `src/E1BA8.ASM` | `1BA8:0005` | 140 | 4 | Unpack0 (far copy), NormalizePtr |
| `e1bb1` | `src/E1BB1.ASM` | `1BB1:0001` | 208 | 2 | SetMousePointerShape |
| `e1bbe` | `src/E1BBE.ASM` | `1BBE:0001` | 860 | 7 | the pointer: hide, show, locked pairs, zone hide |
| `e1bf3` | `src/E1BF3.ASM` | `1BF3:000D` | 196 | 4 | cursor position setters |
| `e1c00` | `src/E1C00.ASM` | `1C00:0001` | 235 | 3 | loader of the graphics driver *.OVL (RunOverlayRequest) |
| `e1c0e` | `src/E1C0E.C` | `1C0E:000C` | 6,099 | 31 | file layer: FileOpen, FileRead, the file table, error dialog |
| `e1d8c` | `src/E1D8C.ASM` | `1D8C:0000` | 50 | 1 | RandomByte |
| `e1d8f` | `src/E1D8F.ASM` | `1D8F:0002` | 185 | 3 | the INT 1Ch tick handler and its install and remove |
| `e1d9a` | `src/E1D9A.ASM` | `1D9A:000B` | 1,067 | 8 | the four unpackers (formats 1 to 4) |
| `e1ddd` | `src/E1DDD.C` | `1DDD:0006` | 106 | 2 | SetVideoMode (INT 10h) |
| `e1de4` | `src/E1DE4.ASM` | `1DE4:0000` | 57 | 2 | vertical retrace polling |
| `e1de7` | `src/E1DE7.C` | `1DE7:0009` | 2,619 | 9 | text console: windows, DrawText, scrolling |
| `e1e8b` | `src/E1E8B.ASM` | `1E8B:0004` | 2,329 | 31 | the VROOMM overlay manager |
| `eov0` | `src/EOV0.C` | `ovl0:0000` | 10,873 | 35 | party creation and the character panel |
| `eov1` | `src/EOV1.C` | `ovl1:0000` | 9,864 | 12 | camp menu, spell screens, button lists, Yes/No and message boxes |
| `eov2` | `src/EOV2.C` | `ovl2:0000` | 3,332 | 11 | art sheet and item loading, title menu, find-the-word panel |
| `eov3` | `src/EOV3.C` | `ovl3:0000` | 6,961 | 23 | level load, level state, SaveGame, LoadGame, wall sets |
| `eov4` | `src/EOV4.C` | `ovl4:0000` | 8,259 | 18 | first-person view renderer |
| `eov5` | `src/EOV5.C` | `ovl5:0000` | 8,094 | 31 | NPC and event panels, temple, endings, winner password |
| `eov6` | `src/EOV6.C` | `ovl6:0000` | 3,258 | 6 | resting (CampLoop) and its helpers |

<!-- FIG arc_units -->

The roles group like this. Start-up, runtime, sound drivers and the AdLib driver in `e_text` are the oldest layer; the platform layer (keyboard, mouse, pointer, timer, retrace, video mode, memory, files, unpackers, text console) is about 40 small units, most in assembly; the UI toolkit (nodes, option lists) sits on it; the game proper is the map-screen handlers, the world (scripts, monsters, timers, spells) and the party (characters, items, combat). More than a quarter of all the code (50,641 of 178,076 bytes) is overlaid. [V] The table and the figure.

#### 3.3.1 Overlays: which feature is where

The seven overlays are Borland VROOMM overlays (section 2.3): code that lives in the EXE file after the load image and is read into a shared pool on demand. [V] What each holds:

| Overlay | Stub | Entries | Code bytes | Holds |
|---|---|---:|---:|---|
| 0 | 20F6 | 30 | 10,873 | party creation (race, class, alignment, stats, name, portrait) and the character panel's drawing helpers |
| 1 | 2102 | 6 | 9,864 | the camp menu and everything it opens: spell memorising and praying, scroll copying, dropping a member, the button lists, `AskYesNo` and `ShowMessageBox` |
| 2 | 2106 | 11 | 3,332 | loading of `ITEM.DAT`, `ITEMTYPE.DAT` and the item and decoration art sheets; the title menu; the "find the word" panel |
| 3 | 210C | 23 | 6,961 | level loading (maze, wall sets, door and monster shapes), the level state, `SaveGame`, `LoadGame` |
| 4 | 2116 | 18 | 8,259 | the first-person view: the slot walk and the drawing of walls, doors, items, monsters, projectiles |
| 5 | 211E | 31 | 8,094 | the event panels: temple, joining NPCs, resurrection, bribes and fights, the two endings, the winner password |
| 6 | 212A | 6 | 3,258 | resting (`CampLoop`) and its helpers |

An overlay's *entries* are the functions other units call; the rest are internal. [V] The stub tables name 125 entries for 136 functions. The renderer is called from the main loop on every dirty frame, so overlay 4 is the busiest; level changes bring in overlay 3, the camp overlays 1 and 6, and an event overlay 5. The "find the word" panel (`FindWordPanel`, ovl2:09ED) is the copy protection: it shows a symbol and asks for a word from the manual by line number, and three wrong answers shut the game down. [V] `src/EOV2.C:450-517`.

**How a call crosses into an overlay.** The caller does an ordinary far call to a stub segment (`call 2106:002A`). The stub is a table of 5-byte entries `CD 3F`, the code offset, 0; the `INT 3Fh` lands first in the game's own hook (`Int3FHandler`, 1A72:0006), which checks that `EOB.EXE` can still be opened (the file may be on a floppy that was swapped; if not, it reopens it) and chains to the vector it saved. [V] `src/E1A72.ASM:163-175`. That the saved vector is the overlay manager's is [I]: nothing else answers INT 3Fh. The manager is a unit of the program itself, segment 1E8B (2,329 bytes, 31 functions); its INT 3Fh handler is at 1E8B:04D0. It reads the word after the `INT`, finds the overlay's record, and if the overlay is not in the pool calls `LoadOverlayRecord` (1E8B:0580): `AgeOverlayRecords` drops or moves other overlays to make room, the image is read from the EXE at the FBOV data base plus the record's offset, `1E8B:03FD` adds the pool's segment to every fixup (the fixups hold segment-table indexes, section 2.3), and `1E8B:073B` walks the stack's BP chain so that return addresses into an overlay that moved are rewritten. [V] Comments of `src/E1E8B.ASM`; the effect on stacks is [I] from the code of 073B and 072F. Then the call continues into the overlay at the stub's offset, and its `retf` returns to the original caller. [V]

<!-- FIG arc_overlay_call -->

The **pool** is one far-heap block allocated before `main`: `DGROUP:3088` holds 715 paragraphs, the manager adds one, so 716 paragraphs, 11,456 bytes. [V] The manager doubles that only if its largest record (code plus fixups plus 2 paragraphs) is at least 715; the largest, overlay 0, is 705 plus 2, so the pool is the default. [I] from the record fields (`OverlayTotalParagraphs`, 1E8B:0789; `InitOverlayRecords`, 1E8B:0286). Overlay 0 nearly fills the pool alone, and the four big ones (0, 1, 4 and 5 are each over 8,000 bytes) cannot be there together; switching between the camp and the renderer reloads from the file each time. [I]

**Names that mislead.** Four routines say "overlay" and are not VROOMM: `RunOverlayRequest` (1C00:0055) loads a graphics driver `.OVL` with DOS function 4B03h; `LoadOverlayScreen` (082C:05EC) loads a picture; `InitOverlayCache` and `ShutdownOverlayCache` (1C0E:13E7, :13FD) start and stop the *file table* of section 3.5, not the overlay manager. [V]

<!-- FIG arc_callgraph -->

### 3.4 Memory

The game runs in DOS real mode in conventional memory (640 KB at most). The game's own code in `src/` makes no extended or expanded memory call, and the overlay manager's EMS option is never started. [I]

#### 3.4.1 The image and DGROUP

`EOB.EXE` loads a 201,552-byte image (`min_alloc` 0, `max_alloc` FFFFh, so DOS hands it the whole block). It holds 127,440 bytes of resident code, 8,464 bytes of other data segments (7,568) and the seven overlay stubs (896 with padding), DGROUP (212E, 65,508 bytes, ending at `212E:FFE4`) and a 128-byte stack from the header. Inside DGROUP the data up to `ds:8A56` is initialised; from there the file holds zeros (the game's live records). [V] Segment table of `build/re/eob.json`; the non-zero extent of the image.

<!-- FIG arc_memory -->

<!-- FIG arc_dgroup -->

DGROUP holds everything addressed with a near pointer: the tables and strings, the node records, the sound state, `gMonsters` (30 x 28 = 840 bytes), `gItems` (500 x 14 = 7,000), `gItemTypes` (57 x 16), `gMaze` (9,216), `gChars` (6 x 243 = 1,458) and the 64-byte block of the saved game. Large things are not here: the screens, the wall graphics and the level script are far-heap blocks (below), and a few art sheets are separate small data segments of the image (`PLAYFLD` colour map at 2069, the camp strings at 2003, the item names at 1F28).

#### 3.4.2 The far heap

All other memory comes from the Turbo C++ far heap through one wrapper, `MemAlloc(size, flags)` (19BC:0007). [V] `src/E19BC.C`. It asks `farmalloc` for `size + 1` bytes (`size + 16` with flag 20h, the *aligned* flag), normalises the pointer, and writes a header: the byte before the block holds the flags, or for an aligned block the header is at offset 15 of the paragraph before and the block starts at offset 0 of a paragraph, which is what the page buffers and the unpack work buffer need. Flag 10h (zero-fill) exists and no caller uses it. `MemFree` undoes the header (`farfree`), `MemRealloc` resizes (its only caller, the driver loader, does not test the result), `LargestFreeBlock` walks the heap with `farheapwalk` and returns the biggest free block less 16 bytes.

When `farmalloc` fails, `MemAlloc` first tries `EvictFile(0, 0xFF)`, which would free a cached file (none are ever cached, section 3.5), then calls the hook at `ds:810A`, which `Main` set to `AllocationFailure`: it prints "Allocation failure, returning to DOS!", waits for a key and shuts down. There is no recovery: running out of memory is fatal. [V] A failed `MemAlloc` returns NULL only if the hook returns 0, and it never does. Quirks in the wrapper: `MemFree(NULL)` still decrements the allocation counter `gAllocCount` (`ds:810E`), which nothing reads, and `StartupAlloc` zeroes it after its five allocations. [V]

**What start-up allocates** (VGA, graphics choice 3):

| When | Bytes | Pointer | What it is |
|---|---:|---|---|
| before `main` | 11,456 | (overlay manager) | the overlay pool |
| `RunOverlayRequest` | 64,000, then shrunk to the driver size (11,320 for `MCGA.OVL`) | `ds:818A` | the graphics driver code |
| `StartupGame` | 65,000 / 32,000 / 52,000 | `gPageSegs[2..7]` | three page buffers |
| `StartupGame` | 1,284 and 1,028, and 768 | `gFont8x8`, `gFont6x6`, `gPaletteBuf` | fonts and palette |
| `StartupAlloc` | 7,500 (aligned) | `gLevelInf` (`ds:8DCA`) | the unpacked level script (`LEVELn.INF`) |
| | 6,000 | `gWallBlockMaps` (`ds:F43E`) | the level's `.VMP` or `.EMP` wall block map |
| | 120 | `gSoundMap` (`ds:DCAC`) | game sound number to driver sound number |
| | 4,096 | `gScratchBuf` (`ds:DCFC`) | scratch: packing a level state, the party swap |
| | 1,600 | `gItemRemapTable` (`ds:F534`) | `ITEMRMP`: 100 colour rows of 16 |
| `InitGameTables` | 6 x 1,300 (0x514) | `gChars[i].portrait` | one portrait buffer per character |
| `LoadItemSheets` | 300 | `gRemappedIcon` (`ds:DF48`) | the translated icon shown as the pointer |

Their sum is 202,272 bytes for VGA, so the image plus these blocks need about 404 KB of conventional memory before the sprite cells (icons, item sheets, compass, per-level door, monster and wall shapes, all cut with `gGetSpriteCell` and freed per level) are counted. [V] arithmetic; the total need is [I]. Start-up refuses to go on with less than 65,000 bytes free, which is a weaker test than the real need.

#### 3.4.3 The page buffers

The page table `gPageSegs` (`ds:FC0C`) is eight segment words, four pairs. A pair is a page kind (0, 2, 4, 6): the even slot is the video segment the driver draws into, the odd slot a memory copy; `GetPageBuffer(kind)` returns the odd slot, `GetPageVideo(kind)` the even. [V] `src/E19EC.C`. `StartupGame` fills it for each graphics choice:

<!-- FIG arc_pages -->

In VGA mode kind 0 is the screen (A000, both slots) and kinds 2, 4 and 6 are plain RAM blocks, so drawing to "page 2" draws into memory, and `CopyRegion(..., 2, 0)` blits it to the screen: that is the double buffer. [V] `gDrawPage = 2` is how `BuildMenuButtons` and `LoadOverlayScreen` draw off screen. The three RAM pages double as work areas, and the code treats them by slot number rather than by name:

- **slot 3, `gArtPage`** (65,000 bytes): the unpacked picture on its way to the driver (`LoadPacked(name, 5, 3)` reads the packed file into slot 5 and unpacks it into slot 3), the level-state images (a 2,040-byte slot of `LEVELS.TMP`, or the whole 24,480-byte image), and the buffer for the six blocks of a saved game.
- **slot 5, `gStagingPage`** (32,000 bytes): the packed file just read, then scratch: the maze (4,096 bytes), the wall-set decorations at +0x3E80, the camp menu's saved background at +0x4E20, the spell panel's at +0x61A8.
- **slot 7, `gWallBlocks`** (52,000 bytes): the wall block graphics; also, by `SetUnpackBuffer`, the LZW work buffer of unpack formats 1 and 2, which wants 0xFC00 (64,512) bytes, more than this block holds in VGA mode. [V] No pinned file uses format 2 and format 1 needs only 3C00h, so the shortfall never bites. [I]

**Tandy.** The Tandy path (graphics choice 1) is the odd one: `CheckMemoryChain` and `ReserveMemoryBelowVideo` (1990:0071, 1990:0133) move the environment block and reserve 64 KB under the video segment, probably because Tandy video memory lives in main memory; `TrimMemoryToVideo` gives it back on exit. [H] The "video memory is the top of RAM" reading is a hypothesis of the unit's comments.

### 3.5 Files and the PAK catalogue at run time

Sections 4.1 and 4.4 describe the PAK archives and the files; this section is how the game finds them. The layer is `src/E1C0E.C` (31 functions), and everything that reads data goes through `FileOpen`, `FileRead` and `FileClose`, except the two fonts and the graphics drivers, which use DOS directly. [V]

**The file table.** `gFiles` (`ds:01FF`) has 203 records of 19 bytes (name far pointer, size, memory copy, link, position, flags). The names are in two kinds. Ten are plain files: `INTRO.EXE` and `eobdata1.pak` (link 0), `EOB.EXE`, `LEVELS.TMP` and `eobdata2.pak` (link 1), `eobdata3.pak` (2), `eobdata4.pak` (3), `eobdata5.pak` (4), and `EOBDATA.SAV` and `eobdata6.pak` (5). The other 193 are *aliases*: one record per member of a PAK, with the link field negative (bit 15 set, the low 15 bits the index of the record of the PAK that holds it) and a size equal to the member's stored length in the PAK (192 of 193 records; the `ITEM.DAT` record says 9,601 where the member is 9,482 bytes). [V] Decoded from the image; 193 equals the number of PAK members (section 4.1).

<!-- FIG arc_files -->

A plain file's link is a disk number minus one. The dialog that appears when a file cannot be found says "Could not find `NAME`", "Insert disk `N` into any drive." where `N` is `ResolveFileLink(name) + 1`, and Esc turns it into "A fatal disk error has occurred. Now returning to DOS." and a shutdown. So `EOB.EXE` is on disk 2, the six PAKs on disks 1, 2, 3, 4, 5 and 6, and the saved game on disk 6. [V] `DisplayFileError` (1C0E:0ADD). That these numbers are the floppy release's is [I].

**Opening a file.** `FileOpen(name, mode)` (1C0E:00DB): mode 1 reads, mode 2 deletes and recreates, mode 3 opens for update and creates when missing. For a read it checks the name with `CheckFile` (`LocateOverlayFile`, 1C0E:1310, opens the name on the current drive, and only when `gSearchAllDrives` is set on the other drives from `gLastDrive` down to A; `CheckFile` compares the file's length with the record only for a record with link above 0 and a nonzero size, and no record in the shipped table has both, so no length is ever checked [V]), retrying through the error dialog until the user gives up. For an alias it opens the PAK and `LookupFileEntry` (1C0E:172E) reads the PAK directory from its start (a 4-byte offset and a name per record) to find the member, remembers its offset and length, and seeks to it. The *handle slot* is one of 10 words in `gFileHandles` (`ds:FEEA`): the DOS handle, with 4000h or'd in for an alias; the slot's offset and length live in `gSlotOffsets` and `gSlotLengths`. More than 10 files open at once is fatal ("Too many files open."). `FileRead` reads in chunks of 0FA00h bytes, normalising the far pointer between chunks, and for an alias clamps the length to what is left of the member. [V]

**The memory cache is switched off.** The table has the machinery of a cache: `IsFileInMemory` and `FindLoadedFile` look a record up and report whether it has a copy, `LoadFileSlot` (1C0E:144E) reads a whole file into a `MemAlloc` block, `DropFile` loads it when the record has flag 40h (FLAG_LOAD), `BuildFileCache` (called from `InitOverlayCache`) loads every record with flag 80h (FLAG_KEEP) in order of the low flag bits, `EvictFile` frees the least important copy that is big enough when the heap runs short. But every flags byte in the shipped table is zero, so `BuildFileCache` loads nothing, `DropFile` never loads, `EvictFile` never finds a copy, and every read goes to the disk. [V] Decoded from the image. Whether other releases ship flagged records is not known. [?] Four quirks live in this dormant code: `FindLoadedFile` returns 0 on a miss, which `FileOpen` then treats as record 0 (`INTRO.EXE`); `LoadFileSlot` tests `link > 0` for "own file" where `FileOpen` tests `link < 0` for "alias"; `EvictFile` receives only the low word of a file's size; and `EvictFile` blocks when any handle slot's DOS handle equals its slot number. [V]

**What is loaded when.**

| When | Files | How |
|---|---|---|
| process start (before `main`) | `EOB.EXE` itself: the overlay manager opens it and reads the FBOV records | DOS |
| `StartupGame` | the driver (`MCGA.OVL`, `EGA.OVL`, ...); `Font8.fnt`, `FONT6.FNT` | DOS 4B03h; `LoadFont` |
| | `eobpal.col` (VGA) | `LoadFile`, alias of `eobdata2.pak` |
| `StartupAlloc` | `adlib.dat` or `pcgamesn.dat` (120-byte map, then up to 32C8h of driver data) | `FileOpen`, alias of `eobdata1.pak` |
| title menu (`LoadItemData`) | `ITEMICN`, `ITEM.DAT`, `ITEMTYPE.DAT`, the `INTRO` picture | `LoadPacked` or `FileOpen` |
| after the title menu | `ITEML1`, `ITEMS1`, `THROWN`, `ITEMICN`, `ITEMRMP.VGA` or `.EGA`, `DECORATE`; `PLAYFLD`, `INVENT` | art sheets are `.CPS` for VGA and `.EGA` otherwise (`LoadScreenImage` builds the name) |
| a new party | the `CHARGEN`, `CHARGENA`, `CHARGENB` sheets; portraits | |
| every level entry | `LEVELn.INF`, `LEVELn.MAZ`, the wall set's `.VMP` or `.EMP` (6,000 bytes) and `.PAL` (VGA), the wall set's `.DAT` and picture, the two monster kinds' pictures, `LEVELS.TMP` if visited | `LoadLevel` |
| level 12, endings | `XANATHA.*`; `XDEATH1` to `3`, `TEXT.DAT` | |
| camp: save and load | `LEVELS.TMP`, `EOBDATA.SAV` | section 5.11 |

Because every wall set, monster and level lives in a different PAK, a floppy play of the original swapped disks as the party went down: levels 1 to 3 on disk 3, 4 to 6 on disk 4, 10 to 12 on disk 5 and 7 to 9 on disk 6 (by the PAK that holds `LEVELn.MAZ`, section 4.1). [I]

### 3.6 Interrupts and time

#### 3.6.1 What the game hooks

| Vector | Handler | Installed by | Notes |
|---|---|---|---|
| INT 9 (keyboard) | 1AAD:07C0 | `InstallKeyboard` (`Main`) | takes the key and ends the interrupt itself; section 3.6.3 |
| INT 23h (Ctrl-C) | 1AAD:09F5, a bare `iret` | `InstallKeyboard` | Ctrl-C does nothing |
| INT 24h (DOS critical error) | `HardErrHandler` (1A63:0004) | `InstallHardErr` | stores the code in `gHardErr` and returns to the program instead of DOS's abort, retry or fail prompt |
| INT 1Ch (timer tick) | 1D8F:0055 | `InstallTickHandler` | counters only |
| INT 08h (timer) | AdLib: `_AdlibTimerIsr` (0000:3E66); speaker and Tandy: the driver of `e1948` | the sound driver | reprograms PIT channel 0 and chains to the BIOS |
| INT 3Fh (overlay call) | `Int3FHandler` (1A72:0006) | `Main` | chains to the overlay manager |
| INT 33h callback | 1A72:017A | `InitMouse` | moves and draws the pointer, reports buttons |

`ShutdownGame` restores all of them except INT 24h, which nothing in the game restores (DOS does it at process end) [I]. [V] Section 2.7 covers INT 9 and INT 33h basics.

#### 3.6.2 The clocks

<!-- FIG arc_clocks -->

The tick comes from IRQ0. With no sound driver that is the BIOS 18.2 Hz. The AdLib driver sets PIT channel 0 to divisor 555h (874 Hz) and its INT 08h handler counts passes: every 12th it runs `SoundDriverTick` (72.8 Hz, the music clock), and every 48th it chains to the BIOS handler, which sends the end of interrupt and calls INT 1Ch, so the game's tick stays at 18.2 Hz. The speaker and Tandy driver uses divisor 1000h (291 Hz) and chains every 16th pass. [V] `src/E_TEXT.ASM:365-420`, `src/E1948.ASM:1-9`. The sample player (digitised sound on the AdLib) takes the interrupt completely (divisor 4Bh) and never chains, so the BIOS clock loses time while a sample plays. [V] The 15.9 kHz is [H].

What the INT 1Ch handler does, in full: if bit 0 of `ds:849E` is set add 1 to `ds:848E`; if bit 1 is set add 1 to `gGameTicks` (`ds:8492`); always add 1 to `gTicksSinceEvent` (`ds:8488`), to `gTicks` (`ds:848A`) and to `ds:8496`; count `gFrameWaitTicks` (`ds:849A`) down to zero; chain to the old vector. [V] `src/E1D8F.ASM:76-105`. `ds:848E` and `ds:8496` have no reader in `src/` (`ReadTickCount`, 1D8F:0002, reads the second and has no caller).

The game keeps these clocks: `gTicks` for delays and blinking (`WaitTicks` busy-waits on it; `DelaySecondsThenKey` waits `seconds x 18` ticks; the text field cursor blinks every 2 ticks; the selection frame advances every tick); `gGameTicks` for everything in the game world (the 16 effects, the level timer, the characters' effect timers); `gFrameWaitTicks` as a hold-the-frame timer (`DrawViewFrame` spins until it is zero, and the attack and hit flashes set it to 8 or 1); `gTicksSinceEvent` to time recorded input. Bit 1 of `gTimerEnableFlags` is cleared around the camp menu, `LoadLevel`, wand and spell animations, monster attack and hit flashes (`TryMonsterMelee`, `FlashMonsterHit`). [V] Section 5.16 adds the random clock.

**Screen timing is not an interrupt.** `WaitFrames(n)` (1A16:000B) polls port 3DAh bit 3 (vertical retrace) once per frame, in the sense `gRetraceInverted` says. `CalibrateRetrace` (1A65:000E) decides that sense at start-up: it waits for the BIOS tick counter at 0040:006C to turn over, waits four more ticks (about 220 ms), samples the bit 10,000 times and sets `gRetraceInverted` when it read set more often than clear (some adapters report the bit the other way round). [V] `src/E1A65.C`. Frame waits are used by the menu flashes (`WaitFrames(5)`), `BlinkText` (twice 2 frames, three times) and the message box (120 frames at most).

#### 3.6.3 The key ring and the mouse queue

The keyboard driver (`E1AAD.ASM`) turns scancodes into *key codes* and puts *events* in one ring of 256 bytes at 1AAD:007C (128 words): 1 word for a key, 3 words for a mouse button event (code, x, y). When the ring is full a new event is dropped. [V] `src/E1AAD.ASM:938-963`. The key code is positional: `kScanToKey` (1AAD:0186) maps the scancode to a number (Esc is 6Eh, the digit keys 02h to 0Bh, Q is 11h, W 12h, A 1Fh, S 20h, Z 2Eh, M 34h, F1 to F10 70h to 79h, the keypad keys 5Bh to 6Ah (7, 4, 1 are 5Bh to 5Dh; 8, 5, 2 are 60h to 62h; 9, 6, 3 are 65h to 67h; 0 is 63h; point 68h, minus 69h, plus 6Ah)); the extended (E0) keys are looked up in a second table, and with the driver's option bit 02h clear (the game runs with option value 1) the arrow keys, Home, End, PgUp and PgDn are folded onto the keypad codes they share a position with: Left is keypad 4 (5Ch), Up keypad 8 (60h), Right keypad 6 (66h), Down keypad 2 (62h), Home keypad 7 (5Bh), End keypad 1 (5Dh), PgUp keypad 9 (65h), PgDn keypad 3 (67h). [V] NumLock is ignored. The high byte of an event carries the modifiers: 01h shift (inverted by Caps Lock for letters), 02h Ctrl, 04h Alt, 08h release (set on mouse button releases; the keyboard handler does not queue key releases). The handler also keeps a held-key bitmap (`GetKeyState`) and toggles `gSoundOn` on Alt-S (event 0420h) and `gTunesOn` on Alt-M (0434h) wherever the game is. Modifier keys themselves (Caps Lock, both Shifts, Ctrl, Alt, NumLock) are in the "special" list at 1AAD:0219 and are skipped by the readers. [V]

The readers: `IsKeyPending` (1AAD:0471) says whether anything waits (the xor of the two ring indexes); `ReadKey` (1AAD:038E) peeks without consuming and returns the translated key; `WaitKeyEvent` (1AAD:05D3) blocks and returns the raw event with its modifiers; `WaitGameKey` (1AAD:053A) blocks, skips releases and modifiers and returns the key translated to ASCII for text entry (`TranslateKey`, 1AAD:0223); `FlushKeyBuffer` sets the write index to the read index. The node dispatcher uses the raw form (section 5.10). [V]

The mouse arrives by the same ring. The INT 33h callback (1A72:017A) clamps x to 13Fh (halving it first in the 640-wide mode of section 2.7), publishes `gCursorX`, `gCursorY`, and when the button state changed calls `HandleMouseButtons` (1AAD:0B0E), which turns each changed button into event 41h (left) or 42h (right), the release marked by AH bit 8. The handler also redraws the game's own pointer from inside the interrupt, under the busy flag `gPointerBusy` (`ds:812E`); the main code brackets its own drawing with `LockHidePointer` and `LockShowPointer`, which wait for the flag, so the interrupt never draws over a half-drawn screen. [V] The flag is test-then-increment, not atomic. `GetKeyState(0x41)` and `(0x42)` tell the dispatcher whether a button is held.

A byte, `gRecordMode` (`ds:80EC`), selects a record and replay mode for input: mode 1 writes every event with the tick stamp to the file handle at `ds:80ED`, mode 2 replays it (and the mouse handler stops publishing the real position). Nothing in `src/` sets the byte to 1 or 2. [V] Who turns it on, a debug build or a command line, is not known. [?]

## 4. Data files

### 4.1 PAK archives

`tools/re/pak.py`, `make pak`, `tests/unit/test_pak.py`. A directory of entries (u32 offset, NUL-terminated name) ending at the first file's offset; files back to back; a file's size is the distance to the next offset (the last one to the end of the file). The directory's last 4 bytes are a nameless terminator: the file size (EOBDATA1, 5), zero (EOBDATA3, 4, 6) or `0C 00 10 04` (EOBDATA2). All 6 archives, 193 files, rebuild byte for byte.

| Archive | Files | Contents (by extension) |
|---|---:|---|
| EOBDATA1 | 17 | sound drivers data (`ADLIB.DAT`, `PCSOUND.DAT`, `PCGAMESN.DAT`), intro pictures (`.CMP`) |
| EOBDATA2 | 2 | `DOOR.EGA`, `EOBPAL.COL` |
| EOBDATA3 to 6 | 174 | per level set: wall graphics (`.VCN`, `.VMP`, `.ECN`, `.EMP`), pictures (`.CPS`, `.EGA`), palettes (`.PAL`, `.COL`), `.DAT`, level maps (`LEVELn.MAZ`) and scripts (`LEVELn.INF`) |

<!-- FIG data_pak -->

### 4.2 Compressed files

CPS, CMP, EGA, INF, VCN and ECN files share the header the game's loader reads (`LoadPacked` and `Unpack`, 1B82:0041 and 1B82:00C7, `src/E1B82.C`): u16 size of the rest (some files: the file size), u8 format, u8 0, u32 unpacked size (`src+2`), u16 bytes of palette that follow (`src+6`). `Unpack` switches on the format: 0 stored (copied from `src+8`), 1 and 2 decoded with a work buffer of 3C00h and FC00h bytes (allocated unless the shared buffer `ds:812A`, size `ds:812C`, is big enough), 3, and 4 (LCW, after the palette).

<!-- FIG data_unpack -->

`make data` (`tools/re/wwfmt.py`) decodes every file: 130 files of format 4 unpack to exactly their declared size (LCW as published: back references, literals, absolute copies, fills). `TITLE-V.CMP` with `TITLE-V.COL` is the title picture. One anomaly: `EOBTITLE.CMP` decodes coherently for 31,962 bytes (99 rows of a title picture), then a fill of 13,082 bytes and garbage past 64,000; [H] damaged in this copy. The game's own routine agrees [V]: `tests/unit/test_wwfmt.py` runs Unpack4 (1D9A:0013) in the oracle on all 131 files of format 4 and compares every byte, `EOBTITLE.CMP` included (the game writes the same 64,000 bytes, so the file is odd, not the decoder).

The palette-length field (`src+6`) is 0 in all 137 packed picture, level and wall files [V] (a scan of the six PAKs, 2026-11-05). Of the pictures, all 47 `.CPS` and 42 of 48 `.EGA` are format 4, five `.EGA` are format 1 and one is format 0.

Format 1 [V] (1D9A:018B, `wwfmt.lzw12`; the five EGA wall-set pictures `BRICK1`, `BRICK2`, `BRICK3`, `BLUE`, `DROW`): 12-bit LZW. Codes are big-endian, two in three bytes (1D9A:0299 alternates whole and half bytes). A code below 100h is a literal; from 100h it names a dictionary entry, which is (offset, length) of a string already written to the output, 4 bytes in the 3C00h work buffer, so at most F00h entries, then no more are added and the dictionary is never reset. Each code after the first adds the previous string plus the first byte of the current one; a code naming the entry being made is the previous string plus its own first byte. The game's routine, run in the oracle on the five files, writes the same bytes. No pinned file uses formats 2 or 3.

### 4.3 Mazes and tables

`LEVELn.MAZ` (12 files): u16 width 32, u16 height 32, u16 4 bytes per cell, then the cells (4,102 bytes). `.VMP` and `.EMP`: u16 count, then u16 entries: 2,916 words, a 330-word floor and ceiling background and six wall sets of 431 words (5.12.3). `.PAL` and `.COL`: 768 bytes, VGA colours in the low six bits of each byte (the top two bits are 00 or 11 in the five pinned `.PAL` files). `EOBDATA4.PAK` lists its eight `.COL` files twice.

[V] `.INF` is the level script, a header, a trigger table and bytecode run by `RunCellScript` (5.3.3, 5.5), and the four bytes of a maze cell are its four wall sides in direction order 0 to 3 (5.3.1). (The first reading of both came from ScummVM's Kyra engine, read only [H]; the decompiled code now decides.)

### 4.4 The game's own files

`tools/re/eobfmt.py`, `make data`, `tests/unit/test_eobfmt.py`. Files without the Westwood header. Where the loader was not read yet, the layout comes from the reads the oracle sees during `load-walk`: the read sizes in order, and the caller of the read through the BP chain, resolved to overlay code from the memory at the time of the read (overlays are swapped; resolving at the end of the run gave wrong overlays). The field meanings are [H] until the code that uses them is lifted.

| File | Layout | Evidence |
|---|---|---|
| `FONT6.FNT`, `FONT8.FNT` | u16 bytes after this field; u16 offsets[128] of the glyphs; u8 height, u8 width at 102h; 128 glyphs from 104h, height × ((width + 7) / 8) bytes, one bit per pixel. 6 × 6 and 8 × 8 | [V] 1A47:0005 checks each of these |
| `TEXT.DAT` | u16 offsets of 51 NUL-terminated strings, the table ending at the first: the dwarf and other story texts | invariants over the file |
| `ITEM.DAT` | u16 count (432), records of 14 bytes; u16 count (98), names of 35 bytes | read by ovl2:005E (the call returns to ovl2:0088), which `LoadItemData` (ovl2:09CC, `src/EOV2.C`, L2) calls |
| `ITEMTYPE.DAT` | u16 count (57), records of 16 bytes | [V] `LoadItemTypes` (ovl2:0110, `src/EOV2.C`, L2): count, then a 16-byte `FileRead` per record into `ds:A9F0` |
| wall set `.DAT` (`BRICK`, `BLUE`, `GREEN`, `XANATHA`, `DROW`) | u16 count, decorations of 52 bytes; u16 count, rectangles of 8 bytes (x, y, w, h, in columns and lines: `GetSpriteCell`'s arguments) [V]; a decoration's first 10 bytes index the rectangles or are FFh | read by ovl3:0096 (the call returns there), once for each wall set a level's script loads (three for level 1; `DefineWallType`, `src/EOV3.C:256`; 5.3.5); the index invariant holds in all five |
| `ITEMRMP.VGA`, `ITEMRMP.EGA` | 1,600 bytes read whole (the name is `ITEMRMP.%s` by display); [H] 100 rows of 16 colours | read at ovl2:0484 (in ovl2:0171, called from ovl2:0577, which `StartupAlloc` calls); [V] into the 1,600-byte buffer `ds:F534` that `StartupAlloc` (07A9:000A, `src/E07A9.C`) allocates |
| `ADLIB.DAT`, `PCGAMESN.DAT` | u8 map[120] from the game's sound number to the driver's (values from 96h are ignored, 177D:0070), then the driver's data, at most 32C8h bytes: u16 offsets, the first one the size of their table | [V] 177D:000A loads `adlib.dat` or `pcgamesn.dat` by `ds:B892`; the map goes to the 120-byte buffer `ds:DCAC` that `StartupAlloc` allocates, which calls 177D:000A next |
| `PCSOUND.DAT`, `SOUND.DAT` | the same layout [V]; `EOB.EXE` does not use them, `INTRO.EXE` does (`sound.dat` for AdLib, `pcsound.dat` for the speaker and Tandy, `src/I0892.C:393-415`; `SOUND.DAT` is in `EOBDATA4.PAK`); 5.14.2 | |
| `EOBDATA.SAV` | 6 characters of 243 bytes (u8 index, u8 status whose bit 0 means the slot holds a character, name[11], ...), 64 bytes (the game state from `ds:FBC8`: level, cell, direction, the item on the pointer, saved-level mask, spell flags, level flags, game flags), 500 items of 14 bytes (the `ITEM.DAT` record size), 12 level states of 2,040 bytes, 3 bytes (the Tunes, Sounds and Bar Graphs options), 96 bytes (the six spell-made item types, `gSpellItemTypes`): 33,101 bytes; 5.11 | read by ovl3:160A (the call returns there); decode and encode give the same bytes |
| `LEVELS.TMP` | 12 level states of 2,040 bytes | [V] loading the test save makes the game write the save's 12 level states here, in order (slow test); one is read back per level (the call returns to ovl3:1194). A run of the oracle hides both files unless the session names a save: `LEVELS.TMP` is then made from the save, as the game leaves it after saving (loading reads it before rewriting it and stalls without it) |

Every file of the PAKs and both fonts decode (`make data`). The file names in the data segment around `ds:1EE0` are the PAK catalogue (which archive holds which file), not the loaders' arguments: the loaders build names with `%s.%s` and `ITEMRMP.%s`, or use lower-case copies (`adlib.dat`, ds:5E2C).

## 5. Game systems

The previous sections said what the programs are. This one says what the game does. Each subsection takes one system, gives what it does for the player in a sentence, then its data, its algorithm and its quirks, with the function or address to open. The systems share one loop and one clock (sections 3.1 and 3.6), so most of them are small: the interesting parts are the data formats, the tables and the places where the original does something odd.

<!-- FIG sys_map -->

**The game in numbers** [V]. Each figure is cited where its section proves it.

| What | Number | Section |
|---|---:|---|
| Party slots (four chosen characters, two ready-made ones) | 6 | 5.1 |
| Dungeon levels, in five wall sets | 12 | 5.3 |
| Cells in a level (32 by 32); a cell on disk, in memory | 1,024; 4 and 9 bytes | 5.3 |
| Monster types | 22 | 5.6 |
| Spell records, named spells | 53, 49 | 5.8 |
| Item types in `ITEMTYPE.DAT` | 57 | 4.4 |
| Timed effects (the clock of hands, monsters, spells) | 16 | 5.7 |
| Missile records (thrown items and spell missiles) | 10 | 5.7 |
| Opcodes of the cell script interpreter | 26 | 5.5 |
| Cells the 3D view looks at, drawn from pre-made art | 18 | 5.12 |
| Sounds in the AdLib data, voices of the driver | 150, 10 | 5.14 |
| Length of the intro (instruction clock, no sound) | 147 s | 5.15 |
| The saved game | 33,101 bytes | 5.11 |

**Reading the figures.** A tick is 1/18.2 of a second: the game's clock is the BIOS timer interrupt (section 3.6), and every duration in combat, magic and the camp is counted in ticks. Directions are numbered 0 to 3 (5.3.1). A colour is a number from 0 to 15 unless a section says otherwise. A routine name in backticks is the name in `src/`, with the address in the form of section 2; where a name is known to mislead, section 6.2 says so.

| Section | System | Where the code is |
|---|---|---|
| 5.1 | Party and characters | `E0DE8.C`, `E0A72.C`, `E0D1C.C`, `E1547.C`, `EOV0.C` |
| 5.2 | Items | `E0BD2.C`, `E0ACD.C`, `EOV2.C` |
| 5.3 | Mazes and level state | `E122E.C`, `EOV3.C` |
| 5.4 | Moving through the world | `E0ACD.C`, `E122E.C`, `E1414.C` |
| 5.5 | Cell scripts | `E08E6.C`, `E122E.C` |
| 5.6 | Monsters | `E08E6.C`, `E1414.C`, `EOV3.C` |
| 5.7 | Combat | `E0DE8.C`, `E0D1C.C`, `E1414.C`, `E08E6.C` |
| 5.8 | Spells and effects | `E0EF3.C`, `E1414.C`, `EOV1.C`, `EOV6.C` |
| 5.9 | Camp, rest and food | `EOV1.C`, `EOV6.C`, `E1414.C` |
| 5.10 | Input and the interface | `E16A5.C`, `E1658.C`, `E1AAD.ASM`, `E14EF.C` |
| 5.11 | Saving and loading | `EOV1.C`, `EOV2.C`, `EOV3.C` |
| 5.12 | The 3D view | `EOV4.C`, `E17A6.ASM`, `EOV3.C` |
| 5.13 | Graphics output | `E19EC.C`, `E1A29.C`, `E1DE7.C`, `E1547.C`, `E07C8.C`, `src/drivers/` |
| 5.14 | Sound | `E177D.C`, `E_TEXT.ASM`, `E1948.ASM` |
| 5.15 | The intro | `I0892.C`, `IOV0.C` (`INTRO.EXE`) |
| 5.16 | Random numbers and time | `E1AAA.C`, `E082C.C`, `E1D8F.ASM` |

### 5.1 Party and characters

The party is six character records in the data segment. Everything the game knows about a character, its numbers, its spells, its pack and its timers, is in one 243-byte record, and the rules that read it are a small set of tables and a few dozen short functions. Section 4.4 describes the save file; this section describes the record in memory and the rules around it. Table addresses are `ds:` offsets of `EOB.EXE`; function addresses are resident segment offsets or `ovlN:` for overlays. Overlay 0 (`src/EOV0.C`) holds the character panel and creation, `src/E0A72.C` the rule tables, `src/E0DE8.C` rolls and levels, `src/E0D1C.C` the party tests, `src/E1547.C` the party panel.

#### 5.1.1 Six slots and the character record

`gChars` is an array of six `struct CharRec` at `ds:F580` (`src/EOB.H`), 6 x 243 = 1,458 bytes. [V] The save file writes it as one block, the first 1,458 bytes of `EOBDATA.SAV` (`SaveGame`, ovl3:1368, `src/EOV3.C`); the test save holds four named characters and two empty records (status 0), which I decoded from the file for this section.

<!-- FIG chr_record -->

The fields, in offset order [V: each is read or written by the function named; the layout compiles to these offsets because the units that use it match]:

| Offset | Size | Field | What it holds |
|---|---:|---|---|
| 00 | 1 | `index` | the record's own slot number; `InitGameTables` (ovl2:0496) sets it for all six at start-up |
| 01 | 1 | `status` | bit 0 the slot holds a character, bit 1 poisoned, bit 2 paralysed |
| 02 | 11 | `name` | up to 10 characters and a NUL (the name editor allows 10, `EditTextField(.., 0xA, ..)`, ovl0:23EC) |
| 0D to 1A | 14 | seven (current, base) pairs | strength, strength percentile, intelligence, wisdom, dexterity, constitution, charisma |
| 1B, 1C | 1, 1 | `hp`, `maxHp` | signed bytes; -10 is dead (section 5.1.8) |
| 1D | 1 | `armorClass` | signed; lower is better (section 5.1.6) |
| 1E | 1 | `m1E` | bit n is set while hand n (item slot 0 or 1) is busy after an action |
| 1F, 20, 21 | 1 each | `race`, `classIndex`, `alignment` | indexes into the name tables (section 5.1.3) |
| 22 | 1 | `portraitIndex` | 0 or more: a face on the `CHARGENA` sheet; negative: the face `-(n+1)` on the `OUTTAKE` sheet (`src/EOV3.C:891-908`) |
| 23 | 1 | `food` | 0 to 100 (section 5.9.4) |
| 24 | 3 | `levels[3]` | the level of each class slot |
| 27 | 12 | `experience[3]` | 32-bit experience of each class slot |
| 33 | 4 | `portrait` | far pointer to a 1,300-byte face buffer (`MemAlloc(0x514)`, ovl2:0496); rebuilt at load from `portraitIndex`, so the saved value is not used |
| 37 | 30 | `mageSpells` | five levels of six slots (section 5.9.2) |
| 55 | 30 | `clericSpells` | the same for cleric and paladin spells |
| 73 | 4 | `spellBook` | bit i set: the mage's book holds spell i + 1 |
| 77 | 54 | `items[27]` | 27 item ids (section 5.2.3) |
| AD | 40 | `timers[10]` | ten 32-bit deadlines in game ticks, 0 when free |
| D5 | 10 | `mD5` | the type byte of each timer |
| DF | 1 | `mDF` | damage the Armor spell can still absorb |
| E0 | 1 | `mE0` | hits the Stoneskin spell can still ignore |
| E2 | 1 | `mE2` | temporary hit points given by Aid |
| E3 | 4 | `effectFlags` | spell effects on the member (table below) |
| E7 | 1 | `lastDamage` | the number shown beside the portrait after a hit |
| E8 | 11 | `lastAction` | the result label of the last use of hand slots 0 and 1 (HIT number, MISS, HACK, CAN'T REACH, NO AMMO) |

Each field has two copies where the game changes it temporarily. The seven ability pairs hold the current value and the base value; `SetBaseStats` (ovl0:284A) copies current to base when the party is made, and the giant strength effect sets strength to 22 and restores it from the base byte when it ends (`RunCharEffects`, 1414:07CA, case 7; `ApplyItemEffect`, 0BD2:0D2D). [V]

`mDF`, `mE0` and `mE2` are the small counters of three mage and cleric spells: `CastArmor` (0EF3:0F0B) stores caster level + 8 in `mDF`; every hit subtracts from it and when it reaches 0 `EndArmor` recomputes the armour class (`DamageCharacter`, 0DE8:09B9); `CastStoneskin` (0EF3:1865) stores 1d4 + half the caster level in `mE0`, and each monster hit that lands on the member costs one count and does no damage (`ResolveMonsterMelee`, 08E6:10F3, through `TickBandageCountdown`, 0EF3:18A2); `CastAid` (0EF3:1BA3) adds 1d8 to hp and the same amount to `mE2`, which is taken back when the spell ends. [V]

`effectFlags` bits, each set by the spell function named [V]:

| Bit | Set by | What it does |
|---|---|---|
| 02h | `CastShield` | armour class cut to 4 (`ComputeArmourClass`); a magic missile aimed at the member does nothing (`DamageTargetCell`, 0EF3:0A3F) |
| 04h | `CastInvisibility` | a monster picks the member only on a 1d20 roll below 5; ends when the member attacks with an item (`UseItem`) or casts an attack spell (`CastSpell`, spells whose second flag byte has bit 0 set) |
| 08h | `CastHaste` | halves the hand delay (section 5.1.8) |
| 40h | `CastAid` | +1 to the member's attack roll |
| 80h | `CastSlowPoison` | the poison timer deals no damage |
| 100h | `CastMagicalVestment` | armour class cut to 5, or to 5 + (level - 5) / 3 for a cleric above level 5 (a higher, worse cap as the level rises) |
| 200h | `CastProtFromEvil` | a monster's roll against the member is 2 lower |

The field is a 32-bit `long` so that the compiler writes tests such as `and ax,4; or ax,0`, the same form as the long bit test whitepaper 2.1.1 settled.

#### 5.1.2 Party slots, ranks and the other nine characters

Slots 0 to 3 are the characters the player makes. Slots 4 and 5 stay empty until a ready-made character joins. [V] The party panel draws the six slots as three rows of two (`RedrawMember`, 1547:0991: x from `slot & 1`, y from `slot >> 1`), and a monster that attacks the party scans them in a fixed order (`gAttackTargetOrder`, `ds:325B`: eight orders of six slots, two for each side the monster can come from). The two orders for a monster in front begin with slots 0 and 1 and end with 4 and 5 (1 0 3 2 5 4 and 0 1 2 3 4 5); for a monster behind they begin with 4 and 5 (4 5 2 3 0 1 and 5 4 3 2 1 0); from a side they run down one column first (1 3 5 0 2 4, or 0 2 4 1 3 5). [V for the table, I for the name "rank", which is mine.] Only slots 0 and 1 can use a weapon in melee: `ApplyItemAttack` (0DE8:0359) returns -3 for any `who` above 1, which `DrawActionLabel` shows as CAN'T REACH. [V]

<!-- FIG chr_party -->

In the world a party occupies one cell, split into four quarters, and a slot maps to a quarter through `gViewToCellPos` (`ds:1A1B`, four per facing); slots 4 and 5 use the quarters of slots 2 and 3 (`UseCarriedItem`, 0DE8:0537, subtracts 2 from the slot above 3). [V]

The ready-made characters are nine full records at `gCharPool` (`ds:7298`, `src/EOV5.C`): Anya, Beohram, Kirath, Ileria, Tyrra, Tod Uphill, Taghor, Dohrum and Keirgar, from level 3 to level 11 (the figure lists race, class and hit points; decoded from the image). [V] `AddPartyMember` (ovl5:070E) copies a pool record into the first slot whose status bit 0 is clear, keeps the slot's own face buffer, recomputes armour class, copies the items of slots 0 to 24 into fresh item records (`CopyItemToFreeSlot`; the loop stops at 0x19, so the two ring slots are not copied, and none of the nine pool records holds a ring), and cuts the face from row `0xA0` of the loaded art page (a saved game cuts the same cell from the `OUTTAKE` sheet, `src/EOV3.C:903-908`). When all six slots are in use `ChooseChar` (ovl5:067B) asks whom to remove first. The panels that offer a character (`AddCharPanel`, ovl5:0828) first delete the matching "bones" item, the item of type 21h whose value is the pool index plus 1; for example record 64 is "Halfling Bones" with value 6, and pool character 5 is Tod Uphill, a halfling. [V code, I for the story link] A dead party member can be restored by the resurrect panel (`ResurrectPicker`, ovl5:0DBB), which sets `hp = maxHp`.

`RemovePartyMember` (0D1C:035A) clears the status byte, then puts every item the member carries on the floor of the party's cell at a random front position, including the whole quiver list. `DropCharacter` (ovl1:0000) is the camp menu's version: it is refused with fewer than five members present ("You cannot have less than four characters."), and for a member in slots 0 to 3 it swaps slot 5, or slot 4 if 5 is empty, into the gap. [V]

The party panel is one of three views of the same records, chosen by `gPanelMode` (`src/E1547.C`): 0 the four-icon party panel, 1 the inventory screen of one member, 2 the CHARACTER INFO page of that member. `RedrawMember` switches on it; `DrawMemberPortrait` draws the dead face at `hp == -10` and a "wounded" overlay at `hp < 1`; `DrawMemberHp` draws a ratio bar of `hp + 10` against `maxHp + 10`; `DrawMemberFrame` outlines the portrait in colour 3 or 6 while Shield, Magical Vestment, Protection from Evil, Armor or Stoneskin is on and in colour 5 while Haste, Aid, Slow Poison, Bless or Prayer is; an invisible member's portrait is blanked. [V]

<!-- FIG chr_status -->

#### 5.1.3 Races, classes and alignments

The game has six races, each in a male and a female entry: `gRaceNames` has 12 entries and the code halves the index (`race >> 1`) to reach the race tables; the low bit is the sex, used for "his" and "her" (`ReportLostItem`, 0BD2:000E) and for the face list. [V] There are 15 classes (`gClassNames`): the six single classes fighter, ranger, paladin, mage, cleric, thief, and nine multi-classes of two or three. Nine alignments exist (`gAlignmentNames`).

The central idea is the **class slot**. A character has up to three class slots, and everything per class (the level, the experience, the experience table, the hit die, the saving-throw table and the spell list) is selected by the slot. `gClassSlotBaseClass` (`ds:35D4`, 15 rows of 4 bytes) names the base class of each slot of each class, or -1 for no slot. `GetClassSlotByte` (0A72:0509) reads it. The base classes are numbered 0 fighter, 1 mage, 2 cleric, 3 thief, 4 paladin, 5 ranger. `FindClassSlot(base, class)` (0A72:007B) returns the slot that holds a base class, or -1; the code asks it "does this character have a cleric slot" (`FindClassSlot(2, class)`) and uses the answer to index `levels[]`. [V]

<!-- FIG chr_classes -->

The other per-class tables in the figure are `gClassSlotCount` (`ds:115F`, how many classes share the experience), `gClassStartLevels` (`ds:5EF9`), `gHitDieSlot0/1/2` (`ds:174B`, `175A`, `1769`), `gClassCombatGroup` (`ds:116E`) and `gClassBaseBits` (`ds:1231`, a four-bit mask: bit 0 fighter, bit 1 mage, bit 2 cleric, bit 3 thief; paladin has bits 0 and 2, ranger only bit 0). The mask decides who may pick the mage or cleric menus at camp and which items a class may use (section 5.2.3). [V] Three details: a single ranger has no spell bit, so a ranger casts only as a RANGER/CLERIC; a paladin has the cleric bit but its spell menu is closed until level 9 (section 5.9.1); and THIEF/MAGE has its two hit dice in the opposite order from its slots (the thief slot rolls d4, the mage slot d6). [V for the data, ? for the cause]

Which classes a race may take is `gRaceClassMasks` (`ds:6081`, one 32-bit mask per race, bit n = class n), and which alignments a class may take is `gClassAlignmentMasks` (`ds:6099`). [V]

| Race | Classes | Minimum (STR INT WIS DEX CON CHA) | Maximum | Score change |
|---|---|---|---|---|
| Human | the six single classes | 3 3 3 3 3 3 | 18/00 18 18 18 18 18 | none |
| Elf | fighter, ranger, mage, cleric, thief, F/T, F/M, F/M/T, T/M | 3 8 3 7 6 8 | 18/00 18 18 19 17 18 | DEX +1, CON -1 |
| Half-elf | all but paladin and C/T (13 of 15) | 3 4 3 6 6 3 | 18/00 18 18 18 18 18 | none |
| Dwarf | fighter, cleric, thief, F/C, F/T | 8 3 3 3 12 2 | 18/00 18 18 17 19 16 | CON +1, CHA -1 |
| Gnome | fighter, cleric, thief, F/C, F/T, C/T | 6 7 2 3 8 3 | 18/00 19 17 18 18 18 | INT +1, WIS -1 |
| Halfling | fighter, cleric, thief, F/T | 7 6 3 8 10 6 | 17 18 17 19 18 18 | STR -1, DEX +1 |

The six masks in the image are 3Fh, 7BBh, 77FBh, F1h, 8F1h and B1h; the table lists the classes they select. The class minimums (`gClassMinAbilities`, `ds:5FC5`) are 9 in the prime score of each slot (fighter STR 9, mage INT 9, cleric WIS 9, thief DEX 9), ranger STR 13, WIS 14, DEX 13, CON 14, paladin STR 12, WIS 13, CON 9, CHA 17. Alignment: paladin lawful good only; ranger and R/C the three good alignments; thief, F/T, F/M/T, T/M, C/T every alignment but lawful good; the rest all nine. [V]

Two coupling rules act across the party (`PickClass`, ovl0:207A, and `PickAlignment`, ovl0:2238): once any named member is evil (alignment 6 or more) the paladin class is removed from the list, and once any named member is a paladin the three evil alignments are removed. [V]

#### 5.1.4 Making a character

`RunCharacterPanel` (ovl0:28F9) shows four name slots; choosing an empty slot runs `CreateCharacter` (ovl0:23EC).

<!-- FIG chr_chargen -->

The sequence is race, class, alignment, then one roll of the stats, then a review screen where the player may roll again as often as wanted, edit the scores, pick a face or accept, then the name. The class and alignment lists have a back button that returns one step. [V] Faces come from the 44 cells of `CHARGENA` (`PickPortrait`, ovl0:0A5E; `FindFreePortrait`, ovl0:0D5B): a female face must be cell 26 to 43, a male face cell 0 to 28, and a face already used by a named member is skipped. [V] The name is typed with `EditTextField` (082C:000B) until it is not empty. `ViewCharacter` (ovl0:0EE5) lets the player look at a named character, erase the name or press PLAY. When PLAY is chosen and all four slots are named, `GiveStartingEquipment`, `GiveStartingSpellBooks` and `SetBaseStats` finish the party.

<!-- FIG chr_abilities -->

`RollCharacterStats` (ovl0:1BF2) makes the numbers. Each of the six scores is `RollAbilityScore` (four d6, lowest dropped) plus the race adjustment, raised to the larger of the class and race minimum, and, for strength only, an exact 18 gets a d100 percentile in the high byte; it is then lowered to the race maximum, and strength is held at 18 for every class whose `gClassCombatGroup` is not 0 (mage, cleric, thief and their multi-classes: only the fighter group can reach 18/xx). [V] An 18/100 prints as "18/00" (`FormatStrengthPercent`, 0A72:0468). Armour class starts as `gDexArmourAdjust[DEX] + 10`, hit points as `RollHitPoints` (section 5.1.7), the three levels as `gClassStartLevels[class]` and each of the three experience totals as `5000 / number of classes` (even for slots the class does not have). [V] That last rule is why a new party starts at levels 1 to 4: 5000 points are enough for level 3 in most single classes (the thief's table needs only 5000 for level 4), and a three-class character gets 1666 per class. The rolled stats are only a start: `EditStatField` (ovl0:135D) lets the player change each score and the hit points inside the same limits, so every score can be set to its race maximum. [V] Its seventh pair of limits is the hit point range; the limit words sit right after the six ability limits (`gStatMin[6]` is `ds:DD10`, `gStatMax[6]` is `ds:DCA4`), which is why those two globals carry other names in `src/EOV0.C`.

`GiveStartingEquipment` (ovl0:26C6) copies prototype items into each character (the prototype records 1 to 10 of `ITEM.DAT`, section 5.2.1). `gClassStartItems` (`ds:6101`) lists three or four of them per class, for example short sword, boots and leather armour for a fighter; dagger, spell book and robe for a mage; mace, holy symbol and leather armour for a cleric; short sword, lock picks and leather armour for a thief. Every member also gets iron rations in slot 3, and character 0 gets the "Commission and Letter of Marque" (item 35). Mage classes then get `spellBook = 0x26C`, five spells (bits 2, 3, 5, 6 and 9, which read as spells 3, 4, 6, 7 and 10 of the mage list). [V for the table, I for the spell names] Status is set to 1 and food to 100 here.

#### 5.1.5 Ability bonuses

A few small tables turn a score into a bonus. Strength gives a to-hit and a damage bonus through `gStrHitBonus` (`ds:3439`) and `gStrDamageBonus` (`ds:345C`), 26 bytes each, indexed by the score; with a percentile, the last of five thresholds {1, 51, 76, 91, 100} the percentile reaches replaces the value with `gStrPctHitBonus` {1, 2, 2, 2, 3} or `gStrPctDamageBonus` {3, 3, 4, 5, 6}. [V] Score 17 gives +1 to hit, score 19 +3, score 25 +7; damage is +1 at 16, +2 at 18, +7 at 19, +14 at 25. Dexterity gives a missile to-hit bonus (`gDexMissileBonus`, `ds:3474`, words) and an armour class adjustment (`gDexArmourAdjust`, `ds:349E`): DEX 15 or more lowers armour class by 1 to 5, DEX 6 or less raises it. Constitution gives a hit point bonus per die (`gConHitPointBonus`, `ds:1778`) and, for dwarves, gnomes and halflings, a saving throw bonus (`gConSaveBonus`, `ds:54F6`). The strength, armour class and hit point tables agree with the AD&D first edition tables as I remember them [H]. The dexterity missile table does not: it has only six zeros between DEX 6 and DEX 11, where first edition has ten between 6 and 15, so DEX 12 already gives +1 and DEX 15 gives +3 (read from the image at `ds:3474`). Whether the original data had this slip or the patch changed it is not known. [?]

#### 5.1.6 Armour class, to-hit and saving throws

Armour class counts down: an unarmoured character with average dexterity is 10, plate mail is 3. `ComputeArmourClass` (0DE8:0D5E) builds it as shown, from the dexterity adjustment plus 10, the armour value of the item types in the body armour, shield and bracer slots minus each item's enchantment, and, when the body armour carries no enchantment, the better ring of protection; three spells then cap it.

<!-- FIG chr_combat -->

A helmet gives nothing: the walk covers slots 17, 0, 1 and 18, while the table `gArmourSlots` (`ds:553E`) holds a fifth entry, 19, the helmet slot, after those four. [V for the loop, I for the fifth entry being part of the table] The item types make this visible: the two helmets have an armour value of -1 and no other use.

Both attack rolls are a d20 against a number. A character needs `GetHitTarget - bonus`, where `GetHitTarget` (0A72:0522) is `20 - (levels[0] - 1) / step * gain - the monster's armour byte`; fighters and their kin gain 2 points every 3 levels, mages 1 every 2, clerics 1 every level and thieves 1 every 3 (`gHitGainPerStep` {2, 1, 1, 1}, `gHitLevelsPerStep` {3, 2, 1, 3}, indexed by `gClassCombatGroup`). Only the first class slot counts. [V] The pattern is not the first edition one, in which fighters improve fastest [H]; the four rows look rotated [?]. The bonus is the item's enchantment (only for item types 1 to 3, the swords and the orb) plus the strength bonus for melee or the dexterity missile bonus for thrown and shot items. A monster needs `type +1 - armorClass`, a natural 20 always hits, and Protection from Evil and Prayer each lower its roll (`RollMonsterToHit`, 08E6:106B). [V]

A saving throw (`GetSavingThrowTarget`, 0A72:0006) is one byte looked up in the matrix of the character's first class slot, with its column from the level and its row from the kind of danger.

<!-- FIG chr_save -->

`RollMemberSave` (0D1C:059B) rolls a d20 against the target; poison is tested with kind 0, paralysis kind 2 and spell damage kind 4 (`ApplyStatusEffect`, 0DE8:101A, `HalveSaveDamageA`, 0DE8:0F98). [V] The dwarf, gnome and halfling bonus is the constitution value from `gConSaveBonus`, subtracted from the target; the code tests `race * 2` against 3, 4 and 5 where `race >> 1` was meant (`shl ax,1` at 0D1C:05FB), so the bonus never applies to a dwarf, gnome or halfling and does apply, for kinds 4 and 1, to a male elf (race 2). [V, an original bug or a patch edit] Two more oddities: `HalveSaveDamageA` compares a d20 with `RollMemberSave`'s 0 or 1 result, so it always halves the damage (0DE8:0F98), and the paladin's saves use the fighter table by the folding of base classes 4 and 5 onto 0. [V]

#### 5.1.7 Experience, levels and hit points

`AddPartyExperience` (0DE8:0C2B) is called with a monster's experience value (word `m13`, `MonsterType` +13h, by 08E6:0EC3 when it dies) or by a level script (`AddExperienceOp`, 122E:1B5A). It divides the amount by the number of members that pass `MemberIs(n, 3)` (present and above -10 hit points) and hands each share to `AddExperience` (0DE8:0B45), which divides again by `gClassSlotCount[class]` and tests every class slot.

<!-- FIG chr_levelup -->

The test is `experience[slot] > table[levels[slot]]`, strictly greater, and only one level is gained per award, however large. The first table entry is level 1 at 0, so at level 2 a fighter needs more than 4,000 points, and the table entry FFFFFFFFh ends advancement. Maximum levels follow from the table lengths: 11 for fighter, ranger, paladin, mage and thief, 10 for cleric. [V] Because creation fixes the level without the test, a mage at creation sits at level 3 on exactly 5,000 points, which an award would not have made level 3. [I]

`LevelUpSlot` (0DE8:0C82) adds one hit die for the slot that advanced: `RollHitPoints(class, CON, 1, 0, 0)` for slot 0, and the same with the 1 in the second or third place for the other slots. `RollHitPoints` (0A72:01AE) rolls `count` dice of the slot's die, adds the constitution bonus once per die (capped at +2 unless the slot-0 die is d10), and divides by 3 for a three-class and by 2 for a two-class character; `MaxHitPoints` and `MinHitPoints` (0A72:02B3, 03A4) are the same formula with every die at its maximum or at 1, used by the hit point editor of creation. [V] A fighter/mage gains (d10 + bonus) / 2 hit points for a fighter level and (d4 + bonus) / 2 for a mage level. `hp` and `maxHp` are signed bytes and nothing caps `maxHp` below 128. [I]

#### 5.1.8 Damage, death and conditions

`DamageCharacter` (0DE8:09B9) subtracts the damage from `hp`, remembers it in `lastDamage`, and at -10 or less sets `hp = -10`, clears invisibility and keeps only status bit 0, plays sound 16h, and calls `CheckPartyWiped` (0DE8:093B). The party is wiped when no member passes `MemberIs(n, 5)` (present and above 0 hit points); the game then prints "All of your party has been defeated." and ends the program through `ShutdownGame`. [V] Healing (`HealCharacter`, 0DE8:0ADB) works only on a member above -10 and never above `maxHp`; it does not raise the dead.

Poison and paralysis are a status bit plus a timer. `ApplyStatusEffect` returns at once if the bit is already set or the saving throw succeeds; otherwise it sets the bit and arms a timer: poison `1 * 222h` ticks of type 8, paralysis `5 * 222h` ticks of type 9 (222h = 546 ticks, 30 seconds at the 18.2 ticks a second of INT 1Ch, `src/E1D8F.ASM`). [V] When the poison timer fires the member takes 5 damage unless Slow Poison is on, and the timer re-arms every 30 seconds with no natural end: poison lasts until a cure. Paralysis clears itself. [V] Monsters apply them (`m0E` bits 10h and 20h of the monster type, `ResolveMonsterMelee`).

The ten `timers` of a member are deadlines in `gGameTicks`, and `mD5[i]` says what to do when timer i passes. `RunCharEffects` (1414:07CA) is effect 6 + slot of the global effect table (`ds:5BA8`, section 5.8); it handles the types below, finds the smallest remaining deadline and sets its own period to it. [V]

| Type | When the timer ends |
|---|---|
| 0, 1 | hand 0 or 1 becomes ready (toggle `m1E` bit) |
| 2, 3 | clear the hand's result label, then arm type 4 or 5 after 36 ticks (18 under Haste) |
| 4, 5 | hand 0 or 1 becomes ready |
| 6 | clear `lastDamage` |
| 7 | giant strength ends |
| 8 | poison tick |
| 9 | paralysis ends |
| negative | spell -n ends (`ExpireSpellEffect`) |

Using a weapon in hand slot n toggles `m1E`, arms type n + 2 after 18 ticks and so keeps the hand busy for 18 + 36 = 54 ticks, about 3 seconds, or 36 ticks under Haste; a miss with no target ("can't reach", "no ammo") frees the hand after 18 ticks (`UseItem`, 0DE8:0763). [V] The save file stores the timers as remaining ticks: `AgeCharTimers` (ovl3:123D) subtracts the clock before writing and `AdvanceCharTimers` adds it back at load. [V]

### 5.2 Items

An item in Eye of the Beholder is a 14-byte record that says where it is and what kind it is, and a 16-byte type record that says what that kind does. Items are never copied around as data: a character's slot, the pointer and a cell's list all hold item ids, and every move changes where an id is.

#### 5.2.1 The item records

`ITEM.DAT` (9,482 bytes, in `EOBDATA6.PAK`) holds 432 item records of 14 bytes and 98 names of 35 bytes; `LoadItems` (ovl2:005E) reads the records into `gItems` (`ds:8DD0`, 500 slots) and the names into the far buffer at `1F28:0000`. Before it reads, it sets the key word of all 500 records to -1, so the 68 slots after the file's 432 start free. `ITEMTYPE.DAT` (914 bytes) holds 57 type records of 16 bytes (`LoadItemTypes`, ovl2:0110) into `gItemTypes` (`ds:A9F0`). [V]

<!-- FIG chr_itemrec -->

The record fields [V: the functions named read them, and `ITEM.DAT` decodes to them]:

| Field | Use |
|---|---|
| `nameIndex`, `identifiedNameIndex` | indexes into the 98 names; the second is the name shown once identified, or 1 (the empty name) for none |
| `flags` | bit 80h magical, 40h identified, 3Fh charges on wands, other bits for spell-made items (figure) |
| `icon` | the icon number in the item sheets |
| `type` | index into `gItemTypes` |
| `cellPos` | where in its cell the item lies: 0 to 3 a quarter, 8 a wall niche; a thrown item is tagged `kind | 4` |
| `key` | the list owner: the map cell number for a floor item, -1 free, -2 a free spell item, or an item in a character's quiver, 0 not in a map list |
| `prevLink`, `nextLink` | the neighbours in the item's circular list |
| `level` | the dungeon level of a floor item |
| `value` | the item's number: its enchantment (weapons, armour, rings of protection), the spell number (scrolls), the food value, the potion, wand or ring effect |

Of the 432 records, 350 lie on the maps (key greater than 0, level 1 to 12), 62 have key 0 (the starting-kit prototypes, the five records the two dwarf pool characters carry, and others in no list) and 20 are free. `RebuildItemList` (ovl3:0951) rebuilds the lists of a level when it loads: it walks all 500 records and inserts every record whose level is the current one and whose key is positive. [V] Records 1 to 10 are the prototypes of the starting kit (leather armour, robe, staff, dagger, short sword, lock picks, spell book, holy symbol, boots, iron rations); `CopyItemToFreeSlot` (0BD2:089E) clones a live record into the first record with key -1 from id 1 on. Records 11 to 16 are named "NULL" and look reserved for spell-made items (the touch spells and Flame Blade): `CreateItemRecord` (0EF3:0418) scans ids 11 to 16 for key -2, `IsDiscardedItem` (0EF3:121B) treats ids 11 to 17 as spell-made, and `CreateItemType` (0EF3:035A) takes one of the types 51 to 56, whose armour byte is E2h while free. The six types are saved in the last 96 bytes of `EOBDATA.SAV`. [V] The pinned `ITEM.DAT` gives records 11 to 16 key 0, not -2, and the only stores of -2 in `src/` are `FreeItemRecord` (0EF3:03D8), which runs on an item that already exists, and the quiver code, which stamps the owner key 0xFFFE on every item put into a character's quiver (`InsertListItem`, `src/E0BD2.C:620`, `src/E0ACD.C:762`, `:833`, `src/E122E.C:441`); a spell item has slot mask 0 and cannot enter the quiver, so neither frees one of records 11 to 16. So the scan finds nothing and the code falls through to id 17, the unplaced "Jeweled Key" prototype, which a spell-made item then overwrites; two spell items at once would share it. [V for the bytes and code, I for the consequence] Whether the original data had -2 in those records, and the patch changed it, is not known. [?]

<!-- FIG chr_itemlist -->

#### 5.2.2 Item types

A type record gives the slot mask, an armour change, a class mask, a hand count, two damage dice, and the use. The weapon types are [V from the file]:

| Type | Example | Dice vs ordinary / large | Use kind |
|---:|---|---|---|
| 0 | Axe | 1d8 / 1d10 | melee |
| 1 | Long sword | 1d8 / 1d12 | melee |
| 2 | Short sword | 1d6 / 1d8 | melee |
| 5 | Dagger | 1d4 / 1d3 | thrown |
| 7 | Bow | 1d6 | launcher |
| 9 | Spear | 1d6 / 1d8 | thrown |
| 10 | Halberd | 1d10 / 2d6 | melee, two-handed |
| 11 | Mace | 1d6+1 / 1d6+1 | melee |
| 12 | Flail | 1d6+1 / 2d4+1 | melee |
| 13 | Staff | 1d6 | melee, two-handed |
| 14 | Sling | 1d4 | launcher |
| 15 | Dart | 1d3 / 1d2 | thrown |
| 16 | Arrow | 1d1 | ammunition |

"Large" is a monster type with bit 0 of its `m0E` byte set (`RollAttackDamage`, 0DE8:011A). A hit does the weapon's dice, plus the item's enchantment, plus the strength damage bonus for melee, and is halved when the monster type has bit 2 of its `m0E` byte set (the skeleton kinds) and the weapon type's class mask lacks the cleric bit (`IsWeaponDamageHalved`, 0D1C:0BEB; section 5.7.3); the item's enchantment is not consulted. [V] Armour types carry the signed armour change: leather -2, scale -4, chain -5, banded -6, plate -7, shield -1, helmet -1.

The low seven bits of the last word, the "kind", select what happens when the item is used. They are read by `UseItemSlot` (0BD2:1001):

| Kind | Items | Use |
|---:|---|---|
| 0 | armour, robe, boots, bracers, ring of protection | nothing; worn |
| 1 | axe, swords, mace, flail, staff | melee attack (`ApplyItemAttack`) |
| 2 | dagger, spear, dart, arrow, rock | the item is thrown (`UseCarriedItem`) |
| 3 | bow, sling | shoot: takes ammunition from the pack (`UseItemByKind`) |
| 5, 6 | spell book, holy symbol | open the spell panel for a mage or a cleric |
| 7 | rations | eat, but only with the mouse disabled (`UseItemSlot` tests `gMouseDisabled` before `EatFood`; section 5.9.4) |
| 9, 10 | mage scroll, cleric scroll | cast the scroll's spell, the value is the spell number |
| 11 | letters and quest scrolls | open an event panel in overlay 5 |
| 14 | potions | drink |
| 16 | special rings | nothing; worn |
| 18 | wands and staffs | `UseWandItem` |
| 21 | medallions | no case in `UseItemSlot`: nothing happens |
| 4, 8, 12, 13, 15 | orbs, necklaces, eggs, bones, stones, keys, gems | "This item is not used in this way." |

Bit 7 of the kind (80h) marks an item that a monster with the stealing attack can take: `ReportLostItem` (0BD2:000E) picks a random slot and takes the first item with that bit set. [V]

#### 5.2.3 Slots and what fits

A character has 27 item slots, drawn as the inventory screen of mode 1. Their screen positions come from `gInvSlotX` and `gInvSlotY` (`ds:17CE`, `ds:1804`).

<!-- FIG chr_slots -->

An item may enter a slot when `gSlotItemMask[slot] & type.slotMask` is not zero (`CanPlaceItemInSlot`, 0BD2:0AF2; `gSlotItemMask`, `ds:1C8D`). For body armour the class mask is tested first: `ItemClassAllowed` (0BD2:11FD) is `type.classes & gClassBaseBits[class]`, and the message "can't wear that type of armor" appears when it is 0. An item in a slot that has flag 20h cannot be taken out (`SwapSelectedItem`, 0BD2:0C8E). The class mask is also checked in use: `UseItemSlot` prints "can not use this item." when it is 0, and `ComputeArmourClass` ignores armour the class cannot use. `CanUseHand` (0D1C:07DB) adds two hand rules, both read from the byte at +06 of the type record: a type with +06 = 2 (bow, sling, halberd, staff) in hand 0 blocks hand 1, and a weapon with +06 not 0 (axe, long sword, spear, mace, flail and the two-handers) cannot be used from hand 1, so only short sword, dagger, dart, arrow and rock work in the second hand. [V] The class bytes of the item types give the usual restrictions [V from the file]: plate, chain, banded and scale mail (05h) need a fighter or a cleric bit, so a pure mage or thief cannot wear them; daggers and darts (0Bh) are for classes with the fighter, mage or thief bit but not clerics; maces (0Dh) are not for mages; lock picks (08h) are for the thief bit only.

Slot 16 is not an item but the head of a list, the quiver; `DrawItemRow` shows the number of items in it (`ScanListItem`, 0BD2:0385). Removing the member drains it.

#### 5.2.4 The pointer, the floor and the lists

The pointer item is `gMouseItem` (`ds:FBCE`). `SetMouseItem` (07A9:010C) stores the id, prints the item's name and " taken." when the text window shows messages, picks the icon, and when Detect Magic is on and the item has flag 80h draws the icon through the colour remap table. With the mouse disabled, `GetCurrentItem` and `SetCurrentItem` (0D1C:02DC, 0D1C:0304) use the keyboard cursor's slot of the selected member instead. [V]

<!-- FIG chr_itemflow -->

A click on a character's slot runs `UseSelectedItem` (0BD2:0BA0): it checks that the slot takes the item in the hand, exchanges the two ids, redraws and recomputes armour class. A click on a hand slot with the left button runs `SwapSelectedItem` (0BD2:0C8E, the same exchange for hands) and with the other button `UseItemSlot` (`ClickHandSlot`, 0ACD:0415). A click on the floor runs `ClickFloorItem` (0ACD:0644): with an item in hand it is inserted into the cell's list (`InsertListItem`, 0BD2:01D0) and the cell script runs with trigger 4, drop; with an empty hand the first item at that position is removed (`RemoveListItem`, 0BD2:02B9) and the script runs with trigger 8, take. [V] That is how a dungeon script learns that you put a gem on a pedestal.

A floor item is a node in a circular, doubly linked list whose head id is a word in the cell's nine-byte record at `gMaze + cell * 9 + 5`. `InsertListItem` links the new item before the old head and makes it the head, so lists read newest first; `RemoveListItem` walks from the head and removes the first item whose `cellPos` equals the position asked for, or the item with the id asked for. `IdentifyListItems` (0BD2:040E) sets flag 40h on every item of a list; `PushListItem` and `DrainListItem` (0BD2:0458, 04AA) queue and drain a cell list. [V] A thrown item stays on the cell's list while a projectile record (`AddDDEffect`, 0BD2:07DE, ten records at `ds:DD2C`) carries the picture. [V]

#### 5.2.5 Using, naming and identifying

Potions are applied by `ApplyItemEffect` (0BD2:0D2D) on the item's value: 1 Giant Strength (strength 22 for (1d4 + 4) x 546 ticks, 2.5 to 4 minutes), 2 Healing (2d4 + 2), 3 Extra Healing (3d8 + 3), 4 Poison, 5 Vitality (food 100), 6 Speed (only a flash and the message "fast and agile": the handler does nothing else), 7 Invisibility (sets the flag; it lasts until the member acts), 8 Cure Poison; names come from the string tables at `ds:37E1` (potions), `ds:37B4` (rings), `ds:3837` (wands). Rings work only by being worn: the ring of Wizardry (value 1) doubles a mage's fourth and fifth level spells at camp, and the ring of Sustenance (value 2) should stop hunger, but see section 5.9.4. Wands call `UseWandItem` (0BD2:0EE8): the wand value picks the spell in `gWandSpells` (`ds:38FA`), value 5 (Silvias) pushes monsters back, and every other wand value is cast through `StartItemCast`, which takes a charge from the low bits of `flags` (`UseItemRow`, 0EF3:2604, called only by `CastSpell`) and destroys the wand at the last charge. The Silvias branch (value 5) and value 0 never reach `UseItemRow`, so a Silvias wand never loses a charge (`src/E0BD2.C:728-755`). [V]

`DrawItemName` (0BD2:05B5) shows what the player knows. An item with flag 40h shows its second name if it has one ("Dagger +4 'Guinsoo'"); with no second name the name is built from the type: "%s +%d" or "Cursed %s %d" for a weapon or armour with a value, "Mage Scroll of X", "Cleric Scroll of X", "Ring of X", "Potion of X" or "Wand of X" from the value. An item without the flag always shows its first name, so a magical sword is just "Long Sword" until identified. Identification is not a spell: script opcode `IdentifyItemsOp` (122E:1CAD) sets the flag on every carried item and quiver item of every present member when the party stands on the cell the script names, and always on the items lying on that cell. A negative enchantment is only a name and a penalty: it does not stick to the hand. [V] Flag 80h is independent: it makes Detect Magic show the item through the colour remap (`DrawItemIcon`, 0D1C:070F).

### 5.3 Mazes and level state

A level is made of three things that the game keeps apart. The walls come from `LEVELn.MAZ`. Everything that is not a wall (the script, the first monsters, the kinds of wall, the timers, the pictures to load) comes from `LEVELn.INF`. The items are not in either file: they live in one global table of 500 records (`gItems`, described with the items) and each record says which level and which cell it lies in. All twelve levels are 32 by 32 cells, so one set of arrays serves every level; only one level is in memory at a time.

#### 5.3.1 The grid and its cell

`LEVELn.MAZ` is a 6-byte header (u16 32, u16 32, u16 4) followed by 1,024 cells of 4 bytes, 4,102 bytes in all [V] (`wwfmt.maze`, `tools/re/wwfmt.py`; `LoadMaze`, ovl3:09AA, `src/EOV3.C`, reads 6 bytes and then 0x1000). A cell is numbered `y * 32 + x`. [V] `StepCell` (0D1C:00DA) adds `gDirCellOffset[dir]` (the bytes -32, +1, +32, -1 at ds:1790) and masks with 0x3FF; `DirectionFromPoints` (17A6:0314) and `Distance` (17A6:0F61) split a cell number into `x = c & 31` and `y = c / 32`.

Directions are numbered 0 to 3 everywhere (the party's `gPartyDir`, a monster's `orient`, a projectile's `dir`). [V] Direction 0 steps by -32, so it decreases y; [I] I call it north, then east, south and west. North and south wrap between row 0 and row 31, but a step east from x = 31 enters x = 0 of the next row, so the grid is not a torus. [V] With each level's own wall flags, two cells in the twelve mazes would accept a step off the map (level 1 at x 15, y 31; level 4 at x 30, y 31); whether they can be reached was not examined, so the wrap is probably never used [I].

<!-- FIG wld_grid -->

**The four bytes of a cell are its four sides, in direction order.** [V] `LoadMaze` copies each cell's 4 bytes unchanged to offsets +0 to +3 of the 9-byte record. `StepIfOpen` (0D1C:0080) reads the target cell's side `gOppositeSide[dir]` (the bytes 2, 3, 0, 1 at ds:547E), that is the side that faces the party after the step. The door code changes sides `s` and `s + 2` of one cell together (section 5.4.3). A wall between two cells is therefore stored twice, once in each cell, and nothing in the loader keeps the two copies equal; scripts change one side at a time (`SetWallTypeOp`, section 5.5).

The value of a side is a wall kind: 0 is open space, and the kind selects a row in four per-kind tables (section 5.3.2).

**Nine bytes in memory.** [V] `LoadMaze` clears `gMaze` (ds:B894, 0x2400 = 9,216 bytes, 1,024 cells of nine bytes), reads the file, and copies the four wall bytes of each cell to the start of its nine-byte record. The other five bytes are filled in by `LoadLevel` and by the game as it runs:

<!-- FIG wld_cell -->

- Byte +4, bits 0 to 2: the number of monsters in the cell. `MoveMonsterToCell` (08E6:0297) increments and decrements it, `StepIfOpen`, `SpawnMonster` and `CloseDoor` test `& 7`. [V]
- Byte +4, bits 3 to 7: the trigger mask, which script events the cell listens to (section 5.5.1). `LoadLevel` ORs the file's flag byte into it.
- Bytes +5 and +6: a word, the head of a circular doubly linked list of the items lying in the cell, as an index into `gItems` (0 for none). `InsertListItem` (0BD2:01D0) and `RemoveListItem` (0BD2:02B9) maintain it; each item record holds its two links, its owner cell (`key`) and its position in the cell (`cellPos`). [V]
- Bytes +7 and +8: the offset of the cell's script inside the level file (0 for none). [V] (`RunCellScript`, 122E:1DA8).

**Positions inside a cell.** A cell is not a point. A monster, an item or a flying object has a position in it: 0 to 3 for the four quadrants and 4 for the centre (items also use 8 for a niche in the wall, and add 4 while flying). [V] The quadrant numbers are consistent across three tables: `gViewToCellPos` (ds:1A1B, which cell position the party sees at each of its four view places, per facing), `gNextPosByDir` (ds:5CAB, where a thrown object goes next) and `gFrontLeftPosByDir` (ds:31F2, the quadrant a centre monster steps to). [I] Read together they put 0 at the north-west, 1 at the north-east, 2 at the south-west and 3 at the south-east; for example `gNextPosByDir` for a northbound object maps 2 to 0, 3 to 1 and sends 0 and 1 into the next cell as 2 and 3.

Two loader quirks. [V] The 6-byte file header is read into `gMaze[0..5]` before the walls overwrite bytes 0 to 3, so cell 0 keeps the header's last word (`04 00`) in bytes 4 and 5: it starts with a monster count of 4 (`LoadLevel` clears the item head later, not byte +4). [V] Dead monsters are moved to cell 0 (`KillMonster`), which counts them in its byte +4, so after a few kills the count carries into cell 0's trigger bits. Cell 0 also holds the level's timer script (section 5.5.1), whose mode is always enabled; I found no code that depends on either effect.

#### 5.3.2 Wall kinds

The byte in a side is an index into four parallel tables of 70 bytes each (the largest kind in any maze is 69) [V] (`DefineWallType`, ovl3:01C8):

<!-- FIG wld_walltype -->

| Table | Address | Meaning |
|---|---|---|
| `gWallVmpSet` | ds:1879 | the picture block set of the level's `.VMP`/`.EMP` that draws the kind (0: nothing is drawn) |
| `gWallDecorHead` | ds:18BF | the first decoration record of the kind (1-based; 0 none) |
| `gWallHandlerIndex` | ds:1905 | what happens when the wall is clicked or hit (section 5.4.4) |
| `gWallTypeFlags` | ds:194B | eight flag bits |

Kinds 0 to 22 are in the program's data and every level uses them: kind 0 is open space (flags 07), kinds 1 and 2 are plain walls (flags 00 and 40; [I] plain because they are solid and draw picture sets 1 and 2), and kinds 3 to 22 are four families of doors of five kinds each (section 5.4.3). The level file defines the others and redefines 23, 24 and, in level 12, 22. [V] The 12 level files define 24 to 36 kinds each, with kind numbers from 22 to 69; a definition is five bytes, (kind, picture set, decoration, handler, flags), and `DefineWallType` stores `flags ^ 4`, so the bit reads "monsters may pass" in memory and "blocks monsters" in the file. The values the files give the table are 00 (solid), 01, 02, 03, 05 and 07 (combinations of bits 0 to 2), 0E, 08 and 0A (stuck doors) and 80 (niches).

The bits are read from the functions that test them (the figure lists them). Bit 0 lets the party walk through and a thrown item fly; bit 1 lets small things (items whose icon size index is at least 15, and the line of sight of a monster's ranged attack) pass bars; a spell effect needs bit 0; bit 2 lets a monster in; bits 3, 4 and 5 mark a door and its open and closed frames; bit 7 marks a face that is not a plain wall picture. Bit 6 is set only on kind 2 and no code tests it [?].

#### 5.3.3 The level file

`LEVELn.INF` is in the same packed form as the other Westwood files (section 4.2); `LoadLevel` reads the u16 size and the body into the staging page and unpacks it into a 7,500-byte buffer (`gLevelInf`, ds:8DCA; the largest level, 2, unpacks to 7,444 bytes). The file is a header of fixed fields, then lists, then bytecode, then a table; the fields are in the figure.

<!-- FIG wld_inf -->

[V] Every field was decoded from `LoadLevel` and checked on all twelve files: the trigger table starts at the offset in the first word and ends exactly at the end of the file, and the scripts between the wall records and that table decode with no byte left over (section 5.5). Details that are easy to miss: the file names are NUL-terminated strings in 12-byte fields that still hold what the authoring tool left after the NUL (`brick\0.maz\0\0`); monster slots run 1 to 29, and a record with cell 0 is an empty slot (`InitMonster` clears slot 0 and returns); and the "pairs" are the periods of the monster update effects (section 5.6.3).

The wall records come in two kinds. [V] A record starting `EC` carries two names, a decoration sheet picture (for example `brick1`) and a `.DAT` file (`brick.dat`), and runs `LoadWallSet`; a record starting `FB` defines one wall kind (`DefineWallType`). Records of both kinds interleave: level 1 has three `EC` records (`brick1`, `brick2`, `brick3` with the same `brick.dat`), levels 2 and 3 have two, the others one. Each group of `FB` records cuts its pictures from the sheet named by the `EC` record before it.

#### 5.3.4 Loading a level

`LoadLevel(n)` (ovl3:0A7C) is called when a new game starts (`ResetMapScreen`, 0ACD:1033, loads level 1 and puts the party at cell 0x1EA = 490, x 10, y 15, facing 0), when a saved game is loaded (`LoadGame`, ovl3:15B9) and by the script opcode that changes level. It first clears bit 1 of `gTimerEnableFlags`, which stops the game clock (`gGameTicks`) while the level builds, and sets it again at the end. The figure lists the stages in order.

<!-- FIG wld_load -->

Two properties of the order are worth stating. [V] `LoadMaze` decides between a new level and a visited one: when bit `n - 1` of `gSavedLevels` is set it hands the level to `LoadLevelState`, which overwrites the walls (and fills the monster table) from `LEVELS.TMP`; otherwise the walls come from the maze file and the 30 monster records of the INF file are run through `InitMonster`. The scripts, the trigger masks and the wall kinds always come from the INF file. A change to a wall therefore survives a level change, while a script's own memory lives in flag bits (section 5.5.3). [V] The items are not loaded from any level file: `RebuildItemList` (ovl3:0951) walks the 500 records of `gItems` and puts every record whose `level` is the current level and whose `key` is positive into the list of the cell `key`. The first fill of that table is `ITEM.DAT`: 432 records, 82 of them with level 0 (not placed) and 350 placed on levels 1 to 12 (15, 26, 38, 30, 28, 45, 37, 30, 32, 17, 23 and 29) [V] (decoded `ITEM.DAT`, record bytes +6 and +12 read by `RebuildItemList`).

#### 5.3.5 Wall sets and decorations

A wall kind draws two things: a wall picture from the level's `.VCN`/`.VMP` (VGA) or `.ECN`/`.EMP` (EGA and CGA) pair, chosen by `gWallVmpSet`, and a chain of decorations, chosen by `gWallDecorHead`. [V] The wall sets are five: `brick` (levels 1 to 3), `blue` (4 to 6), `drow` (7 to 9), `green` (10 and 11) and `xanatha` (12), each with a `.DAT` file in the same archive as its levels.

[V] A `.DAT` file holds a u16 count and that many 52-byte decoration records, then a u16 count and that many 8-byte rectangles (`LoadWallSet`, ovl3:004F; the counts are 60 and 177 for `BRICK.DAT`, 40 and 143 for `BLUE.DAT`, 36 and 118 for `GREEN.DAT`, 27 and 86 for `XANATHA.DAT`, 35 and 115 for `DROW.DAT`, and every file's size equals its two tables exactly). A rectangle is four words (x, y, width, height) that `DefineWallType` passes to the sprite cutter to take one picture out of the decoration sheet; the cut pictures go to a 200-entry table of far pointers, `gDecorShapes` (ds:EFD4), each cut once. A decoration record is:

| Bytes | Content |
|---|---|
| +0 to +9 | ten rectangle numbers (FF: none), one per face the renderer draws [I] |
| +10 | the next decoration of the same wall kind (0 ends the chain) |
| +11 | flags (`DrawSlotDecorations`, ovl4:1091): bit 0 mirrors the picture, bit 1 mirrors it with the cell parity, bit 2 selects the x base |
| +12 to +31 | ten x positions (words) |
| +32 to +51 | ten y positions (words) |

[V] `DefineWallType` builds the chain of a kind: it copies the decoration the INF record names into the next free slot of `gWallRecs` (ds:E040), follows byte +10 to the next one, and stores the number of the first in `gWallDecorHead[kind]`. The renderer walks the chain for each face (`DrawSlotDecorations`, ovl4:1091, which addresses the copies at ds:E06A = `gWallRecs` + 0x2A as `gViewWalls`). The click test reads the same copies at ds:E06B, one byte further, so its `kind`, `x` and `y` are rectangle number 1, x position 1 and y position 1 [V, by the address arithmetic]: a click on a wall decoration is tested against entry 1 only. What the ten entries are is not established beyond one per face the renderer draws [?].

[V] Six pictures are cut from the `DECORATE` sheet at start-up by `LoadDecorateIcons` (ovl2:0577) into `gTeleporterCells`. They are drawn for wall kind 0x34, whose flags 3 and picture set 0 mean it is a passable face without a wall picture; effect 13 (period 10 ticks) flips `gAnimWallPhase` and redraws when kind 0x34 is in view, which animates the six cells. That kind 0x34 is a teleporter is [H] (ScummVM's Kyra engine, wall type 52, and the name `DrawSlotTeleporter` in `re/renames.toml`).

#### 5.3.6 Level state, saves and LEVELS.TMP

A level the party has left must come back as it was left. [V] The state kept is two things (`PackLevelState`, ovl3:0FD1; `UnpackLevelState`, ovl3:109D): the first four bytes of every cell, LCW-compressed into at most 0x4B0 = 1,200 bytes, and the 30 monster records, copied whole (840 bytes); a slot is 2,040 bytes (0x7F8). If the compressed walls exceeded 1,200 bytes the game would print "Map compression failure: size of map = %d" and quit. The sample save's slot for level 1 decodes to `LEVEL1.MAZ` byte for byte (550 bytes of LCW with its end marker) and so does the slot for level 7; the slots for levels 2 to 6 and 8 to 12 decode to mazes that differ from their files by 10 to 125 wall bytes (doors left open by an earlier game; only bit 0 of `gSavedLevels` is set, so the game never reads them from this save).

<!-- FIG wld_state -->

[V] `SaveLevelState(n)` (ovl3:11AF) writes slot `n` of `LEVELS.TMP` at `(n - 1) * 0x7F8` and sets bit `n - 1` of `gSavedLevels`; `LoadLevelState` (ovl3:1147) reads it back. `LEVELS.TMP` holds 12 slots (24,480 bytes) and is the same data as the fourth block of `EOBDATA.SAV`: `SaveGame` (ovl3:1368) first packs the current level into the image, then writes the six blocks; `LoadGame` rewrites `LEVELS.TMP` from block 3 of the save. On load, `UnpackLevelState` also adds one to byte +4 of the cell of each of the 30 monsters, because the count is not stored.

What is not in a slot: the items (global, with their own level and cell), the 32 flags per level and the 32 game flags, and the party's position. [V] Those travel in the 64-byte block at ds:FBC8 that `SaveGame` writes between the characters and the items: `gCurLevel` (FBC8), `gPartyCell` (FBCA), `gPartyDir` (FBCC), `gMouseItem` (FBCE), `gSavedLevels` (FBD0), the party spell flags (FBD2), `gLevelFlags` (one 32-bit entry per level, indexed by the level number 1 to 12 from FBD4, so level 1 is at FBD8 and level 12 shares FC04 with `gGameFlags`, section 5.11.1) and `gGameFlags` (FC04). The test save has level 1, cell 490 (x 10, y 15), facing 0, and only bit 0 of `gSavedLevels` set.

#### 5.3.7 Between levels, and the twelve levels

A level change is the script opcode `EC` (section 5.5). [V] `LoadLevelOp` (122E:1AAB) sets the party's cell (and its facing unless the byte is FF), calls `ResetLevel` (ovl3:19F7: clears the attack frames, puts every flying item down, finishes a door that is still moving), `SaveLevelState` of the old level, `LoadLevel` of the new one, reloads the inventory screen and calls `MoveParty`. Because the cell was stored before `MoveParty` read it, `MoveParty` runs the destination cell's leave script (mode 2) and then its enter script (mode 1), both from the new level's file; the old script is stopped (`gScriptDone`) because its data are gone. [V] Stairs and pits use the same opcode; a pit is a script with direction FF (keep the facing) and always leads one level down.

<!-- FIG wld_levels -->

[V] The 12 files contain 125 level changes. 54 scripts have the pair of four-way groups described in section 5.5.5 and 50 of those change level by stairs; 41 of the 59 falls are on level 5 (a hole of 41 cells [I]), and every fall goes down by one level. [V] Level 7 is a hub: it links to levels 2, 4, 5, 9 and 11, each with one cell on each side, in addition to its stairs to 6 and 8, and levels 6 and 8 each link to level 10. [?] What those links are in the game is not established from the code; the archive of level 7 holds `PORTALA.CPS` and `PORTALB.CPS`, so they may be magic portals. The levels differ in their timer (section 5.5.1): level 1 has none, levels 2 and 9 and 10 use 80 steps, 3 uses 100, 4 uses 150 and the others 50.

### 5.4 Moving through the world

#### 5.4.1 Facing, keys and the cells in view

The party has one position, `gPartyCell`, and one facing, `gPartyDir` (0 to 3). [V] There is no separate row or column: the cell number is the position. Turning changes only the facing (`TurnLeft` and `TurnRight`, 0ACD:056C and 0ACD:0593, wrap modulo 4 and set `gViewDirty`). Walking is four handlers over one test (`MoveForward`, `MoveBackward`, `MoveLeft`, `MoveRight`, 0ACD:04F8 to 05FF): each picks a heading (the facing, +2, -1, +1) and calls `StepIfOpen`; a cell number moves the party with `MoveParty`, and -1 prints "You can't go that way." and plays sound 0x1D (`MoveBlocked`, 0ACD:0008). The `load-walk` golden session shows it on the original: the up arrow into rubble is blocked, the right arrow steps one square sideways without turning, and the left arrow steps back.

<!-- FIG wld_view -->

What the party sees is the 18 cells in front of it and beside it (`InitViewSlots`, ovl4:0000): 7 three cells ahead, 5 two ahead, 3 one ahead and the 3 in its own row, taken by a per-facing row of offsets from ds:199D. [V] `IsCellInView` (0D1C:0006) is how anything that changes a cell (a monster moving, a door, an item) asks if the cell is on screen; it searches only the first 15 words of `gViewCellIds` (ds:A9C8), while the table has 18, so the party's own row never marks the view dirty by itself [V; the loop count is 15 in `src/E0D1C.C` and the table has 18 in `src/EOV4.C`].

#### 5.4.2 MoveParty and the loop around it

`MoveParty(cell)` (122E:0001) is short and is the place where the world reacts to a step: [V]

1. `AlertNearbyMonsters` (monsters near the old position, section 5.6.5);
2. `gPartyCell = cell`;
3. `RunCellScript(old, 2)` then `RunCellScript(new, 1)`: the leave and the enter events;
4. `AlertNearbyMonsters` again;
5. `gStepCount++` (the level timer counts steps), `gPickViewPos = 0xFF`, `gViewDirty = 1`, a newline to the message area, `UpdateDDEffects` (flying objects are tested at the new position).

<!-- FIG wld_step -->

Time passes elsewhere. [V] `RunMapLoop` (0ACD:0E8A) loops forever: poll for one input event and dispatch it, `RunEffects`, redraw if `gViewDirty` is set, an ambient sound on levels 1 to 3, `TurnUndead` (levels 2 and 7). `RunEffects` (1414:04E7) runs each of 16 timed effects whose expiry has passed and then `TickLevelScriptTimer`. The table `gEffects` (ds:5BA8) has the periods in the figure; monsters, doors and flying objects are all effects. [V] The input step does not wait: `DispatchKeyWait` (16A5:005A) reads a key only if one is pending and always dispatches (a zero key for a pure mouse event), so the loop runs continuously and effects run without input. The game clock is the INT 1Ch counter `gGameTicks`, 18.2 ticks per second on a stock timer [I; the code only counts ticks, and `RunMapLoop` schedules its random sound in multiples of 18 ticks].

#### 5.4.3 Doors

[V] A door is not a wall between two cells. It is a cell of its own whose two opposite sides carry the same door kind: `OpenDoor` (122E:0154) takes side 0 when its kind has flag bit 3 (a door running north to south) and side 1 otherwise, and `StepDoor` (1414:03B4) adds 1 or -1 to that side and to the one opposite. A door is five wall kinds in a row (family A is kinds 3 to 7, B 8 to 12, C 13 to 17 and D 18 to 22; A and C carry the door-button handler on every frame, B and D the "cannot be pried" handler on the closed frame, and C and D let small things through the bars).

<!-- FIG wld_door -->

The first kind of a family has flag 0x20 (closed), the last 0x10 (open) and the three between are the opening frames; `StepDoor` stops itself at the end of its direction. Effect 12 does the animation: `OpenDoor`, `CloseDoor` and `ToggleDoor` arm it twice around a first `StepDoor` call, so the first frame shows at once, and it steps again every 5 ticks. The door pictures come from the `DOOR` sheet; two INF header bytes choose which of eight picture sets to cut and into which of six slots (`LoadDoorShapes`, ovl3:03A2). [V] Sound 3 plays at each opening step, 4 at each closing step and 5 when the door is closed. There is one animation slot: while a door moves, a second request for the same cell sets or reverses the direction and a request for another cell is ignored. [V] Quirks: `CloseDoor` does nothing while any monster is in the cell (`& 7`), but `ToggleDoor` tests only bit 0 of the count, so a door holding two or four monsters can be toggled shut; and `ResetLevel` finishes a moving door by adding the step until the kind has bit 0x30, with no other bound.

Doors are moved in three ways. [V] A click on the button picture next to a door (`DoorButtonAction`, 122E:04CA) toggles it and plays sound 6; a script opens, closes or toggles it by cell; and a stuck door (kinds 30 and 31, handler 5, on levels 2 and 5 to 9; on levels 10 to 12 the same numbers are scripted wall kinds) is forced by `ForceDoorAction` (122E:0325): the member with the highest strength score rolls 1d20 against `gForceDoorRoll[strength]` (1 at strength 0, 3 at 3, 7 at 11), and on success the kind becomes 8 (for 30) or 0x12 (for 31) on both sides and the door opens. [V] The member choice has a quirk: it adds the same percentile to both sides of the comparison, so only the score counts.

#### 5.4.4 Clicking and hitting walls

Everything the party can do to a wall goes through one dispatcher, `DispatchWallHandler` (122E:0896).

<!-- FIG wld_click -->

[V] The event is carried in the global `gWallTrigger`: 0x40 for a mouse click on the 128 by 96 view node (the node at ds:4836 has the callback `ClickWallAhead`, 122E:0933, read from the data), 0xFF for the keyboard "activate" (key 0x34, `ActivateWallAhead`, 0ACD:08E7), 0x10 when a thrown item meets a wall and 1 when a spell effect does (`StepDDEffect`, 1414:003E). The dispatcher takes the wall of the cell facing the party or the flight, looks up the handler selector of its kind, stops if the cell's trigger mask does not include the event, and calls the handler from a table of ten far pointers.

The handlers are: 1 the door button, 2 run the cell's script, 3 and 4 change the wall kind by +1 or -1 (a lever or a button that flips its picture) and run the script, 5 force a door, 6 print "No one is able to pry this door open.", and 10 the wall niche. [V] The niche (`WallNicheAction`, 122E:06D0) is a shelf in a wall face: items put there have position 8 in the cell's list; clicking with an empty hand takes one (script mode 8), clicking with an item puts it in unless the item is too large (`gIconPictureIndex` below 15: "The item is too large to fit.") and runs mode 4. The selectors 7 to 9 are empty in the table and unused. Selector 255 is never dispatched at all (`gWallHandlerIndex[...] - 1 < 0`): a wall of such a kind is hit by `ApplyItemAttack` (0DE8:0359), which adds 1 to its kind on both faces; [?] the kinds that use it are probably breakable walls.

[V] Handlers 2, 3, 4 and 10 begin with `IsWallDecorClicked`: in mode 0x40 the last click position (`gClickX`, `gClickY`, tested by `ClickInRect`, 082C:0A5D) must fall inside the decoration's rectangle; in any other mode the answer is yes. A script placed on a click-driven cell does not usually test the mode: of the 871 scripts in the twelve files, 397 listen to no event of their own and run only on a click, the key or the timer; the mode is tested 209 times in all and never against 0x40 or 0xFF.

Items on the floor use a related path. [V] `ClickFloorItem` (0ACD:0644) puts the mouse item into, or takes one from, the list of the party's cell (floor slots 0 and 1) or of the cell ahead (slots 2 and 3, only when the wall in between has flag bits 0, 1 or 3), at the quadrant `slotKind[gPartyDir][slot]`; it then runs the cell's script in mode 4 (put) or 8 (take). `ThrowMouseItem` (0ACD:0494) starts a flying item instead.

#### 5.4.5 Things in flight

Ten records (`gProjectiles`, ds:DD2C, 18 bytes) hold every thrown item and every spell effect on the level. [V] `AddDDEffect` (0BD2:07DE) takes a free record for a thrown item (state 1), puts the item in the list of its cell with the tag `quadrant | 4` and sets the range to 12; `StartCellEffect` (0EF3:097B) does the same for a spell effect (state 2, with the spell's own callback). Monsters use both: a mage or a kuotoa starts spell effects, a drider throws item template 0x3C (section 5.6.5).

<!-- FIG wld_proj -->

[V] Effect 1 runs `MoveDDEffects` (1414:0285) every 3 ticks. For each live record the next position comes from `gNextPosByDir`: one half-cell step, with `StepCell` when it crosses a cell edge. `StepDDEffect` tests the wall of the target on the side facing back: an item passes with flag bit 0, while it is still in its launch cell, or with bit 1 when it is small; a spell with bit 0 or when the move stays inside the cell. Otherwise the flight ends and the wall's handler is dispatched with mode 0x10 (items) or 1 (spells), which [I] is how a thrown item can press a button. A successful move re-files the item in the new cell's list, runs the new cell's script in mode 0x10 when it enters a new cell, and runs `TestDDEffect` (1414:0172): monsters in that quadrant or the centre get `TryRecordAttack` (to-hit and damage, section 5.7); the party's cell gets `RunRecordAttack`, which hits a party member chosen from the quadrant. A record ends after a hit, when its range runs out or at a wall; `ClearDDEffect` (1414:020F) puts the item down (tag `& 3`), runs the cell's script in mode 4 and plays sound 0x12. `UpdateMonsters` and `MoveParty` also call `UpdateDDEffects`, so a monster that steps into a flight path is hit.

#### 5.4.6 What is not there

[V] There is no automap and no minimap. The name `gMapPhase` (ds:DCAA) is a leftover: `EnterMapScreen` sets it to 1 and the only functions that read it test for 5, which is never stored (`HandleMapKey`, `MoveKeyCursor`, `MoveListItem`), and `StepMapPhase` (0ACD:0753), which would advance it, has no caller. "The map" in function names (`EnterMapScreen`, `RunMapLoop`, `HandleMapKey`) means the dungeon screen.

### 5.5 Cell scripts

Everything that happens in a level beyond walking and fighting is a script: stairs, levers, doors that open on a lever far away, messages, traps, wandering monsters, the quests. A script is a fragment of bytecode inside the level file, attached to one cell, run by an interpreter that reacts to events.

#### 5.5.1 Where a script is, and what wakes it

[V] The trigger table at the end of the file has one entry per scripted cell: the cell (u16), a flag byte and the script's offset (u16). `LoadLevel` ORs the flag into byte +4 of the cell and stores the offset at +7. Of the 871 entries in the twelve files, scripts are 4 to 321 bytes long, 56 on average, and no two entries share a script.

The flag byte is a mask over five events, which `RunCellScript` (122E:1DA8) and `DispatchWallHandler` test as `mode & (((cell[+4] & 0xF8) >> 3) | 0xE0)`. [V] The cell's bits 3 to 7 enable modes 1, 2, 4, 8 and 0x10; the modes 0x20, 0x40 and 0x80 are always enabled. The modes are raised by:

| Mode | Raised by | Entries that listen (of 871) |
|---|---|--:|
| 1 | the party enters the cell (`MoveParty`); a spell effect hits a wall of it | 394 |
| 2 | the party leaves it (`MoveParty`) | 43 |
| 4 | an item is put into the cell or lands in it (`ClickFloorItem`, `WallNicheAction`, `MoveListItem`, `ClearDDEffect`) | 144 |
| 8 | an item is taken from it | 32 |
| 0x10 | a thrown item flies into the cell or hits its wall (`MoveDDEffects`, `StepDDEffect`) | 116 |
| 0x20 | the level timer, on cell 0 only (`TickLevelScriptTimer`, 08E6:1410) | always |
| 0x40 | a mouse click on a wall (`ClickWallAhead`) | always |
| 0xFF | the keyboard "activate" | always |

The flag values the files use are 0 (397 entries: click, key and timer only), 08 (247), 18 (40), 20 (37), 28 (2), 40 (6), 48 (8), 60 (15), 78 (3), 80 (22), 88 (7) and A8 (87, the stairs). [V] Nothing in the program raises 0x80.

**The level timer.** The INF header gives a mode (0 off, 1 ticks, 2 steps, 3 both), a period in ticks and a number of steps; only mode 2 is used, with 50 to 150 steps (level 1: off). [V] `RunEffects` calls `TickLevelScriptTimer` on every pass; when `gStepCount` has reached the steps, it runs the script of cell 0 in mode 0x20, then, when none of monster slots 1 to 29 is free, kills the first one by slot number that is more than 3 cells from the party (and `KillMonster` pays its experience and drops its loot), and resets the count. Levels 2 to 10 have a script on cell 0; it is the "wandering monsters" script (it spawns monsters). [V] In mode 3 the source's deadline test is the reverse of mode 1's (it fires while the deadline is still ahead); no level uses mode 3.

#### 5.5.2 The interpreter

[V] `RunCellScript(cell, mode)` reads the offset at cell byte +7, tests the mode against the mask, records the cell and the mode (`gScriptCell`, `gScriptTrigger`), clears `gScriptDone` and loops: read a byte; if it is in 0xE6 to 0xFF, call handler `table[0xFF - op]` with the pointer just after the opcode, and add the number of bytes it returns to the pointer; any other byte is skipped. The loop ends when a handler sets `gScriptDone`. A handler reads its own operands and returns how many bytes it used, or, for a jump, how far the pointer moved. Jumps and calls are offsets from the start of the unpacked file.

<!-- FIG wld_script -->

The state is global: `gScriptCell`, `gScriptTrigger`, `gScriptDone`, a call stack of ten far pointers (`gScriptStack`, `gScriptStackDepth`), and the expression stack `gEvalStack` with its index. [V] Because the opcode table is indexed by `0xFF - op`, the table order is the opcode order from FF down to E6. Scripts can nest: `LoadLevelOp` calls `MoveParty`, which runs two inner scripts, and the outer driver then stops through `gScriptDone` because the data it was reading belong to the old level.

**The format has been verified on every byte of the twelve levels.** [V] A disassembler written from the handlers (a research tool, not part of the repository) decodes the script area of all twelve files, 52,449 bytes and 7,054 instructions, with every script ending exactly at the start of the next and the last at the start of the trigger table. No opcode is unknown and no byte is left over.

#### 5.5.3 The opcodes

<!-- FIG wld_opcodes -->

Conventions in the table. Many opcodes begin with a **sub-opcode byte** that names the kind of object the operands refer to, with the same codes everywhere: E8 the party, EA a door, ED the facing, EF a flag of the level, F0 a game flag, F3 monsters, F5 items and flying things, F7 a cell's four sides, E9 one side. A **cell** is a u16 cell number and an **offset** a u16 from the start of the file. A member number of FF means "all six" (or, for `STEAL ITEM`, a random one). **Flags** are the bits of two 32-bit words, `gLevelFlags[gCurLevel]` and `gGameFlags`; they are the only memory a script has between runs. `SPAWN` has the 14 bytes of a monster record of the INF file (section 5.6.1). Two quirks: `DAMAGE` on one member rolls the dice and then damages with the dice count, not the roll (all-members damage uses the roll; `DamagePartyMember`, 122E:12CE), and `MOVE` with sub F1 reads a cell and does nothing but redraw.

The panels `OVERLAY` can open (`CharOverlay`, ovl5:1A82) are numbered 0 temple, 1 item panel, 2 resurrection, 3 attack, 4 kill her, 5 surrender, 6 quit, 7 free him, 8 tile animation, 9 the ending and 10 the find-word panel. [V] Panel 10 (`FindWordPanel`, ovl2:09ED) is a manual look-up check: it picks one of 30 symbols, asks for a given word of a given line of the manual, and quits the game after three wrong answers. The stairs of levels 1 and 6 run it the first time the party goes down (level flag 31).

#### 5.5.4 Expressions

`IF` is the only conditional. [V] Its operands are a postfix expression, then `EE`, then the u16 offset to jump to when the result is false (`EvaluateScriptExpr`, 122E:147F). It evaluates over `gEvalStack`: a byte from 0 to 0x7F pushes itself, a token pushes a value, a binary token pops two and pushes the result. Unknown bytes with the high bit set push nothing.

| Token | Operands | Pushes |
|---|---|---|
| 0 to 7F | none | itself |
| E0 | none | the mode that started the script |
| ED | none | the party's facing |
| F7 | cell | the kind of side 0 of the cell |
| E9 | side u8, cell | the kind of that side |
| F5 | type u8 (FF: any), cell | the number of items of that type in the cell |
| F2 | item u16, cell | the item's index if it lies in the cell, else 0 |
| F3 | cell | 1 if the cell's monster count is odd |
| F1 | cell | 1 if the party is in the cell |
| F0, EF | bit | the game flag, the level flag |
| E7 | sub | the item on the mouse: sub E1 its type, F5 its number, other its value |
| DD | race | 1 if a living member has that race (`race >> 1`) |
| DC | mask | 1 if a living member's class has any of the bits |
| DB | n, sides, bonus | a dice roll |
| DA | none | 1 if every living member has effect bit 2 set |
| FF, FE, FD, FC, FB, FA | none | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| F9, F8 | none | `&&`, `||` |

The most common expressions in the files are `literal literal ==` (432, see section 5.5.5), `mode N ==` (195), `level flag N == N` (178), `wall(cell, side) == N` (104), `wall(cell) == N` (91), the held item's type `== N` (91), `dice == N` (56), `facing == N` (54), the class test (47) and the race test (46). [V] [I] A key door is the pair "held item's type is T and its number is N" (68 times), and the 121 uses of `DRAIN` with sub FF take the held item off the mouse and free it, so keys are probably consumed.

#### 5.5.5 A worked example

<!-- FIG wld_stairs -->

The level 1 stairs carry flag A8 (enter, drop and flight events), as do 86 other entries; 54 scripts, 50 of them with a stair `LOAD LEVEL`, contain the same four-way groups. [V] The script tests the mode, then the facing, may show a panel the first time, changes level, and otherwise pushes the party (or an item put there) out of the cell. The push is a quirk of the data that appears 108 times: the four tests of a group compare a constant with 0, 1, 2 and 3, where the design seems to call for the party's facing; the constant is fixed, so exactly one arm of each group runs, always the same one. [?] Whether the authoring tool wrote the constant, or token ED was meant, is not known.

#### 5.5.6 What the data shows

[V] Counted over the twelve files (7,054 instructions): `IF` 1,572 (22%), `END` 871, `SOUND` 770, `TEXT` 668, `MOVE` 653 (324 of the party, 310 of items, 19 of monsters), `JUMP` 544, `SET WALL` 491 (274 of all four sides, 203 of one side, 14 of the facing), `DRAIN` 238, `SET FLAG` 208 (187 of level flags, 13 of game flags, 8 of monsters), `GIVE XP` 143, `CALL` 136, `LOAD LEVEL` 125, `TOGGLE WALL` 112; `HEAL` is never used. Experience is given by scripts as well as by kills: a script pays once, guarded by a level flag (cell 753 of level 1 pays 1,600 and sets level flag 10). The 136 `CALL`s go to shared subroutines that sit between the wall records and the first trigger script (the area holds 35 `RETURN`s).

[V] Scripts are where the game's quests live. Level 2's subroutine area ends with a check headed by the text "special quest for this level!": when level flags 4, 5, 6 and 7 are all set and game flag 2 is clear, it prints the text, sets game flag 2 and creates four items (`NEW ITEM`, item 55, in four cells). The same flags tie the systems together: waking a sleeping group of type 7 sets game flag 18 and level 5's timer script (cell 0) tests it to spawn a dwarf; waking a group of type 12 sets game flag 27 and level 7's timer script then tries to spawn two drow (section 5.6.4). Scripts also build and tear down walls: [I] the 491 `SET WALL` instructions are probably most of the secret doors and the gates that open when a lever far away is pulled. What each of the 871 scripts is for in the game was not traced.

### 5.6 Monsters

A level has at most 30 monsters at a time, and all of them are in one array. Their rules are few: a record, a row of a type table, an update that runs by mode and group, a placement rule inside a cell, and what happens when they are hit.

#### 5.6.1 Three records

<!-- FIG wld_monster -->

[V] The level file lists 30 monsters of 14 bytes (slot, speed class, cell, position, facing, type, alternate sheet, mode, timer, two drops). `InitMonster` (08E6:131D) builds the live record from it: it clears the 28 bytes, stores the type (`typeIndex`), `updateGroup = speedClass * 2 + (slot & 1)`, position, sheet, mode, timer, facing, `actionsLeft` from the type, rolls the hit points as `HD d8` (or 1d4 when the type's hit dice byte is 0xFF), stores both drops, and moves the monster into its cell (`MoveMonsterToCell`, which counts it in byte +4 of the cell). The record has fields for maximum hit points at +C, written with the same roll as +E; nothing reads +C, so the name `maxHp` is [H]. Of the 12 files, 269 of the 360 monster slots hold a monster at the start (24, 27, 29, 15, 23, 28, 29, 28, 22, 14, 22 and 8 per level).

**The type table.** [V] `gMonsterTypes` (ds:1A3B) has 22 rows of 27 bytes. The meaning of each byte is read from the code that uses it: `+0` is the armour class that `GetHitTarget` (0A72:0522) subtracts to give the party's to-hit number; `+1` is the monster's own to-hit number; `+2` the hit dice; `+3` the number of attacks and `+4` three (count, sides, bonus) dice; `+E` a flag byte; `+F` three bits (held by Hold Person, held by Hold Monster, undead); `+13` the experience `KillMonster` hands to the party; `+15` a size flag; `+16` and `+17` the attack and movement sounds; `+18` the number of ranged actions (255 unlimited); `+19` the row of the turn-undead table; `+1A` the magic resistance in percent (`MonsterResistsMagic`, 0D1C:0BAB: it resists when 1d100 is below it). The game never prints a monster's name, so the names below are the sprite sheet the level files pair with the type [I]; the type of the final monster appears in no level file and is spawned by a script on level 12.

| # | sheet | AC | to-hit | HD | attacks | XP | flags | holds / undead | size | MR % |
|--:|---|--:|--:|---|---|--:|---|---|--:|--:|
| 0 | kobold | 7 | 18 | 1d4 | 1d4 | 15 | - | HP, HM | 0 | 0 |
| 1 | leech | 7 | 17 | 2d8 | 1d4 | 120 | W | HM | 1 | 0 |
| 2 | skeleton | 5 | 17 | 1d8 | 1d6 | 65 | M, F | U | 0 | 0 |
| 3 | zombie | 8 | 15 | 2d8 | 1d8 | 65 | - | U | 0 | 0 |
| 4 | kuotoa | 4 | 16 | 2d8 | 1d4+1 and 1d4+1 | 175 | W, R | - | 2 | 20 |
| 5 | flind | 5 | 17 | 3d8 | 1d8 | 65 | F | HP, HM | 0 | 0 |
| 6 | spider | 6 | 19 | 2d8 | 1d8 | 650 | W, P | HM | 2 | 0 |
| 7 | dwarf | 3 | 12 | 7d8 | 1d8 | 730 | - | HP, HM | 0 | 0 |
| 8 | skelwar | 5 | 7 | 10d8 | 1d8+6 | 1500 | M, F | U | 0 | 0 |
| 9 | disbeast | 4 | 15 | 6d8 | 1d8 and 1d8 | 975 | W | HP, HM | 1 | 0 |
| 10 | mantis | 5 | 13 | 6d8 | 1d8, 1d4, 1d4 | 1400 | F, Z | HP, HM | 2 | 0 |
| 11 | shindia | 4 | 13 | 5d8 | 1d4 and 1d4 | 1250 | - | HP | 0 | 0 |
| 12 | drowelf | 4 | 15 | 4d8 | 1d8+4 | 650 | Z | HP | 0 | 60 |
| 13 | golem | -1 | 7 | 10d8 | 3d8 | 10000 | W | HM | 2 | 100 |
| 14 | mage | -1 | 7 | 9d8 | 3d4 | 2000 | W, R | HP, HM | 0 | 0 |
| 15 | drider | 3 | 13 | 7d8 | 1d6 | 5000 | R | HM | 2 | 60 |
| 16 | kenku | 5 | 13 | 7d8 | 1d6 | 5000 | R | HM | 0 | 0 |
| 17 | mflayer | 5 | 11 | 8d8 | 5d20 | 8000 | W, R | - | 0 | 90 |
| 18 | rust | 2 | 15 | 5d8 | 1d1 | 270 | W, S | HM | 2 | 0 |
| 19 | xorn | -2 | 13 | 8d8 | 6d4 | 4000 | W | HM | 2 | 50 |
| 20 | hellhnd | 4 | 13 | 7d8 | 1d10 | 975 | W | HM | 1 | 40 |
| 21 | xanath | 0 | 5 | 35d8 | 1d100 | 14000 | W, R | - | 2 | 100 |

Flags (`+E`), each read from the code that tests it: L large-creature weapon damage (`RollAttackDamage`, 0DE8:011A), W the mirrored stepping frames (`ovl4:0D1B` picks the walk frame table), M half damage from weapons whose item type lacks bit 2 (`IsWeaponDamageHalved`, 0D1C:0BEB; [I] the bit is a magic or silver weapon mark), F attacks again after moving (`UpdateHuntingMonster`), P poison on a hit, Z paralysis on a hit, R ranged or spell attack, S destroys an item (`ReportLostItem`). Holds (`+F`): HP held by Hold Person (`HitHoldPerson`, 0EF3:1530), HM held by Hold Monster (0EF3:1A30), U undead (`TurnUndead`). The "to-hit" column is the number `RollMonsterToHit` subtracts the target's armour class from. [?] Some rows do not look like what their sprite sheet suggests (the kenku and the drider rows are both 7d8 with 5,000 experience); the sheet-to-type pairing is the level files' choice and the names are only labels.

#### 5.6.2 Placement inside a cell

[V] A cell holds at most four monsters, one per quadrant, or one in the centre. `BuildCellPosMask` (08E6:017E) builds the bit mask of the quadrants in use: a count of 4 gives -2 (full), and a monster whose type has the size byte nonzero in the cell gives -1, so a large monster is alone in its cell and no one joins it. `FindFreeCellPos` (08E6:0218) picks a free quadrant, searching in a fixed order that depends on the monster's facing (`gPosSearchOrder`, ds:31F6). A monster that moves into an empty cell keeps its quadrant; one that arrives where others stand takes a free one; `RepickMonsterCellPos` (called when a monster next to the party is not in its front half) puts a lone monster in the centre and any other in a free quadrant. [V] Position matters for fighting: a monster can only strike the party if it stands in the half of its cell that faces the party (`IsMonsterAtFront`, 08E6:103D, the table `gMonsterFrontPos`), and the party's blows and flying objects hit by quadrant (section 5.7).

#### 5.6.3 The update

Monsters move on timers, not on the party's steps. [V] `LoadLevel` reads from the INF header two speed periods (pairs of group and ticks, for example 25 and 35 on level 1, 15 and 25 on level 4) and sets them as the periods of effects 2 to 5; effects 2 and 3 update groups 0 and 1 (speed class 0, even and odd slots) and effects 4 and 5 groups 2 and 3. The odd groups' first expiry is half a period after the load. At 18.2 ticks a second [I] the periods are about 0.8 to 2 seconds per move, and the two halves of a class alternate so a group does not move all at once. Each effect calls `UpdateMonsters(kind)` (08E6:0CC8), which for every monster of the group with hit points above 0 runs the steps in the figure.

<!-- FIG wld_monai -->

#### 5.6.4 Modes

The mode (`Monster +8`) is the state machine. [V] 0 hunts: it heads for `targetCell`. 1, 2 and 3 wander: step one cell ahead, and when blocked turn by +2, -1 or +1 (`StepMonsterOrTurn`, 08E6:0B34; the turn bytes are shared with modes 5 and 6 in the table at ds:3234, which overlaps ds:3237). 5 and 6 walk along the wall on the left or the right (`StepMonsterAlongWall`, 08E6:0B94, with the `state` byte as memory). 4 stands. 7 and 10 count the timer down and then become 0; the hold spells put monsters in mode 10 for 15 updates. 8 is asleep with its group: nothing alerts it by distance, but when it is attacked or one of its group dies `AlertMode8Monsters` (08E6:0055) wakes them all, and it also sets two game flags when the last one woken is of type 7 or 12. 9 is never alerted by distance but fights when attacked (`ChasePartyIfAttacked`). The level files start monsters in modes 1 (10 monsters), 3 (1), 4 (84), 5 (87), 6 (57), 7 (7) and 8 (23).

#### 5.6.5 Noticing, hunting and attacking

**Noticing.** [V] `AlertMonsterToParty` (08E6:00C3), run for every monster at every update and twice by `MoveParty`, sets mode 0 with `targetCell = gPartyCell` for a monster that is not in mode 7 to 10 when `Distance` to the party is under 4 and the party is not in the monster's rear arc (a direction code 3, 4 or 5 octants from its facing) unless it is under 2 away. There is no wall test and no line of sight: monsters notice the party through walls. The distance is the octagonal one, the larger delta plus half the smaller.

**Hunting.** [V] A hunting monster does one of three things per update, in this order (`UpdateHuntingMonster`, 08E6:0AB8). It tries a ranged attack (`TryMonsterRangedAttack`, 08E6:0765): the type must have the `R` flag, actions left and a delay of 0 (5 updates), the party must be within 3 cells exactly in front of it (the direction code equals twice its facing) and every cell between must pass flag bits 0 or 1 and hold no monster, which is a straight corridor. Otherwise it tries a melee swing (`TryMonsterMelee`, 08E6:05EA): the cell ahead must be the party's and the monster in the front half; the swing flag toggles, so a monster swings every other update; sound, the two swing frames (0xFE, 0xFF) and `ResolveMonsterMelee`. Otherwise it moves (`StepMonsterToward`, 08E6:0430): next to the party it turns one quarter at a time; elsewhere it follows `DirectionFromPoints` to `targetCell`, trying the eight headings in a fixed order from `gDirSearchRight` (or `gDirSearchLeft` when `gDirSearchLeftFirst` is set; only a `state` above 6 would flip it, and I found no code that sets `state` above 1) and taking the first that `TryMoveMonster` (08E6:0309) accepts. A fleeing monster (flag 8, set by `MakeMonsterFlee`, the Fear spell's helper, and by a turned undead) reverses the heading. [V] `TryMoveMonster` refuses a cell whose wall on the back side lacks flag bit 2, the party's cell and a full cell, and plays the type's movement sound at the monster's cell. A type with flag `F` melee-swings again after moving. [V] The target cell is only updated while the monster notices the party; when the party moves out of range the monster keeps going to the last cell. [V] The direction helper returns 255 for "no direction" (the same cell), but `StepMonsterToward` tests `== -1`, which can never be true (the assembly returns the byte zero-extended); a monster standing on its target therefore falls into the search loop with direction 255 instead of returning.

**The attack, up to the call.** [V] `ResolveMonsterMelee` (08E6:10F3) picks the side of the party the monster faces (`gAttackSideByDir`), then one of two halves of that side by the monster's quadrant (a centre monster rolls 1d2), and scans the six members of the half in a fixed order (`gAttackTargetOrder`, ds:325B, front pair first) for the first one who is alive (status bit 0, hit points above -10), skipping an invisible member unless 1d20 is under 5. It then rolls each of the type's attacks with `RollMonsterToHit` (08E6:106B: 1d20; a 20 always hits; the target's effect bit 9 costs the roll 2 and the party spell flag bit 5 costs 1; it hits when the roll reaches the type's to-hit number minus the target's armour class), adds that attack's damage dice for each hit, and, if the total is above 0, reports a lost item for `S` types, calls `DamageCharacter`, and applies poison or paralysis for `P` and `Z` types. The saving throw for the status effects and the damage rules themselves are section 5.7.

The special attackers are the types with the `R` flag, and `TryMonsterRangedAttack` switches on the type: 4 (kuotoa) starts spell effect 9 with sound 0x1F; 14 (the mage) casts a fixed sequence indexed by its remaining actions (eight effects, `gSpellSequenceModes` and `gSpellSequenceSounds`); 15 (drider) throws item template 0x3C as a flying item; 16 starts effect 0 with sound 0x55; 17 (the mind flayer) tries to paralyze every living member of the party with a mind blast (each with a saving throw, section 5.7); 21 (the last monster) rolls 1d4, casts one of three effects for a roll below 3, and otherwise strikes the first living member with a "cause serious wounds" hit of 2d8+1. Each use costs one action (unless unlimited) and starts a 5-update delay.

#### 5.6.6 Being hit, dying, spawning

**Damage.** [V] `DamageMonster` (08E6:0FBE) subtracts the damage from the hit points, stops fleeing, flashes the monster (an extra drawing pass with all colours but 0 remapped to 15), marks it attacked (so `ChasePartyIfAttacked` sends it after the party) and calls `KillMonster` at 0 or less. While several monsters are hit together (`DamageNearRowMonsters`, 08E6:16D6, for area spells and turn undead) the flashes and deaths are deferred to `FinishDeferredHits` (08E6:0E48). Spell damage goes through `DamageMonsterWithSave` (08E6:1634), which halves it (flag 1) or voids it (flag 2) on a successful saving throw by the monster's hit dice, and `MonsterResistsMagic`. `FindMeleeTarget` (08E6:15B4) chooses the monster a party member hits in the cell ahead: the one in the centre if there is one, else the first quadrant in an order that depends on the facing and on which side the attacker stands.

**Death and loot.** [V] `KillMonster` (08E6:0EC3) picks the position the drops land at (the monster's quadrant, or 1d4-1 if it is in the centre), copies the first drop item with a 50% chance (1d2 equal to 1) and the second always, hands the type's experience to the party (`AddPartyExperience`), moves the monster to cell 0 (it stays in its slot with hit points at or below 0; that is what "free slot" means), wakes a sleeping group if it was in mode 8, and, if it is type 21, shows the winner panel and shuts the game down. The drops are item templates of `ITEM.DAT` copied to a free item record (`CopyItemToFreeSlot`). The level timer's cull calls `KillMonster` too, so a monster removed to make room still pays its experience and drops its loot far from the party.

**Spawning.** [V] `SpawnMonster` (08E6:1505) returns without doing anything if the target cell already holds a monster, then fills the first slot with hit points at or below 0 (from slot 0). The four consecutive `SPAWN` instructions of the level 2 timer script aim at positions 0 to 3 of one cell (241 or 406), and level 7's two spawns aim at one cell too; since the first spawn makes the cell's count nonzero, only the first of such a group can succeed [V by reading `SpawnMonster`]. [?] Whether the original game really adds one monster where the script reads as a group of four was not run in the oracle.

### 5.7 Combat

Combat is not a mode of the game [V]. There is no battle screen, no initiative roll and no round counter. The dungeon view stays up, the map loop keeps running, and only two things start an action: a click or key of the player, and an entry of a table of timers. This section follows one blow from the click to the damage, then the monsters, the saving throws, death and experience. Spells are in 5.8; the view that draws the monsters is in 5.4 and the random number generator that feeds every roll is in 5.16.

#### 5.7.1 Real time, one clock

The clock is the 32-bit counter `gGameTicks` (ds:8492). The INT 1Ch handler (1D8F:0055, `src/E1D8F.ASM:76-91`) adds one to it for every timer interrupt, but only while bit 1 of `gTimerEnableFlags` (ds:849E) is set [V: `src/E1D8F.ASM:85-87`]. A second counter, `gTicks` (ds:848A), always runs and serves the busy waits (`WaitTicks`, 082C:0425). The rate is 18.2 ticks a second [V]: the sound driver reprograms the PIT to sixteen times that and chains to the BIOS handler on every sixteenth interrupt (`src/E1948.ASM:163-170` and `:651`), and the code itself counts seconds as 18 ticks (`DelaySecondsThenKey`, 082C:0B43; the ambient sound timer in `RunMapLoop`, 0ACD:0E8A). One tick is 1/18.2 s.

The game clock stops while a screen effect runs [V]. Bit 1 is cleared by the monster hit flash (`FlashMonsterHit`, 08E6:0DCA), the swing animation (`TryMonsterMelee`, 08E6:05EA), the damage flash over the view (`FlashViewDamage`, 0EF3:06E7), the particle bursts of the spells (`AnimateParticleBurst`, 0EF3:2A53; `AnimateConeOfCold`, 0EF3:2EBC), the loading of a level (`LoadLevel`, ovl3:0A7C) and the camp menu (`RunCampMenu`, ovl1:1ACD). Everything the game times by `gGameTicks` therefore waits for them, and everything it times by `gTicks` does not.

The map loop `RunMapLoop` (0ACD:0E8A) repeats forever: read one key or mouse event (`PollMapInput`), call `RunEffects`, redraw the view if `gViewDirty` is set, maybe play an ambient sound, and call `TurnUndead` (5.7.10). `RunEffects` (1414:04E7) walks the 16 entries of `gEffects` (ds:5BA8, 16 bytes each: handler, period, expiry, argument, active byte). An armed entry whose expiry has passed is called with its argument and, if it is still armed afterwards, re-armed for `gGameTicks + period`. The new expiry is taken from the current tick, not the old expiry, so a late call makes the entry drift. The period and expiry initial values are in the image; `LoadLevel` rewrites entries 2 to 5 for each level.

<!-- FIG cmb_effects -->

The loop has no sleep, so how often it runs depends on the machine, but nothing in combat depends on that: every duration is in ticks. The spell panel is a list of nodes handled inside the same loop (`HandleSpellList`, 0EF3:2929) [I], so the monsters go on acting while the player chooses a spell. The only pauses are the ones above.

The unit of most effects is `0x222` = 546 ticks, 30 seconds. The source comments call it a turn [V: `src/E0EF3.C`, `ApplyTimedCharEffect`]; the value is a half minute only if the interrupt really comes at 18.2 Hz, which it does by the chaining above.

#### 5.7.2 The party's attack

A member attacks by using the item in a hand. A click on a member's panel reaches `ClickHandSlot` (0ACD:0415) with the member and the hand (0 or 1, chosen by whether the click is in the upper or lower box, `gHandSplitY`). A click the dispatcher reports as a left-button press swaps the hand's item with the one on the pointer; any other click uses it [I: `gLeftButtonMode & 0x7F == 1`, `src/E0ACD.C:394`]. With the mouse disabled, key `0x17` does the same for the slot under the keyboard cursor (`HandleMapKey`, 0ACD:0D9D). `UseItemSlot` (0BD2:1001) refuses unless `MemberIs(who, 13)` holds (present, above 0 hp, not paralysed), the hand's busy bit is clear and the class may use the item, then dispatches on the item type's use kind, the low seven bits of the word at +0E of its `ITEMTYPE.DAT` record [V]:

| Kind | Items (type index) | What the click does |
|---:|---|---|
| 1 | axe 0, long sword 1, short sword 2, halberd 10, mace 11, flail 12, staff 13 | melee: `ApplyItemAttack` |
| 2 | dagger 5, spear 9, dart 15, arrow 16, rock 18 | the item itself is thrown |
| 3 | bow 7, sling 14 | a launcher: ammunition is found and shot |
| 5, 6 | spell book 29, holy symbol 30 | open the spell panel (5.8) |
| 9, 10 | mage scroll 34, cleric scroll 35 | cast the scroll's spell |
| 14, 18, 7 | potion, wand 48, food | drink, zap, eat (5.8.6; food only with the mouse disabled) |

So a dagger is only thrown, never stabbed with: kind 2 goes to `UseCarriedItem` (0DE8:0537), which calls `AddDDEffect` and refills the hand from the item slots 24, 23, 22. A launcher is not used up: `UseItemByKind` (0DE8:05BB) looks up the ammunition type in `gLauncherAmmoType` (ds:552F: bow to arrows, type 16; sling to rocks, type 18). Arrows are taken from the two hands first and then from the quiver (item slot 16, a list with a counter shown in the inventory); rocks from any of the 27 slots. The ammunition is the thing that flies. No ammunition: result -4.

<!-- FIG cmb_attack_flow -->

`ApplyItemAttack` (0DE8:0359) is the melee case. A member above 1 (members 2 to 5, the middle and rear ranks) cannot swing: result -3, drawn as CAN'T REACH [V]. The target comes from `FindMeleeTarget` (5.7.4). With no monster there, a wall of a type whose handler index is 255 has its wall byte raised on both sides, which is the HACK result (-2); otherwise the result is -1 (MISS). With a monster, `RollToHit` is made; a miss is -1. A weapon made by a spell (item types `0x34` to `0x38`) is also stopped by `MonsterResistsMagic`. The damage comes from `RollAttackDamage` and is halved by `IsWeaponDamageHalved`. `UseItem` (0DE8:0763) then turns a melee result of 0 into -1, applies the damage with `DamageMonster` (items with flag 8 also heal the wielder by the same amount, capped at 127; items with flag 16 are used up), stores the result in `lastAction[slot]` (the label on the hand: the damage, MISS, HACK, CAN'T REACH, NO AMMO, from `DrawActionLabel`, 1547:0642) and starts the cooldown.

The cooldown is not a round. It is built from timed effects (5.8.5). `UseItem` flips the hand's busy bit in `CharRec.m1E` and arms a type 2 or 3 effect for 18 ticks; when that fires it clears the label and arms type 4 or 5 for 36 ticks, or 18 if the member is hasted (effect flag 8, `RunCharEffects`, 1414:07CA); that one flips the busy bit back. A refused attack (result -3 or -4) arms type 0 or 1 for 18 ticks only [V]. Each hand has its own cooldown and a spell cast from the panel has none at all.

<!-- FIG cmb_cooldown -->

#### 5.7.3 To-hit and damage

The arithmetic is AD&D's with descending armour class, in whole numbers and one die.

`RollToHit` (0DE8:0007) computes `need = 20 - progress - AC - bonus` and succeeds when a d20 roll, kept between 1 and 20, is at least `need` [V]. The progress is `(level - 1) / levelsPerStep * gain`, with level the member's first class slot (`GetHitTarget`, 0A72:0522) and the two numbers from the combat group of the class (ds:116E, ds:3610, ds:3614). The AC is the monster type's `m00`. The bonus is the weapon's +N (`ItemRecord.value`) plus the strength to-hit bonus for a melee blow or the dexterity bonus otherwise. The roll gets +1 while Bless or Prayer is on (`gPartySpellFlags` bits 4 and 5, which do not add up, the test is `& 0x30`) and +1 while the member has Aid (effect flag `0x40`). The call also sets the monster's "attacked" flag, hit or miss.

<!-- FIG cmb_tohit -->

Three details are not what one would expect [V]. The weapon's +N is added to the to-hit only when the item's type index is 1, 2 or 3 (long sword, short sword, and an orb), not for the use kinds 1 to 3; the damage bonus is added for every weapon. A need above 20 can never be rolled, and a need of 1 or less always hits, so there is no automatic hit on a 20 for the party. The progress rates look swapped [I]: the four combat groups have exactly AD&D's four rates (2 per 3 levels, 1 per 2, 1 per level, 1 per 3), but the class to group map (ds:116E) gives the first to fighters, rangers and paladins, the second to mages, the third to clerics and the fourth to thieves, so a level-9 fighter needs 16 and a level-9 cleric 12 before AC. Nothing in the code looks wrong in isolation; whether the shipped game plays this way was not tested [?].

`RollAttackDamage` (0DE8:011A) adds three terms: the strength damage bonus (melee only, `GetStrengthDamageBonus`, 0A72:0128), the weapon's dice and the weapon's +N, and never goes below 0. A bare hand does 1d2. The dice come from the item type, in two sets: columns +07 to +09 (small) and +0A, +0B and +09 (large). The large set is used when bit 0 of the monster type's flag byte (`m0E`) is set, and no monster type has that bit, so it is never used in the shipped game [V]. Strength and its 18/xx percentile follow AD&D's table: score 17 gives +1 to hit, 18 +1, and the percentile thresholds {1, 51, 76, 91, 100} replace the base with +1, +2, +2, +2, +3 to hit and +3, +3, +4, +5, +6 to damage (ds:3430 to ds:345C) [H: AD&D 2nd edition's values; matches by inspection].

| Type | Weapon | Kind | Small dice | Large dice | Classes |
|---:|---|---:|---|---|---|
| 0 | axe | 1 | 1d8 | 1d10 | fighter, thief |
| 1 | long sword | 1 | 1d8 | 1d12 | fighter, thief |
| 2 | short sword | 1 | 1d6 | 1d8 | fighter, thief |
| 10 | halberd | 1 | 1d10 | 2d6 | fighter, thief |
| 11, 12 | mace, flail | 1 | 1d6+1 | 1d6+1, 2d4+1 | fighter, cleric, thief |
| 13 | staff | 1 | 1d6 | 1d6 | all four |
| 5 | dagger | 2 | 1d4 | 1d3 | fighter, mage, thief |
| 9 | spear | 2 | 1d6 | 1d8 | fighter, thief |
| 15, 16, 18 | dart, arrow, rock | 2 | 1d3, 1d1, 1d2 | 1d2, 1d1, 1d2 | by item |
| 7, 14 | bow, sling | 3 | 1d6, 1d4 | same | fighter, thief; fighter, cleric, thief |

(`ITEMTYPE.DAT` in `EOBDATA6.PAK`, 57 records of 16 bytes, read with `tools/re/pak.py`; "Classes" is the class mask at +05 against `gClassBaseBits`, ds:1231.) A shot arrow does the dice of the launcher, not of the arrow: `UseItemByKind` passes the launcher's type to `AddDDEffect`, and `RollAttackDamage` rolls that type's dice and adds the arrow's +N [V]. A shot or thrown weapon gets no strength bonus (`useStrength` 0 in `TryRecordAttack`, 0DE8:02D4).

Edged weapons do half damage to undead of the skeleton kind. `IsWeaponDamageHalved` (0D1C:0BEB) tests bit 2 of the monster's `m0E` and that bit 2 of the weapon type's class mask is clear; the class mask bit 4 is the cleric's, so the weapons halved are the ones a cleric may not use (swords, axes, spears, halberds, daggers, darts) [V, with the meaning inferred]. Only melee uses this test.

#### 5.7.4 Ranks and quarters

A map cell holds up to four monsters in its four quarters and one in the centre. The quarters are numbered 0 north-west, 1 north-east, 2 south-west, 3 south-east on the map, the centre 4 (`gMaze` +4 holds the count in its low three bits, `BuildCellPosMask`, 08E6:017E). The party stands in a cell in the same way: `gViewToCellPos` (ds:1A1B, four entries per facing) gives each of members 0 to 3 a quarter, and `gViewRelativePos` (ds:1A2B) is the inverse. Members 0 and 1 are the front rank, 2 and 3 the middle, and members 4 and 5 (the two characters that can join) are drawn at the quarters of 2 and 3 (`idx -= 2` in `UseCarriedItem`, 0DE8:0537), which makes the formation three ranks of two [I: the monster target lists below list members in the order 0 1, 2 3, 4 5].

<!-- FIG cmb_cellgrid -->

`FindMeleeTarget` (08E6:15B4) lists the monsters of the cell ahead by quarter (`ListMonstersByCellPos`), takes the monster in the centre if there is one, and otherwise the first of four quarters in `gTargetPosOrder` (ds:329B): the near half before the far half, and within a half the swinging member's own side first (even members the left, odd members the right). `DamageNearRowMonsters` (08E6:16D6, used by Burning Hands) hits the centre and only the two near quarters (`gNearPosPair`, ds:32BB), so a monster in the far half of the cell is out of its reach.

The same code serves the touch spells: `CastMeleeEffect` (0EF3:0E1B) refuses when `gInFrontRank` (ds:5717) is 0 for the casting member, "%s must be in the front ranks to hit!". The table has four bytes (1, 1, 0, 0); members 4 and 5 read the next bytes, which belong to another table (ds:571B, 3 and -3), so they count as front rank [V, a read past the table].

#### 5.7.5 Thrown weapons, ammunition and flying objects

A thrown item and a spell missile are the same kind of object: one of ten 18-byte records at ds:DD2C (`gProjectiles`, struct `Projectile`). The functions are named "DD effects" after the address (`AddDDEffect`, `UpdateDDEffects`, `StepDDEffect`); they have nothing to do with the timed effects of 5.8.5. `AddDDEffect` (0BD2:07DE) takes a free record (ten at most in flight), puts the item on the item list of the party's cell with the tag `quarter | 4` (bit 2 of the tag marks it as flying), and sets `active` 1, `m0D` 12 and the start quarter. A spell uses `StartCellEffect` (0EF3:097B), which sets `active` 2 and copies the spell's hit callback into `fn`.

Effect 1 of `gEffects` (`MoveDDEffects`, 1414:0285) runs every 3 ticks, which is 6 steps a second. Each step moves the record half a cell: `gNextPosByDir` (ds:5CAB) gives the next quarter and sets bit 7 when the step enters the next cell. `StepDDEffect` (1414:003E) refuses the step at a wall; an item then stops and the wall handler of that cell is run (`DispatchWallHandler`, 122E:0896; a thrown item can trigger wall switches [I]) and a missile with the flag `0x20` detonates on the spot. `TestDDEffect` (1414:0172) then tests the quarter entered: for an item, every monster in that quarter or the centre gets `TryRecordAttack` (0DE8:02D4: an attack roll with the thrower's THAC0 and dexterity, `RollAttackDamage`, `DamageMonster`), and the party's cell gets `RunRecordAttack` (0DE8:01FF) for objects thrown by monsters; for a missile, its callback runs. The record is cleared on a hit, a stop or when its budget is spent. A flying item gets 13 steps (about 6 cells); Lightning Bolt and Ice Storm get 7 (about 3 cells); the other missiles are unlimited.

<!-- FIG cmb_projectile -->

Two consequences [V]. Nothing destroys an item on a hit: `ClearDDEffect` (1414:020F) only clears the flying bit of the tag and runs the cell script for "item dropped" (mode 4), so thrown weapons and arrows end up on the floor of the cell where they stopped and can be picked up again. And while `m02` is 1, which lasts until the record has left the cell it was thrown from, `TestDDEffect` returns at once, so a thrower cannot hit himself.

#### 5.7.6 Monsters

Thirty monster records of 28 bytes live at ds:8A82 (`gMonsters`, struct `Monster`); the type of each indexes the 22 records of 27 bytes at ds:1A3B (`gMonsterTypes`). A level file lists 30 starting records (`LoadLevel` reads 14 bytes each and calls `InitMonster`, 08E6:131D). The hit points of a monster are `RollDice(HD, 8)`, or 1d4 for the type value -1. The type table follows. AC and THAC0 are read the way AD&D reads them: the first is the number the party's attack must beat, the second is the monster's own attack number [I, from the arithmetic of 5.7.3].

<!-- FIG cmb_montypes -->

Monsters move by group timers, not by the player's clock. A starting record carries a kind (0 or 1); the monster's update group is `kind * 2 + (slot index & 1)`, and group `g` runs when effect `2 + g` fires (`UpdateMonsters`, 08E6:0CC8). `LoadLevel` stores two periods from the level file (an (index, value) list) into the entries: kind 0 monsters use the first, kind 1 monsters the second, and the odd slots of each kind are offset by half a period (their first expiry is half a period away, the even slots' a full one), so a crowd does not move on one beat. The shipped levels use 15, 25 or 35 ticks:

| Level | Picture sets named in `LEVELn.INF` | Period of kind 0, kind 1 (ticks) | Script timer (steps) | Records |
|---:|---|---|---|---:|
| 1 | kobold, leech | 25, 35 | none | 24 |
| 2 | zombie, skeleton | 35, 25 | 80 | 27 |
| 3 | kuotoa, flind | 35, 25 | 100 | 29 |
| 4 | spider | 15, 25 | 150 | 15 |
| 5 | dwarf, spider | 25, 15 | 50 | 23 |
| 6 | kenku, mage | 25, 25 | 50 | 28 |
| 7 | drowelf, skelwar | 25, 25 | 50 | 29 |
| 8 | drider, hellhnd | 25, 15 | 50 | 28 |
| 9 | rust, disbeast | 35, 15 | 80 | 22 |
| 10 | shindia, mantis | 25, 15 | 80 | 14 |
| 11 | xorn, mflayer | 35, 25 | 50 | 22 |
| 12 | xanath, golem | 15, 35 | 50 | 8 |

(Decoded from `LEVELn.INF` in `EOBDATA3` to `EOBDATA6.PAK` with `tools/re/wwfmt.py`; the layout is the one `LoadLevel` reads. The record count is those with a nonzero cell.) The script timer fires `RunCellScript(0, 0x20)` after that many party steps (`TickLevelScriptTimer`, 08E6:1410) [V]; what the scripts do with it belongs to the script chapter, but note that the function also kills a monster more than 3 cells away when no monster slot is free, to make room for a new one. A monster update takes 15 to 35 ticks (0.8 to 1.9 s); its melee swing comes on every second update.

<!-- FIG cmb_monster_loop -->

`AlertMonsterToParty` (08E6:00C3) wakes a monster (mode 0, target the party's cell) when the party is within 3 cells and not behind it, or within 1 cell; `ChasePartyIfAttacked` does the same for a monster the party has hit. Modes 1 to 3 walk ahead and turn when blocked (`StepMonsterOrTurn`), 5 and 6 follow a wall to the left or right, 4 stand, 8 sleep as a group until one is attacked or dies (`AlertMode8Monsters`), 9 never wake, 7 and 10 wait for a timer. A hunting monster (mode 0, `UpdateHuntingMonster`, 08E6:0AB8) first tries a ranged attack (`TryMonsterRangedAttack`, 08E6:0765): only types with flag 64, with actions left, five updates after the last one, within 3 cells and straight ahead with open cells in between. The ranged actions of the types are fixed by the type:

| Type | Action |
|---:|---|
| 4 | cell effect 9: 12 damage on one member, halved |
| 14 | a sequence by actions left (8 to 1): Ice Storm, Lightning Bolt, Fireball twice, Magic Missile four times |
| 15 | throws a copy of item 60 |
| 16 | Magic Missile |
| 17 | Mind Blast: every present member must save (kind 1) or is paralysed for 5 * 546 ticks |
| 21 | one die: Fireball, "disintegrate" (one member for 100), "death spell" (up to 1d4 members below level 8, 100 each), or cause serious wounds on the first member |

A monster-cast spell counts as caster level 5 on dungeon levels 1 to 6 and 9 from level 7 on (`GetCasterLevel` with member -1, 0EF3:0498) [V]. If there is no ranged action, `TryMonsterMelee` (08E6:05EA) fires when the cell ahead is the party's: the monster must stand in the half of its cell it faces (`IsMonsterAtFront`, 08E6:103D) or in the centre, otherwise it moves to another quarter. Otherwise `StepMonsterToward` takes one step toward the target cell.

`ResolveMonsterMelee` (08E6:10F3) picks the victim. The side the monster attacks from is `gAttackSideByDir[gPartyDir * 4 + facing]`; each side has 12 bytes in `gAttackTargetOrder` (ds:325B), two lists of six members, and the half is chosen by the quarter the monster stands in (`gRightSidePosByDir`, ds:328B; a monster in the centre rolls 1d2). The first member on the list who is present, above -10 hp and, if invisible, wins a 1d20 below 5, is the victim; if no member qualifies the swing does nothing.

<!-- FIG cmb_monster_target -->

For each of the type's attacks (1 to 3), `RollMonsterToHit` (08E6:106B) rolls a d20: `need = THAC0 - AC of the victim`; a roll of 20 always hits; otherwise Protection from Evil subtracts 2 and Prayer 1 from the roll. Each hit adds its dice; while the victim's Stoneskin counter is above 0 a hit also lowers the counter and sets the running total back to 0 (it throws away the damage of the earlier hits of the same swing too) [V]. The total goes to one `DamageCharacter`. Type flags then add an effect: 128 destroys one item of the victim that has the metal bit (bit 7 of its `m0E`: armour, shields, swords), starting from a random slot (`ReportLostItem`, 0BD2:000E); 16 poisons and 32 paralyses unless the victim saves (kind 0 and kind 2). Type 18 (flag 128, one attack of 1d1) is the metal-eating monster; it is on level 9, whose file names a "rust" picture set [I].

Two flags on the monster show what the party did to it: flag 1 (attacked) and flag 8 (fleeing, set by Fear and by turning). `DamageMonster` (08E6:0FBE) clears flag 8, sets flag 1, flashes the sprite and calls `KillMonster` (08E6:0EC3) when the hit points reach 0. Nothing but a hit clears flag 8, so a frightened monster runs until it is damaged [V].

#### 5.7.7 Saving throws

`GetSavingThrowTarget` (0A72:0006) looks the number up: `table[kind * columns + min(level, cap) / levelsPerColumn]`, with one table per base class, and a d20 at or above the number is a success. The tables (ds:3406) are AD&D's; paladins and rangers use the fighter table, and monsters use it with their hit dice as the level and kind 4.

<!-- FIG cmb_saves -->

Monsters save in `DamageMonsterWithSave` (08E6:1634, the damage of Burning Hands), `HalveSaveDamageB` (0DE8:0FCA, the missiles) and Fear. The party saves in `RollMemberSave` (0D1C:059B, returns 1 on success), used by `ApplyStatusEffect` (poison, paralysis) and `HalveSaveDamageA` (0DE8:0F98). Three slips are in the party side [V]. `HalveSaveDamageA` takes the 0 or 1 that `RollMemberSave` returns as a threshold and tests a d20 against it, which always passes, so a member always takes half damage from a save-for-half effect and never full. `DamageOneMember` (0EF3:20A8, the "disintegrate") asks `HalveSaveDamageA(who, 1)` for its save, which returns `1 >> 1 = 0` every time, so the first eligible member is always destroyed. And the Constitution bonus meant for dwarves, halflings and gnomes tests `race * 2` against 3, 5 and 4 where it should test `race / 2` [I: the pattern is AD&D's racial saving throw bonus, race index 6 to 11 being dwarf, gnome and halfling in male and female]; the only race index with `race * 2 == 4` is 2, the male elf, so the bonus goes to male elves for kinds 4 and 1 and to nobody else. The tenth column of the fighter table cannot be reached: the cap in `gSaveMaxLevel` is 17 and 17 / 2 is 8.

#### 5.7.8 Hit points, unconsciousness and death

Hit points are a signed byte (`CharRec.hp`, +1B; `maxHp` +1C). `DamageCharacter` (0DE8:09B9) subtracts the damage, stores it in `lastDamage` (the number drawn on the portrait, cleared after 18 ticks by a type 6 effect), plays sound `0x15`; when hp reaches -10 or less it instead sets it to -10, clears the invisibility flag, keeps only the "present" bit of the status byte and plays `0x16`. A member at 0 to -9 is unconscious: `MemberIs` fails for acting, the member is still a target and still heals, and nothing makes it bleed; at -10 it is dead, `HealCharacter` (0DE8:0ADB) ignores it, and only Raise Dead (5.8) brings it back [V]. The armour-absorbing Armor spell counts its points down here (`CharRec.mDF`, 5.8). A second pool, `mE2`, holds the extra hit points of Aid and is spent first, but its floor at 0 is dead code: the test `&rec->mDF < 0` compares a far pointer with 0 and is always false, so a hit larger than the pool leaves `mE2` negative and, when Aid ends, `EndAid` subtracts that negative number, a small bonus to hit points [I].

`CheckPartyWiped` (0DE8:093B) runs only when the member who just took damage has died: if no member is present with hp above 0, it shows "All of your party has been defeated" and ends the program after a key. There is no retry and no menu [V]. A party that is entirely unconscious but not dead is not wiped until someone dies.

<!-- FIG cmb_states -->

`ComputeArmourClass` (0DE8:0D5E) gives the AC a monster must beat: 10 plus the dexterity word (ds:349E, from +4 at dexterity 3 to -5 at 20), plus the armour type's byte (a negative number, leather -2 to plate -7) minus the item's +N, with the cap of Armor (dexterity word + 6, from `mDF`), Shield (4) and Magical Vestment (5 + (level - 5) / 3 for cleric levels over 5) applied after.

#### 5.7.9 Experience and levels

A kill ends in `KillMonster`, which drops the monster's items on its cell (the first with 1 chance in 2, the second always), gives the party the type's experience value, and, for type 21, shows the winner panel and ends the program [V]. `AddPartyExperience` (0DE8:0C2B) divides by the number of members that pass `MemberIs(n, 3)`, so an unconscious member shares and a dead one does not; `AddExperience` (0DE8:0B45) divides again by the number of the member's classes (ds:115F) and adds the share to each slot's total. A slot levels up when its total is greater than the threshold in the table for its base class (`GetClassSlotExpThreshold`, 0A72:04B7), one level per award, even when the total passed two thresholds. `LevelUpSlot` (0DE8:0C82) adds one hit die of the slot plus the Constitution bonus (capped at +2 unless the class's first die is d10) to the hit points and the maximum, divided by 2 or 3 for multi-class members (`RollHitPoints`, 0A72:01AE), so a fighter/mage gains `(1d10 + CON bonus) / 2` per fighter level.

<!-- FIG cmb_xp -->

#### 5.7.10 Turning undead

There is no command. `TurnUndead` (0D1C:0A43) is called on every pass of the map loop and acts by itself [V]. It works only on dungeon levels 2 and 7, which are the levels whose files name zombie, skeleton and skelwar [I]. When the cell ahead holds monsters, the party's best turner is looked up (`FindBestUndeadTurner`, 0D1C:0903: a member who can act, holds an item of kind 6, a holy symbol, in a hand, and has a cleric slot, or a paladin slot with its level less 2; the highest level wins). One d20 is rolled for the cell, and each monster whose `m0F` has bit 2 (undead) and that has not been tried (flag `0x10`, set here and never cleared) gets one `TryTurnMonster` (0D1C:09CB): `GetTurnUndeadTarget(level, row)` with the monster type's row `m19`. Zero turns it at once, -1 destroys it, 99 means never, anything else needs that d20. A turned monster gets flag 8 and a facing away from the party and runs until hit; the message "%s uses the power to turn undead!", sound `0x5F` and a flash follow. Each undead therefore gets exactly one attempt for the rest of the level.

| Monster type (row) | L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8 | L9 | L10 | L11 up |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 2 (row 2) | 13 | 10 | 7 | 4 | turn | turn | destroy | destroy | destroy | destroy | destroy |
| 3 (row 3) | 16 | 13 | 10 | 7 | 4 | turn | turn | destroy | destroy | destroy | destroy |
| 8 (row 7) | never | never | 20 | 19 | 16 | 13 | 10 | 7 | 4 | 4 | turn |

(ds:3618, ten rows of 14 bytes, column = the turner's level. The column is clamped to 14, which reads the first byte of the next row; QUIRK 0A72:0597.)

#### 5.7.11 The dice and the feedback

`RollDice(count, sides, bonus)` (082C:0458) sums `count` rolls of `rand() * sides / 32767 + 1` and adds the signed byte `bonus`; the one overflow value `sides + 1` (only when `rand()` returns 32767) is folded back to 1. A zero count or side gives just the bonus. All three parameters are signed bytes, so a bonus of `0xFF` is -1 and `RollDice(1, 4, 0xff)` is a 0 to 3. The generator is the Borland `rand` (5.16). `RandomRange` (1AAA:0005) is the same scaling between two bounds.

The player sees the fight through four pieces of feedback [V]: the label on the hand (damage, MISS, HACK, CAN'T REACH, NO AMMO), the damage number under the portrait of a member that was hit, the white flash of a monster sprite for one frame (`FlashMonsterHit`: every colour but 0 mapped to 15) and the red burst over the view (`FlashViewDamage`, eleven passes of two ticks) or the eight-pass flash over the members (`FlashPartyMembers`, 0EF3:0665). The deferred form (`gDeferMonsterHits`) lets an area attack mark several monsters and draw all their flashes and deaths together (`FinishDeferredHits`, 08E6:0E48).

### 5.8 Spells and effects

The game has 49 named spells (44 with an effect, 5.8.7): 25 for mages and 24 for clerics, of which paladins can use 10. They are cast through one routine; their effects are small handlers in `src/E0EF3.C`. Everything that lasts, from a spell to the pause after an attack, goes through one mechanism, the timed slots of a member (5.8.5).

#### 5.8.1 Books, slots and memorising

A character record holds three spell fields [V, `struct CharRec`]: `mageSpells[30]` (+37) and `clericSpells[30]` (+55), five rows of six signed bytes, one row per spell level, and `spellBook` (+73), a long with bit `i` set when the mage knows spell `i + 1`. A positive byte is a spell ready to cast; a negative byte is the same spell spent or not yet memorised; 0 is an empty slot. A spell is a number: mages 1 to 25, clerics 1 to 24, in the order of the figure below. The index into the table of spell records `gSpells` (ds:1344) is the number, plus 25 for a cleric.

<!-- FIG cmb_spells -->

How many slots a member has comes from `SpellSlotScreen` (ovl1:097A), the "Memorize Spells" and "Pray for Spells" screens of the camp menu. A mage's allowance by level is `gMageSpellsPerLevel[(level - 1) * 5 + i]` (ds:6394); a cleric's is `gClericSpellsPerLevel` (ds:62FE) plus, for each level that already has a slot, `gClericWisdomBonus` (ds:6330) by wisdom - 12; a paladin's is `gPaladinSpellsPerLevel` (ds:6358) indexed by the level itself, not level - 1, and can only be used from level 9 on (`SelectPartyMember` refuses below). The ring of wizardry (item type `0x2F`, value 1) doubles the allowance for spell levels 4 and 5.

| Class level | Mage slots (levels 1 to 5) | Cleric slots | Paladin slots |
|---:|---|---|---|
| 1 | 1 | 1 | none |
| 2 | 2 | 2 | none |
| 3 | 2 1 | 2 1 | none |
| 4 | 3 2 | 3 2 | none |
| 5 | 4 2 1 | 3 3 1 | none |
| 6 | 4 2 2 | 3 3 2 | none |
| 7 | 4 3 2 1 | 3 3 2 1 | none |
| 8 | 4 3 3 2 | 3 3 3 2 | none |
| 9 | 4 3 3 2 1 | 4 4 3 2 1 | 2 |
| 10 | 4 4 3 2 2 | 4 4 3 3 2 | 2 1 |
| 11 | 4 4 4 3 3 | (none) | 2 2 |

(Wisdom 13 to 19 add 1 to 3 first-level slots and a few more at the higher levels, up to 3 2 1 2 at 19.) The mage can choose only spells whose bit is in the spell book; the cleric's list is `0x7BFFFF` (all but Protection from Lightning and Lay on Hands), the paladin's `0x800FF2`. A new mage starts with the book value `0x26C`: Detect Magic, Magic Missile, Shield, Shocking Grasp and Melf's Acid Arrow (`GiveStartingSpellBooks`, ovl0:280D). Spells are added to the book by scribing scrolls (`ScribeScrolls`, ovl1:1691).

Choosing spells does not make them ready. `SpendSpellSlot` (ovl1:025D) writes the choice back: a copy that was ready before stays positive, a new copy is stored negative, and the camp loop turns negative slots positive: it adds one to a counter per member each sub-step (six a rest hour), and a negative slot of spell level `n` is turned positive when the counter is above 48 + (n - 1); after each one the counter restarts at 48. So the first spell after a change costs about 8 hours (49 sub-steps), and every later one `n` sub-steps. Mages must hold a spell book (item type 29) to memorise (`CampLoop`, ovl6:0000). A cleric with spare Cure spells uses them on the most wounded member during the rest (spells 2, 15 and 20, 1d8, 2d8+1 and 3d8+3, "%s casts healing on %s.") and every 8 hours of rest a paladin (class index 2) gets Lay on Hands, spell 24, back in a first-level slot (`CampLoop`; the source comment says "class-2") [V].

#### 5.8.2 The spell table

`gSpells` (ds:1344) has 53 records of 19 bytes (struct `Spell`): the name (a far pointer), a flag byte, three far callbacks, a sound, and a second flag byte. Index 0 is empty, 1 to 25 are the mage spells, 26 to 49 the cleric spells, and 50 to 52 have no name: they are the three monster-only effects of 5.8.4. The callbacks are `apply` (+05, the cast), `ddFn` (+09, the hit of a missile) and `remove` (+0D, the end of a timed effect). The first flag byte (+04) says how the cast goes:

| Bit | Meaning | Spells |
|---:|---|---|
| 1 | needs a target member, asked after the cast | Armor, Invisibility, Dispel Magic (both), Stoneskin; Cure Light, Serious and Critical Wounds, Protection from Evil, Aid, Slow Poison, Neutralize Poison, Raise Dead, Lay on Hands |
| 2 | needs a free hand and creates a weapon there | Shocking Grasp, Vampiric Touch, Flame Blade |
| 4 | redraw the member | Shield, Magical Vestment, Protection from Lightning |
| 8 | the whole party: flash and redraw all | Detect Magic, Haste, Invisibility 10', Bless, Create Food, Prayer, Remove Paralysis, Protection from Evil 10' |

The flag +12 bit 0 marks the spells that count as an attack: they end the caster's invisibility (`CastSpell`). In the table below the level is in the figure above; "level" in a formula is the caster's level in the class of the spell (`GetCasterLevel`, 0EF3:0498: the mage level; 9 when cast from an item; 1 if the member has no mage class) or, for clerics, the cleric level or the paladin level minus 8 (`GetClassSlot24Value`, 0EF3:04F7). A unit "30 s" is 546 ticks.

| # | Spell | Handler and effect | Lasts, ends by |
|---:|---|---|---|
| 1 | Armor | `CastArmor`: if AC is above the dexterity word + 6, `mDF` = level + 8 and AC is capped at the dexterity word + 6 | until `mDF` points of damage are taken (`DamageCharacter` calls `EndArmor`) |
| 2 | Burning Hands | `CastBurningHands`: 1d3 + 2 * level to the centre and near quarters of the cell ahead, monsters save for half | instant |
| 3, 29 | Detect Magic | `CastDetectMagic`: `gPartySpellFlags` bit 0, the icons of magic items are drawn recoloured | 60 s, `EndDetectMagic` |
| 4 | Magic Missile | missile mode 0: (1d4 + 1) * max(1, (level - 1) / 2), no attack roll | on hit |
| 5, 9 | Read Magic, Knock | the record is all zero; casting would call a null far pointer [?] | |
| 6 | Shield | effect flag 2: AC at most 4 | 150 s, `EndShield` |
| 7 | Shocking Grasp | temporary weapon in a free hand, 1d8 + level, used up by its first hit | 30 s |
| 8 | Invisibility | effect flag 4 | until the member attacks, casts an attack spell or is dispelled |
| 10 | Melf's Acid Arrow | mode 1: 2d4 * max(1, level / 3), attack roll | on hit |
| 11 | Stinking Cloud | `CastStinkingCloudNoOp`: nothing | |
| 12, 36 | Dispel Magic | `CastDispelMagic`: runs every `remove` callback of the target's timed slots, clears them, `mDF` and all effect flags | instant |
| 13 | Fireball | mode 2: 1d6 * level to every monster in the cell, save for half | on hit or at a wall |
| 14 | Flame Arrow | mode 3: 5d6 * level, attack roll, save for half | on hit |
| 15 | Haste | every present member: effect flag 8, halves the cooldown after an attack | 120 s |
| 16, 33 | Hold Person | mode 4: monsters of the cell with flag H1 and no magic resistance go to mode 10 with timer 15 | 15 monster updates |
| 17 | Invisibility 10' | effect flag 4 on all members | as Invisibility |
| 18 | Lightning Bolt | mode 5: 1d6 * level, no attack roll, the monster saves for half, one monster per half step; it passes through monsters, range about 3 cells | 7 steps or a wall |
| 19 | Vampiric Touch | weapon: (level / 2)d6, the damage also heals the wielder (item flag 8), used up by its first hit | 30 s |
| 20 | Fear | monsters of the cell with H bits that fail their save flee (flag 8) | until hit |
| 21 | Ice Storm | mode 6: 1d6 * level to the cell and its four neighbours when the first hits | on hit |
| 22 | Stoneskin | `mE0` = 1d4 + level / 2 hits turned away (named "bandage" in the code) | until used |
| 23 | Cloudkill | `CastCloudkillNoOp`: nothing | |
| 24 | Cone of Cold | (level)d4 + level on every monster of a 7-cell cone, no save | instant |
| 25 | Hold Monster | mode 7: as Hold Person for the H2 monsters | 15 monster updates |
| 26 | Bless | `gPartySpellFlags` bit 4: +1 to every party attack roll; cannot be cast twice | 180 s, `EndBless` |
| 27, 40, 45 | Cure Light, Serious, Critical Wounds | `HealCharacter` by 1d8, 2d8+1, 3d8+3 | instant |
| 28, 41, 46 | Cause Light, Serious, Critical Wounds | `CastMeleeEffect`: touch the monster ahead, front rank only, attack roll, 1d8, 2d8+1, 3d8+3, magic resistance applies | instant |
| 30 | Protection from Evil | effect flag `0x200`: monsters' attack rolls at -2 | 90 s |
| 31 | Aid | 1d8 temporary hit points (`mE2`), effect flag `0x40`: +1 to attack rolls | 60 s, `EndAid` takes the hit points back |
| 32 | Flame Blade | weapon 1d4 + 4, not used up by hits | (4 + level / 2) * 30 s |
| 34 | Slow Poison | only on a poisoned member: effect flag `0x80`, poison does no damage | (1 + level) * 30 min |
| 35 | Create Food | food of all six members set to 100 | instant |
| 37 | Magical Vestment | effect flag `0x100`: AC at most 5 + (level - 5) / 3 for levels over 5; the cap gets worse with level, which looks inverted [I] | 150 s |
| 38 | Prayer | `gPartySpellFlags` bit 5: +1 on the party's attack rolls, -1 on monsters' | 60 s, `EndPrayer` |
| 39 | Remove Paralysis | clears paralysis on up to four members | instant |
| 42 | Neutralize Poison | clears poison (the test is the whole status byte, so the "no effect" message never shows for a present member) | instant |
| 43 | Protection from Evil 10' | effect flag `0x200` on all members | level * 5 min |
| 44 | Protection from Lightning | `CastProtFromLightningNoOp`: nothing | |
| 47 | Flame Strike | mode 8: 6d8 on every monster of the cell, save for half | on hit |
| 48 | Raise Dead | member at exactly -10 hp and not an elf (race index 2 or 3): hp = 1 | instant |
| 49 | Lay on Hands | heals twice the paladin's first class level | instant |

The Hold spells reach monsters only when the type allows it (`m0F` bits 0 and 1) and only after `MonsterResistsMagic` (a 1d100 roll below the type's resistance, 0D1C:0BAB) lets them through; no saving throw is made. The same resistance check protects against every damage spell except Burning Hands' save, and against weapons made by spells.

#### 5.8.3 Casting

The casting path is one routine for the spell panel, scrolls and wands.

<!-- FIG cmb_castflow -->

`OpenSpellPanel` (0EF3:21E0) is entered when the member uses a spell book or a holy symbol (kinds 5 and 6) and nothing else is open. It points `gNameTable` and `gSpellBookSlots` at the mage or cleric lists, selects the first ready slot, saves the screen under the panel and draws it (`DrawSpellPanel`, 0EF3:2295: five level tabs, six slot rows, an ABORT row). `SelectSpellLevel` (0EF3:25BD) is the callback of the five level tabs. `HandleSpellList` (0EF3:2929) is the callback of the list nodes: node 6 steps the highlight through the ready slots, nodes 0 to 5 pick a slot and 7 is ABORT; a click on a ready slot casts the spell it holds. `CastSpell` (0EF3:2676) first remembers the spell and its callback, ends the invisibility of the caster for an attack spell and, for the spells that make a weapon, refuses with "You must have a free hand for this spell." when both hands are taken, and with "You can't have two of this spell type active." when a hand already holds a spell weapon. Only then is the spell paid for: a panel cast shifts the row left and stores the negated number at the end (the slot is spent); a scroll is used up and a wand loses a charge (`UseItemRow`, 0EF3:2604). It prints "%s casts %s.", and if the spell needs a target it asks "Cast spell on which character?" and returns; the next click on a portrait runs `CastSpellOnTarget` (0EF3:08CF), which does the common tail for that member. The tail plays the spell's sound, flashes the caster (flag bits 2 and 4) or the party (bit 8), prints "ok" and calls the handler.

Scrolls and wands come through `StartItemCast` (0EF3:28ED): scroll value 1 to 25 is a mage spell, 26 to 49 (cleric scroll value + 25) a cleric spell, and a wand's value goes through `gWandSpells` (ds:38FA).

| Wand value | Name (`gWandNames`) | Spell cast |
|---:|---|---|
| 1 | Lightning | Lightning Bolt |
| 2 | Frost | Cone of Cold |
| 3 | Curing | Cure Serious Wounds |
| 4 | Fireball | Fireball |
| 5 | Silvias | no spell: every monster in the cell ahead is moved one cell back, if that cell is free |
| 6 | Magic Missile | Magic Missile |

Wands and scrolls cast at level 9 whatever the holder is (`GetCasterLevel` and `GetClassSlot24Value` return 9 while `gItemCastHand` is set), but only for what the cast itself computes (Burning Hands, Cone of Cold, healing, durations, weapon dice, Armor). A flying missile takes its level when it hits (`GetCasterLevel(rec->m0B)` in the `Hit*` callbacks); `EndItemCast` has cleared `gItemCastHand` by then, so a wand or scroll of Magic Missile, Fireball or Lightning Bolt (and a scroll of Acid Arrow, Flame Arrow or Ice Storm) uses the holder's own mage level, or 1 for a non-mage [V by code, not run]. When an item cast finishes, `EndItemCast` (0EF3:026F) puts the hand through the same 18 + 36 tick cooldown as an attack.

#### 5.8.4 Cell effects: missiles and monster spells

The spells that fly (Magic Missile, Acid Arrow, Fireball, Flame Arrow, Hold Person and Monster, Lightning Bolt, Ice Storm, Flame Strike) call `StartCellEffect` with an effect mode. `gEffectModes` (ds:56E7, four bytes each) maps the mode to the spell whose `ddFn` is the callback, a step budget and a flag byte. When the missile reaches a monster (or a wall, for modes with bit `0x20`) the callback calls `DamageTargetCell` (0EF3:0A3F), which reads the flags:

| Bit | Meaning |
|---:|---|
| `0x01` | one target, chosen by quarter (`FindMonsterNearPos`, 08E6:1775) |
| `0x02` | every monster of the cell; every present member of the party |
| `0x04` | an attack roll is needed (`RollToHit` for a member caster, `RollMonsterToHit` against the party) |
| `0x08` | the target saves for half |
| `0x10` | hits also when cast by a monster or a script |
| `0x20` | at a wall the missile still tests its own cell, else bursts |
| `0x40` | play the burst animation on a hit |

| Mode | Spell | Flags | Damage |
|---:|---|---|---|
| 0 | Magic Missile (4) | 41h | (1d4+1) * max(1, (L-1)/2) |
| 1 | Melf's Acid Arrow (10) | 04h | 2d4 * max(1, L/3) |
| 2 | Fireball (13) | 7Ah | 1d6 * L |
| 3 | Flame Arrow (14) | 0Ch | 5d6 * L |
| 4 | Hold Person (16) | 4Ah | none; monsters held |
| 5 | Lightning Bolt (18) | 38h | 1d6 * L per half step, not stopped by a hit |
| 6 | Ice Storm (21) | 7Ah | 1d6 * L, then its four neighbours |
| 7 | Hold Monster (25) | 4Ah | none; monsters held |
| 8 | Flame Strike (47) | 4Bh | 6d8 |
| 9 | (50) | 09h | flat 12, halved |
| 10 | (51) | 09h | one member for 100 ("disintegrated") |
| 11 | (52) | 09h | 1d4 members below level 8 for 100 ("death spell") |

Modes 9 to 11 have records in `gSpells` with no name and exist only for monsters (5.7.6) and level scripts (`LaunchProjectileOp`, 122E:0C4D, which can also throw items as a trap). Three details of `DamageTargetCell` [V]. The damage is `RollDice(count, sides, bonus) * n` where n is the level: one roll, multiplied, not n rolls (so the mean is the same as Ld6 but the spread is larger). The product is stored in a signed byte: Flame Arrow at level 5 can already pass 127 and wrap negative, and a negative damage passed to `DamageMonster` raises the monster's hit points [I: the store is `mov byte ptr [bp - 1], al` at 0EF3:0AF8; a simulation of 5d6 times the level puts the wrap chance at 2% at level 5, 15% at 6, 40% at 7 and about 70 to 90% from level 8 on; whether the game shows it was not run [?]]. And a missile that hits the party rolls its attack as monster number 0 (`RollMonsterToHit(gMonsters, i)`), whoever fired it.

#### 5.8.5 Timed effects: the slots

Each member has ten timed slots: a type byte in `mD5[10]` (+D5) and a 32-bit absolute expiry tick in `timers[10]` (+AD); a time of 0 is an empty slot. One entry of `gEffects` per member (6 + who) watches the earliest time.

<!-- FIG cmb_slots -->

`ApplyCharEffect` (1414:05ED) arms a slot: it computes `gGameTicks + interval`, finds the slot, and pulls the member's entry forward if the new time is earlier. A slot of the same type is reused, so casting the same spell again refreshes it instead of stacking. When the entry fires, `RunCharEffects` (1414:07CA) clears each expired slot and dispatches on its type. A negative type `-n` is the end of spell `n`: `ExpireSpellEffect` (0EF3:0D78) prints "%s's %s spell expires.", calls `gSpells[n].remove` and redraws the member or all members by the spell's flags. The positive types 0 to 6 are the hand cooldown and the damage number (5.7.2, 5.7.8); 7 is the end of Giant Strength (the strength score returns to `baseStrength`); 8 and 9 are poison and paralysis. The times are saved in a game relative to the clock and restored on loading (`AgeCharTimers`, ovl3:123D; `AdvanceCharTimers`, ovl3:12F1; then `RearmCharEffects`, 1414:0B2E). If all ten slots hold other types, `ApplyCharEffect` writes slot -1: `timers[-1]` is the last two item slots (the two ring slots, items 25 and 26, which end where `timers` begins) and `mD5[-1]` the top byte of `timers[9]` [V for the arithmetic, QUIRK 1414:0755]; it takes ten different effects on one member at once to get there.

<!-- FIG cmb_durations -->

Effects that need no slot are flags in `CharRec.effectFlags` (+E3, a long) set by the handler and cleared by the matching `remove`:

| Flag | Set by | Effect |
|---:|---|---|
| `0x02` | Shield | AC at most 4 |
| `0x04` | Invisibility, potion | monsters pick the member only on a 1d20 below 5 |
| `0x08` | Haste | halves the second part of the attack cooldown |
| `0x40` | Aid | +1 on attack rolls |
| `0x80` | Slow Poison | poison ticks do no damage |
| `0x100` | Magical Vestment | AC cap |
| `0x200` | Protection from Evil | monster attack rolls at -2 |

The party-wide ones are bits in `gPartySpellFlags` (ds:FBD2): `0x01` Detect Magic, `0x10` Bless, `0x20` Prayer.

#### 5.8.6 Status, healing and items that act

Poison and paralysis are set by `ApplyStatusEffect` (0DE8:101A): unless the member already has the bit (when asked to skip) or passes `RollMemberSave`, it sets the bit in `status`, arms a type 8 or 9 slot for `count * 546` ticks and prints "%s is %s!". Poison arms 1 * 546 ticks, and the tick re-arms itself for as long as the bit is set, doing 5 damage each time unless Slow Poison is on; Neutralize Poison and the Cure Poison potion end it by clearing the bit (`ClearPoisonEffect`, 0EF3:1CE8), and the next tick finds the bit clear and stops. Paralysis is 5 * 546 = 2730 ticks, "%s is no longer paralyzed!", or Remove Paralysis.

`HealCharacter` adds up to `maxHp` and ignores the dead; Aid is the only way above the maximum. The potion table (`ApplyItemEffect`, 0BD2:0D2D; names `gPotionNames`): value 1 Giant Strength sets strength to 22 with no percentile for (1d4 + 4) * 30 s, 2 Healing 2d4+2, 3 Extra Healing 3d8+3, 4 Poison (a poisoning with a save), 5 Vitality food 100, 6 Speed only prints "fast and agile", 7 Invisibility sets flag 4, 8 Cure Poison. The speed potion does nothing but flash [V]. The four rings (`gRingNames`): Adornment, Wizardry (doubles the allowance of levels 4 and 5), Sustenance and Feather Fall (the script operation that damages the whole party skips a wearer, `DamagePartyMember`, 122E:12CE).

The spell weapons (Shocking Grasp, Vampiric Touch, Flame Blade) are made by `CreateItemType` and `CreateItemRecord` (0EF3:035A, 0418): one of six scratch item types (`0x33` to `0x38`, ds:AD20) and one of six scratch item records (11 to 16); the weapon sits in a hand, is used like any melee weapon, and the timed slot ends it (`DiscardEquippedItems`, 0EF3:1233). `ApplyItemAttack` tests for the magic resistance of spell weapons with `m02 > 0x33`, which leaves out the first scratch type, `0x33`, an off-by-one [I].

#### 5.8.7 What the code does that the data would not suggest

- Stinking Cloud, Cloudkill and Protection from Lightning have an empty handler: casting them spends the slot and does nothing (Protection from Lightning is not offered to clerics). Read Magic and Knock have no record at all.
- The Speed potion does nothing but print.
- The four combat saving-throw and race slips of 5.7.7, and the always-false clamp of `mE2` in 5.7.8.
- The signed byte of `DamageTargetCell`, and the use of monster 0 for to-hit against the party.
- Lightning Bolt is not stopped by a hit: its callback returns 0 after a hit (`rec->m08 == 5`), so it goes on to its seventh step or a wall and can strike a monster in the centre of a cell twice.
- The Magical Vestment cap gets worse as the cleric levels up.

### 5.9 Camp, rest and food

#### 5.9.1 The camp menu

The camp button of the main screen calls `RunCampMenu` (ovl1:1ACD, `src/EOV1.C`) as the callback of its screen node. It clears bit 1 of `gTimerEnableFlags` while it runs, which stops the INT 1Ch handler from counting `gGameTicks` (`src/E1D8F.ASM`), so the game clock does not move in the menu. [V]

<!-- FIG chr_campmenu -->

Three button lists exist (`gMenus`, `ds:628C`, with the button definitions at `208E:0000`): Camp with seven buttons, Game Options with five and Preferences with five. Other lists serve the member picker (`SelectPartyMember`, ovl1:0416, header message 17h, 1Ah, 31h or 35h), the spell screen, the yes/no box and the scroll list. All strings are one table of NUL-terminated messages at `2003:0000`. [V]

`SelectPartyMember` offers only members who are present, above 0 hit points, have food, and whose class bit fits the action (mage bit for Memorize and Scribe, cleric bit for Pray); a paladin at level 8 or below is refused with "too low a level for spells". A mage needs a spell book in the pack (an item of type 29, `FindCarriedSlot`). [V]

#### 5.9.2 Choosing spells

A character's spells are two arrays of 30 bytes, five spell levels of six slots, indexed `level * 6 + k`. A positive byte is a spell that is memorised and ready; a negative byte is a spell that is chosen but not yet memorised, spent or waiting for the next rest; 0 is an empty slot. [V] `SpellSlotScreen` (ovl1:097A) shows the spell list for one level at a time with "n of m Remaining". The allowance per level is looked up: a cleric from `gClericSpellsPerLevel` (`ds:62FE`, 10 rows) by level, with the `gClericWisdomBonus` row (`ds:6330`) for wisdom 13 to 19 added to every level that already has a base spell; a mage from `gMageSpellsPerLevel` (`ds:6394`, 11 rows), doubled at levels 4 and 5 with a ring of Wizardry; a paladin from `gPaladinSpellsPerLevel` (`ds:6358`), one row per level. [V]

| Class level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Cleric (spells of level 1..5) | 1 | 2 | 2/1 | 3/2 | 3/3/1 | 3/3/2 | 3/3/2/1 | 3/3/3/2 | 4/4/3/2/1 | 4/4/3/3/2 | none |
| Mage | 1 | 2 | 2/1 | 3/2 | 4/2/1 | 4/2/2 | 4/3/2/1 | 4/3/3/2 | 4/3/3/2/1 | 4/4/3/2/2 | 4/4/4/3/3 |

The paladin has spells only above level 8: 2 first-level spells at level 9, 2/1 at 10, 2/2 at 11. The code reads its row with the level itself, not the level minus one that the cleric and mage use, so the paladin gets the allowance that first edition gives one level higher [H, ?]. The spells on offer are bit masks: a cleric gets `7BFFFFh`, a paladin `800FF2h` (it includes Lay on Hands, spell 24), a mage whatever is in `spellBook`.

When the screen is left `SpendSpellSlot` (ovl1:025D) rewrites the 30 bytes. It keeps a pair of counters per spell, the copies held and the copies ready; ready copies are stored positive, the rest negative, and `gNeedsRest` is set if any was stored negative. A level holds six slots and a copy that finds no free slot is lost (the loop counts the copy off first), which is why the doubled mage allowance is exactly six. [V for the code, I for the cleric with wisdom 19 reaching seven first-level spells] `SortSpellSlots` (0EF3:000E) then bubble-sorts each row of six: positive spells first by number, then negative, then empty. `ScribeScrolls` (ovl1:1691) copies a mage scroll (an item of type 34 whose value is the spell number) into the spell book by setting the bit and destroying the scroll record; it lists up to six scrolls the chosen mage lacks. [V]

#### 5.9.3 Resting

"Rest Party" runs `CampLoop` (ovl6:0000, `src/EOV6.C`).

<!-- FIG chr_rest -->

It refuses at once, with "You can't rest here, monsters are near.", if any of the 30 monster records is within `Distance` 1 of the party's cell (the same cell or one of the eight neighbours; `AreMonstersNear`, ovl6:09D2, which also runs `UpdateMonsters` for groups 0 and 1, so monsters keep moving while the party rests). With a poisoned member present the game asks "Poisoned party members will die! Rest anyway?" (message 3Bh). If someone is wounded and a conscious, fed cleric is present it asks "Will your healers heal the party?" (28h). [V]

The loop's unit is the tick, a sixth of an hour. Each tick increments a per-member counter and tries to memorise: for each present, conscious member with food it takes the first negative slot of the cleric array whose level row is below the counter (`row + 48 < counters[i]`), makes it positive, prints "<name> gained <spell>." and resets the counter to 48; a member who carries a spell book does the same with the mage array and prints "memorized". The counter starts at 0 for each rest, so no spell is memorised until the 49th tick, about eight hours, and then a level n spell needs n ticks after the last one; a longer rest memorises the whole list. [V for the code, I for the timing] A cleric casts its ready Cure Light Wounds (1d8), Cure Serious (2d8+1) and Cure Critical (3d8+3) at the most wounded member, all of them in one pass, and its counter is reset to 0, so it needs another eight hours to memorise again. A cleric that has a spent cure waiting and a counter of 48 or more does not heal, and because the code uses `break` where `continue` was meant, the healing pass ends for the members after it as well. [V] The most wounded member is chosen at the start and every eight hours (`FindMostWoundedMember`, ovl6:07E2). [V]

Every sixth tick is an hour: the loop counts `hours`, adds `7FF8h` ticks (32,760, half an hour of the 18.2 Hz clock) to `gGameTicks`, shows "Hours rested: n" and deals 10 damage to every poisoned member. Every eight hours each paladin gets Lay on Hands (spell 24) put back among its first-level slots, each member with food and below `maxHp` gains one hit point, and the most wounded member is chosen again. If a member's food is 0 and the hour is a multiple of 24, that member takes 1 damage and the game asks "Your party is starving. Do you wish to continue resting?". Resting alone therefore heals almost nothing; healing is by the clerics' spells. [V]

The loop goes on while `HasSpellsToRegain` (ovl6:0A92) is true or the party agreed to heal and someone is wounded, and ends at a key press, a monster, or a "no" to a prompt; when it ends because nothing is left to do and nobody is wounded, it shows "All characters are fully rested." The return value is the last `AreMonstersNear` answer, but on several exits it is read before it is set, so the value is what the frame held (`src/EOV6.C` documents the quirk). [V] `HasSpellsToRegain` asks `FindClassSlot(4, class) != 0`, true for every class without a paladin slot, and so skips the cleric spell scan for exactly the paladin: a paladin alone with a spent spell does not keep the rest going. [V reading, ? whether intended]

#### 5.9.4 Food and starvation

`food` is one byte, 100 at the start. Effect 14 of the global timer table (`DecayMemberState`, 1414:0C82, period 1080 ticks, about 59 seconds) takes 1 from every present member with food above 0 and hit points above -10, so 100 lasts about 99 minutes of play.

<!-- FIG chr_food -->

The test meant to exempt a Ring of Sustenance wearer is `IsWearingRing(who, 2) < 0` (`or ax,ax; jl` at 1414:0C96), but `IsWearingRing` (0D1C:0163) returns 1 or 0 and never a negative number, so every living member eats and the ring has no effect there. The same test guards the food cost of the eight-hour step of `CampLoop` (ovl6:0653), which therefore never runs. [V] The cost of a rest in food is then at most one decrement: `CampLoop` advances the clock without running the effects, and the first pass of the map loop after the camp fires effect 14 once. [I] Whether the original release behaved so, or this is the fan patch's doing, I cannot tell. [?]

Food comes from rations (value 25), iron rations (value 50, one stack 100) and eggs (10 to 40) eaten from the pointer through `EatFood` (0ACD:07E0, item types 1Fh and 31h, clamped at 100); from the Vitality potion; and from Create Food, which sets all six members to 100 (`CastCreateFood`, 0EF3:1DC0). `DrawMemberFood` fills the bar with colour 3 when food is 33 or more, colour 5 below 33 and colour 8 below 20 (`gFoodBarColours`, ds:5D29); what these look like depends on the palette. [I]

At food 0 nothing happens while walking: no unit reads the byte for a penalty. In a rest the member no longer memorises spells, cannot be picked for Memorize or Pray, gains no hit point and loses one hit point per 24 hours. [V by search of every use of `food` in `src/`]

### 5.10 Input and the interface

The whole interface is built from one idea: a screen is a linked list of *nodes*, each a rectangle with a key, flags and a callback. One routine, `DispatchNodeEvent`, offers every key press and every mouse event to the list, and the callbacks (the same functions for the keyboard and the mouse) do the work. The camp menu, the title menu, character creation and the inventory all use it. The 2.7 section covers the drivers; this section covers what sits above them.

#### 5.10.1 From scancode to command

<!-- FIG arc_input -->

A key travels: scancode, key code (a positional number, section 3.6.3), a ring event with modifier bits in the high byte, then `DispatchKeyWait` (16A5:005A). That routine reads at most one event with `IsKeyPending` and `WaitKeyEvent`, drops the Shift bit of a letter (key codes 11h to 1Ah, 1Fh to 27h and 2Eh to 34h, so shifted and unshifted letters match the same node), and calls `DispatchNodeEvent(head, key)`. With nothing waiting it still calls it with key 0, so the mouse can act without a key. [V] The sibling `DispatchKey` (16A5:0009) tags the event with 8000h and uses the translated key; no caller was found. [V] `src/E16A5.C:85-130` for what the two do, [I] for the missing caller.

`DispatchNodeEvent` (16A5:00BD, 1,798 bytes) does four things (the node figure below, part C): it sets the left and right button *mode*; redraws the list when it changed; walks the nodes; and for each node that matches updates its state, redraws it and calls its callback. A node matches a key when the event word (key code in the low byte, modifiers in the high byte) equals its `key06` word, which acts as a left click, or its `key08` word, a right click; the walk records which with `gMatchedNodeKey` (1 or 2), which several callbacks read. It matches a click when the hit point lies inside its box and its `flags` allow the mode. [V] The four modes are 1 press, 2 held, 3 release, 4 idle; `GetKeyState(41h)` and `(42h)` give 2 or 4 each call, and an event 41h or 42h gives 1 (or 3 for a release, AH bit 8) and moves the hit point from the pointer to the click's own position. [V] `src/E16A5.C:180-225`. A callback returning nonzero ends the walk. Almost every node of the map screen has `flags` 1100h: react to a press of either button; the six name strips have 1000h, right button only.

<!-- FIG arc_nodes -->

**Keys in node lists.** The nodes carry the keys, so a binding is a table entry. The bindings of the map screen (list 1b), read from the `key06` and `key08` words of its nodes:

| Key (or keys) | Words | Command | Callback |
|---|---|---|---|
| keypad 8, Up | 60h; Shift: 160h | step forward | `MoveForward` (0ACD:04F8) |
| keypad 2, Down; keypad 5 | 62h; 61h | step back | `MoveBackward` (0ACD:0529) |
| keypad 4, Left (+Shift) | 5Ch; 15Ch | step left | `MoveLeft` (0ACD:05BA) |
| keypad 6, Right (+Shift) | 66h; 166h | step right | `MoveRight` (0ACD:05FF) |
| keypad 7, Home | 5Bh | turn left | `TurnLeft` (0ACD:056C) |
| keypad 9, PgUp | 65h | turn right | `TurnRight` (0ACD:0593) |
| F1 to F6 | 70h to 75h | select member 1 to 6 | `SelectClickedMember` (0ACD:078C) |
| Shift+F1 to F6 | 170h to 175h | pick up a member to swap places; the second pick completes it, Esc cancels | `PickOrSwapMember` (0ACD:0264), list 4 |
| C | 30h | the camp menu | `RunCampMenu` (ovl1:1ACD, stub 2102:0020) |
| I | 18h | the inventory of the selected member | `OpenInventory` (0ACD:0034) |

Elsewhere: in the inventory (list 2) and character sheet (list 3), Esc or I returns to the map (`RestorePortraitPanel`, 0ACD:01E3), P toggles between the two (`ToggleCharacterInfo`, 0ACD:018E), and F1 to F6 jump to members 1 to 6 (the three callbacks `SelectPrevMember` 0ACD:008C and `SelectNextMember` 0ACD:00EB read `gMatchedNodeKey` to turn the key into a member). In the spell panel (the "a" lists) the digits 1 to 5 choose the spell level (`SelectSpellLevel`), comma and period page the list and Space casts (`HandleSpellList`, 0EF3:2929); while a spell waits for its target (list 5) F1 to F6 choose the member to cast on and Esc aborts (`CastSpellOnTarget`). [V] Decoded from the node tables, `ds:5102` to `ds:5462`.

**Keys that work anywhere.** Alt-S toggles sound and Alt-M the music, inside the INT 9 handler, so they work in every screen including the title menu. [V] `src/E1AAD.ASM:970-991`.

**Keyboard-only play.** When the mouse is disabled (`gMouseDisabled`: no driver, mouse argument 0, or "Mouse is OFF" in the camp menu), `HandleMapKey` (0ACD:0D9D) also acts on the key `DispatchKeyWait` returned. It drives a *hand cursor*: `gCursorSlot` is the selected row (hand slot or a row of the member's own list), `gInventoryMember` the selected member. [V] `src/E0ACD.C:853-906`.

| Key | Code | Command |
|---|---|---|
| W | 12h | cursor up (previous member row, or previous action row) |
| S | 20h | cursor right |
| Z | 2Eh | cursor down |
| A | 1Fh | cursor left |
| U | 17h | use the item under the cursor (`UseItemSlot`) |
| D | 21h | drop it in front (`MoveListItem(0)`, an item to the floor of the party's cell) |
| T | 15h | throw it (`MoveListItem(1)`, `AddDDEffect`) |
| G | 23h | get an item from the floor (`PickListItem`) |
| M | 34h | touch the wall ahead, as a click on the view does (`ActivateWallAhead`) |

U, D, T and G do nothing and the cursor does not move on the character sheet (`gPanelMode` 2). The assignment of letters to directions (W, S, Z, A) is what the key codes give; the names of the commands are inferred from what the callbacks do. [I]

#### 5.10.2 Mouse regions

The node boxes are the mouse regions. List 1b is the map screen (38 nodes); the figure draws them to scale.

<!-- FIG arc_clickmap -->

The nodes sit in the list in this order: the 3D view; the six portrait boxes; the camp icon; the four floor spots; the six faces; the six hand boxes; the six name strips; the six movement arrows; the two throw halves. Order matters, because a click is offered to every node under the pointer in list order until a callback returns nonzero. [V] So a click on a hand box reaches the portrait box first (`SelectClickedMember` selects the member and returns 0, which lets the walk go on) and then the hand node (`ClickHandSlot`, which returns 1 and stops): one click selects the member and acts on the hand. A click on a face selects the member and then `OpenInventory` opens that member's inventory; the left button on a hand swaps the item on the pointer with the hand's item and the right button uses the hand's item (`ClickHandSlot` tests `gLeftButtonMode & 7Fh == 1`, which holds for a left press only), and which half of the box was clicked is passed on (`gHandSplitY`). A right click on a name strip starts the member swap. The four floor boxes are two spots in the party's own cell (the lower pair) and two in the cell ahead (the upper pair); which of a cell's four positions each stands for depends on the facing (the `slotKind` table in `ClickFloorItem`, 0ACD:0644), and a click picks up the item lying there or drops the one on the pointer. [V] The two throw halves, which overhang the view by 24 pixels on each side, throw the item on the pointer ahead (`ThrowMouseItem`). A click on the view itself sets the trigger to 40h (a click) and calls `DispatchWallHandler` for the wall ahead (`ClickWallAhead`, 122E:0933); the handlers for buttons, levers and niches then ask `IsWallDecorClicked` whether the click fell inside the decoration's drawn rectangle. [V]

The inventory screen replaces the portrait, face, hand and name nodes by the member's 27 item boxes, the food box and the arrows:

<!-- FIG arc_invmap -->

Which nodes belong to which screen is a choice of table. The eight tables are pointer arrays of far pointers to 60-byte node records (`ds:3A62` and the grid after it); `BuildList1` to `BuildList5` chain the records of one table into a list with `ListInit` and `ListAdd`. [V] `src/E0CF7.C`.

| Table | Records | Address | Used for |
|---|---:|---|---|
| 1b | 38 | `ds:506A` | the map screen |
| 1a | 52 | `ds:5102` | the map with the spell panel open (`gFlag` 1) |
| 2b | 47 | `ds:51D2` | the inventory |
| 2a | 59 | `ds:52DA` | the inventory with the spell panel |
| 3b | 32 | `ds:53C6` | the character sheet when `gFlag` is clear |
| 3a | 19 | `ds:528E` | the character sheet when `gFlag` is set |
| 4 | 7 | `ds:5446` | swapping members: six name strips and Esc |
| 5 | 7 | `ds:5462` | picking a spell target: six portraits and Esc |

`gFlag` is set while a spell panel is open (`OpenSpellPanel`) and cleared when it closes (`RedrawPartyPanel`), and `BuildList1` to `BuildList3` pick the "a" table when it is set. The "a" tables of lists 1 and 2 are the plain lists plus the spell panel's regions: in 1a 14 nodes (the five level tabs, the six spell rows, the comma-and-period node, the Space node and the abort row), in 2a 12 (the same without comma, period and Space). [V] Table 3a, however, has *no* spell nodes and table 3b has them, the other way round from lists 1 and 2: on the character sheet the spell regions are live when no spell panel is open and missing when one is. The tables are named by what `BuildList3` does with `gFlag`, so this is how the original behaves, whether by design or by a swapped pair of table addresses is not known. [?] The first node of each of the tables 1a to 3b is the 3D view, one shared record (`ds:4836`); tables 4 and 5 start with their first name strip (`ds:3E9A`) and first portrait box (`ds:4B7E`).

#### 5.10.3 The node record and the drawing

A node is 60 bytes (`struct Node`, `src/EOB.H`): the `next` pointer, a sort `key` that doubles as the id the callback returns, the two key words, a draw `kind` (0 to 5) and two modes, `flags`, three art or callback pointers, a text region index and the box (x, y, width, height) in that region's pixels (a negative x or y is measured from the region's right or bottom edge), four colour pairs, a `state` word (on, previous-on, pressed bits), the `callback` and `data` (two tile ids, or for the camp menus a pointer to the button definition). `DrawNode` (16A5:07C3) hides the pointer over the box, then draws by kind: 1 an art pointer through `DrawShape`, 2 text, 3 a tile, 4 calls the art pointer as a function, 5 a box, 0 a box only when a mode asks. The map-screen nodes all have kind 0 and draw nothing themselves: the art is part of the `PLAYFLD` sheet and the nodes are invisible hit zones. [V] `src/E16A5.C:482-700`.

List operations (`src/E1658.C`): `ListAdd` appends, `FindNode(head, key)` searches by id, `ToggleNode`, `SetNode`, `ClearNode` change the on bit and redraw, `DisableNode` and `EnableNode` set and clear the inactive flag, `InsertNode` inserts in key order. `FindNode` has a bug: when the list starts at its last node and the key does not match it reads the far pointer at 0000:0000. [V]

#### 5.10.4 Menus, option lists and boxes

There are three menu mechanisms. [V]

**Option lists** (`src/E14EF.C`) are for the title menu and the character creation pick lists. A *panel* is a 16-byte record at `ds:017F` (x, y, width, rows, selected row, two colours); `DrawOptionList(panel, names, mask, shift, spacing)` writes the names of the selectable entries (a 32-bit mask says which of the names are offered, so the class list shows only the classes the race allows) and `UpdateOptionList` handles one event: Up and Down (keypad 8, 2) move the row with wrap, Home and PgUp (5Bh, 65h) the first, End and PgDn (5Dh, 67h) the last, Enter, Space and keypad 5 (2Bh, 3Dh, 61h) or a click inside the box confirm, and a confirmed row blinks three times before the call returns the index of the chosen name, or -1 when nothing was picked. The pointer's position selects a row (hover highlight) while `gHoverSelectsOption` is set; its image value is 1 and no code writes it, so it always is. Keys the routine does not handle are left in `gUnhandledOptionKey`.

**Button menus** (`src/EOV1.C`) are the camp menus and the Yes/No box. Two tables build them: a 5-byte menu record (message index, text window, first button, count, colour; `ds:628C`) and a 9-byte button definition (label message, x, y, width, height in 8-pixel columns and pixels, key word, flags: bit 0 fill the box, bit 1 selectable, bit 2 centre the label; `208E:0000`). `BuildMenuButtons` (ovl1:1153) makes one 60-byte node per button, with key06 the definition's key and key08 the same plus 100h, draws the header, and returns the list. `RunCampMenu` loops on `DispatchKeyWait` over it, remaps Esc (6Eh) to the Exit button through two byte tables at `ds:6457`, lights a button with the cursor keys and confirm keys, and flashes the pressed one for 5 frames before acting.

<!-- FIG arc_menus -->

**Boxes.** `AskYesNo(message)` (ovl1:243A) shows a message over the two buttons of menu 5 (Yes: Y, No: N, left and right keys move the highlight, confirm keys press it) and returns 1 or 0. `ShowMessageBox(message)` (ovl1:13F7) frames a message in text window 11 and waits for a key or 120 frames. `EditTextField(text, x, y, maxlen, colour, attr, cursorAttr)` (082C:000B) is the line editor of character names: printable keys insert (converted to upper case), Backspace and Delete delete, Left and Right move, Enter ends and returns the length, Esc returns -1; the cursor blinks every 2 ticks on `gTicks`; a name is at most 10 characters (`maxlen` 0Ah); a space is refused at the start. [V]

#### 5.10.5 The message area and text windows

Text goes into *text windows*, rectangles of the region table at `ds:009F` (14 records of 16 bytes: left and width in 8-pixel columns, top and height in pixel lines, two colours, and the saved cursor). `SetTextWindow(n)` saves the cursor and colours of the window it leaves into its record and loads the new one; `ClearTextRegion` fills it with `gTextAttr2`; `DrawText(string)` writes into it with word wrap and scrolls it one line when the cursor passes the bottom (a `CopyRegion` of the window up by one line and a fill of the last line). [V] Some of the windows in use:

| Window | Rectangle (x, y, w, h in pixels) | Use |
|---|---|---|
| 0 | 0, 0, 320, 200 | the whole screen; the default at start |
| 2 | 152, 64, 160, 128 | character creation lists |
| 6 | 80, 168, 168, 24 | the title menu |
| 7 | 8, 179, 272, 18 | the map screen's messages |
| 9 | 8, 125, 304, 63 | the defeat message, event panels |
| 10, 11 | 0, 0, 176, 144; 8, 20, 160, 56 | the camp menu area; message boxes |
| 12, 13 | 8, 4, 160, 160; 8, 25, 304, 100 | the winner text; the "find the word" panel |

`DrawText` understands control codes in the string: 0Dh ends the line, 02h and 06h take the next byte as the fill and the text colour (the game's messages start with `\x06\x06...`), 09h the next byte as a column, 13h the next byte as a column to pad to, and a byte with bit 7 set ends the string. [V] `src/E1DE7.C:316-420`. Every step of the party prints a newline (`MoveParty`), so the message window scrolls as the party walks.

#### 5.10.6 The character sheet and inventory screens

The right-hand panel of the map screen is redrawn per member by `RedrawMember` (1547:0991), and its layout depends on `gPanelMode`: 0 draws the name (or "Swapping"), the portrait, the two hand slots, the HP line and bar, a status bar, and the last damage; 1 (inventory) draws the HP line, the food bar, one status line (DEAD, UNCONSCIOUS, POISON (SLOW), POISONED or PARALYZED) and the item boxes; 2 (character sheet) draws the stat panel: class, race and alignment names, the six ability lines, armour class, experience and level per class slot. `gBarGraphsOn` chooses bars or numbers for hit points. [V] `src/E1547.C`. `ToggleCharacterInfo` flips mode 1 and 2 and rebuilds the list (`BuildList3` or `BuildList2`), so the P key and the P box on the inventory screen swap the two without leaving the member.

### 5.11 Saving and loading

There is one saved game and no slot menu. The files are `EOBDATA.SAV` (33,101 bytes) and `LEVELS.TMP` (24,480 bytes), both in the game's directory (the file table gives `EOBDATA.SAV` disk number 6, and the game asks for a missing file's disk by number, section 3.5). [V] The camp menu's Save Game and Load Game each ask "Are you sure you wish to SAVE the game?" or "... LOAD a saved game?" through `AskYesNo`; the title menu's first entry loads directly. A save ends with the box "Game saved." or "Save game failure!"; a load that fails says "Load game failure!" from the camp and "Unable to load your saved game!" from the title. [V] `RunCampMenu`, `LoadGamePanel`.

#### 5.11.1 The layout of `EOBDATA.SAV`

`SaveGame` (ovl3:1368) writes six blocks back to back, with no header, no checksum and no version, each straight out of memory; `LoadGame` (ovl3:15B9) reads them with the sizes in the table at `ds:6A8C` (1,458, 64, 7,000, 24,480, 3, 96). [V]

<!-- FIG arc_savefile -->

| Offset | Bytes | Source in memory | Meaning |
|---:|---:|---|---|
| 0000 | 1,458 | `gChars` (`ds:F580`) | the six 243-byte character records, in slot order; before the write `AgeCharTimers` turns each member's ten effect timers into remaining ticks and `AdvanceCharTimers` restores them in memory afterwards |
| 05B2 | 64 | `ds:FBC8` | the header of game state (below) |
| 05F2 | 7,000 | `gItems` (`ds:8DD0`) | the 500 item records of 14 bytes; each says its level, cell and place, so items are not part of a level's state |
| 214A | 24,480 | `LEVELS.TMP` | 12 level states of 2,040 bytes |
| 80EA | 1 each | `gTunesOn`, `gSoundOn`, `gBarGraphsOn` | the three options |
| 80ED | 96 | `gSpellItemTypes` (`ds:AD20`) | six 16-byte item-type records, numbers 33h to 38h, which the game creates at run time for the shocking grasp, vampiric touch and flame blade items (`CreateItemType`, 0EF3:035A) |

The header is the variables at `ds:FBC8` to `ds:FC07` in memory order: the level (`gCurLevel`), the party's cell, its direction, the item on the pointer, the mask of levels saved (`gSavedLevels`), the party spell flags (bless, prayer, detect magic), and the 32-bit script flags: `gLevelFlags` (one 32-bit entry per level, 12 entries from `ds:FBD4`) and `gGameFlags`. [V] `gLevelFlags` is indexed by `gCurLevel`, which runs from 1 to 12, but the array has 12 entries from index 0, so level 12's flags are the same four bytes as `gGameFlags`. [V] `src/E122E.C:843, 893, 1087`. Whether the script uses this aliasing on purpose is not known. [?]

The test save in `eob/` is the game's starting position: level 1, cell 490 (0x1EA), facing north, no item on the pointer, `gSavedLevels` 1, four characters (KIMIL, IMMAR, ZANDI' and AZALIS) with the portrait pointer words of the machine that made it still inside their records (the load overwrites them). [V] The options bytes read 1, 1, 0 there. [V]

#### 5.11.2 Level states and `LEVELS.TMP`

Only what changes in a level is kept. The pristine level is `LEVELn.MAZ` (the four wall bytes of each cell, section 4.3) plus `LEVELn.INF` (the script, the monster list); a visited level adds a *level state* of 2,040 bytes (7F8h): the wall bytes of the 1,024 cells packed by `CompressLcw` (19FC:0008, the LCW encoder, the inverse of the game's `Unpack4`) into a stream that must not exceed 1,200 bytes (4B0h), followed by the 840 bytes (348h) of the 30 monster records. [V] `PackLevelState` (ovl3:0FD1). If the stream is longer the game prints "Map compression failure: size of map = N" and shuts down; the shipped levels fit. So the save as a whole is not compressed, only the maps inside it, and a save of an unmodified level costs the same 2,040 bytes as a heavily modified one: the unused tail of the stream area is not zeroed.

`LEVELS.TMP` is the working copy of the 12 states. Its life:

<!-- FIG arc_savedata -->

1. Leaving a level by a script (`LoadLevelOp`, 122E:1AAB) calls `ResetLevel` (a moving door is finished, projectiles and attack frames cleared), then `SaveLevelState` (ovl3:11AF), which packs the state and writes it at `(level - 1) x 2,040` in `LEVELS.TMP` and sets bit `level - 1` of `gSavedLevels`.
2. Entering a level calls `LoadLevel` (ovl3:0A7C): `LoadMaze` reads the pristine walls, and if the level's bit is set `LoadLevelState` (ovl3:1147) reads the 2,040-byte slot, `Unpack4`s the walls over them, copies the monsters back and adds 1 to the occupancy byte of the cell of each of the 30 monster records; if not, `LoadLevel` creates the monsters from the `.INF` list. Items are rebuilt from the global item array for every entry.
3. `SaveGame` does not rely on the file being current for the level the party is in: it reads the *whole* `LEVELS.TMP` into `gArtPage`, packs the current level over its own slot in that buffer, sets its bit, and writes the buffer as block 4. The file itself is not updated.
4. `LoadGame` reads the six blocks into `gArtPage`, unpacks the characters (keeping the portrait pointers of the running session, then `AdvanceCharTimers` adds the current clock to every timer so effects resume), the header and the items, recreates `LEVELS.TMP` from block 4 (deleted and written whole), restores the three options and the 96 bytes, re-cuts each character's portrait from the `CHARGENA` or `OUTTAKE` sheet by its `portraitIndex`, and, unless a title-menu load is still starting up (`gGameLoadState` 0FFh), calls `LoadLevel` and redraws. [V] `src/EOV3.C:837-929`.

The game clock `gGameTicks` is not saved: effects are stored as remaining durations and the clock starts again. Monster timers, the level timer and the effect table are rebuilt by `LoadLevel`. The camp menu's save therefore preserves positions and conditions, not the exact phase of the monsters' next moves. [V]

#### 5.11.3 Integrity and errors

There is none beyond lengths. A load is "ok" when each of the six reads returned its full length; a short file gives failure and `gGameLoadState` is cleared. A save is "ok" when every write returned its full length, and the INVENT sheet is reloaded afterwards whether or not it was. No checksum, no signature, no version; a save made by one patch level would be read by another as long as the sizes agree. [V] `SaveGame` first opens `EOBDATA.SAV` for reading and closes it, which, when the file is on another disk, makes the file layer ask for the disk before it deletes and recreates the file. [V] Two things the code leaves undefined: if `LEVELS.TMP` is shorter than 24,480 bytes (a new game that has visited few levels) `SaveGame` reads fewer bytes than it asks for and the unread part of the 12-slot image is whatever `gArtPage` held; those slots belong to levels whose bit is not set and are never read back. [I] And the file is a raw memory image of a 16-bit little-endian program: every integer is two bytes, `long`s are four, and the stored far pointers (the characters' portraits) mean nothing outside the machine that wrote them. [V]

### 5.12 The 3D view

The first-person view is the part of the game that looks most like a 3D renderer and is the least like one. There is no projection and no scaling of walls: every wall, door and decoration the player can see is a picture drawn by hand at one of four fixed depths, and the program only decides which pictures to put where. Everything below is in overlay 4 (`src/EOV4.C`, 8,259 bytes, 18 functions, entry `DrawViewFrame` at ovl4:1F51), plus the tile blitter `DrawTile` (17A6:03E7, `src/E17A6.ASM`) and the level loader (`src/EOV3.C`). The view is 176 by 120 pixels, 22 by 15 blocks of 8 by 8 pixels, at the top left of the screen: region 5 of the region table is `0, 0, 22, 120` in 8-pixel columns and lines (ds:009F, section 5.13.2) [V].

#### 5.12.1 One frame

`DrawViewFrame(full)` is called whenever something visible changed (`gViewDirty`, set by every move, turn, door step, floor item and monster change; `RunMapLoop` calls it, `src/E0ACD.C:937`) and by the code that wants to show a monster hit. It does seven things in this order (figure below) [V] (`src/EOV4.C:1085`):

1. `BuildViewBlockMap(gPartyCell, gPartyDir)` computes the 18 cells the view looks at and fills a two-layer map of 22 by 15 block words at ds:ADB0.
2. `DrawTile(gWallBlocks)` turns that map into pixels on page 2 (the off-screen draw page, section 5.13.1), 176 by 120 at the top left.
3. `DrawViewSlots(0)` draws the things standing in those 18 cells, farthest first, on the same page.
4. On VGA only (`gGraphicsChoice == 3`), if a new level's palette is waiting (`gLevelPalettePending`), the view area of the screen is blanked first when `full` is set and the palette is loaded (section 5.13.7).
5. The frame waits until `gFrameWaitTicks` is zero. The INT 1Ch tick handler counts that 32-bit word down (`src/E1D8F.ASM`, 1D8F:0098); the attack and hit-flash code sets it to 1 or 8 to hold a frame on the screen.
6. When `full` is set and `gQuietMonsterUpdate` is clear, `CopyRegion(0, 0, 0, 0, 22, 120, 2, 0)` copies the view from page 2 to page 0, the screen, between `HidePointerZone` and `ShowPointerZone` (section 5.13.6).
7. Sound 0 is requested, the compass is redrawn if the facing changed and `gFlag` is clear, and `gViewDirty` is cleared.

All callers in `src/` pass `full = 1` except one: the camp and option panel (`src/EOV1.C:1479`) calls `DrawViewFrame(0)` with the draw page set to 2, which composes the view and does not copy it to the screen [V]. There is no wait for the vertical retrace in the view path: only the menu and animation code calls `WaitFrames` (section 5.13.10) [V, `grep WaitRetrace`: the only callers are `WaitFrames` and `CalibrateRetrace`]. The view reaches the screen as one rectangle copy.

<!-- FIG gfx_frame_flow -->

The compass has a name problem. The function at ovl4:1EDE is named `DrawCompass` in `src/EOV4.C`, but its callers in `src/E0EF3.C:149` and `src/EOV3.C:38` (and `re/renames.toml`) call it `DrawViewRows`. It draws three facing-dependent shapes (12 shapes in all, 3 columns wide, `gCompassIcons[gPartyDir + 4 * i]`) at (112, 127), (77, 154) and (149, 154) below the view, inside the pointer zone (70, 121) to (176, 175), and returns at once when the facing is the one already drawn (`gCompassDir`, ds:7296). It draws on page 0 directly. It is the compass, not rows of the view [V, `src/EOV4.C:1050`].

#### 5.12.2 The 18 cells

The view looks at 18 cells of the 32 by 32 map: 7 in the farthest row, 5, 3 and the 3 of the party's own row (the party's cell and its two neighbours). Their order is the slot number, and every per-slot table of the view is indexed by it [V].

<!-- FIG gfx_view_slots -->

`InitViewSlots(base, row)` (ovl4:0000) is called by `BuildViewBlockMap` with the party's cell and facing. For each slot it adds the signed offset `gViewCellOffsets[gPartyDir * 18 + slot]` (ds:199D, four rows of 18 signed bytes, one per facing) to the party's cell and masks the sum with 3FFh. The result goes to `gViewCellIds[slot]` (ds:A9C8) and points `gViewCells[slot]` (ds:EF6A, far pointers) at the 9-byte record of that cell in `gMaze` (ds:B894, 1,024 records of 9 bytes, `LoadMaze` in `src/EOV3.C:465`). The masking makes the map wrap at its edges: a step off the top lands on the bottom row (`StepCell`, `src/E0D1C.C:182`). `IsCellInView(cell)` (0D1C:0006) searches the first 15 words of `gViewCellIds` (the three outer rows) and sets `gViewDirty` when the cell is there, so that a monster or a door that changes in view gets redrawn; the party's own row is not searched [V].

The maze record is nine bytes. Bytes 0 to 3 are the wall types of the four sides; the level loader copies them from the `.MAZ` file's four bytes a cell (`LoadMaze`). Byte 4 counts the monsters in the cell (bits 0 to 2; `UnpackLevelState` increments it for each of the 30 monster records). Bytes 5 and 6 are the head of the cell's item list, a word that indexes `gItems` (`src/EOV4.C:1023`) [V]. Which of the four bytes is the "front" depends on the facing. Three small tables give, per facing, the side that faces the party (`gNearSideByDir`, ds:1991), the side on its right (ds:1995) and on its left (ds:1999); `BuildViewBlockMap` caches them in `gViewFrontSide` (ds:A9C6), `gViewRightSide` (ds:EFB9) and `gViewLeftSide` (ds:EFD0) [V]:

| Party facing | north | east | south | west |
|---|---:|---:|---:|---:|
| front side, `(dir + 2) & 3` | 2 | 3 | 0 | 1 |
| right side, `(dir + 1) & 3` | 1 | 2 | 3 | 0 |
| left side, `(dir + 3) & 3` | 3 | 0 | 1 | 2 |

A cell stores the wall between it and the party's side in its own byte, so for a cell to the left of the party the wall that faces the party's axis is its right-hand byte. The same byte is what movement tests: `StepIfOpen` (0D1C:0080) looks at the target cell's side facing the mover and needs bit 0 of its flag byte set [V].

The wall byte is a wall type, 0 to 69. Four parallel tables of 70 bytes say what each type looks like and does: `gWallVmpSet` (ds:1879, which wall set of the level's `.VMP`, 0 for none), `gWallDecorHead` (ds:18BF, the head of its decoration chain, section 5.12.6), `gWallHandlerIndex` (ds:1905, the script handler) and `gWallTypeFlags` (ds:194B). The start-up image holds the door types: types 3 to 7, 8 to 12, 13 to 17 and 18 to 22 are four families of five, all wall set 3, with flag bit 3 (0x08, "this is a door"), bit 5 (0x20, closed) or bit 4 (0x10, open). A door opens by `StepDoor` (1414:03B4) adding or subtracting one from the type of both sides of the cell, so the type number is the door's position, 0 (closed) to 4 (open) above its family's first number (`gDoorStepBase`, ds:714E: 3, 8, 13, 18) [V]. Every level script then defines the other types with `DefineWallType` (ovl3:01C8) records of five bytes: the type, its wall set, its decoration chain, its handler and its flags (stored with bit 2 toggled). Level 1 defines 24 types, level 9 defines 36. Wall types 1 and 2 are never defined by a script (all 12 levels checked) [V]. Flag bit 0 means the party may pass (`StepIfOpen`, `src/E0D1C.C:171`), bit 2 that monsters may (`TryMoveMonster`, `src/E08E6.C:272`), bit 3 a door, and bit 7 that the cell behind can be seen: items and projectiles in a cell are drawn only when the wall on its front side has no art (set 0) or has bit 7 set (`DrawSlotItems`, `DrawSlotProjectiles`) [V]. What each of the six wall sets draws is not recorded in the code. By count, set 1 is by far the commonest (14 to 25 of the 24 to 36 types a level defines); set 3 is the door frame; types 23 and 24 are the only ones to use sets 4 and 5, on levels 1 to 11 (level 12 gives them set 1); set 6 is used by 1 to 6 types on levels 2 and 4 to 12, set 2 by type 2 and by five types of level 12 [V, all twelve scripts decoded]. [H] Types 23 and 24 are the two staircases, since each level has exactly one such pair.

#### 5.12.3 Wall graphics: `.VMP` and `.VCN`

Each level names its wall set in its `LEVELn.INF`. `LoadLevel` (ovl3:0A7C) reads, at offsets 2, 14 and 26 of the unpacked script, three 12-byte names: the maze, the name it forms the `.VMP` and `.PAL` from, and the name `LoadWallTextures(gLevelInf + 0x1A, ...)` forms the `.VCN` from. They are the same word in all twelve scripts [V]:

| Levels | Wall set | `.VMP` words | `.VCN` blocks | `.ECN` blocks |
|---|---|---:|---:|---:|
| 1, 2, 3 | BRICK | 2,916 | 1,575 | 1,484 |
| 4, 5, 6 | BLUE | 2,916 | 1,512 | 1,585 |
| 7, 8, 9 | DROW | 2,916 | 1,456 | 1,579 |
| 10, 11 | GREEN | 2,916 | 1,523 | 1,617 |
| 12 | XANATHA | 2,916 | 1,305 | 1,018 |

A non-VGA graphics choice takes `.ECN` and `.EMP` instead of `.VCN` and `.VMP` (`src/EOV3.C:427`, `src/EOV3.C:455`): the block numbering differs, so the maps differ, but the layout is the same (the pinned `.EMP` files are 5,834 bytes like the `.VMP` files and differ in content) [V].

**The `.VCN` file** is a count, two 16-byte colour tables and that many blocks of 32 bytes, each 8 by 8 pixels at 4 bits a pixel, high nibble on the left (offsets 0, 2, 12h, 22h; the length of every pinned file is 2 + 32 + 32 * count). `LoadWallTextures` (ovl3:07A5) loads the packed file into page buffer 7, unpacks it into the art page (page 3), and copies the two tables (to `gWallPlaneTableA`, ds:F4D6, and `gWallPlaneTableB`, ds:FB4D, then into the code segment of `DrawTile`, 17A6:03AF and 03BF, by `SetPlaneTables`) and the blocks (`0xCB20` bytes, room for 1,625 blocks) into `gWallBlocks`, which is page buffer 7 itself (ds:DCA6). For the `.ECN` files both tables are zero [V].

**The `.VMP` file** is the same 2,916 words in all five pinned sets: a 22 by 15 background of 330 words, then six wall sets of 431 words (`0x35E` bytes) each, 330 + 6 * 431 = 2,916. `LoadLevel` reads it into `gWallBlockMaps` (ds:F43E, a 6,000-byte buffer; the first read of 2 bytes is the count). The background is the floor and ceiling; it is read from the start of the file for every level. A wall set is nine rectangles of block words tiling its 431 words exactly (45 + 36 + 16 + 5 + 15 + 12 + 30 + 80 + 192), one for each size at which a wall piece can appear. A set holds only the left-hand side pieces and one copy of each front piece; the right-hand sides are made by mirroring [V, `CopyViewColumn`, `ToggleViewColumn`].

<!-- FIG gfx_vmp_layout -->

A block word has three fields (figure below). Bits 13 to 0 are the block number in the `.VCN` (0 means "nothing here, show the background"). Bit 14 means the block is drawn mirrored (columns reversed). Bit 15 means "a transparent block over the background": the 15 low bits are then the block to draw over the background block, and pixels of colour 0 in it let the background show through [V, `DrawTile` at 17A6:0AE1 to 0CBB].

<!-- FIG gfx_block_word -->

The two colour tables turn a 4-bit code into a palette index. Table A is used for the background layer and table B for wall blocks, and both keep 0 as 0; `BRICK.VCN` uses indices E0h to E7h, F0h to F6h and 0Ch, and the other sets other bands of the top 32 entries (E2h to EEh for DROW, F0h to FEh for BLUE, GREEN and XANATHA) [V, tables read from the five files]. The level's `.PAL` says what those indices look like, which is the whole difference between a stone corridor and a drow one (section 5.13.7).

#### 5.12.4 Building the block map

`BuildViewBlockMap` (ovl4:044B) does all of the placement; `DrawTile` does none. It clears the 330-word wall layer at ds:ADB0 (22 by 15 words), writes the background into the second 330 words behind it (ds:ADB0 + 294h; the background copy is made with a row offset of 15), initialises the 18 slots and then walks a fixed list of 25 pieces, nearest-last. Each is a call of one of two functions, with the rectangle's position and size and the slot whose wall it is [V]:

- `CopyViewColumn(x, y, type, x2, w, h)` copies a w by h rectangle of block words from the wall set of `gWallVmpSet[type]`, starting `x2` words into the set, to the map at column x, row y. Words outside columns 0 to 21 are skipped, and only non-zero source words are stored, so a piece lets what is already in the map show through where it has zeros.
- `ToggleViewColumn` does the same with the columns of each row read in reverse order and bit 14 of every word flipped. This is how a right-hand wall is made from the left-hand piece: reversed order puts the block that was at the left edge at the right edge, and flipping the bit mirrors each block's own pixels.

<!-- FIG gfx_view_blocks -->

The figure shows the placements. The list is in code order: far to near, and within a depth the side walls before the front walls; later copies overwrite earlier ones, which is the whole hidden-surface algorithm of the walls. Both copy functions return when the wall type has no set (set 0). The off-centre front pieces (slots 1, 2, 4, 5, 8, 10, 12 and 14) are also skipped when the type is a door (`IsSlotLive`: non-zero and flag bit 3 clear); the central front pieces (slots 3, 9 and 13) are drawn for doors too, as the door frame. Next to a door the code swaps the piece: when the front wall of slot 2 (or slot 4) is a door, the side piece at that place (`x = 2`, 3 by 5 blocks) is taken from the door's wall type, so the doorway has its own jamb [V].

Two oddities are worth knowing. First, the mirrored floor: `gViewMirrorParity` (ds:F4BC) is `((cell >> 5) + (cell & 31) + facing) & 1`, the parity of row plus column plus facing, and when it is 1 the background is copied with `ToggleViewColumn` instead of `CopyViewColumn`. Stepping from one cell to the next changes the parity, so the floor and ceiling pattern alternates with its own mirror image at every step. [I] That is what gives a step its change of picture in a view of otherwise fixed pictures. Wall pieces that carry a decoration with flag bits 0 or 1 use the same parity (section 5.12.6). Second, a rule the code states but does not explain (`src/EOV4.C:337` to `:349`): the 3 by 15 side pieces at the edges of the view are not drawn for some wall types. On levels 4 to 6 and 10 to 12 the wall on the party's left is dropped when its type is 0x18 and the wall on the party's right when it is 0x17; on levels 7 to 9 the wall on the party's right is dropped when it is 0x17 or 0x18 and the left is kept; on levels 1 to 3 nothing is dropped [V; the reason is [?]].

#### 5.12.5 Drawing the map: `DrawTile`

`DrawTile` (17A6:03E7) loops over 15 rows of 22 words and writes 8 by 8 pixels for each into the page, starting at its top-left corner and using 320 bytes a line. In VGA mode (the default branch, `L0ADA`) a word is handled like this [V]:

| Word | What is drawn |
|---|---|
| 0 | the background block from layer 2 at the same position (ds:ADB0 + 294h), through table A |
| n | block n, through table B |
| n and 4000h | block n with its 4 bytes of each row read backwards and their nibbles swapped, through table B |
| 8000h or n | the layer 2 block through table A, then block n over it, through table B, skipping pixels that come out as 0 |

A block's source is paragraph `gWallBlocks + 2 * n` (32 bytes a block). When the word is 0 and layer 2's word has bit 14, that block is drawn mirrored too. The destination is `gArtPage`, page buffer 3. In every mode except EGA that is the same memory as page 2 (the two slots hold one segment), and in EGA mode `DrawTile` writes to A200h, which is slot 2 of that mode's table; so the view and the sprites drawn by `DrawShapeClipped(2, ...)` always land on one page [V from the page table, `src/E07C8.C:332` to `:372`, and `src/E17A6.ASM:253`].

The other modes have a path of their own. EGA saves the driver's far pointer and register shadow, programs the sequencer and the graphics controller, and reads the blocks from the video planes they were copied to at load time (`LoadWallTextures` calls `CopyToPlanar` on `gWallBlocks` into A400h, expands the nibbles into the staging page and copies them to AA00h and ABF4h). The CGA path works on blocks that `LoadWallTextures` converted in place with `BuildMask` (17A6:18EA): it translates the 52,000 bytes through two 256-byte tables found at the driver's row-offset base plus 190h and plus 290h, which are the CGA driver's even-line and odd-line dither tables (04B1h and 05B1h; the base is 0321h), and sets mask bits, so the blocks become CGA pixels with a transparency mask. [I] The one-cell cut of a 1 by 8 rectangle that `LoadWallTextures` makes and frees at once in CGA mode builds those dither tables from the sheet's colour map first. The Tandy path reads the blocks as stored [V from headers and addresses; the inner loops of the CGA, Tandy and EGA paths were not traced].

#### 5.12.6 Wall decorations

Notices, buttons, niches, cracks and the like are the decorations of the wall set's `.DAT` (`BRICK.DAT` and four others). Section 4.4 listed the layout as a hypothesis; it is now established [V] (`LoadWallSet`, ovl3:004F; `DefineWallType`; `DrawSlotDecorations`, ovl4:1091):

- The file is a count and that many records of 52 bytes (BRICK 60, BLUE 40, DROW 35, GREEN 36, XANATHA 27), then a count and that many rectangles of four words, x, y, w, h (BRICK 177, BLUE 143, DROW 115, GREEN 118, XANATHA 86), with x and w in 8-pixel columns and y and h in lines. They are the arguments of `GetSpriteCell`: each rectangle is cut out of the wall set's decoration picture (`BRICK1`, `BRICK2`, `BRICK3` for levels 1 to 3, `BLUE`, `DROW`, `GREEN` or `XANATHA`; the level script names them in `0xEC` records) into a shape (section 5.12.10) kept in `gDecorShapes` (ds:EFD4, 200 far pointers).
- A record has ten entries: bytes 0 to 9 are rectangle numbers (FFh for none), byte 10 chains to the next record (0 ends the chain), byte 11 holds flag bits, and the ten x and ten y positions follow as words (offsets 12 and 32). A wall type's decoration chain is a list of such records, so a type can carry several decorations.

The ten entries are the ten places a decoration can appear in the view. Entries 1, 2 and 3 are on a front wall 1, 2 and 3 rows ahead. Entries 4 to 9 are on side walls: 4 in the party's row, 5 one row ahead, 6 and 8 two rows ahead (inner and outer cell), 7 and 9 three rows ahead. Entry 0 is a front wall of the party's own row, used only by slot 16 ([?] what it draws). Which side of a cell and which entry a slot uses is a table (`gWallFaces`, ds:7018, far pointers to `gViewFrontSide`, `gViewRightSide` or `gViewLeftSide`, and `gWallOffsets`, ds:70A8, signed entry numbers). Each slot has up to two faces, a front wall (face A) and a side wall (face B), and a negative entry number means "draw mirrored" [V].

<!-- FIG gfx_decor_faces -->

For each face `DrawSlotDecorations` reads the cell's wall byte on the chosen side, follows its chain and draws the shape of entry `|offset| - 1` of every record at the entry's x and y; a negative offset draws it mirrored about the 176-pixel centre (`x = 0xB0 - x - width * 8`, `DrawShapeClippedFlipped`). Face B is drawn first and face A second. Face A also reads the record's flags: bit 0 (or bit 1 when the floor parity is odd) reflects the entry, and bit 2 adds the slot's centre x instead of a per-slot bias (`gWallXBias`, ds:70CC: -144, -96, -48, 0, 48, 96, 144 for the far row, -160 to 160 in steps of 80, -128, 0, 128, then three zeros) [V].

A decoration is pre-drawn at each depth it can appear at: the first record of `BRICK.DAT` has rectangles of 72 by 25, 48 by 17 and 32 by 11 pixels for entries 1, 2 and 3. Nothing scales it [V].

#### 5.12.7 Doors

A door is a wall type with flag bit 3 (section 5.12.2). The block map only draws its frame (set 3). The leaf is a separate shape, drawn by `DrawSlotDoor` (ovl4:1411) for slots 0 to 14 [V].

`LoadDoorShapes` (ovl3:03A2) cuts three shapes for each of up to two door styles from the `DOOR` picture, one for each row of distance (`gDoorShapes`, six far pointers, ds:FB5E), and the same for a button shape for each (`gDoorButtons`, ds:A99C, with an x and a y). The level script names the styles in four bytes at offset 38: a style for columns 0 to 2 and optionally a second one for columns 3 to 5 (level 1 style 0 alone, levels 2 and 3 styles 0 and 1, levels 4 to 6 style 2, levels 7 to 9 styles 3 and 4, levels 10 and 11 styles 5 and 6, level 12 style 7; the `DOOR` picture has 8 styles). The door's family decides which three columns: `gDoorShapeSet` is 0 for the families starting at types 3 and 8 and 3 for the families at 13 and 18 [V].

The code is a switch on the dungeon level, one case for each door animation (the same code for 1 to 3 and 12, one for 4 to 6, one for 7 to 9, one for 10 and 11). A door's progress is its wall type minus its family's base, 0 to 4. Cases 1 to 3 and 12 move the leaf straight up by `progress * (8, 12, 18)` pixels for rows 0, 1, 2; cases 4 to 6 draw it twice, once mirrored, each clipped half sliding sideways by `progress * (4, 6, 9)`; cases 7 to 9 draw a back panel first and then the leaf; cases 10 and 11 draw two panels that move in opposite directions, the second by 3/8 of the first (the first by `progress * (7, 11, 14)`). Door families 1 and 3 (types 3 to 7 and 13 to 17) have decoration head FFh in `gWallDecorHead`, and for them the function also draws a button shape; families 2 and 4 (types 8 to 12, 18 to 22) have none [V]. [I] The two button families are the doors that open from a button and the doors that do not.

Before it draws, the function may narrow region 5 vertically for the column (`SetRegionColumnExtent`, ovl4:021E, from the per-level tables `gDoorClipTopRow` and `gDoorClipBottomRow`, ds:6F6D and ds:6F79) and afterwards it sets the region back to the visible columns. QUIRK ovl4:021E: those tables have 12 bytes indexed by level 1 to 12, so level 12 reads one byte past the end of the first [V, `src/EOV4.C:126`].

#### 5.12.8 Monsters

A monster is drawn from the 28-byte monster record (30 of them, ds:8A82, `struct Monster` in `src/EOB.H`). `DrawSlotMonsters` (ovl4:0D1B) is called for slots 0 to 14 when the cell's monster count byte is non-zero. It scans all 30 records for the ones in the cell and puts each in one of five slots by its position in the cell (`cellPos`, 0 to 3 for the quarters and 4 for the centre), ordered for the party's facing so that the far half is drawn first (`gMonsterDrawOrder`, ds:7001: 0 1 2 3 4, 2 0 3 1 4, 2 3 0 1 4, 1 3 0 2 4) [V].

**Shapes.** Each monster kind has six shapes cut from its sheet by one of three loaders (`LoadMonsterShapesA`, `B` and `C`, ovl3:0624, 069E, 0718), chosen by the level script, which names up to two kinds: the first goes to shape slots 0 to 5, the second to slots 18 to 23 (`gLoadedMonsterShapes`, ds:B802, 36 pointers; `FreeMonsterShapes(0, 0x24)` frees all of them on a level change). A monster with `altShapes` set uses the second kind. Level 1 has `kobold` (loader B) and `leech` (loader A); level 4 has `spider` (loader C, sheet `SPIDER1`) and level 12 `xanath` (C) and `golem` (B) [V]. Each loader cuts only six rectangles; the tables of loaders A and B (ds:6930, ds:69C0) have 18 entries (the loop count is 6: the other 12 entries, in three sizes, are not used by the loaders), the table of loader C (ds:6A50) only the six [V; the reason is [?]]. The six are 7 by 96 (56 by 96 pixels) in most sheets (loader A has two wide ones, 96 by 56) and 80 by 88 in loader C's. The shape numbers the draw code uses are 1 to 6: `gMonsterShapes` is ds:B7FE, four bytes before the loaded array, so shape `n` is loaded slot `n - 1` [V from the addresses].

**Which view.** The shape comes from a table indexed by walk phase, party facing and the monster's own facing (`orient`, 0 to 3): `gMonsterFrames` (ds:6FE1) or, for kinds with bit 1 of `MonsterType.m0E`, `gMonsterFramesMirrorStep` (ds:6FC1). A negative entry is the mirror image.

<!-- FIG gfx_monster_shrink -->

Shape 1 is the front, 4 the back and 2 and 3 the side in two walk phases, which I checked on `KOBOLD.CPS` through the loader's rectangle table [V]. The attack shapes are 5 and 6: in the nearest row (no shrink steps) a monster whose attack frame byte is FEh uses shape 5, and FFh shape 6. The two tables agree in phase 0. In phase 1 the table for most monsters swaps the two side shapes and keeps front and back as they are; the mirror-step table also mirrors the front and back shapes, so those monsters step with a mirrored picture instead of a second drawing [I from the tables].

**Size.** Shapes are not pre-drawn at three sizes. A shape is cut once at full size and shrunk by `BlitSpriteDefault` (17A6:0FFA; `BlitSpriteCga` and `BlitSpriteEga` are the same for those formats) once for each step in `gMonsterShrinkSteps[row]` (ds:6FBE): 0 steps for monsters 1 row ahead, 1 for 2 rows ahead and 2 for 3 rows ahead. One step keeps 2 of every 3 rows and, in every row, 2 of every 3 pixel columns (pixels 0, 1, 3 and 4 of each group of six), so a 56 by 96 shape becomes 40 by 64 and then 32 by 43 [V from the code, arithmetic mine]. The result goes to the 4,096-byte scratch block `gScratchBuf` (ds:DCFC), which holds the largest cut shape (80 by 88 pixels is 3,540 bytes); a second step shrinks that block in place, which works because the routine never writes ahead of what it has read [I]. The cut shape itself is never changed [V].

**Position.** The anchor is the bottom centre of the shape. The x and y come from five points for each slot (four positions in the cell and the centre, `gViewPosX` and `gViewPosY`, ds:1D61 and ds:1D63, indexed `(slot * 5 + position) * 2` and interleaved), the position being the monster's `cellPos` rotated into the party's frame by `gViewRelativePos` (ds:1A2B: 0 1 2 3, 2 0 3 1, 3 2 1 0, 1 3 0 2). For the cell straight ahead, slot 13, the four positions are at (66, 89), (110, 89), (61, 102) and (115, 102) and the centre at (88, 94); for slot 3, three rows ahead, at (80, 61), (96, 61), (78, 65), (98, 65) and (88, 63) (the table values plus 0x58 for x and 0x7C for y, which is what `DrawSlotItems` adds; `DrawSlotMonsters` adds 0x58 and 0x7F, three lines lower) [V]. The shape is then moved left by half its width, up by its height and by its own jitter (`drawOffset`, x in the high nibble and y in the low, re-rolled by `RandomiseMonsterOffsets`, 1414:0CE7) [V].

A monster that was hit (`flags` bit 1) is drawn through the remap row `gHitFlashRemap` (ds:1CD3), 0 for 0 and 15 (white) for every other colour on VGA, Tandy and EGA, and a CGA row (ds:1CE3) in CGA mode, with the remapped draw calls (`DrawShapeClippedRemapped`, or the flipped one). `FlashMonsterHit` (08E6:0DCA) sets the bit for exactly one `DrawViewFrame` and then holds the frame for one tick (section 5.12.12) [V].

#### 5.12.9 Items, thrown objects and the teleporter

**Items on the floor** (`DrawSlotItems`, ovl4:0AD0). The cell's item list is the ring of `ItemRecord`s starting at the head stored in the cell record (`prevLink` and `nextLink`); each item has a position in the cell (`cellPos`, 0 to 3, or 8 for a wall niche) and an icon number. The icon number goes through a byte table (`gIconPictureIndex`, ds:181F) to one of 37 pictures: rows 0 to 13 are the 14 pictures of the `ITEML1` sheet (large items) and rows 15 to 37 the 23 pictures of `ITEMS1` (small items, `gItemS1Icons`, ds:ED8A, which continues the 15 rows of `gItemL1Icons`), and each picture exists at four sizes, cut by `LoadItemSheets` (ovl2:0171) at y offsets 0, 24, 40 and 56 (`gCellDX`): 64 by 24, 48 by 16, 32 by 16 and 16 by 8 pixels (`gCellSize1`, ds:65B0) [V]. The size is a frame number chosen by row and by position in the cell (`gItemScaleFrame`, ds:6F98, four bytes a row): the farthest row uses frame 3 for positions 2 and 3 and draws nothing for positions 0 and 1 (-1); the next two rows use frame 2 and frame 1 for all four positions; the party's own row uses frame 0 for positions 0 and 1 and draws nothing for 2 and 3. So, as with decorations, the size at each distance is drawn art and not scaled [V]. A cell's items are drawn in list order, each at its own position plus a jitter (`gItemJitter`, ds:6FAE) taken from the item's index [V].

**Thrown items and spell missiles** (`DrawSlotProjectiles`, ovl4:19C4). Up to ten `Projectile` records (18 bytes, ds:DD2C) are in flight. A record with `active` 1 is a thrown item: its shape comes from the `THROWN` sheet (12 pictures, four sizes 32 by 24, 24 by 16, 16 by 16 and 8 by 8, `gThrownIcons`, ds:F2F8) when the item has a thrown picture for the side it is seen from, and from the `ITEML1` sheet otherwise; the side is whether the record's direction equals the party's facing, its opposite, or neither (then only the `ITEML1` picture is used). Any other record is a spell effect and takes its shape directly from `gThrownIcons`. A table (`gProjectileFlip`, ds:7262) says whether to draw it mirrored. They are drawn at table positions that depend on the slot, the row and the same rotated position (`gProjectileX`, `gProjectileY`, `gProjectileFrame`, ds:71A6, 71EE, 71FE) [V].

**The teleporter** (`DrawSlotTeleporter`, ovl4:1D20, for wall type 0x34 only, `src/EOV4.C:1032`): two columns of 13 placements of a small shape from `DECORATE` (`gTeleporterCells`, ds:F4BE, six cells) at x and y offsets from a table of byte pairs (`gCellBytes`, 20AB:0000); which of two shapes each column shows toggles with `gAnimWallPhase` (ds:FB78), which the effect timer flips (`CheckFarTable`, 1414:04B0). It is the only wall that animates by itself; doors animate by changing their wall type [V].

#### 5.12.10 The shape format

Every sprite the game draws is a shape: a cell cut out of an art page by one of two driver routines, `GetSpriteCellRle` (driver export 15) or `GetSpriteCell` (export 16), with arguments `(column, y, columns, rows, colour map)` in 8-pixel columns and lines. The cutter reads the rectangle from the page after the draw page (`NextPageSeg`: page 3 while `gDrawPage` is 2, which `LoadOverlayScreen` sets), so the sequence is always: load a picture into the art page, cut its rectangles (each cell is a separate memory block that outlives the picture), load the next picture. Both formats begin with the same four bytes [V] (`src/drivers/MCGA.ASM`, `DrawShape` at 1CAB, `GetSpriteCell` at 0DD9, `GetSpriteCellRle` at 0C8D):

<!-- FIG gfx_shape_format -->

The size record structs in `src/EOB.H` (`SlotSize` and `RectKind`) are this header read through other names: byte 1 is the height in lines and byte 2 the width in 8-pixel columns. `DrawShape` clips the shape to the current region (`SetRegion`, in columns and lines) and draws colour 0 as transparent; `DrawShapeFlipped` writes each row right to left; the remapped forms send each 4-bit pixel through a 16-byte row set by `SetRemapTable`. The wrappers in `src/E082C.C` (`DrawShapeClipped` and its three siblings) round x down to an even number and subtract the region's left and top; the front ends in `src/E1A29.C` they call select the region if it is not the current one and add the same origin back. The net effect is that the coordinates stay screen coordinates and the region only sets the clip [V].

A driver that was never used: `DrawShapeMasked` and its mirror (driver slots 27 and 28, front ends 1A29:0107 and :0148) draw an 8-bit shape "behind the scenery planes of page 4", where page 4 holds up to seven bit planes of 8,000 bytes with a plane set where scenery at that depth covers the pixel (`src/drivers/MCGA.ASM:2133`). No caller exists in `src/` (a search of all of `src/` for the front ends finds only their definitions, and a byte search of `EOB.EXE`, `INTRO.EXE` and the overlay images for a far call through ds:809A or ds:8096 finds only the front ends' own two) and the three non-VGA drivers return at once. Depth ordering by painter's algorithm made them unnecessary, or they belong to an earlier design of the view [H] [V for "no caller"].

#### 5.12.11 What hides what

Painter's order is not the only occlusion. Before each slot is drawn, `ComputeViewSpan(slot, ...)` (ovl4:0097) clips region 5 to the columns of the view that the nearer walls do not cover. It starts with the whole width (columns 0 to 22) and walks all 18 cells: for each cell whose front wall has art it applies a limit from a table indexed by slot and cell (`gSpanWallClip`, ds:6B82, 18 by 18 signed bytes; a positive value pulls the right edge in, a negative one the left edge, -40 means "no effect" and -41 "nothing of this slot can be seen"), and for door cells it uses a table of left and right bounds (`gSpanDoorClip`, ds:6CC6). If the right edge falls below the left, the slot is skipped, decorations and all. The result is cached per slot for the frame (`gViewSlots`, ds:DCB2) and written to region 5 with `SetRegionRect`, so every `DrawShapeClipped(2, ..., 5)` of the slot is clipped to it [V]. For slots 0, 6, 7 and 11 (the corners of the two far rows) every entry of the table is -41 except entry 16 (the party's own cell), which is -40, so these slots are drawn only when no cell of the view other than the party's own has a front wall with art (a door is exempt); for the other slots the table's diagonal is -40: a cell's own front wall never narrows its own span [V from the table].

Two limits of the code as read. The table skips a cell's own front wall, and `DrawSlotMonsters` tests no wall flag; items and projectiles are suppressed by the wall type on their cell's front side, monsters are not. How a monster in the cell directly behind a solid wall stays hidden is not shown by the code I read [?]. The table of limits is also hand-made rather than derived: it says which cells hide which at 8-pixel granularity.

#### 5.12.12 Effects drawn over the view

There is no special effects layer. A spell in flight is a `Projectile` record (section 5.12.9); the effects below are drawn by the code that causes them, straight on page 2 and the screen.

- **Hit flash on a monster** (`FlashMonsterHit`, 08E6:0DCA): one frame with the monster in white (remap row 1CD3). When several monsters are hit at once (`gDeferMonsterHits`, a turn-undead or a cone of cold), `FinishDeferredHits` (08E6:0E48) draws one frame with all the marked ones flashed and then removes the dead.
- **Damage burst** (`FlashViewDamage`, 0EF3:06E7): three explosion shapes (16 by 16 pixels, `gFlashFrames`, cut from the `DECORATE` sheet) on 16 cells at fixed places of the view, 11 passes of 2 ticks each, a wave coded as one 32-bit word of 2-bit frame numbers a pass [V].

<!-- FIG gfx_damage_flash -->

- **Member flash** (`FlashPartyMembers`, 0EF3:0665): eight passes of the same shapes over a party member's portrait, drawn straight on the screen at a base position plus four offsets (`gFlashBaseX`, `gFlashBaseY`, `gFlashDelta`).
- **Compass and the palette change** are in 5.12.1 and 5.13.7. `LoadLevel` ends by setting the palette-pending flag; the new palette appears on the first view frame.

#### 5.12.13 The view in each graphics mode

The view code is one program for all four modes. The differences are in the data and in four routines: the file set (`.VCN` and `.VMP` for VGA, `.ECN` and `.EMP` for the rest), `LoadWallTextures` (EGA copies the blocks into video RAM, CGA converts them), `DrawTile` (a path for each mode) and the monster shrinkers (`BlitSpriteDefault` for Tandy and VGA, `BlitSpriteCga`, `BlitSpriteEga`, selected by `gGraphicsChoice` in `DrawSlotMonsters`). The shape cutter and `DrawShape` are driver exports with the same call in every mode, so the sprite code does not know which mode it runs in. The comparison table in section 5.13.11 puts the four side by side. The only mode with a palette of its own per level is VGA: the other modes have a fixed 16-colour palette and the walls carry the colours themselves (`.ECN` tables are zero).

### 5.13 Graphics output

The game draws a 320 by 200 screen through 47 driver slots (section 2.5 lists them). This section is about how the game uses them: where it puts things, in which units, through which pages, and what it does about the mouse pointer, the palette and text. The code is in the drawing helpers of `src/E082C.C`, the region front ends `src/E1A29.C`, the text console `src/E1DE7.C`, the pointer `src/E1BBE.ASM` and `src/E1BB1.ASM`, the character screen `src/E1547.C` and the loaders `src/E1B82.C` and `src/EOV2.C`.

#### 5.13.1 Pages

The game does not draw on the screen unless it has to. It owns eight page slots, a table of eight segment words (`gPageSegs`, ds:FC0C) that `StartupGame` (07C8:0199) fills for the graphics choice (figure below). Slots come in pairs, the even one the video-memory side and the odd one the main-memory side, and for most choices the two name the same segment. A page number in a driver call (0 and 2 are the ones the game uses to draw) is a slot number, and the driver turns it into a segment (`PageSeg`, MCGA.OVL 0B3F). `gDrawPage` (ds:8115) names the page the drawing routines write to; the code sets it to 0 to draw on the screen, to 2 to compose off screen [V].

<!-- FIG gfx_pages -->

The roles are fixed by the code, not by the table [V]:

- Slot 0 (and 1) is the screen: A000 for VGA and EGA, B800 for CGA and Tandy.
- Slot 2 is the draw page: 65,000 bytes on VGA (a screen is 64,000), the page the view and the party blocks are composed on. Slot 3, its memory twin, is the art page (`gArtPage`, ds:EFC2): `LoadPacked` unpacks pictures into it, and the sprite cutters read it (section 5.12.10).
- Slot 5 is the staging page (`gStagingPage`, ds:DF4E): the raw packed file is read there before it is unpacked, level scripts and the maze are read there, the character-swap buffer and the saved screen strips live there.
- Slot 7 is `gWallBlocks` and also the unpack work buffer (`SetUnpackBuffer(gPageSegs[7], 0xFC00)`, `StartupAlloc`); in modes 0 to 3 the block is 52,000 bytes, which is below the 0xFC00 the call states, a gap that matters only to the unpackers that need the whole work buffer (format 2, which no pinned file uses).
- Slots 4 and 6, the even sides of the staging and wall-block pairs, differ from their twins only in EGA mode (A200). The game reaches those buffers through the odd slots (`GetPageBuffer(5)`, `(7)`); CGA and Tandy `LoadScreenImage` with mode 2 use page 4 as a conversion target before copying to page 2 [V].

CopyPage (19EC:002A) copies whole pages by slot, and does nothing when the two slots have the same segment; `CopyRegion` (19F3:000E) copies a rectangle between pages, `(srcCol, srcY, dstCol, dstY, cols, rows, srcPage, dstPage)`, in 8-pixel columns and lines. The two "extra" arguments, which `src/E19F3.C` records as roles not established, are the source and destination page numbers: the callers read pass `2, 0` for a copy from the draw page to the screen, `0, 2` for the reverse, `2, 2` within page 2 and `gDrawPage` twice for the text scroll, and the MCGA driver's own header names them (`MCGA.ASM:327`). The front end clips the request: widths over 40 columns and heights over 200 lines are capped, a negative origin moves the other origin and the size by the same amount, and a request starting at column 40 or beyond returns without calling (`src/E19F3.C`) [V].

<!-- FIG gfx_page_traffic -->

There is no page flip. EGA.OVL has a `SwapPageWithScreen` that would flip (INT 10h function 5), but no call of any driver's slot 26 exists in the game [V]. A frame is shown by a rectangle copy, as in 5.12.1; the party blocks are shown the same way (5.13.3); the pointer is drawn on the screen over whatever is there.

#### 5.13.2 Regions and how the game addresses the screen

The game keeps 14 rectangles in a table of 16-byte records (`gRegions`, ds:009F, `struct Region`): left, top, width and height (the left and width in 8-pixel columns, the top and height in lines), plus four saved words of text state. A region is both a clip rectangle for shapes and a text window. `SelectRegion` (1A29:0189) makes one current and tells the driver (`gSetRegion`); every shape front end selects its region first when it is not the current one, adds the region's origin to the coordinates it is given, and calls the driver. So shape coordinates in the game's code are relative to a region (`src/E1A29.C`, `src/E082C.C`) [V].

| Region | left, top, columns, lines | Pixels | Seen used for |
|---:|---|---|---|
| 0 | 0, 0, 40, 200 | the screen | all drawing by default (`SetTextWindow(0)` at start-up) |
| 1 | 8, 72, 24, 56 | 64..256 x 72..128 | a panel window [?] |
| 2 | 19, 64, 20, 128 | 152..312 x 64..192 | `SetTextWindow(2)` in `src/EOV0.C` |
| 3 | 29, 120, 8, 64 | 232..296 x 120..184 | [?] |
| 4 | 2, 24, 20, 120 | 16..176 x 24..144 | `SetTextWindow(4)`, `src/EOV6.C:319` |
| 5 | 0, 0, 22, 120 | 0..176 x 0..120 | the 3D view; clip of every view shape |
| 6 | 10, 168, 21, 24 | 80..248 x 168..192 | `SetTextWindow(6)`, `src/EOV2.C:364` |
| 7 | 1, 179, 34, 18 | 8..280 x 179..197 | the message window of the play screen |
| 8 | 2, 24, 20, 0 | 16..176 x 24 | [?] |
| 9 | 1, 125, 38, 63 | 8..312 x 125..188 | `SetTextWindow(9)`, `src/E0DE8.C:450`, `src/EOV5.C:1221` |
| 10 | 0, 0, 22, 144 | 0..176 x 0..144 | the camp menu (`SetTextWindow(0xA)`, `src/EOV1.C:1482`) |
| 11 | 1, 20, 20, 56 | 8..168 x 20..76 | `SetTextWindowAndClear(0xB)`, `src/EOV1.C:941` |
| 12 | 1, 4, 20, 160 | 8..168 x 4..164 | `SetTextWindow(0xC)`, `src/EOV5.C:1078` |
| 13 | 1, 25, 38, 100 | 8..312 x 25..125 | the word panel (`SetTextWindow(0xD)`, `src/EOV2.C:473`) |

(The initial values of ds:009F read from the load image [V]; code can change them at run time: `SetRegionRect` rewrites region 5 for every slot, and `SetRegionExtents` any region.)

The units are mixed on purpose. The driver's rectangle calls take pixels for fills, lines and text (`gFillRect`, `gDrawLine`, `gDrawText`), and 8-pixel columns for the copies, the save and restore of rectangles and the cell cutters, because the EGA planes and the CGA bytes cut on 8-pixel and 4-pixel boundaries [V from section 2.5]. A shape is the exception: its x is a pixel position but is rounded down to an even number by the clipped front ends.

#### 5.13.3 The play screen

<!-- FIG gfx_screen_layout -->

The play screen is the `PLAYFLD` picture on page 0 with the 3D view top left and six party blocks on the right: two columns, three rows, at x 184 and 256 and y 2, 54 and 106 (the tables at ds:5D1C to 5D4F; the hit rectangles are screen nodes of 63 by 50 at the same places) [V]. `EnterMapScreen` (0ACD:0F72) builds it: it loads the level, loads `PLAYFLD` into the art page and copies that to the screen (`LoadScreenImage("PLAYFLD", ..., 5, 3, 0)`), loads `INVENT` the same way into page 2, selects the 6 by 6 font, redraws the party and the view and opens text window 7 [V].

A party block is composed off screen and then copied. `RedrawMember` (1547:0991) in plain mode first copies two pieces of background from page 2 (`CopyRegion(0x16, 0xA8, x, y, 8, 0x18, 2, 2)` and the same for the lower 0x1A lines, from columns 22 and 30 at line 168; [I] the panel art of the `INVENT` picture, which `EnterMapScreen` leaves in page 2) to the block's place at x 8 or 80 on page 2, sets `gDrawPage` to 2, draws the name (6 by 6 font, colour 0Ch or 8 when poisoned or paralysed, on the panel fill), the portrait, the two hand slots, the HP bar and the status frame, restores `gDrawPage`, and copies the finished block to the screen at x + 176 (`CopyRegion(x >> 3, y, 23 or 32, y, 8, 0x32, 2, 0)`). Each drawing routine adds 0xB0 to x while `gDrawPage` is 0, which is how the same routine draws a block either on page 2 or on the screen. The panel and inventory modes do the same with a 144 by 168 piece, keep the pristine panel in the right half of page 2 (`CopyRegion(0x16, 0, 0, 0, 0x12, 0xA8, 2, 2)` makes the backup) and restore it afterwards [V]. [I] The left part of page 2, where the view is composed, doubles as the working area for these blocks and backups, so the old view cannot be assumed to be there; `DrawViewFrame` rebuilds all of it each time anyway.

A block is 64 by 50 pixels: the name at (x + 2, y + 2), the portrait 32 by 32 at (x, y + 9), the two hand slots of 31 by 16 at (x + 32, y + 9) and (x + 32, y + 25), the HP line at (x + 15, y + 44), 38 wide and 3 high, with the label `HP` 13 pixels to its left, and a status frame of four lines around the block (colours depend on poison, paralysis, and spell states, `DrawMemberFrame`, 1547:0009) [V]. The block shows the hit-point bar in three colours picked from the wound state (`gHpBarColours`): the bar is `hp + 10` over `maxHp + 10`, the 10 keeping it visible at negative hit points, and the "wounded" colour starts at a third of the maximum [V].

The move buttons, compass, message window and CAMP button are drawn by the picture itself; the program only keeps their hit rectangles (the 38 nodes of `gTable1b`, section 5.13.4) and redraws the compass shapes when the facing changes. The view has nodes too: two 88 by 18 and two 88 by 29 boxes at the bottom (y 102 and 72), two 88 by 48 boxes above them and one 128 by 96 box over the centre. [H] They are the clickable zones for floor items and wall buttons (`ClickFloorItem`, `ActivateWallAhead`, `src/E0ACD.C`).

#### 5.13.4 Panels, frames, lines and screen nodes

Every box in the game is built from lines and filled rectangles, not from pictures [V] (`src/E082C.C`, `src/E0D1C.C`, `src/E1A1F.C`):

- `DrawRect(x1, y1, x2, y2, colour)` (082C:04C4) draws four lines with no clipping; `DrawBox` (1A1F:000C) clamps the corners to 0..319 and 0..199 first.
- `DrawFilledPanel(x1, y1, x2, y2, colourA, colourB, fill)` (082C:09D5) is the bevel: it fills the interior unless `fill` is -1, draws the top and right edges in `colourA` and the left and bottom in `colourB` (the top starts one pixel in and the right ends one up, so the corners are not painted twice).
- `DrawFramedPanel(x, y, w, h, first, second)` (0D1C:0C21) draws that twice, one pixel apart, for menu boxes and buttons: an outer frame (flat in the fill colour when `first` is set, a bevel otherwise) and an inner bevel (hollow when `second` is set). It hides the pointer while it draws (`LockHidePointer`).
- `DrawRatioBar` (1547:028C) is a sunken panel (the bevel colours swapped) with a filled and an empty bar side by side; it is the HP and food bar.
- `DrawShadowText` (082C:0898) draws a string four times: in colour 0Ch at x - 1, at y + 1 and at both, and then in the wanted colour at (x, y). Colour 0Ch is black on VGA (entry 12 of `EOBPAL.COL` is 0, 0, 0).
- `DrawBevelBox` (082C:074D) has no caller.

The colours the UI uses are not constants: `SetUiPalette` (ovl2:0937, called once by `LoadItemData`) sets eleven variables for the graphics choice: on VGA the panel fill is 84h, the highlight 82h, the shadow 87h, the empty bar 85h, the alternative fill B4h, the unselected tab colours B1h and B8h and the action-label colours 11h, 17h and 14h; on CGA, Tandy and EGA they are 2, 9, 0Dh, 2, 0Eh, 0Fh, 0Dh and 6, 0Dh, 8 [V]. The UI indices are the same in every level palette (5.13.7).

Screen nodes are the other half of the interface (`struct Node`, 60 bytes, `src/E1658.C`, `src/E16A5.C`): each is a rectangle in a region with a key, a kind, flags and one to three shapes or callbacks, and `DrawNode` (16A5:07C3) draws it in the state it is in. They are linked into lists the key dispatcher walks (`gList`, `BuildList1` to `BuildList5`, `src/E0CF7.C`; the play screen's list has 38 nodes). A mouse click is matched against the nodes' rectangles, and the node's key goes to the game as though it were a key press: the nodes decode the 3D view's hit zones, the move buttons (21 by 16 at x 3, 24 and 45, y 128 and 144) and the party blocks [V for the rectangles and the walk; what each key does is outside this section].

#### 5.13.5 Text and fonts

<!-- FIG gfx_text_window -->

Text is drawn in two layers. The driver draws one string at a pixel position in the current font (`gDrawText`, export 33: each character through `DrawGlyph`, CR starts a new line at x, a character past the right edge wraps, the text colour sets bits and a non-zero background colour clears the others; background 0 is transparent) [V]. The text console in `src/E1DE7.C` builds on it.

**Fonts.** `FONT8.FNT` and `FONT6.FNT` are loaded whole at start-up (`LoadFont`, 1A47:0005, which validates the length against the header: `0x104 + 128 * height * ceil(width / 8)`); `SetFont` (1A47:0169) makes one current and caches its width and height from bytes 103h and 102h. Both are fixed-width: 8 by 8 and 6 by 6, 128 glyphs. The 8 by 8 font is current at start-up, but the play screen runs on the 6 by 6 font (`EnterMapScreen`, `StartupAlloc`), so the message window, 272 pixels wide and 18 lines high, holds 3 lines of 45 characters [V].

**The window.** `SetTextWindow(n)` (1DE7:0009) makes region n the text window: it saves the cursor and the two colours of the window it leaves into that region's record (`textAttr1`, `textAttr2`, `textCol`, `textRow`) and loads those of the new one, so each window remembers its own cursor and colours. The colours are `gTextAttr1` (text, default 0Fh, white) and `gTextAttr2` (fill, default 0Ch). `ClearTextRegion` (1DE7:0344) hides the pointer, fills the window with `gTextAttr2` and homes the cursor [V].

**`DrawText(string)`** (1DE7:060A) accumulates a word, a line and a column, and flushes lines through `DrawTextLine` (1DE7:03B2), which draws the line at the cursor. When the next line would pass the bottom it scrolls the window by one line: `CopyRegion` of the window up by the font height, refill of the last line with `gTextAttr2`, draw. It then returns the cursor to column 0 or advances it by the string's length, wrapping when the text no longer fits. Control bytes inside the string (figure) change colours or position in the middle of a message; a byte with bit 7 set ends the string [V]. Word wrap is by words: a word that would pass the right edge starts the next line.

#### 5.13.6 The mouse pointer

The pointer is not the hardware cursor of the mouse driver. The game takes the INT 33h mouse position and draws its own pointer shape on page 0, restoring what was under it first. `InitMouse` (1A72:00DA) installs an event handler (1A72:017A) for moves and left and right button changes; `SetMousePointerShape` (1BB1:0002) gives the pointer a shape and a hot spot (a 16 by 16 item icon with the hot spot at 8, 8 when the mouse carries an item, the default arrow with 0, 0 otherwise, `SetMouseItem`, 07A9:010C). The pointer and the save-under buffer are allocated to the shape's size by `SaveRectSize`, columns * rows * 8 bytes: the pointer rectangle is the shape's width in columns plus one (the shape may begin between two columns) [V].

<!-- FIG gfx_pointer -->

When the mouse moves, the handler (if no drawing routine holds the busy flag) saves the driver's drawing state, hides the pointer (the saved background goes back to the screen), moves the drawn position to the new mouse position, saves the screen under the new rectangle and draws the shape. Each routine that draws on page 0 brackets its work with a hide and a show: `HidePointerZone(left, top, right, bottom)` and `ShowPointerZone` (1BBE:01FE, 02C2) take the rectangle that is about to change, hide the pointer only if its rectangle overlaps, and set a "redraw owed" flag the show reads; `LockHidePointer` and `LockShowPointer` are the same without a zone. A hide count (`gHideDepth`, ds:8146) lets hides nest. While a zone hide is on the mouse handler leaves the pointer alone inside the zone (QUIRK 1A72:0248: the zone's left edge is never tested, so a pointer at the left of the zone is also treated as outside). That is why nearly every drawing routine in the game, text, panels, party blocks, the view copy, is wrapped in `HidePointerZone` or `LockHidePointer` [V]. EGA's driver keeps its registers and latches in a shadow that `SaveDrawState` and `RestoreDrawState` swap around each pointer redraw, so the pointer can be drawn in the middle of another draw (section 2.5) [V].

#### 5.13.7 The palette

Only VGA has a palette the game loads at run time. At start-up the VGA choice loads `EOBPAL.COL` (768 bytes of 6-bit red, green, blue, in `EOBDATA2.PAK`) into `gPaletteBuf` and calls `gSetPalette`, which loads all 256 DAC registers by INT 10h AX=1012h. Tandy and EGA call `gSetPalette` with a built-in table (ds:3057, length not established); CGA does not set one (its driver's `SetPalette` is a stub, and CGA's colour comes from the dither tables). `LoadLevel` then reads `<set>.PAL` over `gPaletteBuf` and sets `gLevelPalettePending`; the next `DrawViewFrame` loads it after the sprites are drawn and before they are copied to the screen, so no frame is ever shown in the wrong palette (5.12.1) [V].

<!-- FIG gfx_palette_flow -->

The five `.PAL` files are copies of `EOBPAL.COL` with a few entries changed: 6 entries differ for BRICK (indices 29h, 68h, 6Ah and D0h to D2h), 33 to 41 for the others, most of them at D0h and above, where the wall tables point; the interface colours the code names (82h, 84h, 85h, 87h, B1h, B4h, B8h and 11h, 14h, 17h) are the same in all of them (checked by comparing the low six bits of every entry). `EOBPAL.COL` is therefore level 1's palette, apart from six entries, and not a separate interface palette. The `.PAL` bytes carry 11b in their two top bits for many entries (values up to 0FFh), where `EOBPAL.COL` stays below 40h; [H] the DAC takes only the low six bits, which is what makes the two files agree [V for the data, H for the hardware].

There is no fade in or out in `EOB.EXE` and no gradual palette change at all: a palette is loaded whole or not at all, and nothing in `src/` of `EOB.EXE` changes entries one at a time. Screen changes in the game proper are instant. `INTRO.EXE` has its own dissolves and palette loads (`IOV0.C`, `LoadPalette` and `Dissolve`) [V for EOB.EXE by `grep SetPalette`; the intro is outside this section]. What looks like a flash is a remap (5.12.8): the flash of a hit monster is a draw with a different 16-colour row, never a palette change [V].

#### 5.13.8 Pictures and sheets

Every full-screen picture and every sprite sheet is a packed 320 by 200 file read by `LoadScreenImage` (082C:051B) and `LoadPacked` (1B82:0041).

<!-- FIG gfx_load_image -->

`LoadScreenImage(name, map, srcKind, dstKind, mode)` builds `name.CPS` on VGA and `name.EGA` otherwise, hands it to `LoadPacked`, which reads the 2-byte length and the packed block into page buffer `srcKind` and unpacks it into `dstKind` by its format byte (section 4.2), and copies the page when `mode` is not -1. The art is always one byte a pixel when it comes out of the unpacker, in every mode; for CGA the copy to the screen goes through the dither tables, built from the 32-byte `map` (section 2.5); [I] for Tandy and EGA the driver's page copy converts it to the screen format [V for the art, I for the conversion]. In the pinned data, 47 of 47 `.CPS`, 42 of 48 `.EGA`, 20 of 20 `.CMP`, the 12 `.INF` and the 10 wall files are format 4; 5 `.EGA` (the wall-set pictures, e.g. `BRICK1.EGA`) are format 1 and one is stored; the palette-length field of all 137 files is 0 [V, scan of the six PAKs, 2026-11].

`LoadOverlayScreen` (082C:05EC), despite its name (it has nothing to do with code overlays), loads a picture into page 3 without showing it and sets `gDrawPage` to 2. It is the way sprite sheets are loaded: `LoadItemSheets` (ovl2:0171) loads `ITEML1`, `ITEMS1`, `THROWN` and `ITEMICN` one after another and cuts every cell out of each before the next replaces it. A sheet is therefore alive only while its cells are being cut, and the cells stay in memory as shapes [V].

#### 5.13.9 Portraits, hand slots, item icons

All of these are shapes (5.12.10) cut once and drawn many times [V].

- **Item icons.** `ITEMICN` is a sheet of 88 icons of 16 by 16 pixels, 20 to a row (`gItemIcons[i]` is the cell at column `(i % 20) * 2`, line `(i / 20) * 16`, 2 columns by 16 lines; slot 0 is the default mouse arrow, slot 89 holds the recoloured copy for the pointer). `DrawItemIcon(page, item, x, y)` (0D1C:070F) draws `gItemIcons[item.icon]`; when the item has flag 0x80 and bit 0 of `gPartySpellFlags` is set it first loads a 16-colour row from `ITEMRMP.VGA` or `.EGA` (1,600 bytes, 100 rows of 16, `gItemRemapTable + (icon << 4)`) and draws the icon through that remap, so those items appear in other colours. What the state bit means is not recorded in the code [?].
- **Hand slots** (`DrawHandSlot`, 1547:07E7): a 31 by 16 bevelled panel (`DrawFilledPanel` in the UI highlight and shadow colours), the item icon at x + 8, or the empty-slot icon for that hand, or, when the last action of that hand gave a result, the result label (`DrawActionLabel`: the thrown-icon shape and the words MISS, HACK, CAN'T REACH, NO AMMO or a number). A shade icon (`gHandShadeIcon`) is laid over it when the hand cannot be used or the member cannot act.
- **Portraits.** The 32 by 32 portrait of a member (`gChars[i].portrait`, a 1,300-byte block, 0x514) is a `GetSpriteCellRle` shape cut at `LoadGame` or character creation from the sheet `CHARGENA`: portrait `i` is the cell at column `(i % 10) * 4`, line `(i / 10) * 32`, 4 columns by 32 lines. A negative portrait index takes the character's face from the `OUTTAKE` sheet at line 160 (`src/EOV3.C:884` to `:907`). A dead member draws `gDeadPortraitIcon` and a member at 0 hit points or less gets `gUnconsciousIcon` over the portrait [V].

#### 5.13.10 Timing and effects

Nothing in the game paces itself by frames except the menus. Time is the INT 1Ch tick counter, 18.2 Hz: `WaitTicks(n)` (082C:0425) is a busy wait on `gTicks`, and the effects (the damage burst, the member flash, hit flashes) use it at 2 ticks a step. `WaitFrames(n)` (1A16:000B) polls the vertical retrace of the display through port 3DAh (`WaitRetrace`, 1DE4:000D); it paces blinking selections and menu animations (5 to 10 frames). Whether the retrace bit reads high or low during the retrace depends on the adapter, so start-up (`CalibrateRetrace`, 1A65:000E) waits for the BIOS tick to turn over, waits four more ticks (about 220 ms), samples the bit 10,000 times and sets `gRetraceInverted` when it read set more often than clear [V].

The visible effects are those of 5.12.12, a text cursor that blinks every 2 ticks in the text field editor (`EditTextField`, 082C:000B), the selection flash of `BlinkText` (14EF:0078) and the menu list highlights. None changes the palette.

#### 5.13.11 The four modes

<!-- FIG gfx_mode_compare -->

The comparison shows what differs where the game's own code can see it. The pixel format, the line layout and the palette are the driver's; the wall file set, the wall block conversion, the tile draw and the monster shrinker are the game's, one path per mode. The user-interface colour set is a table of eleven values (5.13.4). The hit-flash row differs only for CGA, whose row (ds:1CE3) holds other values (0, 255, 255, 255, 15, 176, 178, 180, ...) which the CGA driver's remap reads in its own pixel format ([?] how); every other mode uses the row of 0 and 15 (ds:1CD3) [V for the rows].

### 5.14 Sound

The game has three sound drivers and one way to call them. `START1.EXE` asks "What type of sound do you wish to have?" and offers four answers (`strings START1.EXE`): "1. Adlib or Sound Blaster", "2. Built in speaker", "3. Tandy speaker", "4. No sound". The digit travels through `INTRO.EXE` to `EOB.EXE` as `argv[2]` (section 2.6). `Main` (07C8:0001) stores `gSoundCard = digit - '1'` and `gSoundOn = (digit != '4')` (`src/E07C8.C:223-224`). `StartupGame` then starts the driver the card names (`src/E07C8.C:403-431`): card 0 calls `StartAdlibDriver` (0000:3F77); card 1 calls `SoundDriverInstall` (1948:0377); card 2 calls `SoundDriverInstall`, sets `gTandyOn` and `gTandySelected` to 1 and calls `SoundDriverResume` (1948:0458). **[V]** Nothing probes the hardware (section 2.7): "Sound Blaster" works because a Sound Blaster has an OPL2 at the same ports, 388h and 389h **[H]**.

<!-- FIG snd_pipeline -->

Four data files feed the drivers. Each has the layout of section 4.4 (a 120-byte map, then the block: at most 32C8h bytes for the game, 36B0h for the intro) and is read once, at start-up:

| File | Archive | Bytes | Read by | For |
|---|---|---:|---|---|
| `ADLIB.DAT` | EOBDATA1 | 7,489 | `LoadSoundDriver` (177D:000A), card 0 | the game on AdLib |
| `PCGAMESN.DAT` | EOBDATA1 | 4,757 | `LoadSoundDriver`, cards 1 and 2 | the game on the speaker or Tandy |
| `SOUND.DAT` | EOBDATA4 | 13,139 | `INTRO.EXE`'s `StartupGame` (0892:0241), card 1 | the intro on AdLib |
| `PCSOUND.DAT` | EOBDATA1 | 1,990 | the same, cards 2 and 3 | the intro on the speaker or Tandy |

(`INTRO.EXE` numbers the cards from 1: it keeps `atol(argv[2])`, `src/I0892.C:213`, where `EOB.EXE` subtracts `'1'`.) The block is read straight into the program's own code segment, at `cs:0416`, because the AdLib driver runs with DS = CS (section 2.1.2). `LoadSoundDriver` then calls `RebaseDriverBlock` (0000:03FA), which adds 0416h to the first 300 words of the block so that file offsets become near pointers in segment 0. The speaker driver reads its songs from the same place. **[V]**

#### 5.14.1 What the game calls

Game code never talks to a driver. It calls one of four functions in `src/E177D.C` (the unit at 177D:000A, 646 bytes) and gives a *game sound number* from 0 to 119.

- `PlaySound(n)` (177D:0070): if `gSoundOn`, look up `gSoundMap[n]`; a driver number of 96h or more is ignored; otherwise play it now. On AdLib "now" means `QueueSound(id)`. On the other cards it means `StartSound(GetSoundData(id AND 7Fh))`, which restarts the one song stream. About 59 call sites in `src/` (a `grep`).
- `PlaySoundAtCell(n, cell)` (177D:0106): a sound that comes from a map cell, so it can be quieter with distance. It does **not** play `n` (below).
- `PlaySoundDefault(n)` (177D:00F3): `PlaySoundAtCell(n, gPartyCell)`, for sounds that come from the party itself.
- `SayAndPlaySound(text)` (082C:0B8F): prints the text, then `PlaySound(4Fh)`, the refusal sound. 13 call sites, all in the spell and item code (`src/E0EF3.C`).

Sound numbers come from literals, from tables (`gSpells[action].sound`, `gEffectSound`, `src/E0EF3.C:697`, `:1986`), from the monster type records (fields `m16` and `m17`, `src/E08E6.C:287`, `:418`) and from level scripts: opcode `PlaySoundOp` (122E:1194) carries a sound byte and a cell word and calls `PlaySoundAtCell` when the cell is not 0, `PlaySound` when it is. **[V]**

The map is the first 120 bytes of the data file. On AdLib 116 of the 120 game numbers lead to a sound (2 map to 0, 2 hold FFh); on the speaker only 66 do (26 map to 0, 28 hold values of 96h or more), so a speaker player hears about half of the game's sounds and not the character generation music (game sound 20 maps to 0). **[V]** (`ADLIB.DAT`, `PCGAMESN.DAT` decoded with `tools/re/pak.py`, 2026-11-05.) Some numbers the code names:

| Game number | Use | Evidence |
|---:|---|---|
| 0 | silence: the map holds 0 | `PlaySound(0)` when a level is entered queues driver sound 0, a script that kills voices 0 to 8 (`src/EOV3.C:651`); `PlaySoundDefault(0)` is the flush below |
| 20 (14h) | the music of character generation | `PlaySound(0x14)` (`src/EOV0.C:224`, `:1432`, `src/EOV5.C:1073`) |
| 21, 22 (15h, 16h) | a hit on a character; a character's death | `src/E0DE8.C:488-490` (22 plays when hit points fall to -10 or less) |
| 30 (1Eh) | an ambient noise on levels 1 to 3 | `RunMapLoop` (`src/E0ACD.C:941`): at a cell 0 to 11 along the row from the party, every 4 to 13 times 18 ticks (about 4 to 13 s) |
| 79 (4Fh) | "refused" | `SayAndPlaySound` (`src/E082C.C:603`) |

##### The deferred call

The three `AtCell` and `Default` calls share a quirk that is the design of the function [V]: **`PlaySoundAtCell` plays the sound the previous call recorded, then records its own arguments.** The new sound waits in `gCurSound` (ds:5E22) and its distance in `gCurAttenuation` (ds:5E24). The first call after start-up plays nothing. Game numbers that map to driver sound 0 are never played by this path (the test is `gCurSound > 0`), and that is how the main loop flushes: `RunMapLoop` calls `PlaySoundDefault(0)` once per pass (`src/E0ACD.C:944`), which plays whatever was recorded and records silence. In the `load-walk` session (reproduced in the oracle, 2026-11-05; 26 seconds of it in the dungeon) the flush ran 5,954 times and 251 real sounds were recorded; two calls of `AtCell` in one pass push the first sound out at the second call, so one sound is held at most. A speaker card has no volume and no queue: it starts the recorded sound (still one call late) with no attenuation and no distance test.

<!-- FIG snd_atcell -->

On AdLib the play does more than queue. The distance is `Distance(gPartyCell, cell)` (17A6:0F61): a map cell is `y * 32 + x`, and the metric is the larger of the two differences plus half the smaller (an octagon). A `cell` of 0 means "no position" and gives 0. The recorded sound is dropped when its distance is 16 or more. Otherwise `PlaySoundAtCell` changes two bytes of the sound's own data, with `SetSoundByte(sound, offset, value)` (0000:03B0, which writes the byte at `offset` of the sound's data): **[V]**

- offset 1 is the priority byte of the sound's header (`ProcessSoundQueue` reads it from the second byte of a sound, 0000:402C); it becomes `20h - distance`;
- offset 3 is the operand of the first script command. In 141 of the 149 sounds of `ADLIB.DAT` that command is `ABh`, which sets the voice's second level offset (+30h in the record), and `VoiceCarrierLevel` adds that offset to the carrier's total level; it becomes `L + ((3Fh - L) * (distance - 1) >> 4)`, with `L` the file's value. A level register counts attenuation (3Fh is the quietest, section 2.7), so distance makes the carrier quieter, in steps up to 55 at distance 15 for the usual `L` of 5.

`src/E177D.C` calls these "register 1" and "register 3" and says what the first one means is not known; the header layout and the command table settle it. The sound's own bytes are restored on the next play from copies kept in `gSoundReg1` (ds:5E2A) and `gSoundLevel` (ds:5E28), so the file's data is intact between plays. `gBusyCountdown` (0000:3B36) guards the swap: set to 0Bh after a start, counted down by the timer interrupt, and waited out at the next call so that the driver has read the patched bytes before they are put back. **[I]** (11 passes against a driver tick every 12: a margin of one pass.)

#### 5.14.2 The AdLib data: a script for each sound

`ADLIB.DAT` is, after its 120-byte map, a table of 300 words and then the data they point to (figure `snd_files`). Words 0 to 149 are *sounds*, words 150 to 299 are *patches* (`gPatchTable` at 0542h is word 150 of the table, `src/E_TEXT.ASM:168-172`). The file holds 149 sounds (word 149 is 0; sounds 0 and 27 share one script) and 127 patches of 11 bytes, of which the table names 124. `SOUND.DAT` has 42 sounds. **[V]**

<!-- FIG snd_files -->

A **sound** starts with two header bytes: the *voice* it plays on (0 to 9) and its *priority*. The script follows, in a byte code (below). A **patch** is an instrument: 11 bytes that `LoadVoicePatch` (0000:4DC8) writes to the 11 operator and channel registers of a voice (figure `snd_opl`, later).

The voices of the 149 sounds are not evenly used: voice 0 has 41 sounds, voice 1 25, voice 2 24, voice 3 18, voice 4 9, voice 5 6, voice 6 5, voice 7 6, voice 8 8 and voice 9, the conductor, 7. Priorities say which kind of sound it is. **Music** has priority 1 (the character generation piece: a conductor on voice 9 and eight tracks on voices 0 to 7). **Effects** mostly have 40 to 52; 16 sounds, among them the monsters' move sounds, have priority 0, and that is why `PlaySoundAtCell`'s `20h - distance` raises them (17 to 31) when they are heard from a cell. Voice 8 carries only effects (priorities 39 to 44): the game's music never uses it. When a sound is installed on a voice (`ProcessSoundQueue`, 0000:402C), the new priority must be equal to or higher than the voice's current one, or the sound is dropped; the voice's old script pointer is overwritten and never resumed, so an effect on voices 0 to 7 silences that track of a piece for good. **[V]**

A **conductor** is a short script on voice 9, which has a voice record but no OPL2 channel: it sets the speed and the vibrato depth and starts the tracks with `START`. The character generation music is driver sound 66, whose script is `AEh 01 AFh 01 A6h A8h` (both depths on, tempo rate 168), then `82h 43h` to `82h 4Ah` (start sounds 67 to 74) and `88h`. The intro's `SOUND.DAT` has three such pieces: sounds 1, 11 and 21 start tracks 2 to 10, 12 to 20 and 22 to 29. The death jingle (game sound 22, driver 75: three tracks) and two five-voice stings (drivers 88 and 143) use the same shape with short tracks of 15 or 16 events. **[V]**

##### The script byte code

A script byte below 80h is a **note**: it takes a second byte, the duration, and sets the pitch, keys the voice on and waits (the high nibble of the first byte is the octave, the low nibble the semitone; figure `snd_pitch`). A byte of 80h or more is a **command**; its number is `byte - 80h` in a 74-entry jump table at 0000:4F94, and the operands follow it. Of the 74 entries 54 have a handler and 20 point at the end-of-script handler (4311); so do the bytes F1h to FFh. CAh to F0h pass the range check (`cmp al,70h`) but index past the table into the text that follows it (a bug that no shipped sound reaches). **[V]**

<!-- FIG snd_commands -->

The names below are labels of this chapter; the source knows the handlers by address. Only 27 of the 54 handlers (and `88h`, the end byte) occur in the shipped `adlib.dat` and `sound.dat`; the others (LOOP, JUMP, CALL, BEAT, the pitch effect FX and the rhythm commands except RCLR) are code a later game of the family may have used **[H]**. The listing is from `src/E_TEXT.ASM`, 4271 to 4A3A.

**Flow and sequencing**

| Byte | Operands | Label | Effect (handler) |
|---|---|---|---|
| 80h | n | `LOOP#` | loop counter `[+06] := n` (4271) |
| 81h | lo hi | `LOOP` | decrement `[+06]`; not zero: jump to the word, else skip it (4276) |
| 82h | n | `START` | start sound n on its own voice, as if queued (priority test, `ClearVoice`, install, `VoiceSilence`); FFh does nothing (427F) |
| 84h | lo hi | `JUMP` | script pointer := the word (42D4) |
| 85h | lo hi | `CALL` | push the return address on a 4-word stack (+09, +0A) and jump (42E0) |
| 86h | | `RET` | pop it (42F8) |
| 88h, and 19 other bytes | | `END` | priority := 0, script pointer := 0, key off (not on voice 9) (4311) |
| 8Eh | v | `STOP` | clear delay, priority and script pointer of voice v (4389) |
| 9Dh | mask | `BEAT` | wait for the next rising edge of (beat AND mask) (442E) |
| C7h | n | `FLAG` | `gScriptFlag := n` (4A23) |

The addresses of `LOOP`, `JUMP` and `CALL` are near addresses in segment 0 that nothing rebases. No shipped sound uses them, which is why the question never mattered. **[I]**

**Time and tempo** (a duration is a count of *units*; see 5.14.3)

| Byte | Operands | Label | Effect (handler) |
|---|---|---|---|
| 89h | d | `REST` | key off, wait d (4327) |
| A0h | d | `HOLD` | wait d, key state unchanged (4467) |
| A1h | d | `RETRIG` | key on again at the current pitch, wait d (447A) |
| 8Bh | note d | `PITCH` | set the pitch without keying on, wait d: a slide to a new note (434B) |
| 83h | n | `OFF2` | second key-off count (1 after `ClearVoice`): the voice is keyed off when its remaining delay equals n (42CE) |
| A4h | n | `GATE` | key-off multiplier := n AND 7; each note is then keyed off when (d >> 3) times n of its units remain (4490, 4B5F) |
| A6h | n | `RATE` | global tempo rate `gTempoRate := n` (4499) |
| A9h | n | `SPEED` | this voice's speed := n (44A9) |
| BAh | | `ONCE` | speed := `gTempoRate`, once (46A3) |
| C8h | n | `TRACK` | follow flag: the speed is copied from `gTempoRate` every tick (4A2A) |
| BDh | d | `SPD+` | speed += signed d, saturating at FFh and 1 (46B7) |
| BCh | m | `RNDLY` | random delay: each note's delay += `SoundRandom AND m` (46B1) |
| 9Ch | n | `TEMPO` | sets the beat counters (4412) |
| BBh | | `NOP` | nothing (46AD) |

**Pitch**

| Byte | Operands | Label | Effect (handler) |
|---|---|---|---|
| 87h | n | `TRANS` | transpose `[+07] := n`, added to the octave nibble: 10h is one octave (430B) |
| 8Ch | n | `DETUN` | detune `[+12] := n`, added to the semitone (4366) |
| 93h | n | `FINE` | fine tune `[+24] := n`, added to the F-number (43E7) |
| B9h | n | `BEND` | bend amount `[+42] := n` (signed), rewrite the pitch (45E9) |
| 91h | rate hi lo | `SLIDE` | each tick the phase adds rate; on a carry the F-number moves by the signed word hi:lo (big end first) and the octave is renormalised (43C3, 4F00) |
| 92h | | `SLID0` | stop the slide (43D9) |
| 95h | rate shift count wait | `VIBRA` | software vibrato: after a delay of wait ticks, each phase carry adds or subtracts F-number >> (9 - shift), reversing every count steps (43ED, 4E7E) |
| B5h | hi lo | `JITTR` | add `SoundRandom AND mask` to the F-number low byte and rewrite A0h and B0h (45AD) |
| 8Dh | rate reload reg lo hi | `SWEEP` | each phase carry steps a counter down from reload and writes table[counter] to register reg + the voice's operator offset (436C, 4ED7) |
| 8Ah | reg val | `OPL` | write val to OPL2 register reg (4344) |

**Levels, patches, depth**

| Byte | Operands | Label | Effect (handler) |
|---|---|---|---|
| 90h | n | `PATCH` | load patch n into this voice (43AD) |
| 9Eh | n | `LEVEL` | level offset `[+20] := n`, rewrite the level registers (445E) |
| B0h | n | `ADDLV` | `[+20] += n`, rewrite (4535) |
| ABh | n | `LVL2` | second level offset `[+30] := n` (44AF); the byte `PlaySoundAtCell` patches |
| ACh | v n | `SETAJ` | level adjustment `[+00] := n` of voice v, rewrite its levels (44B5) |
| ADh | v n | `ADDAJ` | the same, adding (44D6) |
| AEh | b | `AMDEP` | amplitude depth: bit 7 of register BDh := b AND 1 (44F9) |
| AFh | b | `VIBDP` | vibrato depth: bit 6 of BDh := b AND 1 (4517) |
| C9h | a b | `SHIFT` | decay shifts for the modulator and carrier; each note then carries a third byte (4A30, 421A) |

**Voice control and the rest**

| Byte | Operands | Label | Effect (handler) |
|---|---|---|---|
| 9Ah | n | `PRIO` | priority `[+08] := n` (440C) |
| A7h | | `NOHND` | clear the per-voice handler (+2B) (44A0) |
| B6h | | `NOTIK` | clear the per-tick handler (+29) (45E0) |
| B3h | v | `KILL` | silence and clear voice v: C0h+v := 0, carrier level 3Fh, release FFh, B0h+v := 0; v = 0 also clears the pitch effect (4541) |
| BFh | mode curve | `FX` | pitch effect: `gEffectMode := mode`, picks two curves from `gCurveTablePtrs`; the timer interrupt then toggles voice 0's key bit every `gEffectPeriod` passes (46D4, 3E66) |
| C0h | | `FXOFF` | `gEffectMode := 0` (4720) |
| C1h | 9 bytes | `RSET` | rhythm set-up: patches for voices 6 to 8, then three (B0h, A0h) pairs (4729) |
| C2h | bits | `RKEY` | rhythm key bits into BDh (4808) |
| C3h | | `RCLR` | clear the rhythm key bits and rewrite BDh; no operand (4838) |
| C4h, C5h, C6h | mask val | `RLVC`, `RLV+`, `RLVB` | rhythm operator levels, registers 51h to 55h (484B, 48EF, 4993) |

##### Voice record

Each of the ten voices has a 43h-byte record at `3B93 + 43h * voice` (figure `snd_voice`; the fields are listed one by one, with the code that proves each, in the comment at 0000:3B93). `ProcessSoundQueue` and `START` write a script pointer, a priority, a speed of FFh and a delay of 1 into it; `UpdateVoices` runs from there. The same record serves every command: a single set of state per voice, so a voice can carry a patch, a vibrato, a slide and a register sweep at once. **[V]**

<!-- FIG snd_voice -->

#### 5.14.3 The AdLib driver at run time

##### The clock

`StartAdlibDriver` (0000:3F77) saves the BIOS INT 08h vector, installs its own handler at 3E66, resets the card (`ResetVoices`) and programs PIT channel 0 with mode 36h and divisor 0555h = 1,365, so the interrupt comes 874 times a second if the PIT clock is 1,193,182 Hz **[H]**. The handler counts passes (`gTimerPasses12` and `gTimerPasses48`, initial 0Ch and 30h): every **12th** pass it calls `SoundDriverTick` (72.8 Hz, 13.7 ms); every **48th** it jumps to the BIOS handler it saved (18.2 Hz), which sends the end of interrupt and runs the BIOS clock and INT 1Ch; on every other pass it sends the EOI itself. So the game's tick counter (`gTicks`, ds:848A, stepped by the INT 1Ch handler at 1D8F; `src/E082C.C:64`) keeps its 18.2 Hz while the card sees 72.8. `StopAdlibDriver` (3FCA) restores the vector, resets the voices and writes divisor 0000h (65,536, the BIOS rate). The pass also counts `gBusyCountdown` down and, while a pitch effect is on, toggles voice 0's key-on bit. **[V]**

<!-- FIG snd_timer -->

##### One driver tick

`SoundDriverTick` (3FF4) does three things in order: `ProcessSoundQueue`, `UpdateVoices`, `TickTempo`.

1. **Queue.** `QueueSound` (400C) stores the driver number in a 16-slot ring (`gQueueRing`, 3B83) and moves the write index, masked with 0Fh. `ProcessSoundQueue` (402C) drains it: it looks the sound up in the 300-word table, reads the header word (voice, priority), compares priorities, and on success `ClearVoice`s the record (4D95), stores the script pointer (header + 2), sets the speed and accumulator to FFh and the delay to 1, and (except for voice 9) calls `VoiceSilence` (4E34): FFh to the two envelope registers of both operators, 0 then 20h to B0h. **[V]** Quirk: `QueueSound` stores a *word* at a *byte* index, so a sound queued at index 15 also writes 3B93, the first byte of voice 0's record (its level adjustment, +00), with the word's zero high byte. **[V]** (`src/E_TEXT.ASM:612-615`.)
2. **Voices.** `UpdateVoices` (418F) runs voices 9 down to 0. A voice whose delay counter (+05) is 0 is skipped. For the others the speed (+25, or `gTempoRate` if the follow flag is set) is added to the accumulator (+26); **only when the add carries** does the delay count down, so a voice with speed *s* advances *s*/256 units per tick. At the keying-off counts (+21 and +35) the voice is keyed off. When the delay reaches 0 the script runs: notes and commands are read in turn until one sets a new delay (`StartVoiceNote`, 4B47). Finally the voice's two handlers (+29, +2B: slide, vibrato, sweep) run every tick. A duration of *d* units therefore lasts *d* x 256 / *speed* ticks; at speed FFh one unit is one tick. **[V]**
3. **Tempo.** `TickTempo` (4C8D) adds `gTempoRate` to an accumulator; each carry steps a counter, and a counter that reaches 0 adds one to the beat count `gTempoBeat`, which `BEAT` waits on. No shipped sound uses `BEAT`, so the beat count is dead state. **[V]**

<!-- FIG snd_pitch -->

##### Notes, levels and what the card sees

`SetVoicePitch` (4A3A) turns a note byte into registers (figure `snd_pitch`): the octave nibble plus transpose goes to the block bits of B0h, the semitone plus detune (wrapping at 12) indexes the 12 F-numbers `gFNumbers` (3E31: 0134h to 0246h, a semitone apart), the fine tune is added to the F-number, and the bend amount adds or subtracts a value from one of 14 curves of 32 bytes (`gLevelCurvePtrs`, selected by the semitone). **The curves are pitch-bend tables, not level curves**: their sum goes into the register pair A0h/B0h (4A9B to 4AC8). The key-on bit is kept from the record. `VoiceKeyOn` (4AEF) sets bit 5 of B0h; `VoiceKeyOff` (4B1F) clears it. **[V]**

Levels are sums. The carrier's total level (register 40h + slot + 3) is the patch's carrier byte AND 3Fh plus the level offset (+20), the adjustment (+00) and the second offset (+30), clamped to 0 to 3Fh, with the top two bits (key scale) of the patch byte kept (`VoiceCarrierLevel`, 4D1F). The modulator gets the same sum only when the patch's connection bit (bit 0 of its C0h byte, kept at +31) says both operators sound (`VoiceModulatorLevel`, 4D57). So a script can fade a note with `LEVEL` and the game can fade a whole sound with `LVL2`. **[V]**

##### What reaches the card

Every write goes through `AdlibWrite` (4C6B): register number to 388h, 4 status reads, the value to 389h, 23 status reads. The reads are the delay (3.3 and 23 microseconds on a real card **[H]**; the oracle charges 20 microseconds a read and so makes a write cost 540 to 640 microseconds). A patch load is 11 writes, a note 3, a silence 6; figure `snd_opl` shows the registers and `snd_trace` a whole short sound as the oracle saw it on the ports.

<!-- FIG snd_opl -->

<!-- FIG snd_trace -->

Measured in the oracle (scratch runs, 2026-11-05; the first two are goldens):

| Session | Register writes | By register |
|---|---:|---|
| `boot-adlib` | 21 | the reset: 01h := 20h, 08h := 0, BDh := 0, the 18 operator levels := 3Fh |
| `load-walk` (the whole session, 26 s of it in the dungeon) | 5,334 | B0h 1,596; A0h 897; 80h 753; 60h 744; 40h 399; 20h 372; E0h 372; C0h 195; BDh 4; 01h and 08h 1 each |
| the intro, to its hand-over | 11,766 | B0h 7,165; A0h 2,407; 40h 793; 60h and 80h 392 each; 20h and E0h 242 each; C0h 121; BDh 8; 01h and 08h 2 each (two resets) |

Piece 1 of the intro runs at 130 to 150 writes a second; piece 2 is sparser (30 to 160) and starts with a burst of 187 in one second when its nine tracks load their patches. B0h dominates because every note writes it three times (pitch, key-on, key-off) and every slide step writes it again. In `load-walk` the recorded sounds are almost all monsters' move sounds (game numbers 75 and 57 in that save, driver 133 and 139: one note each, the first with a pitch slide). **[V]**

#### 5.14.4 The speaker and Tandy driver

The unit `src/E1948.ASM` (segment 1948, 1,147 bytes, 13 functions) is a simpler driver for cards 1 and 2. It keeps its state in a data segment that aliases DGROUP (DS is set to 29B8h in every entry; `gTandyOn`, 1 Tandy and 0 PC speaker, is DGROUP:8A67). `SoundDriverInstall` (0377) saves INT 08h, installs the handler at 000A, programs PIT channel 0 to divisor 1000h (4,096, about 291 Hz **[H]**) and puts channel 2 in square-wave mode. The handler calls `SoundTick` (0050) on **every** pass and chains to the BIOS on every 16th (`gChainReload` 0Fh) to keep 18.2 Hz. **[V]**

`StartSound` (0418) does no work: it stores the song pointer and sets the start request; the next tick copies the pointer to the song base and starts voices 0 to 2 at position 0. The song is the data file's bytes at the offset the 300-word table gives (`GetSoundData`, 03E3). Its format is a stream (figure `snd_speaker`): a note is two bytes, the note index (0 is a rest; 1 to 5Fh index a 96-entry table) and a duration byte; the commands are `E2` (length multiplier), `E8` and `E9` (adjust it), `E6` (a note by frequency), `CD`/`CE` and `D1`/`D2` (two nested repeat counters and loops back by a word) and any other byte of 80h or more ends the voice. A duration byte *d* lasts (*d* + 1) x multiplier - 1 passes. The note table is in the EXE's data image: 96 words of `1193180 / f` for the PC speaker (index 1: 65,023, 18.35 Hz; index 95: 285, 4,187 Hz) and 96 of `111860 / f` for the Tandy (clamped to 1023). **[V]** (tables read from `EOB.EXE` at DGROUP:88BA and 897A.)

<!-- FIG snd_speaker -->

On the **PC speaker** a note is `out 42h` of the 16-bit divisor and the gate bits 0 and 1 of port 61h (`VoiceOn` 02C7, `VoiceOff` 02F1, `SetVoicePitch` 0316). On a **Tandy** the SN76489 sits at port C0h: a tone is two bytes (`1 cc 0 dddd`, then `00 dddddd`), a volume one byte (`1 cc 1 vvvv`, inverted) **[H, the chip's format]**. `SoundDriverResume`, `SetThreeVoices` and the uninstall routines switch between the 1-voice and 3-voice outputs.

The three voices are not three parts. **All three read the same stream from byte 0** (`gVoicePos` is reset to 0 for voices 0 to 2 at `SoundTick` 0050-0086 and the base is one pointer), but voice 1 waits 35 passes and voice 2 70 (the counters 23h and 46h, 120 and 240 ms), and the volumes in the data image are 15, 10 and 5 (DGROUP:8A54 holds `0F 0A 05 00`). That is an echo, quieter each time **[I]**; on the PC speaker `gVoiceCount` is 1 and only the first voice sounds. The decoded songs agree with this being a single melody: 67 streams, all of them ending in `FF`, 176 `E6` commands (in 25 streams), and a sound such as game sound 1 is three notes (110, 220 and 440 Hz) of 7 ms each. **[V]** (PCGAMESN.DAT decoded, 2026-11-05.)

#### 5.14.5 Dead and odd code

- **A sample player that nothing starts.** `StartSample` (0000:4B7F) sets up voice 0 and puts PIT channel 0 at divisor 004Bh = 75 (15.9 kHz **[H]**); the second handler (3EE5) then reads one 4-bit delta per pass from a sample (low nibble first), adds it to a level (bit 3 clear: add the nibble; set: subtract its low 3 bits) and writes the level to the data port. It is digitised sound by a very old trick **[H]**. No far call to it, to `SetSamplePlaying` or to `GetSampleOffset` exists in `EOB.EXE` or `INTRO.EXE` (a search for the call bytes), so it never runs. It also never chains to the BIOS handler (a quirk that would stop the clock for its length). **[V]**
- **An entry table with no reader.** The 11 words at 3944 list the driver's routines (start, stop, tick, queue, sample start, voice delay, sample offset, table, set note, sample flag, select table): the interface of a driver library **[I]**. The game calls the routines directly instead.
- **A pitch effect with no sound.** `FX` (BFh) and its timer-interrupt half are not used by any shipped sound. So are the rhythm commands except `RCLR` (used once, by the silence script, driver sound 0 and its twin 27), `LOOP`, `JUMP`, `CALL` and `BEAT`; `LOOP#` appears in four sounds (`ADLIB.DAT` 7 and 125, `SOUND.DAT` 33 and 34) with no `LOOP` to use the counter.
- **Music that loops by polling.** Of the character generation tracks only one, driver sound 67 (voice 0), ends with `C7h 01` (at byte offset 124 of its script); no other sound of `ADLIB.DAT` uses `FLAG`. `WaitKeyOrPause` (ovl0:00F7, `src/EOV0.C:221-226`) tests `gScriptFlag` (0000:03A0); when it is 1 it clears it and plays sound 20 again. So the music repeats only while the game waits for a key on those screens. **[V]**
- **A "Tunes" option that does nothing.** The preference "Tunes are ON" toggles `gTunesOn` (ds:80E8); the INT 9 handler toggles it on key 0434h too (`src/E1AAD.ASM:978-990`), and it is saved with the game. No code reads it. `gSoundOn` (ds:80E6), toggled by the "Sounds" preference and key 0420h, is the only switch the sound functions test. **[V]** (`grep` of `src/`.)
- **The first deferred play writes into sound 0.** `gPrevSound` starts at 0 and `gSoundLevel`, `gSoundReg1` at FFFFh (ds:5E26 to 5E2A in the image), so the first `AtCell` play "restores" bytes 1 and 3 of driver sound 0, the silence script, to FFh. The oracle shows it: after `load-walk` the sound's bytes in memory are `09 FF AE FF` where the file has `09 32 AE 00` (voice 9, priority 50 becomes FFh; the operand of `AMDEP` 0 becomes 1 as far as the command reads it). `ProcessSoundQueue` compares priorities as signed bytes (`cmp al,[di+8]` then `jl`, `src/E_TEXT.ASM:667-668`), so FFh is -1 and a later `PlaySound(0)`, the "stop everything" the game calls when it enters a level, loses to any voice priority of 0 or more and is dropped. **[V]** for the bytes, **[I]** for the consequence (not run: a second level entry after a positional sound). The first level entry of a session is not affected, because it comes before any positional sound.
- **Table names.** `src/E_TEXT.ASM` calls the 14 curves at 3976 "level curves"; the code that reads them writes the pitch registers (above). `gDriverTablePtrs` (039C) holds two copies of 0416h.

### 5.15 The intro

`INTRO.EXE` is the program `START1.EXE` runs. It plays a 147-second sequence of 13 scenes with music (2,680 ticks of the INT 1Ch clock in the oracle), then hands over to `EOB.EXE` (section 2.6 has the mechanism). Nothing in it is data-driven: there is no script file, no scene table, no interpreter. The sequence is **code**, a fixed series of functions whose loops draw and wait; the pictures and the sounds are the only data. This is the opposite of the game's sound, and the reason it fits in one overlay of 22 functions.

<!-- FIG snd_intro_prog -->

#### 5.15.1 Structure

`Main` (0892:0000, `src/I0892.C`) reads the graphics and sound digits and the "homie" marker, installs the INT 3Fh overlay handler, the keyboard, the hard-error handler and the tick handler, looks for `EOB.EXE` (asking for "DISK2" while it is missing) and calls `StartupGame` (0892:0241). That sets the video mode, loads the graphics driver overlay (`MCGA.OVL` on VGA), fills the page table, loads `Font8.fnt`, starts the overlay cache and the sound card (`SOUND.DAT` for AdLib). `Main` then loads `palette.col` and calls `Ovl0Main` (stub entry 16, `RunIntro` at ovl0:0000). `RunIntro` calls `PlayLogoSequence` (ovl0:2A99) and, unless a key aborted, `PlayIntroSequence` (ovl0:003A), which plays the title card and then seven scene functions in a fixed order. **[V]** After that `Main` runs `ShutdownGame` (0892:0835: sound off, keyboard and tick handler removed) and `execl("EOB.EXE", ...)` (`src/I0892.C:263`).

The 22 functions of the overlay (`src/IOV0.C`, 11,741 bytes) are 3 entry points, 7 scenes and 12 helpers (one of them, `RevealInStrips`, ovl0:2913, has no caller). `INTRO.EXE` has 33 units (section 2.1.3), of which section 2.6 counts 23 as twins of `EOB.EXE`'s: the sound driver for speaker and Tandy, `MemAlloc`, the page table and `CopyPage`, `CopyRegion`, the shape wrappers, the font loader, the mouse, the file layer, the tick handler, the LCW unpacker, the text drawing. What `INTRO.EXE` has of its own is the start-up unit, the AdLib driver (`i_text`, the EOB driver in another version: the same 74-entry table at 536C), four small units and the overlay. **[V]**

A scene function loads its pictures, cuts them, and runs loops of the form "set `gFrameDelay`, draw, copy to the screen, `WaitFrameDelay`". Most of the code in `src/IOV0.C` is such loops. A picture is a `.CMP` file in `EOBDATA1.PAK` or `EOBDATA4.PAK`: format 4 (LCW), 64,000 bytes unpacked, no palette inside (the header's palette size is 0) for each of the 19 the intro loads; the palettes are separate 768-byte `.COL` files (6-bit VGA colours) in `EOBDATA4.PAK`. **[V]** (`tools/re/wwfmt.py` on the `.CMP` files, 2026-11-05; `EOBTITLE.CMP`, also in EOBDATA4, is the odd one of section 4.2 and the intro does not load it.)

#### 5.15.2 The clock, the waits and ESC

The unit of time is the BIOS tick, 18.2 a second. `gFrameDelay` (ds:2326, a long) is counted down by the INT 1Ch handler (the twin of EOB's handler at 1D8F: the countdown at ds:849A in EOB is at ds:2326 here, the 6174h shift of every DGROUP variable). `WaitFrameDelay` (ovl0:0FD9) is a busy loop: while `gFrameDelay` is non-zero it polls the keyboard. The key rule is the whole of the skip logic: **any key** drains the key buffer and clears `gFrameDelay`, so the current wait ends; **ESC** also sets `gAbort`, after which every wait returns at once (`WaitFrameDelay` and `DelayFrames` test it first) and `RunIntro` and `PlayIntroSequence` start no further scene. The scene that is running still finishes its loops, now without waiting, so it flashes by. **[V]** (`src/IOV0.C:529-558`; oracle session `intro-skip`: ESC at tick 400, in the logo, then `EOB.EXE`'s title menu 300 ticks later.)

Two consequences. First, a delay is a *minimum*: the delays armed before a drawing step count down while the step runs, so a long step eats into the next wait. The four SSI cards arm 50, 100, 50 and 140 ticks (`gLogoDelays`, ds:1D2A; the Westwood card 140, `gWestwoodDelay`, ds:1D28), yet the oracle spaces them 75 to 87 ticks, 100 and 140 ticks apart: the dissolve of a card takes 75 to 86 ticks there and is longer than the delay of cards 1 and 3. That part scales with the CPU. Second, the total is the fixed delays plus the time of the CPU-bound steps (the dissolves above all): 2,680 ticks (147.2 s) in the oracle with no sound, 2,723 ticks with AdLib (the interrupts take time), at the oracle's nominal 1 MIPS. A faster PC would shorten it, down to the sum of the delays **[I]**. **[V]** for the oracle's numbers (scratch runs of `INTRO.EXE` with sound digits 4 and 1, 2026-11-05). 

#### 5.15.3 Pictures and animation

The page model of the graphics driver (section 2.5) is the whole toolkit (figure `snd_intro_pages`). `StartupGame` makes a page table of four pages of two slots each: page 0 is the screen, pages 2, 4 and 6 are MemAlloc'd buffers of 64,000, 64,000 and 64,512 bytes (`src/I0892.C:337-342`). The primitives are:

- **`LoadPackedImage(name, 5, 3)`** (ovl0:2B7F): read the file's first word (the packed size), read that many bytes into slot 5, call `Unpack4` (LCW) on them past the 8-byte header into slot 3. Always 64,000 bytes.
- **`LoadPalette(name)`** (ovl0:2A4E): on VGA only, read 768 bytes, force colour 0 to black, wait for one frame (`WaitFrame`), set the palette through the driver. **There are no palette fades**: a scene change is a hard palette swap.
- **Cell cutting, `gMakeImage(col, y, cols, rows, map)`**: the driver's `GetSpriteCellRle` (slot 22, export 15, section 2.5), 26 call sites in the overlay (and 83 of `ClipRect`). It cuts a rectangle of a picture, in 8-pixel columns and pixel rows, into a MemAlloc'd shape in which runs of 0 are coded and so transparent. The scenes cut animation frames, stars and sprites this way and `MemFree` them at the end.
- **`DrawShape(page, shape, x, y, region)`** and `DrawShapeFlipped` (0998:0008, 0998:0046): draw such a shape to a page, clipped to the region. The last scene flips two panels to make mirror images.
- **`ClipRect(srcCol, srcY, dstCol, dstY, cols, rows, fromPage, toPage)`**: the shared `CopyRegion` (E19F3.C, unit 0986) that clamps its arguments and calls the driver's rectangle copy between pages. This is the frame step: draw into page 2 or 4, copy the changed rectangle to page 0. Columns are 8 pixels; the screen is 40 columns.
- **`CopyPage(from, to, map)`**: a whole page. `CopyPage(3, 4)` keeps a picture in page 4 so that later `ClipRect(..., 4, 0)` calls can uncover it piece by piece.
- **`Dissolve`** (ovl0:2C0D): the title and the logo cards appear by shuffling two permutation tables (320 columns, 200 rows, shuffled with `RandomRange`) and copying every pixel through `gGetPixel`/`gPutPixel`, walking the columns in order and the shuffled rows cyclically (row `k`, then `k + 1`, wrapping), which spreads the pixels evenly.
- **`BlitImage`** (ovl0:09AD) and **`DrawGrid`** (ovl0:29C9): the two plain-loop effects of figure `snd_intro_stipple`. `BlitImage` copies a rectangle keeping a pattern of pixels (modes 1, 2, 3: one in 2, 4, 9). `PlayWtrdpScene` cuts the full picture, then the three sparse copies, and draws the four in turn three ticks apart: a fade-in with no palette change. `DrawGrid` draws 16 interleaved lines of colour 12 that cover the whole screen: it is the screen wipe used twice in the tower mage scene, once in the tunnel scene and at the very end (the final call, from `PlayIntroSequence`, is the last thing on screen before the hand-over). **[V]**
- **`GrowFrame`** (ovl0:1041): a framed rectangle that moves and grows a pixel step at a time (used four times in the hands scene).

<!-- FIG snd_intro_pages -->

<!-- FIG snd_intro_stipple -->

The scenes also use `RandomByte` (0CDA:0006, a 24-bit shift generator, the twin of EOB's 1D8C) for the star drops and the king sprites' start heights. Every picture is 320 by 200 and the same code serves the other video modes: `CopyPage` and the driver convert the art through the 32-byte colour map `gCgaMapDefault`, and `PlayIntroSequence` loads `TITLE-E.CMP` instead of `TITLE-V.CMP` unless the mode is VGA. I did not check the intro in the CGA, EGA and Tandy modes. **[H]**

#### 5.15.4 The scenes

The order is the order of the calls in `PlayIntroSequence`. The ticks are the oracle's, in a silent run and in a run with AdLib: the entry of each scene function (found with breakpoints on the overlay's functions) and, for the logo cards, the first load of the picture.

| # | Scene | Function | Files loaded | Entry tick, silent | Entry tick, AdLib | Sound calls |
|---|---|---|---|---:|---:|---|
| 1 | Westwood logo | `PlayLogoSequence` (ovl0:2A99) | WESTWOOD.CMP/COL | 17 | 18 | `PlaySound(1)`: music piece 1 |
| 2 to 5 | AND, SSI, PRESENT, DAND cards | the same | AND, SSI, PRESENT, DAND .CMP | 163, 238, 338, 413 | 164, 250, 350, 437 | none |
| 6 | title | `PlayIntroSequence` (ovl0:003A) | TITLE-V.CMP, WESTWOOD.COL, TEXT.CMP | 553 | 577 | `PlaySound(2)`: piece 2 |
| 7 | the tower mage | `PlayTowerMageScene` (ovl0:0B20) | TOWRMAGE.COL/CMP | 701 | 735 | none |
| 8 | the orb | `PlayOrbScene` (ovl0:06E2) | ORB.CMP, TOWRMAGE.CMP | 865 | 901 | `PlaySound(6)` twice |
| 9 | the corridor | `PlayWtrdpScene` (ovl0:015D) | WTRDP2.COL, WTRDP1 to 3.CMP | 1000 | 1036 | `PlaySound(6)`; 5 or 14 at random |
| 10 | the kings | `PlayKingScene` (ovl0:1265) | KING.CMP, ADVENTUR.CMP | 1335 | 1373 | `PlaySound(7)` at random |
| 11 | the hands | `PlayHandsScene` (ovl0:15F9) | HANDS.CMP | 1506 | 1544 | 11, 10, 12, 11, 10, 13 |
| 12 | the tunnel | `PlayTunnelScene` (ovl0:1DF3) | WTRDP3.CMP, ORB.COL/CMP, TUNNEL.CMP, ZOOMTUNL.COL | 1856 | 1899 | 5 or 14 at random; `PlaySound(3)`: piece 3 |
| 13 | the avalanche | `PlayAvalancheScene` (ovl0:23A3) | AVALANCH.CMP, ZOOMTUNL.CMP | 2343 | 2385 | 7 six times; 8, 4, 9, 6 |
| (end) | grid wipe, hand-over | `DrawGrid`, `ShutdownGame` | | 2675 / 2680 | 2718 / 2723 | |

The scene names are labels taken from the picture files each function loads (the source names its functions the same way, `src/IOV0.C:1-15`); what a scene shows is not described here. A few structure facts that the code settles: the corridor scene (`PlayWtrdpScene`) and the tunnel scene share their star-drop loop (31 random tiles of `WTRDP3.CMP`, five drawn per tick, a window sliding 71 steps) with a sprite drawn at the window's position; the kings scene draws four sprites rising from random starting heights (`(RandomByte() AND 3) * 8 + B4h`) with step values `-4, -8, -2, -2, 1, 0, 0, 0` per frame; the hands scene has four phases, each opened by a `GrowFrame`; the avalanche scene flashes two flipped panels six times and then draws six rectangles from the tables `gZoomX1` to `gZoomY2` (ds:1D04 to 1D22). **[V]** (`src/IOV0.C`.)

<!-- FIG snd_intro_timeline -->

#### 5.15.5 Music and effects in the intro

The intro's 21 `PlaySound` call sites use all 14 non-zero game numbers of `SOUND.DAT`'s map (1 to 14 map to driver sounds 1, 11, 21, 30 to 35 and 37 to 41; the other 105 entries are 96h or more, FFh, which the intro's `PlaySound` turns into 0). `PlaySound` (ovl0:2D4E) is the game's with two differences: no `gSoundOn` test and no 96h cut-off. The map tells the structure: **three music pieces** (game sounds 1, 2 and 3, driver sounds 1, 11 and 21) and **11 effects** (sounds 4 to 14: ten on voice 8, one (game 9) on voice 7, at priority 6 or 7 against the music's 4, so an effect cuts the piece's track on that voice for good).

Each piece is a conductor (voice 9) that sets the speed with `RATE`, switches both depths on and starts the tracks: piece 1 has nine (sounds 2 to 10) at rate 168, piece 2 nine at rate 234, piece 3 eight at rate 180. Tracks run from a handful of events to 556. The timing model of 5.14.3, applied to each track, gives the longest track of each piece a nominal length of about 27 s, 76 s and 30 s **[I]**. Piece 1 is started at the first image, piece 2 at the title, piece 3 in the tunnel scene about 30 s before the end.

**The music is not synchronised to the images by any mechanism.** The scenes wait on tick counts; the music is started by three `PlaySound` calls and then runs on the timer interrupt, which the scenes never look at (no function in the overlay reads the driver's state). Any fit is by construction: piece 1 ends a few seconds before the logo sequence does (nominally tick 520 against 577), piece 3 about when the intro does (tick 2191 plus 546 against 2723). In the oracle each write is charged for its status reads, the interrupts take more virtual time than on a real card, and the music runs about 14 percent slow (piece 2 stops at tick 2160 where its nominal length ends it near tick 1970). **[V]** for what the code does, **[I]** for the lengths.

The speaker versions are thinner. `PCSOUND.DAT` maps only 13 game numbers; game sound 3 maps to 0, so there is no third piece; one voice plays; on a Tandy the song is echoed. Section 5.14.4 describes the driver.

#### 5.15.6 Hand-over

After `PlayIntroSequence` returns, `Main` probes `EOB.EXE` (opens and closes it), shuts down (the AdLib driver is switched off and the voices reset, so the music never carries over: the oracle shows the PIT back at 65,536 at tick 2723 and at 1,365 again about a second of virtual time later, when `EOB.EXE` starts its own driver), closes the overlay handle and calls `execl("EOB.EXE", "EOB.EXE", argv[1], argv[2], argv[3], argv[4], restoreMode, memTop, NULL)` (`src/I0892.C:254-263`). Section 2.6 has what the library's `execl` does with that. The sixth argument is not unused: `EOB.EXE`'s `StartupGame` reads it with `atol` as the memory-top segment (`src/E07C8.C:311`); INTRO passes `gMemTopSeg`, which is 0 unless the Tandy path reserved memory.

<!-- FIG snd_handover -->

### 5.16 Random numbers and time

#### 5.16.1 Two generators

<!-- FIG arc_rng -->

**`rand`.** Almost all randomness is the Turbo C runtime's linear congruential generator (0000:55B7): `seed = seed x 015A4E35h + 1` modulo 2^32 and a result of `(seed >> 16) & 7FFFh`, from the 32-bit state at `ds:8784` (image value 1). `srand` (0000:55A6) stores its argument in the low word and 0 in the high word. `StartupGame` calls `srand(time(NULL))` once; `time` returns the DOS clock's seconds since 1970, and `srand` takes an `unsigned`, so only the low 16 bits of the seconds are used: 65,536 distinct sequences, one per second of an 18.2-hour cycle. [V] Run in the oracle on the original: with the image seed the first values are 346, 130, 10982, 1090, 11656; after `srand(1234)` they are 1356, 29133, 21998, 27018; and, after `srand(1234)`, `RollDice(1, 6, 0)` gives 1, 6, 5, 5, 6, 3, 5, 4. Each equals a Python model of the formulas above.

**`RandomByte`.** A second generator with a 24-bit state (`ds:8484`, bytes 34h 12h 76h in the image) in `E1D8C.ASM`. One step shifts `b2` and `b1` left and `b0` right, passing the bits that fall out to the next byte (bit 1 of `b0` into `b2`, bit 7 of `b2` into `b1`) and mixing in `(b0 >> 2) - b0 - carry`; the result is `b0 xor b1`. Its cycle is 139,810 steps from the image state, short of the 16.7 million of a maximal 24-bit generator. [V] The oracle gives 62, 196, 85, 196, 63, 165, 39, 120, 235, 218, 51, 165 for the first twelve calls, as the model does. It is never seeded, so it repeats the same sequence in every run; its only two callers in `EOB.EXE` choose the starting edge and the mirror of a particle in the spell burst effect (`E0EF3.C:2255` and `:2281`), a purely visual use. `INTRO.EXE` has the same routine and uses it for its animation. [V]

#### 5.16.2 The dice

`RollDice(count, sides, bonus)` (082C:0458) sums `count` rolls of `rand x sides / 32767 + 1` (the one value `rand` = 32767 would give `sides + 1`, which is folded back to 1) and adds `bonus`; with `count` or `sides` not positive it returns just the bonus. All three arguments are `char`, so a bonus of 0FFh is -1: `RollDice(1, 4, 0xFF)` is 0 to 3. 68 call sites in 14 units use it (63 call instructions in the image: 54 resident, 9 in overlays). [V] A few that show the shape of the game's rules:

| Roll | Where | Use |
|---|---|---|
| 4 x 1d6, drop the lowest, plus a race bonus | `RollAbilityScore` (ovl0:1177), `RollCharacterStats` (ovl0:1BF2) | an ability score at creation |
| 1d100, 1d(class hit dice) | `RollCharacterStats`, `RollHitPoints` (0A72:01AE) | strength percentile; hit points at creation and on a level gain |
| 1d20 | `RollToHit` (0DE8:0007), `RollMemberSave` (0D1C:059B), `TurnUndead`, `DamageMonsterWithSave` | to-hit rolls and saving throws |
| the item's (count, sides, bonus) | `RollAttackDamage` (0DE8:011A) | a weapon's damage; a bare 1d2 when the hand is empty |
| the monster type's (count, sides, bonus) per attack | `ResolveMonsterMelee` (08E6:10F3) | monster damage |
| 1d2 | `KillMonster` (08E6:0EC3) | whether a monster drops its first item |
| 1d100 | `MonsterResistsMagic` (0D1C:0BAB) | resistance to spells |
| 1d30, minus 1 | `FindWordPanel` (ovl2:09ED) | which of the 30 symbols of the manual check is asked |
| 1d10 + 3 | `RunMapLoop` | the delay of the next ambient sound, times 18 ticks |

`RandomRange(lo, hi)` (1AAA:0005) is the same scaling with a lower bound: `rand x (hi - lo + 1) / 32767 + lo`, drawn again in the one case that gives `hi + 1`. It sizes the spell bursts and shuffles the tiles of the overlay-5 animation. [V]

What randomness does *not* do: level layout, monster placement and loot tables are all data (`LEVELn.INF` and the item tables); the random numbers only roll the outcomes. A faithful replay of a session therefore needs the clock mocked (the oracle does this) and nothing else. [I]

#### 5.16.3 Time

Section 3.6 describes the clocks. For rules the numbers to keep in mind are the periods in ticks of 1/18.2 s (54.9 ms): 3 for projectiles, 5 for doors, 15 to 35 for monster groups, 50 for character status effects (2.7 s), 1,080 for food (59 s), plus the per-level step timer of 50 to 150 steps. The camp and animations stop `gGameTicks`, so rest and menus take no game time; but `CampLoop` advances time by its own hour loop, and `gTicks`, which never stops, drives only delays and blinking. The Borland `time` call is used for the seed and for nothing else. [V]

## 6. Open questions

Everything here is a question the evidence did not settle. Each is tagged **[?]** and says what would settle it: most need a run of the oracle that exercises the case, and a few need the retail 1.7 files (section 1), which would tell an original defect from a patch edit.

### 6.1 Questions

#### Party, items and the camp

- **[?]** The Ring of Sustenance never works: both `IsWearingRing(who, 2) < 0` tests (`DecayMemberState`, 1414:0C82; `CampLoop`, ovl6:0653) can never be true. Original defect or patch edit? (5.9; the 1.7 files would decide.)
- **[?]** `RollMemberSave` tests `race * 2` where `race >> 1` was meant (`shl ax,1` at 0D1C:05FB), so the dwarf, gnome and halfling Constitution saving bonus never applies and a male elf gets the gnome rule (5.1, 5.7.7).
- **[?]** The to-hit progression rows give fighters two points per three levels and clerics one per level, the reverse of the AD&D matrix; does the shipped game play it that way (5.1, 5.7.3)? A run that levels a fighter and a cleric and prints the needed roll would settle it.
- **[?]** Helmets give no armour class: `ComputeArmourClass` walks four slots and the byte after `gArmourSlots` is 19, so the table probably had five entries (5.1).
- **[?]** The dexterity missile table has six zeros where first edition AD&D has ten; THIEF/MAGE has its hit dice in the opposite order from its slots; `gPaladinSpellsPerLevel` is read with the level, not level minus one (5.1).
- **[?]** `hp` and `maxHp` are signed bytes with no cap below 128 at level-up: can a long-lived fighter wrap (5.1)? A cleric with Wisdom 19 may be allowed seven first-level spells in six slots (5.9).
- **[?]** The meaning of `ItemType` +2 and +C, `ItemRecord.flags` bits 0 to 2 other than the wand charges, and what inventory slot 22 is for (5.2).
- **[?]** In the pinned `ITEM.DAT`, records 11 to 16 have key 0, not -2, so `CreateItemRecord` falls through to id 17, the "Jeweled Key" prototype. Was the original data different (5.2)?

#### Combat and magic

- **[?]** `HalveSaveDamageA` always halves, so the save in `DamageOneMember` always fails and the disintegrate effect always lands. Everything suggests `if (RollMemberSave(who, 4)) value >>= 1` was meant (5.7.7).
- **[?]** Flame Arrow damage is `RollDice() * level` in a signed byte: from caster level 5 can it wrap negative and heal the monster (5.8.4)? A direct call of `HitFlameArrow` in the oracle would show it.
- **[?]** Read Magic and Knock have all-zero records in `gSpells`, so casting them would call 0000:0000; can the player select them (a scroll could carry them) (5.8.2)? Stinking Cloud, Cloudkill and Protection from Lightning do nothing when cast; the Speed potion has no effect.
- **[?]** `MemberIs(6, 1)` in `RunRecordAttack` and `DamageTargetCell` reads past the party array, at the high byte of `gLevelTimerSteps` (ds:FB33); the intent looks like "the party has a fifth or sixth member" (5.7).
- **[?]** Turn Undead works only on levels 2 and 7 (the code tests `gCurLevel` directly); why (5.7.10)?
- **[?]** Are members 4 and 5 drawn in a third rank? Targeting treats them as one (5.7.4).

#### World, scripts and monsters

- **[?]** What the ten entries of a wall decoration are (one per renderer face); the click test uses entry 1 only (5.3.5, 5.4.4).
- **[?]** What the 125 level changes in the scripts are in the game (59 are falls, one level down); level 7 links to levels 2, 4, 5, 9 and 11 in both directions and its archive holds `PORTALA.CPS` and `PORTALB.CPS`, so they may be portals (5.3.7).
- **[?]** 432 script tests compare a literal with 0 to 3 (108 four-way groups in 54 scripts), so only one arm of each runs: a constant standing for the facing, or an authoring artefact (5.5.6)?
- **[?]** `SpawnMonster` refuses an occupied cell, so the four-spawn groups of the timer scripts may add one monster only (5.6.6; read, not run).
- **[?]** What bit 6 of `gWallTypeFlags` means, whether wall kind 0x34 is a teleporter [H], and which kinds use handler 255 (5.3.2, 5.4.4).
- **[?]** The monster type names: the game never prints them, so the table uses the sprite-sheet names from the level files, and the kenku and drider rows have identical statistics (5.6.1). Types 11, 14 and 21 are in no level's starting records.
- **[?]** The level step timer runs `RunCellScript(0, 0x20)`; is it a wandering-monster spawn (3.1.2)? The mode 3 timer test is reversed in the source and no level uses the mode (5.6).

#### Graphics

- **[?]** What each of the six wall sets of a `.VMP` draws (plain wall, a door frame in set 3, probably two staircases) (5.12.3).
- **[?]** How a monster in the cell behind a solid wall or a closed door stays hidden: `DrawSlotMonsters` tests no wall flag (5.12). An oracle run with a monster behind a wall would settle it.
- **[?]** Why wall pieces of types 0x17 and 0x18 are dropped at the near edge on levels above 3; what `gMonsterYAdjust` lowers; the 0x80 item-flag recolour path and `gPartySpellFlags` bit 0; the CGA hit-flash row at ds:1CE3 (5.12, 5.13).
- **[?]** `DrawShapeMasked`, the scenery planes of page 4 and `SwapPageWithScreen` have no caller: vestiges of an earlier renderer [H] (5.12, 2.5).
- **[?]** The size of the Tandy memory reserved for pages 2 and 3 [H, 64,000 bytes], and the inner pixel loops of `DrawTile` for CGA, Tandy and EGA, which were read at their entry only (5.12).

#### Sound and the intro

- **[?]** Is a second `PlaySound(0)` really dropped after the first deferred play leaves driver sound 0 with priority FFh (5.14.1; by the signed compare it is, not run)?
- **[?]** Which AdLib patches are unused (the table names 124 of 127 stored, the scripts use 120); whether `LOOP`, `JUMP`, `CALL` and `BEAT` were ever used (5.14.2).
- **[?]** The OPL2 and SN76489 register meanings are the chips' documented formats [H]; nothing in the repository documents them (5.14).
- **[?]** The real-hardware length of the intro: the oracle's clock is instruction counts, and the CPU-bound dissolves (75 to 141 ticks each) depend on the machine (5.15.2). The CGA, EGA and Tandy intro modes were not run.

#### Architecture and the interface

- **[?]** Who sets `gRecordMode` (ds:80EC) to 1 or 2, the input record and replay mode of the key driver: nothing in `src/` does (3.6.3).
- **[?]** Node tables 3a and 3b: the spell nodes are in 3b, the opposite of lists 1 and 2; `gLevelFlags[12]` is `gGameFlags` (the same four bytes at ds:FC04). Original behaviour or table mistakes (5.10)?
- **[?]** All 203 file table flags are 0 in this copy, so the whole memory cache is dormant; do other releases ship flags (3.5)?
- **[?]** How the INT 3Fh vector first reaches the overlay manager (no installer was found in the load image), where the stack really lives, and what an odd stack pointer at the INT 3Fh handler means (3.3, 3.4).
- **[?]** The key names of the keyboard-only commands (W, S, Z, A, U, D, T, G, M) are what the key codes give; the original manual's names are not checked (5.10.1).

### 6.2 Names and comments that the evidence contradicts

These are in `src/` and `re/renames.toml`. They were not changed in this revision: a rename goes through `make rename` with its evidence (playbook 65). Each is a candidate.

- `TickBandageCountdown` and `ClearBandageCountdown` (0EF3:18A2, 0EF3:1894) act on the Stoneskin counter (`CharRec.mE0`, set by `CastStoneskin`), not a bandage. In `EOB.H`, `mDF` is Armor's absorb counter, `mE0` Stoneskin's hit count and `mE2` Aid's temporary hit points; `status` bit 0 means "the slot holds a character", not "alive"; `m1E` is the hand-busy mask (5.1, 5.7, 5.8).
- `GetClassSlot24Value` (0EF3:04F7) returns a cleric's level; `FindClassSlot`'s first parameter is a base class number (0 fighter, 1 mage, 2 cleric, 3 thief, 4 paladin, 5 ranger); `gHpMaxOrAttackTarget` and `gHpMinOrHitFlag` are the seventh entries of `gStatMax` and `gStatMin` (5.1).
- `AddDDEffect`, `UpdateDDEffects` and the other DD names work on the thrown items and spell missiles of ds:DD2C ("DD" is the address); the timed effects are `gEffects` (ds:5BA8), a different table (5.7.1, 5.7.5).
- Three names for one decoration table: `gWallRecs` (ds:E040), `gViewWalls` (ds:E06A) and `gSlotRegions` (ds:E06B). `LoadLevel`'s `itemCount` and `itemIdx` are the trigger table's count and cell. `IsCellInView` searches 15 of the 18 view slots (5.3, 5.12).
- ovl4:1EDE is `DrawCompass` in `EOV4.C` and `DrawViewRows` in `E0EF3.C`, `EOV3.C` and `renames.toml`; it draws the compass. `gMonsterShapes` (ds:B7FE) and `gLoadedMonsterShapes` (ds:B802) are one array; `LoadMonsterShapesA/B/C` cut monster shapes, not map icons (5.12).
- `RunOverlayRequest` loads a graphics driver, `LoadOverlayScreen` a picture, and `InitOverlayCache` and `ShutdownOverlayCache` start and stop the file table: none is the VROOMM overlay manager (`E1E8B.ASM`), which opens `EOB.EXE` itself (3.3, 3.5).
- The game's tick handler is INT 1Ch (`InstallTickHandler`, 1D8F:0010); INT 08h belongs to the sound drivers. `PlaySoundAtCell` plays the sound recorded by the previous call, so a sound starts one call late, and `RunMapLoop`'s `PlaySoundDefault(0)` is the flush (3.6, 5.14).
- The `gLevelCurve*` tables of `E_TEXT.ASM` are pitch-bend curves, not level curves. In `E177D.C`, `SetSoundByte(.., 1, ..)` writes a sound's priority and `(.., 3, ..)` the operand of its first command (5.14).
- The `CampLoop` header comment says the eight-hour step drops a member's level; the code changes `food`. `EndInvisibility(rec)` redraws `gCurMember`, not `rec`'s member (5.9, 5.8).
- `ApplyCharEffect`'s slot -1 store lands on the two ring item slots and the top byte of `timers[9]`, not on padding as the source comment says (5.8).
- `E_TEXT.ASM`'s `GetScriptFlag` has a caller after all: `EOV0.C:221` reads the byte at 0000:03A0 directly. `DrawMaskedCell16` and `DrawShapeMasked` have none (2.5).
- The `MonsterType` fields of `EOB.H` are still `m00` to `m1A`; sections 5.6 and 5.7 give each field's meaning (armour class, to-hit number, hit dice, flag bytes, experience, size, sounds), a candidate for `make field`. `gMapPhase` and `StepMapPhase` are dead code.
