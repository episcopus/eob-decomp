#!/usr/bin/env python3
"""next.py: what to work on next (playbooks 30 and 70).

  make next [TIER=2]        the next unit to decompile: the first of work/queue.toml that is not
                            complete (or has open must-fix review findings), not locked, has no
                            hand-off waiting, and that its tier has not given up on (common.attempts)
  make next REVIEW=1        the next complete unit nobody has reviewed
  make queue [WRITE=1]      the queue made from re/units.toml: the units of EOB.EXE (then INTRO.EXE) that are game
                            code (C, assembly, not classified), smallest first; WRITE=1 writes
                            work/queue.toml, which the human may reorder

Units of INTRO.EXE come after EOB.EXE's; those with an EOB twin (re/units.toml `twin`) are proven by
`make twin`, not decompiled again. The runtime library (`e_lib`, `i_lib`) and the start-up code
(`e_c0`, `i_c0`) are library, never decompiled (AGENTS.md).
"""
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402

GAME_KINDS = ("c", "asm", "unknown")


def candidates() -> list[dict]:
    # INTRO.EXE units with an EOB twin are proven by tools/match/twin.py, not decompiled again
    return sorted((u for u in common.units() if u["program"] in ("EOB.EXE", "INTRO.EXE") and u["kind"] in GAME_KINDS
                   and u.get("functions", 0) > 0 and not u.get("twin")),
                  key=lambda u: (u["program"] != "EOB.EXE", u["bytes"], u["id"]))


def busy(key: str) -> bool:
    return common.lock_path(key).exists() or common.handoff_path(key).exists()


def next_unit(tier: int = 1, max_attempts: int = 2) -> str | None:
    """Tier 1 takes units its sessions have not given up on (fewer than TIER1_ATTEMPTS sessions without
    progress); tier 2 takes only those, up to `max_attempts` of its own sessions without progress.
    Units both tiers gave up on wait for the human (`stuck`)."""
    fs, att = common.functions(), common.attempts()
    for uid in common.queue():
        if busy(uid):
            continue
        if common.unit_complete(uid, fs) and not common.open_must_fix(uid):
            continue
        if common.unit_blocked(uid, fs) and not common.open_must_fix(uid):
            continue                       # waits for the human (make watch-agents lists it)
        a = att.get(uid, {})
        t1 = a.get("1", 0) >= common.TIER1_ATTEMPTS
        if tier == 1 and not t1:
            return uid
        if tier == 2 and t1 and a.get("2", 0) < max_attempts:
            return uid
    return None


def blocked_units() -> list[str]:
    fs = common.functions()
    return [u for u in common.queue() if common.unit_blocked(u, fs)]


def stuck(max_attempts: int = 2) -> list[str]:
    """Units both tiers gave up on: for the human."""
    att = common.attempts()
    return [u for u, a in sorted(att.items()) if a.get("1", 0) >= common.TIER1_ATTEMPTS and a.get("2", 0) >= max_attempts]


def next_review() -> str | None:
    fs = common.functions()
    for uid in common.queue():
        if common.unit_complete(uid, fs) and not common.unit_reviewed(uid, fs) and not busy("review-" + uid) \
                and not common.open_must_fix(uid):
            return uid
    return None


def queue_text() -> str:
    lines = ["# The order in which units are claimed (make claim, make next). Made by `make queue WRITE=1`",
             "# from re/units.toml: the game units of EOB.EXE, then those of INTRO.EXE without an EOB twin, smallest\n"
             "# first. The human may reorder it.", ""]
    for u in candidates():
        lines += ["[[unit]]", 'id = "%s"' % u["id"], "# %s %s, %d bytes, %d functions" % (
            u["kind"], u["start"], u["bytes"], u["functions"]), ""]
    return "\n".join(lines)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--review", action="store_true", default=bool(os.environ.get("REVIEW")))
    ap.add_argument("--queue", action="store_true")
    ap.add_argument("--write", action="store_true", default=bool(os.environ.get("WRITE")))
    ap.add_argument("--tier", type=int, default=int(os.environ.get("TIER") or 1))
    a = ap.parse_args(argv)
    if a.queue:
        text = queue_text()
        if a.write:
            (common.ROOT / "work/queue.toml").write_text(text)
            print("queue: wrote work/queue.toml (%d units)" % len(candidates()))
        else:
            print(text)
        return 0
    n = next_review() if a.review else next_unit(a.tier)
    if n is None:
        print("next: nothing to %s" % ("review" if a.review else "claim"))
        return 1
    u = common.unit(n)
    print("next: %s (%s, %s, %d bytes, %d functions)%s" % (
        n, u["kind"], u["start"], u["bytes"], u["functions"],
        "; open must-fix findings in work/reviews/%s.md" % n if common.open_must_fix(n) else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
