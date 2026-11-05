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
    a = ap.parse_args(argv)
    if not a.lib.exists():
        print("hybrid: %s is missing (make port-hybrid)" % a.lib)
        return 1
    sessions = a.session or sorted(p.stem for p in golden.SESSIONS.glob("*.keys")
                                   if "program INTRO.EXE" not in p.read_text())
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
