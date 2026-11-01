# Eye of the Beholder decompilation: every command humans and agents use.
# Targets of later phases (docs/spec/SPEC.md section 10) are added when that phase delivers them.

PY ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
PROGRAMS := eob/EOB.EXE eob/INTRO.EXE eob/START1.EXE

.PHONY: help setup doctor inputs overlays pak emu2 test p0-check

help:
	@echo "Phase 0:  setup doctor inputs overlays pak emu2 test p0-check"
	@echo "See AGENTS.md and docs/spec/SPEC.md."

setup:            ## create .venv with the Python tools (uv)
	uv sync

doctor:           ## check the environment and pinned inputs
	bash tools/setup/doctor.sh

inputs:           ## verify hashes of eob/ against re/inputs.toml
	$(PY) tools/setup/inputs.py --check

overlays:         ## segment table and FBOV overlay manifest of each program (build/re/)
	@for f in $(PROGRAMS); do \
	  n=$$(basename $$f .EXE | tr A-Z a-z); \
	  $(PY) tools/re/fbov.py $$f -o build/re/$$n.json --extract build/re/$$n-overlays || exit 1; \
	done

pak:              ## verify every PAK archive (directory, tiling, byte-exact round trip)
	$(PY) tools/re/pak.py verify eob/

emu2:             ## build the headless DOS emulator that runs the Turbo C++ tools
	sh tools/setup/build-emu2.sh

test:             ## Python tests
	$(PY) -m pytest -q tests/unit

p0-check: inputs overlays pak test  ## Phase 0 exit criteria (SPEC section 10)
	@echo "p0-check: OK"
