#!/usr/bin/env python3
"""readability.py: the checkable rules of docs/process/READABILITY.md (`make readability`, part of `make check`).

  make readability [UNIT=<id>]       every unit with a source file, or one; prints every finding
  make readability GATE=1            fails only on findings that are not in work/readability-baseline.txt,
                                     in the files READABILITY_FILES names when it is set (make integrate:
                                     the files the hand-off changed), or during a merge in the files the
                                     merge brings, else in every file
  make readability BASELINE=1        rewrites the baseline from the current findings (the human, when
                                     the rules change; it only ever shrinks otherwise)

For every function a file defines (not its stubs) and for the file:

  R1  the file starts with a comment of 8 words or more that is not the skeleton's
  R2  the function has a real name, not a placeholder (f19EC_0016, Ovl2_0110), or its comment says
      `Purpose unknown:` with what is known
  R3  a comment right above the function (or on its line), 8 words or more, with its address
  R4  no parameter or local is a single letter (except i, j, k, n, x, y) or a placeholder (tmp,
      val, a1, arg2, local3 ...)
  R5  the file uses the recorded name of every address re/renames.toml names (`make rename APPLY=1`
      fixes it), and one name per address across src/ (otherwise `make rename` the address)

Assembly files are held to R1, R2 and R5. Findings are keyed `file|rule|address` so they survive
renames; the baseline holds the findings that existed when a rule came in, so the gate stops new
ones without failing the units merged before it.
"""
from __future__ import annotations

import argparse
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/match"))
import cmatch  # noqa: E402
import rename  # noqa: E402

BASELINE = ROOT / "work/readability-baseline.txt"
PLACEHOLDER = re.compile(r"^_?(f[0-9A-F]{4}_[0-9A-F]{4}|Ovl\d+_[0-9A-F]{4}|fn_?[0-9A-Fa-f]{4}\w*|sub_[0-9A-Fa-f]+)$")
BAD_LOCAL = re.compile(r"^(?:[a-hl-mo-wzA-Z]|tmp\d*|temp\d*|val\d*|value\d+|var\d+|arg\d+|a\d+|p\d+|param\d+|local\d+)$")
TYPE_WORDS = r"(?:unsigned|signed|register|static|const|volatile|char|int|long|short|void|far|near|huge|struct\s+\w+|union\s+\w+|enum\s+\w+)"
DECL = re.compile(r"^\s*(?:%s\s+)+[\s*]*(\w+)\s*(?:\[[^\]]*\])?\s*(?:=[^,;]*)?((?:,\s*\**\s*\w+\s*(?:\[[^\]]*\])?\s*(?:=[^,;]*)?)*);" % TYPE_WORDS)
ANNOT_DEF = re.compile(r"/\*@\s*(ovl\d+:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})(\s+stub)?(\s+draft)?\s*\*/")


def words(text: str) -> int:
    return len(re.findall(r"[A-Za-z]{2,}", text))


def comment_above(lines: list[str], i: int) -> str:
    """The comment block that ends right above line i (blank lines not allowed between), plus any
    comment on line i itself besides the annotation."""
    own = re.sub(r"/\*@.*?\*/", "", lines[i])
    text = " ".join(re.findall(r"/\*(.*?)\*/|//(.*)", own and "".join(own)) and
                    [a or b for a, b in re.findall(r"/\*(.*?)\*/|//(.*)", own)])
    j, block = i - 1, []
    if j >= 0 and lines[j].rstrip().endswith("*/"):
        while j >= 0:
            block.insert(0, lines[j])
            if "/*" in lines[j]:
                break
            j -= 1
    return (" ".join(block) + " " + text).replace("/*", " ").replace("*/", " ")


def params_and_locals(lines: list[str], i: int) -> list[str]:
    """The names of the parameters (from the definition line) and the locals declared at the top of
    the body (C)."""
    head = re.sub(r"/\*.*?\*/", "", lines[i])
    m = re.search(r"\(([^)]*)\)", head)
    names = []
    if m and m.group(1).strip() not in ("", "void"):
        for p in m.group(1).split(","):
            ids = re.findall(r"[A-Za-z_]\w*", re.sub(r"\[.*?\]", "", p))
            if ids and ids[-1] not in rename.C_RESERVED:
                names.append(ids[-1])
    j = i + 1
    while j < len(lines) and "{" not in lines[j] and "{" not in head:
        j += 1
    j += 1
    while j < len(lines):
        line = re.sub(r"/\*.*?\*/", "", lines[j])
        if not line.strip():
            j += 1
            continue
        d = DECL.match(line)
        if not d:
            break
        names.append(d.group(1))
        names += re.findall(r",\s*\**\s*(\w+)", d.group(2) or "")
        j += 1
    return names


def check_file(path: Path, recorded: dict[str, str], names_by_addr: dict[str, set]) -> list[tuple[str, str, str]]:
    """[(key, rule, message)]; key is file|rule|address."""
    asm = path.suffix.upper() == ".ASM"
    text = path.read_text()
    lines = text.splitlines()
    out = []

    def add(rule, where, msg):
        out.append(("%s|%s|%s" % (path.name, rule, where), rule, msg))
    first = re.match(r"\s*(/\*.*?\*/)", text, re.S) if not asm else None
    head = first.group(1) if first else "\n".join(l for l in lines[:12] if l.lstrip().startswith(";")) if asm else ""
    if words(head) < 8 or "Skeleton by `make skeleton`" in head:
        add("R1", "unit", "no unit comment of 8 words or more (what the unit does, what is known)")
    for i, line in enumerate(lines):
        m = ANNOT_DEF.search(line) if not asm else re.search(r";\s*@\s*([0-9A-Fa-f]{4}:[0-9A-Fa-f]{4}|ovl\d+:[0-9A-Fa-f]{1,4})(\s+stub)?", line)
        if not m or m.group(2):
            continue
        decl = ANNOT_DEF.sub("", line) if not asm else line.split(";")[0]
        if not asm and (decl.strip().startswith("extern") or decl.rstrip().endswith(";")):
            continue
        if asm and not re.search(r"\bPUBLIC\b", decl, re.I):
            continue
        ids = re.findall(r"([A-Za-z_]\w*)\s*\(", decl) if not asm else re.findall(r"PUBLIC\s+(\w+)", decl, re.I)
        if not ids:
            continue
        name, addr = ids[0], rename.norm(m.group(1))
        note = comment_above(lines, i) if not asm else " ".join(l.split(";", 1)[1] for l in lines[max(0, i - 6):i + 1] if ";" in l)
        if PLACEHOLDER.match(name) and "Purpose unknown:" not in note:
            add("R2", addr, "%s: a placeholder name; name it from the evidence, or say `Purpose unknown:` "
                            "and what is known in its comment" % name)
        if not asm:
            if words(note) < 8 or addr.upper() not in note.upper().replace("OVL", "ovl").upper():
                add("R3", addr, "%s: no comment of 8 words or more with its address %s right above it" % (name, addr))
            bad = [n for n in params_and_locals(lines, i) if BAD_LOCAL.match(n)]
            if bad:
                add("R4", addr, "%s: parameter or local named %s; name its role" % (name, ", ".join(bad)))
    for n, addr in rename.bindings(path).items():
        bare = n[1:] if asm and n.startswith("_") else n
        if addr in recorded and bare != recorded[addr]:
            add("R5", addr, "%s is recorded as %s (re/renames.toml): make rename APPLY=1" % (bare, recorded[addr]))
        elif addr not in recorded and len(names_by_addr.get(addr, ())) > 1:
            add("R5", addr, "%s is %s here and %s elsewhere: make rename OLD=%s NEW=<the name> WHY=..." % (
                addr, bare, "/".join(sorted(names_by_addr[addr] - {bare})), addr))
    return out


def findings(unit: str = "") -> list[tuple[str, str, str]]:
    recorded = rename.recorded()
    names_by_addr = defaultdict(set)
    for p in rename.sources():
        for n, a in rename.bindings(p).items():
            names_by_addr[a].add(n[1:] if p.suffix.upper() == ".ASM" and n.startswith("_") else n)
    files = rename.sources()
    if unit:
        files = [p for p in files if p.stem.lower() == unit.lower()]
        if not files:
            raise ValueError("no src file for %s" % unit)
    return [f for p in files for f in check_file(p, recorded, names_by_addr)]


def merging() -> bool:
    """A merge is in progress in this checkout (make integrate's temporary worktree)."""
    import subprocess
    r = subprocess.run(["git", "rev-parse", "--git-path", "MERGE_HEAD"], cwd=ROOT, capture_output=True, text=True)
    return r.returncode == 0 and (ROOT / r.stdout.strip()).exists()


def changed_in_merge() -> list[str]:
    """The files the merge in progress changes (what the branch brings)."""
    import subprocess
    r = subprocess.run(["git", "diff", "--name-only", "--cached", "HEAD"], cwd=ROOT, capture_output=True, text=True)
    return r.stdout.split()


def baseline() -> set[str]:
    return {l.strip() for l in BASELINE.read_text().splitlines() if l.strip() and not l.startswith("#")} \
        if BASELINE.exists() else set()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="?", default=os.environ.get("UNIT", ""))
    ap.add_argument("--gate", action="store_true", default=bool(os.environ.get("GATE")))
    ap.add_argument("--baseline", action="store_true", default=bool(os.environ.get("BASELINE")))
    a = ap.parse_args(argv)
    try:
        found = findings(a.unit)
    except ValueError as e:
        print("readability: %s" % e, file=sys.stderr)
        return 2
    if a.baseline:
        BASELINE.write_text("# Readability findings that existed when the rules came in (tools/match/readability.py).\n"
                            "# The gate fails only on findings not listed here. Fix them and delete the lines.\n" +
                            "".join("%s\n" % k for k in sorted({k for k, _, _ in found})))
        print("readability: baseline of %d findings written to %s" % (len(found), BASELINE.relative_to(ROOT)))
        return 0
    base = baseline()
    only = os.environ.get("READABILITY_FILES")
    if a.gate and only is None and merging():
        only = " ".join(changed_in_merge())
    if a.gate and only is not None:
        names = {Path(f).name for f in only.split() if f.startswith("src/")}
        found = [f for f in found if f[0].split("|")[0] in names]
    new = [f for f in found if f[0] not in base]
    shown = new if a.gate else found
    for key, rule, msg in shown:
        print("%s %-4s %s%s" % (key.split("|")[0], rule, msg, "" if key in base or a.gate else "   (new)"))
    print("readability: %d findings, %d of them new (not in %s)%s" % (
        len(found), len(new), BASELINE.relative_to(ROOT),
        ": FAILED, fix the new ones (docs/process/READABILITY.md)" if a.gate and new else ""))
    return 1 if (a.gate and new) else 0


if __name__ == "__main__":
    sys.exit(main())
