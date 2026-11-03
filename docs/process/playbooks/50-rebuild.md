# 50 The rebuild (Phase 5)

**Inputs:** SPEC section 10 (P5); a tree where `make check` passes; every function of the program at L2
(`make progress`); the pinned toolchain (`make tc-check`). Brought over from poolrad's playbook 50 and
adapted from Turbo Pascal to Turbo C++.

**Goal:** the program built from `src/` is the original: the same load image, the same relocation table,
the same overlay data (FBOV), and every golden session plays on it as on the original. `make rebuild
PROGRAM=<X.EXE>` builds and compares; it says `IDENTICAL`, and `the whole file is identical` when the EXE
header agrees too.

| Program | State |
|---|---|
| START1.EXE | identical, the whole file (8,652 bytes), 2026-11-03 |
| INTRO.EXE | links, 2026-11-03, but for the data two modules own (below) |
| EOB.EXE | not yet: the large model's link and seven overlays |

## Why the source that matches is not yet the program

`make match` compares each function's code and resolves every name by its address. A program can match
function by function and still not link, or link to other bytes, because four things only the full link
sees:

1. **One name per address.** A C file calls an assembly routine `PutChar` that the assembly defines as
   `PutChar` (C looks for `_PutChar`); the function the start-up code calls is `main`, whatever its file
   named it. `make rebuild` gives every address one name in its working copies (`build/rebuild/<program>/`),
   by address, as `make rename` does; `src/` keeps its names.
2. **Where every global is, and its initial value.** `src/` declares each global as an extern at its DGROUP
   offset (`extern char gModeMask; /*@ ds:00A8 */`); nothing defines one. `make rebuild` generates
   `DATA.ASM`: the original's bytes of the program's own data, from the end of the start-up code's data to
   the start of the library's, with a public at each declared offset, and the program's uninitialised data
   (_BSS, past the end of the load image) reserved. A probe link finds the library's data in the original by
   its bytes (pieces of it vote for one shift), which bounds the program's. Like poolrad's generated
   POOLVARS, this is a copy of the original's data, not yet C: P6 turns it into definitions.
3. **Link order.** Modules follow each other in the link, so the units go to TLINK in address order, between
   the start-up code (C0S, C0L) and the library (CS.LIB, CL.LIB). Within a unit of two files (START1's
   `S_TEXT.C` and `S_TEXT.ASM`), the file with the lower address comes first.
4. **Alignment.** A C module's code segment is byte aligned; TASM's simplified `.MODEL` adds a word-aligned
   `_TEXT` to every file, even one without code, and TLINK pads the next module to an even address (the
   first START1 rebuild was off by one byte from 0239h on). Generated data modules declare only `_DATA`
   and `_BSS`.

Found on INTRO.EXE (the large model, one overlay, 23 units built from their EOB.EXE twins' files):

5. **Twin files** carry EOB.EXE's addresses: a function's is translated by its offset in the unit, a
   global's by the twin proofs' relabelling, a call into a unit that is not a twin by the far calls the
   proofs record.
6. **The library by name.** A call into the runtime library takes the library's name for its address
   (rtl_compare; where it named nothing, the code found in the reference link: INTRO.EXE 0000:549D is
   `_close`, EOB.EXE 0000:60BD `_int86`). A name the source already uses that the library has too stays.
   When a header declares the library name with another prototype (`unlink` in dos.h), the copy's own
   declaration goes.
7. **Segments.** In the large model a unit in segment 0000 is part of `_TEXT` with the start-up code and
   the library (INTRO.EXE's AdLib driver), not a segment of its own. Far objects outside DGROUP are a
   generated `FAR_DATA` segment (its bytes) or a label at the start of the overlay segments TLINK fills
   (`_OVERLAY_`, `_OVRDATA_`).
8. **Overlay modules' place.** An overlay module's code is not in the image, so only its data shows where
   it was linked: INTRO.EXE's IOV0 right after I0892, its main module.
9. **Library modules nothing calls.** The original links GETCWD, which no code calls: an assembly
   module's unused `EXTRN` pulled it in. Where the library's modules stop lining up, the tool names the
   missing one from the reference link and adds an `EXTRN` for it.
10. **Uninitialised globals** go in their module's `_BSS`, before the library's (INTRO.EXE's library code
    addresses its BSS from ds:2E30, after Westwood's from ds:28A0): one generated `_BSS` module linked
    before the libraries, from where the start-up code clears (C0's `bdata`, found by its reference) to
    where the library's code addresses its own (found the same way). Not communal (`c_common` would go
    after the library's).
11. **A module's own data.** A C module's `_DATA` is its initialised globals, then one pool of every
    string literal in source order. The copy names each literal by its original address (`__s1A3C`,
    from the matcher's literal map) and declares it `extern`, so the compiled module has no data and the
    generated data supplies it. The data is then split by owner (item 15).
12. **Twin assembly** spells some addresses as EOB.EXE numbers: DGROUP (`212Eh`), a far segment
    (`mov bx,1A16h`, a relocated word, becomes `SEG` of a function or far object there), a DS or CS
    offset (`ds:[8496h]`, `cs:[4]`). Each takes the twin proof's value for that unit; a frame offset
    (`[bp+6]`) is never relabelled.
13. **Segment starts.** TLINK's segment table (`make overlays`) records where each segment begins, after
    alignment (the low four bits). The map can count a word-alignment pad as a unit's first byte; `src/`
    then has it as `db 0` (E1BBE.ASM, I0CDA.ASM, E1D9A.ASM's scratch), and the copy drops it. A segment
    that begins at an odd address (E1948.ASM at 084A:0005) was byte aligned: the copy declares it `BYTE`
    before `.MODEL`, which TASM 2.0 then keeps. An assembly data segment of its own (E1948's `SND`) is
    byte aligned, class `DATA`, filled with the original's bytes.
14. **Empty segments** are in the segment table too. `.MODEL` declares an empty code segment that TLINK
    lists (and aligns); the copies avoid it (segment-0 assembly without `.MODEL`, the `EXTRN` module
    without segments). The original has three of its own (INTRO.EXE 0892:0000, 0A19:0000, 0AED:000D):
    data-only modules (item 15), each an empty code segment in its place.
15. **Relocation order.** TLINK writes the relocation table module by module in fixup order. Each run of
    the original's data relocations follows the code relocations of the module that owns that data
    (I0892.C, E1C0E.C) or precedes a data-only module (after E1948.ASM, E1AAA.C, E1AAD.ASM). The data is
    split there, each piece linked after its owner, its fixups (`dw DGROUP`, `dw SEG f`) written with
    `ORG` in the original's order.
16. **A library variable the program defines.** INTRO.EXE defines `_ovrbuffer` (the overlay buffer, 00C7h
    paragraphs, ds:1A28) in its own data, so OVERLAY.LIB's OVRBUFF is not linked. The probe finds such a
    public: the original's library code addresses it apart from where the library's data moved.
17. **The output name** is written into the overlay data (`_EXEINFO_`): the link names its output after
    the program.

Left on INTRO.EXE (2026-11-03): 42 bytes. The **link date**: TLINK writes the date into `_EXEINFO_`
(INTRO.EXE: 29 April 1991), and emu2 gives the host's. The **overlay stub order**: TLINK numbers an
overlay module's stub entries in the reverse of its public definitions, and Turbo C writes those in an
order of its symbol hash of the names (and, for names that share a bucket, of definition order), so
the order is set by the original function names; one far call (0892:0161) and the stub table differ.
The human decided (2026-11-03) that both stay documented differences: the rebuild gives neither a
fixed date nor invented link names, and `test_intro_rebuilds_but_stub_order_and_date` pins the
42 bytes, so a change there shows.

Still to come, from poolrad's experience: external objects, and patch
sites (`re/patches.toml`, SPEC section 2: a byte the fan patch changed by hand in code is applied after
linking, never forced in C; START1's "Version 1.9" banner is data and needs none).

## Steps

1. `make check`, then `make rebuild PROGRAM=<X.EXE> V=1`. Each range that differs is printed with the unit
   or region it lies in. A shift (ours one or two bytes late from some address on) is alignment or a
   missing module; a word that is a DS offset on one side only is the data layout; a far call whose
   offset differs is the overlay stub order or the link order.
2. Fix the tool, not the bytes: a difference is a fact about how the original was built (an option, an
   order, a module), found once and then applied to every program. Never edit the copies by hand.
3. When it is identical, add the program to `tests/unit/test_rebuild.py`, so `make check` keeps it so.
