# Project status

2026-10-31.

| Phase | State |
|---|---|
| P0 Bootstrap | done: `make doctor`, `make p0-check` (16 files pinned; 99 + 54 segments; 7 + 1 overlays; 6 PAKs, 193 files) |
| P1 Map and toolchain | toolchain pinned (Turbo C++ 1.00, `-ml -O -Z`, TLINK 3.0; TASM 2.0 [H]); map written (`make map`: EOB.EXE 62 units, 885 functions; INTRO.EXE 33 units, 250 functions, 21 shared); **go/no-go passed**: 8 functions in 2 whole units at L2 (`make match`). Next: the rest of the matcher (assembly units, jump tables, overlay code, shifted-data builds), then Phase 2 |

## Environment

macOS arm64, Python 3.12.11, uv 0.11.0, capstone 5.0.7, unicorn 2.1.4, pytest 9.2.0, DOSBox-X 2026.10.01, emu2 9d8698d, Turbo C++ 1.00 and 1.01, TASM 2.0 and 2.01 under emu2. Not installed: Java 21, Ghidra (needed in P1).
