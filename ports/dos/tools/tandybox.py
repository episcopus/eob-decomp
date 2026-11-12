#!/usr/bin/env python3
"""tandybox.py: a Tandy 1000 EX around the project's oracle (tools/oracle), for measuring and for proving behaviour.

Two clocks, one machine:
  cycles    virtual time is the modelled 8088 clocks (m8088.py): timer interrupts, the retrace bit, the time of day all
            follow it. This is `prof.py`: what the machine would do, at 7.16 MHz.
  program   virtual time moves only when the game waits (keyboard polls, retrace reads, tick waits): drawing and logic
            take no time, so a faster build sees the same times and must do the same things. This is `func.py`.

The Tandy part of the machine, which the oracle (written for MCGA) does not have:
  - video memory is the top 32 KB of RAM: 9800:0000 is B800:0000 (the game probes for that alias and reserves the
    64 KB below it for its pages, ReserveMemoryBelowVideo);
  - the video registers (3D8h-3DFh): palette through the array at 3DAh/3DEh, the status bits at 3DAh from the clock;
  - the SN76489 at C0h (the oracle logs it).
The two routines that walk real DOS memory control blocks (the oracle's DOS has none) are replaced at start-up.
"""
from __future__ import annotations

import ctypes
import re
import struct
import sys
import tomllib
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(HERE))

from unicorn import UC_HOOK_BLOCK, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_PROT_ALL, UcError  # noqa: E402
from oracle.machine import LOAD_SEG, GuestCrash, Machine, OracleError, Stop  # noqa: E402
from oracle.input import SCAN, no_mouse, scancode  # noqa: E402
import m8088  # noqa: E402

GAME = ROOT / "eob"
VRAM = 0x98000
PAGE = 0x88000                     # the pages the game reserves below video memory
FRAME = 119_467                    # 7.15909 MHz / 59.92 Hz
LINE = 456
CLOCK = 7_159_090
ARGS = "1 3 0 homie 3 40960"       # Tandy video, Tandy sound, no mouse (what START1.EXE passes for those choices)
INTRO_ARGS = "1 3 0 homie"         # the same choices, as START1.EXE runs INTRO.EXE
WATCH_GAP = 2_000_000              # clocks: two runs of a watched routine further apart than this are two runs
TANDY_RGB = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170), (170, 85, 0),
             (170, 170, 170), (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255), (255, 85, 85),
             (255, 85, 255), (255, 255, 85), (255, 255, 255)]
POLLS = ["_ReadKeySpecial", "_IsKeyPending", "_WaitKeyEvent", "_GetKeyState", "_ReadKey", "_ReadKeyIfAny"]
SPINS = ["_WaitTicks", "_CalibrateRetrace", "_WaitGameKey"]             # busy waits on the tick counters: on the program clock, virtual time passes in them


class Build:
    """The files of one build and the names in them."""

    def __init__(self, name: str, exe: Path, drv: Path, map_: Path, drv_lst: Path, program: str = "EOB.EXE"):
        self.name, self.exe, self.drv, self.program = name, exe, drv, program
        self.pubs: dict[str, tuple[int, int]] = {}
        self.segs: list[tuple[int, int, str, str]] = []
        in_seg = in_pub = False
        for line in map_.read_text(errors="replace").splitlines():
            if "Start  Stop" in line:
                in_seg = True
                continue
            if "Publics by Value" in line:
                in_pub, in_seg = True, False
                continue
            if "Publics by Name" in line or "Address" in line and "Publics" in line:
                in_seg = False
            if in_seg:
                m = re.match(r"\s*([0-9A-F]+)H\s+([0-9A-F]+)H\s+([0-9A-F]+)H\s+(\S+)\s+(\S+)", line)
                if m:
                    self.segs.append((int(m[1], 16), int(m[3], 16), m[4], m[5]))
            if in_pub:
                m = re.match(r"\s*([0-9A-F]{4}):([0-9A-F]{4})(?:\s+\w+)?\s+(\S+)\s*$", line)
                if m:
                    self.pubs[m[3]] = (int(m[1], 16), int(m[2], 16))
        self.by_seg: dict[int, list[tuple[int, str]]] = defaultdict(list)
        for n, (s, o) in self.pubs.items():
            self.by_seg[s].append((o, n))
        for v in self.by_seg.values():
            v.sort()
        self.drv_procs = []
        for line in drv_lst.read_text(encoding="latin-1").splitlines():
            m = re.match(r"\s*\d+\s+([0-9A-F]{4})\s+(\w+)\s+PROC\b", line)
            if m:
                self.drv_procs.append((int(m[1], 16), m[2]))
        self.drv_procs.sort()

    def routine_size(self, name: str) -> int:
        """Bytes from a public to the next one in its segment."""
        s, o = self.pubs[name]
        v = self.by_seg[s]
        for i, (off, n) in enumerate(v):
            if n == name:
                if i + 1 < len(v):
                    return v[i + 1][0] - off
                lin = s * 16 + off
                for start, length, _n, _c in self.segs:
                    if start <= lin < start + length:
                        return start + length - lin
                return 64
        raise KeyError(name)

    def addr(self, name: str) -> tuple[int, int]:
        s, o = self.pubs[name]
        return LOAD_SEG + s, o


def builds_dir() -> Path:
    return HERE.parent / "build"


def get_build(name: str, program: str = "EOB.EXE") -> Build:
    """`orig`: what `make rebuild` links from src/ (the pinned program). Otherwise ports/dos/build/<name>."""
    stem = program.split(".")[0].lower()
    if name == "orig":
        return Build("orig", ROOT / "eob" / program, ROOT / "eob/TGA.OVL", ROOT / ("build/rebuild/%s/%s.map" % (stem, stem)),
                     ROOT / "build/rebuild/tga/d.lst", program)
    w = builds_dir() / name
    return Build(name, w / program, w / "TGA.OVL", w / (stem + ".map"), w / "d.lst", program)


class TandyMachine(Machine):
    def __init__(self, build: Build, *, clock: str = "cycles", args: str | None = None, save: Path | None = None,
                 watch: tuple[str, ...] = ()):
        self.build = build
        if args is None:
            args = ARGS if build.program == "EOB.EXE" else INTRO_ARGS
        self.watch_names = set(watch)      # routines whose runs are timed: a run is the span with no gap over WATCH_GAP
        self.watch_ids: set[int] = set()
        self.spans: dict[str, list] = defaultdict(list)
        self.clock_mode = clock
        self._cycled = clock == "cycles"
        self.block_cache: dict[int, m8088.Block] = {}
        self.routine_names: list[str] = []
        self.routine_ids: dict[str, int] = {}
        self.per = defaultdict(int)
        self.bus_total = 0
        self.spin_count = 0
        self.palette = list(range(16))
        self.pal_idx = None
        self.vmode = {}
        self.drv_load = None
        self.driver_writes = 0
        super().__init__(build.exe, GAME, cmdline=b" " + args.encode(), insn_per_sec=CLOCK, strict=True)
        self._hide_unpinned()
        no_mouse(self)
        self.vfs.alias(build.program, build.exe)
        self.vfs.alias("TGA.OVL", build.drv)
        for f in ("SN.DAT", "SNSOUND.DAT"):                 # tandy-sound.patch: the Tandy's sounds (sound/sndat.py)
            if (build.exe.parent / f).exists():
                self.vfs.alias(f, build.exe.parent / f)
        if save is not None:
            self._use_save(save)
        self._map_video()
        for p in range(0x3D0, 0x3E0):
            self.port_handlers[p] = (self._vid_in, self._vid_out)
        self.file_listeners.append(self._on_file)
        self.reads_log: list = []
        self._next_stop = 1 << 62
        self._hook_memory_chain()
        self._deadline = 0
        self._stop_after = None
        self._prev: m8088.Block | None = None
        self.work = 0                      # modelled clocks of the code run so far (in program mode they cost no time)
        self.blocks_since_poll = 0
        self.uc.hook_add(UC_HOOK_BLOCK, self._on_block)

    def _hide_unpinned(self):
        keep = {Path(k).name.upper() for k in tomllib.loads((ROOT / "re/inputs.toml").read_text())["file"]}
        for full in list(self.vfs.host_index):
            if full.rsplit("\\", 1)[-1] not in keep:
                self.vfs.deleted.add(full)

    def _use_save(self, save: Path):
        sys.path.insert(0, str(ROOT / "tools/re"))
        import eobfmt
        self.vfs.alias("EOBDATA.SAV", save)
        self.vfs.deleted.discard(self.vfs.mount + "\\EOBDATA.SAV")
        levels = eobfmt.save(save.read_bytes())["levels"]
        self.vfs.layer[self.vfs.mount + "\\LEVELS.TMP"] = bytearray(levels)
        self.vfs.deleted.discard(self.vfs.mount + "\\LEVELS.TMP")

    def _map_video(self):
        u = self.uc
        u.mem_unmap(0xA0000, 0x20000)
        u.mmio_map(0xA0000, 0x18000, self.ega.mmio_read, None, self.ega.mmio_write, None)
        base = ctypes.addressof(self.ram) + VRAM
        u.mem_map_ptr(0xB8000, 0x8000, UC_PROT_ALL, ctypes.cast(base, ctypes.c_void_p))
        orig = self.ega.set_mode

        def set_mode(mode, clear=True):
            orig(mode, clear)
            self.vmode["mode"] = mode
            if clear:
                u.mem_write(VRAM, bytes(0x8000))
        self.ega.set_mode = set_mode

    def _hook_memory_chain(self):
        b = self.build

        def chain(m):
            m.set("ax", 0)
        if "_CheckMemoryChain" in b.pubs:
            self.hook_routine(*b.addr("_CheckMemoryChain"), chain)

        def reserve(m):
            m.dos.arena_top = 0x8800
            m.dos.arena[-1][1] = min(m.dos.arena[-1][1], 0x8800 - m.dos.arena[-1][0])
            m.set("ax", 0x8800)
        if "_ReserveMemoryBelowVideo" in b.pubs:
            self.hook_routine(*b.addr("_ReserveMemoryBelowVideo"), reserve)
        if "_TrimMemoryToVideo" in b.pubs:
            self.hook_routine(*b.addr("_TrimMemoryToVideo"), lambda m: None)

    def dgroup(self) -> int:
        return LOAD_SEG + min(x[0] for x in self.build.segs if x[2] == "_DATA") // 16

    # ------------------------------------------------------------------ the video registers
    def _vid_in(self, port, size):
        if port == 0x3DA:
            self.pal_idx = None
            t = self.insns if self.clock_mode == "cycles" else self.spin_count * 977
            if self.clock_mode == "program":
                self.program_wait(977)        # a read of the status port: about a scan line and a bit
                t = self.insns
            frame = t % FRAME
            line = frame // LINE
            vr = 8 if line >= 224 and line < 240 else 0
            dis = 1 if (frame % LINE) > 300 or line >= 200 else 0
            return vr | dis
        return 0xFF if size == 1 else 0xFFFF

    def _vid_out(self, port, size, val):
        if port == 0x3DA:
            self.pal_idx = val & 0x1F
        elif port == 0x3DE and self.pal_idx is not None and self.pal_idx >= 0x10:
            self.palette[self.pal_idx - 0x10] = val & 0x0F

    # ------------------------------------------------------------------ files
    def _on_file(self, m, op, path, pos, n):
        if op == "read":
            self.reads_log.append((self.insns, path.rsplit("\\", 1)[-1], pos, n))
            if path.upper().endswith("EOB.EXE"):
                self.block_cache.clear()
            if path.upper().endswith("TGA.OVL"):
                self.block_cache.clear()

    # ------------------------------------------------------------------ cost of the code
    def routine_of(self, lin: int) -> int:
        name = self._name(lin)
        i = self.routine_ids.get(name)
        if i is None:
            i = self.routine_ids[name] = len(self.routine_names)
            self.routine_names.append(name)
            if name in self.watch_names:
                self.watch_ids.add(i)
        return i

    def _name(self, lin: int) -> str:
        import bisect
        b = self.build
        if self.image_loads:
            for nm, seg, _f in self.image_loads:
                if seg * 16 <= lin < seg * 16 + 0x3000:
                    off = lin - seg * 16
                    i = bisect.bisect_right([o for o, _n in b.drv_procs], off) - 1
                    return "TGA." + (b.drv_procs[i][1] if i >= 0 else "?")
        if not hasattr(self, "_seg_ranges"):
            self._seg_ranges = []
            self._stubs = []
            for start, length, name, cls in b.segs:
                if cls == "STUBSEG":
                    if length:
                        self._stubs.append((LOAD_SEG + (start >> 4), start >> 4, name))
                elif cls == "CODE":
                    self._seg_ranges.append((LOAD_SEG * 16 + start, LOAD_SEG * 16 + start + length, start >> 4, name))
            self._seg_ranges.sort()
        for stub, para, name in self._stubs:
            seg = self.rw(stub, 0x10)
            if seg and seg * 16 <= lin < seg * 16 + self.rw(stub, 8):
                off = lin - seg * 16
                v = b.by_seg.get(para, [])
                i = bisect.bisect_right(v, (off, "\xff")) - 1
                return (v[i][1] if i >= 0 else name + "+%04X" % off)
        for lo, hi, para, name in self._seg_ranges:
            if lo <= lin < hi:
                off = lin - (LOAD_SEG * 16 + (para << 4))
                v = b.by_seg.get(para, [])
                i = bisect.bisect_right(v, (off, "\xff")) - 1
                return v[i][1] if i >= 0 else name + "+%04X" % off
        return "BIOS/DOS"

    def _on_block(self, uc, addr, size, _ud):
        b = self.block_cache.get(addr)
        if b is None or b.size != size:
            code = bytes(uc.mem_read(addr, size))
            b = m8088.block_cost(code, addr, self.routine_of(addr))
            self.block_cache[addr] = b
        c = b.cycles
        if b.cl_bits:
            c += b.cl_bits * 4 * (uc.reg_read(self._cx) & 0xFF)
        p = self._prev
        if p is not None and p.taken:
            c += p.not_taken if addr == p.fall else p.taken
        if self._cycled:
            if self.insns + c >= self._deadline and not self._first:
                uc.emu_stop()                     # a stop in the hook keeps the block from running: not charged
                return
            self.insns += c
        else:
            if self.want_stop and not self._first:   # interrupts due at a wait point: taken at the block boundary
                self.want_stop = False
                uc.emu_stop()
                return
            self.blocks_since_poll += 1
            if self.blocks_since_poll > self.STALL_BLOCKS:
                raise OracleError("program clock: no wait point in 8M blocks, at %04X:%04X (%s)" % (
                    self.get("cs"), self.get("ip"), self.routine_names[b.routine]))
        self._first = False
        self._prev = b
        self.work += c
        if b.routine in self.watch_ids:
            runs = self.spans[self.routine_names[b.routine]]
            if runs and self.insns - runs[-1][1] < WATCH_GAP:
                runs[-1][1] = self.insns
                runs[-1][3] = self.work
            else:
                runs.append([self.insns, self.insns, self.work, self.work])
        self.bus_total += b.bus
        self.per[b.routine] += c

    from unicorn.x86_const import UC_X86_REG_CX as _cx

    # ------------------------------------------------------------------ the run loop
    def request_stop(self):
        self._stop_after = True
        self.uc.emu_stop()

    def program_wait(self, cycles: int):
        """program clock: the game waited; time moves. Interrupts due are taken at this point."""
        self.insns += cycles
        self.blocks_since_poll = 0
        if self.insns >= self.next_irq0 or self.irq0_pending or self.irq1_pending or self.kbd_queue:
            self.want_stop = True
    want_stop = False
    STALL_BLOCKS = 8_000_000

    def run_clocked(self, limit: int) -> str:
        uc = self.uc
        try:
            while True:
                if self.exit_code is not None:
                    return "exit"
                now = self.insns
                if self.irq0_pending is False and now >= self.next_irq0:
                    self._raise_irq0()
                    self.next_irq0 += self._irq0_period()
                    if now >= self.next_irq0:             # the program clock can jump past several periods
                        self.next_irq0 = now + self._irq0_period() // 2
                self._try_deliver_irq0()
                self._try_deliver_irq1()
                if self.irq0_pending and self.insns - self._pending_since > 40_000_000:
                    raise OracleError("stalled with interrupts off at %04X:%04X" % (self.get("cs"), self.get("ip")))
                wait = self.irq0_pending or self.irq1_pending or self.kbd_queue
                self._deadline = (now + 150) if wait else self.next_irq0
                if self._cycled:
                    self.keys.service(self)
                if self.insns >= limit:
                    return "limit"
                start = self.linear(self.get("cs"), self.get("ip"))
                self._prev = None
                self._first = True
                uc.emu_start(start, 0xFFFFFFFF)
        except Stop as s:
            return s.reason
        except UcError as e:
            raise GuestCrash(self.crash_report(str(e))) from e

    # ------------------------------------------------------------------ the screen
    def screen(self) -> bytes:
        return bytes(self.uc.mem_read(VRAM, 0x8000))

    def png(self, path: Path):
        from oracle.ega import write_png
        out = bytearray(320 * 200 * 3)
        mem = self.screen()
        for y in range(200):
            base = (y & 3) * 0x2000 + (y >> 2) * 160
            for x in range(320):
                b = mem[base + x // 2]
                c = self.palette[(b >> 4) if x % 2 == 0 else (b & 15)]
                out[3 * (y * 320 + x):3 * (y * 320 + x) + 3] = bytes(TANDY_RGB[c])
        write_png(path, 320, 200, bytes(out))


# ---------------------------------------------------------------------------------------------- scripted input
class Session:
    """A script of the game's own pace: a `key` goes in when the game has been sitting idle (polling the keyboard,
    nothing drawn in between). What a key costs is the time from the key to the next time the game sits idle.

      key UP            a key press and release, when idle
      down / up X       hold or release
      wait N            N ticks (1/18.2 s) of virtual time
      idle              wait for the game to sit idle
      shot name         screenshot (with --shots) and a checkpoint
      mark text         label the following actions (`step`, `turn`...)
      quit
    """

    IDLE_GAP = 500_000                 # cycles between two polls below which the game is sitting idle
    IDLE_RUN = 4
    PATIENCE = 60 * CLOCK              # a screen that never goes idle: the key goes in anyway after a minute

    def __init__(self, text: str, m: TandyMachine, on_action=None, shots: Path | None = None):
        self.ops = []
        self.options = {}
        for raw in text.splitlines():
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            op, _, arg = line.partition(" ")
            arg = arg.strip()
            if op == "option":
                k, _, v = arg.partition(" ")
                self.options[k] = v.strip()
            else:
                self.ops.append((op, arg))
        self.m = m
        self.i = 0
        self.mark = ""
        self.on_action = on_action
        self.shots = shots
        self.polls = 0
        self.last_poll = 0
        self.run = 0
        self.run_start = 0
        self.t_key = None
        self.label = None
        self.release_at = None
        self.release_code = None
        self.wait_until = None
        self.checkpoints = []
        self.settled_at = None
        self.waiting_key_since = None
        self.last_polls_seen = 0
        self.nread = 0

    # the program clock's notion of `the game is doing something it will come back from`
    def progress(self) -> bool:
        p = self.polls != self.last_polls_seen
        self.last_polls_seen = self.polls
        return p

    def attach(self):
        m = self.m
        b = m.build
        for name in POLLS:
            if name in b.pubs:
                m.hook_routine(*b.addr(name), self._poll_cb(name), far=True) if False else self._bp(name, self._on_poll)
        m.keys = self
        if m.clock_mode == "program":
            self._install_program_spins()

    def _bp(self, name, fn):
        m = self.m
        seg, off = m.build.addr(name)
        lin = m.linear(seg, off)
        m.uc.hook_add(UC_HOOK_CODE, lambda uc, a, s, ud: fn(), begin=lin, end=lin)

    def _install_program_spins(self):
        """On the program clock a busy wait on a tick counter is where time passes: every instruction of the routine
        advances it (the same instructions in every build) and the interrupts due are taken."""
        m = self.m
        for name in SPINS:
            if name in m.build.pubs:
                seg, off = m.build.addr(name)
                lin = m.linear(seg, off)
                size = m.build.routine_size(name)
                m.uc.hook_add(UC_HOOK_CODE, self._spin_tick, begin=lin, end=lin + size - 1)
        if m.build.program == "EOB.EXE":
            # DrawViewFrame's `while (gFrameWaitTicks != 0) ;` (ds:849A): it spins on the counter the timer decrements, from an
            # overlay, so no address hook can reach it: reading the counter is where time passes
            lin = m.dgroup() * 16 + 0x849A
            m.uc.hook_add(UC_HOOK_MEM_READ, lambda uc, acc, a, sz, v, ud: self._spin_tick(uc, a, sz, ud), begin=lin, end=lin + 3)

    def _spin_tick(self, uc, addr, size, ud):
        m = self.m
        m.insns += 60
        m.blocks_since_poll = 0
        if m.insns >= m.next_irq0:
            m.want_stop = True

    def _on_poll(self):
        m = self.m
        self.polls += 1
        if m.clock_mode == "program":
            self.service(m)
            m.program_wait(14_000)          # a poll is 2 ms
        now = m.insns
        gap = now - self.last_poll
        self.last_poll = now
        if gap < self.IDLE_GAP and not m.kbd_queue and not self.key_pending():
            if self.run == 0:
                self.run_start = now
            self.run += 1
        else:
            self.run = 0
        if self.run >= self.IDLE_RUN:
            self.at_idle()
        if self.release_at is None and self.t_key is None and self.i >= len(self.ops):
            pass

    def key_pending(self) -> bool:
        return False

    def at_idle(self):
        m = self.m
        if self.t_key is not None:
            t = self.run_start
            if self.on_action:
                self.on_action(self.label, self.t_key, t)
            self.t_key = None
        self.settled_at = m.insns

    def service(self, m):
        now = m.insns
        if self.release_at is not None and now >= self.release_at:
            m.kbd_queue.append(self.release_code | 0x80)
            self.release_at = None
        if self.wait_until is not None:
            if now < self.wait_until:
                return
            self.wait_until = None
        while self.i < len(self.ops):
            op, arg = self.ops[self.i]
            if op == "mark":
                self.mark = arg
            elif op == "wait":
                self.wait_until = now + int(arg) * 393_216
                self.i += 1
                return
            elif op == "shot":
                self.checkpoint(arg)
            elif op == "poke":
                off, v = arg.split()
                m.ww(m.dgroup(), int(off, 16), int(v, 0))
            elif op == "quit":
                self.i += 1
                raise Stop("quit")
            elif op in ("key", "down", "up", "idle"):
                if self.t_key is not None or self.run < self.IDLE_RUN:
                    if self.waiting_key_since is None:
                        self.waiting_key_since = now
                    if now - self.waiting_key_since < self.PATIENCE:
                        return
                self.waiting_key_since = None
                if op == "key":
                    k = scancode(arg)
                    m.kbd_queue.append(k)
                    self.release_at, self.release_code = now + 700_000, k
                    self.t_key = now
                    self.label = "%s %s" % (self.mark, arg)
                    self.run = 0
                elif op == "down":
                    m.kbd_queue.append(scancode(arg))
                elif op == "up":
                    m.kbd_queue.append(scancode(arg) | 0x80)
            else:
                raise ValueError(op)
            self.i += 1

    def checkpoint(self, name):
        m = self.m
        self.checkpoints.append((name, m.insns, m.screen()))
        if self.shots is not None:
            m.png(self.shots / (name + ".png"))

    def done(self):
        return self.i >= len(self.ops)
