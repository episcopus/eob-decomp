# ports/dos/Makefile: Eye of the Beholder for the Tandy 1000 EX: the game rebuilt from src/ with this port's patches.
# Run from here or with `make -C ports/dos <target>`. src/ and the other tools are never touched: the build stages a
# copy under build/. Needs the project's setup (make setup, make tc-install, make emu2 at the repository root).

ROOT := ../..
PY := $(if $(wildcard $(ROOT)/.venv/bin/python),$(ROOT)/.venv/bin/python,python3)

.PHONY: help game ref func prof spans tandy test dist play clean
.DEFAULT_GOAL := help

help:
	@echo "ports/dos: the Tandy 1000 EX build (README.md)"
	@echo "  make game    build/game/EOB.EXE and TGA.OVL: the game, with patches/series"
	@echo "  make ref     build/ref: the same, no patches: must be the original program"
	@echo "  make func    the gate: the same game on ref and game, screen, pages, DGROUP, sound, file reads (SESSION=name)"
	@echo "  make prof    the 8088 model: what each action costs on the EX, ref against game (SESSION=name, BUS=4|5|6)"
	@echo "  make spans   the timed effects: every spell burst and intro dissolve, ref against game"
	@echo "  make tandy   DOSBox-X machine=tandy, headless: ref and game show the same screens (about 3 min)"
	@echo "  make test    game, ref, func and tandy"
	@echo "  make dist    build/dist: the files to copy to the machine, with INSTALL.TXT and checksums"
	@echo "  make play    DOSBox-X window on the Tandy machine, for a look (not a measurement)"

game:
	$(PY) tools/build.py

ref:
	$(PY) tools/build.py --name ref --none

func: game ref
	$(PY) tools/func.py $(if $(SESSION),--session $(SESSION))

prof: game ref
	EOB_BUS=$(or $(BUS),5) $(PY) tools/prof.py run --build ref --build game $(if $(SESSION),--session $(SESSION))
	EOB_BUS=$(or $(BUS),5) $(PY) tools/prof.py diff ref game $(if $(SESSION),--session $(SESSION))

spans:
	$(PY) tools/prof.py spans ref game $(if $(SESSION),--session $(SESSION),--session spell --session intro)

tandy: game ref
	$(PY) tools/dbx.py

test: game ref func tandy

dist: game
	rm -rf build/dist && mkdir -p build/dist
	cp build/game/EOB.EXE build/game/INTRO.EXE build/game/TGA.OVL build/dist/
	perl -pe 's/\r?\n/\r\n/' INSTALL.TXT > build/dist/INSTALL.TXT
	cd build/dist && shasum -a 256 EOB.EXE INTRO.EXE TGA.OVL | perl -pe 's/\r?\n/\r\n/' > SHA256.TXT
	@echo "dist: build/dist (EOB.EXE, INTRO.EXE, TGA.OVL, INSTALL.TXT, SHA256.TXT)"

play: game
	$(PY) tools/dbx.py --play

clean:
	rm -rf build
