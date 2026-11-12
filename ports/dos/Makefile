# ports/dos/Makefile: Eye of the Beholder for the Tandy 1000 EX: the game rebuilt from src/ with this port's patches.
# Run from here or with `make -C ports/dos <target>`. src/ and the other tools are never touched: the build stages a
# copy under build/. Needs the project's setup (make setup, make tc-install, make emu2 at the repository root).

ROOT := ../..
PY := $(if $(wildcard $(ROOT)/.venv/bin/python),$(ROOT)/.venv/bin/python,python3)
YMFM := $(ROOT)/tools/vendor/ymfm/src
SOUND := build/sound
CAPTURES := $(SOUND)/sfx-adlib.json $(SOUND)/sfx-tandy.json $(SOUND)/intro-adlib.json $(SOUND)/intro-tandy.json

.PHONY: help game ref func prof spans tandy test dist play sound sn music sound-check clean
.DEFAULT_GOAL := help

help:
	@echo "ports/dos: the Tandy 1000 EX build (README.md)"
	@echo "  make game    build/game/EOB.EXE, INTRO.EXE, TGA.OVL, SN.DAT and SNSOUND.DAT: the game, with patches/series"
	@echo "  make ref     build/ref: the same, no patches: must be the original program"
	@echo "  make func    the gate: the same game on ref and game, screen, pages, DGROUP, sound, file reads (SESSION=name)"
	@echo "  make prof    the 8088 model: what each action costs on the EX, ref against game (SESSION=name, BUS=4|5|6)"
	@echo "  make spans   the timed effects: every spell burst and intro dissolve, ref against game"
	@echo "  make tandy   DOSBox-X machine=tandy, headless: ref and game show the same screens (about 3 min)"
	@echo "  make test    game, ref, func and tandy"
	@echo "  make dist    build/dist: the files to copy to the machine, with INSTALL.TXT and checksums"
	@echo "  make play    DOSBox-X window on the Tandy machine, for a look (not a measurement)"
	@echo "  make sound   the Tandy FM study: AdLib, today's Tandy and the mapper model side by side (sound/README.md)"
	@echo "  make sn      build/sound/SN.DAT and SNSOUND.DAT: the Tandy's sounds (sound/picks.json, the intro's rich), and their player"
	@echo "  make music   build/sound/music.html: the four pieces arranged for the chip, beside the AdLib and the mapping"
	@echo "  make sound-check  the patched game plays every sound of SN.DAT, and the intro all of SNSOUND.DAT, as the model does"

game: $(SOUND)/SN.DAT $(SOUND)/SNSOUND.DAT
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
	cp build/game/EOB.EXE build/game/INTRO.EXE build/game/TGA.OVL build/game/SN.DAT build/game/SNSOUND.DAT build/dist/
	perl -pe 's/\r?\n/\r\n/' INSTALL.TXT > build/dist/INSTALL.TXT
	cd build/dist && shasum -a 256 EOB.EXE INTRO.EXE TGA.OVL SN.DAT SNSOUND.DAT | perl -pe 's/\r?\n/\r\n/' > SHA256.TXT
	@echo "dist: build/dist (EOB.EXE, INTRO.EXE, TGA.OVL, SN.DAT, SNSOUND.DAT, INSTALL.TXT, SHA256.TXT)"

play: game
	$(PY) tools/dbx.py --play

# the Tandy FM study (sound/README.md): the captures take a few minutes each and are kept until `make clean`

$(SOUND)/render: sound/render.cpp
	@test -f $(YMFM)/ymfm_opl.h || { echo "sound: ymfm not found (make ymfm at the repository root)"; exit 1; }
	mkdir -p $(SOUND)
	c++ -O2 -std=c++17 -I$(YMFM) -o $@ $< $(YMFM)/ymfm_opl.cpp $(YMFM)/ymfm_adpcm.cpp $(YMFM)/ymfm_pcm.cpp

$(SOUND)/sfx-%.json:
	test -f build/ref/EOB.EXE || $(MAKE) ref
	$(PY) sound/capture.py sfx --card $*

$(SOUND)/intro-%.json:
	test -f build/ref/INTRO.EXE || $(MAKE) ref
	$(PY) sound/capture.py intro --card $*

sound: $(SOUND)/render $(CAPTURES)
	$(PY) sound/tandymap.py clips
	@echo "sound: open $(SOUND)/index.html"

# what the game plays on a Tandy (sound/README.md, "The game's sound"): the picked clips' chip logs, so the clips first
$(SOUND)/clips.json: $(SOUND)/render $(CAPTURES) sound/tandymap.py sound/designs.py
	$(PY) sound/tandymap.py clips

# the intro's sounds, each alone on the AdLib (about a minute), and their rich mapping
$(SOUND)/introparts-adlib.json:
	mkdir -p $(SOUND)
	$(PY) sound/capture.py introparts

$(SOUND)/clips-intro.json: $(SOUND)/render $(SOUND)/introparts-adlib.json sound/tandymap.py
	$(PY) sound/tandymap.py intro

# sndat.py build writes both files
$(SOUND)/SN.DAT: $(SOUND)/clips.json $(SOUND)/clips-intro.json sound/picks.json sound/sndat.py sound/SNPLAY.ASM \
		sound/arrange.py sound/score.py
	$(PY) sound/sndat.py build
	$(PY) sound/sndat.py selftest

$(SOUND)/SNSOUND.DAT: $(SOUND)/SN.DAT

sn: $(SOUND)/SN.DAT $(SOUND)/SNSOUND.DAT

# the music arranged for the chip beside the AdLib and the mapping: build/sound/music.html
music: $(SOUND)/clips.json $(SOUND)/clips-intro.json
	$(PY) sound/arrange.py clips

sound-check: game
	$(PY) sound/capture.py sfx --card tandy --build game
	$(PY) sound/sndat.py check
	$(PY) sound/sndat.py check-intro

clean:
	rm -rf build
