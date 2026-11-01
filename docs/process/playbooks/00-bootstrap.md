# 00 Bootstrap

**Inputs:** a clone on macOS (Apple Silicon) or Linux, with `eob/` in place.

## Steps

1. `make setup` (needs `uv`): creates `.venv` with the Python tools.
2. `make doctor`. Every line must be `ok` or an optional `skip`.
3. `make inputs`. All hashes in `eob/` must match `re/inputs.toml`; if not, stop: the spec targets exactly these bytes (AGENTS.md rule 6).
4. `make overlays`. Expect `EOB.EXE: 99 segments (58 code, 22 data, 12 kind4, 7 overlay-stub), 7 overlays, 125 entries, 50641 overlay code bytes; OK` and `INTRO.EXE: 54 segments ..., 1 overlays, 22 entries, 11741 overlay code bytes; OK`.
5. `make pak`. Expect `pak: 6 archives, 193 files, OK`.
6. `make emu2` (optional until Phase 1). The compiler itself: `docs/setup/SETUP-tc.md`.

## Done when

`make doctor` and `make p0-check` pass.
