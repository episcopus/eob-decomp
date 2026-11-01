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
| `-O` jump optimisation | needed: with `-Z` alone `ListInit` keeps a second `les bx`; `-O` also merges the common tails of `if (g8102) f(a); else f(b);` in `f07A9_010C` |
| `-Z` register load suppression | needed: neither module matches without it; `xor ax,ax; push ax` is reused for the next zero argument |
| `-r` register variables (default) | `-r-` breaks both modules (SI holds the loop counter) |
| no `-G` (optimise for size, default) | `-G` breaks both modules |
| no `-1` (8086 code, default) | `-1` breaks both modules; `shl` by 1 twice, `mov cl,4; shl ax,cl` |
| no `-N` (no stack check, default) | `-N` breaks both modules |
| `-K`, `-a`, `-d` | not decided: these modules use no plain `char`, no structure layout that `-a` changes and no repeated string |

**Switches differ by unit.** `e1b82` (`src/E1B82.C`) is compiled with `-G` (optimise for speed): it cleans the stack after calls with `inc sp; inc sp` and `add sp,4` where the `-O -Z` units use `pop cx`. Its four functions match with `-ml -O -Z -G -P`, three of them with `-ml -O -Z -G` and none without `-G`. The small units `e1a16` and `e1a63` show the same `-G` pattern. A source file names its switches with `/*@flags ... */` (playbook 30).

**The C and C++ front ends differ.** `e1b82`'s `Unpack` (1B82:00C7) sign-extends its format byte to long and then switches on the low word (`mov al,es:[bx]; cbw; cwd; mov bx,ax; cmp bx,4; jbe; shl bx,1; jmp cs:[bx+20d]`). `TCC`'s C front end folds `(int)(long)char` to `char` (leaving `cbw` without `cwd`) and never turns a `long` switch into a jump table, while its C++ front end (`-P`) keeps the `cwd` and emits exactly these bytes. So a unit that shows a `long` conversion before a jump table is C++ (`-P`, names then mangled, so the file wraps its definitions in `extern "C"` for the matcher). The same front end can also fold `FP_SEG(p)` on a far-pointer local to `push dx` where the C front end reloads the segment; the original 1B82 pushes the memory word, so the file keeps the block's two halves in a union.

Idioms settled by matching (playbook 30 has the full list): an assignment tested and the result unused (`mov [g],ax; or ax,ax`) is an `if (g = x) ;` with an empty body (`SetUnpackBuffer`, 1B82:0217); two `continue` tests merged into one `||` keep ES:BX loaded (`UpdateObjects`, 08E6:0CC8); a `switch`'s case bodies are laid out in source order, so the jump table gives the order of the `case` labels.

Not reproduced yet (drafts, notes in `re/symbols.toml`): `0ACD:091C`, where the original keeps two identical tails that our `-O` merges.

No game code uses floating point: no emulator interrupts (`INT 34h` to `3Dh`) in any code segment. The switches are `cflags` in `tools/re/tc_toolchain.toml`; a module that needs others records them (`match_flags` in `re/symbols.toml`).

Three negative tests confirm the matcher rejects a near miss: a loop bound changed from 52 to 53, the wrong table passed, and an extra store (scratch probe, 2026-10-31).


**Some units were compiled through the assembler** [V] (escalation e14ef, 2026-11-01). Turbo C++'s own code generation (`-c`) merges identical jump tails; in `UpdateOptionList` (14EF:0274) the original keeps four case tails apart, and only `TCC -S` followed by TASM gives those bytes: the whole unit e14ef matches (6 of 6, 1,410 bytes) that way and not otherwise. That is what `TCC -B` does, which a file with inline `asm` gets automatically. The assembler encoded instructions without TASM 2.0's SMART forms (with SMART, `or bx,1` becomes `83` where the original has `81`), as TASM 2.0 `/jNOSMART` does, or an earlier TASM [H]. Of the 24 C units matched on 2026-11-01, all but E07C8 also match this way, so the switch is per file: `/*@flags ... -B */`. It does not explain `0ACD:091C`.
### 2.1.2 What segment 0 holds

Segment `0000` (`_TEXT`, 31,386 bytes) starts with `C0L` (524 bytes), then 19,996 bytes that are not library code (`0000:020C` to `0000:5028`), then the runtime library from `0000:5028` (`OVRHALT`, the first module found) to the end, 10,866 bytes. The 20 KB are assembly: lookup tables kept in the code segment, a routine that loads DS from CS to fix up a table of words (`0000:03FA`), near calls. [H] They are Westwood's assembly modules that use `_TEXT` (what TASM's simplified segments name a large-model module's code), linked between the startup and the library; units `e_c0`, `e_text`, `e_lib` in `re/units.toml`. `INTRO.EXE` has the same three parts, its library from `0000:5400`.

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
