# 25 The oracle

`tools/oracle/` is poolrad-decomp's Unicorn PC (virtual time, IVT dispatch so the program's own handlers run, DOS and BIOS services, a write layer over the pinned files), with what Eye of the Beholder needs added:

| Module | What |
|---|---|
| `ega.py` | VGA mode 13h and the DAC, besides poolrad's EGA |
| `dos.py` | DOS load-overlay 4B03h (the graphics drivers, and `INTRO.EXE` loading `EOB.EXE`), 29h (parse a file name into an FCB); exec 4B00h ends the run with the child's command line |
| `machine.py` | IRQ1 and port 60h for the keyboard; EOI clears the highest request in service; a port handler table for devices |
| `input.py` | scripts of keys and mouse moves and clicks, delivered as hardware does at fixed virtual times; the INT 33h driver and its event trampoline |
| `adlib.py` | the OPL2: registers, timers, status; every register write logged |
| `run.py` | one run: program, command line, devices, checkpoints, summary; only the pinned files of `eob/` are visible (`hide_unpinned`), a session's save comes with its `LEVELS.TMP` (`use_save`); `--rebuilt` runs the programs `make rebuild` linked |
| `layout.py` | which files a run loads (the originals, or the rebuilds served under their names), and the rebuild's memory read in the original's form for the state digest (playbook 50) |
| `call.py` | `call_far(m, seg, off, args)`: run one routine of the original and return (decoders checked against the game's own unpack routines, `tests/unit/test_wwfmt.py`; later, lifted routines) |
| `golden.py` | record and verify golden sessions |

## Running

```sh
make oracle-run                                         # to the title menu: build/oracle/run/
.venv/bin/python tools/oracle/run.py --program INTRO.EXE  # the intro, then EOB.EXE (args "3 4 0 homie")
make oracle-run SCRIPT=tests/sessions/load-walk.keys INSNS=90000000
.venv/bin/python tools/oracle/run.py --script x.keys --mouse --sound adlib --save tests/sessions/fixtures/EOBDATA.SAV
```

The command line is the game's configuration (whitepaper 2.6). A run stops with exit code 3 when the program uses a service the oracle does not model (`UNIMPLEMENTED ...`): add the service, never answer it silently.

## Sessions and goldens

A session is `tests/sessions/<name>.keys` (the format is in `tools/oracle/input.py`: `wait`, `key`, `text`, `mouse`, `click`, `shot`, `digest`, `quit`, and `option mouse | sound adlib | insns N | save FILE | program INTRO.EXE`). Timing is in ticks of virtual time (1/18.2 s); the title menu is up after about 550 ticks. The game buffers keys, so a key sent early is not lost.

- `make golden NAME=x` runs the session twice and writes `tests/sessions/golden/x.json` (instructions, state digest, image hash of every checkpoint and of the end, sound digest) and the screenshots beside it, only if both runs agree.
- `make golden CHECK=1` verifies every session; `make test-slow` does the same from pytest.
- `make golden CHECK=1 REBUILT=1` replays every golden on the rebuilt `EOB.EXE` and `INTRO.EXE` (`make rebuild` both first); a session that differs is played again on both and the first state that differs is printed, with the memory that differs.
- A golden changes only when the oracle changes what the program can observe (a new device, a fix): record again and say why in the commit.

Sessions so far: `boot-title`, `boot-adlib`, `new-party`, `mouse-party`, `load-walk`, `intro-play`, `intro-skip` (see `tests/sessions/README.md`).

A golden must not depend on the checkout: the files the game writes (`EOBDATA.SAV`, `LEVELS.TMP`) are hidden from every run, and a session that needs a saved game names a fixture (`option save fixtures/EOBDATA.SAV`). `make integrate` checks the goldens in a fresh worktree, which is how a dependency on an unpinned `eob/LEVELS.TMP` was found.

## Not modelled yet

Sound Blaster digitised sound, the PC speaker's and Tandy chip's audio (logged, not checked by ear), START1.EXE (a text-mode menu that spawns INTRO.EXE), a mouse cursor drawn by the driver (the game draws its own).
