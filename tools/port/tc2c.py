#!/usr/bin/env python3
"""tc2c.py: translate matched Turbo C units of src/ into portable C for the arena (playbook 80).

The matched C stays as it is. clang reads a unit as C for the MSP430 target (16-bit int, 32-bit
long, the widths of Turbo C++), with signed char and byte-packed structs (port/include/tc/prelude.h),
and dumps its typed AST as JSON. This tool walks that AST and writes C99 in which

  - every value has an exact width (u8 s8 u16 s16 u32 s32) and every conversion the 16-bit
    compiler made implicitly is a cast, so int arithmetic wraps at 16 bits as on the 8086;
  - every pointer is a far pointer (farptr, segment:offset in a u32, large model), and every load
    and store of memory goes through the arena helpers of port/include/tcport.h at the address the
    original uses: a global at its `ds:` annotation, a struct field at its Turbo C offset (bytes,
    no padding), an array element at index * size added to the offset only;
  - a call through a far function pointer finds its C function by the original code address
    (tc_code_lookup), and each defined function registers its own address (`@ SSSS:OOOO`).

The subset handled is what the units listed in port/units.txt use; anything else stops the tool
with the construct and its line, so a new unit grows the tool, never a silent guess.

  tc2c.py --out DIR UNIT.C ...      write DIR/<unit>.c, DIR/units.c (init table), DIR/ported.h
                                    (their prototypes) and DIR/unported.c (a stub for each called
                                    function no unit defines)
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "match"))

import cmatch  # noqa: E402  (read-only use: the `/*@ */` annotation parser)

PRELUDE = ROOT / "port" / "include" / "tc" / "prelude.h"
CLANG_ARGS = ["-x", "c", "-std=gnu89", "-target", "msp430", "-fsigned-char", "-fsyntax-only",
              "-Wno-pragma-pack", "-Xclang", "-ast-dump=json"]


class Unsupported(Exception):
    pass


# ---- types ---------------------------------------------------------------------------------------
# ("int", size, signed) | ("ptr", target) | ("arr", elem, n or None) | ("struct", tag of a struct or union)
# | ("func", ret, [params], variadic) | ("void",)

INT_NAMES = {
    "char": (1, True), "signed char": (1, True), "unsigned char": (1, False),
    "short": (2, True), "short int": (2, True), "signed short": (2, True), "unsigned short": (2, False),
    "int": (2, True), "signed int": (2, True), "signed": (2, True), "unsigned int": (2, False),
    "unsigned": (2, False), "long": (4, True), "long int": (4, True), "signed long": (4, True),
    "unsigned long": (4, False), "unsigned long int": (4, False),
}
TOKEN = re.compile(r"\s*(\.\.\.|[A-Za-z_]\w*|\d+|[*()\[\],])")


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
        self.t = tokens(s)
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
        while self.peek() in ("const", "volatile"):
            self.take()
        if self.peek() in ("struct", "union"):
            self.take()
            name = self.take()
            while self.peek() in ("const", "volatile"):
                self.take()
            return ("struct", name)
        if self.peek() == "enum":
            self.take()
            self.take()
            return ("int", 2, True)
        while self.peek() in ("signed", "unsigned", "char", "short", "int", "long", "void", "const",
                              "volatile"):
            w = self.take()
            if w not in ("const", "volatile"):
                words.append(w)
        if words == ["void"]:
            return ("void",)
        key = " ".join(words)
        if key not in INT_NAMES:
            raise Unsupported("base type %r" % key)
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
            while self.peek() in ("const", "volatile"):
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


# ---- the unit ------------------------------------------------------------------------------------

class Unit:
    def __init__(self, path: Path, records: dict, defined_everywhere: set):
        self.path = path
        self.name = path.stem.upper()
        self.src = path.read_text(encoding="latin-1")
        code_only = re.sub(r"/\*.*?\*/", "", self.src, flags=re.S)
        if re.search(r"\binterrupt\b", code_only):
            raise Unsupported("%s: interrupt functions (playbook 80, open question 6)" % path.name)
        self.funcs, self.data, self.defined, stubs = cmatch.parse_annotations(self.src)
        self.fardata = set(cmatch.FARDATA)
        if stubs:
            raise Unsupported("%s: stub functions %s" % (path.name, sorted(stubs)))
        self.records = records
        self.everywhere = defined_everywhere
        self.out: list[str] = []
        self.tmp = 0
        self.locals: dict[str, tuple] = {}
        self.decls: dict[str, tuple] = {}  # function name -> type
        self.called: set[str] = set()
        self.line = 0

    # -- clang
    def ast(self) -> dict:
        clang = shutil.which("clang") or shutil.which("cc")
        cmd = [clang, *CLANG_ARGS, "-include", str(PRELUDE), "-I", str(self.path.parent), str(self.path)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            raise Unsupported("clang failed on %s:\n%s" % (self.path.name, r.stderr))
        return json.loads(r.stdout)

    # -- layout (Turbo C: bytes, no padding; every pointer four bytes)
    def size(self, t) -> int:
        k = t[0]
        if k == "int":
            return t[1]
        if k == "ptr":
            return 4
        if k == "arr":
            if t[2] is None:
                raise Unsupported("size of an array of unknown size")
            return self.size(t[1]) * t[2]
        if k == "struct":
            return self.layout(t[1])[1]
        raise Unsupported("size of %r" % (t,))

    def layout(self, name: str):
        rec = self.records.get(name)
        if rec is None:
            raise Unsupported("struct %s has no definition" % name)
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
        raise Unsupported("member %s" % member_id)

    # -- C names of types
    @staticmethod
    def cname(t) -> str:
        if t[0] == "int":
            return ("s%d" if t[2] else "u%d") % (8 * t[1])
        if t[0] in ("ptr",):
            return "farptr"
        if t[0] == "void":
            return "void"
        raise Unsupported("a value of type %r" % (t,))

    def load(self, t, addr: str) -> str:
        if t[0] == "int":
            return "tc_ld_%s(%s)" % (self.cname(t), addr)
        if t[0] == "ptr":
            return "tc_ld_u32(%s)" % addr
        raise Unsupported("load of %r" % (t,))

    def store(self, t, addr: str, val: str) -> str:
        if t[0] == "int":
            return "tc_st_%s(%s, %s)" % (self.cname(t), addr, val)
        if t[0] == "ptr":
            return "tc_st_u32(%s, %s)" % (addr, val)
        raise Unsupported("store of %r" % (t,))

    def global_addr(self, name: str) -> str:
        if name in self.data:
            return "TC_MKFP(tc_ds, 0x%04X)" % self.data[name]
        if name in self.fardata:
            seg, off = self.funcs[name]
            return "TC_MKFP(tc_load_seg + 0x%04X, 0x%04X)" % (seg, off)
        raise Unsupported("global %s has no address annotation" % name)

    def code_addr(self, name: str) -> str:
        if name not in self.funcs or isinstance(self.funcs[name][0], str):
            raise Unsupported("code address of %s (no resident annotation)" % name)
        seg, off = self.funcs[name]
        return "TC_MKFP(tc_load_seg + 0x%04X, 0x%04X)" % (seg, off)

    def fresh(self) -> str:
        self.tmp += 1
        return "t%d_" % self.tmp

    # -- lvalues: ("mem", addr, type) or ("var", cname, type)
    def lv(self, n):
        k = n["kind"]
        t = ctype(n["type"])
        if k == "ParenExpr":
            return self.lv(n["inner"][0])
        if k == "DeclRefExpr":
            d = n["referencedDecl"]
            if d["kind"] in ("ParmVarDecl", "VarDecl") and d["name"] in self.locals:
                return ("var", d["name"], t)
            if d["kind"] == "VarDecl":
                return ("mem", self.global_addr(d["name"]), t)
            raise Unsupported("reference to %s %s" % (d["kind"], d.get("name")))
        if k == "MemberExpr":
            off, ft = self.field(n["referencedMemberDecl"])
            base = n["inner"][0]
            if n["isArrow"]:
                p = self.rv(base)
            else:
                b = self.lv(base)
                if b[0] != "mem":
                    raise Unsupported("member of a local struct (needs the arena stack)")
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
        raise Unsupported("lvalue %s" % k)

    def rd(self, lval) -> str:
        if lval[0] == "var":
            return lval[1]
        return self.load(lval[2], lval[1])

    # -- rvalues
    def rv(self, n) -> str:
        k = n["kind"]
        if "loc" in n and "line" in n["loc"]:
            self.line = n["loc"]["line"]
        m = getattr(self, "x_" + k, None)
        if m is None:
            raise Unsupported("expression %s" % k)
        return m(n)

    def x_ParenExpr(self, n):
        return "(%s)" % self.rv(n["inner"][0])

    def x_IntegerLiteral(self, n):
        t = ctype(n["type"])
        v = int(n["value"])
        return "((%s)%d%s)" % (self.cname(t), v, "u" if not t[2] else "")

    def x_CharacterLiteral(self, n):
        return "((s16)%d)" % int(n["value"])

    def x_ImplicitCastExpr(self, n):
        ck = n["castKind"]
        c = n["inner"][0]
        t = ctype(n["type"])
        if ck == "LValueToRValue":
            return self.rd(self.lv(c))
        if ck in ("IntegralCast",):
            return "((%s)%s)" % (self.cname(t), self.rv(c))
        if ck == "NullToPointer":
            return "((farptr)0)"
        if ck in ("BitCast", "NoOp"):
            return self.rv(c)
        if ck == "ArrayToPointerDecay":
            lval = self.lv(c)
            if lval[0] != "mem":
                raise Unsupported("a local array's address (needs the arena stack)")
            return lval[1]
        if ck == "FunctionToPointerDecay":
            return self.code_addr(c["referencedDecl"]["name"])
        raise Unsupported("implicit cast %s" % ck)

    x_CStyleCastExpr = x_ImplicitCastExpr

    def x_DeclRefExpr(self, n):
        d = n["referencedDecl"]
        if d["kind"] == "EnumConstantDecl":
            raise Unsupported("enum constants")
        return self.rd(self.lv(n))

    def x_MemberExpr(self, n):
        return self.rd(self.lv(n))

    x_ArraySubscriptExpr = x_MemberExpr

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
            lval = self.lv(a)
            v = self.rv(b)
            if lval[0] == "var":
                return "(%s = %s)" % (lval[1], v)
            return self.store(lval[2], lval[1], v)
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
        raise Unsupported("operator %s" % op)

    def pointer_op(self, op, a, b, at, bt, t):
        if op in ("==", "!="):
            return "((s16)(%s %s %s))" % (self.rv(a), op, self.rv(b))
        if op in ("<", ">", "<=", ">="):
            # Turbo C compares far pointers by their offsets only [H: compile and check]
            return "((s16)(TC_OFF(%s) %s TC_OFF(%s)))" % (self.rv(a), op, self.rv(b))
        if op in ("+", "-") and at[0] == "ptr" and bt[0] == "int":
            el = self.size(at[1])
            sign = "" if op == "+" else "0u - "
            return "tc_fp_add(%s, (u16)(%s(u32)%s * %d))" % (self.rv(a), sign, self.rv(b), el)
        raise Unsupported("pointer operator %s" % op)

    def x_CompoundAssignOperator(self, n):
        op = n["opcode"][:-1]
        a, b = n["inner"]
        lval = self.lv(a)
        lt = lval[2]
        ct = ctype(n["computeLHSType"])
        rt = ctype(n["computeResultType"])
        if lt[0] != "int" or ct[0] != "int":
            raise Unsupported("compound assignment on a pointer")
        rhs = self.rv(b)
        if lval[0] == "var":
            cur = "((%s)%s)" % (self.cname(ct), lval[1])
            return "(%s = (%s)%s)" % (lval[1], self.cname(lt), self.arith(op, rt, cur, rhs))
        tmp = self.fresh()
        cur = "((%s)%s)" % (self.cname(ct), self.load(lt, tmp))
        val = "(%s)%s" % (self.cname(lt), self.arith(op, rt, cur, rhs))
        return "({ farptr %s = %s; %s; })" % (tmp, lval[1], self.store(lt, tmp, val))

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
            lval = self.lv(c)
            if lval[0] != "mem":
                raise Unsupported("address of a local (needs the arena stack)")
            return lval[1]
        if op in ("++", "--"):
            return self.step(self.lv(c), 1 if op == "++" else -1, n.get("isPostfix", False))
        raise Unsupported("unary %s" % op)

    def step(self, lval, sign: int, postfix: bool) -> str:
        """++ and --: a pointer moves by its element size (the offset only), an integer by one."""
        t = lval[2]
        if t[0] == "ptr":
            delta = "(u16)%d" % (sign * self.size(t[1]) & 0xFFFF)
            new = lambda old: "tc_fp_add(%s, %s)" % (old, delta)  # noqa: E731
        elif t[0] == "int":
            new = lambda old: "(%s)((u32)%s %s 1u)" % (self.cname(t), old, "+" if sign > 0 else "-")  # noqa: E731
        else:
            raise Unsupported("increment of %r" % (t,))
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
        argv = ", ".join(self.rv(a) for a in args)
        inner = callee
        while inner["kind"] in ("ImplicitCastExpr", "ParenExpr") and inner.get("castKind", "FunctionToPointerDecay") \
                in ("FunctionToPointerDecay",):
            inner = inner["inner"][0]
        if inner["kind"] == "DeclRefExpr" and inner["referencedDecl"]["kind"] == "FunctionDecl":
            name = inner["referencedDecl"]["name"]
            self.called.add(name)
            return "%s(%s)" % (name, argv)
        # a far function pointer: find the C function by the address it holds
        ft = ctype(callee["type"])
        if ft[0] != "ptr" or ft[1][0] != "func":
            raise Unsupported("call through %r" % (ft,))
        fn = ft[1]
        proto = "%s (*)(%s)" % (self.cname(fn[1]), ", ".join(self.cname(p) for p in fn[2]) or "void")
        site = "%s:%d" % (self.path.name, self.line)
        return "((%s)tc_code_lookup(%s, \"%s\"))(%s)" % (proto, self.rv(callee), site, argv)

    # -- statements
    def emit(self, depth: int, text: str):
        self.out.append("    " * depth + text)

    def cond(self, n) -> str:
        return self.rv(n)

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
                if d["kind"] != "VarDecl" or d.get("storageClass") in ("static", "extern"):
                    raise Unsupported("local declaration %s %s" % (d["kind"], d.get("storageClass")))
                t = ctype(d["type"])
                if t[0] not in ("int", "ptr"):
                    raise Unsupported("local %s of type %r (needs the arena stack)" % (d["name"], t))
                self.locals[d["name"]] = t
                init = " = %s" % self.rv(d["inner"][0]) if d.get("inner") else ""
                self.emit(depth, "%s %s%s;" % (self.cname(t), d["name"], init))
        elif k == "ReturnStmt":
            self.emit(depth, "return %s;" % self.rv(n["inner"][0]) if n.get("inner") else "return;")
        elif k == "IfStmt":
            inner = n["inner"]
            self.emit(depth, "if (%s)" % self.cond(inner[0]))
            self.block(inner[1], depth)
            if n.get("hasElse"):
                self.emit(depth, "else")
                self.block(inner[2], depth)
        elif k == "WhileStmt":
            c, body = n["inner"]
            self.emit(depth, "while (%s)" % self.cond(c))
            self.block(body, depth)
        elif k == "DoStmt":
            body, c = n["inner"]
            self.emit(depth, "do")
            self.block(body, depth)
            self.emit(depth, "while (%s);" % self.cond(c))
        elif k == "ForStmt":
            init, _condvar, c, inc, body = n["inner"]
            parts = [self.rv(init) if init else "", self.cond(c) if c else "", self.rv(inc) if inc else ""]
            self.emit(depth, "for (%s; %s; %s)" % tuple(parts))
            self.block(body, depth)
        elif k == "NullStmt":
            self.emit(depth, ";")
        elif k == "BreakStmt":
            self.emit(depth, "break;")
        elif k == "ContinueStmt":
            self.emit(depth, "continue;")
        elif k.endswith("Operator") or k.endswith("Expr"):
            self.emit(depth, "%s;" % self.rv(n))
        else:
            raise Unsupported("statement %s" % k)

    def block(self, n, depth: int):
        if n["kind"] == "CompoundStmt":
            self.stmt(n, depth + 1)
        else:
            self.emit(depth, "{")
            self.stmt(n, depth + 1)
            self.emit(depth, "}")

    # -- the whole unit
    def proto(self, name: str, t, params: list[str] | None = None) -> str:
        if t[0] != "func":
            raise Unsupported("%s is not a function" % name)
        if t[3]:
            raise Unsupported("variadic function %s" % name)
        ps = [self.cname(p) for p in t[2]]
        if params is not None:
            ps = ["%s %s" % (c, p) for c, p in zip(ps, params)]
        return "%s %s(%s)" % (self.cname(t[1]), name, ", ".join(ps) or "void")

    def translate(self, ast: dict) -> str:
        bodies = []
        for d in ast["inner"]:
            if d.get("isImplicit") or d["kind"] != "FunctionDecl":
                continue
            t = ctype(d["type"])
            self.decls[d["name"]] = t
            body = [x for x in d.get("inner", []) if x["kind"] == "CompoundStmt"]
            if not body:
                continue
            if d["name"] not in self.defined:
                raise Unsupported("%s is defined but has no address annotation" % d["name"])
            if d.get("storageClass") == "static":
                raise Unsupported("static function %s" % d["name"])
            params = [p for p in d.get("inner", []) if p["kind"] == "ParmVarDecl"]
            self.locals = {p["name"]: ctype(p["type"]) for p in params}
            self.out = []
            seg, off = self.funcs[d["name"]]
            self.out.append("/* %s %04X:%04X */" % (self.path.name, seg, off))
            self.out.append(self.proto(d["name"], t, [p["name"] for p in params]))
            self.stmt(body[0], 1)
            bodies.append("\n".join(self.out))
        head = ["/* Generated by tools/port/tc2c.py from src/%s: do not edit. */" % self.path.name,
                '#include "tcport.h"', ""]
        for name in sorted(self.decls):
            if name in self.called or name in self.defined:
                head.append("%s;" % self.proto(name, self.decls[name]))
        head.append("")
        init = ["void tc2c_init_%s(void)" % self.name, "{"]
        for name in sorted(self.defined):
            init.append("    tc_code_register(%s, (tc_fn)%s, \"%s\");" % (self.code_addr(name), name, name))
        init.append("}")
        return "\n".join(head) + "\n" + "\n\n".join(bodies) + "\n\n" + "\n".join(init) + "\n"


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
                    if ft[0] != "struct":
                        raise Unsupported("an untagged record used as %r" % (ft,))
                    record(pending, ft[1])
                    pending = None
                fields.append((f["id"], f["name"], ft))
        out[name] = {"fields": fields, "ids": {f[0] for f in fields}, "offsets": {},
                     "union": d.get("tagUsed") == "union"}

    for d in ast["inner"]:
        if d["kind"] == "RecordDecl" and d.get("completeDefinition"):
            record(d, d["name"])
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("units", nargs="+", type=Path)
    a = ap.parse_args(argv)
    a.out.mkdir(parents=True, exist_ok=True)
    done, called, decls = [], {}, {}
    defined_everywhere: set[str] = set()
    try:
        units = []
        for p in a.units:
            u = Unit(p, {}, defined_everywhere)
            ast = u.ast()
            u.records = records_of(ast)
            units.append((u, ast))
            defined_everywhere |= u.defined
        for u, ast in units:
            text = u.translate(ast)
            (a.out / ("%s.c" % u.name)).write_text(text)
            done.append(u.name)
            for name in u.called - defined_everywhere:
                called[name] = (u.decls[name], u.funcs.get(name), u)
    except Unsupported as e:
        print("tc2c: unsupported: %s" % e, file=sys.stderr)
        return 2
    tab = ["/* Generated by tools/port/tc2c.py: the translated units. */", '#include "tcport.h"', ""]
    tab += ["void tc2c_init_%s(void);" % n for n in done]
    tab += ["", "const char *const tc2c_units[] = {%s};" % ", ".join('"%s"' % n for n in done),
            "const unsigned tc2c_unit_count = %d;" % len(done), "", "void tc2c_init(void)", "{"]
    tab += ["    tc2c_init_%s();" % n for n in done] + ["}"]
    (a.out / "units.c").write_text("\n".join(tab) + "\n")
    hdr = ["/* Generated by tools/port/tc2c.py: the functions the translated units define, for the",
           "   port's own C (port/main.c and later the machine layer). */", "#ifndef TC2C_PORTED_H",
           "#define TC2C_PORTED_H", '#include "tcport.h"', ""]
    for u, _ast in units:
        hdr.append("/* src/%s */" % u.path.name)
        hdr += ["%s;" % u.proto(n, u.decls[n]) for n in sorted(u.defined)]
    hdr += ["", "void tc2c_init(void);", "extern const char *const tc2c_units[];",
            "extern const unsigned tc2c_unit_count;", "", "#endif"]
    (a.out / "ported.h").write_text("\n".join(hdr) + "\n")
    stubs = ["/* Generated by tools/port/tc2c.py: functions the translated units call that no translated",
             "   unit defines yet. Each reports itself and stops (tc_unported). */", '#include "tcport.h"', ""]
    for name in sorted(called):
        t, addr, u = called[name]
        where = "%04X:%04X" % addr if addr and not isinstance(addr[0], str) else str(addr)
        ps = ["%s a%d" % (u.cname(p), i) for i, p in enumerate(t[2])]
        stubs.append("%s %s(%s)" % (u.cname(t[1]), name, ", ".join(ps) or "void"))
        stubs.append("{")
        for i in range(len(ps)):
            stubs.append("    (void)a%d;" % i)
        stubs.append('    tc_unported("%s", "%s");' % (name, where))
        if t[1][0] != "void":
            stubs.append("    return 0;")
        stubs.append("}")
        stubs.append("")
    (a.out / "unported.c").write_text("\n".join(stubs))
    print("tc2c: %d units translated (%s), %d functions still unported" % (len(done), " ".join(done), len(called)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
