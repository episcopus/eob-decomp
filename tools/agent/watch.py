#!/usr/bin/env python3
"""watch.py: the live view of a run (`make watch-agents`, playbook 90).

  make watch-agents [INTERVAL=5] [ONCE=1]     redraw every INTERVAL seconds until Ctrl+C

One screen: progress on main (and what the claimed worktrees have matched but not merged yet);
every agent of the run with its state (a session on a unit with its model, minutes and the cost
OpenCode reports so far; idle; backing off after a provider refusal; stopped), the last line of its
log; the spend of the run so far; hand-offs waiting (escalations and rejections first); units both
tiers gave up on (for the human); the latest events. It only reads, so it is safe to leave running.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import next as nxt  # noqa: E402
import run as runmod  # noqa: E402
import worker  # noqa: E402

COLOR = sys.stdout.isatty()
BOLD, DIM, RED, GREEN, YELLOW, CYAN = "1", "2", "1;31", "32", "33", "36"
COSTS: dict[str, tuple[float, float | None]] = {}       # session id -> (when read, cost)


def c(code: str, text: str) -> str:
    return "\033[%sm%s\033[0m" % (code, text) if COLOR else text


def cut(text: str, width: int) -> str:
    return text if len(text) <= width else text[:max(width - 3, 0)] + "..."


def section(title: str, width: int) -> str:
    return c(BOLD + ";" + CYAN, "== %s " % title + "=" * max(width - len(title) - 4, 0))


def minutes_since(iso: str) -> int:
    try:
        return int((dt.datetime.now(dt.timezone.utc) - dt.datetime.fromisoformat(iso)).total_seconds() // 60)
    except (ValueError, TypeError):
        return 0


def live_cost(session_id: str) -> float | None:
    """The session's cost, read from OpenCode at most every 30 seconds."""
    if not session_id:
        return None
    when, cost = COSTS.get(session_id, (0.0, None))
    if time.time() - when > 30:
        cost = worker.session_cost(session_id)
        COSTS[session_id] = (time.time(), cost)
    return cost


def events(since: str = "") -> list[dict]:
    p = common.common_dir() / "eob-events.log"
    out = []
    if p.exists():
        for line in p.read_text().splitlines():
            try:
                e = json.loads(line)
            except ValueError:
                continue
            if e.get("time", "") >= since:
                out.append(e)
    return out


def describe(e: dict) -> str:
    k = e.get("event")
    if k == "worker-session":
        cost = e.get("cost")
        return "%s %s: %s, %s, L2 %s -> %s%s" % (
            e.get("agent"), e.get("key"), e.get("outcome"), "USD %.2f" % cost if cost is not None else "cost ?",
            e.get("l2_before"), e.get("l2_after"), ", REFUSED" if e.get("refused") else "")
    if k == "integrate":
        return "merged %s" % e.get("key")
    if k == "integrate-rejected":
        return c(RED, "rejected %s: %s" % (e.get("key"), e.get("reason", "")))
    if k == "handoff":
        return "%s handed off %s (%s)" % (e.get("agent"), e.get("key"), e.get("state"))
    if k == "claim":
        return "%s claimed %s" % (e.get("agent"), e.get("key"))
    return "%s %s" % (k, " ".join("%s=%s" % (a, b) for a, b in e.items() if a not in ("time", "event")))


def screen(width: int) -> str:
    out = []
    state = runmod.load_state()
    started = state.get("started", "")
    stop = runmod.stop_file().exists()
    head = "eob run: %s" % ("started %s (%dm ago)" % (started, minutes_since(started)) if state else "no run")
    if stop:
        head += c(YELLOW, "  STOP requested")
    out.append(c(BOLD, head) + c(DIM, "   %s" % time.strftime("%H:%M:%S")))

    fs = common.functions()
    game = {u["id"] for u in common.units() if u["program"] == "EOB.EXE" and u["kind"] not in ("library", "startup")}
    gf = [f for f in fs if f["unit"] in game]
    done = [f for f in gf if f.get("level") in ("L2", "L3")]
    q = common.queue()
    locks = {}
    for p in common.state_dir("eob-locks").glob("*.lock"):
        f = common.fields(p.read_text())
        locks[f.get("agent")] = p.stem
    pending = 0
    for key in locks.values():
        wt = common.worktree_of(key)
        if not key.startswith("review-") and (wt / "re/symbols.toml").exists():
            try:
                pending += common.l2_count(wt, key) - common.l2_count(common.ROOT, key)
            except (OSError, KeyError, ValueError):
                pass
    out.append(section("progress", width))
    out.append("  EOB.EXE  %d of %d functions at L2 (%d of %d bytes)%s" % (
        len(done), len(gf), sum(f["size"] for f in done), sum(f["size"] for f in gf),
        c(GREEN, "   +%d matched in worktrees, not merged" % pending) if pending > 0 else ""))
    out.append("  queue    %d units: %d complete, %d reviewed" % (
        len(q), sum(common.unit_complete(u, fs) for u in q), sum(common.unit_reviewed(u, fs) for u in q)))

    out.append(section("agents", width))
    statuses = common.agent_statuses()
    sessions = state.get("sessions", {})
    run_cost = sum(e.get("cost") or 0 for e in events(started) if e.get("event") == "worker-session") if state else 0.0
    live = 0.0
    for name in sorted(set(sessions) | ({k for k, v in statuses.items() if v.get("state") != "stopped"})):
        v, st = sessions.get(name, {}), statuses.get(name, {})
        alive = runmod.alive(v["pid"]) if v.get("pid") else (st.get("pid") and runmod.alive(st["pid"]))
        role = v.get("role") or st.get("role", "")
        if role == "integrator":
            what = "alive" if alive else "stopped"
            label = "integrator"
        else:
            s = st.get("state", "starting") if alive else "stopped"
            label = "%s t%s %s" % (role[:3], st.get("tier", 1), (st.get("model") or "").split("/")[-1])
            what = s
            if alive and s == "session":
                cost = live_cost(st.get("session_id", ""))
                live += cost or 0
                what = "%s %s %dm %s" % (c(GREEN, "session"), st.get("key"), minutes_since(st.get("started", "")),
                                         "USD %.2f" % cost if cost is not None else "")
            elif s == "backoff":
                what = c(YELLOW, "backoff: %s" % st.get("note", ""))
            elif s == "idle":
                what = c(DIM, "idle: %s" % st.get("note", ""))
            elif not alive:
                what = c(RED if state and not stop else DIM, "stopped")
        out.append("  %-15s %-28s %s" % (name, cut(label, 28), cut(what, max(width - 48, 20))))
        if v.get("log"):
            out.append(c(DIM, "  %15s %s" % ("", cut(runmod.last_line(v["log"]), max(width - 18, 20)))))
    if state:
        out.append("  spend this run: USD %.2f finished sessions + USD %.2f running" % (run_cost, live))

    out.append(section("hand-offs", width))
    files = sorted(common.state_dir("eob-handoffs").glob("*.md"), key=lambda p: p.stat().st_mtime)
    if not files:
        out.append(c(DIM, "  none waiting"))
    for p in sorted(files, key=lambda p: (not p.stem.startswith("escalate-"),
                                          common.fields(p.read_text()).get("integrate") != "rejected")):
        f = common.fields(p.read_text())
        tag = (c(RED, "ESCALATION") if f.get("state") == "escalate" else
               c(RED, "REJECTED") if f.get("integrate") == "rejected" else "to merge")
        out.append("  %-24s %-10s %-14s %s" % (p.stem, tag, f.get("agent", ""), cut(
            f.get("integrate_reason") or f.get("note", ""), max(width - 56, 20))))

    stuck = nxt.stuck()
    att = {u: a for u, a in common.attempts().items() if u not in stuck}
    if stuck or att:
        out.append(section("attempts without progress", width))
        if att:
            out.append("  " + cut(", ".join("%s t1:%d t2:%d" % (u, a.get("1", 0), a.get("2", 0))
                                            for u, a in sorted(att.items())), width - 4))
        if stuck:
            out.append(c(RED, "  for the human (both tiers gave up): %s" % ", ".join(stuck)))

    out.append(section("latest", width))
    for e in events(started)[-8:] if state else events()[-8:]:
        out.append("  %s %s" % (c(DIM, e.get("time", "")[11:19]), cut(describe(e), width - 12)))
    return "\n".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--interval", type=float, default=float(os.environ.get("INTERVAL") or 5))
    ap.add_argument("--once", action="store_true", default=bool(os.environ.get("ONCE")))
    a = ap.parse_args(argv)
    try:
        while True:
            text = screen(shutil.get_terminal_size((120, 40)).columns)
            if a.once:
                print(text)
                return 0
            sys.stdout.write("\033[H\033[2J" + text + "\n")
            sys.stdout.flush()
            time.sleep(a.interval)
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    sys.exit(main())
