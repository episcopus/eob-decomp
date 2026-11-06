import sys; sys.path.insert(0, '.')
from figlib import *

rows = [
    ("grey", "1  C0L start-up code", "0000:0000",
     ["DS = DGROUP (212E), the C runtime and the overlay manager are set up, then main is called (07C8:0001)"],
     "before any game code"),
    ("teal", "2  Main", "07C8:0001",
     ["reads argv: graphics, sound, mouse digits, the word homie, the mode to restore",
      "saves INT 3Fh, installs Int3FHandler; needs a 65,000-byte free block",
      "hooks: gExitHook, gAllocFailHook; InstallKeyboard, InstallHardErr, InstallTickHandler"],
     "INT 9, INT 1Ch, INT 24h, INT 3Fh taken"),
    ("teal", "3  StartupGame", "07C8:0199",
     ["video mode, InitMouse, load the graphics driver *.OVL (RunOverlayRequest)",
      "build the page table (8 segments, MemAlloc), load FONT8 and FONT6, palette",
      "InitOverlayCache (file table), CalibrateRetrace, srand(time), SetTextWindow(0), sound card on"],
     "gStartupDone = 1"),
    ("teal", "4  Main again", "07C8:0001",
     ["eobpal.col to the DAC for VGA, text attributes 0F and 0C, node callback cleared",
      "mouse flag from argv[3] and whether a driver answered; calls StartupAlloc"],
     "gKeyboardOptions = 1"),
    ("navy", "5  StartupAlloc", "07A9:000A",
     ["five MemAlloc blocks: 7,500, 6,000, 120, 4,096 and 1,600 bytes",
      "GetPageBuffer(7), (5), (3); LoadSoundDriver; SetUnpackBuffer; then LoadItemData"],
     "gBarGraphsOn = 1"),
    ("plum", "6  LoadItemData", "ovl2:09CC",
     ["pointer shape, InitGameTables, ITEM.DAT, ITEMTYPE.DAT, UI colours, INTRO picture",
      "LoadGamePanel: the title menu; returns once a game is loaded or a party is made"],
     "title menu runs here"),
    ("navy", "7  RunMapLoop", "0ACD:0E8A",
     ["after LoadDecorateIcons (item sheets, ITEMRMP) the game loop starts and never returns",
      "the program ends in ShutdownGame (07C8:058A), through exit"],
     "the game"),
]
RH, GAP = 70, 12
c = Canvas(760, len(rows) * (RH + GAP) + 6)
BW = 560
for i, (k, t, a, body, note) in enumerate(rows):
    y = 4 + i * (RH + GAP)
    tc, sc, fc = PAL[k]
    c.rect(0, y, BW, RH, fill=fc, stroke=sc, sw=1.2, rx=6)
    c.text(10, y + 18, t, size=13.5, weight=700, color=tc)
    c.text(BW - 10, y + 18, a, size=11.5, color=tc, anchor="end", mono=True)
    yy = y + 35
    for b in body:
        for l in wrap(b, BW - 22, 11.5):
            c.text(10, yy, l, size=11.5, color=INK2)
            yy += 14
    c.text(BW + 22, y + RH / 2 + 4, note, size=11.5, color=MUTED)
    c.line(BW + 4, y + RH / 2, BW + 16, y + RH / 2, stroke=RULE, sw=1.2)
    if i < len(rows) - 1:
        c.arrow([(BW / 2, y + RH + 1), (BW / 2, y + RH + GAP - 1)], color=GREY)
c.save("figs/arc_startup.svg")
