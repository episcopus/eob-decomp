"""The portable build (SPEC O5, playbook 80): tools/port/tc2c.py translates every C unit of EOB.EXE
(port/units.txt) from src/ without touching them, the C compiler links them with the arena runtime and
the platform layer, and the program's self-test runs translated functions on EOB.EXE's own DGROUP.
A unit with a construct tc2c does not handle is refused by name, never translated by a guess.
Skipped where clang or SDL3 is missing (the playbook says what to install)."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
TC2C = ROOT / "tools" / "port" / "tc2c.py"

needs_clang = pytest.mark.skipif(shutil.which("clang") is None, reason="clang is not installed")
needs_sdl3 = pytest.mark.skipif(
    shutil.which("pkg-config") is None
    or subprocess.run(["pkg-config", "--exists", "sdl3"]).returncode != 0,
    reason="SDL3 is not installed (pkg-config sdl3)")


@needs_clang
@needs_sdl3
def test_the_port_builds_and_passes_its_self_test():
    before = subprocess.run(["git", "status", "--porcelain", "src"], cwd=ROOT, capture_output=True, text=True).stdout
    r = subprocess.run(["make", "--no-print-directory", "port", "PY=%s" % sys.executable], cwd=ROOT,
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    assert "8 lists: ok" in r.stdout
    assert "41 units" in r.stdout, "every C unit of EOB.EXE is translated (M1)"
    # platform separation: the self-test ran on the headless program, which links no SDL
    assert "eob-port-headless --self-test" in r.stdout
    assert (ROOT / "build" / "port" / "eob-port-headless").exists()
    after = subprocess.run(["git", "status", "--porcelain", "src"], cwd=ROOT, capture_output=True, text=True).stdout
    assert before == after, "the port build changed src/"


def test_only_the_platform_directory_uses_sdl():
    """platform.h is the only boundary (playbook 80, "Platform separation"): the game, the runtime and
    the machine never include or call SDL, so they build for any host."""
    offenders = []
    for f in (ROOT / "port").rglob("*"):
        if f.suffix in (".c", ".h") and "platform" not in f.relative_to(ROOT / "port").parts[:1]:
            if re.search(r"#include *<SDL|SDL_[A-Za-z]", f.read_text()):
                offenders.append(str(f.relative_to(ROOT)))
    assert offenders == []


@needs_clang
def test_an_unhandled_construct_is_refused_by_name(tmp_path):
    """A struct passed by value is a construct no unit of EOB.EXE uses, so tc2c has no rule for it."""
    unit = tmp_path / "X.C"
    unit.write_text("struct S { int a; };\n"
                    "/*@ 1235:0000 */ int F(struct S s)\n{\n    return s.a;\n}\n")
    r = subprocess.run([sys.executable, str(TC2C), "--out", str(tmp_path / "gen"), str(unit)],
                       capture_output=True, text=True)
    assert r.returncode == 2
    assert "a struct passed by value" in r.stderr


PROBE_UNIT = r"""
struct P { int a; char b[3]; long c; };
/*@ 1000:0000 */ int Sum(int far *p, int n)
{
    int s = 0;
    while (n--)
        s += *p++;
    return s;
}
/*@ 1000:0020 */ int Frames(int x)
{
    int v[4];
    struct P q, r;
    int k = x;
    int i;
    for (i = 0; i < 4; i++)
        v[i] = i * 1000 + k;
    q.a = 7;
    q.b[0] = 'x';
    q.c = 70000L;
    r = q;
    switch (x) {
    case 1: k = 10; break;
    case 2: k = 20;
    default: k += 5;
    }
    if (k > 100)
        goto big;
    return Sum(v, 4) + r.a + (int)(r.c >> 16) + sizeof(struct P) + (x ? 1 : 2);
big:
    return -1;
}
/*@ 1000:0040 */ unsigned Wrap(unsigned a) { return a * 3 + 65535u; }
/*@ 1000:0060 */ int Widen(char c) { return c; }
/*@ 1000:0070 */ int Cmp(unsigned a) { return a + 65535u > a; }
/*@ 1000:0080 */ int Bump(int far *p)
{
    return ++*p + (*p)++;
}
/*@ 1000:00A0 */ int Taken(int x)
{
    int y = x;
    Bump(&y);
    return y;
}
"""

PROBE_MAIN = r"""
#include <stdio.h>
#include <string.h>
#include "tcport.h"
#include "tclib.h"
#include "ported.h"

static int bad;
static void eq(long got, long want, const char *what)
{
    if (got != want) { printf("FAIL %s: %ld, want %ld\n", what, got, want); bad++; }
}
static farptr put(u16 off, const char *s)
{
    farptr p = TC_MKFP(0x3000, off);
    u16 i;
    for (i = 0; i <= strlen(s); i++) tc_st_u8(tc_fp_add(p, i), (u8)s[i]);
    return p;
}
static void streq(farptr p, const char *want, const char *what)
{
    char got[128];
    u16 i = 0;
    while ((got[i] = (char)tc_ld_u8(tc_fp_add(p, i))) != 0 && i < 126) i++;
    if (strcmp(got, want) != 0) { printf("FAIL %s: [%s], want [%s]\n", what, got, want); bad++; }
}
int main(void)
{
    u8 va[16];
    farptr buf = TC_MKFP(0x3000, 0x0800);
    tc_ss = 0x2000; tc_sp = 0x1000;
    eq(Frames(1), 6022, "Frames(1)");
    eq(Frames(2), 6026, "Frames(2): case 2 falls into default");
    eq(Frames(200), -1, "Frames(200): goto");
    eq(tc_sp, 0x1000, "the frames are gone");
    eq(Wrap(10), 29, "Wrap: unsigned arithmetic wraps at 16 bits");
    eq(Widen((s8)-56), -56, "Widen: char is signed");
    eq(Cmp(5), 0, "Cmp: the sum wraps before the compare");
    eq(Taken(5), 7, "Taken: a local whose address is taken lives in the frame");
    /* the runtime library, Turbo C semantics */
    tc_va32(va + 0, put(0x100, "ab")); tc_va16(va + 4, 5); tc_va16(va + 6, 7); tc_va16(va + 8, (u16)-12);
    tc_va32(va + 10, 70000u);
    eq(tcl_sprintf(buf, put(0x200, "%-3s|%3d|%02d|%-4d|%lu|%%"), va, 14), 23, "sprintf's count");
    streq(buf, "ab |  5|07|-12 |70000|%", "sprintf");
    streq(tcl_itoa(-5, buf, 10), "-5", "itoa -5");
    streq(tcl_itoa(-1, buf, 16), "ffff", "itoa -1 radix 16");
    streq(tcl_ltoa(-70000, buf, 10), "-70000", "ltoa");
    eq(tcl_strcmp(put(0x300, "abc"), put(0x310, "abd")) < 0, 1, "strcmp");
    streq(tcl_strupr(put(0x320, "aBc1")), "ABC1", "strupr");
    eq(tcl_atol(put(0x330, "  -123x")), -123, "atol");
    eq(tcl_strlen(put(0x340, "hello")), 5, "strlen");
    printf(bad ? "probe: FAILED\n" : "probe: ok\n");
    return bad != 0;
}
"""


@needs_clang
def test_translated_constructs_run_with_turbo_c_semantics(tmp_path):
    """A unit with what M1 added to tc2c (frames for arrays, structs and address-taken locals, struct
    copy, switch with a fall-through, goto, ?:, sizeof with Turbo C's sizes, 16-bit wraparound, signed
    char), translated, compiled natively and run; and the runtime library on arena strings."""
    unit = tmp_path / "PROBE.C"
    unit.write_text(PROBE_UNIT)
    gen = tmp_path / "gen"
    r = subprocess.run([sys.executable, str(TC2C), "--out", str(gen), str(unit)], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    (tmp_path / "main.c").write_text(PROBE_MAIN)
    exe = tmp_path / "probe"
    cc = shutil.which("cc") or shutil.which("clang")
    srcs = [str(gen / "PROBE.c"), str(gen / "unported.c"), str(tmp_path / "main.c"),
            str(ROOT / "port" / "runtime" / "arena.c"), str(ROOT / "port" / "runtime" / "tclib.c")]
    r = subprocess.run([cc, "-std=gnu99", "-O1", "-Wall", "-Werror", "-Wno-unused-value", "-Wno-parentheses-equality",
                        "-I", str(ROOT / "port" / "include"), "-I", str(gen), "-o", str(exe), *srcs],
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    assert r.returncode == 0 and "probe: ok" in r.stdout, r.stdout + r.stderr


@needs_clang
def test_the_literal_table_agrees_with_src_and_the_original():
    """port/literals.json (make port-literals, from the matcher) lists each unit's literals as src/ has
    them, and the original's DGROUP holds each one's bytes where the table puts it."""
    sys.path.insert(0, str(ROOT / "tools" / "port"))
    import tc2c
    prog = tc2c.Program(ROOT / "eob" / "EOB.EXE", None)
    units = [n for n in (ROOT / "port" / "units.txt").read_text().split() if not n.startswith("#") and n.endswith(".C")]
    for name in units:
        u = tc2c.Unit(ROOT / "src" / name, prog)
        lits = [b for _i, b in tc2c.unit_literals(u.ast())]
        rows = prog.literal_table.get(u.name, [])
        assert [bytes.fromhex(r[0]) for r in rows] == lits, "%s: make port-literals" % name
        for hexb, off, _how in rows:
            b = bytes.fromhex(hexb) + b"\0"
            assert prog.dgroup[off:off + len(b)] == b, "%s: ds:%04X" % (name, off)


@needs_clang
@pytest.mark.slow
def test_each_function_against_the_original(tmp_path):
    """M2 (playbook 80): boot-title plays on the oracle; at the first call of each translated function
    it reaches, the original and the C run on the bench from the same state and their memory, return
    registers, screen, sound and files agree. The session is still its golden afterwards (the bench
    never touches it). Then with two translated functions made wrong in a copy of the library (FindMaskBit
    counts one bit more, SetTextWindow stores the region plus one): the test must see both."""
    r = subprocess.run(["make", "--no-print-directory", "-f", "port/port.mk", "PY=%s" % sys.executable,
                        "port-hybrid"], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    hybrid = [sys.executable, str(ROOT / "tools" / "port" / "hybrid.py"), "--session", "boot-title",
              "--samples", "1"]
    r = subprocess.run(hybrid + ["--out", str(tmp_path / "ok")], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    assert "the session is still its golden" in r.stdout
    m = re.search(r"(\d+) the same on every call tested, 0 differ or stop", r.stdout)
    assert m and int(m.group(1)) >= 30, r.stdout

    # the mutants
    gen, obj = ROOT / "build" / "port" / "gen", ROOT / "build" / "port" / "hybrid" / "obj"
    mutants = {"E14EF": ("s16 i = 0;", "s16 i = 0; n = (s16)(n + 1);", "s16 FindMaskBit("),
               "E1DE7": ("tc_st_s16(TC_MKFP(tc_ds, 0x811F), region);",
                         "tc_st_s16(TC_MKFP(tc_ds, 0x811F), (s16)(region + 1));", "void SetTextWindow(")}
    cc = shutil.which("cc") or shutil.which("clang")
    objs = [str(o) for o in obj.rglob("*.o") if o.stem not in mutants]
    for unit, (old, new, fn) in mutants.items():
        text = (gen / ("%s.c" % unit)).read_text()
        at = text.index(fn)
        assert old in text[at:], unit
        text = text[:at] + text[at:].replace(old, new, 1)
        (tmp_path / ("%s.c" % unit)).write_text(text)
        o = tmp_path / ("%s.o" % unit)
        r = subprocess.run([cc, "-std=gnu99", "-O2", "-w", "-DTC_HYBRID", "-fPIC", "-I", str(ROOT / "port" / "include"),
                            "-I", str(gen), "-c", "-o", str(o), str(tmp_path / ("%s.c" % unit))], capture_output=True, text=True)
        assert r.returncode == 0, r.stderr
        objs.append(str(o))
    lib = tmp_path / "mutant.so"
    r = subprocess.run([cc, "-shared", "-o", str(lib), *objs], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    r = subprocess.run(hybrid + ["--out", str(tmp_path / "bad"), "--lib", str(lib), "--function", "FindMaskBit",
                                 "--function", "SetTextWindow"], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 1, r.stdout + r.stderr
    assert "0 the same on every call tested, 2 differ or stop" in r.stdout, r.stdout
    assert re.search(r"FindMaskBit \(E14EF.C\): returns \w+, the original", r.stdout), r.stdout
    assert "SetTextWindow (E1DE7.C): ds:811F" in r.stdout, r.stdout


@needs_clang
@pytest.mark.slow
def test_the_assembly_routines_in_c_against_the_original():
    """M3 (playbook 80): the routines of port/asm, each called on the bench with chosen arguments, the
    original's code then the C, compared as the per-function test compares (hybrid.py, Direct): the
    random step in sequence, the far copy (overlaps, odd lengths, 64 KB blocks whose words straddle
    offset FFFFh), the LCW encoder and Unpack4 on its output (a round trip), Unpack4 on random streams,
    the dictionary unpackers on random code streams and Unpack3 on well-formed ones; the INT 1Ch
    handler, its install and removal; the retrace read and waits."""
    import random
    r = subprocess.run(["make", "--no-print-directory", "-f", "port/port.mk", "PY=%s" % sys.executable,
                        "port-hybrid"], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    sys.path.insert(0, str(ROOT / "tools" / "port"))
    import hybrid
    d = hybrid.Direct()
    rng = random.Random(1)
    bad = []

    def call(name, *args, **regs):
        res = d.call(name, *args, **regs)
        if res["status"] != "same":
            bad.append(res)
        return res

    def fp(seg, off):
        return ((seg << 16) | off, 4)

    for _ in range(100):
        call("RandomByte")
    for _ in range(20):
        call("NormalizePtr", fp(rng.randrange(65536), rng.randrange(65536)))
    for k, n in enumerate([0, 1, 2, 3, 255, 4096, 0x10001, 0x18003]):
        d.write(0x5000, 0, bytes(rng.randrange(256) for _ in range(0x300)))
        call("Unpack0", fp(0x5000, rng.randrange(16)), fp(0x5000, rng.randrange(40)) if k % 2 else fp(0x7000, 3), (n, 4))
    for n in (2, 16, 1000, 4096):
        data = bytes(rng.choice(b"aab") for _ in range(n // 2)) + bytes(rng.randrange(256) for _ in range(n - n // 2))
        d.write(0x5000, 0, data)
        call("CompressLcw", fp(0x5000, 0), fp(0x6000, 5), n)
        call("Unpack4", fp(0x6000, 5), fp(0x7000, 9), n)
        assert d.read(0x7000, 9, n) == data, "the original's Unpack4 inverts the original's CompressLcw"
    for _ in range(8):
        d.write(0x6000, 0, bytes(rng.randrange(256) for _ in range(3000)))
        call("Unpack4", fp(0x6000, 0), fp(0x7000, 0x10), rng.randrange(1, 3000))
    for name in ("Unpack1", "Unpack2"):
        for _ in range(8):
            hdr = b"\0\0" + rng.randrange(1, 6000).to_bytes(2, "little") + b"\0\0\0\0"
            d.write(0x6000, 0, hdr + bytes(rng.randrange(256) for _ in range(12000)))
            call(name, 0x6000, 0x7000, 0x8000)
    for _ in range(8):
        toks, total, target = bytearray(), 0, rng.randrange(1, 5000)
        while total < target:
            n = min(rng.randrange(1, 128), target - total)
            kind = rng.randrange(3)
            if kind == 0:
                toks += bytes([n]) + bytes(rng.randrange(256) for _ in range(n))
            elif kind == 1:
                toks += bytes([256 - n]) + bytes([rng.randrange(256)])
            else:
                toks += b"\0" + n.to_bytes(2, "big") + bytes([rng.randrange(256)])
            total += n
        d.write(0x6000, 0, b"\0\0" + target.to_bytes(4, "little") + b"\0\0" + bytes(toks))
        call("Unpack3", 0x6000, 0x7000)
    # the INT 1Ch unit: install, the handler on random counters (and all ones, every carry) with any DS
    call("InstallTickHandler")
    for k in range(8):
        d.write(hybrid.DGROUP, 0x8488, b"\xff" * 0x18 if k % 2 else bytes(rng.randrange(256) for _ in range(0x18)))
        call("TickIsr", ds=rng.randrange(65536))
    call("RemoveTickHandler")
    call("TickIsr", ds=0x1234)
    # the retrace, read and waited on in both modes at many phases of the frame (each poll charges the
    # clock, which is compared: a wait that ends one poll early or late differs)
    for k in range(30):
        call("IsInVerticalRetrace")
        call("WaitRetrace", k % 3)
        call("WaitRetrace", 0x0101)
        d.m.extra += 977 * k
    assert bad == [], bad[:5]


@needs_clang
@pytest.mark.slow
def test_the_machine_against_the_oracle(tmp_path):
    """M3 (playbook 80): the machine boots to the oracle's memory before the program runs (the vector
    table, the ROM, the BIOS data area, the PSP and environment, the loaded image: every byte; with no
    mouse and no sound, and with the mouse driver and the AdLib), and shadowing every service call and
    port access of the five EOB.EXE goldens it answers as the oracle does. A copy of
    the library with three bugs (the current drive one off, a relocation of a 4B03h load one off, a
    retrace poll charging one instruction too many) is caught by the shadow on boot-title."""
    import ctypes
    r = subprocess.run(["make", "--no-print-directory", "-f", "port/port.mk", "PY=%s" % sys.executable,
                        "port-hybrid"], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    sys.path.insert(0, str(ROOT / "tools"))
    from oracle import run
    lib = ctypes.CDLL(str(ROOT / "build" / "port" / "hybrid" / "libeobhybrid.so"))
    lib.tc_hybrid_boot.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint16, ctypes.c_int,
                                   ctypes.c_int]
    for mouse, sound in ((False, None), (True, "adlib")):
        m = run.build_machine(mouse=mouse, sound=sound)
        cmd = b" " + run.command_line(run.DEFAULT_ARGS, mouse, sound).encode()
        assert lib.tc_hybrid_boot(str(ROOT / "eob" / "EOB.EXE").encode(), b"EOB.EXE", cmd, len(cmd), mouse,
                                  sound == "adlib") == 0
        mem = ctypes.string_at(ctypes.addressof(ctypes.c_uint8.in_dll(lib, "tc_mem")), 0x100000)
        assert mem[:0xA0000] == bytes(m.uc.mem_read(0, 0xA0000)), "conventional memory after boot (mouse %s)" % mouse
        assert mem[0xF0000:] == bytes(m.uc.mem_read(0xF0000, 0x10000)), "the ROM (mouse %s)" % mouse

    hybrid = [sys.executable, str(ROOT / "tools" / "port" / "hybrid.py"), "--shadow"]
    r = subprocess.run(hybrid + ["--out", str(tmp_path / "ok")], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    n = int(re.search(r"(\d+) service calls the same as the oracle's, 0 differ; the machine lacks none", r.stdout).group(1))
    assert n > 300000, r.stdout

    mutants = {"dos": (("r->ax = (u16)((r->ax & 0xFF00u) | 2u);", "r->ax = (u16)((r->ax & 0xFF00u) | 1u);"),
                       ("tc_va16(image + p, (u16)(tc_vr16(image + p) + factor));",
                        "tc_va16(image + p, (u16)(tc_vr16(image + p) + factor + (i == 3u)));")),
               "ports": (("machine_time.extra += 40;", "machine_time.extra += 41;"),)}
    cc = shutil.which("cc") or shutil.which("clang")
    for unit, edits in mutants.items():
        src = (ROOT / "port" / "runtime" / ("%s.c" % unit)).read_text()
        for old, new in edits:
            assert old in src, old
            src = src.replace(old, new)
        (tmp_path / ("%s.c" % unit)).write_text(src)
        r = subprocess.run([cc, "-std=gnu99", "-O2", "-w", "-DTC_HYBRID", "-fPIC", "-I", str(ROOT / "port" / "include"),
                            "-I", str(ROOT / "build" / "port" / "gen"), "-c", "-o", str(tmp_path / ("%s.o" % unit)),
                            str(tmp_path / ("%s.c" % unit))], capture_output=True, text=True)
        assert r.returncode == 0, r.stderr
    objs = [str(o) for o in (ROOT / "build" / "port" / "hybrid" / "obj").rglob("*.o") if o.stem not in mutants]
    r = subprocess.run([cc, "-shared", "-o", str(tmp_path / "mutant.so"), *objs,
                        *[str(tmp_path / ("%s.o" % u)) for u in mutants]], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    r = subprocess.run(hybrid + ["--session", "boot-title", "--lib", str(tmp_path / "mutant.so"), "--out",
                                 str(tmp_path / "bad")], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 1, r.stdout + r.stderr
    assert "INT 21h AH=19h" in r.stdout and "AX 1901, the oracle 1902" in r.stdout, r.stdout
    assert "INT 21h AH=4Bh" in r.stdout, r.stdout
    assert "in 3DAh" in r.stdout and "the clock's charges" in r.stdout, r.stdout


@pytest.mark.slow
def test_the_c_unit_clock():
    """M3 (playbook 80): the oracle's C-unit clock (tools/oracle/cclock.py), which counts by blocks and
    corrects where a run chunk cuts one, equals a count taken instruction by instruction, over the first
    5 million instructions of EOB.EXE (the start-up, the overlay manager's first load, the title); and
    every C-clock recording beside the goldens (tests/sessions/golden/<name>.clock.json) is what playing
    its session with the clock gives again, the session still its golden."""
    sys.path.insert(0, str(ROOT / "tools"))
    from oracle import run
    from oracle.cclock import CClock
    from unicorn import UC_HOOK_CODE
    m = run.build_machine("EOB.EXE", args=run.PROGRAM_ARGS["EOB.EXE"])
    cc = CClock(m)
    exact = [0]

    def each(uc, a, size, ud):
        if cc.is_c(a):
            exact[0] += 1
    m.uc.hook_add(UC_HOOK_CODE, each)
    m.run(5_000_000)
    assert cc.overlay_ranges, "an overlay was loaded and its code counted"
    assert cc.count == exact[0] and 0 < cc.count < m.insns
    r = subprocess.run([sys.executable, str(ROOT / "tools" / "oracle" / "golden.py"), "--clock", "--check"],
                       cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]


@needs_clang
@pytest.mark.slow
@pytest.mark.parametrize("unit", ["e1aad", "e1948", "e_text", "waits"])
def test_the_interrupt_units_against_the_original(unit):
    """M3 (playbook 80): the units with interrupt handlers, written in C (port/asm/E1AAD.c, the keyboard;
    E1948.c, the speaker and Tandy driver; E_TEXT.c, the AdLib driver), each called on the bench with
    chosen states, the original then the C, compared with the device logs and the clock charges; and the
    machine's wait (port/runtime/irq.c), the routines that spin on interrupts called with the original's
    interrupts: tests/port_asm/<unit>.py (each script's docstring says what it covers)."""
    r = subprocess.run(["make", "--no-print-directory", "-f", "port/port.mk", "PY=%s" % sys.executable,
                        "port-hybrid"], cwd=ROOT, capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    r = subprocess.run([sys.executable, str(ROOT / "tests" / "port_asm" / ("%s.py" % unit))], cwd=ROOT,
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
