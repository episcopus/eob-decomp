# Project status

2026-10-31.

| Phase | State |
|---|---|
| P0 Bootstrap | done: `make doctor`, `make p0-check` (16 files pinned; 99 + 54 segments; 7 + 1 overlays; 6 PAKs, 193 files) |
| P1 Map and toolchain | toolchain pinned: Turbo C++ 1.00, large model, TLINK 3.0 (`make tc-check`, `make rtl-compare`); assembler provisionally TASM 2.0. Next: switches, `re/units.toml`, function enumeration, the go/no-go spike |

## Environment

macOS arm64, Python 3.12.11, uv 0.11.0, capstone 5.0.7, unicorn 2.1.4, pytest 9.2.0, DOSBox-X 2026.10.01, emu2 9d8698d, Turbo C++ 1.00 and 1.01, TASM 2.0 and 2.01 under emu2. Not installed: Java 21, Ghidra (needed in P1).
