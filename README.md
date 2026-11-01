# eob-decomp

A matching decompilation of Westwood's Eye of the Beholder for DOS (1991, published by SSI) into C, done largely by AI agents: every one of the 1,011 functions of its three programs (`START1.EXE`, `INTRO.EXE`, `EOB.EXE`) is C or assembly that the original Borland Turbo C++ 1.00 toolchain turns back into the original's machine code, byte for byte, and `make match` proves it. From that source the project rebuilds the programs and the four graphics drivers (`make rebuild`), translates the game to portable C with an SDL3 host (`make port`), and patches the DOS build to run well on a Tandy 1000 EX (`ports/dos/`).

This repository is the public companion to a series of videos and articles about the project. It holds the source, the tools, the process the agents follow and what was learned about the game. It does not hold the game.

Learn more at [anonymousformula.com/eob](https://anonymousformula.com/eob/), where the articles and the story of the project are published. The sister project, on SSI's Pool of Radiance, is [poolrad-decomp](https://github.com/episcopus/poolrad-decomp).

| Start here | |
|---|---|
| `docs/design/WHITEPAPER.md` | How the game works: the three programs, the main loop and its clock, the dungeon and its scripts, the 3D view, combat, magic, sound and video |
| `docs/spec/SPEC.md` | The process specification: goals, approach, verification ladder, phases |
| `AGENTS.md` | The operating manual the AI agents work from |
| `src/` | The decompiled programs: one C or TASM file per original module, the graphics drivers in `src/drivers/` |
| `port/`, `tools/port/` | The portable build: the translator from `src/` to C, the runtime and the SDL3 host |
| `ports/dos/` | The DOS build for the Tandy 1000 EX: speed patches and new Tandy sound |
| `work/status.md` | Where the project stands |

## What you need to supply

Nothing copyrighted by others is in this repository. To build or verify anything you need:

1. Your own copy of Eye of the Beholder for DOS (it is still sold, for example on GOG), with the unofficial fan patch "1.9" applied: the project targets that one version. Put its files in `eob/` (see `eob/README.md`). `make inputs` checks them against the sizes and hashes in `re/inputs.toml`; other versions will not match.
2. Borland Turbo C++ 1.00 and Turbo Assembler 2.0, for matching and rebuilding. They are proprietary (now Embarcadero's) and never part of this repository. `docs/setup/SETUP-tc.md` says how the project pins them by hash and where to put them.

Then, on macOS:

```sh
make setup doctor inputs overlays
make emu2              # the headless DOS emulator the compiler runs in
make tc-install tc-check   # once the toolchain archives are in build/toolchain/dist/
make check             # tests, every unit's match, the golden sessions
```

`make port` and `make port-play` build and run the portable game (needs LLVM and SDL3; `docs/process/playbooks/80-port.md`), with your game files.

## Legal

This is an independent, non-commercial research and preservation project. It is not affiliated with or endorsed by Westwood, Strategic Simulations, Inc., Wizards of the Coast, Hasbro, Electronic Arts or Embarcadero. Eye of the Beholder, Advanced Dungeons & Dragons and Forgotten Realms are trademarks of their owners and are used here only to identify the game studied.

No game data, executables, artwork, or compiler files are distributed here, and no build of the game is published. See `LICENSE` for what the licence covers and what it does not.
