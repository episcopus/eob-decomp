#!/usr/bin/env python3
"""hybrid.py: each translated function against the original, on the states the goldens reach (M2,
playbook 80).

A golden session (tests/sessions/<name>.keys) plays on the oracle as `make golden` plays it. Where
the original calls a function of src/ that tc2c translated (its address, or an overlay function's
stub entry: build/port/gen/hybrid.c), the first --samples calls of each are tested on a second
machine, the bench, so the session itself is not touched (it is checked against its golden at the
end):

  1. the bench takes the session's state: conventional memory, the registers, and the state the
     oracle keeps in Python (the EGA, DOS and its files, the BIOS, the devices);
  2. the original runs on the bench from the entry to its return (no interrupts are delivered on the
     bench, so what the handlers write is the same in both runs);
  3. the bench takes the same state again and the C runs on it: the shared library of
     `make port-hybrid` (build/port/hybrid/libeobhybrid.so) has the bench's memory as its arena, its
     arguments where the caller pushed them; a routine the port has no C for (an assembly unit, a
     library function the runtime lacks) runs as the original's code on the bench (unported.c,
     TC_HYBRID);
  4. the two results are compared: memory but the stack below the caller's frame (the frames, the
     return address and the arguments, which a callee may change, are laid out differently by the C)
     and what the overlay manager wrote in the original's run (the C calls an overlay function
     directly, the original has the manager load it): the bytes its code wrote (CS its segment, the one
     of the vector at ds:FC08) and those it read from EOB.EXE (an overlay's code); the return
     registers the function's type says it sets (AL, AX or DX:AX); and the screen, the sound devices
     and the files.

A call is "same", "differs" (with the bytes), "C stopped" (with why: the time limit, a routine run
for the C that did not return), or not tested: the original did not return on the bench (it waits
for memory only an interrupt handler changes, such as the tick count or the sound driver's busy
countdown; it never returns, as Main). The first --samples tested calls of each function count; a
function the original does not return from twice in a session is left for that session. Interrupts
come to the bench with the instruction clock of M3.

  hybrid.py [--session NAME ...] [--function NAME ...] [--samples N] [--out build/port/hybrid] [--lib SO]
"""
from __future__ import annotations

import argparse
import copy
import ctypes
import hashlib
import json
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "match"))

from oracle import run  # noqa: E402
from oracle.machine import LOAD_SEG, REG, Stop, OracleError  # noqa: E402
from oracle.input import Script  # noqa: E402
from oracle import golden  # noqa: E402
from unicorn import UC_PROT_ALL, UC_HOOK_MEM_WRITE, UcError  # noqa: E402
from unicorn.x86_const import UC_X86_REG_CS  # noqa: E402

import cmatch  # noqa: E402  (read-only: the `/*@ */` annotations, for the names of globals)

LIB = ROOT / "build" / "port" / "hybrid" / "libeobhybrid.so"
RAM = 0xA0000
DGROUP = LOAD_SEG + 0x212E
SENTINEL = (0xF000, 0x3E00)       # a NOP in the oracle's ROM area: the bench's calls return here
SAVED_INT3F = 0xFC08              # ds: the overlay manager's INT 3Fh vector, which the game's own handler
                                  # (1A72:0006, src/E1A72.ASM) chains to: its segment is the manager's code
BUDGET = 20_000_000               # instructions the original may take on the bench
LIMIT_MS = 5000                   # CPU time the C may take
MAX_WAITS = 2                     # calls of a function the original does not return from, then it is left
# Machine attributes that are its wiring, not its state: the bench keeps its own
WIRING = {"uc", "ram", "services", "port_handlers", "breakpoints", "routine_hooks", "watch_hooks",
          "overlay_listeners", "file_listeners", "tracer", "canonical", "keys"}


class Entry(ctypes.Structure):
    _fields_ = [("seg", ctypes.c_uint16), ("off", ctypes.c_uint16), ("overlay", ctypes.c_int16),
                ("code_off", ctypes.c_uint16), ("name", ctypes.c_char_p), ("unit", ctypes.c_char_p),
                ("thunk", ctypes.c_void_p), ("ret_bytes", ctypes.c_uint8), ("arg_bytes", ctypes.c_uint8)]


ORACLE_FN = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint16)


def components(m) -> list:
    """The machine and the oracle's objects it holds (the EGA, DOS, the VFS, the BIOS, the devices),
    in a fixed order, so two machines built alike pair up."""
    out, seen = [m], {id(m)}
    i = 0
    while i < len(out):
        for k, v in vars(out[i]).items():
            if out[i] is m and k in WIRING:
                continue
            if id(v) not in seen and hasattr(v, "__dict__") and not isinstance(v, type) \
                    and type(v).__module__.startswith("oracle."):
                seen.add(id(v))
                out.append(v)
        i += 1
    return out


def globals_by_offset() -> dict[int, str]:
    out = {}
    for p in sorted((ROOT / "src").glob("E*.C")):
        _f, data, _d, _s = cmatch.parse_annotations(p.read_text(encoding="latin-1"))
        for name, off in data.items():
            out.setdefault(off, name)
    return out


class Bench:
    """The second machine: the shared library's arena is its memory."""

    def __init__(self, lib, build):
        self.lib = lib
        self.m = m = build()
        base = ctypes.addressof(ctypes.c_uint8.in_dll(lib, "tc_mem"))
        data = ctypes.string_at(ctypes.addressof(m.ram), RAM)
        m.uc.mem_unmap(0, RAM)
        m.uc.mem_map_ptr(0, RAM, UC_PROT_ALL, base)
        ctypes.memmove(base, data, RAM)
        m.ram = (ctypes.c_char * RAM).from_address(base)
        self.base = base
        m.uc.mem_write(m.linear(*SENTINEL), b"\x90")
        self.comps = components(m)
        self.tc_ss = ctypes.c_uint16.in_dll(lib, "tc_ss")
        self.tc_sp = ctypes.c_uint16.in_dll(lib, "tc_sp")
        self.tc_ds = ctypes.c_uint16.in_dll(lib, "tc_ds")
        self.failed = ctypes.c_int.in_dll(lib, "tc_hybrid_oracle_failed")
        self.message = (ctypes.c_char * 512).in_dll(lib, "tc_hybrid_message")
        self.callback = ORACLE_FN(self.oracle)
        ctypes.c_void_p.in_dll(lib, "tc_hybrid_oracle").value = ctypes.cast(self.callback, ctypes.c_void_p).value
        self.oracle_calls = 0
        self.oracle_error = ""
        self.written: set | None = None      # what the overlay manager wrote, while watched
        self.ovr_seg = 0
        m.file_listeners.append(self.on_file)

    # -- what the overlay manager writes (the C calls an overlay function directly: none of it)
    def watch(self):
        self.written = set()
        self.ovr_seg = self.m.rw(DGROUP, SAVED_INT3F + 2)
        self.hook = self.m.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write, begin=0, end=RAM - 1)

    def unwatch(self) -> set:
        self.m.uc.hook_del(self.hook)
        w, self.written = self.written, None
        return w

    def on_write(self, uc, access, addr, size, value, ud):
        if uc.reg_read(UC_X86_REG_CS) == self.ovr_seg:
            self.written.update(range(addr, addr + size))

    def on_file(self, m, op, path, pos, n):
        # the manager reads an overlay's code and fixups from the program's own file
        if self.written is not None and op == "read" and path.upper().endswith("EOB.EXE"):
            lin = m.linear(m.get("ds"), m.get("dx"))
            self.written.update(range(lin, lin + n))

    # -- state
    def take(self, src, pairs, memo0):
        """The bench takes `src`'s state: memory, registers, and the oracle's Python state."""
        m = self.m
        ctypes.memmove(self.base, ctypes.addressof(src.ram), RAM)
        m.uc.ctl_remove_cache(0, RAM)
        for r in REG:
            m.set(r, src.get(r))
        memo = dict(memo0)
        for a, b in pairs:
            for k, v in vars(a).items():
                if a is src and k in WIRING:
                    continue
                b.__dict__[k] = copy.deepcopy(v, memo)

    def memory(self) -> bytes:
        return ctypes.string_at(self.base, RAM)

    def outside(self) -> dict:
        m = self.m
        h = hashlib.sha256()
        for path in sorted(m.vfs.layer):
            h.update(path.encode() + b"\0" + bytes(m.vfs.layer[path]))
        return {"screen": m.ega.digest_bytes().hex(), "adlib": len(getattr(m, "adlib_log", [])),
                "adlib_tail": repr(getattr(m, "adlib_log", [])[-64:]), "speaker": repr(m.speaker_log[-16:]),
                "files": h.hexdigest(), "deleted": sorted(m.vfs.deleted)}

    # -- running the original on the bench
    def until(self, seg, off, sp_after, budget=BUDGET) -> str:
        """Run from CS:IP until seg:off is reached with SP at sp_after; '' or why not."""
        m = self.m
        try:
            m.uc.emu_start(m.linear(m.get("cs"), m.get("ip")), m.linear(seg, off), count=budget)
        except Stop as s:
            return "stopped: %s" % s.reason
        except OracleError as e:
            return "oracle: %s" % str(e).splitlines()[0]
        except UcError as e:
            return "guest fault: %s at %04X:%04X" % (e, m.get("cs"), m.get("ip"))
        if (m.get("cs"), m.get("ip")) != (seg, off):
            return "did not return in %d instructions (at %04X:%04X%s)" % (
                budget, (m.get("cs") - LOAD_SEG) & 0xFFFF, m.get("ip"),
                ", halted: waits for an interrupt" if m.uc.mem_read(m.linear(m.get("cs"), m.get("ip") - 1), 1) == b"\xF4" else "")
        if m.get("sp") != sp_after:
            return "returned with SP %04X, not %04X" % (m.get("sp"), sp_after)
        return ""

    def oracle(self, entry, args, n):
        """The C calls a routine it has no C for: run the original's on the bench."""
        m = self.m
        try:
            self.oracle_calls += 1
            ss, sp = self.tc_ss.value, self.tc_sp.value
            sp = (sp - n) & 0xFFFF
            m.write(ss, sp, ctypes.string_at(args, n) if n else b"")
            sp = (sp - 4) & 0xFFFF
            m.ww(ss, sp, SENTINEL[1])
            m.ww(ss, sp + 2, SENTINEL[0])
            m.set("ss", ss)
            m.set("sp", sp)
            m.set("ds", self.tc_ds.value)
            m.set("es", self.tc_ds.value)
            m.set("cs", entry >> 16)
            m.set("ip", entry & 0xFFFF)
            why = self.until(SENTINEL[0], SENTINEL[1], (sp + 4) & 0xFFFF)
            if why:
                self.oracle_error = "%04X:%04X %s" % (((entry >> 16) - LOAD_SEG) & 0xFFFF, entry & 0xFFFF, why)
                self.message.value = self.oracle_error.encode()[:500]
                self.failed.value = 1
                return 0
            return (m.get("dx") << 16) | m.get("ax")
        except Exception as e:  # noqa: BLE001  (a Python error must not unwind through C)
            self.oracle_error = "the bench failed: %r" % e
            self.message.value = self.oracle_error.encode()[:500]
            self.failed.value = 1
            return 0


class Tester:
    def __init__(self, samples: int, only: set[str] | None, lib: Path = LIB):
        self.lib = ctypes.CDLL(str(lib))
        n = ctypes.c_uint.in_dll(self.lib, "tc_hybrid_entry_count").value
        self.entries = (Entry * n).in_dll(self.lib, "tc_hybrid_entries")
        self.lib.tc_hybrid_init.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
        self.lib.tc_hybrid_run.argtypes = [ctypes.c_uint, ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint16,
                                           ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint]
        self.lib.tc_hybrid_run.restype = ctypes.c_int
        self.lib.tc_hybrid_init(LOAD_SEG, DGROUP)
        self.samples = samples
        self.only = only
        self.names = globals_by_offset()
        self.results: list[dict] = []
        self.busy = False

    # -- the session
    def play(self, name: str) -> dict:
        text = (golden.SESSIONS / ("%s.keys" % name)).read_text()
        opts = Script(text).options
        build = run.build_machine
        state = {}

        def build_and_hook(*a, **k):
            m = build(*a, **k)
            state["session"] = m
            state["bench"] = Bench(self.lib, lambda: build(*a, **k))
            self.hook(m, state["bench"], name)
            return m
        run.build_machine = build_and_hook
        save = golden.SESSIONS / opts["save"] if "save" in opts else None
        try:
            with tempfile.TemporaryDirectory() as tmp:
                s = run.run_session(text, out=Path(tmp), max_insns=int(opts.get("insns", 60_000_000)),
                                    mouse="mouse" in opts, sound=opts.get("sound"), save=save,
                                    program=opts.get("program", "EOB.EXE"))
        finally:
            run.build_machine = build
        g = {k: s[k] for k in golden.KEEP}
        g["checkpoints"] = [{k: c[k] for k in ("kind", "name", "instructions", "digest", "image") if k in c}
                            for c in s["checkpoints"]]
        g["args"] = s["args"]
        want = json.loads((golden.GOLDEN / ("%s.json" % name)).read_text())
        return {"session": name, "stop": s["stop"], "golden": golden.differences(want, g)}

    def hook(self, m, bench, session):
        counts = {}           # calls tested, by entry
        waits = {}            # calls the original did not return from on the bench
        handles = {}
        for i, e in enumerate(self.entries):
            if e.seg == 0xFFFF:
                continue
            nm = e.name.decode()
            if self.only and nm not in self.only:
                continue
            lin = m.linear(LOAD_SEG + e.seg, e.off)
            if lin in handles:
                continue

            def at_entry(mach, i=i, lin=lin):
                if self.busy:
                    return
                c = counts.get(i, 0)
                if c >= self.samples or waits.get(i, 0) >= MAX_WAITS:
                    return
                self.busy = True
                try:
                    r = self.test(mach, bench, i, session, c)
                finally:
                    self.busy = False
                self.results.append(r)
                if r["status"].startswith("not tested"):
                    waits[i] = waits.get(i, 0) + 1
                else:
                    counts[i] = c + 1
            m.add_breakpoint(LOAD_SEG + e.seg, e.off, at_entry)
            handles[lin] = i
        pairs = list(zip(components(m), bench.comps))
        if [type(a) for a, _ in pairs] != [type(b) for _, b in pairs]:
            raise SystemExit("hybrid: the bench is not built like the session's machine")
        self.pairs = pairs
        self.memo0 = {id(a): b for a, b in pairs}
        self.memo0[id(m.uc)] = bench.m.uc

    # -- one call
    def test(self, m, bench, i, session, sample) -> dict:
        e = self.entries[i]
        r = {"function": e.name.decode(), "unit": e.unit.decode(), "session": session, "sample": sample,
             "at": m.insns}
        ss, sp, ds = m.get("ss"), m.get("sp"), m.get("ds")
        ret_ip, ret_cs = m.rw(ss, sp), m.rw(ss, sp + 2)
        if ds != DGROUP:
            r["status"] = "not tested: DS is %04X at the entry" % ds
            return r
        t0 = time.monotonic()
        # 2. the original
        bench.take(m, self.pairs, self.memo0)
        bench.watch()
        why = bench.until(ret_cs, ret_ip, (sp + 4) & 0xFFFF)
        manager = bench.unwatch()
        if why:
            r["status"] = "not tested: the original %s" % why
            return r
        bm = bench.m
        o_mem, o_out = bench.memory(), bench.outside()
        o_regs = (bm.get("ax"), bm.get("dx"))
        # 3. the C
        bench.take(m, self.pairs, self.memo0)
        bench.oracle_calls = 0
        bench.oracle_error = ""
        val = ctypes.c_uint32(0)
        code = self.lib.tc_hybrid_run(i, ss, sp, ds, ctypes.byref(val), LIMIT_MS)
        r["oracle_calls"] = bench.oracle_calls
        r["seconds"] = round(time.monotonic() - t0, 3)
        if code:
            r["status"] = "C stopped"
            r["why"] = bench.message.value.decode("latin-1")
            return r
        c_mem, c_out = bench.memory(), bench.outside()
        # 4. compare
        lo = ss * 16
        masks = [(lo, lo + ((sp + 4 + e.arg_bytes) & 0xFFFF))]
        spans = diff(o_mem, c_mem, masks, manager)
        if manager:
            r["masked_overlay_manager"] = len(manager)
        regs = []
        rb = e.ret_bytes
        if rb:
            mask = 0xFF if rb == 1 else (0xFFFF if rb == 2 else 0xFFFFFFFF)
            ov = (o_regs[1] << 16 | o_regs[0]) & mask
            cv = val.value & mask
            if ov != cv:
                regs.append("returns %0*X, the original %0*X" % (2 * rb, cv, 2 * rb, ov))
        outs = ["%s differs" % k for k in o_out if o_out[k] != c_out[k]]
        if not spans and not regs and not outs:
            r["status"] = "same"
        else:
            r["status"] = "differs"
            r["registers"] = regs
            r["outside"] = outs
            r["memory"] = [self.describe(a, b, o_mem, c_mem) for a, b in spans[:12]]
            r["memory_spans"] = len(spans)
        return r

    def describe(self, a, b, o_mem, c_mem) -> str:
        where = "%05X" % a
        d = a - DGROUP * 16
        if 0 <= d < 0x10000:
            name = self.names.get(d)
            if name is None:
                near = [k for k in self.names if k <= d]
                if near:
                    k = max(near)
                    name = "%s+%X" % (self.names[k], d - k)
            where = "ds:%04X%s" % (d, " (%s)" % name if name else "")
        n = b - a
        return "%s, %d bytes: original %s, C %s" % (where, n, o_mem[a:b][:16].hex(" "), c_mem[a:b][:16].hex(" "))


class Direct:
    """One routine called with chosen arguments, on the bench: the original, then the C from the same
    state, compared as a golden's calls are. For the routines the goldens do not reach (an unpacker no
    pinned file uses, the encoder the game runs when it saves); tests/unit/test_port.py feeds them.
    The bench is the program as loaded, not started. After a call the bench keeps the original's
    result, so a sequence of calls (a random generator's) goes on from the original's state."""

    STACK = (0x9000, 0xFFF0)          # SS:SP of the calls: conventional memory the loaded program leaves free

    def __init__(self, lib: Path = LIB):
        self.t = Tester(1, None, lib)
        self.bench = Bench(self.t.lib, run.build_machine)
        self.m = self.bench.m
        self.index = {e.name.decode(): i for i, e in enumerate(self.t.entries)}

    def write(self, seg: int, off: int, data: bytes) -> None:
        self.m.write(seg, off, data)

    def read(self, seg: int, off: int, n: int) -> bytes:
        return bytes(self.m.read(seg, off, n))

    def call(self, name: str, *args) -> dict:
        """args: ints (a word each) or (value, 4) for a long or a far pointer. Returns {"status":
        "same" | "differs" | "C stopped" | "not tested: ...", "value": the original's DX:AX, ...}."""
        m, bench, i = self.m, self.bench, self.index[name]
        e = self.t.entries[i]
        block = b""
        for a in args:
            v, w = a if isinstance(a, tuple) else (a, 2)
            block += (v & (0xFFFFFFFF if w == 4 else 0xFFFF)).to_bytes(w, "little")
        ss, top = self.STACK
        sp = (top - len(block) - 4) & 0xFFFF
        m.write(ss, sp + 4, block)
        m.ww(ss, sp, SENTINEL[1])
        m.ww(ss, sp + 2, SENTINEL[0])
        for r, v in (("ss", ss), ("sp", sp), ("ds", DGROUP), ("es", DGROUP), ("cs", LOAD_SEG + e.seg), ("ip", e.off)):
            m.set(r, v)
        before = bench.memory()
        why = bench.until(SENTINEL[0], SENTINEL[1], (sp + 4) & 0xFFFF)
        r = {"function": name}
        if why:
            r["status"] = "not tested: the original %s" % why
            return r
        o_mem, o_regs = bench.memory(), (m.get("ax"), m.get("dx"))
        r["value"] = (o_regs[1] << 16) | o_regs[0]
        ctypes.memmove(bench.base, before, RAM)
        m.uc.ctl_remove_cache(0, RAM)
        val = ctypes.c_uint32(0)
        code = self.t.lib.tc_hybrid_run(i, ss, sp, DGROUP, ctypes.byref(val), LIMIT_MS)
        c_mem = bench.memory()
        ctypes.memmove(bench.base, o_mem, RAM)          # go on from the original's state
        m.uc.ctl_remove_cache(0, RAM)
        if code:
            r["status"] = "C stopped"
            r["why"] = bench.message.value.decode("latin-1")
            return r
        spans = diff(o_mem, c_mem, [(ss * 16, ss * 16 + top)])
        regs = []
        if e.ret_bytes:
            mask = (1 << (8 * e.ret_bytes)) - 1
            if r["value"] & mask != val.value & mask:
                regs.append("returns %X, the original %X" % (val.value & mask, r["value"] & mask))
        r["status"] = "same" if not spans and not regs else "differs"
        if r["status"] == "differs":
            r["registers"] = regs
            r["memory"] = [self.t.describe(a, b, o_mem, c_mem) for a, b in spans[:8]]
        return r


class Regs(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in ("ax", "bx", "cx", "dx", "si", "di", "bp", "ds", "es", "flags")]


class Video(ctypes.Structure):
    _fields_ = [("mode", ctypes.c_uint8), ("page", ctypes.c_uint8), ("dac", ctypes.c_uint8 * 768)]


class Hw(ctypes.Structure):
    u8, u16, u32 = ctypes.c_uint8, ctypes.c_uint16, ctypes.c_uint32
    _fields_ = [("pic_mask", u8), ("port61", u8), ("irq0_in_service", u8), ("irq1_in_service", u8),
                ("kbd_data", ctypes.c_int16), ("pit_reload", u32), ("pit_latch_lo", u8), ("pit_mode_access", u8),
                ("pit_lo", u8), ("pit2_lo", u8), ("pit2_reload", u16), ("next_irq0", u32), ("dac_read", u16),
                ("dac_write", u16), ("attr_ff", u8), ("adlib", u8), ("adlib_index", u8), ("adlib_flags", u8),
                ("adlib_regs", u8 * 256), ("adlib_running", u8 * 2), ("adlib_started", u32 * 2)]


class MouseState(ctypes.Structure):
    u16 = ctypes.c_uint16
    _fields_ = [("x", u16), ("y", u16), ("buttons", u16), ("visible", u16), ("range_x0", u16), ("range_x1", u16),
                ("range_y0", u16), ("range_y1", u16), ("has_handler", ctypes.c_uint8), ("handler_seg", u16),
                ("handler_off", u16), ("handler_mask", u16)]


class Clock(ctypes.Structure):
    _fields_ = [("insns", ctypes.c_uint32), ("extra", ctypes.c_uint32), ("insn_per_sec", ctypes.c_uint32), ("pinned", ctypes.c_int),
                ("clock_pin", ctypes.c_uint32), ("tod_offset_set", ctypes.c_int), ("tod_offset", ctypes.c_int32)]


FILE_SIZE = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_char_p)
FILE_READ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32)
SHADOWED = (0x10, 0x11, 0x12, 0x13, 0x1A, 0x20, 0x21, 0x29, 0x2F, 0x33)
REG_NAMES = ("ax", "bx", "cx", "dx", "si", "di", "bp", "ds", "es")


class Shadow:
    """The machine's proof (M3, --shadow): a golden plays on the oracle, and every call of a service
    the machine has (DOS, the BIOS's 10h, 11h, 12h, 13h, 1Ah, INT 33h with no mouse) is made to the machine
    too, just before the oracle's own service answers it: the machine runs on a copy of the oracle's
    memory (the library's arena) with the same registers and the flags the INT pushed, then the two
    answers are compared: the registers, the flags returned (CF), memory but the flags word of the INT
    frame, which the oracle's services edit in place, and the video: the mode, the DAC and mode 13h's
    64 KB at A000:0000 (the oracle's EGA model keeps them; the machine's MCGA, video.c). The machine's own state (its files, the
    memory arena, the clock's offset) lives on from call to call, as the oracle's does, so a
    difference in it shows at the first call it changes. The files are what the session's VFS had
    when the first call came (the pinned files, the saved game the session names)."""

    def __init__(self, lib: Path = LIB):
        self.lib = ctypes.CDLL(str(lib))
        self.lib.tc_hybrid_init.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
        self.lib.tc_hybrid_init(LOAD_SEG, DGROUP)
        self.lib.tc_hybrid_service.argtypes = [ctypes.c_uint8, ctypes.POINTER(Regs)]
        self.lib.tc_hybrid_machine_reset.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint16, ctypes.c_int,
                                                     ctypes.c_int]
        self.lib.tc_hybrid_in.argtypes = [ctypes.c_uint16, ctypes.c_int, ctypes.POINTER(ctypes.c_uint16)]
        self.lib.tc_hybrid_out.argtypes = [ctypes.c_uint16, ctypes.c_int, ctypes.c_uint16]
        self.hw = Hw.in_dll(self.lib, "hw")
        self.mouse = MouseState.in_dll(self.lib, "mouse")
        self.adlib_count = ctypes.c_uint32.in_dll(self.lib, "tc_hybrid_adlib_count")
        self.adlib_last = (ctypes.c_uint8 * 2).in_dll(self.lib, "tc_hybrid_adlib_last")
        self.speaker_count = ctypes.c_uint32.in_dll(self.lib, "tc_hybrid_speaker_count")
        self.speaker = (ctypes.c_uint16.in_dll(self.lib, "tc_hybrid_speaker_divisor"),
                        ctypes.c_uint8.in_dll(self.lib, "tc_hybrid_speaker_bits"))
        self.base = ctypes.addressof(ctypes.c_uint8.in_dll(self.lib, "tc_mem"))
        self.message = (ctypes.c_char * 512).in_dll(self.lib, "tc_hybrid_message")
        self.clock = Clock.in_dll(self.lib, "machine_time")
        self.video = Video.in_dll(self.lib, "video")
        self.size_cb, self.read_cb = FILE_SIZE(self.file_size), FILE_READ(self.file_read)
        ctypes.c_void_p.in_dll(self.lib, "tc_hybrid_file_size").value = ctypes.cast(self.size_cb, ctypes.c_void_p).value
        ctypes.c_void_p.in_dll(self.lib, "tc_hybrid_file_read").value = ctypes.cast(self.read_cb, ctypes.c_void_p).value
        self.calls: dict[str, dict] = {}
        self.differences: list[dict] = []
        self.vfs = None

    # -- the files, as the session's VFS had them when the first call came
    def lookup(self, name: bytes):
        v = self.vfs
        full = v.mount + "\\" + name.decode("latin-1").upper()
        if full in v.deleted:
            return None
        if full in v.layer:
            return bytes(v.layer[full])
        if full in v.host_index:
            return v.host_index[full].read_bytes()
        return None

    def file_size(self, name):
        d = self.lookup(name)
        return -1 if d is None else len(d)

    def file_read(self, name, buf, size):
        d = self.lookup(name)
        if d is None or len(d) != size:
            return -1
        ctypes.memmove(buf, d, size)
        return 0

    def attach(self, m, session: str):
        self.session = session
        self.vfs = None
        self.m = m
        for n in SHADOWED:
            fn = m.services.get(n)
            if fn is not None:
                m.services[n] = (lambda mach, n=n, fn=fn: self.call(mach, n, fn))
        rd, wr = m.port_read, m.port_write
        m.port_read = lambda port, size: self.port(m, "in", port, size, None, rd)
        m.port_write = lambda port, size, val: self.port(m, "out", port, size, val, wr)

    def key(self, m, n) -> str:
        ah = m.get("ax") >> 8
        return "INT %02Xh" % n + (" AH=%02Xh" % ah if n in (0x10, 0x21, 0x1A, 0x13) else "")

    def start(self, m):
        """The first call of the session: the machine boots as the session's machine was built."""
        if self.vfs is None:
            self.vfs = copy.deepcopy(m.vfs)
            cmd = m.read(0x0FF0, 0x81, m.rb(0x0FF0, 0x80))
            code = self.lib.tc_hybrid_machine_reset(m.exe.name.upper().encode(), cmd, len(cmd),
                                                    m.mouse is not None, hasattr(m, "adlib_log"))
            if code:
                raise SystemExit("hybrid: the machine does not boot: %s" % self.message.value.decode())

    def set_clock(self, m):
        self.clock.insns = m.insns & 0xFFFFFFFF
        self.clock.extra = m.extra & 0xFFFFFFFF
        self.clock.insn_per_sec = m.insn_per_sec
        self.clock.pinned = m.clock_pin is not None
        self.clock.clock_pin = m.clock_pin or 0

    def state(self, m) -> list[str]:
        """The hardware and driver state the machine keeps, against the oracle's."""
        hw, out = self.hw, []
        want = {"pic_mask": m.pic_mask, "port61": m.port61, "irq0_in_service": int(m.irq0_in_service),
                "irq1_in_service": int(m.irq1_in_service), "pit_reload": m.pit_reload,
                "pit_latch_lo": int(m.pit_latch_lo), "pit_mode_access": m.pit_mode_access,
                "pit2_reload": m.pit2_reload, "pit2_lo": int(m.pit2_lo), "next_irq0": m.next_irq0,
                "dac_read": m.ega.dac_read, "dac_write": m.ega.dac_write, "attr_ff": int(m.ega.attr_ff)}
        for k, v in want.items():
            if getattr(hw, k) != v:
                out.append("%s %X, the oracle %X" % (k, getattr(hw, k), v))
        if self.clock.extra != m.extra & 0xFFFFFFFF:
            out.append("the clock's charges %d, the oracle %d" % (self.clock.extra, m.extra))
        if bytes(self.video.dac) != bytes(m.ega.dac):
            out.append("the DAC differs")
        ad = getattr(m, "adlib_log", None)
        if ad is not None:
            o = m.port_handlers[0x388][0].__self__
            if bytes(hw.adlib_regs) != bytes(o.regs) or hw.adlib_flags != o.flags or hw.adlib_index != o.index:
                out.append("the AdLib's registers or status differ")
            for t in range(2):
                started = None if not hw.adlib_running[t] else hw.adlib_started[t]
                theirs = None if o.started[t] is None else round(o.started[t] * m.insn_per_sec)
                if started != theirs:
                    out.append("AdLib timer %d started at %s, the oracle %s" % (t + 1, started, theirs))
        mo = m.mouse
        if mo is not None:
            mine = (self.mouse.x, self.mouse.y, self.mouse.buttons, self.mouse.visible & 0xFFFF,
                    (self.mouse.range_x0, self.mouse.range_x1), (self.mouse.range_y0, self.mouse.range_y1),
                    (self.mouse.handler_seg, self.mouse.handler_off, self.mouse.handler_mask) if self.mouse.has_handler else None)
            theirs = (mo.x, mo.y, mo.buttons, mo.visible & 0xFFFF, tuple(mo.range_x), tuple(mo.range_y),
                      tuple(mo.handler) if mo.handler else None)
            if mine != theirs:
                out.append("the mouse driver %r, the oracle %r" % (mine, theirs))
        return out

    def sync(self, m):
        """What the interrupts and the host's input change, which the shadow does not run yet: the
        oracle's, before a call."""
        self.hw.irq0_in_service = int(m.irq0_in_service)
        self.hw.irq1_in_service = int(m.irq1_in_service)
        self.hw.kbd_data = -1 if m.kbd_data is None else m.kbd_data
        self.hw.next_irq0 = m.next_irq0 & 0xFFFFFFFF
        if m.mouse is not None:              # the host's moves and clicks (input.py Mouse.move, buttons_to)
            self.mouse.x, self.mouse.y, self.mouse.buttons = m.mouse.x, m.mouse.y, m.mouse.buttons

    def port(self, m, d, port, size, val, fn):
        self.start(m)
        self.sync(m)
        self.set_clock(m)
        key = "%s %Xh" % (d, port)
        ad0, sp0 = self.adlib_count.value, self.speaker_count.value
        ad_len = len(getattr(m, "adlib_log", []))
        sp_len = len(m.speaker_log)
        got = ctypes.c_uint16(0)
        if d == "in":
            code = self.lib.tc_hybrid_in(port, size, ctypes.byref(got))
        else:
            code = self.lib.tc_hybrid_out(port, size, val & 0xFFFF)
        why = self.message.value.decode("latin-1")
        res = fn(port, size) if d == "in" else fn(port, size, val)
        st = self.calls.setdefault(key, {"same": 0, "differs": 0, "machine lacks": 0})
        out = []
        if code == 74:
            st["machine lacks"] += 1
            out.append("the machine: %s" % why)
        else:
            if d == "in" and got.value != (res & 0xFFFF):
                out.append("reads %X, the oracle %X" % (got.value, res))
            out += self.state(m)
            ad = getattr(m, "adlib_log", [])
            if (self.adlib_count.value - ad0) != (len(ad) - ad_len):
                out.append("%d AdLib writes, the oracle %d" % (self.adlib_count.value - ad0, len(ad) - ad_len))
            elif len(ad) > ad_len and tuple(self.adlib_last) != tuple(ad[-1][1:]):
                out.append("AdLib %r, the oracle %r" % (tuple(self.adlib_last), ad[-1][1:]))
            if (self.speaker_count.value - sp0) != (len(m.speaker_log) - sp_len):
                out.append("%d speaker changes, the oracle %d" % (self.speaker_count.value - sp0, len(m.speaker_log) - sp_len))
            elif len(m.speaker_log) > sp_len:
                div, bits = self.speaker[0].value, self.speaker[1].value
                on = m.speaker_log[-1][2]
                # the oracle's port 42h entry tests port61 & 1 (`port61 & 3 == 3` binds as port61 & True)
                mine_on = (bits == 3) if port == 0x61 else bool(bits & 1)
                if div != m.speaker_log[-1][1] or mine_on != on:
                    out.append("speaker (%X, %s), the oracle %r" % (div, mine_on, m.speaker_log[-1][1:]))
            st["differs" if out else "same"] += 1
        if out and len(self.differences) < 50:
            self.differences.append({"session": self.session, "call": key, "at": m.insns, "what": out})
        return res

    def call(self, m, n, fn):
        self.start(m)
        self.sync(m)
        key = self.key(m, n)
        ss, sp = m.get("ss"), m.get("sp")
        frame = m.linear(ss, (sp + 4) & 0xFFFF)
        r = Regs(*[m.get(x) for x in REG_NAMES], m.rw(ss, (sp + 4) & 0xFFFF))
        ctypes.memmove(self.base, ctypes.addressof(m.ram), RAM)
        ctypes.memmove(self.base + 0xA0000, bytes(m.ega.vga), 0x10000)    # mode 13h's memory
        self.set_clock(m)
        code = self.lib.tc_hybrid_service(n, ctypes.byref(r))
        why = self.message.value.decode("latin-1")
        raised = None
        try:
            fn(m)
        except Exception as e:  # noqa: BLE001  (compared, then raised again)
            raised = e
        st = self.calls.setdefault(key, {"same": 0, "differs": 0, "machine lacks": 0})
        o_reason = None if raised is None else (
            "exit" if isinstance(raised, Stop) and raised.reason == "exit" else type(raised).__name__)
        c_reason = {0: None, 76: "exit"}.get(code, "stopped")
        out = []
        if code == 74:
            st["machine lacks"] += 1
            out.append("the machine: %s" % why)
        elif o_reason != c_reason:
            out.append("the oracle %s, the machine %s" % (o_reason or "returned", why or "returned"))
        elif raised is None:
            for x in REG_NAMES:
                if getattr(r, x) != m.get(x):
                    out.append("%s %04X, the oracle %04X" % (x.upper(), getattr(r, x), m.get(x)))
            of = m.rw(ss, (sp + 4) & 0xFFFF)
            if (r.flags ^ of) & 0x0FD5:
                out.append("flags %04X, the oracle %04X" % (r.flags, of))
            out += self.state(m)
            if self.video.mode != m.ega.mode:
                out.append("video mode %02Xh, the oracle %02Xh" % (self.video.mode, m.ega.mode))
            if ctypes.string_at(self.base + 0xA0000, 0x10000) != bytes(m.ega.vga):
                out.append("A000 differs")
            a, b = bytes(m.ram), ctypes.string_at(self.base, RAM)
            if a != b:
                for x, y in diff(a, b, [(frame, frame + 2)])[:6]:
                    out.append("%05X: the oracle %s, the machine %s" % (x, a[x:y][:12].hex(" "), b[x:y][:12].hex(" ")))
        if code != 74:
            st["differs" if out else "same"] += 1
        if out and len(self.differences) < 50:
            self.differences.append({"session": self.session, "call": key, "at": m.insns, "what": out})
        if raised is not None:
            raise raised


def run_shadow(sessions, lib: Path, out: Path) -> int:
    sh = Shadow(lib)
    played = []
    for name in sessions:
        text = (golden.SESSIONS / ("%s.keys" % name)).read_text()
        opts = Script(text).options
        build = run.build_machine

        def build_and_attach(*a, **k):
            m = build(*a, **k)
            sh.attach(m, name)
            return m
        run.build_machine = build_and_attach
        save = golden.SESSIONS / opts["save"] if "save" in opts else None
        t0 = time.monotonic()
        try:
            with tempfile.TemporaryDirectory() as tmp:
                s = run.run_session(text, out=Path(tmp), max_insns=int(opts.get("insns", 60_000_000)),
                                    mouse="mouse" in opts, sound=opts.get("sound"), save=save,
                                    program=opts.get("program", "EOB.EXE"))
        finally:
            run.build_machine = build
        played.append({"session": name, "stop": s["stop"], "seconds": round(time.monotonic() - t0, 1)})
        print("%s: %s, %.0f s" % (name, s["stop"], played[-1]["seconds"]))
    out.mkdir(parents=True, exist_ok=True)
    report = out / "shadow.json"
    report.write_text(json.dumps({"sessions": played, "calls": sh.calls, "differences": sh.differences}, indent=1) + "\n")
    same = sum(c["same"] for c in sh.calls.values())
    differs = sum(c["differs"] for c in sh.calls.values())
    lacks = {k: c["machine lacks"] for k, c in sh.calls.items() if c["machine lacks"]}
    print("shadow: %d service calls the same as the oracle's, %d differ; the machine lacks %s (report: %s)" % (
        same, differs, ", ".join("%s (%d)" % kv for kv in sorted(lacks.items())) or "none", report))
    for d in sh.differences[:12]:
        print("  %s, %s at %d: %s" % (d["session"], d["call"], d["at"], "; ".join(d["what"][:4])))
    return 1 if differs or lacks else 0


def diff(a: bytes, b: bytes, masks, skip=frozenset()) -> list[tuple[int, int]]:
    """The spans [start, end) where a and b differ, outside the masks and the bytes in skip."""
    spans = []
    page = 4096
    for p in range(0, len(a), page):
        if a[p:p + page] == b[p:p + page]:
            continue
        for k in range(p, min(p + page, len(a))):
            if a[k] != b[k] and k not in skip and not any(x <= k < y for x, y in masks):
                if spans and k - spans[-1][1] <= 4:
                    spans[-1][1] = k + 1
                else:
                    spans.append([k, k + 1])
    return [tuple(s) for s in spans]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--session", action="append", help="a golden of EOB.EXE (default: all of them)")
    ap.add_argument("--function", action="append", help="test only these functions")
    ap.add_argument("--samples", type=int, default=3, help="calls tested per function and session")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "port" / "hybrid")
    ap.add_argument("--lib", type=Path, default=LIB, help="the hybrid library (default: make port-hybrid's)")
    ap.add_argument("--shadow", action="store_true", help="the machine's test: every service call against the oracle's")
    a = ap.parse_args(argv)
    if not a.lib.exists():
        print("hybrid: %s is missing (make port-hybrid)" % a.lib)
        return 1
    sessions = a.session or sorted(p.stem for p in golden.SESSIONS.glob("*.keys")
                                   if "program INTRO.EXE" not in p.read_text())
    if a.shadow:
        return run_shadow(sessions, a.lib, a.out)
    t = Tester(a.samples, set(a.function) if a.function else None, a.lib)
    played = []
    for s in sessions:
        t0 = time.monotonic()
        p = t.play(s)
        p["seconds"] = round(time.monotonic() - t0, 1)
        played.append(p)
        print("%s: %d calls tested in %.0f s; the session %s" % (
            s, sum(1 for r in t.results if r["session"] == s), p["seconds"],
            "is still its golden" if not p["golden"] else "DIFFERS from its golden: " + "; ".join(p["golden"])))
    by_fn: dict[str, dict] = {}
    for r in t.results:
        f = by_fn.setdefault(r["function"], {"unit": r["unit"], "same": 0, "differs": 0, "stopped": 0, "untested": 0})
        f[{"same": "same", "differs": "differs", "C stopped": "stopped"}.get(r["status"], "untested")] += 1
    a.out.mkdir(parents=True, exist_ok=True)
    report = a.out / "report.json"
    report.write_text(json.dumps({"sessions": played, "functions": by_fn, "calls": t.results}, indent=1) + "\n")
    total = len(t.entries)
    reached = len(by_fn)
    ok = sorted(n for n, f in by_fn.items() if f["same"] and not f["differs"] and not f["stopped"])
    bad = sorted(n for n, f in by_fn.items() if f["differs"] or f["stopped"])
    print("hybrid: %d functions, %d reached by the sessions; %d the same on every call tested, %d differ or "
          "stop, %d only untested calls (report: %s)" % (
              total, reached, len(ok), len(bad), reached - len(ok) - len(bad), report))
    for n in bad[:40]:
        first = next(r for r in t.results if r["function"] == n and r["status"] in ("differs", "C stopped"))
        print("  %s (%s): %s" % (n, first["unit"], first.get("why") or "; ".join(
            first.get("registers", []) + first.get("outside", []) + first.get("memory", [])[:2])))
    bad_session = any(p["golden"] for p in played)
    return 1 if bad or bad_session else 0


if __name__ == "__main__":
    sys.exit(main())
