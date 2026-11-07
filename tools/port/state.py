"""state.py: what of a session's state the port reproduces, and its digest (playbook 80, M5).

The golden's state digest (Machine.state_digest) covers the registers and all of conventional memory,
code included. The port has neither: it runs no machine code, ships none of the game's bytes (decision
6), and keeps its own frames on the stack and its own library state. So the port's checkpoints compare
a view of memory both sides can compute, which the oracle records beside the golden
(tests/sessions/golden/<name>.clock.json, `port`) and tools/port/replay.py computes from the port's
dump:

  - conventional memory from 0500h to A0000h, with these ranges masked (zero in the view):
  - the load module's code, 1000:0000 to DGROUP (the port has no code bytes; the assembly units' and
    the library's variables kept in their code segments are masked with it for now), but Westwood's far
    data (1F1D:0000 to 20B5:0000, src/far/FARDATA.ASM), which is compared;
  - the start-up code's data in DGROUP (ds:0000 to 008F) but the words the port's far heap reads;
  - the library's data (ds:84B2 to 88A0) but the variables the port's runtime keeps there
    (tools/port/image.py, RUNTIME) and the heap's size word;
  - the stack segment, 412D:0000 to the heap (frames, return addresses, argv), and the words in DGROUP
    that hold a stack address (STACK_SAVES);
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
# Westwood's far data, 1F1D:0000 to the overlay manager's data at 20B5:0000 (src/far/FARDATA.ASM): compared
FAR_DATA = (LOAD + 0x1F1D, LOAD + 0x20B5)
VGA = 0xA0000

# the words of the start-up code's data the port's far heap reads (port/runtime/tcheap.c)
C0_KEPT = [(0x0079, 0x007B), (0x0087, 0x008F)]
# DGROUP words that hold a stack address, which the port's stack cannot share: E1C00's SS:SP saved
# across the driver's EXEC (src/E1C00.ASM, ds:8192 and 8194)
STACK_SAVES = [(0x8192, 0x8196)]
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
    out = [(LOAD * 16, FAR_DATA[0] * 16), (FAR_DATA[1] * 16, ds)]
    out += [(ds + a, ds + b) for a, b in _holes(0x0000, 0x0090, C0_KEPT)]
    out += [(ds + a, ds + b) for a, b in _holes(0x84B2, 0x88A0, LIB_KEPT)]
    out += [(ds + a, ds + b) for a, b in STACK_SAVES]
    out.append((STACK * 16, HEAP * 16))
    paras = mem[HEAP * 16] | (mem[HEAP * 16 + 1] << 8)
    if paras:
        out.append((HEAP * 16 + 4, (HEAP + paras) * 16))
    return out


# The tick-driven state the comparison leaves out (playbook 80, M6, "The relaxed check": the human,
# 2026-11-07). The port places an interrupt by the game's accesses and the library's file calls; one that
# came while the original computed with neither (drawing a panel) reaches the port at its next wait,
# so what a loop derives from the ticks it sees can be a few ticks off. Only these are left out, each
# found by a replay and named:
#   - the selection frame's colour step, gSelectFrameStep (ds:54F4: one step each second tick the frame's
#     loop sees, 0D1C:0454, src/E0D1C.C);
#   - the MCGA driver's line colour, kept inside its code (driver:09E9, port/drivers/mcga_rect.c
#     G_LINE_COLOUR): the frame's colour drawn last;
#   - on the screen, the outline of the frame being drawn, gSelectFrameRects[gSelectFrameIndex]
#     (ds:5482, ds:54F2), whose colour is the step's; and the tick the frame next steps at,
#     gSelectFrameTick (ds:FC5E);
#   - the character panel's box animation (AnimateMemberBox, ovl0:1224, src/EOV0.C): its frame, the
#     frame it drew and the tick it next steps at (gBoxAnimFrame ds:FC84, gBoxAnimDrawnFrame ds:FC85,
#     gBoxAnimNextTick ds:FC86), the cell it draws in page 2 (288,128, 32 by 32) and the box on the
#     screen it copies the cell to (gChargenBoxX/Y[gChargenMember], ds:5E54, 5E5C, DCB0), while a
#     member's box is animated (gBoxAnimMember, ds:5EB0, below 6);
#   - where a mouse button's release lands against the game's reading of the ring: the last event read
#     (ds:80EF, src/E1AAD.ASM) and the button changes not yet read (gUnreadButtonChanges, ds:8134);
#   - the pointer's save-under buffer (gPointerBackOff/Seg, ds:816A, 180h bytes: src/E1BBE.ASM): what lay
#     under the pointer when the mouse interrupt last drew it.
TIMING_DS = [(0x54F4, 0x54F6), (0xFC5E, 0xFC62), (0xFC84, 0xFC8A), (0x80EF, 0x80F1), (0x8134, 0x8136)]
PAGE_SEGS = 0xFC0C               # gPageSegs (src/E07C8.C)
DRIVER_SLOTS = 0x8046            # the driver's far pointers (src/E1C00.ASM): their segment is the driver's
DRIVER_LINE_COLOUR = 0x09E9


def _w(mem, lin: int) -> int:
    return mem[lin] | (mem[lin + 1] << 8)


def timing_masks(mem) -> list[tuple[int, int]]:
    ds = DGROUP * 16
    out = [(ds + a, ds + b) for a, b in TIMING_DS]
    drv = _w(mem, ds + DRIVER_SLOTS + 2)
    if drv:
        out.append((drv * 16 + DRIVER_LINE_COLOUR, drv * 16 + DRIVER_LINE_COLOUR + 2))
    back = _w(mem, ds + 0x816C) * 16 + _w(mem, ds + 0x816A)
    if back:
        out.append((back, back + 0x180))
    page2 = _w(mem, ds + PAGE_SEGS + 4)
    if mem[ds + 0x5EB0] < 6 and page2 and page2 != 0xA000:
        for y in range(0x80, 0xA0):
            a = page2 * 16 + y * 320 + 0x120
            out.append((a, a + 0x20))
    return out


def box_rect(mem) -> list[int]:
    """The screen offsets of the animated member box, while one is animated."""
    ds = DGROUP * 16
    if mem[ds + 0x5EB0] >= 6:
        return []
    m = _w(mem, ds + 0xDCB0)
    if m > 3:
        return []
    x, y = _w(mem, ds + 0x5E54 + 2 * m), _w(mem, ds + 0x5E5C + 2 * m) + 1
    return [(y + r) * 320 + x + c for r in range(0x20) for c in range(0x20) if (y + r) * 320 + x + c < 64000]


def frame_outline(mem) -> list[int]:
    """The screen offsets of the selection frame's outline, when one is drawn."""
    ds = DGROUP * 16
    i = _w(mem, ds + 0x54F2)
    if i == 0xFFFF or i > 13:
        return []
    r = ds + 0x5482 + 8 * i
    x1, y1, x2, y2 = (_w(mem, r + k) for k in (0, 2, 4, 6))
    out = []
    for x in range(x1, x2 + 1):
        out += [y1 * 320 + x, y2 * 320 + x]
    for y in range(y1, y2 + 1):
        out += [y * 320 + x1, y * 320 + x2]
    return [o for o in out if 0 <= o < 64000]


def view(mem) -> bytes:
    """Conventional memory 0500h to A0000h as the port can have it: `mem` is at least A0000h bytes of
    the machine's memory from linear 0."""
    b = bytearray(mem[:TOP])
    for a, z in masks(b) + timing_masks(b):
        b[a:z] = bytes(z - a)
    return bytes(b[LOW:TOP])


def digest(mem) -> str:
    return hashlib.sha256(view(mem)).hexdigest()


def screen(vga: bytes, dac: bytes, mem=None) -> str:
    """The sha256 of mode 13h's screen as RGB, as tools/oracle/ega.py renders it (the golden's
    `image`): each DAC component (6 bits) widened to 8. With `mem`, the selection frame's outline is
    left out, and the animated member box (colour 0, timing_masks)."""
    if mem is not None:
        out = frame_outline(mem) + box_rect(mem)
        if out:
            vga = bytearray(vga[:64000])
            for o in out:
                vga[o] = 0
    rgb = bytes((c << 2) | (c >> 4) for c in dac[:768])
    table = [rgb[3 * i:3 * i + 3] for i in range(256)]
    return hashlib.sha256(b"".join(table[v] for v in vga[:64000])).hexdigest()
