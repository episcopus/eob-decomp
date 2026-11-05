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
