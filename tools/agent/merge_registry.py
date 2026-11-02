#!/usr/bin/env python3
"""merge_registry.py: git merge driver for the registry, entry by entry (playbook 90).

Every worker records its matches in re/symbols.toml (`make match WRITE=1`), and a worker may also
note what it learns about a function of another unit (a signature, a note) in that function's
entry. Git's line merge conflicts when two branches change one entry, even different fields of it
(both append lines at its end). This driver merges the `[[function]]` (or `[[unit]]`) entries by
`id`, and inside an entry field by field (`key = value`, one per line):

  - a field one side changed and the other did not: that side's value;
  - both changed it to different values: `level` keeps the higher (L3, L2, others), `review` the
    later stamp, `notes` both texts joined with "; "; any other field (a name, a signature, the
    switches) is a real conflict, left with conflict markers on that field, exit 1;
  - an entry one side added: kept, after the entry that precedes it on that side;
  - an entry one side deleted and the other left alone: deleted; deleted on one side and changed on
    the other: a conflict.

  git config merge.eob-registry.driver "python3 tools/agent/merge_registry.py %O %A %B"
  .gitattributes: re/symbols.toml merge=eob-registry   (make claim, make integrate and
  make start-run set it up: common.install_git_config)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

LEVELS = {"L3": 3, "L2": 2, "L1": 1, "L0": 0}
FIELD = re.compile(r"^(\w+) = (.*)$")


def parse(text: str) -> tuple[str, str, list[tuple[str, list[tuple[str, str]]]]]:
    """(header, table name, [(id, [(key, value)])])."""
    m = re.search(r"^\[\[(\w+)\]\]$", text, re.M)
    if not m:
        return text, "", []
    table = m.group(1)
    parts = text.split("[[%s]]\n" % table)
    entries = []
    for b in parts[1:]:
        fields = []
        for line in b.splitlines():
            f = FIELD.match(line)
            if f:
                fields.append((f.group(1), f.group(2)))
            elif line.strip():
                raise ValueError("not a `key = value` line: %r" % line)
        key = dict(fields).get("id")
        if key is None:
            raise ValueError("an entry without id")
        entries.append((key, fields))
    return parts[0], table, entries


def merge_value(key: str, o: str | None, a: str | None, b: str | None) -> tuple[str | None, bool]:
    """(merged value, conflict)."""
    if a == b:
        return a, False
    if a == o:
        return b, False
    if b == o:
        return a, False
    if key == "level" and a and b:
        return max(a, b, key=lambda v: LEVELS.get(v.strip('"'), -1)), False
    if key == "review" and a and b:
        return max(a, b), False
    if key == "notes" and a and b:
        ta, tb = a.strip('"'), b.strip('"')
        return '"%s"' % (ta if tb in ta else tb if ta in tb else "%s; %s" % (ta, tb)), False
    return None, True


def merge_entry(o: list | None, a: list, b: list) -> tuple[list[str], bool]:
    """The lines of the merged entry, and whether a field conflicted."""
    do, da, db = dict(o or []), dict(a), dict(b)
    order = [k for k, _ in a] + [k for k, _ in b if k not in da]
    lines, conflict = [], False
    for k in order:
        v, c = merge_value(k, do.get(k), da.get(k), db.get(k))
        if c:
            conflict = True
            # a side that deleted the field shows no line (not `key = None`, which is not TOML)
            side = lambda d: ["%s = %s" % (k, d[k])] if d.get(k) is not None else []
            lines += ["<<<<<<< ours"] + side(da) + ["======="] + side(db) + [">>>>>>> theirs"]
        elif v is not None:
            lines.append("%s = %s" % (k, v))
    return lines, conflict


def merge(base: str, ours: str, theirs: str) -> tuple[str, list[str]]:
    ho, table, eo = parse(base)
    ha, ta, ea = parse(ours)
    hb, tb, eb = parse(theirs)
    table = ta or tb or table
    if ta and tb and ta != tb:
        raise ValueError("different tables: %s, %s" % (ta, tb))
    header = hb if ha == ho else ha
    o, a, b = dict(eo), dict(ea), dict(eb)
    order = [k for k, _ in ea]
    for i, (k, _) in enumerate(eb):                   # entries only theirs has, after their predecessor
        if k not in a:
            prev = next((eb[j][0] for j in range(i - 1, -1, -1) if eb[j][0] in order), None)
            order.insert(order.index(prev) + 1 if prev else 0, k)
    out, conflicts = [], []
    for k in order:
        ina, inb, ino = k in a, k in b, k in o
        if ina and inb:
            lines, c = merge_entry(o.get(k), a[k], b[k])
            if c:
                conflicts.append(k)
        elif ina:                                     # theirs deleted it, or ours added it
            if ino and a[k] != o[k]:
                conflicts.append(k)
                lines = ["<<<<<<< ours (theirs deleted this entry)"] + ["%s = %s" % f for f in a[k]] + [">>>>>>> theirs"]
            elif ino:
                continue
            else:
                lines = ["%s = %s" % f for f in a[k]]
        else:
            if ino and b[k] != o[k]:
                conflicts.append(k)
                lines = ["<<<<<<< ours (ours deleted this entry)", "======="] + ["%s = %s" % f for f in b[k]] + [">>>>>>> theirs"]
            elif ino:
                continue
            else:
                lines = ["%s = %s" % f for f in b[k]]
        out.append("[[%s]]\n%s\n" % (table, "\n".join(lines)))
    return header + "\n".join(out), conflicts


def main(argv: list[str]) -> int:
    if len(argv) != 4:
        print(__doc__)
        return 2
    base, ours, theirs = (Path(p).read_text() for p in argv[1:])
    try:
        text, conflicts = merge(base, ours, theirs)
    except ValueError as e:
        print("merge_registry: %s; leaving the merge to git" % e, file=sys.stderr)
        return 1
    Path(argv[2]).write_text(text)
    for k in conflicts:
        print("merge_registry: conflict in entry %s" % k.strip(chr(34)), file=sys.stderr)
    return 1 if conflicts else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
