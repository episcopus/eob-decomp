import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
# lane 1: timer
c.section(0, 11, "Timer: everything the game calls time comes from IRQ0")
c.box(0, 24, 120, 78, "grey", "PIT channel 0", ["IRQ0 at 18.2 Hz, or faster while a sound driver runs"], size=10, tsize=11.5)
c.box(150, 24, 190, 78, "ochre", "INT 08h, driver ISR", ["AdLib: divisor 555h, 874 Hz, every 48th pass chains on;", "speaker, Tandy: 1000h, 291 Hz, every 16th"], size=9.8, tsize=11.5)
c.box(370, 24, 100, 78, "grey", "BIOS INT 08h", ["18.2 Hz, then INT 1Ch"], size=10, tsize=11.5)
c.box(500, 24, 254, 78, "navy", "INT 1Ch: 1D8F:0055", ["adds 1 to each counter below, counts a countdown down, chains to the old handler"], size=10, tsize=11.5)
for x in (121, 341, 471):
    c.arrow([(x, 63), (x + 28, 63)], color=GREY)
# counters table
c.text(0, 126, "What the handler writes", size=11, weight=700, color=PAL["navy"][0])
rows = [
    ("ds:8488", "gTicksSinceEvent", "16-bit, always", "stamps recorded input events"),
    ("ds:848A", "gTicks", "32-bit, always", "WaitTicks, cursor blink, delays, selection frame"),
    ("ds:8492", "gGameTicks", "32-bit, only while ds:849E bit 1", "timed effects, level script timer, member effect timers"),
    ("ds:849A", "gFrameWaitTicks", "32-bit countdown to 0", "DrawViewFrame waits for it: holds attack and hit frames"),
    ("ds:848E, 8496", "(no reader)", "32-bit each; 848E only while bit 0", "the counters nothing in src/ reads"),
]
y = 134
c.rect(0, y, 760, 18, fill=PANEL, rx=3)
for x, t in ((8, "address"), (92, "name"), (240, "kind"), (440, "read by")):
    c.text(x, y + 13, t, size=10.5, weight=700, color=INK2)
y += 20
for a, n, k, r in rows:
    c.text(8, y + 13, a, size=10.3, color=INK2, mono=True)
    c.text(92, y + 13, n, size=10.8, weight=600, color=INK)
    c.text(240, y + 13, k, size=10.3, color=INK2)
    c.text(440, y + 13, r, size=10.3, color=INK2)
    y += 19
c.text(0, y + 12, "Bit 1 of ds:849E is cleared around the camp menu, level loads, wand and spell animations, hit flashes: game time stops while the player waits.", size=10.5, color=MUTED)

# lane 2: keyboard
y0 = 290
c.section(0, y0 - 10, "Keyboard and mouse: interrupts fill one 128-word ring")
c.box(0, y0, 120, 70, "grey", "IRQ1", ["scancode on port 60h"], size=10, tsize=11.5)
c.box(150, y0, 190, 70, "teal", "INT 9: 1AAD:07C0", ["scancode to key code, held-key bitmap, Alt-S and Alt-M toggle sound"], size=9.8, tsize=11.5)
c.box(370, y0, 170, 70, "navy", "ring, 1AAD:007C", ["256 bytes of events: 1 word, or 3 for a mouse click (code, x, y)"], size=9.8, tsize=11.5)
c.box(570, y0, 184, 70, "plum", "readers", ["IsKeyPending, ReadKey, WaitKeyEvent, WaitGameKey, GetKeyState, FlushKeyBuffer"], size=9.8, tsize=11.5)
for x in (121, 341, 541):
    c.arrow([(x, y0 + 35), (x + 28, y0 + 35)], color=GREY)
c.box(0, y0 + 90, 120, 70, "grey", "mouse driver", ["INT 33h function 0Ch callback"], size=10, tsize=11.5)
c.box(150, y0 + 90, 190, 70, "teal", "1A72:017A", ["clamp x, publish gCursorX/Y, redraw the game's own pointer, report buttons"], size=9.8, tsize=11.5)
c.box(370, y0 + 90, 170, 70, "teal", "1AAD:0B0E", ["a button change becomes code 41h (left) or 42h (right), release marked by AH bit 8"], size=9.8, tsize=11.5)
for x in (121, 341):
    c.arrow([(x, y0 + 125), (x + 28, y0 + 125)], color=GREY)
c.arrow([(455, y0 + 90), (455, y0 + 72)], color=GREY)
c.para(570, y0 + 108, "Waiting for the screen is not an interrupt: WaitFrames polls port 3DAh bit 3 (vertical retrace).", 184, size=10.3, color=INK2)
c.h = y0 + 172
c.save("figs/arc_clocks.svg")
