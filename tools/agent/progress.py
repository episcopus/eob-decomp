#!/usr/bin/env python3
"""progress.py: where the decompilation stands (`make progress`).

Functions and bytes at L2 per program, units complete and reviewed (of the queue), claims and
hand-offs waiting. Numbers come from re/symbols.toml and re/units.toml on this checkout.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402


def main() -> int:
    fs, us = common.functions(), common.units()
    for prog in ("EOB.EXE", "INTRO.EXE", "START1.EXE"):
        ids = {u["id"] for u in us if u["program"] == prog and u["kind"] not in ("library", "startup")}
        pf = [f for f in fs if f["unit"] in ids]
        done = [f for f in pf if f.get("level") in ("L2", "L3")]
        print("%-9s %4d of %4d functions at L2, %6d of %6d bytes" % (
            prog, len(done), len(pf), sum(f["size"] for f in done), sum(f["size"] for f in pf)))
    q = common.queue()
    print("queue     %4d units, %d complete, %d reviewed" % (
        len(q), sum(common.unit_complete(u, fs) for u in q), sum(common.unit_reviewed(u, fs) for u in q)))
    locks = list(common.state_dir("eob-locks").glob("*.lock"))
    hand = list(common.state_dir("eob-handoffs").glob("*.md"))
    print("claims    %4d; hand-offs waiting %d (%d escalations)" % (
        len(locks), len(hand), sum(p.stem.startswith("escalate-") for p in hand)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
