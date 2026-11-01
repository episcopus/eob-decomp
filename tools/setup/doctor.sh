#!/usr/bin/env bash
# doctor.sh: report whether this machine can run each phase of the project.
# Prints one line per check: ok / missing / skip (optional), then a summary.
# Exit code 1 if any REQUIRED check for the current phases is missing.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
REQ_MISSING=0
line() { printf "%-8s %-28s %s\n" "$1" "$2" "$3"; }
req()  { if eval "$2" >/dev/null 2>&1; then line ok "$1" "$(eval "$3" 2>/dev/null | head -1)"; else line missing "$1" "$4"; REQ_MISSING=1; fi; }
opt()  { if eval "$2" >/dev/null 2>&1; then line ok "$1" "$(eval "$3" 2>/dev/null | head -1)"; else line skip "$1" "$4"; fi; }

echo "== host"
line info "system" "$(uname -sm)"
PY=python3; [ -x .venv/bin/python ] && PY=.venv/bin/python

echo "== P0: analysis"
req "git"            "command -v git"     "git --version"                       "install Xcode command line tools"
req "python >= 3.11" "$PY -c 'import sys; assert sys.version_info >= (3,11)'" "$PY --version" "uv python install 3.12"
req "uv"             "command -v uv"      "uv --version"                        "brew install uv"
req "python venv"    "test -x .venv/bin/python" "echo .venv"                    "uv sync (creates .venv)"
req "capstone (py)"  "$PY -c 'import capstone'" "$PY -c 'import capstone; print(\"capstone\", capstone.__version__)'" "uv sync"
req "unicorn (py)"   "$PY -c 'import unicorn'" "$PY -c 'import unicorn; print(\"unicorn\", unicorn.__version__)'" "uv sync"
req "pytest (py)"    "$PY -c 'import pytest'" "$PY -c 'import pytest; print(\"pytest\", pytest.__version__)'" "uv sync"

echo "== P1: map (optional until then)"
opt "java 21+"       "java -version 2>&1 | grep -E 'version \"(2[1-9]|[3-9][0-9])'" "java -version 2>&1" "brew install openjdk@21 and set JAVA_HOME"
opt "ghidra"         "test -x \"\${GHIDRA_INSTALL_DIR:-/nonexistent}/support/analyzeHeadless\"" "echo \$GHIDRA_INSTALL_DIR" "install Ghidra and export GHIDRA_INSTALL_DIR"
opt "dosbox-x"       "command -v dosbox-x" "dosbox-x --version 2>&1 | grep -i -m1 dosbox" "brew install dosbox-x (reference runs of the game)"

echo "== Turbo C++ toolchain (docs/setup/SETUP-tc.md)"
opt "emu2"           "test -x tools/vendor/emu2/emu2" "echo emu2 \$(git -C tools/vendor/emu2 rev-parse --short HEAD 2>/dev/null)" "make emu2"
opt "toolchain archives" "ls build/toolchain/dist/* >/dev/null 2>&1" "echo \$(ls build/toolchain/dist | wc -l | tr -d ' ') files in build/toolchain/dist" "put the Turbo C++ archives in build/toolchain/dist/ (docs/setup/SETUP-tc.md)"
opt "toolchain installed" "$PY tools/re/tc_toolchain.py check" "$PY tools/re/tc_toolchain.py check | tail -1" "make tc-install (docs/setup/SETUP-tc.md)"

echo "== inputs"
TMP=$(mktemp)
if $PY tools/setup/inputs.py --check >"$TMP" 2>&1; then line ok "pinned inputs" "$(tail -1 "$TMP")"; else line missing "pinned inputs" "$(tail -1 "$TMP")"; REQ_MISSING=1; fi
rm -f "$TMP"

echo
if [ $REQ_MISSING -eq 0 ]; then echo "doctor: required checks pass"; else echo "doctor: required checks missing (see 'missing' lines)"; fi
exit $REQ_MISSING
