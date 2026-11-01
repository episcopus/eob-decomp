#!/usr/bin/env python3
"""watch.py: the live view of a run (`make watch-agents`, playbook 90).

  make watch-agents [INTERVAL=5] [ONCE=1]     redraw every INTERVAL seconds until Ctrl+C

One screen: progress on main (and what the claimed worktrees have matched but not merged yet);
every agent of the run with its state (a session on a unit with its model, minutes and the cost
OpenCode reports so far; idle; backing off after a provider refusal; stopped), the last line of its
log; the costs: per agent (sessions, functions matched, USD of finished sessions and of the running
one, USD per function matched), per model, for this run (with USD per hour) and for every run so
far (from the events log; a session OpenCode could not price counts as 0 and is flagged); hand-offs waiting (escalations and rejections first); units both
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


def bar(n: float, total: float, width: int) -> str:
    """A progress bar of `width` cells, eighths of a cell resolved with the partial block characters."""
    if total <= 0:
        return " " * width
    eighths = int(round(min(n / total, 1.0) * width * 8))
    full, part = divmod(eighths, 8)
    cells = "\u2588" * full + (" \u258f\u258e\u258d\u258c\u258b\u258a\u2589"[part] if part else "")
    return c(GREEN, cells) + c(DIM, "\u2591" * (width - len(cells)))


def pct(n: float, total: float) -> float:
    return 100.0 * n / total if total else 0.0


def span(hours: float) -> str:
    if hours < 1:
        return "%dm" % round(hours * 60)
    if hours < 48:
        return "%dh%02dm" % (int(hours), round((hours % 1) * 60))
    return "%dd%02dh" % (int(hours // 24), round(hours % 24))


BASELINES: dict[str, tuple[int, int]] = {}


def baseline(started: str, game: set[str]) -> tuple[int, int]:
    """(functions, bytes) at L2 on main when the run started: the registry of the last commit of main
    before that time."""
    if started not in BASELINES:
        rev = common.git("rev-list", "-1", "--before=%s" % started, "main", check=False)
        try:
            import tomllib
            fs = tomllib.loads(common.git("show", "%s:re/symbols.toml" % rev))["function"] if rev else []
        except (RuntimeError, ValueError):
            fs = []
        done = [f for f in fs if f["unit"] in game and f.get("level") in ("L2", "L3")]
        BASELINES[started] = (len(done), sum(f["size"] for f in done))
    return BASELINES[started]


def velocity(state: dict, game: set[str], nf: int, tf: int, nb: int, tb: int) -> list[str]:
    """Functions and bytes merged into main per hour since the run started, and when the rest would
    be done at that pace (by bytes: functions differ a lot in size), with what it would cost."""
    started = state["started"]
    hours = minutes_since(started) / 60
    f0, b0 = baseline(started, game)
    df, db = nf - f0, nb - b0
    if hours < 0.25 or db <= 0:
        return [c(DIM, "  velocity  measuring (%s of run, %d functions merged so far)" % (span(hours), df))]
    fph, bph = df / hours, db / hours
    eta = (tb - nb) / bph
    spend = sum(e.get("cost") or 0 for e in events(started) if e.get("event") == "worker-session")
    finish = dt.datetime.now().astimezone() + dt.timedelta(hours=eta)
    return ["  velocity  %.1f functions/hour, %d bytes/hour (merged into main since the run started, %s ago)" % (
                fph, bph, span(hours)),
            c(BOLD, "  ETA       %s at this pace: about %s (by functions: %s); about USD %.0f more at USD %.2f/hour" % (
                span(eta), finish.strftime("%a %d %b %H:%M"), span((tf - nf) / fph) if fph else "?",
                spend / hours * eta, spend / hours)),
            c(DIM, "            the queue goes smallest unit first, so the pace and the ETA will change as units grow")]


def money(x: float | None) -> str:
    return "-" if x is None else "%.2f" % x


def tally(evs: list[dict]) -> dict:
    """Sessions, functions gained at L2, and USD of finished worker sessions."""
    t = {"sessions": 0, "gained": 0, "cost": 0.0, "unknown": 0}
    for e in evs:
        t["sessions"] += 1
        t["gained"] += max((e.get("l2_after") or 0) - (e.get("l2_before") or 0), 0)
        if e.get("cost") is None:
            t["unknown"] += 1
        t["cost"] += e.get("cost") or 0.0
    return t


def costs(state: dict, statuses: dict, running: dict[str, float], width: int) -> list[str]:
    """Spend per agent, per model, for this run and for every run so far: finished sessions as
    OpenCode reported them when they ended, plus what the running sessions have spent so far."""
    allev = [e for e in events() if e.get("event") == "worker-session"]
    started = state.get("started", "")
    runev = [e for e in allev if state and e.get("time", "") >= started]
    out = [section("costs (USD)", width)]
    head = "  %-15s %-22s %8s %8s %9s %9s %9s %10s" % (
        "agent", "model", "sessions", "matched", "finished", "running", "total", "per match")
    out.append(c(BOLD, head))
    names = sorted({e.get("agent") for e in runev} | set(running))
    models: dict[str, list] = {}

    def row(label: str, model: str, t: dict, run_now: float) -> str:
        total = t["cost"] + run_now
        per = total / t["gained"] if t["gained"] else None
        return "  %-15s %-22s %8d %8d %9s %9s %9s %10s%s" % (
            cut(label, 15), cut(model, 22), t["sessions"], t["gained"], money(t["cost"]),
            money(run_now) if run_now else "-", money(total), money(per),
            c(DIM, "  (%d without a cost)" % t["unknown"]) if t["unknown"] else "")
    for n in names:
        mine = [e for e in runev if e.get("agent") == n]
        model = (statuses.get(n, {}).get("model") or next((e.get("model") for e in mine if e.get("model")), "") or "")
        model = model.split("/")[-1]
        models.setdefault(model, [[], 0.0])
        models[model][0] += mine
        models[model][1] += running.get(n, 0.0)
        out.append(row(n, model, tally(mine), running.get(n, 0.0)))
    if not names:
        out.append(c(DIM, "  no session in this run yet"))
    if len(models) > 1:
        for m, (evs, now) in sorted(models.items()):
            out.append(c(DIM, row("  model", m, tally(evs), now)))
    if state:
        t = tally(runev)
        total = t["cost"] + sum(running.values())
        hours = max(minutes_since(started), 1) / 60
        out.append(c(BOLD, row("this run", "%dm, USD %.2f/hour" % (minutes_since(started), total / hours),
                               t, sum(running.values()))))
    out.append(row("all runs", "every session so far", tally(allev), sum(running.values())))
    return out


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
    nbytes, total_bytes = sum(f["size"] for f in done), sum(f["size"] for f in gf)
    complete = sum(common.unit_complete(u, fs) for u in q)
    bw = max(10, min(40, width - 62))
    out.append("  functions %s %5.1f%%  %d of %d" % (bar(len(done), len(gf), bw), pct(len(done), len(gf)), len(done), len(gf)))
    out.append("  bytes     %s %5.1f%%  %d of %d" % (bar(nbytes, total_bytes, bw), pct(nbytes, total_bytes), nbytes, total_bytes))
    out.append("  units     %s %5.1f%%  %d of %d complete, %d reviewed" % (
        bar(complete, len(q), bw), pct(complete, len(q)), complete, len(q), sum(common.unit_reviewed(u, fs) for u in q)))
    if pending > 0:
        out.append(c(GREEN, "            +%d functions matched in worktrees, not merged yet" % pending))
    if state:
        out += velocity(state, game, len(done), len(gf), nbytes, total_bytes)

    out.append(section("agents", width))
    statuses = common.agent_statuses()
    sessions = state.get("sessions", {})
    running: dict[str, float] = {}
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
                running[name] = cost or 0.0
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
    out += costs(state, statuses, running, width)

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
    for u in nxt.blocked_units():
        stuck.append("%s (blocked: %s)" % (u, ", ".join(f["addr"] for f in common.unit_functions(u)
                                                         if common.blocked(f))))
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
