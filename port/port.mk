# port.mk: the portable build (playbook 80). Run through `make port` from the repository root.
#
# tools/port/tc2c.py translates the units of port/units.txt from src/ (which it never writes) into
# build/port/gen/. Platform separation (playbook 80): the game (the generated C), the runtime and
# main.c are compiled once, with no platform headers on the include path, and linked twice:
#
#   build/port/eob-port-headless   with port/platform/null.c; needs no SDL; the self-test runs on it
#   build/port/eob-port            with port/platform/sdl_host.c and SDL3 (built when SDL3 is found)
#
# port-boundary fails if any file outside port/platform/ includes SDL or calls it, so the boundary stays
# platform.h. The self-test runs on the pinned eob/EOB.EXE (read only); tc2c reads it too, to place
# the string literals where its DGROUP holds them, and reads build/re/eob.json (make overlays) for the
# overlay stub entries and the segment table.

PY ?= python3
CC ?= cc
OUT := build/port
GEN := $(OUT)/gen
OBJ := $(OUT)/obj
UNITS := $(shell sed -e 's/\#.*//' port/units.txt)
HAVE_SDL3 := $(shell pkg-config --exists sdl3 2>/dev/null && echo yes)
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl3 2>/dev/null)
# gnu99: the generated code uses statement expressions for compound assignments (decision 2)
CFLAGS := -std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-value -Wno-parentheses-equality -Iport/include -I$(GEN)
# the generated C keeps every parameter of src/, read or not
GEN_CFLAGS := -Wno-unused-parameter
HEADERS := $(wildcard port/include/*.h port/include/tc/*.h)
TC2C_IN := port/units.txt tools/port/tc2c.py port/include/tc/prelude.h $(wildcard port/include/tc/sys/*.h) \
	port/include/tclib.h port/literals.json build/re/eob.json eob/EOB.EXE $(wildcard src/*.H src/*.ASM)

GEN_C := $(addprefix $(GEN)/,$(UNITS:.C=.c)) $(GEN)/units.c $(GEN)/unported.c
CORE_C := port/runtime/arena.c port/runtime/tclib.c port/main.c
CORE_O := $(patsubst $(GEN)/%.c,$(OBJ)/gen/%.o,$(GEN_C)) $(patsubst %.c,$(OBJ)/%.o,$(CORE_C))

.PHONY: port port-check port-boundary port-sdl

port: port-boundary $(OUT)/eob-port-headless port-check port-sdl

port-boundary:
	@files=$$(grep -rlE '#include *<SDL|SDL_[A-Za-z]' port --include='*.c' --include='*.h' | grep -v '^port/platform/' || true); \
	if [ -n "$$files" ]; then echo "port: only port/platform/ may use SDL; found in: $$files"; exit 1; fi

# one translation writes every generated file (a stamp: make 3.81 has no grouped targets)
$(GEN)/.stamp: $(TC2C_IN) $(addprefix src/,$(UNITS))
	$(PY) tools/port/tc2c.py --out $(GEN) --image eob/EOB.EXE --layout build/re/eob.json $(addprefix src/,$(UNITS))
	@touch $@

$(GEN_C): $(GEN)/.stamp

$(OBJ)/gen/%.o: $(GEN)/%.c $(GEN)/.stamp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GEN_CFLAGS) -c -o $@ $<

$(OBJ)/port/platform/sdl_host.o: CFLAGS += $(SDL_CFLAGS)
$(OBJ)/port/%.o: port/%.c $(GEN)/.stamp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/eob-port-headless: $(CORE_O) $(OBJ)/port/platform/null.o
	$(CC) -o $@ $^

$(OUT)/eob-port: $(CORE_O) $(OBJ)/port/platform/sdl_host.o
	$(CC) -o $@ $^ $(SDL_LIBS)

port-check: $(OUT)/eob-port-headless
	$(OUT)/eob-port-headless --self-test eob/EOB.EXE

ifeq ($(HAVE_SDL3),yes)
port-sdl: $(OUT)/eob-port
else
port-sdl:
	@echo "port: SDL3 not found (pkg-config sdl3): built the headless program only. Ask the human to install SDL3 (macOS: brew install sdl3); do not download it from a tool."
endif
