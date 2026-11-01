#!/bin/sh
# Build emu2 (https://github.com/dmsc/emu2, GPL-2.0), a headless DOS emulator that runs
# the Turbo C++ command-line tools (TCC, TLINK, TASM) without a window. Used by
# the compiler probes from Phase 1 on (docs/setup/SETUP-tc.md). Installs to tools/vendor/emu2/ (gitignored).
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
DEST="$ROOT/tools/vendor/emu2"
REPO=https://github.com/dmsc/emu2
COMMIT=9d8698d06e67359bc8b230c2d19d5b1503e60819   # 2026-04-26, same commit as poolrad-decomp (verified there with TP 6.0 and 7.0)

if [ ! -d "$DEST/.git" ]; then
  mkdir -p "$ROOT/tools/vendor"
  git clone --quiet "$REPO" "$DEST"
fi
git -C "$DEST" fetch --quiet origin "$COMMIT" 2>/dev/null || true
git -C "$DEST" checkout --quiet "$COMMIT"
make -C "$DEST" >/dev/null
echo "emu2 ready: $DEST/emu2 ($(git -C "$DEST" rev-parse --short HEAD))"
