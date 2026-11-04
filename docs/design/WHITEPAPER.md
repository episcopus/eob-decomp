# Eye of the Beholder: how the program works

A living document. Every claim cites its evidence (an address, a tool, a test). Claims not yet proven on our files are tagged [H].

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

**Switches differ by unit.** `e1b82` (`src/E1B82.C`) is compiled with `-G` (optimise for speed): it cleans the stack after calls with `inc sp; inc sp` and `add sp,4` where the `-O -Z` units use `pop cx`. Its four functions match with `-ml -O -Z -G -P`, three of them with `-ml -O -Z -G` and none without `-G`. The small units `e1a16` and `e1a63` show the same `-G` pattern. A source file names its switches with `/*@flags ... */` (playbook 30). `eov0` (`src/EOV0.C`) is compiled with `-v` (source-level debugging): its `RunCharacterPanel` (ovl0:28F9) keeps two sibling calls to `ovl0:23EC` apart that `-Z` alone cross-jumps, and `make match` needs `-ml -O -Z -v` for it (2026-11-02).

An idiom found on `eov0`: when a function keeps a *parameter* in DI and a computed *local* in SI while both are live across calls, the local is marked `register` (`DrawPanelIcon`, ovl0:0128: `register int cellRow;` puts the cell row in SI and the parameter in DI; without `register` the parameter takes SI). Turbo C promotes an explicit `register` local over a parameter (2026-11-02).

**The C and C++ front ends differ.** `e1b82`'s `Unpack` (1B82:00C7) sign-extends its format byte to long and then switches on the low word (`mov al,es:[bx]; cbw; cwd; mov bx,ax; cmp bx,4; jbe; shl bx,1; jmp cs:[bx+20d]`). `TCC`'s C front end folds `(int)(long)char` to `char` (leaving `cbw` without `cwd`) and never turns a `long` switch into a jump table, while its C++ front end (`-P`) keeps the `cwd` and emits exactly these bytes. So a unit that shows a `long` conversion before a jump table is C++ (`-P`, names then mangled, so the file wraps its definitions in `extern "C"` for the matcher). The same front end can also fold `FP_SEG(p)` on a far-pointer local to `push dx` where the C front end reloads the segment; the original 1B82 pushes the memory word, so the file keeps the block's two halves in a union.

Idioms settled by matching (playbook 30 has the full list): an assignment tested and the result unused (`mov [g],ax; or ax,ax`) is an `if (g = x) ;` with an empty body (`SetUnpackBuffer`, 1B82:0217); two `continue` tests merged into one `||` keep ES:BX loaded (`UpdateObjects`, 08E6:0CC8); a `switch`'s case bodies are laid out in source order, so the jump table gives the order of the `case` labels.

`0ACD:091C` and `14EF:0274` keep identical case tails that the matcher's build merges (draft notes in `re/symbols.toml`). That merge is not `-O`: it is TCC's **internal assembler**, which cross-jumps a later basic block into an earlier identical one, while the original was assembled by **TASM 2.0 with SMART off**, which does not (found 2026-11-01 on `e14ef`: with `TCC -B` via TASM the whole unit matches, 6 of 6, 1410 bytes; `14EF:0274` 789 bytes). SMART off is also what fixes `e19ec`'s immediates (`or bx,1` is `0x81` in the original, `0x83` under TASM SMART, a TASM directive, not a listed switch). Open: a unit whose bytes need that inversion with `-c` cannot always take `-B` instead (TASM keeps TCC -S's forward-jump form). TCC -S inverts a conditional jump it estimates as more than 127 bytes from its target, and the estimate can exceed the built distance when the block holds a long argument computation (`e0d1c` `DrawRowList`, 0D1C:0465: with `-B`, `jne then; jmp else` where the original has `je`; probes 2026-11-01). `e0d1c` `FindBestUndeadTurner` is the mirror case: `-c` cross-jumps its two identical `bestVal = ...; best = who;` tails (6 bytes short) where the original keeps both, and `-B` keeps them apart but inverts the first `continue`, so neither flag set reproduces it; with `-c` 24 of 26 functions match, with `-B` 22 of 26.

No game code uses floating point: no emulator interrupts (`INT 34h` to `3Dh`) in any code segment. The switches are `cflags` in `tools/re/tc_toolchain.toml`; a module that needs others records them (`match_flags` in `re/symbols.toml`).

Three negative tests confirm the matcher rejects a near miss: a loop bound changed from 52 to 53, the wrong table passed, and an extra store (scratch probe, 2026-10-31).


**The struct copies are array initialisers** [V] (2026-11-01, escalations eov6 and e0d1c). All 15 calls of `F_SCOPY@` (0000:78C9) push the destination's segment before its offset (`push ss; lea ax,[bp-14h]; push ax`), then the source, then `mov cx,n`. Turbo C++ 1.00 compiles a local *array* initialised from a constant list (`char counters[6] = { 0, 0, 0, 0, 0, 0 };`, the initialiser kept as an unnamed block in DGROUP) exactly so, and a struct initialised or assigned with the offset first. The source is an array, not another compiler: Turbo C++ 1.01, Turbo C 2.0 and 2.01, Borland C++ 2.0 and Turbo C++ 3.0 were installed and tested on the way, and all agree with 1.00 on both forms, so the pin stays. (The lead came from a research pass; CampLoop's copy then matched.) Likewise `mov ax,..; and ax,K; or ax,0; je` is Turbo C++ 1.00's bit test of a `long` (verified), not a different code generator. TASM 1.0 and 1.01 assemble e14ef like TASM 2.0 `/jNOSMART` but break other units compiled with `-B`, so TASM 2.0 stays.

**Some units were compiled through the assembler** (escalation e14ef, 2026-11-01; superseded 2026-11-02 by the next paragraph: the units matched this way match with `-v`, without the assembler). Turbo C++'s own code generation (`-c`) merges identical jump tails; in `UpdateOptionList` (14EF:0274) the original keeps four case tails apart, and only `TCC -S` followed by TASM gives those bytes: the whole unit e14ef matches (6 of 6, 1,410 bytes) that way and not otherwise. That is what `TCC -B` does, which a file with inline `asm` gets automatically. The assembler encoded instructions without TASM 2.0's SMART forms (with SMART, `or bx,1` becomes `83` where the original has `81`), as TASM 2.0 `/jNOSMART` does, or an earlier TASM [H], and in two passes (`/m2`): one pass pads a forward jump that turns out short with a NOP (`EB xx 90`), which the original does not have (e1414 matches whole, 19 functions, 3,511 bytes, only with `/m2`). Of the 24 C units matched on 2026-11-01, all but E07C8 also match this way, so the switch is per file: `/*@flags ... -B */`. It does not explain `0ACD:091C`.
**The game modules were compiled with debug information** [V] (2026-11-02, escalations eov4, eov5, e122e, e0d1c). The original keeps identical code tails apart (case tails, `continue` tails, two equal stores) and, in the same functions, sizes every jump exactly. TCC's own code generation (`-c`) sizes jumps but merges the tails; `-B` keeps the tails but sizes jumps conservatively (a short `jne` 124 bytes from its target becomes `je over; jmp`), and TASM writes `xchg ax,bx` as `93` where the original has TCC's `87 D8`. With `-v` (source debugging; `-y`, line numbers, gives the same bytes) TCC does both: it does not merge code from different source lines and still sizes jumps itself. Rewording a tail does not help: the merge is on machine code. Evidence: every function that matched only under `-B` matches under `-v` (e14ef, e1414, e082c, e08e6, e0acd, e16a5, eov4); the functions that refused `-B` (DisarmMemberEffect 122E:01D9, MarkMemberItemsOp 122E:1CAD, DrawRowList 0D1C:0454, SurrenderPanel ovl5:10A6, DrawSlotMonsters's `87 D8`) match under it too; three drafts no build reproduced match (WriteObjectRunOp 122E:0A16, DrawCharPortrait ovl5:00A1, FindBestUndeadTurner); and every C unit of src/ that matches still matches with `-v` added, except e1de7. The two-file explanation (a unit made of a `-B` file and a `-c` file) fails, because in e122e and eov4 the two kinds of function alternate by address. The exception fits a pattern: e1de7 is one of the shared library modules (`-G`, the units INTRO.EXE links too), and its DrawTextLine needs its tail merged, so the library was built without debug information. The debug information stays in the OBJ files and never reaches the EXE (TLINK without `/v`), so only the code shows it. E0BD2 and EOV0 had found `-v` earlier for the same reason.

### 2.1.2 What segment 0 holds

Segment `0000` (`_TEXT`, 31,386 bytes) starts with `C0L` (524 bytes), then 19,996 bytes that are not library code (`0000:020C` to `0000:5028`), then the runtime library from `0000:5028` (`OVRHALT`, the first module found) to the end, 10,866 bytes. The 20 KB are one assembly module, Westwood's AdLib sound driver, which `src/E_TEXT.ASM` reproduces byte for byte (`make match`, the whole unit, TASM 2.0 `/mx`): code and data in one segment that runs with DS = CS. [V] Its parts, in address order: the pitch curve tables and the script flag (`020C` to `03A0`), five far routines the game calls (`03AA` to `03FA`: the block's address, a byte write and a byte read in a sound record, a sound's data pointer, and `03FA`, which adds `0416h` to the first 300 words of the block), the 32C8h data block (`0416` to `36DD`; `LoadSoundDriver` (177D:000A) reads `adlib.dat` or `pcgamesn.dat` over it, so the EXE's bytes there are an image that is overwritten), the tables (`3944`: the entry table of eleven routines, fourteen level curves), the variables (`3B36`), ten voice records of 43h bytes (`3B93`), the voice, operator and F-number tables (`3E31`), the two INT 08h handlers (`3E66` the AdLib timer, divided from a PIT divisor of 0555h into a driver tick every 0Ch passes and a BIOS chain every 30h; `3EE5` the 4-bit delta sample player, PIT divisor 004Bh), the driver (`3F77` to `4F83`: start, stop, tick, the sound queue, the per-voice script interpreter `418F` and its register helpers) and the interpreter's 74-entry command jump table (`4F94`). The PC speaker and Tandy driver is the separate unit `E1948.ASM` (segment `1948`); units `e_c0`, `e_text`, `e_lib` in `re/units.toml`. `make map` took `03FA` to `3F76` for one function and the table `4F94` for part of `SoundRandom`; `re/symbols.toml` lists the parts one by one. `INTRO.EXE` has the same three parts, its library from `0000:5400`.

### 2.1.3 Modules

`make map` (playbook 10) writes `re/units.toml` and `re/symbols.toml`:

| Program | Units | By kind ([H] until matched) | Functions |
|---|---:|---|---:|
| `EOB.EXE` | 62 | 30 C, 25 assembly, 5 unknown, startup, library (7 of the units are overlays) | 885 |
| `INTRO.EXE` | 33 | 8 C, 19 assembly, 4 unknown, startup, library; 21 units have the same code as an `EOB.EXE` unit | 250 |
| `START1.EXE` | 1 | not mapped yet | |

Matched (L2): `e0cf7` (6 functions) and `e07a9` (2 functions), whole units.

### 2.2 EOB.EXE layout

- Entry `0000:0000` is Borland's startup (`c0`): `mov dx,212Eh` then DOS version (`int 21h`, `ah=30h`) and `mov ds,dx`. DGROUP is paragraph `212E`. Stack `312D:0080`.
- The FBOV header (section 2.3) gives the file offset of TLINK's segment table, 144,400 (load module offset `20C10h`), 99 entries of 8 bytes: frame paragraph, end offset in that frame (exclusive; `FFFF` for no extent), kind, flags. `make overlays` writes it to `build/re/eob.json`.
- **Segments are byte aligned**: each starts where the previous one ends, inside the paragraph that holds that end, so the frame paragraph is the start rounded down. `main` is at `07C8:0001`, right after the `retf` at `07C8:0000` that ends the module framed at `07A9`; that module starts at `07A9:000A`. Some entries have zero length. (Phase 0 first read the second field as size - 1; corrected 2026-10-31, `tests/unit/test_cmatch.py`.)
- Kinds: 1 code (58 entries, 53 not empty, 127,435 bytes), 0 data (DGROUP among them, `212E`, ending at `212E:FFE4`), 3 overlay stub (7), 4 not established (12 entries). The flags take values 0 to 15; meaning not established.
- Segment `0000` is 31,386 bytes with 136 near returns: the startup, about 20 KB of assembly, and the runtime library (section 2.1.2). Every other non-empty code segment is one Westwood module: the runtime library is all in segment 0. Segment `17A6` (6,694 bytes) has 36 `iret`, 77 `out` and `int 10h`: [H] hand-written interrupt and hardware code.

### 2.3 Overlays (VROOMM)

The overlay data follows the load module in the EXE (`tools/re/fbov.py`):

| Offset | Field |
|---|---|
| +00 | "FBOV" |
| +04 | u32 bytes after this 16-byte header (52,672 in `EOB.EXE`, 12,272 in `INTRO.EXE`) |
| +08 | u32 file offset of the segment table |
| +0C | u32 segment table entries (99, 54) |
| +10 | one record per overlaid segment: code, u16 fixup offsets, zero padding to the next record |

Overlay stub segments in the load module have Turbo Pascal's layout (poolrad whitepaper, `tpovl.py`): `CD 3F`, u32 record offset (from +10), u16 code bytes, u16 fixup bytes, u16 entry count, u16 link, 16 runtime bytes zero on disk, then 5-byte entries `CD 3F`, u16 code offset, 0.

| Program | Stub paragraphs | Entries | Code bytes |
|---|---|---:|---:|
| `EOB.EXE` | `20F6 2102 2106 210C 2116 211E 212A` | 125 | 50,641 |
| `INTRO.EXE` | `0E96` | 22 | 11,741 |

**Segment fixups in overlay code hold a byte offset into the segment table** (index times 8), not a paragraph: overlay 2 calls `FileOpen` as `0188:00DB`, and entry 49 (188h / 8) of the table is `1C0E`, where the resident code calls it as `1C0E:00DB`. `tools/match/cmatch.py` translates them (`Original.seg`). Overlay code compiles with the resident units' switches: `ovl2:0110` and `ovl2:09CC` match without `-Y` (`src/EOV2.C`).

The padding between records is 0 to 42 bytes in `EOB.EXE` and 169 bytes after `INTRO.EXE`'s one record; the rule is not established. Fixup offsets within a record come in runs that descend within each 1 KB of code (not explained yet).

### 2.4 INTRO.EXE and START1.EXE

- `INTRO.EXE`: 54 segments, 34 code (59,286 bytes). It names the same data files as `EOB.EXE` (`eobdata*.pak`, `WESTWOOD.COL`, `SSI.CMP`, the graphics drivers): [H] the two programs share library modules.
- `START1.EXE`: one small program without overlays or a segment table: the setup menu (CGA, Tandy, EGA, VGA; AdLib or Sound Blaster, PC speaker, Tandy speaker), then runs `INTRO.EXE`. **Small model**: its first 193 instructions are Turbo C++ 1.00's `C0S.OBJ`, the 18 that differ all DGROUP offsets (scratch probe, 2026-10-31). Being edited by the patch ("Version 1.9", section 1), it may hold bytes no compiler wrote.

### 2.5 Graphics drivers

`CGA.OVL`, `EGA.OVL`, `MCGA.OVL`, `TGA.OVL`: MZ files with 40 relocations each and entry `0000:0000`, loaded by the game with DOS function 4B03h (load overlay: an image relocated to a segment of the caller's choosing, no PSP); the VGA mode loads `MCGA.OVL` (oracle trace, section 2.6). `EOB.EXE` also names `vga.ovl`, which is not in the copy. [H] Assembly.

### 2.6 Start-up and command line

`START1.EXE` asks for graphics, sound and the mouse and runs `spawnl(.., "INTRO.EXE", "INTRO.EXE", graphics, sound, mouse, "homie", NULL)` (START1 0000:0447), each choice a one-character string ('0' + index). `INTRO.EXE` passes them on to `EOB.EXE` with a fifth argument. `main` (07C8:0001):

| Argument | Use |
|---|---|
| `argv[1][0]` | graphics: `ds:8102`, reduced by '1' in 07C8:0199 to 1 Tandy (the MCB check `CheckMCB` runs), 3 VGA (clears A000:0000 with `memset`, palette `eobpal.col`), then the mode number from the table at `ds:84A2` through `int86` (1DDD:002D) |
| `argv[2][0]` | sound: `ds:B892` = digit - '1'; `ds:80E6` = sound on unless '4' |
| `argv[3][0]` | mouse: digit - '0' |
| `argv[4]` | must be "homie" (`strcmp`), or the program ends at once: the reason for the string found in Phase 0 |
| `argv[5]` | `atol`: `ds:84A0`, the video mode restored on exit |

Without enough memory `main` prints "Not enough memory to run program." through `PrintString` (19EA:0000, `src/E19EA.ASM`).

`INTRO.EXE` does not exec `EOB.EXE` [V] (oracle, sessions `intro-play` and `intro-skip`). When the intro ends (after 147 s of virtual time) or on ESC it writes the command tail " 3 4 0 homie 3 0" (its own four arguments, the video mode to restore, and a sixth argument "0" [H] unused by `main`) into its own PSP, fills the PSP's two FCBs with DOS 29h, loads `EOB.EXE` with DOS 4B03h (load overlay) at segment 1000h, the same place DOS loads it, and jumps to it. So `EOB.EXE` addresses are the same whether it runs alone or after the intro, and the title menu belongs to `EOB.EXE`.

**INTRO.EXE shares its library with EOB.EXE** [V] (`make twin`, 2026-11-02). 23 of its 32 units are EOB units again: the same object module, every instruction and length equal, only addresses moved, each EOB address to one INTRO address (e1948's data block 4 bytes later, its code one byte earlier). They are the sound, graphics, input, file and memory routines (e1948 to e1e8b, the C ones e19bc, e19ec, e1a29, e1a47, e1c0e, e1de7 included), 164 functions proven with their twins' sources. INTRO.EXE's own code is its overlay (22 functions), i0892 (its out-of-memory exit at 0892:08C8 among them, compiled `-G` like e1b82), segment 0 (i_text) and four small units; i0aed is e1b82 in another version (it differs at +0204) and i0923 is three bytes shorter than e1990.

**START1.EXE is small model** [V] (2026-11-03). Its 8,140-byte load module has 3 relocations, all the DGROUP paragraph 01AA loaded by the start-up code (0000:0001), and no far call: one code segment, near calls (`-ms`, C0S and CS.LIB). `rtl_compare.py --model s` finds 38 Turbo C++ 1.00 library modules in it (31 of 1.01's); the library runs from ATEXIT at 0682h to the end of the code at 1AA0h (OVRHP, "found" at 0574h, is one byte of code and matched by chance; review of s_text, 2026-11-03). With no overlays TLINK wrote no segment table, so the map is three units: the start-up code (0000 to 0239h, the size of the small model's C0.ASM), Westwood's code (s_text, 0239h to 0682h, 1,097 bytes, 15 functions in two modules: a C module, the menu at 0000:0239 that ends in `execl` of INTRO.EXE and a string printer at 0000:0474, and an assembly module of video probes (0496 to 05F5) and console and mouse routines (05F6 to 0681), src/S_TEXT.C and src/S_TEXT.ASM) and the library. Its strings show the patch's edit ("Eye of the Beholder Version 1.9"); a byte the patch changed in code is a patch site (SPEC 2), not a source form.

**START1.EXE rebuilds identical from src/** [V] (`make rebuild`, 2026-11-03): src/S_TEXT.C, src/S_TEXT.ASM and a data module generated from the original's DGROUP, linked by TLINK 3.0 between C0S and CS.LIB, give the whole file, all 8,652 bytes, header included. The original link order is the start-up code, the C module (0239h), the assembly module (0496h), the library; the program's data is ds:00A8 to 0340 (after C0.ASM's 0A8h bytes, before the library's, which the probe link finds shifted by 0298h) and its uninitialised data ds:052C to 053A, past the end of the image. The C module's code is byte aligned: it follows the start-up code at the odd 0239h.

**INTRO.EXE rebuilds from src/ but for 42 bytes** [V] (`make rebuild PROGRAM=INTRO.EXE`, 2026-11-03): the load image, the relocation table (806 entries, in order) and the overlay are the original's except the link date and the overlay stub order. What the rebuild showed about how it was built (playbook 50, items 10 to 17): the initialised data was owned by modules, among them three data-only modules (empty code segments in TLINK's table at 0892:0000, 0A19:0000 and 0AED:000D, their data's relocations right after the module before them); I0892.C and E1C0E.C held data of their own; uninitialised globals sit in their modules' `_BSS` (ds:28A0 to 2E30) before the library's, not communal; the program defines OVERLAY.LIB's `_ovrbuffer` itself (00C7h paragraphs, ds:1A28); E1948.ASM's code segment is byte aligned and its `SND` data segment byte aligned in class DATA (frame 110C, its data from ds:26DA); the `db 0` that src/ keeps at the start of E1BBE.ASM, I0CDA.ASM and E1D9A.ASM is a word-alignment pad (the segment table records each segment's start after alignment). TLINK wrote the output name ("intro.exe") and the date of the link into the overlay data: **29 April 1991**. The stub order: TLINK numbers an overlay module's stub entries in the reverse order of its public definitions, and Turbo C++ 1.00 writes a module's publics in the order of its symbol hash of their names, not of definition (tested: six functions defined in three orders give one order), so the stub table encodes the original names of IOV0's 22 functions, not recovered.

**EOB.EXE rebuilds from src/ but for the stub order, the link date and one hook** [V] (`make rebuild PROGRAM=EOB.EXE`, 2026-11-03): the load image (201,552 bytes), all 2,504 relocations in the original's order, every overlay's code to its length and every overlay fixup but three are the original's. What differs: the link date (29 April 1991, as INTRO.EXE); the stub entries, which follow the original function names (the stub tables, 26 far pointers in the image, far calls in five overlays); and three fixups in overlay 5, where `gScrollHook` is set to its own MessagePanel through the stub, as Turbo C++ writes only for a function of another module [H]. What the rebuild showed about how it was built (playbook 50, items 18 to 27): the overlay modules were linked among the resident ones, EOV0 and EOV1 after E1414 and EOV3, EOV5 and EOV6 after E177D (their data and its relocations say so; EOV2 and EOV4, with no data, go between their neighbours, as TLINK numbers overlays in link order); 15 overlay functions were static (no stub entry: five of overlay 0's, ten of overlay 1's); several overlay modules owned initialised globals between their templates (E122E's g5A40, EOV5's menu tables of far pointers and gYesNoMenu), so src/'s externs there are definitions in the original; the program defines `_ovrbuffer` (02CBh paragraphs, ds:3088); E07C8.C's gArgv is the library's `_argv`; E_TEXT.ASM's byte at 0000:03A0 is the far object overlay 0 reads (now `gScriptFlag` in both files); a segment in an overlay's code is TLINK's segment-table index times 8, so EOV3.C's `MK_FP(0x1d8, 0)` is gMapSheet (entry 59).

### 2.7 Input and sound

- **Keyboard: the game's own INT 9 handler.** `EOB.EXE` never calls INT 16h; its handler (assembly unit `e1aad`) reads scancodes from port 60h (1AAD:080C) and acknowledges the interrupt controller itself. It buffers keys: keys typed before the title menu exists are applied once it does (oracle, `tests/sessions/boot-title.keys`).
- **Mouse: an INT 33h event handler.** The game (1A72:0106) looks at the INT 33h vector and decides there is no driver if it is null or points at an `IRET`; otherwise it resets the driver (function 0, expects FFFFh), reads the position (function 3) and treats x above A0h as a 640-wide range (doubling its own x), sets the position (4), installs a handler at 1A72:017A for moves and both buttons (function 0Ch, mask 1Fh) and shows the pointer (1). The game draws its own pointer.
- **Arrow keys move without turning**: UP forward, LEFT and RIGHT one square sideways (`tests/sessions/load-walk.keys`).
- **AdLib**: with sound '1' the game does not detect the card; it sets waveform select (register 01h = 20h), clears the CSM and rhythm registers and puts every operator at full attenuation (40h to 55h = 3Fh): 21 writes before the title menu. In the dungeon it makes about 10,000 register writes in 25 seconds of play.
- **Saved game**: "Load game in progress" reads `EOBDATA.SAV` and writes `LEVELS.TMP` (the levels visited, kept while playing).

**In the oracle** (`make oracle-run`, `tools/oracle/run.py`, arguments `3 4 0 homie 3`), `EOB.EXE` sets mode 13h, loads `MCGA.OVL`, both fonts and four PAKs, runs the overlay manager, and shows its title menu ("Load game in progress", "Start a new party", "Exit to DOS") after 30 million instructions; two runs end in the same state digest (`tests/unit/test_oracle.py`).

## 4. Data files

### 4.1 PAK archives

`tools/re/pak.py`, `make pak`, `tests/unit/test_pak.py`. A directory of entries (u32 offset, NUL-terminated name) ending at the first file's offset; files back to back; a file's size is the distance to the next offset (the last one to the end of the file). The directory's last 4 bytes are a nameless terminator: the file size (EOBDATA1, 5), zero (EOBDATA3, 4, 6) or `0C 00 10 04` (EOBDATA2). All 6 archives, 193 files, rebuild byte for byte.

| Archive | Files | Contents (by extension) |
|---|---:|---|
| EOBDATA1 | 17 | sound drivers data (`ADLIB.DAT`, `PCSOUND.DAT`, `PCGAMESN.DAT`), intro pictures (`.CMP`) |
| EOBDATA2 | 2 | `DOOR.EGA`, `EOBPAL.COL` |
| EOBDATA3 to 6 | 174 | per level set: wall graphics (`.VCN`, `.VMP`, `.ECN`, `.EMP`), pictures (`.CPS`, `.EGA`), palettes (`.PAL`, `.COL`), `.DAT`, level maps (`LEVELn.MAZ`) and scripts (`LEVELn.INF`) |

### 4.2 Compressed files

CPS, CMP, EGA, INF, VCN and ECN files share the header the game's loader reads (`LoadPacked` and `Unpack`, 1B82:0041 and 1B82:00C7, `src/E1B82.C`): u16 size of the rest (some files: the file size), u8 format, u8 0, u32 unpacked size (`src+2`), u16 bytes of palette that follow (`src+6`). `Unpack` switches on the format: 0 stored (copied from `src+8`), 1 and 2 decoded with a work buffer of 3C00h and FC00h bytes (allocated unless the shared buffer `ds:812A`, size `ds:812C`, is big enough), 3, and 4 (LCW, after the palette).

`make data` (`tools/re/wwfmt.py`) decodes every file: 150 files of format 4 unpack to exactly their declared size (LCW as published: back references, literals, absolute copies, fills). `TITLE-V.CMP` with `TITLE-V.COL` is the title picture. One anomaly: `EOBTITLE.CMP` decodes coherently for 31,962 bytes (99 rows of a title picture), then a fill of 13,082 bytes and garbage past 64,000; [H] damaged in this copy. The game's own routine agrees [V]: `tests/unit/test_wwfmt.py` runs Unpack4 (1D9A:0013) in the oracle on all 131 files of format 4 and compares every byte, `EOBTITLE.CMP` included (the game writes the same 64,000 bytes, so the file is odd, not the decoder).

Format 1 [V] (1D9A:018B, `wwfmt.lzw12`; the five EGA wall-set pictures `BRICK1`, `BRICK2`, `BRICK3`, `BLUE`, `DROW`): 12-bit LZW. Codes are big-endian, two in three bytes (1D9A:0299 alternates whole and half bytes). A code below 100h is a literal; from 100h it names a dictionary entry, which is (offset, length) of a string already written to the output, 4 bytes in the 3C00h work buffer, so at most F00h entries, then no more are added and the dictionary is never reset. Each code after the first adds the previous string plus the first byte of the current one; a code naming the entry being made is the previous string plus its own first byte. The game's routine, run in the oracle on the five files, writes the same bytes. No pinned file uses formats 2 or 3.

### 4.3 Mazes and tables

`LEVELn.MAZ` (12 files): u16 width 32, u16 height 32, u16 4 bytes per cell, then the cells (4,102 bytes). `.VMP` and `.EMP`: u16 count, then u16 entries. `.PAL` and `.COL`: 768 bytes, 6-bit VGA colours. `EOBDATA4.PAK` lists its eight `.COL` files twice.

[H] (ScummVM's Kyra engine, read only): `.INF` is the level script bytecode; the four bytes of a maze cell are its four walls.

### 4.4 The game's own files

`tools/re/eobfmt.py`, `make data`, `tests/unit/test_eobfmt.py`. Files without the Westwood header. Where the loader was not read yet, the layout comes from the reads the oracle sees during `load-walk`: the read sizes in order, and the caller of the read through the BP chain, resolved to overlay code from the memory at the time of the read (overlays are swapped; resolving at the end of the run gave wrong overlays). The field meanings are [H] until the code that uses them is lifted.

| File | Layout | Evidence |
|---|---|---|
| `FONT6.FNT`, `FONT8.FNT` | u16 bytes after this field; u16 offsets[128] of the glyphs; u8 height, u8 width at 102h; 128 glyphs from 104h, height × ((width + 7) / 8) bytes, one bit per pixel. 6 × 6 and 8 × 8 | [V] 1A47:0005 checks each of these |
| `TEXT.DAT` | u16 offsets of 51 NUL-terminated strings, the table ending at the first: the dwarf and other story texts | invariants over the file |
| `ITEM.DAT` | u16 count (432), records of 14 bytes; u16 count (98), names of 35 bytes | read by ovl2:005E (the call returns to ovl2:0088), which `LoadItemData` (ovl2:09CC, `src/EOV2.C`, L2) calls |
| `ITEMTYPE.DAT` | u16 count (57), records of 16 bytes | [V] `LoadItemTypes` (ovl2:0110, `src/EOV2.C`, L2): count, then a 16-byte `FileRead` per record into `ds:A9F0` |
| wall set `.DAT` (`BRICK`, `BLUE`, `GREEN`, `XANATHA`, `DROW`) | u16 count, decorations of 52 bytes; u16 count, rectangles of 8 bytes [H] (x, y, w, h); a decoration's first 10 bytes index the rectangles or are FFh | read by ovl3:0096 (the call returns there), three times per level load; the index invariant holds in all five |
| `ITEMRMP.VGA`, `ITEMRMP.EGA` | 1,600 bytes read whole (the name is `ITEMRMP.%s` by display); [H] 100 rows of 16 colours | read at ovl2:0484 (in ovl2:0171, called from ovl2:0577, which `StartupAlloc` calls); [V] into the 1,600-byte buffer `ds:F534` that `StartupAlloc` (07A9:000A, `src/E07A9.C`) allocates |
| `ADLIB.DAT`, `PCGAMESN.DAT` | u8 map[120] from the game's sound number to the driver's (values from 96h are ignored, 177D:0070), then the driver's data, at most 32C8h bytes: u16 offsets, the first one the size of their table | [V] 177D:000A loads `adlib.dat` or `pcgamesn.dat` by `ds:B892`; the map goes to the 120-byte buffer `ds:DCAC` that `StartupAlloc` allocates, which calls 177D:000A next |
| `PCSOUND.DAT`, `SOUND.DAT` | the same layout [H]; `EOB.EXE` names them only in its PAK catalogue, and `SOUND.DAT`'s data (13,019 bytes) would not fit the buffer: [H] not used | |
| `EOBDATA.SAV` | 6 characters of 243 bytes (u8 index, u8 in the party, name[11], ...), 64 bytes, 500 items of 14 bytes (the `ITEM.DAT` record size), 12 level states of 2,040 bytes, 3 bytes, 96 bytes: 33,101 bytes | read by ovl3:160A (the call returns there); decode and encode give the same bytes |
| `LEVELS.TMP` | 12 level states of 2,040 bytes | [V] loading the test save makes the game write the save's 12 level states here, in order (slow test); one is read back per level (the call returns to ovl3:1194). A run of the oracle hides both files unless the session names a save: `LEVELS.TMP` is then made from the save, as the game leaves it after saving (loading reads it before rewriting it and stalls without it) |

Every file of the PAKs and both fonts decode (`make data`). The file names in the data segment around `ds:1EE0` are the PAK catalogue (which archive holds which file), not the loaders' arguments: the loaders build names with `%s.%s` and `ITEMRMP.%s`, or use lower-case copies (`adlib.dat`, ds:5E2C).
