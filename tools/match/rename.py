#!/usr/bin/env python3
"""rename.py: give a function or a global its real name, everywhere at once (docs/process/READABILITY.md).

  make rename OLD=<name or address> NEW=<Name> WHY="<evidence>"
  make rename APPLY=1          give every address of re/renames.toml its recorded name in src/, and every
                               function the name its defining file gives it where other files still use
                               a placeholder (make integrate runs it after every merge)

The matcher ties a name to the original by its annotation (`/*@ ds:FC0C */`, `/*@ 19EC:0016 */`), not
by the name, so every file of src/ declares what it uses under a name of its own, and two files can
call one global two things. A rename therefore works by address: OLD is an address (`ds:FC0C`,
`19EC:0016`, `ovl2:0110`) or a name some file of src/ annotates. In every file that declares that
address, its local name is replaced by NEW (in code and comments, not in string literals; assembly
files get `_NEW`). The registry follows: a function's `name` in re/symbols.toml, a global's entry in
re/globals.toml. The whitepaper's mentions of the old names follow too. Every rename is logged in
re/renames.toml (address, old names, new, evidence, date).

On a unit branch (a worker's claim) only the unit's own file changes; the other files that declare
the address get the name when the branch merges (`make integrate` applies the log), so branches never
edit each other's units. A rename never changes the code: `make match` proves every renamed file
exactly as before.
"""
from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/match"))
import cmatch  # noqa: E402

SRC = ROOT / "src"
SYMBOLS = ROOT / "re/symbols.toml"
GLOBALS = ROOT / "re/globals.toml"
LOG = ROOT / "re/renames.toml"
WHITEPAPER = ROOT / "docs/design/WHITEPAPER.md"
ADDRESS = re.compile(r"^(ds:[0-9A-Fa-f]{1,4}|ovl\d+:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})$")
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]{0,30}$")
C_RESERVED = set("""auto break case char const continue default do double else enum extern float for goto if
int long register return short signed sizeof static struct switch typedef union unsigned void volatile while
far near huge cdecl pascal interrupt asm _cs _ds _es _ss""".split())
# What a rename passes over: string and character literals. Comments are renamed like code.
C_LITERALS = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'')
ASM_LITERALS = re.compile(r"'[^'\n]*'|\"[^\"\n]*\"")


def norm(addr: str) -> str:
    """The registry's spelling: ds:FC0C, 19EC:0016, ovl2:0110."""
    seg, off = addr.split(":")
    if seg.lower() == "ds":
        return "ds:%04X" % int(off, 16)
    if seg.lower().startswith("ovl"):
        return "ovl%d:%04X" % (int(seg[3:]), int(off, 16))
    return "%04X:%04X" % (int(seg, 16), int(off, 16))


def bindings(path: Path) -> dict[str, str]:
    """Local name -> address of everything the file annotates."""
    asm = path.suffix.upper() == ".ASM"
    funcs, data, _, _ = cmatch.parse_annotations(path.read_text(), asm)
    out = {n: "ds:%04X" % off for n, off in data.items()}
    for n, (seg, off) in funcs.items():
        out[n] = norm("%s:%X" % (seg if isinstance(seg, str) else "%04X" % seg, off))
    return out


def sources() -> list[Path]:
    return sorted(p for p in SRC.iterdir() if p.suffix.upper() in (".C", ".ASM"))


def replace_word(text: str, old: str, new: str, asm: bool) -> tuple[str, int]:
    """Replace the identifier `old` by `new` outside string and character literals."""
    word = re.compile(r"(?<![A-Za-z0-9_@$?])%s(?![A-Za-z0-9_@$?])" % re.escape(old))
    out, n, pos = [], 0, 0
    for m in (ASM_LITERALS if asm else C_LITERALS).finditer(text):
        chunk, k = word.subn(new, text[pos:m.start()])
        out += [chunk, m.group(0)]
        n += k
        pos = m.end()
    chunk, k = word.subn(new, text[pos:])
    return "".join(out + [chunk]), n + k


def local(name: str, path: Path) -> str:
    return "_" + name if path.suffix.upper() == ".ASM" else name


def resolve(old: str) -> str:
    if ADDRESS.match(old):
        return norm(old)
    found = set()
    for p in sources():
        for n, addr in bindings(p).items():
            if n == old or (p.suffix.upper() == ".ASM" and n == "_" + old):
                found.add(addr)
    if not found:
        raise ValueError("no file of src/ annotates a name %r; give the address instead" % old)
    if len(found) > 1:
        raise ValueError("%r names several addresses (%s); give the address" % (old, ", ".join(sorted(found))))
    return found.pop()


def own_unit() -> str | None:
    """On a unit branch (unit/<id>, a worker's claim), the unit's id; else None."""
    import subprocess
    r = subprocess.run(["git", "branch", "--show-current"], cwd=ROOT, capture_output=True, text=True)
    b = r.stdout.strip()
    return b[5:] if b.startswith("unit/") else None


def rename_address(addr: str, new: str, dry: bool = False, only: str | None = None) -> tuple[list[str], dict[str, int]]:
    """Give `addr` the name `new` in every file of src/ (or only in the files of unit `only`); (the
    old local names, changes per file)."""
    olds, changed = set(), {}
    for p in sources():
        if only and p.stem.lower() != only.lower():
            continue
        b = bindings(p)
        mine = [n for n, a in b.items() if a == addr]
        if not mine:
            continue
        asm = p.suffix.upper() == ".ASM"
        text, total = p.read_text(), 0
        for n in mine:
            # assembly keeps its own convention: `_Name` where the file uses underscores, `Name` where not
            target = "_" + new if asm and n.startswith("_") else new
            if n == target:
                continue
            clash = [m for m, a in b.items() if m == target and a != addr]
            if clash:
                raise ValueError("%s already uses %s for %s" % (p.name, target, b[target]))
            if re.search(r"(?<![A-Za-z0-9_])%s(?![A-Za-z0-9_])" % re.escape(target), text):
                raise ValueError("%s already has an identifier %s; choose another name" % (p.name, target))
            olds.add(n[1:] if asm and n.startswith("_") else n)
            text, k = replace_word(text, n, target, asm)
            total += k
        if total:
            changed[p.name] = total
            if not dry:
                p.write_text(text)
    return sorted(olds), changed


def set_registry(addr: str, new: str, why: str) -> str:
    """The function's name in re/symbols.toml, or the global's entry in re/globals.toml; the kind."""
    if addr.startswith("ds:"):
        text = GLOBALS.read_text() if GLOBALS.exists() else GLOBALS_HEADER
        blocks = text.split("[[global]]\n")
        entry = 'id = "%s"\nname = "%s"\nwhy = %s\n' % (addr, new, toml_str(why))
        for i, b in enumerate(blocks[1:], 1):
            if re.search(r'^id = "%s"$' % re.escape(addr), b, re.M):
                blocks[i] = entry + "\n"
                break
        else:
            blocks.append(entry + "\n")
        GLOBALS.write_text("[[global]]\n".join(blocks).rstrip("\n") + "\n")
        return "global"
    blocks = SYMBOLS.read_text().split("[[function]]\n")
    for i, b in enumerate(blocks):
        if re.search(r'^addr = "%s"$' % re.escape(addr), b, re.M | re.I):
            b = re.sub(r'^name = .*\n', "", b, flags=re.M)
            blocks[i] = b.rstrip("\n") + '\nname = "%s"\n\n' % new
            SYMBOLS.write_text("[[function]]\n".join(blocks))
            return "function"
    return "function (not in re/symbols.toml)"


GLOBALS_HEADER = ("# The names of globals (DGROUP offsets), set by `make rename` (tools/match/rename.py) with their\n"
                  "# evidence. Every file of src/ that declares one uses this name (`make readability` checks it).\n\n")
LOG_HEADER = ("# Every rename of a function or a global (tools/match/rename.py). `make rename APPLY=1` gives each\n"
              "# address its last recorded name in src/; make integrate runs it after every merge.\n\n")


def toml_str(s: str) -> str:
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')


def log(addr: str, olds: list[str], new: str, kind: str, why: str) -> None:
    text = LOG.read_text() if LOG.exists() else LOG_HEADER
    entry = '[[rename]]\nid = "%s=%s"\naddr = "%s"\nold = %s\nnew = "%s"\nkind = "%s"\nwhy = %s\ndate = "%s"\n' % (
        addr, new, addr, toml_str(", ".join(olds)), new, kind.split()[0], toml_str(why), dt.date.today().isoformat())
    LOG.write_text(text.rstrip("\n") + "\n\n" + entry if "[[rename]]" in text else text + entry)


def recorded() -> dict[str, str]:
    """address -> its last recorded name."""
    if not LOG.exists():
        return {}
    out = {}
    for r in tomllib.loads(LOG.read_text()).get("rename", []):
        out[r["addr"]] = r["new"]
    return out


def whitepaper(olds: list[str], new: str) -> int:
    if not WHITEPAPER.exists():
        return 0
    text, n = WHITEPAPER.read_text(), 0
    for o in olds:
        if len(o) >= 5:                    # placeholder names are long and specific; a short word is not renamed
            text, k = re.subn(r"(?<![A-Za-z0-9_])%s(?![A-Za-z0-9_])" % re.escape(o), new, text)
            n += k
    if n:
        WHITEPAPER.write_text(text)
    return n


PLACEHOLDER = re.compile(r"^_?(f[0-9A-F]{4}_[0-9A-F]{4}|Ovl\d+_[0-9A-F]{4}|g[0-9A-F]{4}|gFn[0-9A-F]{4}|sub\w*_[0-9A-F]{4})$")


def definitions() -> dict[str, str]:
    """address -> the name the file that defines the function gives it (not a stub, not a placeholder):
    a unit names its own functions in its own file, often without `make rename`."""
    out = {}
    for p in sources():
        asm = p.suffix.upper() == ".ASM"
        funcs, _, defined, stubs = cmatch.parse_annotations(p.read_text(), asm)
        for n in defined - stubs:
            bare = n[1:] if asm and n.startswith("_") else n
            seg, off = funcs[n]
            addr = norm("%s:%X" % (seg if isinstance(seg, str) else "%04X" % seg, off))
            if not PLACEHOLDER.match(bare):
                out[addr] = bare
    return out


def apply() -> int:
    """Every address gets its recorded name (re/renames.toml), and a function the defining file names
    gets that name in the files that still call it by a placeholder. Two real names for one address
    stay as they are: `make readability` reports them for a person to settle with `make rename`."""
    n = 0
    names = recorded()
    for addr, new in definitions().items():
        if addr in names:
            continue
        local_names = {m.lstrip("_") for p in sources() for m, a in bindings(p).items() if a == addr}
        if all(m == new or PLACEHOLDER.match(m) for m in local_names):
            names[addr] = new
    for addr, new in names.items():
        try:
            olds, changed = rename_address(addr, new)
        except ValueError as e:
            print("rename: %s -> %s not applied: %s" % (addr, new, e))
            continue
        for f, k in changed.items():
            print("rename: %s: %s -> %s (%d)" % (f, "/".join(olds), new, k))
            n += 1
    print("rename: APPLY: %d file changes" % n)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("old", nargs="?", default=os.environ.get("OLD", ""))
    ap.add_argument("new", nargs="?", default=os.environ.get("NEW", ""))
    ap.add_argument("--why", default=os.environ.get("WHY") or os.environ.get("RENAME_WHY", ""))
    ap.add_argument("--apply", action="store_true", default=bool(os.environ.get("APPLY")))
    ap.add_argument("--everywhere", action="store_true", help="on a unit branch, rename in every file anyway")
    a = ap.parse_args(argv)
    try:
        if a.apply:
            return apply()
        if not (a.old and a.new):
            raise ValueError("need OLD=<name or address> NEW=<Name> WHY=\"<evidence>\"")
        if not IDENT.match(a.new) or a.new in C_RESERVED:
            raise ValueError("%r is not a C identifier of at most 31 characters" % a.new)
        if not a.why.strip() or any(ord(c) < 32 for c in a.why):
            raise ValueError("WHY=\"<evidence: an address, a caller, a string, a data file>\" is required, on one line")
        addr = resolve(a.old)
        for p in sources():                # NEW must not name another address anywhere
            other = [x for n, x in bindings(p).items() if n == local(a.new, p) and x != addr]
            if other:
                raise ValueError("%s already names %s in %s" % (a.new, other[0], p.name))
        # On a unit branch only the unit's own file changes: the other files are other units, often
        # being worked on in parallel, and editing them here made merges conflict. make integrate
        # applies the logged rename to them when this branch merges (make rename APPLY=1).
        unit = own_unit() if not a.everywhere else None
        olds, changed = rename_address(addr, a.new, only=unit)
        if unit:
            others = sorted({p.name for p in sources() if p.stem.lower() != unit
                             and any(x == addr for x in bindings(p).values())})
            if others:
                print("rename: the other files that declare %s (%s) get the name when this branch merges" % (
                    addr, ", ".join(others)))
        if not changed and not olds:
            print("rename: %s is already %s everywhere" % (addr, a.new))
        kind = set_registry(addr, a.new, a.why)
        log(addr, olds, a.new, kind, a.why)
        wp = whitepaper(olds, a.new)
        print("rename: %s %s (%s) -> %s: %s%s; logged in re/renames.toml" % (
            kind, addr, ", ".join(olds) or "-", a.new,
            ", ".join("%s %d" % kv for kv in changed.items()) or "no file changed",
            "; whitepaper %d" % wp if wp else ""))
        print("rename: run `make match` (nothing changes in the code) and commit")
    except (ValueError, OSError) as e:
        print("rename: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
