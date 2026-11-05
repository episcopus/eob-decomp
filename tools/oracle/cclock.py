"""cclock.py: the C-unit clock of the port's proof (playbook 80, "The clock with hand-written assembly").

The port runs the C units translated (tc2c), the assembly units hand-written in C, and its own runtime in
place of the Borland library; only the C units have the original's control flow block for block. So the
proof's clock counts only the instructions executed in the code of the C units: those of
port/units.txt, resident (their segments, re/units.toml) or overlaid (an overlay's code, from the moment
the overlay manager reads it into its buffer). Instructions of assembly units, of the library, of the
start-up and of the ROM and services do not advance it.

Attached to a Machine (`CClock(m)`), it counts with a block hook: each translation block whose first
instruction lies in C code adds its instruction count (capstone, cached by address and size). A run chunk
(`Machine.run`'s emu_start with a count) can stop inside a block; after each chunk (Machine.chunk_listeners)
the overshoot, the sum of the chunk's block counts minus the chunk, is taken back from the last block if it
was C code. `test_the_c_unit_clock` checks the sum against a per-instruction count.

It also records what the port needs to replay a session on this clock (the recording file,
tests/sessions/golden/<name>.clock.json, decision E):

  - each hardware interrupt the machine delivers, at its C-clock position, in order: `[position]` for IRQ 0,
    `[position, "irq1", scancode]`, `[position, "mouse", condition, buttons, x, y]`;
  - the oracle's instruction count (`Machine.insns`, which moves by chunks: the timer, the retrace bit, the
    AdLib's timers and the time of day read it) at each port access or service call where it changed since
    the previous one, keyed by the access's ordinal: the port's machine sets its clock from it, since the
    chunks are the oracle's scheduling and no C clock gives them;
  - the C-clock position of each checkpoint of the session.
"""
from __future__ import annotations

import json
import tomllib
from pathlib import Path

import capstone
from unicorn import UC_HOOK_BLOCK

from .machine import LOAD_SEG

ROOT = Path(__file__).resolve().parents[2]
UNITS_TXT = ROOT / "port" / "units.txt"
UNITS_TOML = ROOT / "re" / "units.toml"
LAYOUT = ROOT / "build" / "re" / "eob.json"


def c_units() -> set[str]:
    """The C units of port/units.txt, as re/units.toml names them (e07a9, eov0, ...)."""
    out = set()
    for line in UNITS_TXT.read_text().splitlines():
        line = line.split("#")[0].strip()
        if line:
            out.add(line.rsplit(".", 1)[0].lower())
    return out


def resident_ranges(program: str = "EOB.EXE") -> list[tuple[int, int]]:
    """Load-relative linear [start, end) of each resident C unit's code."""
    units = tomllib.loads(UNITS_TOML.read_text())["unit"]
    want = c_units()
    out = []
    for u in units:
        if u["program"] != program or u["id"] not in want or u["start"].startswith("ovl"):
            continue
        seg, off = (int(x, 16) for x in u["start"].split(":"))
        a = seg * 16 + off
        out.append((a, a + u["bytes"]))
    return sorted(out)


class CClock:
    def __init__(self, m, program: str = "EOB.EXE"):
        self.m = m
        base = LOAD_SEG * 16
        self.ranges = [(base + a, base + b) for a, b in resident_ranges(program)]
        self.overlay_ranges: list[tuple[int, int]] = []     # where overlay code sits now
        self.count = 0                  # C-unit instructions so far
        self.chunk_sum = 0              # instructions of every block entered in this chunk
        self.last = None                # (address, size, instructions, is C) of the last block entered
        self.sizes: dict[tuple[int, int], tuple[int, ...]] = {}
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        self.events: list[list] = []    # [cclock, kind, data...]
        self.times: list[list[int]] = []   # [access ordinal, insns]
        self.accesses = 0
        self._last_insns = None
        self.checkpoints: list[list] = []
        self._overlays = []
        if program == "EOB.EXE" and LAYOUT.exists():
            lay = json.loads(LAYOUT.read_text())
            self._overlays = [(o["code_bytes"], o["code_sha256"]) for o in lay["overlays"]]
        m.uc.hook_add(UC_HOOK_BLOCK, self._on_block)
        m.chunk_listeners.append(self._on_chunk)
        m.file_listeners.append(self._on_file)
        self._wrap(m)

    # ---- counting
    def is_c(self, lin: int) -> bool:
        for a, b in self.ranges:
            if a <= lin < b:
                return True
        for a, b in self.overlay_ranges:
            if a <= lin < b:
                return True
        return False

    def _offsets(self, addr: int, size: int) -> tuple[int, ...]:
        k = (addr, size)
        offs = self.sizes.get(k)
        if offs is None:
            code = bytes(self.m.uc.mem_read(addr, size))
            offs = tuple(i.address - addr for i in self.md.disasm(code, addr))
            self.sizes[k] = offs
        return offs

    def _on_block(self, uc, addr, size, _ud):
        n = len(self._offsets(addr, size))
        c = self.is_c(addr)
        self.last = (addr, size, n, c)
        self.chunk_sum += n
        if c:
            self.count += n

    def _on_chunk(self, m, chunk: int):
        over = self.chunk_sum - chunk
        if over > 0 and self.last is not None and self.last[3]:
            self.count -= over
        self.chunk_sum = 0
        self.last = None

    def _on_file(self, m, op, path, pos, n):
        """The overlay manager reads an overlay's code (with its fixups) into its buffer: that code is C."""
        if op != "read" or not path.upper().endswith(".EXE") or not self._overlays:
            return
        import hashlib
        lin = m.linear(m.get("ds"), m.get("dx"))
        for size, digest in self._overlays:
            if n >= size and hashlib.sha256(bytes(m.uc.mem_read(lin, size))).hexdigest() == digest:
                new = (lin, lin + size)
                self.overlay_ranges = [r for r in self.overlay_ranges if r[1] <= new[0] or r[0] >= new[1]] + [new]
                return

    # ---- the recording
    def _wrap(self, m):
        hw = {"on": False}
        deliver = m.deliver_interrupt

        def deliver_interrupt(n, clear_if=True):
            if hw["on"]:
                if n == 0x09:
                    self.events.append([self.count, "irq1", m.kbd_data])
                elif n == 0xFF:
                    from .input import ROM_MOUSE
                    import struct
                    pkt = struct.unpack("<4H", bytes(m.uc.mem_read(m.linear(*ROM_MOUSE) + 0x80, 8)))
                    self.events.append([self.count, "mouse", *pkt])
                else:
                    self.events.append([self.count])            # IRQ 0: the position alone
            return deliver(n, clear_if)
        m.deliver_interrupt = deliver_interrupt
        for name in ("_try_deliver_irq0", "_try_deliver_irq1", "_try_deliver_mouse"):
            orig = getattr(m, name)

            def wrapped(orig=orig):
                hw["on"] = True
                try:
                    return orig()
                finally:
                    hw["on"] = False
            setattr(m, name, wrapped)

        def access():
            self.accesses += 1
            if m.insns != self._last_insns:
                self._last_insns = m.insns
                self.times.append([self.accesses, m.insns])
        read, write = m.port_read, m.port_write

        def port_read(port, size):
            access()
            return read(port, size)

        def port_write(port, size, val):
            access()
            return write(port, size, val)
        m.port_read, m.port_write = port_read, port_write
        for n, fn in list(m.services.items()):
            def service(mach, fn=fn):
                access()
                return fn(mach)
            m.services[n] = service

    def checkpoint(self, name: str) -> None:
        self.checkpoints.append([name, self.count])

    def recording(self) -> dict:
        return {"clock": "instructions executed in the code of the C units of port/units.txt (playbook 80)",
                "checkpoints": self.checkpoints, "interrupts": self.events, "times": self.times,
                "accesses": self.accesses}
