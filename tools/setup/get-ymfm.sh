#!/bin/sh
# Fetch ymfm (https://github.com/aaronsgiles/ymfm, BSD 3-clause), the OPL2 emulator of the port's
# sound (playbook 80, decision 7). Source only, at one commit: port/port.mk compiles the files it
# needs into the program, so a player needs nothing installed. Goes to tools/vendor/ymfm/ (gitignored).
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
DEST="$ROOT/tools/vendor/ymfm"
REPO=https://github.com/aaronsgiles/ymfm
COMMIT=81aec25ccbb98f4873a255f7551ac4dadac59b4a

if [ ! -d "$DEST/.git" ]; then
  mkdir -p "$ROOT/tools/vendor"
  git clone --quiet "$REPO" "$DEST"
fi
git -C "$DEST" fetch --quiet origin "$COMMIT" 2>/dev/null || true
git -C "$DEST" checkout --quiet "$COMMIT"
echo "ymfm ready: $DEST ($(git -C "$DEST" rev-parse --short HEAD))"
