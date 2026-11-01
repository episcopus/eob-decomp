# Project status

2026-10-31.

| Phase | State |
|---|---|
| P0 Bootstrap | done: `make doctor`, `make p0-check` (16 files pinned; 99 + 54 segments; 7 + 1 overlays; 6 PAKs, 193 files) |
| P1 Map and toolchain | waiting for the Turbo C++ archives in `build/toolchain/dist/` (docs/setup/SETUP-tc.md); mapping can start without them |

## Environment

macOS arm64, Python 3.12.11, uv 0.11.0, capstone 5.0.7, unicorn 2.1.4, pytest 9.2.0, DOSBox-X 2026.10.01, emu2 9d8698d. Not installed: Java 21, Ghidra (needed in P1).
