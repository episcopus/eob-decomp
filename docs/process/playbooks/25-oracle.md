# 25 The oracle

`tools/oracle/` is poolrad-decomp's Unicorn PC (virtual time, IVT dispatch so the program's own handlers run, DOS and BIOS services, a write layer over the pinned files), with what Eye of the Beholder needs added:

| Module | What |
|---|---|
| `ega.py` | VGA mode 13h and the DAC, besides poolrad's EGA |
| `dos.py` | DOS load-overlay 4B03h (the graphics drivers) |
| `machine.py` | IRQ1 and port 60h for the keyboard; EOI clears the highest request in service; a port handler table for devices |
| `input.py` | scripts of keys and mouse moves and clicks, delivered as hardware does at fixed virtual times; the INT 33h driver and its event trampoline |
| `adlib.py` | the OPL2: registers, timers, status; every register write logged |
| `run.py` | one run: command line, devices, checkpoints, summary |
| `golden.py` | record and verify golden sessions |

## Running

```sh
make oracle-run                                         # to the title menu: build/oracle/run/
make oracle-run SCRIPT=tests/sessions/load-walk.keys INSNS=90000000
.venv/bin/python tools/oracle/run.py --script x.keys --mouse --sound adlib --save tests/sessions/fixtures/EOBDATA.SAV
```

The command line is the game's configuration (whitepaper 2.6). A run stops with exit code 3 when the program uses a service the oracle does not model (`UNIMPLEMENTED ...`): add the service, never answer it silently.

## Sessions and goldens

A session is `tests/sessions/<name>.keys` (the format is in `tools/oracle/input.py`: `wait`, `key`, `text`, `mouse`, `click`, `shot`, `digest`, `quit`, and `option mouse | sound adlib | insns N | save FILE`). Timing is in ticks of virtual time (1/18.2 s); the title menu is up after about 550 ticks. The game buffers keys, so a key sent early is not lost.

- `make golden NAME=x` runs the session twice and writes `tests/sessions/golden/x.json` (instructions, state digest, image hash of every checkpoint and of the end, sound digest) and the screenshots beside it, only if both runs agree.
- `make golden CHECK=1` verifies every session; `make test-slow` does the same from pytest.
- A golden changes only when the oracle changes what the program can observe (a new device, a fix): record again and say why in the commit.

Sessions so far: `boot-title`, `boot-adlib`, `new-party`, `mouse-party`, `load-walk` (see `tests/sessions/README.md`).

## Not modelled yet

Sound Blaster digitised sound, the PC speaker's and Tandy chip's audio (logged, not checked by ear), INTRO.EXE and START1.EXE, a mouse cursor drawn by the driver (the game draws its own).
