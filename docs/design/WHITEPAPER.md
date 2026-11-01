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
| `-O` jump optimisation | needed: with `-Z` alone `ListInit` keeps a second `les bx`; `-O` also merges the common tails of `if (g8102) f(a); else f(b);` in `Level07A9` |
| `-Z` register load suppression | needed: neither module matches without it; `xor ax,ax; push ax` is reused for the next zero argument |
| `-r` register variables (default) | `-r-` breaks both modules (SI holds the loop counter) |
| no `-G` (optimise for size, default) | `-G` breaks both modules |
| no `-1` (8086 code, default) | `-1` breaks both modules; `shl` by 1 twice, `mov cl,4; shl ax,cl` |
| no `-N` (no stack check, default) | `-N` breaks both modules |
| `-K`, `-a`, `-d` | not decided: these modules use no plain `char`, no structure layout that `-a` changes and no repeated string |

No game code uses floating point: no emulator interrupts (`INT 34h` to `3Dh`) in any code segment. The switches are `cflags` in `tools/re/tc_toolchain.toml`; a module that needs others records them (`match_flags` in `re/symbols.toml`).

Three negative tests confirm the matcher rejects a near miss: a loop bound changed from 52 to 53, the wrong table passed, and an extra store (scratch probe, 2026-10-31).

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

The padding between records is 0 to 42 bytes in `EOB.EXE` and 169 bytes after `INTRO.EXE`'s one record; the rule is not established. Fixup offsets within a record come in runs that descend within each 1 KB of code (not explained yet).

### 2.4 INTRO.EXE and START1.EXE

- `INTRO.EXE`: 54 segments, 34 code (59,286 bytes). It names the same data files as `EOB.EXE` (`eobdata*.pak`, `WESTWOOD.COL`, `SSI.CMP`, the graphics drivers): [H] the two programs share library modules.
- `START1.EXE`: one small program without overlays or a segment table: the setup menu (CGA, Tandy, EGA, VGA; AdLib or Sound Blaster, PC speaker, Tandy speaker), then runs `INTRO.EXE`. **Small model**: its first 193 instructions are Turbo C++ 1.00's `C0S.OBJ`, the 18 that differ all DGROUP offsets (scratch probe, 2026-10-31). Being edited by the patch ("Version 1.9", section 1), it may hold bytes no compiler wrote.

### 2.5 Graphics drivers

`CGA.OVL`, `EGA.OVL`, `MCGA.OVL`, `TGA.OVL`: MZ files with 40 relocations each and entry `0000:0000`, loaded by the game ("Could not load overlay" message in `EOB.EXE`). `EOB.EXE` also names `vga.ovl`, which is not in the copy. [H] Assembly.

## 4. Data files

### 4.1 PAK archives

`tools/re/pak.py`, `make pak`, `tests/unit/test_pak.py`. A directory of entries (u32 offset, NUL-terminated name) ending at the first file's offset; files back to back; a file's size is the distance to the next offset (the last one to the end of the file). The directory's last 4 bytes are a nameless terminator: the file size (EOBDATA1, 5), zero (EOBDATA3, 4, 6) or `0C 00 10 04` (EOBDATA2). All 6 archives, 193 files, rebuild byte for byte.

| Archive | Files | Contents (by extension) |
|---|---:|---|
| EOBDATA1 | 17 | sound drivers data (`ADLIB.DAT`, `PCSOUND.DAT`, `PCGAMESN.DAT`), intro pictures (`.CMP`) |
| EOBDATA2 | 2 | `DOOR.EGA`, `EOBPAL.COL` |
| EOBDATA3 to 6 | 174 | per level set: wall graphics (`.VCN`, `.VMP`, `.ECN`, `.EMP`), pictures (`.CPS`, `.EGA`), palettes (`.PAL`, `.COL`), `.DAT`, level maps (`LEVELn.MAZ`) and scripts (`LEVELn.INF`) |

[H] (ScummVM's Kyra engine, read only): `.CPS` and `.CMP` are Westwood compressed pictures (LCW, "format 80"), `.INF` is the level script bytecode. Codecs come in Phase 2.
