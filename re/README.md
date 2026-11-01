# re/: the reverse-engineering database

Text files are the source of truth; anything in `build/` is regenerated from them and the pinned inputs.

| File | What | Written by |
|---|---|---|
| `inputs.toml` | hashes of the pinned game files | `tools/setup/inputs.py --write` (humans only) |
| `units.toml` | one entry per module of each program: id, program, segment, start, bytes, functions, kind, `same_as` | `make map` |
| `symbols.toml` | one entry per function: id, unit, addr, size, evidence; then name, signature, level, match_flags, notes, review | `make map` (enumeration), `make match WRITE=1` (name, level) |

Ids: `e` for EOB.EXE, `i` for INTRO.EXE, then the frame paragraph of the code segment (`e0cf7`); `e_c0`, `e_text`, `e_lib` for the three parts of segment 0; `eov0` to `eov6` for overlays; `s1` for START1.EXE. A function id is its unit's id and its offset in the segment frame (`e0cf7_0007`); in segment 0, `e0000_<offset>`.

Levels: absent (enumerated only), `L2` (matched by `make match`). Only tools write levels.
