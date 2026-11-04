#!/usr/bin/env python3
"""field.py: give a struct field one name in every file of src/ that declares the struct (Phase 6,
playbook 65).

  make field STRUCT=<Name> AT=<hex offset> NEW=<name> WHY="<evidence>"

Every C file declares the structs it uses, so one record (struct CharRec, in 16 files) can call one
byte `m01`, `status` and `flags`. A field is known by its struct's name and its offset, computed from
the declaration (names.py): in each file whose struct STRUCT has a field at offset AT, that field
becomes NEW, in its declaration and in its member accesses (`rec->old`, `gChars[i].old`). When
another struct or union of the file also has a field called old, an access is renamed only when
its base is a variable, parameter or global the file declares as STRUCT (or a pointer or array of
it); a file with an access of old whose base the tool cannot tell (`f()->old`, a name declared with
two types) is refused and left unchanged, as is one whose STRUCT already has a field NEW. Comments
follow when the old name was a placeholder (m01) and only STRUCT has it. A file whose STRUCT covers
the offset with padding or does not reach it is left alone. The rename is logged in re/fields.toml.

On a unit branch only the unit's own file changes, as with `make rename`. A field rename never
changes the code: `make match` proves every file as before (a wrong access would not compile).
"""
from __future__ import annotations

import argparse
import datetime as dt
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/match"))
import names  # noqa: E402
import rename  # noqa: E402

LOG = ROOT / "re/fields.toml"
LOG_HEADER = ("# Struct fields named across src/ by `make field` (tools/match/field.py): the struct, the offset,\n"
              "# the names the files used, the new name and its evidence.\n\n")


def typed_names(code: str) -> dict[str, list]:
    """identifier -> [(position, struct or union name)] of each declaration the file makes of it with
    a struct type (variables, parameters, globals, fields; pointers and arrays included)."""
    out: dict[str, list] = {}
    for m in re.finditer(r"\b(?:struct|union)\s+(\w+)\s+((?:far\b|near\b|huge\b|\*|\s)*)(\w+)", code):
        out.setdefault(m.group(3), []).append((m.start(3), m.group(1)))
    for m in re.finditer(r"\b(?:struct|union)\s+(\w+)\s+[^;{}()]*?,\s*((?:far\b|near\b|\*|\s)*)(\w+)", code):
        out.setdefault(m.group(3), []).append((m.start(3), m.group(1)))
    return out


def type_at(types: dict, name: str, pos: int) -> str | None:
    """The struct of `name` at `pos`: its nearest declaration before (a local or a parameter of the
    function, else the global)."""
    before = [s for at, s in types.get(name, []) if at < pos]
    return before[-1] if before else None


def base_of(code: str, at: int) -> str | None:
    """The identifier an access at `at` (the `.` or `->`) is made through: `rec`, `gChars` of
    `gChars[i + 1]`; None for anything else (a call, a cast, a parenthesised expression)."""
    k = at - 1
    while k >= 0 and code[k].isspace():
        k -= 1
    while k >= 0 and code[k] == "]":
        depth = 0
        while k >= 0:
            depth += {"]": 1, "[": -1}.get(code[k], 0)
            k -= 1
            if depth == 0:
                break
        while k >= 0 and code[k].isspace():
            k -= 1
    e = k + 1
    while k >= 0 and (code[k].isalnum() or code[k] == "_"):
        k -= 1
    name = code[k + 1:e]
    if not name or name[0].isdigit():
        return None
    j = k
    while j >= 0 and code[j].isspace():
        j -= 1
    if j >= 0 and (code[j] == "." or code[j:j + 1] == ">" and code[j - 1:j] == "-"):
        return None                                     # a.b.old: the base is a field, not a variable
    return name


def rename_in(text: str, struct: str, at: int, new: str, header: str = "") -> tuple[str | None, str]:
    """(the new text or None, what happened). `header` is src/EOB.H's text for a file that includes
    it: a struct the file does not declare itself is looked up there, and only its uses change here
    (the declaration changes in the header)."""
    found = dict(names.struct_fields(text))
    local = struct in found
    if not local:
        hfound = dict(names.struct_fields(header)) if header and names.INCLUDE.search(text) else {}
        if struct not in hfound:
            return None, ""
        fields = hfound[struct]
    else:
        fields = found[struct]
    old = next((n for o, n in fields if o == at), None)
    if old is None or names.PAD.match(old):
        return None, "no field at +%02X (padding or shorter)" % at
    if old == new:
        return None, "already %s" % new
    if any(n == new for o, n in fields if o != at):
        return None, "REFUSED: struct %s already has a field %s" % (struct, new)
    shared = any(n == old for s, fs in names.struct_fields(text + "\n" + header) if s != struct for o, n in fs)
    code = names.comments_out(text)
    code = rename.C_LITERALS.sub(lambda m: " " * len(m.group(0)), code)
    types = typed_names(code)
    access = re.compile(r"(\.|->)\s*%s(?![A-Za-z0-9_])" % re.escape(old))
    spans = []
    for a in access.finditer(code):
        if shared:
            base = base_of(code, a.start())
            kind = type_at(types, base, a.start()) if base else None
            if kind == struct:
                pass
            elif kind:
                continue                                # another struct's field of that name
            else:
                return None, "REFUSED: cannot tell the struct of the access at line %d (%s)" % (
                    code.count("\n", 0, a.start()) + 1, base or "an expression")
        spans.append((a.end() - len(old), a.end()))
    uses = len(spans)
    if local:
        m = next(m for m in names.STRUCT.finditer(code) if m.group(2) == struct)
        d = re.search(r"(?<![\w.])%s(?=\s*(?:\[[^\]]*\]\s*)*[;)])" % re.escape(old), code[m.start(3):m.end(3)])
        if not d:
            return None, "REFUSED: the declaration of %s was not found" % old
        spans.append((m.start(3) + d.start(), m.start(3) + d.end()))
    elif not spans and not names.FIELD_PLACEHOLDER.match(old):
        return None, ""
    if shared:                                          # `gChars[who].m01` written in a comment
        for c in re.finditer(r"/\*.*?\*/|//[^\n]*", text, re.S):
            for a in access.finditer(c.group(0)):
                base = base_of(c.group(0), a.start())
                if base and type_at(types, base, c.start()) == struct:
                    spans.append((c.start() + a.end() - len(old), c.start() + a.end()))
    for s, e in sorted(set(spans), reverse=True):
        text = text[:s] + new + text[e:]
    if not shared and names.FIELD_PLACEHOLDER.match(old):
        out, pos = [], 0
        for c in re.finditer(r"/\*.*?\*/|//[^\n]*", text, re.S):
            out += [text[pos:c.start()], rename.replace_word(c.group(0), old, new, False)[0]]
            pos = c.end()
        text = "".join(out) + text[pos:]
    return text, "%s -> %s, %d uses%s" % (old, new, uses, " (by type)" if shared else "")


def log(struct: str, at: int, olds: list[str], new: str, why: str) -> None:
    text = LOG.read_text() if LOG.exists() else LOG_HEADER
    entry = '[[field]]\nstruct = "%s"\nat = "%02X"\nold = %s\nnew = "%s"\nwhy = %s\ndate = "%s"\n' % (
        struct, at, rename.toml_str(", ".join(olds)), new, rename.toml_str(why), dt.date.today().isoformat())
    LOG.write_text(text.rstrip("\n") + "\n\n" + entry if "[[field]]" in text else text + entry)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--struct", required=True)
    ap.add_argument("--at", required=True)
    ap.add_argument("--new", required=True)
    ap.add_argument("--why", required=True)
    a = ap.parse_args(argv)
    at = int(a.at, 16)
    if not rename.IDENT.match(a.new) or a.new in rename.C_RESERVED:
        print("field: %s is not a C identifier" % a.new)
        return 2
    if len(a.why.split()) < 4:
        print("field: WHY needs the evidence (an address, a caller, a string)")
        return 2
    only = rename.own_unit()
    olds, changed = [], 0
    hpath = rename.SRC / "EOB.H"
    header = hpath.read_text() if hpath.exists() else ""
    for p in sorted(rename.SRC.glob("*.C")) + ([hpath] if header else []):
        if only and p.stem.lower() != only.lower():
            continue
        text = p.read_text()
        before = dict(names.struct_fields(text)).get(a.struct) or dict(names.struct_fields(header)).get(a.struct)
        new_text, what = rename_in(text, a.struct, at, a.new, "" if p == hpath else header)
        if new_text == text:
            continue
        if what:
            print("field: %-9s %s" % (p.name, what))
        if new_text is not None:
            p.write_text(new_text)
            changed += 1
            olds.append(next(n for o, n in before if o == at))
    if not changed:
        print("field: nothing changed")
        return 1
    log(a.struct, at, sorted(set(olds)), a.new, a.why)
    print("field: struct %s +%02X is %s in %d files; logged in re/fields.toml; run `make match`" % (
        a.struct, at, a.new, changed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
