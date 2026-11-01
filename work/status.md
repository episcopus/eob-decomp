# Project status

2026-11-01.

| Phase | State |
|---|---|
| P0 Bootstrap | done: `make doctor`, `make p0-check` (16 files pinned; 99 + 54 segments; 7 + 1 overlays; 6 PAKs, 193 files) |
| P1 Map and toolchain | done: Turbo C++ 1.00, `-ml -O -Z` (some units `-G`), TLINK 3.0, TASM 2.0 [H]; `make map`: EOB.EXE 62 units, 885 functions; INTRO.EXE 33 units, 250 functions |
| P2 Oracle and data | done: `make data` decodes every pinned file (formats 1 and 4 agree with the game's routines, run in the oracle; DAT, fonts, item tables, saves); 7 golden sessions (`make golden CHECK=1`), `EOB.EXE` and `INTRO.EXE`, independent of the checkout |
| P3 Match harness | done: `make match` (C, assembly, overlays, jump tables, partial units), `make match-gate` (11 negative fixtures), the agent process (claim, packet, skeleton, hand-off, integrate through `make check`, review, worker; playbooks 30, 70, 90); 16 functions in 8 units at L2, 2 drafts. Next: P4, `make worker` on `work/queue.toml` (60 units); INTRO.EXE's own 12 units need the matcher to handle that program |

## Environment

macOS arm64, Python 3.12.11, uv 0.11.0, capstone 5.0.7, unicorn 2.1.4, pytest 9.2.0, DOSBox-X 2026.10.01, emu2 9d8698d, Turbo C++ 1.00 and 1.01, TASM 2.0 and 2.01 under emu2. Not installed: Java 21, Ghidra (needed in P1).
