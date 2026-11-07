"""state.py: what of a session's state the port reproduces, and its digest (playbook 80, M5).

The golden's state digest (Machine.state_digest) covers the registers and all of conventional memory,
code included. The port has neither: it runs no machine code, ships none of the game's bytes (decision
6), and keeps its own frames on the stack and its own library state. So the port's checkpoints compare
a view of memory both sides can compute, which the oracle records beside the golden
(tests/sessions/golden/<name>.clock.json, `port`) and tools/port/replay.py computes from the port's
dump:

  - conventional memory from 0500h to A0000h, with these ranges masked (zero in the view):
  - the load module's code, 1000:0000 to DGROUP (the port has no code bytes; the assembly units' and
    the library's variables kept in their code segments are masked with it for now);
  - the start-up code's data in DGROUP (ds:0000 to 008F) but the words the port's far heap reads;
  - the library's data (ds:84B2 to 88A0) but the variables the port's runtime keeps there
    (tools/port/image.py, RUNTIME) and the heap's size word;
  - the stack segment, 412D:0000 to the heap (frames, return addresses, argv);
  - the overlay buffer, the heap's first block (the overlay manager's, which the port allocates and
    leaves empty: its overlays are plain code);
  - the screen apart: mode 13h's 64,000 bytes at A000:0000 through the DAC, rendered as the golden's
    screenshots are (tools/oracle/ega.py render), so a `shot` checkpoint compares with the golden's
    own image hash.
"""
from __future__ import annotations

import hashlib

LOAD = 0x1000
DGROUP = LOAD + 0x212E
STACK = LOAD + 0x312D            # the stack segment (build/re/eob.json "stack"), up to the heap
HEAP = 0x422E                    # the heap's first block: the end of the stack segment
LOW, TOP = 0x500, 0xA0000
VGA = 0xA0000

# the words of the start-up code's data the port's far heap reads (port/runtime/tcheap.c)
C0_KEPT = [(0x0079, 0x007B), (0x0087, 0x008F)]
# the library's data the runtime keeps (tools/port/image.py RUNTIME) and the heap's size word
LIB_KEPT = [(0x8758, 0x8782), (0x8784, 0x8788), (0x8810, 0x881E), (0x85C6, 0x85C8)]


def _holes(lo: int, hi: int, kept: list[tuple[int, int]]) -> list[tuple[int, int]]:
    out, at = [], lo
    for a, b in sorted(kept):
        if a > at:
            out.append((at, a))
        at = max(at, b)
    if at < hi:
        out.append((at, hi))
    return out


def masks(mem) -> list[tuple[int, int]]:
    """The linear ranges [a, b) the view leaves out, for memory `mem` (the overlay buffer's size is read
    from its heap header)."""
    ds = DGROUP * 16
    out = [(LOAD * 16, ds)]
    out += [(ds + a, ds + b) for a, b in _holes(0x0000, 0x0090, C0_KEPT)]
    out += [(ds + a, ds + b) for a, b in _holes(0x84B2, 0x88A0, LIB_KEPT)]
    out.append((STACK * 16, HEAP * 16))
    paras = mem[HEAP * 16] | (mem[HEAP * 16 + 1] << 8)
    if paras:
        out.append((HEAP * 16 + 4, (HEAP + paras) * 16))
    return out


def view(mem) -> bytes:
    """Conventional memory 0500h to A0000h as the port can have it: `mem` is at least A0000h bytes of
    the machine's memory from linear 0."""
    b = bytearray(mem[:TOP])
    for a, z in masks(b):
        b[a:z] = bytes(z - a)
    return bytes(b[LOW:TOP])


def digest(mem) -> str:
    return hashlib.sha256(view(mem)).hexdigest()


def screen(vga: bytes, dac: bytes) -> str:
    """The sha256 of mode 13h's screen as RGB, as tools/oracle/ega.py renders it (the golden's
    `image`): each DAC component (6 bits) widened to 8."""
    rgb = bytes((c << 2) | (c >> 4) for c in dac[:768])
    table = [rgb[3 * i:3 * i + 3] for i in range(256)]
    return hashlib.sha256(b"".join(table[v] for v in vga[:64000])).hexdigest()
