"""clayout.py: the byte layout of the C types src/ declares, as Turbo C++ 1.00 `-ml` lays them out.

Structs are packed on bytes (no `-a`), so a member is at the sum of the sizes before it; a union is
as large as its largest member, every member at 0. `int` and `unsigned` are 2 bytes, `long` 4, an
`enum` 2 (`-b`, the default), every data and function pointer 4 (far, the large model) unless it is
declared `near`. The make data-skeleton draft (dataskel.py) uses it to write a struct's bytes as its
initialiser; the sizes are checked against the matched units, since a struct the code indexes with a
wrong size would not match.
"""
from __future__ import annotations

import re
from pathlib import Path

INTS = {"char": (1, True), "signed char": (1, True), "unsigned char": (1, False),
        "int": (2, True), "signed": (2, True), "signed int": (2, True), "unsigned": (2, False),
        "unsigned int": (2, False), "short": (2, True), "short int": (2, True), "signed short": (2, True),
        "unsigned short": (2, False), "unsigned short int": (2, False),
        "long": (4, True), "long int": (4, True), "signed long": (4, True), "unsigned long": (4, False),
        "unsigned long int": (4, False)}


class CType:
    """kind: int, ptr (data), fptr (function), array, struct, union."""

    def __init__(self, kind: str, size: int, *, signed: bool = False, elem: "CType | None" = None,
                 count: int = 0, fields: list | None = None, name: str = "", base: str = ""):
        self.kind, self.size, self.signed, self.elem, self.count = kind, size, signed, elem, count
        self.fields = fields or []              # [(name, offset, CType)]
        self.name = name                        # struct or union tag, or the int type's spelling
        self.base = base                        # a pointer's pointee, as written (`struct Node`)

    def spelling(self) -> str:
        if self.kind in ("struct", "union"):
            return "%s %s" % (self.kind, self.name) if self.name else ""
        return self.name

    def __repr__(self):
        return "CType(%s %s, %d)" % (self.kind, self.name or self.base, self.size)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def split_top(text: str, sep: str) -> list[str]:
    out, depth, cur = [], 0, []
    for c in text:
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        if c == sep and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    out.append("".join(cur))
    return out


def matching(text: str, i: int) -> int:
    """The index of the brace that closes the one at i."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced braces")


def split_decl(text: str) -> tuple[str, str]:
    """(the base type, the first declarator) of `struct Node far *next` or `int (far *fn)(int)`."""
    p = " ".join(text.split())
    fm = re.search(r"\(\s*(?:far\s+|near\s+|huge\s+)?\*", p)
    if fm:
        return p[:fm.start()].strip(), p[fm.start():]
    m = re.match(r"^(.*?)((?:\s|\*|\bfar\b|\bnear\b|\bhuge\b)*\b\w+\s*(?:\[[^\]]*\]\s*)*)$", p)
    if not m or not m.group(1).strip():
        raise ValueError(text)
    return m.group(1).strip(), m.group(2)


class Types:
    """The struct, union and typedef names of a set of source files, first definition wins."""

    def __init__(self, files: list[Path]):
        self.tags: dict[tuple[str, str], CType] = {}
        self.typedefs: dict[str, CType] = {}
        self.defines: dict[str, int] = {}
        texts = []
        for f in files:
            raw = f.read_text(errors="replace")
            for m in re.finditer(r"^\s*#\s*define\s+(\w+)\s+([^\n]+)", strip_comments(raw), re.M):
                try:
                    self.defines.setdefault(m.group(1), self.eval(m.group(2)))
                except Exception:
                    pass
            texts.append(strip_comments(re.sub(r"^\s*#[^\n]*", " ", raw, flags=re.M)))
        for text in texts:
            self.scan(text)

    def eval(self, expr: str) -> int:
        e = re.sub(r"\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", expr.strip())
        e = re.sub(r"\b[A-Za-z_]\w*\b", lambda m: str(self.defines[m.group(0)]) if m.group(0) in self.defines else m.group(0), e)
        if re.search(r"[A-Za-z_]", re.sub(r"0[xX][0-9A-Fa-f]+", "", e)):
            raise ValueError(expr)
        return int(eval(e.replace("/", "//"), {"__builtins__": {}}))

    def scan(self, text: str) -> None:
        i = 0
        pat = re.compile(r"\b(typedef\s+)?(struct|union)\s*(\w*)\s*\{")
        while True:
            m = pat.search(text, i)
            if not m:
                break
            j = matching(text, m.end() - 1)
            try:
                t = self.layout(m.group(2), m.group(3), text[m.end():j])
            except (ValueError, KeyError):
                t = None
            tail = text[j + 1:text.find(";", j)]
            if t is not None:
                if m.group(3):
                    self.tags.setdefault((m.group(2), m.group(3)), t)
                if m.group(1):
                    for n in re.findall(r"\b(\w+)\s*$", tail.strip()):
                        self.typedefs.setdefault(n, t)
            i = j + 1
        for m in re.finditer(r"\btypedef\s+([^;{}]*?)\(\s*(?:far\s+|near\s+)?\*\s*(\w+)\s*\)\s*\([^;]*\)\s*;", text):
            self.typedefs.setdefault(m.group(2), CType("fptr", 4, base=m.group(1).strip()))
        for m in re.finditer(r"\btypedef\s+([\w\s]+?)\s+(\w+)\s*;", text):
            try:
                self.typedefs.setdefault(m.group(2), self.base(m.group(1)))
            except (ValueError, KeyError):
                pass

    def base(self, text: str) -> CType:
        b = " ".join(w for w in text.split() if w not in ("far", "near", "huge", "const", "volatile", "register", "static", "extern"))
        if b.startswith(("struct ", "union ")):
            kind, tag = b.split()[:2]
            return self.tags[(kind, tag)]
        if b.startswith("enum "):
            return CType("int", 2, signed=True, name="int")
        if b in self.typedefs:
            return self.typedefs[b]
        if b in INTS:
            size, signed = INTS[b]
            return CType("int", size, signed=signed, name=b)
        raise KeyError(b)

    def declarator(self, base_text: str, base: CType | None, decl: str) -> tuple[str, CType]:
        """One declarator of a member or variable: its name and type."""
        d = decl.strip()
        m = re.match(r"^\(\s*((?:far|near|huge)\s*)?\*\s*(\w+)\s*((?:\[[^\]]*\]\s*)*)\)\s*\(.*\)$", d, re.S)
        if m:
            t = CType("fptr", 2 if (m.group(1) or "").strip() == "near" else 4, base=base_text)
            return m.group(2), self.dims(t, m.group(3), getattr(self, "_open", False))
        m = re.match(r"^((?:\b(?:far|near|huge)\b|\*|\s)*)(\w+)\s*((?:\[[^\]]*\]\s*)*)$", d)
        if not m:
            raise ValueError(decl)
        stars = m.group(1)
        if "*" in stars:
            t = CType("ptr", 2 if "near" in stars else 4, base=base_text.strip())
        else:
            if base is None:
                raise KeyError(base_text)
            t = base
        return m.group(2), self.dims(t, m.group(3), getattr(self, "_open", False))

    def dims(self, t: CType, text: str, open_ok: bool = False) -> CType:
        ns = re.findall(r"\[([^\]]*)\]", text)
        for i, n in reversed(list(enumerate(ns))):
            if not n.strip():
                if not (open_ok and i == 0):
                    raise ValueError("an array of no size in a struct")
                k = 0                           # `extern T gX[];`: its size is the span's
            else:
                k = self.eval(n)
            t = CType("array", t.size * k, elem=t, count=k)
        return t

    def variable(self, decl: str) -> tuple[str, CType]:
        """The name and type of a variable's declaration, `extern struct Node far *gTable[];`."""
        d = re.sub(r"\b(?:extern|static|const|volatile)\b", " ", decl)
        d = d.split("=")[0].split(";")[0]
        base_text, first = split_decl(d)
        try:
            base = self.base(base_text)
        except KeyError:
            base = None
        self._open = True
        try:
            return self.declarator(base_text, base, first)
        finally:
            self._open = False

    def layout(self, kind: str, tag: str, body: str) -> CType:
        fields, off = [], 0
        for member in split_top(body, ";"):
            member = member.strip()
            if not member:
                continue
            if "{" in member:
                i = member.index("{")
                j = matching(member, i)
                head = member[:i].split()
                sub = self.layout(head[0], head[1] if len(head) > 1 else "", member[i + 1:j])
                if len(head) > 1:
                    self.tags.setdefault((head[0], head[1]), sub)
                base_text, base, decls = " ".join(head), sub, member[j + 1:]
            else:
                pieces = split_top(member, ",")
                base_text, first = split_decl(pieces[0])
                try:
                    base = self.base(base_text)
                except KeyError:
                    base = None
                decls = ",".join([first] + pieces[1:])
            for d in split_top(decls, ","):
                if not d.strip():
                    continue
                if ":" in d:
                    raise ValueError("a bit field")
                name, t = self.declarator(base_text, base, d)
                fields.append((name, 0 if kind == "union" else off, t))
                off = max(off, t.size) if kind == "union" else off + t.size
        return CType(kind, off, fields=fields, name=tag)
