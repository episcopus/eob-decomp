# Sessions

Scripts of input for the oracle (playbook 25, format in `tools/oracle/input.py`), and their goldens in `golden/` (recorded by `make golden`, verified by `make golden CHECK=1` and `make test-slow`).

| Session | Covers |
|---|---|
| `boot-title.keys` | start-up to the title menu (VGA, no sound, no mouse); the keyboard moves the menu highlight |
| `boot-adlib.keys` | the same with AdLib: the OPL2 set up silent (21 register writes) |
| `new-party.keys` | "Start a new party" from the keyboard: Character Generation |
| `mouse-party.keys` | the same with the mouse, then the first character box: the race menu |
| `load-walk.keys` | "Load game in progress" with `fixtures/EOBDATA.SAV`, moves in the dungeon, AdLib on (about 10,000 register writes) |
| `intro-play.keys` | `INTRO.EXE` as `START1.EXE` runs it: the whole intro (147 s), then the hand-over to `EOB.EXE` and its title menu |
| `intro-skip.keys` | `INTRO.EXE`, ESC: the hand-over at once, then "Load game in progress": the chain after `START1.EXE` in one run |

`fixtures/EOBDATA.SAV` is a copy of the saved game in `eob/` (which the game rewrites when you save, so it is not pinned): four characters on level 1.
