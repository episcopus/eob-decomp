# Project status

2026-10-31.

| Phase | State |
|---|---|
| P0 Bootstrap | done: `make doctor`, `make p0-check` (16 files pinned; 99 + 54 segments; 7 + 1 overlays; 6 PAKs, 193 files) |
| P1 Map and toolchain | done: Turbo C++ 1.00, `-ml -O -Z` (some units `-G`), TLINK 3.0, TASM 2.0 [H]; `make map`: EOB.EXE 62 units, 885 functions; INTRO.EXE 33 units, 250 functions |
| P2 Oracle and data | started: `make data` decodes 162 of 182 files (one anomaly); the oracle runs EOB.EXE with keyboard, mouse and AdLib; 5 golden sessions verified (`make golden CHECK=1`, `make test-slow`), up to walking in the dungeon of a saved game. Next: sessions that cover more of the game, INTRO.EXE, sound effects compared |
| P3 Match harness | `make match`: C, assembly, overlays, jump tables, partial units; 16 functions in 8 units at L2, 2 drafts. Next: the agent process from poolrad |

## Environment

macOS arm64, Python 3.12.11, uv 0.11.0, capstone 5.0.7, unicorn 2.1.4, pytest 9.2.0, DOSBox-X 2026.10.01, emu2 9d8698d, Turbo C++ 1.00 and 1.01, TASM 2.0 and 2.01 under emu2. Not installed: Java 21, Ghidra (needed in P1).
