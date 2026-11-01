# The reference toolchain: Borland Turbo C++ 1.00

The game was built with **Turbo C++ 1.00** and its TLINK 3.0 (whitepaper 2.1, SPEC section 2). From Phase 1 on the compiler is on the critical path: it proves every function and rebuilds the game. It is proprietary: it lives in `build/toolchain/` (ignored by git) and is never committed or copied into `src/`.

## The archives

Put these in `build/toolchain/dist/` with these names (WinWorld disk sets; hashes in `tools/re/tc_toolchain.toml`). Agents do not download or sign in for these; the human does.

| Release | Archive | Role |
|---|---|---|
| `tc100` | `Borland Turbo CPP 1.00 (5.25-360k).7z` | **Pinned compiler** |
| `tc101` | `Borland Turbo C++ 1.01 (3.5).7z` | Eliminated; kept for comparisons |
| `tasm20` | `Borland Turbo Assembler 2.0 (5.25-360k).7z` | Provisional assembler |
| `tasm201` | `Borland Turbo Assembler 2.01 (5.25).7z` | Kept for the assembler comparison |

## Install

```sh
make emu2          # headless DOS emulator (same commit as poolrad-decomp)
make tc-install    # 7z -> FAT12 disk images -> PKZIP archives -> build/toolchain/<release>/
make tc-check      # installed files against their pinned hashes
```

Each release is laid out as `BIN/` (TCC, TLINK, TLIB, MAKE, TASM), `LIB/` (`C0x.OBJ`, `Cx.LIB`, `MATHx.LIB`, `EMU.LIB`, `OVERLAY.LIB`), `INCLUDE/`, `STARTUP/` (`C0.ASM`, the startup source). Only the command-line tools are installed; the IDE (`TC.CA1`, split archives) is not needed.

## Running a tool

```sh
.venv/bin/python tools/re/tc_toolchain.py run tc100 TCC -ml -c -ID:\\INCLUDE FILE.C
```

Drive `C:` is the current directory, drive `D:` the release (`D:\INCLUDE`, `D:\LIB`). Output file names come back in lower case on macOS.

## Re-proving the pin

`make rtl-compare` (several minutes) links every module of each release's large-model library and reports how many are found in `EOB.EXE`; `build/re/rtl_compare.json` has the addresses.
