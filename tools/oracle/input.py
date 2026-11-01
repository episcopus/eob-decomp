"""input.py: scripted keyboard and mouse for the oracle (playbook 25).

Eye of the Beholder reads the keyboard from its own INT 9 handler (port 60h, 1AAD:080C) and the
mouse through an INT 33h event handler it installs (AX=0Ch, 1A72:016F); it never calls INT 16h.
So input reaches it the way hardware delivers it: scancodes on port 60h with IRQ1, and calls to
its mouse handler from an interrupt, at fixed points of virtual time. A run depends on the
program and the script alone, never on host timing.

Script (tests/sessions/*.keys), one command per line, `#` starts a comment:

    wait 18              let 18 ticks of virtual time pass (a tick is 1/18.2 s, about 55 ms)
    key ENTER            press and release a key (held 2 ticks): ENTER ESC SPACE TAB BKSP UP DOWN
                         LEFT RIGHT HOME END PGUP PGDN INS DEL F1..F10, a letter or digit, or a raw
                         scancode 0x1C
    down SHIFT / up SHIFT  hold or release a key (SHIFT CTRL ALT as well as the keys above)
    text Name            types the characters, one key every 3 ticks
    mouse 160 100        move the pointer to x, y (screen pixels, 320x200)
    click 160 100 [R]    move there, press and release the left (or right) button
    shot title           screenshot `title` now
    digest title         record a state digest checkpoint now
    quit                 stop the run
    option mouse         run settings, read by the runner before the run: `option mouse`,
    option sound adlib   `option sound adlib`, `option insns 80000000` (the instruction budget),
                         `option save fixtures/EOBDATA.SAV` (the saved game, from tests/sessions/)

Every command runs when the previous ones are done: `key` takes 2 ticks, `click` 2 ticks,
`text` 3 ticks per character, `wait N` N ticks; `mouse`, `shot`, `digest` take no time.
"""
from __future__ import annotations

import struct

from .machine import Stop

SCAN = {
    "ESC": 0x01, "BKSP": 0x0E, "TAB": 0x0F, "ENTER": 0x1C, "RETURN": 0x1C, "CTRL": 0x1D, "SHIFT": 0x2A,
    "RSHIFT": 0x36, "ALT": 0x38, "SPACE": 0x39, "HOME": 0x47, "UP": 0x48, "PGUP": 0x49, "LEFT": 0x4B,
    "RIGHT": 0x4D, "END": 0x4F, "DOWN": 0x50, "PGDN": 0x51, "INS": 0x52, "DEL": 0x53,
    "KP5": 0x4C, "KP7": 0x47, "KP9": 0x49, "KP1": 0x4F, "KP3": 0x51,
}
for _i in range(10):
    SCAN["F%d" % (_i + 1)] = 0x3B + _i
for _row, _start in (("1234567890-=", 0x02), ("qwertyuiop[]", 0x10), ("asdfghjkl;'`", 0x1E), ("\\zxcvbnm,./", 0x2B)):
    for _k, _c in enumerate(_row):
        SCAN[_c] = _start + _k
        SCAN[_c.upper()] = _start + _k

TICK = 1 / 18.2065
KEY_HOLD = 2
TEXT_STEP = 3


def scancode(tok: str) -> int:
    if tok.lower().startswith("0x"):
        return int(tok, 16)
    if tok in SCAN:
        return SCAN[tok]
    if tok.upper() in SCAN:
        return SCAN[tok.upper()]
    raise ValueError("unknown key %r" % tok)


class Script:
    """Turns script lines into timed events: (tick, kind, args)."""

    def __init__(self, text: str):
        self.events, t = [], 0
        self.options: dict[str, str] = {}
        for n, raw in enumerate(text.splitlines(), 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            op, _, arg = line.partition(" ")
            arg = arg.strip()
            try:
                if op == "option":
                    k, _, v = arg.partition(" ")
                    self.options[k] = v.strip() or "1"
                elif op == "wait":
                    t += int(arg)
                elif op == "key":
                    k = scancode(arg)
                    self.events += [(t, "kbd", k), (t + KEY_HOLD, "kbd", k | 0x80)]
                    t += KEY_HOLD
                elif op in ("down", "up"):
                    self.events.append((t, "kbd", scancode(arg) | (0x80 if op == "up" else 0)))
                elif op == "text":
                    for c in arg:
                        k = scancode("SPACE" if c == " " else c)
                        shift = c.isupper()
                        if shift:
                            self.events.append((t, "kbd", SCAN["SHIFT"]))
                        self.events += [(t, "kbd", k), (t + 1, "kbd", k | 0x80)]
                        if shift:
                            self.events.append((t + 1, "kbd", SCAN["SHIFT"] | 0x80))
                        t += TEXT_STEP
                elif op == "mouse":
                    x, y = (int(v) for v in arg.split()[:2])
                    self.events.append((t, "move", (x, y)))
                elif op == "click":
                    parts = arg.split()
                    x, y, b = int(parts[0]), int(parts[1]), 2 if parts[2:] == ["R"] else 1
                    self.events += [(t, "move", (x, y)), (t, "button", b), (t + KEY_HOLD, "button", 0)]
                    t += KEY_HOLD
                elif op in ("shot", "digest"):
                    self.events.append((t, op, arg or "t%d" % t))
                elif op == "quit":
                    self.events.append((t, "quit", None))
                else:
                    raise ValueError("unknown command %r" % op)
            except ValueError as e:
                raise ValueError("line %d: %s" % (n, e)) from None
        self.events.sort(key=lambda e: e[0])
        self.next = 0
        self.on_checkpoint = None          # callable(machine, kind, name), set by the runner

    @classmethod
    def from_file(cls, path) -> "Script":
        return cls(open(path).read())

    def due(self, m) -> int:
        return int(m.seconds() / TICK)

    def service(self, m) -> None:
        """Called by Machine.run at every chunk boundary."""
        now = self.due(m)
        while self.next < len(self.events) and self.events[self.next][0] <= now:
            _, kind, arg = self.events[self.next]
            self.next += 1
            if kind == "kbd":
                m.kbd_queue.append(arg)
            elif kind == "move" and m.mouse:
                m.mouse.move(*arg)
            elif kind == "button" and m.mouse:
                m.mouse.buttons_to(arg)
            elif kind in ("shot", "digest") and self.on_checkpoint:
                self.on_checkpoint(m, kind, arg)
            elif kind == "quit":
                raise Stop("script quit")

    def done(self) -> bool:
        return self.next >= len(self.events)


# ---------------------------------------------------------------- the mouse driver (INT 33h)

# The event trampoline at F000:3000, entered like an interrupt: save the interrupted registers,
# load the event (AX condition, BX buttons, CX x, DX y, SI/DI mickeys) from F000:3080, far call
# the program's handler at F000:3090, restore, IRET. The machine marks the mouse free again when
# the IRET at F000:3080 - 1 runs.
TRAMPOLINE = bytes([
    0x50, 0x53, 0x51, 0x52, 0x56, 0x57, 0x55, 0x1E, 0x06,              # push ax bx cx dx si di bp ds es
    0x2E, 0xA1, 0x80, 0x30,                                            # mov ax,cs:[3080]
    0x2E, 0x8B, 0x1E, 0x82, 0x30,                                      # mov bx,cs:[3082]
    0x2E, 0x8B, 0x0E, 0x84, 0x30,                                      # mov cx,cs:[3084]
    0x2E, 0x8B, 0x16, 0x86, 0x30,                                      # mov dx,cs:[3086]
    0x2E, 0x8B, 0x36, 0x88, 0x30,                                      # mov si,cs:[3088]
    0x2E, 0x8B, 0x3E, 0x8A, 0x30,                                      # mov di,cs:[308A]
    0x2E, 0xFF, 0x1E, 0x90, 0x30,                                      # call far cs:[3090]
    0x07, 0x1F, 0x5D, 0x5F, 0x5E, 0x5A, 0x59, 0x5B, 0x58,              # pop es ds bp di si dx cx bx ax
    0xCF,                                                              # iret
])


class Mouse:
    """A two-button mouse driver with the functions the game uses (0, 1, 2, 3, 4, 7, 8, 0Ch)."""

    def __init__(self, m):
        self.x = self.y = 0
        self.buttons = 0
        self.handler = None                # (seg, off, mask)
        self.events: list[tuple[int, int, int, int]] = []   # (condition, buttons, x, y)
        self.range_x, self.range_y = (0, 639), (0, 199)
        self.visible = 0
        base = m.linear(*ROM_MOUSE)
        m.uc.mem_write(base, TRAMPOLINE)
        self.iret = base + len(TRAMPOLINE) - 1
        m.register_service(0x33, self.int33)
        # programs test for a driver by looking at the vector: not null, not pointing at an IRET (1A72:010E)
        m.uc.mem_write(m.linear(0xF000, 0x1000 + 0x33 * 4), b"\x90\xCF\x90\x90")
        m.mouse = self

        def free(uc, addr, size, ud):
            m.mouse_busy = False
        from unicorn import UC_HOOK_CODE
        m.uc.hook_add(UC_HOOK_CODE, free, begin=self.iret, end=self.iret)

    # driver coordinates: x is 0..639 in mode 13h, as with the Microsoft driver
    def move(self, sx: int, sy: int) -> None:
        self.x = max(self.range_x[0], min(self.range_x[1], sx * 2))
        self.y = max(self.range_y[0], min(self.range_y[1], sy))
        self._event(0x01)

    def buttons_to(self, b: int) -> None:
        cond = 0
        for bit, down, up in ((1, 0x02, 0x04), (2, 0x08, 0x10)):
            if b & bit and not self.buttons & bit:
                cond |= down
            elif self.buttons & bit and not b & bit:
                cond |= up
        self.buttons = b
        if cond:
            self._event(cond)

    def _event(self, cond: int) -> None:
        if self.handler and cond & self.handler[2]:
            self.events.append((cond, self.buttons, self.x, self.y))

    def deliver(self, m) -> bool:
        if not self.events or not self.handler:
            return False
        cond, b, x, y = self.events.pop(0)
        seg, off, _ = self.handler
        m.uc.mem_write(m.linear(*ROM_MOUSE) + 0x80, struct.pack("<6H", cond, b, x, y, 0, 0))
        m.uc.mem_write(m.linear(*ROM_MOUSE) + 0x90, struct.pack("<HH", off, seg))
        m.deliver_interrupt(0xFF)          # pushes FLAGS, CS, IP and clears IF, like the driver's IRQ
        m.set("cs", ROM_MOUSE[0])
        m.set("ip", ROM_MOUSE[1])
        return True

    def int33(self, m) -> None:
        ax = m.get("ax")
        if ax == 0x00:                     # reset: installed, two buttons, pointer centred (640x200 range)
            self.handler, self.visible = None, 0
            self.x, self.y = 320, 100
            m.set("ax", 0xFFFF)
            m.set("bx", 2)
        elif ax == 0x01:
            self.visible += 1
        elif ax == 0x02:
            self.visible -= 1
        elif ax == 0x03:
            m.set("bx", self.buttons)
            m.set("cx", self.x)
            m.set("dx", self.y)
        elif ax == 0x04:
            self.x, self.y = m.get("cx"), m.get("dx")
        elif ax == 0x07:
            self.range_x = (m.get("cx"), m.get("dx"))
        elif ax == 0x08:
            self.range_y = (m.get("cx"), m.get("dx"))
        elif ax == 0x0C:
            mask = m.get("cx")
            self.handler = (m.get("es"), m.get("dx"), mask) if mask else None
        else:
            m.unimplemented("INT 33h AX=%04Xh" % ax)


ROM_MOUSE = (0xF000, 0x3000)


def no_mouse(m) -> None:
    """INT 33h with no driver loaded: reset answers AX=0."""
    def int33(mach):
        if mach.get("ax") == 0:
            mach.set("ax", 0)
            mach.set("bx", 0)
    m.register_service(0x33, int33)
