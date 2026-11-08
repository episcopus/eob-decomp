#!/usr/bin/env python3
"""tc2c.py: translate matched Turbo C units of src/ into portable C for the arena (playbook 80).

The matched C stays as it is. clang reads a unit as C for the MSP430 target (16-bit int, 32-bit
long, the widths of Turbo C++), with signed char and byte-packed structs (port/include/tc/prelude.h)
and the port's own library headers (port/include/tc/sys/), and dumps its typed AST as JSON. This
tool walks that AST and writes C99 in which

  - every value has an exact width (u8 s8 u16 s16 u32 s32) and every conversion the 16-bit
    compiler made implicitly is a cast, so int arithmetic wraps at 16 bits as on the 8086;
  - every pointer is a far pointer (farptr, segment:offset in a u32, large model), and every load
    and store of memory goes through the arena helpers of port/include/tcport.h at the address the
    original uses: a global at its `ds:` annotation, a struct field at its Turbo C offset (bytes,
    no padding), an array element at index * size added to the offset only, a string literal where
    the original's code refers to it (port/literals.json, recorded by the matcher);
  - a local that needs an address (an array, a struct, a local whose address is taken) lives in a
    frame on the arena stack (tc_frame_enter), as Turbo C gives it one on SS;
  - a call through a far function pointer finds its C function by the original code address
    (tc_code_find; tc_code_far when there is none), and each defined function registers its own
    address: `@ SSSS:OOOO` for a resident one, its overlay stub entry for an overlay one
    (build/re/eob.json);
  - a Borland library function is called as tcl_<name> (port/include/tclib.h, the port's runtime).
  - an assembly routine written in C in port/asm (M3) is called by its name and registered at
    the address its `/*@ */` mark gives; the others are stubs (unported.c).

What the tool does not handle stops it with the construct and its line, so a new construct grows
the tool, never a silent guess.

  tc2c.py --out DIR [--image EXE] [--layout JSON] UNIT.C ...
      write DIR/<unit>.c, DIR/units.c (the init table), DIR/ported.h (one prototype per function)
      DIR/unported.c (a stub for each called function no unit and no runtime defines) and
      DIR/hybrid.c (the per-function test's thunks and entries, make port-hybrid)
"""
from __future__ import annotations

import argparse
import json
import tomllib
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "match"))

import cmatch  # noqa: E402  (read-only use: the `/*@ */` annotation parser)

PRELUDE = ROOT / "port" / "include" / "tc" / "prelude.h"
SYSINC = ROOT / "port" / "include" / "tc" / "sys"  # the library headers, in place of Turbo C's own
TCLIB_H = ROOT / "port" / "include" / "tclib.h"   # what the port's runtime provides
LITERALS = ROOT / "port" / "literals.json"        # where each literal is (tools/port/literals.py)
ASMDATA = ROOT / "port" / "asmdata.json"          # the assembly units' code-segment data (tools/port/asmlist.py)
ASM_C = ROOT / "port" / "asm"                     # the assembly routines, hand-written in C (M3)
# the library functions the runtime writes on the machine (M3): each definition marked with the library
# routine's address in EOB.EXE, `/*@ 0000:XXXX library */`, so the per-function test calls it beside the
# original's
LIB_C = [ROOT / "port" / "runtime" / "tcdos.c", ROOT / "port" / "runtime" / "tcheap.c"]
LIBRARY_MARK = re.compile(r"/\*@ ([0-9A-F]{4}):([0-9A-F]{4}) library \*/\s*(\w+)\s+(tcl_\w+)\s*\(([^)]*)\)")
# `/*@ SSSS:OOOO */` before a routine; `/*@ SSSS:OOOO interrupt */` before an interrupt handler (entered
# through a vector, `void f(void)`, which keeps every register and returns with IRET). A `static` routine
# so marked is the unit's own helper (its address documents it); it is not registered.
HANDWRITTEN = re.compile(r"/\*@ ([0-9A-F]{4}):([0-9A-F]{4})( interrupt)? \*/\s*((static\s+)?(\w+)\s+(\w+)\s*\(([^)]*)\))")
# the types a routine of port/asm may take or return, by size (a char or an int is pushed as a word)
OWN_SIZE = {"void": 0, "u8": 1, "s8": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "farptr": 4}
CLANG_ARGS = ["-x", "c", "-std=gnu89", "-target", "msp430", "-fsigned-char", "-fsyntax-only",
              "-nostdinc", "-Wno-pragma-pack", "-Wno-incompatible-function-pointer-types",
              "-Xclang", "-ast-dump=json"]
# EOB.EXE: DGROUP is paragraph 212Eh of the load module (whitepaper 2.2; port/main.c)
DGROUP_PARA = 0x212E
# Library functions that no annotation of src/ places: the targets of E19BC's far calls at 19BC:0049,
# 01DA and 0283 (the image's bytes; src/E19BC.C's comment says the same). The hybrid build calls the
# original's routine there (tools/port/hybrid.py).
LIBRARY_AT = {"farmalloc": "0000:72DE", "realloc": "0000:743A", "farheapwalk": "0000:76C0"}


class Unsupported(Exception):
    pass


# ---- types ---------------------------------------------------------------------------------------
# ("int", size, signed) | ("ptr", target) | ("arr", elem, n or None) | ("struct", tag of a struct or union)
# | ("func", ret, [params], variadic or None for `()`) | ("void",)

INT_NAMES = {
    "char": (1, True), "signed char": (1, True), "unsigned char": (1, False),
    "short": (2, True), "short int": (2, True), "signed short": (2, True), "unsigned short": (2, False),
    "int": (2, True), "signed int": (2, True), "signed": (2, True), "unsigned int": (2, False),
    "unsigned": (2, False), "long": (4, True), "long int": (4, True), "signed long": (4, True),
    "unsigned long": (4, False), "unsigned long int": (4, False),
}
TOKEN = re.compile(r"\s*(\.\.\.|[A-Za-z_]\w*|\d+|[*()\[\],])")
ATTR = re.compile(r"__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)")
INT_T = ("int", 2, True)


def tokens(s: str) -> list[str]:
    out, i = [], 0
    s = s.strip()
    while i < len(s):
        m = TOKEN.match(s, i)
        if not m:
            raise Unsupported("type %r" % s)
        out.append(m.group(1))
        i = m.end()
    return out


class TypeParser:
    """clang's printed types (qualType, desugared) as tuples. C abstract declarators, inside out."""

    def __init__(self, s: str):
        self.t = tokens(ATTR.sub("", s))
        self.i = 0

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else None

    def take(self, want=None):
        tok = self.peek()
        if want is not None and tok != want:
            raise Unsupported("type %r: expected %r" % (" ".join(self.t), want))
        self.i += 1
        return tok

    def base(self):
        words = []
        while self.peek() in ("const", "volatile", "__restrict", "restrict"):
            self.take()
        if self.peek() in ("struct", "union"):
            self.take()
            name = self.take()
            while self.peek() in ("const", "volatile", "__restrict", "restrict"):
                self.take()
            return ("struct", name)
        if self.peek() == "enum":
            self.take()
            self.take()
            while self.peek() in ("const", "volatile", "__restrict", "restrict"):
                self.take()
            return INT_T
        while self.peek() in ("signed", "unsigned", "char", "short", "int", "long", "void", "const",
                              "volatile"):
            w = self.take()
            if w not in ("const", "volatile"):
                words.append(w)
        if not words and self.peek() is not None and re.match(r"[A-Za-z_]\w*$", self.peek()):
            return ("struct", self.take())   # a typedef of an untagged struct or union (records_of)
        if words == ["void"]:
            return ("void",)
        key = " ".join(words)
        if key not in INT_NAMES:
            raise Unsupported("base type %r in %r" % (key, " ".join(self.t)))
        size, signed = INT_NAMES[key]
        return ("int", size, signed)

    def parse(self):
        b = self.base()
        t = self.declarator(b)
        if self.peek() is not None:
            raise Unsupported("type %r: trailing %r" % (" ".join(self.t), self.peek()))
        return t

    def declarator(self, b):
        # pointers bind looser than suffixes: `T *[3]` is an array of pointers
        if self.peek() == "*":
            self.take()
            while self.peek() in ("const", "volatile", "__restrict", "restrict"):
                self.take()
            return self.declarator(("ptr", b))
        if self.peek() == "(" and self.t[self.i + 1:self.i + 2] in (["*"], ["("]):
            self.take("(")
            mark = self.i
            depth = 1
            while depth:  # skip the inner declarator, apply the suffixes first
                tok = self.take()
                depth += {"(": 1, ")": -1}.get(tok, 0)
            end = self.i
            outer = self.suffixes(b)
            rest = self.i
            self.i = mark
            inner = self.declarator(outer)
            self.take(")")
            assert self.i == end
            self.i = rest
            return inner
        return self.suffixes(b)

    def suffixes(self, b):
        sfx = []
        while self.peek() in ("[", "("):
            if self.take() == "[":
                n = None
                if self.peek() != "]":
                    n = int(self.take())
                self.take("]")
                sfx.append(("arr", n))
            else:
                params, variadic = [], False
                if self.peek() == ")":
                    variadic = None          # `()`: no prototype (K&R or an implicit declaration)
                while self.peek() != ")":
                    if self.peek() == "...":
                        self.take()
                        variadic = True
                    else:
                        pb = self.base()
                        params.append(self.declarator(pb))
                    if self.peek() == ",":
                        self.take()
                self.take(")")
                if params == [("void",)]:
                    params = []
                sfx.append(("func", params, variadic))
        for s in reversed(sfx):
            b = ("arr", b, s[1]) if s[0] == "arr" else ("func", b, s[1], s[2])
        return b


ANON = re.compile(r"(?:\w+::)*\((?:unnamed|anonymous) (?:struct |union )?at [^)]*\)")


def anon_names(s: str) -> str:
    """clang prints a nested record without a tag as `Outer::(unnamed at FILE:LINE:COL)`: one
    identifier for the type parser and the record table."""
    return ANON.sub(lambda m: "anon_" + re.sub(r"\W", "_", m.group(0)), s)


def ctype(node_type: dict):
    s = node_type.get("desugaredQualType") or node_type["qualType"]
    return TypeParser(anon_names(s)).parse()


def write_if_changed(path: Path, text: str) -> None:
    """A generated file is written only when its text changes, so make recompiles only what changed."""
    if path.exists() and path.read_text() == text:
        return
    path.write_text(text)


def strip(n):
    """Through parentheses and the casts that change nothing."""
    while n["kind"] in ("ParenExpr",) or (n["kind"] in ("ImplicitCastExpr", "CStyleCastExpr")
                                         and n["castKind"] in ("NoOp", "BitCast")):
        n = n["inner"][0]
    return n


# ---- string literals -----------------------------------------------------------------------------

def literal_bytes(value: str) -> bytes:
    """clang's printed StringLiteral value (C escapes, in quotes) as bytes, without the NUL."""
    s = value[1:-1]
    out = bytearray()
    i = 0
    simple = {"n": 10, "r": 13, "t": 9, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, '"': 34, "'": 39, "?": 63}
    while i < len(s):
        ch = s[i]
        if ch != "\\":
            out += ch.encode("latin-1")
            i += 1
            continue
        nxt = s[i + 1]
        if nxt == "x":
            m = re.match(r"[0-9A-Fa-f]+", s[i + 2:])
            out.append(int(m.group(0), 16) & 0xFF)
            i += 2 + len(m.group(0))
        elif nxt in "01234567":
            m = re.match(r"[0-7]{1,3}", s[i + 1:])
            out.append(int(m.group(0), 8) & 0xFF)
            i += 1 + len(m.group(0))
        elif nxt in simple:
            out.append(simple[nxt])
            i += 2
        else:
            raise Unsupported("string escape \\%s" % nxt)
    return bytes(out)


def unit_literals(ast: dict) -> list[tuple[str, bytes]]:
    """The string literals of a unit that Turbo C keeps in its literal pool, in source order, as
    (AST node id, bytes): those used as pointers (clang decays them, ArrayToPointerDecay); a literal
    that initialises a char array, a variable or a struct's field, is that array's bytes."""
    out = []

    def walk(n, decays=False):
        if not isinstance(n, dict):
            return
        k = n.get("kind")
        if k == "StringLiteral":
            if decays:
                out.append((n["id"], literal_bytes(n["value"])))
            return
        through = k == "ParenExpr" and decays
        here = k == "ImplicitCastExpr" and n.get("castKind") == "ArrayToPointerDecay"
        for c in n.get("inner", []) or []:
            walk(c, here or through)

    walk(ast)
    return out


def place_literals(lits: list[bytes], dgroup: bytes) -> list[int]:
    """The DGROUP offset of each of a unit's string literals (in source order), found by content: the
    fallback for a unit port/literals.json does not list, and for the literals of a listed unit that no
    function refers to (a global's initialiser). Turbo C keeps a module's literals one after the other,
    each with its NUL, so a literal whose bytes occur once in DGROUP is an anchor, and one that occurs
    more than once is taken where its neighbours put it. [H] where the table does not confirm it."""
    cands = []
    for b in lits:
        pat = b + b"\0"
        found, k = [], dgroup.find(pat)
        while k >= 0:
            found.append(k)
            k = dgroup.find(pat, k + 1)
        if not found:
            raise Unsupported("string literal %r is nowhere in DGROUP" % b)
        cands.append(found)
    cum, k = [], 0
    for b in lits:
        cum.append(k)
        k += len(b) + 1
    anchors = [(i, c[0]) for i, c in enumerate(cands) if len(c) == 1]
    out = []
    for i, c in enumerate(cands):
        if len(c) == 1:
            out.append(c[0])
            continue
        near = min(anchors, key=lambda a: abs(a[0] - i)) if anchors else None
        want = near[1] + cum[i] - cum[near[0]] if near else 0
        # nearest to where the neighbours put it; at equal distance, one that starts after a NUL
        out.append(min(c, key=lambda x: (abs(x - want), x > 0 and dgroup[x - 1] != 0, x)))
    return out


# ---- the program: what every unit needs to know ------------------------------------------------

class Program:
    def __init__(self, image: Path | None, layout: Path | None, asm_c: list[Path] | None = None,
                 lib_c: list[Path] | None = None):
        self.dgroup = b""
        if image is not None:
            f = image.read_bytes()
            last, pages = struct.unpack_from("<HH", f, 2)
            header = struct.unpack_from("<H", f, 8)[0] * 16
            size = pages * 512 - (512 - last if last else 0) - header
            module = f[header:header + size]
            self.dgroup = module[DGROUP_PARA * 16:DGROUP_PARA * 16 + 0x10000]
        self.stubs: dict[tuple[int, int], tuple[int, int]] = {}  # (overlay, code offset) -> stub entry
        self.segments: list[int] = []
        if layout is not None:
            d = json.loads(layout.read_text())
            self.segments = [s["para"] for s in d["segments"]]
            for i, o in enumerate(d["overlays"]):
                for e in o["entries"]:
                    if e.get("ok", True):
                        self.stubs[(o.get("overlay", i), e["code_offset"])] = (o["stub_para"], 0x20 + 5 * e["index"])
        # functions the assembly units define (port/asm, M3): called by their own names
        self.asm: set[str] = set()
        for p in sorted((ROOT / "src").glob("*.ASM")):
            _f, _d, defined, _s = cmatch.parse_annotations(p.read_text(encoding="latin-1"), asm=True)
            self.asm |= {n.lstrip("_") for n in defined}
        # the library functions the headers of port/include/tc/sys declare
        self.header_funcs: set[str] = {"getvect"}
        for h in SYSINC.glob("*.h"):
            for m in re.finditer(r"^[^#/\n][^;\n]*?\b(\w+)\s*\([^;\n]*\)\s*;", h.read_text(), re.M):
                self.header_funcs.add(m.group(1))
        self.literal_table = json.loads(LITERALS.read_text())["units"] if LITERALS.exists() else {}
        # each module's initialised data, [start, end) in DGROUP (re/datamap.toml; step D3)
        dm = ROOT / "re" / "datamap.toml"
        self.spans = {m["id"]: (int(m["start"][3:], 16), int(m["end"][3:], 16))
                      for m in tomllib.loads(dm.read_text())["module"]} if dm.exists() else {}
        self.runtime = set(re.findall(r"\btcl_(\w+)\s*\(", TCLIB_H.read_text())) if TCLIB_H.exists() else set()
        self.defs: dict[str, tuple] = {}      # function name -> type of its definition
        self.decls: dict[str, tuple] = {}     # function name -> the first declaration seen
        self.where: dict[str, str] = {}       # function name -> its original address, for stubs
        self.library: set[str] = set()
        self.called: set[str] = set()
        self.addr: dict[str, tuple] = {}      # function name -> its annotation, the first seen
        self.alias: dict[str, str] = {}       # a name callers use -> the defined function at its address
        # the assembly routines port/asm has in C: name -> (segment, offset, file, interrupt handler),
        # from the `/*@ */` before each definition, as in src/
        self.handwritten: dict[str, tuple] = {}
        for f in sorted(asm_c if asm_c is not None else ASM_C.glob("*.c")):
            for m in HANDWRITTEN.finditer(f.read_text()):
                if m.group(5):
                    continue
                params = [x.strip().split()[0] for x in m.group(8).split(",") if x.strip() not in ("", "void")]
                own = (m.group(6), params, m.group(4))
                self.handwritten[m.group(7)] = (int(m.group(1), 16), int(m.group(2), 16), f.name, bool(m.group(3)),
                                                own)
        # the library functions the runtime has on the machine, at the library's addresses: tcl_name ->
        # (segment, offset, file, (return type, parameter types))
        self.libentries: dict[str, tuple] = {}
        for f in (lib_c if lib_c is not None else LIB_C):
            if not f.exists():
                continue
            for m in LIBRARY_MARK.finditer(f.read_text()):
                params = []
                for x in m.group(5).split(","):
                    x = x.strip()
                    if x in ("", "void"):
                        continue
                    params.append("va" if x.startswith("const u8 *") else x.split()[0])
                if "va" in params:
                    params = params[:params.index("va")] + ["va"]       # its size parameter follows
                self.libentries[m.group(4)] = (int(m.group(1), 16), int(m.group(2), 16), f.name,
                                               (m.group(3), params))

    def canonical(self, name: str):
        """The one prototype of a function: its definition's; else, among its declarations, a
        variadic one (a library function some unit declares with `...`), else one that returns a
        value (an assembly routine leaves AX for the callers that read it), else the first."""
        if name in self.defs:
            return self.defs[name]
        ds = [t for t in self.decls.get(name, []) if t[0] == "func"]
        if not ds:
            return None
        for pick in (lambda t: t[3] is True, lambda t: t[1][0] != "void", lambda t: True):
            for t in ds:
                if pick(t):
                    return t

    def far_entry(self, a: tuple) -> tuple | None:
        """The far address a call to this annotation goes to: an overlay function's stub entry."""
        if isinstance(a[0], str):
            return self.stubs.get((int(a[0][3:]), a[1]))
        return a

    def resolve_aliases(self):
        """A name no unit defines whose address is a defined function's (E0EF3.C's DrawViewRows at
        the stub entry 2116:0034 is EOV4.C's DrawCompass, ovl4:1EDE): calls go to the function."""
        at = {}
        for name in self.defs:
            e = self.far_entry(self.addr[name]) if name in self.addr else None
            if e is not None:
                at[e] = name
        for name, a in self.addr.items():
            if name not in self.defs:
                e = self.far_entry(a)
                if e in at:
                    self.alias[name] = at[e]


# ---- the unit ------------------------------------------------------------------------------------

class Unit:
    def __init__(self, path: Path, prog: Program):
        self.path = path
        self.prog = prog
        self.name = path.stem.upper()
        self.src = path.read_text(encoding="latin-1")
        self.funcs, self.data, self.defined, stubs = cmatch.parse_annotations(self.src)
        self.fardata = set(cmatch.FARDATA)
        if stubs:
            raise Unsupported("%s: stub functions %s" % (path.name, sorted(stubs)))
        # library declarations: `extern ... name(...); /*@ 0000:XXXX */` (the annotation parser takes
        # a function pointer parameter's name for harderr's)
        self.libdecl = {}
        for m in re.finditer(r"^extern\b[^;(]*?\b(\w+)\s*\(.*\)\s*;\s*/\*@ *0000:([0-9A-Fa-f]{4})", self.src, re.M):
            self.libdecl[m.group(1)] = int(m.group(2), 16)
        self.overlay = None
        mine = {self.funcs[n][0] for n in self.defined}
        if any(isinstance(s, str) for s in mine):
            if len(mine) != 1:
                raise Unsupported("%s: functions in more than one overlay %s" % (path.name, sorted(map(str, mine))))
            self.overlay = int(next(iter(mine))[3:])
        self.records: dict = {}
        self.enums: dict[str, int] = {}
        self.literals: dict[str, int] = {}   # id of a StringLiteral node -> its DGROUP offset
        self.out: list[str] = []
        self.tmp = 0
        self.vars: dict[str, tuple] = {}    # decl id -> ("var", cname, type) | ("mem", offset, type)
        self.frame = 0
        self.labels: dict[str, str] = {}
        self.discarded = None           # the call of the expression statement being translated
        self.blocks: list[str] = []
        self.line = 0

    def fail(self, what: str):
        raise Unsupported("%s:%d: %s" % (self.path.name, self.line, what))

    # -- clang
    def ast(self) -> dict:
        clang = shutil.which("clang") or shutil.which("cc")
        cmd = [clang, *CLANG_ARGS, "-include", str(PRELUDE), "-isystem", str(SYSINC), "-I", str(self.path.parent), "-"]
        # A unit compiled as C++ (`-P`, E1B82.C) uses C++ only for `extern "C" { ... }` around its
        # functions; read as C, those two lines are blank (the line numbers stay).
        text = self.src
        if re.search(r"/\*@flags[^*]*-P\b", text):
            text = re.sub(r'^extern "C" \{\s*$', "", text, flags=re.M)
            text = re.sub(r'^\} /\* extern "C" \*/\s*$', "", text, flags=re.M)
            if 'extern "C"' in re.sub(r"/\*.*?\*/", "", text, flags=re.S):
                raise Unsupported('%s: C++ beyond extern "C"' % self.path.name)
        text = '# 1 "%s"\n' % self.path + text
        r = subprocess.run(cmd, input=text.encode("latin-1"), capture_output=True)
        r.stdout, r.stderr = r.stdout.decode("latin-1"), r.stderr.decode("latin-1")
        if r.returncode:
            raise Unsupported("clang failed on %s:\n%s" % (self.path.name, r.stderr))
        return json.loads(r.stdout)

    def prepare(self, ast: dict):
        """Records, enum constants, the literals' places, the functions declared and defined."""
        self.records = records_of(ast)
        for d in ast["inner"]:
            if d["kind"] == "EnumDecl":
                v = -1
                for c in d.get("inner", []):
                    if c["kind"] == "EnumConstantDecl":
                        v = self.const(c["inner"][0]) if c.get("inner") else v + 1
                        self.enums[c["name"]] = v
        pairs = unit_literals(ast)
        ids, lits = [p[0] for p in pairs], [p[1] for p in pairs]
        table = self.prog.literal_table.get(self.name)
        if table is not None:
            # port/literals.json: where the matcher saw each literal (make port-literals)
            if [bytes.fromhex(t[0]) for t in table] != lits:
                raise Unsupported("%s: port/literals.json does not list this unit's literals as src/ has them"
                                  " (make port-literals)" % self.path.name)
            for i, t in zip(ids, table):
                self.literals[i] = t[1]
        elif lits:
            if not self.prog.dgroup:
                raise Unsupported("%s has string literals: tc2c needs --image" % self.path.name)
            for i, off in zip(ids, place_literals(lits, self.prog.dgroup)):
                self.literals[i] = off
        for d in ast["inner"]:
            if d["kind"] != "FunctionDecl":
                continue
            t = ctype(d["type"])
            name = d["name"]
            has_body = any(x["kind"] == "CompoundStmt" for x in d.get("inner", []))
            if has_body:
                if name in self.prog.defs:
                    raise Unsupported("%s is defined twice (%s)" % (name, self.path.name))
                self.prog.defs[name] = t
            else:
                self.prog.decls.setdefault(name, []).append(t)
            a = self.funcs.get(name)
            if a is not None and (has_body or name not in self.prog.addr):
                self.prog.addr[name] = a
            if a is None and name in self.libdecl:
                a = (0, self.libdecl[name])
            if a is not None:
                self.prog.where.setdefault(name, ("%s:%04X" % a) if isinstance(a[0], str) else "%04X:%04X" % a)
            if not has_body and name not in self.defined and (
                    name in self.prog.header_funcs or (a is not None and a[0] == 0 and name not in self.prog.asm)):
                self.prog.library.add(name)

    def const(self, n) -> int:
        """An integer constant expression (case labels, enumerators, array sizes)."""
        k = n["kind"]
        if k == "ConstantExpr" and "value" in n:
            return int(n["value"])
        if k in ("ConstantExpr", "ParenExpr"):
            return self.const(n["inner"][0])
        if k == "IntegerLiteral":
            return int(n["value"])
        if k == "CharacterLiteral":
            return int(n["value"])
        if k in ("ImplicitCastExpr", "CStyleCastExpr"):
            v = self.const(n["inner"][0])
            t = ctype(n["type"])
            if t[0] == "int":
                v &= (1 << (8 * t[1])) - 1
                if t[2] and v >= 1 << (8 * t[1] - 1):
                    v -= 1 << (8 * t[1])
            return v
        if k == "DeclRefExpr" and n["referencedDecl"]["kind"] == "EnumConstantDecl":
            return self.enums[n["referencedDecl"]["name"]]
        if k == "UnaryOperator":
            v = self.const(n["inner"][0])
            return {"-": -v, "~": ~v, "+": v, "!": int(not v)}[n["opcode"]]
        if k == "BinaryOperator":
            a, b = (self.const(x) for x in n["inner"])
            ops = {"+": a + b, "-": a - b, "*": a * b, "<<": a << b if b >= 0 else 0, ">>": a >> b if b >= 0 else 0,
                   "&": a & b, "|": a | b, "^": a ^ b}
            if n["opcode"] in ops:
                return ops[n["opcode"]]
            if n["opcode"] in ("/", "%") and b:
                q = abs(a) // abs(b) * (1 if (a < 0) == (b < 0) else -1)
                return q if n["opcode"] == "/" else a - q * b
        if k == "UnaryExprOrTypeTraitExpr" and n["name"] == "sizeof":
            return self.sizeof(n)
        self.fail("constant %s" % k)

    def sizeof(self, n) -> int:
        if "argType" in n:
            return self.size(ctype(n["argType"]))
        return self.size(ctype(n["inner"][0]["type"]))

    # -- layout (Turbo C: bytes, no padding; every pointer four bytes)
    def size(self, t) -> int:
        k = t[0]
        if k == "int":
            return t[1]
        if k == "ptr":
            return 4
        if k == "arr":
            if t[2] is None:
                self.fail("size of an array of unknown size")
            return self.size(t[1]) * t[2]
        if k == "struct":
            return self.layout(t[1])[1]
        self.fail("size of %r" % (t,))

    def layout(self, name: str):
        rec = self.records.get(name)
        if rec is None:
            self.fail("struct %s has no definition" % name)
        if "size" not in rec:
            off = 0
            for fid, fname, ft in rec["fields"]:
                rec["offsets"][fid] = (0 if rec["union"] else off, ft)
                off = max(off, self.size(ft)) if rec["union"] else off + self.size(ft)
            rec["size"] = off
        return rec["offsets"], rec["size"]

    def field(self, member_id: str):
        for rname, rec in self.records.items():
            if member_id in rec["ids"]:
                return self.layout(rname)[0][member_id]
        self.fail("member %s" % member_id)

    # -- C names of types
    def cname(self, t) -> str:
        if t[0] == "int":
            return ("s%d" if t[2] else "u%d") % (8 * t[1])
        if t[0] == "ptr":
            return "farptr"
        if t[0] == "void":
            return "void"
        self.fail("a value of type %r" % (t,))

    def load(self, t, addr: str) -> str:
        if t[0] == "int":
            return "tc_ld_%s(%s)" % (self.cname(t), addr)
        if t[0] == "ptr":
            return "tc_ld_u32(%s)" % addr
        if t[0] in ("struct", "arr"):
            return addr            # an aggregate's value is its address (copied by tc_copy)
        self.fail("load of %r" % (t,))

    def store(self, t, addr: str, val: str) -> str:
        if t[0] == "int":
            return "tc_st_%s(%s, %s)" % (self.cname(t), addr, val)
        if t[0] == "ptr":
            return "tc_st_u32(%s, %s)" % (addr, val)
        if t[0] == "struct":
            return "tc_copy(%s, %s, %d)" % (addr, val, self.size(t))
        self.fail("store of %r" % (t,))

    def global_addr(self, name: str) -> str:
        if name in self.data:
            return "TC_MKFP(tc_ds, 0x%04X)" % self.data[name]
        if name in self.fardata:
            seg, off = self.funcs[name]
            return "TC_MKFP(tc_load_seg + 0x%04X, 0x%04X)" % (seg, off)
        self.fail("global %s has no address annotation" % name)

    def code_addr(self, name: str) -> str | None:
        """The far address the original stores for a pointer to this function: resident, its own;
        overlay, its stub entry (whitepaper 2.3). None for an overlay function with no stub entry
        (static in the original: nothing points to it)."""
        a = self.funcs.get(name)
        if a is None:
            return None
        seg, off = a
        if isinstance(seg, str):
            stub = self.prog.stubs.get((int(seg[3:]), off))
            if stub is None:
                return None
            seg, off = stub
        return "TC_MKFP(tc_load_seg + 0x%04X, 0x%04X)" % (seg, off)

    def fresh(self) -> str:
        self.tmp += 1
        return "t%d_" % self.tmp

    def fn_name(self, name: str) -> str:
        name = self.prog.alias.get(name, name)
        return "tcl_" + name if name in self.prog.library else name

    # -- lvalues: ("mem", addr, type) or ("var", cname, type)
    def lv(self, n):
        k = n["kind"]
        t = ctype(n["type"])
        if k == "ParenExpr":
            return self.lv(n["inner"][0])
        if k == "DeclRefExpr":
            d = n["referencedDecl"]
            v = self.vars.get(d["id"])
            if v is not None:
                if v[0] == "var":
                    return v
                return ("mem", "tc_fp_add(fr_.base, 0x%X)" % v[1], v[2])
            if d["kind"] == "VarDecl":
                return ("mem", self.global_addr(d["name"]), t)
            self.fail("reference to %s %s" % (d["kind"], d.get("name")))
        if k == "MemberExpr":
            off, ft = self.field(n["referencedMemberDecl"])
            base = n["inner"][0]
            if n["isArrow"]:
                p = self.rv(base)
            else:
                b = self.lv(base)
                if b[0] != "mem":
                    self.fail("member of a struct held in a C variable")
                p = b[1]
            return ("mem", "tc_fp_add(%s, 0x%X)" % (p, off) if off else p, ft)
        if k == "ArraySubscriptExpr":
            base, idx = n["inner"]
            bt = ctype(base["type"])
            if bt[0] != "ptr":
                base, idx = idx, base
                bt = ctype(base["type"])
            el = self.size(bt[1])
            return ("mem", "tc_fp_add(%s, (u16)((u32)%s * %du))" % (self.rv(base), self.rv(idx), el), t)
        if k == "UnaryOperator" and n["opcode"] == "*":
            return ("mem", self.rv(n["inner"][0]), t)
        if k == "StringLiteral":
            return ("mem", "TC_MKFP(tc_ds, 0x%04X)" % self.literals[n["id"]], t)
        if k in ("CStyleCastExpr", "ImplicitCastExpr") and n["castKind"] in ("NoOp", "BitCast", "LValueBitCast"):
            inner = self.lv(n["inner"][0])
            return (inner[0], inner[1], t)
        self.fail("lvalue %s" % k)

    def rd(self, lval) -> str:
        if lval[0] == "var":
            return lval[1]
        return self.load(lval[2], lval[1])

    def assign(self, lval, v: str) -> str:
        if lval[0] == "var":
            return "(%s = %s)" % (lval[1], v)
        return self.store(lval[2], lval[1], v)

    # -- rvalues
    def rv(self, n) -> str:
        k = n["kind"]
        if "loc" in n and "line" in n["loc"]:
            self.line = n["loc"]["line"]
        m = getattr(self, "x_" + k, None)
        if m is None:
            self.fail("expression %s" % k)
        return m(n)

    def x_ParenExpr(self, n):
        return "(%s)" % self.rv(n["inner"][0])

    def x_ConstantExpr(self, n):
        t = ctype(n["type"])
        if t[0] == "int":
            return "((%s)%d)" % (self.cname(t), self.const(n))
        return self.rv(n["inner"][0])

    def x_IntegerLiteral(self, n):
        t = ctype(n["type"])
        v = int(n["value"])
        return "((%s)%d%s)" % (self.cname(t), v, "u" if not t[2] else "")

    def x_CharacterLiteral(self, n):
        return "((s16)%d)" % int(n["value"])

    def x_UnaryExprOrTypeTraitExpr(self, n):
        if n["name"] != "sizeof":
            self.fail(n["name"])
        return "((u16)%du)" % self.sizeof(n)

    def x_StringLiteral(self, n):
        return self.lv(n)[1]

    def x_ImplicitCastExpr(self, n):
        ck = n["castKind"]
        c = n["inner"][0]
        t = ctype(n["type"])
        if ck == "LValueToRValue":
            return self.rd(self.lv(c))
        if ck in ("IntegralCast", "PointerToIntegral", "IntegralToPointer"):
            st = ctype(c["type"])
            if ck == "IntegralToPointer" and st[0] == "int" and st[1] < 4:
                # Turbo C: a 16-bit value made a far pointer [H: none seen yet]
                self.fail("a 16-bit integer converted to a pointer")
            return "((%s)%s)" % (self.cname(t), self.rv(c))
        if ck == "NullToPointer":
            return "((farptr)0)"
        if ck in ("BitCast", "NoOp"):
            return self.rv(c)
        if ck == "ToVoid":
            return "((void)%s)" % self.rv(c)
        if ck == "ArrayToPointerDecay":
            lval = self.lv(c)
            if lval[0] != "mem":
                self.fail("an array in a C variable")
            return lval[1]
        if ck == "FunctionToPointerDecay":
            name = c["referencedDecl"]["name"]
            a = self.code_addr(name)
            if a is None:
                self.fail("the address of %s (no resident address and no overlay stub entry)" % name)
            return a
        self.fail("cast %s" % ck)

    x_CStyleCastExpr = x_ImplicitCastExpr

    def x_DeclRefExpr(self, n):
        d = n["referencedDecl"]
        if d["kind"] == "EnumConstantDecl":
            return "((s16)%d)" % self.enums[d["name"]]
        return self.rd(self.lv(n))

    def x_MemberExpr(self, n):
        return self.rd(self.lv(n))

    x_ArraySubscriptExpr = x_MemberExpr

    def x_ConditionalOperator(self, n):
        c, a, b = n["inner"]
        return "(%s ? %s : %s)" % (self.rv(c), self.rv(a), self.rv(b))

    ARITH = {"+", "-", "*", "&", "|", "^", "<<"}

    def arith(self, op: str, t, a: str, b: str) -> str:
        """a op b in type t, wrapping as the 8086 does: + - * << in u32 (no host overflow), the rest
        on the converted operands."""
        if op in ("+", "-", "*", "<<"):
            return "((%s)((u32)%s %s (u32)%s))" % (self.cname(t), a, op, b)
        return "((%s)(%s %s %s))" % (self.cname(t), a, op, b)

    def x_BinaryOperator(self, n):
        op = n["opcode"]
        a, b = n["inner"]
        t = ctype(n["type"])
        if op == "=":
            return self.assign(self.lv(a), self.rv(b))
        if op == ",":
            return "(%s, %s)" % (self.rv(a), self.rv(b))
        at, bt = ctype(a["type"]), ctype(b["type"])
        if op in ("&&", "||"):
            return "((s16)(%s %s %s))" % (self.rv(a), op, self.rv(b))
        if at[0] == "ptr" or bt[0] == "ptr":
            return self.pointer_op(op, a, b, at, bt, t)
        if op in ("==", "!=", "<", ">", "<=", ">="):
            return "((s16)(%s %s %s))" % (self.rv(a), op, self.rv(b))
        if op in self.ARITH or op in ("/", "%", ">>"):
            return self.arith(op, t, self.rv(a), self.rv(b))
        self.fail("operator %s" % op)

    def pointer_op(self, op, a, b, at, bt, t):
        if op in ("==", "!="):
            return "((s16)(%s %s %s))" % (self.rv(a), op, self.rv(b))
        if op in ("<", ">", "<=", ">="):
            # Turbo C compares far pointers by their offsets only [H: compile and check]
            return "((s16)(TC_OFF(%s) %s TC_OFF(%s)))" % (self.rv(a), op, self.rv(b))
        if op in ("+", "-") and at[0] == "ptr" and bt[0] == "int":
            return self.ptr_add(self.rv(a), at, self.rv(b), op)
        if op == "+" and bt[0] == "ptr" and at[0] == "int":
            return self.ptr_add(self.rv(b), bt, self.rv(a), op)
        if op == "-" and at[0] == "ptr" and bt[0] == "ptr":
            # the offsets' difference over the element size, in int [H: compile and check]
            el = self.size(at[1])
            return "((s16)((s16)(TC_OFF(%s) - TC_OFF(%s)) / %d))" % (self.rv(a), self.rv(b), el)
        self.fail("pointer operator %s" % op)

    def ptr_add(self, p: str, pt, i: str, op: str) -> str:
        el = self.size(pt[1])
        sign = "" if op == "+" else "0u - "
        return "tc_fp_add(%s, (u16)(%s(u32)%s * %du))" % (p, sign, i, el)

    def x_CompoundAssignOperator(self, n):
        op = n["opcode"][:-1]
        a, b = n["inner"]
        lval = self.lv(a)
        lt = lval[2]
        ct = ctype(n["computeLHSType"])
        rt = ctype(n["computeResultType"])
        rhs = self.rv(b)
        if lt[0] == "ptr":
            if op not in ("+", "-"):
                self.fail("compound %s= on a pointer" % op)
            new = lambda cur: self.ptr_add(cur, lt, rhs, op)  # noqa: E731
        elif lt[0] == "int" and ct[0] == "int":
            new = lambda cur: "(%s)%s" % (self.cname(lt), self.arith(op, rt, "((%s)%s)" % (self.cname(ct), cur), rhs))  # noqa: E731
        else:
            self.fail("compound assignment on %r" % (lt,))
        if lval[0] == "var":
            return "(%s = %s)" % (lval[1], new(lval[1]))
        tmp = self.fresh()
        return "({ farptr %s = %s; %s; })" % (tmp, lval[1], self.store(lt, tmp, new(self.load(lt, tmp))))

    def x_UnaryOperator(self, n):
        op = n["opcode"]
        c = n["inner"][0]
        t = ctype(n["type"])
        if op == "!":
            return "((s16)!%s)" % self.rv(c)
        if op == "~":
            return "((%s)~(u32)%s)" % (self.cname(t), self.rv(c))
        if op == "-":
            return "((%s)(0u - (u32)%s))" % (self.cname(t), self.rv(c))
        if op == "+":
            return self.rv(c)
        if op == "*":
            return self.rd(self.lv(n))
        if op == "&":
            sc = strip(c)
            if sc["kind"] == "DeclRefExpr" and sc["referencedDecl"]["kind"] == "FunctionDecl":
                return self.x_ImplicitCastExpr({"castKind": "FunctionToPointerDecay", "inner": [sc], "type": n["type"]})
            lval = self.lv(c)
            if lval[0] != "mem":
                self.fail("address of a C variable (tc2c's frame scan missed it)")
            return lval[1]
        if op in ("++", "--"):
            return self.step(self.lv(c), 1 if op == "++" else -1, n.get("isPostfix", False))
        self.fail("unary %s" % op)

    def step(self, lval, sign: int, postfix: bool) -> str:
        """++ and --: a pointer moves by its element size (the offset only), an integer by one."""
        t = lval[2]
        if t[0] == "ptr":
            delta = "(u16)%d" % (sign * self.size(t[1]) & 0xFFFF)
            new = lambda old: "tc_fp_add(%s, %s)" % (old, delta)  # noqa: E731
        elif t[0] == "int":
            new = lambda old: "(%s)((u32)%s %s 1u)" % (self.cname(t), old, "+" if sign > 0 else "-")  # noqa: E731
        else:
            self.fail("increment of %r" % (t,))
        c = self.cname(t)
        old = self.fresh()
        if lval[0] == "var":
            if not postfix:
                return "(%s = %s)" % (lval[1], new(lval[1]))
            return "({ %s %s = %s; %s = %s; %s; })" % (c, old, lval[1], lval[1], new(old), old)
        a = self.fresh()
        result = old if postfix else self.fresh()
        return "({ farptr %s = %s; %s %s = %s; %s%s; %s; })" % (
            a, lval[1], c, old, self.load(t, a),
            "" if postfix else "%s %s = " % (c, result), self.store(t, a, new(old)), result)

    def x_CallExpr(self, n):
        callee, *args = n["inner"]
        rt = ctype(n["type"])
        inner = strip(callee)
        while inner["kind"] == "ImplicitCastExpr" and inner["castKind"] == "FunctionToPointerDecay":
            inner = strip(inner["inner"][0])
        if inner["kind"] == "DeclRefExpr" and inner["referencedDecl"]["kind"] == "FunctionDecl":
            name = inner["referencedDecl"]["name"]
            if name in ("MK_FP", "FP_SEG", "FP_OFF"):
                return self.fp_macro(name, args)
            ft = ctype(inner["referencedDecl"]["type"]) if "type" in inner["referencedDecl"] else ctype(inner["type"])
            self.prog.called.add(name)
            if name not in self.prog.decls:
                self.prog.decls[name] = [ft]
            canon = self.prog.canonical(self.prog.alias.get(name, name)) or ft
            if canon[0] == "func" and canon[3]:
                if name not in self.prog.library:
                    self.fail("a variadic function of the game (%s)" % name)
                return self.variadic(name, canon, args, rt)
            argv = self.call_args(name, args)
            call = "%s(%s)" % (self.fn_name(name), argv)
            if rt[0] == "void":
                return call
            if canon[0] == "func" and canon[1][0] == "void":
                # declared here returning a value, defined returning none: harmless where the value
                # is thrown away (a call statement); used, it would be whatever the callee left in AX,
                # which a 0 would not be (DispatchKeyWait passed DispatchNodeEvent's on, playbook 80,
                # M6): refused, so src/ says what is returned
                if self.discarded is n:
                    return call
                self.fail("%s is defined returning void and used here as a value" % name)
            return "CALL_%s_(%s)" % (self.cname(rt), call)
        # a far function pointer: find the C function by the address it holds
        argv = ", ".join(self.rv(a) for a in args)
        ft = ctype(callee["type"])
        if ft[0] != "ptr" or ft[1][0] != "func":
            self.fail("call through %r" % (ft,))
        fn = ft[1]
        if fn[3]:
            self.fail("variadic call through a pointer")
        ps = [self.cname(p) for p in fn[2]] if fn[3] is not None else [self.cname(ctype(a["type"])) for a in args]
        rc = self.cname(fn[1])
        proto = "%s (*)(%s)" % (rc, ", ".join(ps) or "void")
        site = "%s:%d" % (self.path.name, self.line)
        # no C at the address (code the original loads, a driver): tc_code_far, which stops the
        # port and, in the hybrid build, runs the original's code (tools/port/hybrid.py); the
        # arguments as Turbo C pushes them
        f, names, sets, puts, o = self.fresh(), [], [], [], 0
        for a, c in zip(args, ps):
            v = self.fresh()
            names.append(v)
            sets.append("%s %s = %s;" % (c, v, self.rv(a)))
            w = 4 if c in ("u32", "s32", "farptr") else 2
            puts.append("tc_va%d(%s_b + %d, (u%d)%s);" % (8 * w, f, o, 8 * w, v))
            o += w
        far = "tc_code_far(%s, %s, %d, \"%s\")" % (f, "%s_b" % f if o else "0", o, site)
        call = "((%s)%s_c)(%s)" % (proto, f, ", ".join(names))
        head = "farptr %s = %s; %s tc_fn %s_c = tc_code_find(%s);" % (f, self.rv(callee), " ".join(sets), f, f)
        if rc == "void":
            return "({ %s if (%s_c != 0) %s; else { %s%s; } })" % (
                head, f, call, ("u8 %s_b[%d]; %s " % (f, o, " ".join(puts))) if o else "", far)
        return "({ %s %s %s_r; if (%s_c != 0) %s_r = %s; else { %s%s_r = (%s)%s; } %s_r; })" % (
            head, rc, f, f, f, call, ("u8 %s_b[%d]; %s " % (f, o, " ".join(puts))) if o else "", f, rc, far, f)

    def call_args(self, name, args) -> str:
        """The arguments of a direct call. When the caller's declaration and the definition lay the
        arguments out differently on the stack (E14EF's UpdateOptionList takes `int counter, int
        extra` where its callers push one `long`), they are laid out as the caller pushes them (a
        char or an int in a word, a long or a far pointer in two) and read back as the callee's
        parameters, as on the 8086."""
        real = self.prog.alias.get(name, name)
        t = self.prog.canonical(real)
        mine = [ctype(a["type"]) for a in args]
        if not t or t[0] != "func" or t[3] is not False:
            return ", ".join(self.rv(a) for a in args)
        theirs = t[2]

        def slot(x):
            w = self.size(x)
            if w > 4 or x[0] not in ("int", "ptr"):
                self.fail("an argument of type %r" % (x,))
            return 2 if w <= 2 else 4

        if [slot(x) for x in mine] == [slot(x) for x in theirs]:
            return ", ".join(self.rv(a) for a in args)
        total = sum(slot(x) for x in mine)
        if total != sum(slot(x) for x in theirs):
            self.fail("%s takes %d bytes of arguments, called with %d" % (name, sum(slot(x) for x in theirs), total))
        blk = self.fresh()
        puts, off = [], 0
        for a, at in zip(args, mine):
            w = slot(at)
            puts.append("tc_va%d(%s + %d, (u%d)%s)" % (8 * w, blk, off, 8 * w, self.rv(a)))
            off += w
        reads, off = [], 0
        for pt in theirs:
            w = slot(pt)
            reads.append("(%s)tc_vr%d(%s + %d)" % (self.cname(pt), 8 * w, blk, off))
            off += w
        self.blocks.append("u8 %s[%d];" % (blk, total))
        return "(%s, %s)" % (", ".join(puts), reads[0]) + "".join(", " + r for r in reads[1:])

    def fp_macro(self, name, args):
        if name == "FP_SEG":
            return "TC_SEG(%s)" % self.rv(args[0])
        if name == "FP_OFF":
            return "TC_OFF(%s)" % self.rv(args[0])
        seg, off = args
        if self.overlay is not None:
            try:
                v = self.const(seg)
            except Unsupported:
                v = None
            # a constant segment in overlay code is TLINK's segment-table index times 8, which the
            # overlay manager fixes up (whitepaper 2.6, EOV3.C's gWallSetCgaMaps)
            if v is not None and v % 8 == 0 and v // 8 < len(self.prog.segments):
                return "TC_MKFP(tc_load_seg + 0x%04X, %s)" % (self.prog.segments[v // 8], self.rv(off))
            if v is not None and v < 8 * len(self.prog.segments):
                self.fail("MK_FP(0x%X, ...) in overlay code is not a segment-table index" % v)
        return "TC_MKFP(%s, %s)" % (self.rv(seg), self.rv(off))

    def variadic(self, name, ft, args, rt):
        """A variadic library call: the arguments after the fixed ones are laid out as Turbo C
        pushes them (2 bytes for an int, 4 for a long or a far pointer, the first at the lowest
        address), and the runtime reads them from that block."""
        nfix = len(ft[2])
        fixed = [self.rv(a) for a in args[:nfix]]
        va, puts, off = self.fresh(), [], 0
        for a in args[nfix:]:
            at = ctype(a["type"])
            w = self.size(at)
            if at[0] not in ("int", "ptr") or w not in (2, 4):
                self.fail("variadic argument of type %r" % (at,))
            puts.append("tc_va%d(%s + %d, (u%d)%s);" % (8 * w, va, off, 8 * w, self.rv(a)))
            off += w
        call = "tcl_%s(%s)" % (name, ", ".join(fixed + [va, str(off)]))
        if rt[0] != "void":
            call = "(%s)%s" % (self.cname(rt), call)
        return "({ u8 %s[%d]; %s %s; })" % (va, max(off, 1), " ".join(puts), call)

    # -- statements
    def emit(self, depth: int, text: str):
        self.out.append("    " * depth + text)

    def stmt(self, n, depth: int):
        k = n["kind"]
        if "loc" in n and "line" in n["loc"]:
            self.line = n["loc"]["line"]
        if k == "CompoundStmt":
            self.emit(depth - 1, "{")
            for s in n.get("inner", []):
                self.stmt(s, depth)
            self.emit(depth - 1, "}")
        elif k == "DeclStmt":
            for d in n["inner"]:
                self.local(d, depth)
        elif k == "ReturnStmt":
            self.emit(depth, "return %s;" % self.rv(n["inner"][0]) if n.get("inner") else "return;")
        elif k == "IfStmt":
            inner = n["inner"]
            self.emit(depth, "if (%s)" % self.rv(inner[0]))
            self.block(inner[1], depth)
            if n.get("hasElse"):
                self.emit(depth, "else")
                self.block(inner[2], depth)
        elif k == "WhileStmt":
            c, body = n["inner"]
            lp = self.loop(depth)
            self.emit(depth, "while ((tc_spin(&%s), %s))" % (lp, self.rv(c)))
            self.block(body, depth)
            self.emit(depth, "}")
        elif k == "DoStmt":
            body, c = n["inner"]
            lp = self.loop(depth)
            self.emit(depth, "do")
            self.block(body, depth)
            self.emit(depth, "while ((tc_spin(&%s), %s));" % (lp, self.rv(c)))
            self.emit(depth, "}")
        elif k == "ForStmt":
            init, _condvar, c, inc, body = n["inner"]
            lp = self.loop(depth)
            parts = [self.rv(init) if init else "", "(tc_spin(&%s), %s)" % (lp, self.rv(c) if c else "1"),
                     self.rv(inc) if inc else ""]
            self.emit(depth, "for (%s; %s; %s)" % tuple(parts))
            self.block(body, depth)
            self.emit(depth, "}")
        elif k == "SwitchStmt":
            c, body = n["inner"][-2:]
            self.emit(depth, "switch (%s)" % self.rv(c))
            self.block(body, depth)
        elif k == "CaseStmt":
            self.emit(depth - 1, "case %d:" % self.const(n["inner"][0]))
            self.stmt(n["inner"][-1], depth)
        elif k == "DefaultStmt":
            self.emit(depth - 1, "default:")
            self.stmt(n["inner"][0], depth)
        elif k == "LabelStmt":
            self.emit(0, "%s_:" % n["name"])
            self.emit(depth, "tc_spin(&lp_%s_);" % n["name"])
            self.stmt(n["inner"][0], depth)
        elif k == "GotoStmt":
            self.emit(depth, "goto %s_;" % self.labels[n["targetLabelDeclId"]])
        elif k == "NullStmt":
            self.emit(depth, ";")
        elif k == "BreakStmt":
            self.emit(depth, "break;")
        elif k == "ContinueStmt":
            self.emit(depth, "continue;")
        elif k.endswith("Operator") or k.endswith("Expr"):
            top = strip(n)
            self.discarded = top if top.get("kind") == "CallExpr" else None
            self.emit(depth, "%s;" % self.rv(n))
            self.discarded = None
        else:
            self.fail("statement %s" % k)

    def loop(self, depth: int) -> str:
        """Opens a block holding a loop's tc_loop (tcport.h: the replay's waits, playbook 80, M5); the
        caller closes it after the loop."""
        self.loops += 1
        lp = "lp%d_" % self.loops
        self.emit(depth, "{ tc_loop %s = {0, 0, 0, 0};" % lp)
        return lp

    def block(self, n, depth: int):
        if n["kind"] == "CompoundStmt":
            self.stmt(n, depth + 1)
        else:
            self.emit(depth, "{")
            self.stmt(n, depth + 1)
            self.emit(depth, "}")

    def local(self, d, depth: int):
        if d["kind"] != "VarDecl" or d.get("storageClass") in ("static", "extern"):
            self.fail("local declaration %s %s" % (d["kind"], d.get("storageClass")))
        t = ctype(d["type"])
        v = self.vars[d["id"]]
        init = d["inner"][0] if d.get("inner") else None
        if v[0] == "var":
            # no initialiser: the original's value is what the stack held; the port's is 0 [H]
            self.emit(depth, "%s %s = %s;" % (self.cname(t), d["name"], self.rv(init) if init else "0"))
            return
        if init is not None:
            for s in self.initialise("tc_fp_add(fr_.base, 0x%X)" % v[1], t, init):
                self.emit(depth, s + ";")

    def initialise(self, addr: str, t, init) -> list[str]:
        """Stores that give an object in the frame its initial value (an array or struct from its
        initialiser list, a char array from a string, a scalar from its expression)."""
        k = init["kind"]
        if t[0] == "arr" and strip(init)["kind"] == "StringLiteral":
            b = literal_bytes(strip(init)["value"])
            n = self.size(t)
            b = (b + b"\0" * n)[:n]
            return ["tc_st_u8(tc_fp_add(%s, %d), %d)" % (addr, i, x) for i, x in enumerate(b)]
        if k == "InitListExpr":
            items = init.get("inner", [])
            out = []
            if t[0] == "arr":
                el, n = t[1], t[2]
                es = self.size(el)
                for i in range(n):
                    a = "tc_fp_add(%s, %d)" % (addr, i * es)
                    if i < len(items):
                        out += self.initialise(a, el, items[i])
                    else:
                        out.append("tc_zero(%s, %d)" % (a, es))
                return out
            if t[0] == "struct":
                offs, _size = self.layout(t[1])
                fields = self.records[t[1]]["fields"]
                if self.records[t[1]]["union"]:
                    fields = fields[:1]
                for i, (fid, _fn, ft) in enumerate(fields):
                    a = "tc_fp_add(%s, %d)" % (addr, offs[fid][0])
                    if i < len(items):
                        out += self.initialise(a, ft, items[i])
                    else:
                        out.append("tc_zero(%s, %d)" % (a, self.size(ft)))
                return out
            self.fail("initialiser list for %r" % (t,))
        if k == "ImplicitValueInitExpr":
            return ["tc_zero(%s, %d)" % (addr, self.size(t))]
        return [self.store(t, addr, self.rv(init))]

    def image_of(self, t, init, out: bytearray, ptrs: list, off: int) -> None:
        """A file-scope definition's initial value (step D3): its constant bytes into out at off, and
        each far pointer that needs an address as (offset, expression), stored after the bytes."""
        k = init["kind"]
        s = strip(init)
        if t[0] == "arr" and s["kind"] == "StringLiteral":
            b = literal_bytes(s["value"])
            n = self.size(t)
            out[off:off + n] = (b + b"\0" * n)[:n]
            return
        if k == "InitListExpr":
            items = init.get("inner", [])
            if t[0] == "arr":
                es = self.size(t[1])
                for i, it in enumerate(items[:t[2]]):
                    self.image_of(t[1], it, out, ptrs, off + i * es)
                return
            if t[0] == "struct":
                offs, _size = self.layout(t[1])
                fields = self.records[t[1]]["fields"]
                if self.records[t[1]]["union"]:
                    fields = fields[:1]
                for (fid, _fn, ft), it in zip(fields, items):
                    self.image_of(ft, it, out, ptrs, off + offs[fid][0])
                return
            self.fail("initialiser list for %r" % (t,))
        if k == "ImplicitValueInitExpr":
            return
        n = self.size(t)
        if t[0] == "int":
            v = self.const(init)
            out[off:off + n] = (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little")
            return
        try:
            v = self.const(init)                 # a null pointer, or a number cast to a pointer
        except Unsupported:
            v = None
        except (KeyError, TypeError):
            v = None
        if v is not None:
            out[off:off + n] = (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little")
            return
        if n != 4:
            self.fail("a near pointer with an address in an initialiser")
        ptrs.append((off, self.rv(init)))

    def data_definitions(self, ast: dict) -> list[str]:
        """The unit's initialised data (step D3) as stores into the arena: Turbo C writes a module's
        _DATA as its file-scope definitions and the templates of its functions' initialised local
        arrays and structs, in source order, then its string literals, in source order; so the layout
        runs from the start of the module's span (re/datamap.toml) and each definition's annotation
        and each literal's recorded address must fall where it says. Far pointers, which need the load
        segment, are stored after the bytes."""
        span = self.prog.spans.get(self.name.lower())
        items = []                                   # (address, size, bytes, pointers, what)
        cur = span[0] if span else None

        def place(at, t, init, what):
            nonlocal cur
            img, ptrs = bytearray(self.size(t)), []
            self.image_of(t, init, img, ptrs, 0)
            if cur is not None and at is not None and at != cur:
                self.fail("%s is annotated ds:%04X, but the unit's data puts it at ds:%04X" % (what, at, cur))
            at = cur if at is None else at
            if at is None:
                self.fail("%s: the unit has no span in re/datamap.toml" % what)
            items.append((at, img, ptrs, what))
            cur = at + len(img)

        def templates(n):
            if not isinstance(n, dict):
                return
            if n.get("kind") == "DeclStmt":
                for d in n.get("inner", []):
                    if (d.get("kind") == "VarDecl" and d.get("inner") and d.get("storageClass") not in ("static", "extern")
                            and ctype(d["type"])[0] in ("arr", "struct")):
                        self.line = d.get("loc", {}).get("line", self.line)
                        place(None, ctype(d["type"]), d["inner"][0], "the template of %s" % d["name"])
                return
            for c in n.get("inner", []) or []:
                templates(c)

        for d in ast["inner"]:
            if d.get("loc", {}).get("includedFrom") or d.get("isImplicit"):
                continue
            if d["kind"] == "FunctionDecl":
                body = [x for x in d.get("inner", []) if x["kind"] == "CompoundStmt"]
                if body:
                    templates(body[0])
                continue
            if d["kind"] != "VarDecl" or d.get("storageClass") in ("extern",) or not d.get("inner"):
                continue
            name = d["name"]
            if d.get("storageClass") == "static":
                self.fail("static definition %s" % name)
            if name not in self.data:
                self.fail("definition %s has no ds annotation" % name)
            self.line = d.get("loc", {}).get("line", self.line)
            place(self.data[name], ctype(d["type"]), d["inner"][0], name)
        whole = "/*@data */" in self.path.read_text(errors="replace")
        if span is not None and whole:
            for lid, b in unit_literals(ast):
                at = self.literals.get(lid)
                if at != cur:
                    self.fail("the literal %r is at ds:%s, but the unit's pool puts it at ds:%04X"
                              % (b, "%04X" % at if at is not None else "?", cur))
                items.append((cur, bytearray(b + b"\0"), [], "a literal"))
                cur += len(b) + 1
            end = cur + (cur & 1)
            if end != span[1]:
                self.fail("the unit's data ends at ds:%04X, its span in re/datamap.toml at ds:%04X" % (end, span[1]))
        blobs, body = [], []
        for i, (at, img, ptrs, what) in enumerate(items):
            sym = "tc2c_data_%s_%d" % (self.name, i)
            rows = [", ".join("0x%02X" % x for x in img[k:k + 16]) for k in range(0, len(img), 16)]
            blobs.append("/* ds:%04X: %s */\nstatic const u8 %s[%d] = {\n    %s\n};" % (
                at, what, sym, len(img), ",\n    ".join(rows)))
            body.append("    tc_put(TC_MKFP(tc_ds, 0x%04X), %s, %du);" % (at, sym, len(img)))
            for o, e in ptrs:
                body.append("    tc_st_u32(TC_MKFP(tc_ds, 0x%04X), (u32)%s);" % ((at + o) & 0xFFFF, e))
        return blobs + ["", "void tc2c_data_%s(void)" % self.name, "{"] + body + ["}"]

    # -- the whole unit
    def proto(self, name: str, t, params: list[str] | None = None) -> str:
        if t[0] != "func":
            self.fail("%s is not a function" % name)
        ps = [self.cname(p) for p in t[2]]
        if t[3]:
            ps += ["const u8 *va_", "u16 va_size_"]
        if params is not None:
            ps = ["%s %s" % (c, p) for c, p in zip(ps, params)]
        return "%s %s(%s)" % (self.cname(t[1]), self.fn_name(name), ", ".join(ps) or "void")

    def scan_frame(self, d) -> None:
        """Which locals and parameters need memory: arrays, structs, and those whose address is
        taken. Each gets an offset in the function's frame on the arena stack."""
        taken: set[str] = set()

        def walk(n):
            if not isinstance(n, dict):
                return
            if n.get("kind") == "UnaryOperator" and n["opcode"] == "&":
                s = strip(n["inner"][0])
                if s["kind"] == "DeclRefExpr":
                    taken.add(s["referencedDecl"]["id"])
            if n.get("kind") == "LabelStmt":
                self.labels[n["declId"]] = n["name"]
            for c in n.get("inner", []) or []:
                walk(c)

        walk(d)
        self.frame = 0
        self.vars = {}

        def decl(x):
            t = ctype(x["type"])
            if x["kind"] == "ParmVarDecl" and t[0] == "struct":
                self.fail("a struct passed by value (%s)" % x.get("name"))
            if t[0] in ("arr", "struct") or x["id"] in taken:
                self.vars[x["id"]] = ("mem", self.frame, t)
                self.frame += self.size(t)
            else:
                self.vars[x["id"]] = ("var", x.get("name") or "p%d_" % len(self.vars), t)

        def find(n):
            if not isinstance(n, dict):
                return
            if n.get("kind") == "VarDecl":
                decl(n)
            for c in n.get("inner", []) or []:
                find(c)

        for p in d.get("inner", []):
            if p["kind"] == "ParmVarDecl":
                decl(p)
        for b in d.get("inner", []):
            if b["kind"] == "CompoundStmt":
                find(b)

    def translate(self, ast: dict) -> str:
        bodies = []
        for d in ast["inner"]:
            if d.get("isImplicit") or d["kind"] != "FunctionDecl":
                continue
            body = [x for x in d.get("inner", []) if x["kind"] == "CompoundStmt"]
            if not body:
                continue
            name = d["name"]
            self.line = d.get("loc", {}).get("line", self.line)
            if name not in self.defined:
                self.fail("%s is defined but has no address annotation" % name)
            if d.get("storageClass") == "static":
                self.fail("static function %s" % name)
            t = ctype(d["type"])
            self.labels = {}
            self.scan_frame(d)
            params = [p for p in d.get("inner", []) if p["kind"] == "ParmVarDecl"]
            pnames = []
            for p in params:
                v = self.vars[p["id"]]
                pnames.append(v[1] if v[0] == "var" else "%s_in_" % p.get("name", "p"))
            self.out = []
            seg, off = self.funcs[name]
            self.out.append("/* %s %s:%04X */" % (self.path.name, seg if isinstance(seg, str) else "%04X" % seg, off))
            self.out.append(self.proto(name, t, pnames))
            self.out.append("{")
            if self.frame:
                self.out.append("    tc_frame fr_ __attribute__((cleanup(tc_frame_leave))) = tc_frame_enter(%du);"
                                % self.frame)
                for p, pn in zip(params, pnames):
                    v = self.vars[p["id"]]
                    if v[0] == "mem":
                        self.out.append("    %s;" % self.store(v[2], "tc_fp_add(fr_.base, 0x%X)" % v[1], pn))
            # a label is where a backward goto loops: its tc_loop lives as long as the function
            for lab in sorted(set(self.labels.values())):
                self.out.append("    tc_loop lp_%s_ = {0, 0, 0, 0};" % lab)
            self.loops = 0
            n_head = len(self.out)
            self.blocks = []
            self.stmt(body[0], 2)
            # the body's own braces are the function's
            self.out = self.out[:n_head] + ["    " + b for b in self.blocks] + self.out[n_head + 1:-1] + ["}"]
            bodies.append("\n".join(self.out))
        head = ["/* Generated by tools/port/tc2c.py from src/%s: do not edit. */" % self.path.name,
                '#include "tcport.h"', '#include "tclib.h"', '#include "ported.h"', ""]
        init = ["void tc2c_init_%s(void)" % self.name, "{"]
        for name in sorted(self.defined):
            a = self.code_addr(name)
            if a is not None:
                init.append("    tc_code_register(%s, (tc_fn)%s, \"%s\");" % (a, name, name))
        init.append("}")
        data = self.data_definitions(ast)
        return "\n".join(head) + "\n" + "\n\n".join(bodies) + "\n\n" + "\n".join(init) + "\n\n" + \
            "\n".join(data) + "\n"


def records_of(ast: dict) -> dict:
    """Every struct and union with a definition, nested ones too; a nested record without a tag
    takes the name of the type of the field declared with it (the next field, in clang's dump)."""
    out = {}

    def record(d, name):
        fields, pending = [], None
        for f in d.get("inner", []):
            if f["kind"] == "RecordDecl" and f.get("completeDefinition"):
                if f.get("name"):
                    record(f, f["name"])
                else:
                    pending = f
            elif f["kind"] == "FieldDecl":
                ft = ctype(f["type"])
                if pending is not None:
                    base = ft
                    while base[0] == "arr":
                        base = base[1]
                    if base[0] != "struct":
                        raise Unsupported("an untagged record used as %r" % (ft,))
                    record(pending, base[1])
                    pending = None
                fields.append((f["id"], f["name"], ft))
        out[name] = {"fields": fields, "ids": {f[0] for f in fields}, "offsets": {},
                     "union": d.get("tagUsed") == "union"}

    untagged = {}

    def walk(n):
        for d in n.get("inner", []) or []:
            if not isinstance(d, dict):
                continue
            if d.get("kind") == "RecordDecl" and d.get("completeDefinition") and d.get("name"):
                record(d, d["name"])
            elif d.get("kind") == "RecordDecl" and d.get("completeDefinition"):
                untagged[d["id"]] = d
            elif d.get("kind") == "TypedefDecl":
                # `typedef union { ... } WorkBuffer;`: clang names the type by the typedef
                for x in d.get("inner", []):
                    own = x.get("ownedTagDecl")
                    if own and own["id"] in untagged:
                        record(untagged[own["id"]], d["name"])
            elif d.get("kind") in ("FunctionDecl", "CompoundStmt", "DeclStmt"):
                walk(d)

    walk(ast)
    return out


def ret_macros() -> list[str]:
    """CALL_<type>_(call): the value of a call as the caller's declaration types it."""
    return ["#define CALL_%s_(c) ((%s)(c))" % (t, t) for t in ("u8", "s8", "u16", "s16", "u32", "s32", "farptr")]


def hybrid_table(units, prog, handwritten) -> str:
    """DIR/hybrid.c, for the hybrid build only: for each defined function, a thunk that reads its
    arguments where the original's caller pushed them (SS:SP+4 at the entry, a word for a char or an
    int, two for a long or a far pointer) and returns its value as DX:AX holds it; and the table of
    the entries the per-function test stops at (tools/port/hybrid.py): a resident function's own
    address, an overlay function's stub entry (none for an overlay function with no stub entry)."""
    out = ["/* Generated by tools/port/tc2c.py: the per-function test's entry table (hybrid build). */",
           '#include "tcport.h"', '#include "tclib.h"', '#include "ported.h"', '#include "hybrid.h"', ""]
    rows = []
    any_unit = units[0][0]
    todo = [(u, name, prog.defs[name], u.funcs[name], u.path.name, 0) for u, _ast in units
            for name in sorted(u.defined) if u.path.parent.name != "intro"]     # the intro has no entries on the bench
    todo += [(any_unit, name, t, prog.handwritten[name][:2], "port/asm/" + prog.handwritten[name][2],
              int(prog.handwritten[name][3])) for name, t in handwritten]
    for u, name, t, (seg, off), where, isr in todo:
        ovl, code_off = -1, 0
        if isinstance(seg, str):
            ovl, code_off = int(seg[3:]), off
            stub = prog.stubs.get((ovl, off))
            seg, off = stub if stub is not None else (0xFFFF, 0xFFFF)
        args, o = [], 0
        own = t[0] == "own"
        for x in t[2]:
            w = (4 if OWN_SIZE[x] > 2 else 2) if own else (4 if u.size(x) > 2 else 2)
            args.append("(%s)tc_ld_u%d(tc_fp_add(a_, %d))" % (x if own else u.cname(x), 8 * w, o))
            o += w
        call = "%s(%s)" % (name if own else u.fn_name(name), ", ".join(args))
        out.append("static u32 th_%s(void)" % name)
        out.append("{")
        out.append("    farptr a_ = TC_MKFP(tc_ss, (u16)(tc_sp + 4u));")
        if not args:
            out.append("    (void)a_;")
        if (t[1] if own else t[1][0]) == "void":
            out += ["    %s;" % call, "    return 0;"]
            rb = 0
        else:
            rb = OWN_SIZE[t[1]] if own else u.size(t[1])
            out.append("    return (u32)(u%d)%s;" % (8 * rb, call))
        out += ["}", ""]
        rows.append('    {0x%04X, 0x%04X, %d, 0x%04X, "%s", "%s", th_%s, %d, %d, %d},'
                    % (seg, off, ovl, code_off, name, where, name, rb, o, isr))
    # the library functions the runtime has on the machine (port/runtime), at the library's addresses
    for name, (seg, off, where, (ret, params)) in sorted(prog.libentries.items()):
        args, o = [], 0
        for x in params:
            if x == "va":
                # a variadic function's arguments: as many bytes as a caller could push, read from the
                # stack where they would lie (open's mode)
                args += ["va_", "(u16)sizeof va_"]
                continue
            w = 4 if OWN_SIZE[x] > 2 else 2
            args.append("(%s)tc_ld_u%d(tc_fp_add(a_, %d))" % (x, 8 * w, o))
            o += w
        out.append("static u32 th_%s(void)" % name)
        out.append("{")
        out.append("    farptr a_ = TC_MKFP(tc_ss, (u16)(tc_sp + 4u));")
        if "va" in params:
            out += ["    u8 va_[8];", "    unsigned i_;", "    for (i_ = 0; i_ < sizeof va_; i_++) {",
                    "        va_[i_] = tc_ld_u8(tc_fp_add(a_, (u16)(%d + i_)));" % o, "    }"]
        if not args:
            out.append("    (void)a_;")
        call = "%s(%s)" % (name, ", ".join(args))
        if ret == "void":
            out += ["    %s;" % call, "    return 0;"]
            rb = 0
        else:
            rb = OWN_SIZE[ret]
            out.append("    return (u32)(u%d)%s;" % (8 * rb, call))
        out += ["}", ""]
        rows.append('    {0x%04X, 0x%04X, -1, 0x0000, "%s", "port/runtime/%s", th_%s, %d, %d, 0},'
                    % (seg, off, name, where, name, rb, o))
    out.append("const tc_hybrid_entry tc_hybrid_entries[] = {")
    out += rows
    out += ["};", "const unsigned tc_hybrid_entry_count = %d;" % len(rows), ""]
    return "\n".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--image", type=Path, default=ROOT / "eob" / "EOB.EXE",
                    help="the original, whose DGROUP places the string literals (read only)")
    ap.add_argument("--layout", type=Path, default=ROOT / "build" / "re" / "eob.json",
                    help="TLINK's segment table and the overlay stubs (make overlays)")
    ap.add_argument("--asm", type=Path, nargs="*", default=None,
                    help="the files of port/asm to register (default: every one; the build passes its list)")
    ap.add_argument("--lib", type=Path, nargs="*", default=None,
                    help="the runtime's files with library functions on the machine (default: LIB_C)")
    ap.add_argument("units", nargs="+", type=Path)
    a = ap.parse_args(argv)
    a.out.mkdir(parents=True, exist_ok=True)
    prog = Program(a.image if a.image.exists() else None, a.layout if a.layout.exists() else None, a.asm,
                   a.lib)
    done = []
    try:
        units = []
        for p in a.units:
            u = Unit(p, prog)
            ast = u.ast()
            units.append((u, ast))
        for u, ast in units:
            u.prepare(ast)
        if any(u.overlay is not None for u, _ in units) and not prog.stubs:
            raise Unsupported("overlay units need --layout (make overlays)")
        prog.library -= set(prog.defs)
        prog.resolve_aliases()
        for u, ast in units:
            text = u.translate(ast)
            write_if_changed(a.out / ("%s.c" % u.name), text)
            done.append(u.name)
    except Unsupported as e:
        print("tc2c: unsupported: %s" % e, file=sys.stderr)
        return 2
    any_unit = units[0][0]
    tab = ["/* Generated by tools/port/tc2c.py: the translated units. */", '#include "tcport.h"', ""]
    tab += ["void tc2c_init_%s(void);" % n for n in done]
    tab += ["void tc2c_data_%s(void);" % n for n in done]
    tab += ["", "const char *const tc2c_units[] = {%s};" % ", ".join('"%s"' % n for n in done),
            "const unsigned tc2c_unit_count = %d;" % len(done), "", "void tc2c_init(void)", "{"]
    tab += ["    tc2c_init_%s();" % n for n in done]
    tab += ["    /* the assembly routines in C (port/asm), at their original addresses */"]
    tab += ['    tc_code_register(TC_MKFP(tc_load_seg + 0x%04X, 0x%04X), (tc_fn)%s, "%s");' % (sg, of, n, n)
            for n, (sg, of, *_x) in sorted(prog.handwritten.items())]
    # the assembly units' data (step D3): each module of the data map whose file is assembly and defines
    # its data, and E1948's driver segment at ds:88A0
    import asmdata
    asm_units = []
    for mid, (lo, hi) in sorted(prog.spans.items(), key=lambda kv: kv[1]):
        f = ROOT / "src" / (mid.upper() + ".ASM")
        if f.exists() and re.search(r"^\s*;\s*@data\b", f.read_text(errors="replace"), re.M):
            asm_units.append((mid.upper(), f, lo, hi, None))
    asm_units.append(("E1948_SND", ROOT / "src" / "E1948.ASM", 0x88A0, 0x8A6C, "SND"))
    try:
        tab += ["}", ""] + asmdata.c_source(asm_units)
    except asmdata.AsmDataError as e:
        print("tc2c: unsupported: %s" % e, file=sys.stderr)
        return 2
    # the assembly units' data in their code segments (port/asmdata.json, tools/port/asmlist.py: from
    # TASM's listing of src/, each run checked against EOB.EXE when it was written)
    code = json.loads(ASMDATA.read_text())["units"] if ASMDATA.exists() else {}
    blob = []
    for unit, rows in sorted(code.items()):
        for k, (sg, of, hx) in enumerate(rows):
            b = bytes.fromhex(hx)
            sym = "tc2c_code_%s_%d" % (unit, k)
            lines = [", ".join("0x%02X" % x for x in b[j:j + 16]) for j in range(0, len(b), 16)]
            tab += ["/* %s, %s:%s */" % (unit, sg, of), "static const u8 %s[%d] = {" % (sym, len(b)),
                    "    " + ",\n    ".join(lines), "};"]
            blob.append("    tc_put(TC_MKFP(tc_load_seg + 0x%s, 0x%s), %s, %du);" % (sg, of, sym, len(b)))
    # Westwood's far data (src/far/FARDATA.ASM, playbook 80, M6): each FAR_SSSS segment at SSSS:0000
    farsrc = ROOT / "src" / "far" / "FARDATA.ASM"
    if farsrc.exists():
        for seg in re.findall(r"^(FAR_([0-9A-F]{4}))\s+SEGMENT\b", farsrc.read_text(errors="replace"), re.M):
            b, ptrs, _ = asmdata.module_data(farsrc, 0, seg[0])
            if ptrs:
                print("tc2c: unsupported: far pointers in %s" % seg[0], file=sys.stderr)
                return 2
            sym = "tc2c_far_%s" % seg[1]
            lines = [", ".join("0x%02X" % x for x in b[j:j + 16]) for j in range(0, len(b), 16)]
            tab += ["/* src/far/FARDATA.ASM, %s:0000 */" % seg[1], "static const u8 %s[%d] = {" % (sym, len(b)),
                    "    " + ",\n    ".join(lines), "};"]
            blob.append("    tc_put(TC_MKFP(tc_load_seg + 0x%s, 0x0000), %s, %du);" % (seg[1], sym, len(b)))
    tab += ["", "/* the assembly units' data in their code segments (port/asmdata.json) and Westwood's far data */",
            "void tc2c_data_code(void)", "{"] + blob + ["}"]
    tab += ["", "/* step D3: the initial values of the units' definitions, at their DGROUP addresses */",
            "void tc2c_data(void)", "{"]
    tab += ["    tc2c_data_%s();" % n for n in done]
    tab += ["    tc2c_data_asm();", "    tc2c_data_code();", "}"]
    write_if_changed(a.out / "units.c", "\n".join(['#include "ported.h"'] + tab) + "\n")

    def kind(n):
        return "library" if n in prog.library else ("assembly" if n in prog.asm else "C")

    def decl_of(n):
        t = prog.canonical(n)
        return ("func", t[1], [], False) if t[3] is None else t

    # one prototype per function: its definition's, else the first declaration seen
    missing = sorted(n for n in prog.called if n not in prog.defs and n not in prog.alias
                     and not (n in prog.library and n in prog.runtime) and n not in prog.handwritten)
    handwritten = sorted(n for n in prog.called if n in prog.handwritten and n not in prog.defs)
    isrs = sorted(n for n, h in prog.handwritten.items() if h[3])
    # in the per-function test's table: every routine of port/asm (the prototype of the C units that call
    # it, else its own definition's), and the handlers
    tested = sorted(n for n in prog.handwritten if n not in prog.defs)
    hdr = ["/* Generated by tools/port/tc2c.py: one prototype for each function the translated units",
           "   define or call (the library's that the runtime provides are in tclib.h). */",
           "#ifndef TC2C_PORTED_H", "#define TC2C_PORTED_H", '#include "tcport.h"', ""]
    hdr += ret_macros() + ["", "/* defined by the translated units */"]
    for name in sorted(prog.defs):
        hdr.append("%s;" % any_unit.proto(name, prog.defs[name]))
    hdr += ["", "/* the assembly routines in C (port/asm) */"]
    for name in handwritten:
        hdr.append("%s;" % any_unit.proto(name, decl_of(name)))
    for name in tested:
        if name not in handwritten and not prog.handwritten[name][3]:
            hdr.append("%s;" % (any_unit.proto(name, decl_of(name)) if prog.canonical(name)
                                else prog.handwritten[name][4][2]))
    hdr += ["", "/* the interrupt handlers in C (port/asm) */"]
    for name in isrs:
        hdr.append("void %s(void);" % name)
    hdr += ["", "/* called, defined by no translated unit (unported.c) */"]
    for name in missing:
        hdr.append("%s;" % any_unit.proto(name, decl_of(name)))
    hdr += ["", "void tc2c_init(void);", "void tc2c_data(void);", "extern const char *const tc2c_units[];",
            "extern const unsigned tc2c_unit_count;", "", "#endif"]
    write_if_changed(a.out / "ported.h", "\n".join(hdr) + "\n")
    stubs = ["/* Generated by tools/port/tc2c.py: functions the translated units call that no translated",
             "   unit and no runtime defines yet: the assembly units (port/asm, M3) and the library",
             "   functions the runtime does not have yet. Each reports itself and stops (tc_unported); in",
             "   the hybrid build (TC_HYBRID, tools/port/hybrid.py) each runs the original's routine. */",
             '#include "tcport.h"', '#include "ported.h"', '#include "hybrid.h"', ""]
    registered = []
    for name in missing:
        t = decl_of(name)
        ps = ["%s a%d" % (any_unit.cname(p), i) for i, p in enumerate(t[2])]
        if t[3]:
            ps += ["const u8 *va_", "u16 va_size_"]
        rc = any_unit.cname(t[1])
        stubs.append("%s %s(%s)" % (rc, any_unit.fn_name(name), ", ".join(ps) or "void"))
        stubs.append("{")
        at = prog.where.get(name) or LIBRARY_AT.get(name)
        if at is not None and not at.startswith("ovl"):
            seg, off = (int(x, 16) for x in at.split(":"))
            stubs.append("#ifdef TC_HYBRID")
            fixed = sum(4 if any_unit.size(x) > 2 else 2 for x in t[2])
            stubs.append("    u8 b_[%d];" % (fixed + (256 if t[3] else 0) or 1))
            o = 0
            for i, x in enumerate(t[2]):
                w = 4 if any_unit.size(x) > 2 else 2
                stubs.append("    tc_va%d(b_ + %d, (u%d)a%d);" % (8 * w, o, 8 * w, i))
                o += w
            n = str(o)
            if t[3]:
                stubs.append("    for (u16 i_ = 0; i_ < va_size_ && i_ < 256u; i_++) b_[%d + i_] = va_[i_];" % o)
                n = "(u16)(%d + va_size_)" % o
            call = "tc_hybrid_far(TC_MKFP(tc_load_seg + 0x%04X, 0x%04X), b_, %s)" % (seg, off, n)
            if t[1][0] == "void":
                stubs.append("    %s;" % call)
            else:
                w = any_unit.size(t[1])
                stubs.append("    return (%s)(u%d)%s;" % (rc, 8 * w, call))
            stubs.append("#else")
            if not t[3]:
                registered.append((seg, off, any_unit.fn_name(name)))
        for p in ps:
            stubs.append("    (void)%s;" % p.split()[-1].lstrip("*"))
        stubs.append('    tc_unported("%s", "%s, %s");' % (name, prog.where.get(name, "no address"), kind(name)))
        if t[1][0] != "void":
            stubs.append("    return 0;")
        if at is not None and not at.startswith("ovl"):
            stubs.append("#endif")
        stubs.append("}")
        stubs.append("")
    # in the hybrid build a far pointer to one of these (a driver slot, a handler) finds the stub
    stubs += ["#ifdef TC_HYBRID", "void tc2c_init_unported(void)", "{"]
    stubs += ['    tc_code_register(TC_MKFP(tc_load_seg + 0x%04X, 0x%04X), (tc_fn)%s, "%s");' % (sg, of, fn, fn)
              for sg, of, fn in registered]
    stubs += ["}", "#endif", ""]
    write_if_changed(a.out / "unported.c", "\n".join(stubs))
    isr_type = ("func", ("void",), [], False)
    write_if_changed(a.out / "hybrid.c", hybrid_table(
        units, prog, [(n, isr_type if prog.handwritten[n][3] else decl_of(n) if prog.canonical(n)
                       else ("own",) + prog.handwritten[n][4][:2]) for n in tested]))
    kinds = {k: sum(1 for n in missing if kind(n) == k) for k in ("library", "assembly", "C")}
    print("tc2c: %d units translated, %d functions; %d assembly routines in C (port/asm); still unported: "
          "%d library, %d assembly, %d C" % (len(done), len(prog.defs), len(handwritten), kinds["library"],
                                               kinds["assembly"], kinds["C"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
