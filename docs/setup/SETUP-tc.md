# The reference toolchain: Borland Turbo C++

The game was built with Turbo C++ (1990 banner) and TLINK 3.0 (whitepaper 2.1). From Phase 1 on the compiler is on the critical path: it proves every function and rebuilds the game. It is proprietary: it lives in `build/toolchain/` (ignored by git) and is never committed or copied into `src/`.

## What to get (a human step)

| Archive | Why | Where |
|---|---|---|
| Turbo C++ 1.01 | Candidate release; free from Embarcadero | Embarcadero "Antique Software" (needs a free account): https://cc.embarcadero.com/Item/26014 |
| Turbo C++ 1.0 | The other candidate; the runtime comparison decides | Original disks or a software archive |
| Turbo Assembler (TASM 2.x) | Hand-written modules are matched as assembly | Came with Turbo C++ Professional / Borland C++ 2.0 |

Put the archives, as downloaded, in `build/toolchain/dist/`. `make doctor` reports them. Agents do not download or sign in for these; the human does.

## Then (Phase 1)

1. `make emu2`: the headless DOS emulator (same commit as poolrad-decomp) that runs `TCC.EXE`, `TLINK.EXE` and `TASM.EXE`.
2. Install each release under `build/toolchain/<release>/` and record the hashes of `TCC.EXE`, `TLINK.EXE`, the libraries and `C0*.OBJ` in `tools/re/tc_toolchain.toml` (as poolrad's `tp_toolchain.toml`).
3. Compare the runtime: link an empty program per memory model and compare the startup and library modules with `EOB.EXE` segment `0000` and the small runtime segments. The release and model whose bytes match are pinned in SPEC section 2.
