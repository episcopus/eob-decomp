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
| INTRO.EXE | not yet: the large model's link and one VROOMM overlay |
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

Still to come, from poolrad's experience: the overlay stub order (EOB.EXE's and INTRO.EXE's VROOMM stubs
list the entries other modules call, in an order the code does not give), external objects, and patch
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
