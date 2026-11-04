# Eye of the Beholder decompilation: every command humans and agents use.
# Targets of later phases (docs/spec/SPEC.md section 10) are added when that phase delivers them.

PY ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
PROGRAMS := eob/EOB.EXE eob/INTRO.EXE eob/START1.EXE

.PHONY: help setup doctor inputs overlays pak data emu2 test p0-check tc-install tc-check rtl-compare map match oracle-run golden test-slow \
	match-gate check queue next claim locks unlock packet skeleton handoff handoffs handoff-done integrate review-pass worker progress \
	start-run stop-run run-status run-add watch-agents smoke-agent readability readability-gate rename names field

help:
	@echo "Phase 0:  setup doctor inputs overlays pak emu2 test p0-check"
	@echo "Toolchain: tc-install tc-check rtl-compare (docs/setup/SETUP-tc.md)"
	@echo "Phase 1:  map match [FILE=src/E0CF7.C] [WRITE=1]"
	@echo "Phase 2:  data oracle-run [INSNS=30000000] [ARGS='3 4 0 homie 3'] [SCRIPT=tests/sessions/x.keys]"
	@echo "          golden [NAME=x] [CHECK=1] test-slow"
	@echo "Phase 3:  match-gate check progress queue next packet skeleton rename readability (playbook 30)"
	@echo "          claim locks unlock handoff handoffs integrate handoff-done review-pass worker (playbooks 70, 90)"
	@echo "          smoke-agent start-run watch-agents run-status run-add stop-run (work/roster.toml)"
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

data: pak         ## decode every data file: PAKs, fonts, the test save (whitepaper 4)
	$(PY) tools/re/wwfmt.py verify eob/
	$(PY) tools/re/eobfmt.py verify eob/
	$(PY) tools/re/eobfmt.py save tests/sessions/fixtures/EOBDATA.SAV

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

match:            ## prove C and assembly units against their program (FILE=src/X.C, default every src/*.C and *.ASM; WRITE=1 records L2)
	@rc=0; for f in $(or $(FILE),$(wildcard src/*.C src/*.ASM)); do echo "== $$f"; \
	  $(PY) tools/match/cmatch.py $$f $(if $(WRITE),--write) || rc=1; done; exit $$rc

rebuild:          ## build a program from src/ and compare it with the original (PROGRAM=START1.EXE; playbook 50)
	$(PY) tools/rebuild/rebuild.py $(or $(PROGRAM),START1.EXE) $(if $(V),-v)

twin:             ## prove INTRO.EXE units by their EOB.EXE twin, the same module linked elsewhere (UNIT=, WRITE=1 records L2; FIND=1 looks for twins)
	$(PY) tools/match/twin.py $(if $(FIND),--find) $(if $(UNIT),--unit $(UNIT)) $(if $(WRITE),--write)

oracle-run:       ## run the original EOB.EXE in the oracle (build/oracle/run: summary.json, shots/)
	$(PY) tools/oracle/run.py --max-insns $(or $(INSNS),30000000) $(if $(ARGS),--args "$(ARGS)") $(if $(SCRIPT),--script $(SCRIPT))

golden:           ## record the golden sessions (NAME=x for one; CHECK=1 verifies instead; CHECK=1 REBUILT=1 on the programs make rebuild linked)
	$(PY) tools/oracle/golden.py $(if $(CHECK),--check) $(if $(REBUILT),--rebuilt) $(NAME)

test:             ## Python tests (the fast set)
	$(PY) -m pytest -q -m "not slow" tests/unit

test-slow:        ## the tests that run the game: every golden session, on the originals and on the rebuilds (about three minutes)
	$(PY) -m pytest -q -m slow tests/unit

match-gate:       ## every src/ unit matches, every L2 function is still proven, every negative fixture is rejected
	$(PY) tools/match/gate.py -v

check: inputs test match-gate readability-gate test-slow  ## everything a merge must pass (make integrate runs it)
	@echo "check: OK"

readability:      ## the checkable naming and comment rules (docs/process/READABILITY.md): [UNIT=<id>] [BASELINE=1]
	$(PY) tools/match/readability.py $(UNIT) $(if $(BASELINE),--baseline)

readability-gate: ## fail on readability findings not in work/readability-baseline.txt (part of make check)
	$(PY) tools/match/readability.py --gate

names:            ## what is still named by its address, by unit (Phase 6, playbook 65): [UNIT=<id>] [SHARED=1]
	$(PY) tools/match/names.py $(if $(UNIT),--unit $(UNIT)) $(if $(SHARED),--shared)

field:            ## give a struct field one name in every file that declares the struct: STRUCT=<Name> AT=<hex offset> NEW=<name> WHY="<evidence>"
	$(PY) tools/match/field.py --struct "$(STRUCT)" --at "$(AT)" --new "$(NEW)" --why "$(WHY)"

rename:           ## give a function or global its real name everywhere: OLD=<name or address> NEW=<Name> WHY="<evidence>"; APPLY=1 re-applies the recorded renames
	$(PY) tools/match/rename.py $(if $(APPLY),--apply)

progress:         ## functions at L2, complete and reviewed units, claims and hand-offs
	$(PY) tools/agent/progress.py

queue:            ## the claim order from re/units.toml (WRITE=1 writes work/queue.toml)
	$(PY) tools/agent/next.py --queue

next:             ## the next unit to claim (REVIEW=1: the next unit to review)
	$(PY) tools/agent/next.py

claim:            ## start work: AGENT=<name> [UNIT=<id>] [REVIEW=1] [TASK=<name>]; prints the worktree
	$(PY) tools/agent/claim.py

locks:            ## the claims
	$(PY) tools/agent/claim.py --list

unlock:           ## give a claim back: UNIT=<key> AGENT=<owner> [FORCE=1 REASON="<the human's instruction>"]
	$(PY) tools/agent/claim.py --unlock

packet:           ## build/packets/<id>.md, everything to start a unit: UNIT=<id>
	$(PY) tools/agent/packet.py

skeleton:         ## src/<ID>.C (or .ASM) with every function a stub: UNIT=<id>
	$(PY) tools/agent/skeleton.py

handoff:          ## from your worktree: AGENT= UNIT= STATE=complete|partial|blocked|review-passed|review-failed|escalate NOTE="..."
	$(PY) tools/agent/handoff.py

handoffs:         ## what waits: escalations first, then branches
	$(PY) tools/agent/handoff.py --list

handoff-done:     ## the human settles an escalation: KEY=<key> DECISION="..."
	$(PY) tools/agent/integrate.py --done

integrate:        ## merge every waiting hand-off into main through make check: [KEY=<key>] [DRY=1]
	$(PY) tools/agent/integrate.py

review-pass:      ## record a passed review: UNIT=<id> AGENT=<reviewer> [CHECK=1]
	$(PY) tools/agent/review.py

worker:           ## one supervisor by hand: AGENT= [ROLE=] [MODEL= VARIANT= TIER=] [UNIT=] [SESSIONS=] [TIMEOUT=] [COST_CAP=] [DRY=1]
	$(PY) tools/agent/worker.py

start-run:        ## check, then start every entry of work/roster.toml in the background (DRY=1: show and check only)
	$(PY) tools/agent/run.py start $(if $(DRY),--dry)

stop-run:         ## drain the run: workers finish (WAIT minutes, default 30), the integrator finishes, a last integrate
	$(PY) tools/agent/run.py stop $(if $(WAIT),--wait $(WAIT))

run-status:       ## each roster entry: alive, its claim, the last line of its log; then make progress
	$(PY) tools/agent/run.py status

run-add:          ## start (or restart) one roster entry in the running run: NAME=<name>
	$(PY) tools/agent/run.py add $(NAME)

watch-agents:     ## live view of the run: agents, sessions and their cost, progress, hand-offs, stuck units (INTERVAL=5, ONCE=1)
	@$(PY) tools/agent/watch.py

smoke-agent:      ## one real session of a roster entry on one unit, in a scratch clone, merged through make check: [NAME=dec-deepseek] [UNIT=e19ec] [TIMEOUT=30] [WORKTREE=1]
	$(PY) tools/agent/smoke.py

p0-check: inputs overlays pak test  ## Phase 0 exit criteria (SPEC section 10)
	@echo "p0-check: OK"
