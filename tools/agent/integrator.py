#!/usr/bin/env python3
"""integrator.py: `make integrate` on a timer, for a run (playbook 90; `make start-run` starts it).

Every INTERVAL seconds: `make integrate` in the main checkout when a branch hand-off waits. After
IDLE_STOP minutes with no claim, no branch hand-off waiting and no worker of the run alive, the run
is over: it creates build/worker/STOP and exits (`make stop-run` then cleans up). SIGTERM (from
`make stop-run`) lets a merge in progress finish, then exits.

  integrator.py [--interval 120] [--idle-stop 60]
"""
from __future__ import annotations

import argparse
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402
import run  # noqa: E402

STOPPING = False


def waiting() -> int:
    n = 0
    for p in common.state_dir("eob-handoffs").glob("*.md"):
        f = common.fields(p.read_text())
        n += f.get("state") != "escalate" and f.get("integrate") != "rejected"
    return n


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--interval", type=float, default=120)
    ap.add_argument("--idle-stop", type=float, default=60)
    a = ap.parse_args(argv)

    def on_term(sig, frame):
        global STOPPING
        STOPPING = True
    signal.signal(signal.SIGTERM, on_term)
    main_dir, idle_since = common.main_checkout(), time.time()
    print("integrator: every %ds, idle stop after %g minutes" % (a.interval, a.idle_stop), flush=True)
    while not STOPPING:
        if waiting():
            r = subprocess.run(["make", "-s", "integrate"], cwd=main_dir, capture_output=True, text=True)
            print((r.stdout + r.stderr).strip(), flush=True)
        busy = waiting() or any(common.state_dir("eob-locks").glob("*.lock")) or run.workers_alive()
        if busy:
            idle_since = time.time()
        elif time.time() - idle_since > a.idle_stop * 60:
            (main_dir / "build/worker").mkdir(parents=True, exist_ok=True)
            (main_dir / "build/worker/STOP").touch()
            print("integrator: nothing claimed or waiting for %g minutes: the run is over (make stop-run "
                  "cleans up)" % a.idle_stop, flush=True)
            return 0
        for _ in range(int(a.interval)):
            if STOPPING:
                break
            time.sleep(1)
    print("integrator: stopped", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
