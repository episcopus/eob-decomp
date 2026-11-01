# 10 Map the programs

**Inputs:** Phase 0 done; the toolchain installed (`make tc-check`).

## Steps

1. `make overlays`: the segment table and overlays of `EOB.EXE` and `INTRO.EXE` (`build/re/<prog>.json`). Segment extents: whitepaper 2.2.
2. `make rtl-compare` (several minutes; `tools/re/rtl_compare.py --target eob/INTRO.EXE --releases tc100 -o build/re/rtl_intro.json` for the intro): where the runtime library starts in segment 0.
3. `make map`: functions (`tools/re/functions.py`), then `re/units.toml` and `re/symbols.toml` (`tools/re/map_units.py`). Names, signatures, levels and notes already in `re/symbols.toml` are kept.

## What the map says and how sure it is

- A **unit** is a code segment: Turbo C++ in the large model gives each source file its own segment. Segment 0 (`_TEXT`) is split into the startup, the assembly modules that use `_TEXT`, and the library.
- `kind` comes from the shape of the code and is [H] until the unit matches: `c` (C prologues, no `iret`, at most one near return), `asm` (interrupt handlers, near returns, data in the code, or registers pushed on entry), `unknown` (one or two small functions that could be either).
- An INTRO.EXE unit with `same_as` has the same instructions as that EOB.EXE unit (address fields aside): it is decompiled once.
- A function's `size` is the distance to the next known start, so it can include padding or a function the enumeration missed. Evidence for each start is in `evidence` (`ovl`, `far`, `near`, `frame`, `segment`).
