# Eye of the Beholder decompilation: every command humans and agents use.
# Targets of later phases (docs/spec/SPEC.md section 10) are added when that phase delivers them.

PY ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
PROGRAMS := eob/EOB.EXE eob/INTRO.EXE eob/START1.EXE

.PHONY: help setup doctor inputs overlays pak data emu2 test p0-check tc-install tc-check rtl-compare map match oracle-run golden test-slow

help:
	@echo "Phase 0:  setup doctor inputs overlays pak emu2 test p0-check"
	@echo "Toolchain: tc-install tc-check rtl-compare (docs/setup/SETUP-tc.md)"
	@echo "Phase 1:  map match [FILE=src/E0CF7.C] [WRITE=1]"
	@echo "Phase 2:  data oracle-run [INSNS=30000000] [ARGS='3 4 0 homie 3'] [SCRIPT=tests/sessions/x.keys]"
	@echo "          golden [NAME=x] [CHECK=1] test-slow"
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

data: pak         ## decode every data file of the PAKs (LCW, mazes, tables, palettes; whitepaper 4)
	$(PY) tools/re/wwfmt.py verify eob/

emu2:             ## build the headless DOS emulator that runs the Turbo C++ tools
	sh tools/setup/build-emu2.sh

tc-install:       ## unpack the Turbo C++ and TASM archives from build/toolchain/dist/ (FORCE=1 to redo)
	$(PY) tools/re/tc_toolchain.py install $(if $(FORCE),--force)

tc-check:         ## verify the installed toolchain files against tools/re/tc_toolchain.toml
	$(PY) tools/re/tc_toolchain.py check

rtl-compare:      ## which Turbo C++ release's runtime is in EOB.EXE (several minutes; build/re/rtl_compare.json)
	$(PY) tools/re/rtl_compare.py

map: overlays     ## enumerate functions, write re/units.toml and re/symbols.toml (playbook 10)
	$(PY) tools/re/functions.py eob/EOB.EXE build/re/eob.json -o build/re/eob-functions.json
	$(PY) tools/re/functions.py eob/INTRO.EXE build/re/intro.json -o build/re/intro-functions.json
	$(PY) tools/re/map_units.py --write

match:            ## prove C and assembly units against EOB.EXE (FILE=src/X.C, default every src/*.C and *.ASM; WRITE=1 records L2)
	@rc=0; for f in $(or $(FILE),$(wildcard src/*.C src/*.ASM)); do echo "== $$f"; \
	  $(PY) tools/match/cmatch.py $$f $(if $(WRITE),--write) || rc=1; done; exit $$rc

oracle-run:       ## run the original EOB.EXE in the oracle (build/oracle/run: summary.json, shots/)
	$(PY) tools/oracle/run.py --max-insns $(or $(INSNS),30000000) $(if $(ARGS),--args "$(ARGS)") $(if $(SCRIPT),--script $(SCRIPT))

golden:           ## record the golden sessions (NAME=x for one; CHECK=1 verifies instead)
	$(PY) tools/oracle/golden.py $(if $(CHECK),--check) $(NAME)

test:             ## Python tests (the fast set)
	$(PY) -m pytest -q -m "not slow" tests/unit

test-slow:        ## the tests that run the game: every golden session (about half a minute)
	$(PY) -m pytest -q -m slow tests/unit

p0-check: inputs overlays pak test  ## Phase 0 exit criteria (SPEC section 10)
	@echo "p0-check: OK"
