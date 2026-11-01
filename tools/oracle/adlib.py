"""adlib.py: the AdLib card (Yamaha YM3812, OPL2) for the oracle: ports 388h (address and status) and
389h (data).

What is modelled is what a program can observe: the 256 registers, the two timers and the status
register they set (bit 7 IRQ, bit 6 timer 1, bit 5 timer 2), which is how detection routines find
the card (reset the timers, start timer 1 with count FFh, wait, expect status E0h & = C0h). Timer 1
counts in 80 us steps and timer 2 in 320 us steps of virtual time. No sound is synthesised: every
register write is logged as (virtual time, register, value) in machine.adlib_log, which is what a
sound golden compares (as poolrad's speaker and Tandy logs).
"""
from __future__ import annotations


class AdLib:
    def __init__(self, m, base: int = 0x388):
        self.m = m
        self.regs = bytearray(256)
        self.index = 0
        self.flags = 0                     # status bits 7..5
        self.started = [None, None]        # virtual time each timer was started
        m.adlib_log = self.log = []
        for port in (base, base + 1):
            m.port_handlers[port] = (self.read, self.write)
        self.base = base

    def _timers(self) -> None:
        now = self.m.seconds()
        for t, (step, bit) in enumerate(((80e-6, 0x40), (320e-6, 0x20))):
            start = self.started[t]
            if start is None:
                continue
            period = (256 - self.regs[2 + t]) * step
            masked = self.regs[4] & (0x40 if t == 0 else 0x20)
            if now - start >= period and not masked:
                self.flags |= bit | 0x80

    def read(self, port: int, size: int) -> int:
        if port == self.base:
            self._timers()
            self.m.extra += 20             # a status poll costs time, so delay loops end
            return self.flags | 0x06       # bits 2..1 read as 1 on an OPL2
        return 0xFF

    def write(self, port: int, size: int, val: int) -> None:
        val &= 0xFF
        if port == self.base:
            self.index = val
            return
        r = self.index
        self.log.append((self.m.now(), r, val))
        if r == 0x04:
            if val & 0x80:                 # reset the IRQ and timer flags; the other bits are ignored
                self.flags = 0
                return
            for t, bit in enumerate((0x01, 0x02)):
                if val & bit and self.started[t] is None:
                    self.started[t] = self.m.seconds()
                elif not val & bit:
                    self.started[t] = None
        self.regs[r] = val
