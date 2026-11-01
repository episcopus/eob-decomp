#!/usr/bin/env python3
"""review.py: record a passed review (playbook 70).

  make review-pass UNIT=<id> AGENT=<reviewer> [CHECK=1]

Checks, on the review branch: the unit is complete (every function at L2), its file still matches,
work/reviews/<id>.md exists with no open `- [ ] must-fix` item and names the reviewer. Then writes
`review = "passed <date> <agent>"` on every function of the unit in re/symbols.toml (CHECK=1 only
checks). Commit that with the review file and hand off STATE=review-passed. A review that finds
must-fix items hands off STATE=review-failed instead: the unit goes back to the queue (`make next`
offers it again) until a decompiler ticks them off.
"""
from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import handoff  # noqa: E402


def problems(uid: str, agent: str) -> list[str]:
    out = []
    if not common.unit_complete(uid):
        missing = [f["addr"] for f in common.unit_functions(uid) if f.get("level") not in ("L2", "L3")]
        out.append("%s is not complete: %d functions not at L2 (%s)" % (uid, len(missing), ", ".join(missing[:8])))
    why = handoff.unmatched(uid)
    if why:
        out.append("make match fails on %s: %s" % (uid, why.splitlines()[-1] if why else ""))
    p = common.ROOT / "work/reviews" / ("%s.md" % uid)
    if not p.exists():
        out.append("no work/reviews/%s.md (copy work/reviews/TEMPLATE.md)" % uid)
    else:
        if common.open_must_fix(uid):
            out.append("open must-fix items in %s: %s" % (p.name, "; ".join(common.open_must_fix(uid))))
        if not re.search(r"^Reviewer:\s*%s\b" % re.escape(agent), p.read_text(), re.M):
            out.append("%s does not name the reviewer (a line `Reviewer: %s`)" % (p.name, agent))
    return out


def record_pass(uid: str, agent: str) -> int:
    p = common.ROOT / "re/symbols.toml"
    blocks = p.read_text().split("[[function]]\n")
    stamp = 'review = "passed %s %s"\n' % (dt.date.today().isoformat(), agent)
    n = 0
    for i, b in enumerate(blocks):
        if re.search(r'^unit = "%s"$' % re.escape(uid), b, re.M):
            b = re.sub(r"^review = .*\n", "", b, flags=re.M)
            blocks[i] = b.rstrip("\n") + "\n" + stamp + "\n"
            n += 1
    p.write_text("[[function]]\n".join(blocks))
    return n


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", default=os.environ.get("UNIT", ""))
    ap.add_argument("--agent", default=os.environ.get("AGENT", ""))
    ap.add_argument("--check", action="store_true", default=bool(os.environ.get("CHECK")))
    a = ap.parse_args(argv)
    try:
        uid, agent = common.identifier(a.unit, "unit"), common.identifier(a.agent, "agent")
    except ValueError as e:
        print("review-pass: %s" % e, file=sys.stderr)
        return 2
    found = problems(uid, agent)
    for p in found:
        print("review-pass: " + p, file=sys.stderr)
    if found:
        return 1
    if a.check:
        print("review-pass: %s can be recorded as reviewed by %s" % (uid, agent))
        return 0
    print("review-pass: %s reviewed by %s: %d functions recorded in re/symbols.toml" % (uid, agent, record_pass(uid, agent)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
