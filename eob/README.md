# eob/: your copy of the game

Put the files of your own copy of Eye of the Beholder for DOS, with the unofficial fan patch "1.9" applied, here, at the top of this folder: `START1.EXE`, `INTRO.EXE`, `EOB.EXE`, the graphics drivers `MCGA.OVL`, `EGA.OVL`, `CGA.OVL` and `TGA.OVL`, the archives `EOBDATA1.PAK` to `EOBDATA6.PAK`, the fonts `FONT6.FNT` and `FONT8.FNT`, and the patch's `version.nfo`. Nothing in this folder except this note is tracked by git (`.gitignore`), and none of it may be committed.

`make inputs` compares each file with the size and SHA-256 recorded in `re/inputs.toml`. The project targets one version; if a file differs, the matcher and the rebuild will not agree with it.

The golden sessions and play tests that start from a saved game (`tests/sessions/`, `tests/playtest/`) read `tests/sessions/fixtures/EOBDATA.SAV`. The saved game they were recorded from is the author's and is not published, so with a save of your own those sessions run but do not reproduce the recorded checkpoints. The game writes `LEVELS.TMP` and `EOBDATA.SAV` while it runs: never run it inside this folder; the tools copy the files first.
