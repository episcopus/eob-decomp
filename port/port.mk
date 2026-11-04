# port.mk: the portable build (playbook 80). Run through `make port` from the repository root.
#
# tools/port/tc2c.py translates the units of port/units.txt from src/ (which it never writes) into
# build/port/gen/; the C compiler builds them with the runtime and the SDL3 host into
# build/port/eob-port, and the skeleton's self-test runs on the pinned eob/EOB.EXE (read only).

PY ?= python3
CC ?= cc
OUT := build/port
GEN := $(OUT)/gen
UNITS := $(shell sed -e 's/\#.*//' port/units.txt)
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl3 2>/dev/null)
# gnu99: the generated code uses statement expressions for compound assignments
CFLAGS := -std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-value -Wno-parentheses-equality -Iport/include -I$(GEN)
GEN_C := $(addprefix $(GEN)/,$(UNITS:.C=.c)) $(GEN)/units.c $(GEN)/unported.c
RUNTIME_C := port/runtime/arena.c port/main.c
HOST_C := port/host/host_sdl.c

.PHONY: port port-check sdl3

port: $(OUT)/eob-port port-check

sdl3:
	@pkg-config --exists sdl3 || { echo "port: SDL3 is not installed (pkg-config sdl3). Ask the human to install it (macOS: brew install sdl3); do not download it from a tool."; exit 1; }

# one translation writes every generated file (a stamp: make 3.81 has no grouped targets)
$(GEN)/.stamp: port/units.txt tools/port/tc2c.py port/include/tc/prelude.h $(addprefix src/,$(UNITS)) src/EOB.H
	$(PY) tools/port/tc2c.py --out $(GEN) $(addprefix src/,$(UNITS))
	@touch $@

$(GEN_C): $(GEN)/.stamp

$(OUT)/eob-port: $(GEN)/.stamp $(RUNTIME_C) $(HOST_C) port/include/tcport.h port/include/platform.h | sdl3
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(GEN_C) $(RUNTIME_C) $(HOST_C) $(SDL_LIBS)

port-check: $(OUT)/eob-port
	$(OUT)/eob-port --self-test eob/EOB.EXE
