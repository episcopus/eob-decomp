"""call.py: run one routine of the original in the oracle and return when it does.

    m = build_machine()                      # loaded, not started: memory and stack are the program's
    call_far(m, 0x1D9A, 0x018B, [src, dst, work])

Arguments are words, pushed right to left as Turbo C pushes them (`args[0]` ends up at [bp+6]).
The routine returns to a sentinel in the ROM area; a routine that does not return within
`max_insns` raises. Segments are load-relative (the addresses of the map and of re/symbols.toml).
Used to check a decoder against the game's own code (tests/unit/test_wwfmt.py) and, later, a
lifted routine against the original.
"""
from __future__ import annotations

from .machine import LOAD_SEG, Stop

SENTINEL = (0xF000, 0x3F00)


def call_far(m, seg: int, off: int, args: list[int], *, max_insns: int = 50_000_000) -> None:
    if not getattr(m, "_call_sentinel", False):
        m.uc.mem_write(m.linear(*SENTINEL), b"\x90")

        def done(mach):
            raise Stop("call-return")
        m.add_breakpoint(*SENTINEL, done)
        m._call_sentinel = True
    for a in reversed(args):
        m.push(a & 0xFFFF)
    m.push(SENTINEL[0])
    m.push(SENTINEL[1])
    m.set("cs", (LOAD_SEG + seg) & 0xFFFF)
    m.set("ip", off)
    reason = m.run(max_insns)
    if reason != "call-return":
        raise RuntimeError("%04X:%04X did not return: %s" % (seg, off, reason))
    m.set("sp", m.get("sp") + 2 * len(args))      # the caller pops the arguments
