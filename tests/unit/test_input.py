"""Script parsing (input.py), the AdLib timers (adlib.py), and every golden session (slow)."""
import os, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from oracle.input import Script, SCAN  # noqa: E402
from oracle.adlib import AdLib  # noqa: E402


def test_script_timing_and_options():
    s = Script("""option mouse
option insns 1000
wait 10
key ENTER       # make, then break 2 ticks later
text Ab
click 5 6 R
shot x
quit""")
    assert s.options == {"mouse": "1", "insns": "1000"}
    ev = [(t, k, a) for t, k, a in s.events]
    assert (10, "kbd", 0x1C) in ev and (12, "kbd", 0x9C) in ev
    assert (12, "kbd", SCAN["SHIFT"]) in ev and (12, "kbd", 0x1E) in ev and (13, "kbd", 0x9E) in ev
    assert (15, "kbd", 0x30) in ev                      # b, no shift
    assert (18, "move", (5, 6)) in ev and (18, "button", 2) in ev and (20, "button", 0) in ev
    assert ev[-2:] == [(20, "shot", "x"), (20, "quit", None)]


def test_script_rejects_unknown():
    with pytest.raises(ValueError, match="line 2"):
        Script("wait 1\nkey NOPE")


class FakeMachine:
    def __init__(self):
        self.t, self.extra, self.port_handlers = 0.0, 0, {}

    def seconds(self):
        return self.t

    def now(self):
        return int(self.t * 1e6)


def test_adlib_detection():
    m = FakeMachine()
    a = AdLib(m)
    out = lambda r, v: (a.write(0x388, 1, r), a.write(0x389, 1, v))
    out(4, 0x60)
    out(4, 0x80)
    assert a.read(0x388, 1) & 0xE0 == 0
    out(2, 0xFF)
    out(4, 0x21)
    m.t += 50e-6
    assert a.read(0x388, 1) & 0xE0 == 0                  # timer 1 needs 80 us at count FFh
    m.t += 50e-6
    assert a.read(0x388, 1) & 0xE0 == 0xC0
    assert m.adlib_log[0][1:] == (4, 0x60)


@pytest.mark.slow
def test_goldens():
    if not os.path.exists(os.path.join(ROOT, "eob", "EOB.EXE")):
        pytest.skip("eob/ not present")
    from oracle import golden
    assert golden.main(["--check"]) == 0
