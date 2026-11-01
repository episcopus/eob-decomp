# 25 The oracle

`tools/oracle/` is poolrad-decomp's Unicorn PC (virtual time, IVT dispatch so the program's own handlers run, DOS and BIOS services, a write layer over the pinned files), with what EOB needs added: VGA mode 13h and the DAC (`ega.py`), DOS load-overlay 4B03h for the graphics drivers (`dos.py`), INT 33h without a mouse (`run.py`).

```sh
make oracle-run                          # EOB.EXE to its title menu: build/oracle/run/summary.json, shots/final.png
make oracle-run INSNS=60000000 ARGS="3 4 0 homie 3"
.venv/bin/python tools/oracle/run.py --shot-every 1000000 --out build/oracle/x
```

The command line is the game's configuration (whitepaper 2.6). A run stops with exit code 3 when the program uses a service the oracle does not model (`UNIMPLEMENTED ...`): add the service, never answer it silently. Two runs of the same input must end in the same state digest (`tests/unit/test_oracle.py`).

Still to come (SPEC phase 2): key scripts and goldens (poolrad's `keys.py`, `golden.py`, `replay.py`), AdLib and the timer's sound use, the mouse, INTRO.EXE and START1.EXE.
