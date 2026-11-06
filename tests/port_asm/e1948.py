"""E1948.c (the PC speaker and Tandy driver in C) against src/E1948.ASM, by Direct calls on the bench
(playbook 80, M3): every song of PCGAMESN.DAT (build/re/pak, `make data`) played to its end on the
speaker and on Tandy, with install, resume, a stop, the silence and uninstall; synthetic streams over
every command byte; the counters' wraps; random driver states. Each call compares the whole speaker,
Tandy and timer logs it wrote. Exit status 1 if any call differs or the C stops; a call the original
does not return from (a song that loops forever, a divide error the oracle sits on) is not tested.

  e1948.py [LIB]
"""
import sys, random, time, types, collections
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "port"))
import hybrid

LIB = Path(sys.argv[1] if len(sys.argv) > 1 else hybrid.LIB)
L = hybrid.LOAD_SEG
SND = L + 0x29B8                      # the driver segment (DGROUP + 88Ah)
PCDAT = (ROOT / "build" / "re" / "pak" / "EOBDATA1" / "PCGAMESN.DAT").read_bytes()
BLOCK = PCDAT[120:]
NSONG = 67
SONG = [int.from_bytes(BLOCK[2 * i:2 * i + 2], "little") for i in range(NSONG)]

# variables (driver-segment offsets)
TANDY, PLAYING, STOPREQ, VCOUNT, TICK, SECS, CHAIN = 0x1C7, 0x19D, 0x19F, 0x19B, 0x1C8, 0x1CA, 0x08
VAR_LO, VAR_HI = 0x19A, 0x1CB         # the driver's variables after the note tables

d = hybrid.Direct(lib=LIB)
m = d.m
bench = d.bench
stats = collections.Counter()
bad = []
writes = [0, 0]


def outside(self):
    """the bench's outside() plus the whole speaker and Tandy logs and the PIT and port 61h state"""
    o = hybrid.Bench.outside(self)
    mm = self.m
    o["speaker_all"] = repr(mm.speaker_log)
    o["tandy_all"] = repr(mm.tandy_log)
    o["pit0_log"] = repr(mm.pit0_log)
    o["hw"] = repr((mm.port61, mm.pit2_reload, mm.pit2_lo, mm.pit_mode_access, mm.pit_latch_lo, mm.pit_reload,
                    getattr(mm, "_pit_lo", None), mm.irq0_in_service, mm.pic_mask))
    return o


bench.outside = types.MethodType(outside, bench)


def call(tag, name, *args, **kw):
    # the logs written by this call only (each call compares all of them; kept short so the bench's
    # save and restore stay fast)
    m.speaker_log, m.tandy_log, m.pit0_log = [], [], []
    m.adlib_log = []
    r = d.call(name, *args, **kw)
    if r["status"] == "same":
        writes[0] += len(m.speaker_log)
        writes[1] += len(m.tandy_log)
    stats[(tag, name, r["status"])] += 1
    if r["status"] != "same":
        bad.append((tag, name, r))
        if len(bad) < 10:
            print("!!", tag, name, r)
    return r


def wb(off, v):
    d.write(SND, off, bytes([v & 0xFF]))


def ww(off, v):
    d.write(SND, off, (v & 0xFFFF).to_bytes(2, "little"))


def rb(off):
    return d.read(SND, off, 1)[0]


def tick(tag, n=1):
    for _ in range(n):
        call(tag, "SoundTimerIsr", ds=random.choice((hybrid.DGROUP, 0x1234, 0)))


def play_song(tag, ptr, cap):
    call(tag, "StartSound", (ptr, 4))
    for t in range(cap):
        tick(tag)
        if rb(PLAYING) == 0:
            return t + 1
    return cap


# the block at _TEXT:0416, where LoadSoundDriver reads it
d.write(L, 0x416, BLOCK)
# a divide-error handler for the E6h faults: mov word ptr es:[bx], 1000h; iret
d.write(0x8000, 0, bytes([0x26, 0xC7, 0x07, 0x00, 0x10, 0xCF]))
d.write(0, 0, (0x8000 << 16).to_bytes(4, "little"))

t0 = time.time()
for driver in (0, 1):
    tag = "tandy" if driver else "speaker"
    wb(TANDY, driver)
    call(tag, "SoundDriverInstall")
    if driver:
        call(tag, "SoundDriverResume")
    lengths = []
    for i in range(NSONG):
        lengths.append(play_song(tag + "-song", (L << 16) | (0x416 + SONG[i]), 400))
    print(tag, "songs played, ticks:", lengths)
    # a stop in the middle of a song (StopSound has no prototype: its one store, by hand)
    play_song(tag + "-stop", (L << 16) | (0x416 + SONG[3]), 5)
    wb(STOPREQ, 1)
    tick(tag + "-stop", 3)
    # the silence to one voice during a song, then ticks
    play_song(tag + "-silence", (L << 16) | (0x416 + SONG[5]), 3)
    call(tag, "SoundDriverSilenceToOneVoice")
    tick(tag + "-silence", 3)
    call(tag, "SoundDriverResume")
    call(tag, "SoundDriverUninstall")
print("songs: %.0fs" % (time.time() - t0))

# synthetic streams: every command byte, at segment 7000h
SEGS = 0x7000


def stream(tag, data, ticks, driver, vcount=None, pre=None):
    wb(TANDY, driver)
    d.write(SEGS, 0, bytes(data) + bytes(64))
    call(tag, "StartSound", ((SEGS << 16), 4))
    if pre:
        pre()
    if vcount is not None:
        tick(tag)                     # the start path; then the voice count
        ww(VCOUNT, vcount)
    tick(tag, ticks)


call("synth", "SoundDriverInstall")
for driver in (0, 1, 2):
    T = "synth%d" % driver
    for vc in (1, 3, None):
        # notes, a rest, an FFh duration, a 0 duration
        stream(T + "-notes", [0x10, 2, 0x00, 1, 0x5F, 0xFF, 0x20, 0, 0x30, 3, 0xFF], 12, driver, vc)
        # CDh / CEh loop, 0 count read as 1, and a CEh with no CDh before (count from the image or 0)
        stream(T + "-cdce", [0xCD, 3, 0x10, 0, 0xCE, 5, 0, 0xCD, 0, 0x11, 0, 0xCE, 4, 0, 0xFF], 40, driver, vc)
        stream(T + "-d1d2", [0xD1, 2, 0x12, 0, 0xD2, 5, 0, 0xD1, 0, 0x13, 0, 0xD2, 4, 0, 0xFF], 40, driver, vc)
        # E2h (0 read as 1), E8h (wrap, signed overflow), E9h (saturate, wrap)
        stream(T + "-len", [0xE2, 0, 0x10, 1, 0xE2, 5, 0x11, 1, 0xE8, 10, 0x12, 0, 0xE2, 0x81, 0xE8, 2, 0x13, 0,
                            0xE2, 100, 0xE9, 100, 0x14, 0, 0xE2, 200, 0xE9, 100, 0x15, 0, 0xE2, 1, 0xE8, 1, 0x16, 0,
                            0xFF], 300, driver, vc)
        # E6h: explicit frequencies, the Tandy clamp, small and large words
        stream(T + "-e6", [0xE6, 0x58, 0x1B, 2, 0xE6, 0x13, 0x00, 1, 0xE6, 0x02, 0x00, 0, 0xE6, 0xFF, 0xFF, 1,
                           0xE6, 0x40, 0x00, 0, 0xFF], 20, driver, vc)
        # 60h to 7Fh: muted for good; 80h and others: end
        stream(T + "-60", [0x10, 0, 0x60, 0x10, 0], 6, driver, vc)
        stream(T + "-7f", [0x7F], 4, driver, vc)
        for c in (0x80, 0x88, 0xA0, 0xCC, 0xCF, 0xD0, 0xD3, 0xE0, 0xE7, 0xEA, 0xF4, 0xFE):
            stream(T + "-end", [0x10, 0, c, 0x11, 0], 4, driver, vc)
        # loop target with signed overflow (mutes one tick, then reads the wrapped position)
        stream(T + "-ovf", [0xCD, 2, 0xCE, 0x01, 0x80, 0x10, 0], 6, driver, vc)
        stream(T + "-ovf2", [0xD1, 2, 0xD2, 0x01, 0x80, 0x10, 0], 6, driver, vc)
        stream(T + "-ovf3", [0xCD, 2, 0x10, 0, 0xCE, 0x05, 0x80, 0xFF], 6, driver, vc)
        # E6h divide errors (INT 0; the test handler rewrites the divisor and returns to the div)
        stream(T + "-div0", [0xE6, 0x00, 0x00, 1, 0xFF], 3, driver, vc)
        stream(T + "-div1", [0xE6, 0x01, 0x00, 1, 0xFF], 3, driver, vc)
        stream(T + "-div12", [0xE6, 0x12, 0x00, 1, 0xFF], 3, driver, vc)
    # the stop request on sounding and reading voices
    stream(T + "-stop", [0x10, 5, 0x11, 5, 0xFF], 2, driver, None, None)
    wb(STOPREQ, 1)
    tick(T + "-stop", 3)
    call(T, "SoundDriverResume")
    call(T, "SoundDriverSilenceToOneVoice")

# tick and chain counters: wraps and signed compares
for tc, sc, cc in ((0x122, 0, 5), (0x123, 0x3A, 0), (0x123, 0x3B, 0x100), (0x7FFF, 0x7F, 0x1FF), (0x8000, 0x80, 0xFF00),
                   (0xFFFF, 0xFF, 1), (0x124, 0x3C, 0x0F)):
    ww(TICK, tc); wb(SECS, sc); ww(CHAIN, cc)
    tick("counters", 2)

# random states: the driver's variables, the song bytes, the counters
rnd = random.Random(1948)
for k in range(1500):
    var = bytearray(rnd.getrandbits(8) for _ in range(VAR_HI - VAR_LO))
    d.write(SND, VAR_LO, bytes(var))
    wb(TANDY, rnd.choice((0, 1, 1, 0, 2)))
    ww(VCOUNT, rnd.choice((1, 1, 3, 3, 2, 4, 0, 0xFFFF, 5)))
    for o in (0x0E, 0x10, 0x12, 0x14):
        ww(o, rnd.choice((0, rnd.getrandbits(16), rnd.randrange(0, 64))))
    ww(0x0A, rnd.choice((0, 0x100, 0xFFF0, rnd.getrandbits(16))))
    ww(0x0C, SEGS)
    ww(0x16, rnd.choice((0, 0xFFE0, rnd.getrandbits(16))))
    ww(0x18, SEGS)
    ww(0x06, rnd.choice((0x0F, rnd.getrandbits(16))))
    ww(CHAIN, rnd.getrandbits(16))
    # mostly well-formed bytes: notes, commands with operands
    song = bytearray()
    while len(song) < 4096:
        r = rnd.random()
        if r < 0.5:
            song += bytes([rnd.randrange(0, 0x60), rnd.getrandbits(8)])
        elif r < 0.9:
            c = rnd.choice((0xCD, 0xCE, 0xD1, 0xD2, 0xE2, 0xE8, 0xE9, 0xE6))
            if c in (0xCE, 0xD2):
                song += bytes([c]) + rnd.choice((rnd.randrange(0, 40), rnd.getrandbits(16))).to_bytes(2, "little")
            elif c == 0xE6:
                song += bytes([c]) + rnd.choice((rnd.randrange(0x13, 0x4000), rnd.getrandbits(16))).to_bytes(2, "little") + bytes([rnd.getrandbits(8)])
            else:
                song += bytes([c, rnd.getrandbits(8)])
        else:
            song += bytes([rnd.getrandbits(8)])
    d.write(SEGS, 0, bytes(song) * 16)
    tick("random", rnd.choice((1, 1, 2, 5)))

print("total %.0fs" % (time.time() - t0))
by = collections.Counter()
for (tag, name, st), n in sorted(stats.items()):
    by[(tag.split("-")[0], name, st)] += n
for k, n in sorted(by.items()):
    print("%-10s %-30s %-12s %d" % (k[0], k[1], k[2], n))
print("speaker log entries:", writes[0], "tandy writes:", writes[1])
print("calls:", sum(stats.values()), "not same:", len(bad))
sys.exit(1 if any(r.get("status") in ("differs", "C stopped") for *_x, r in bad) else 0)
