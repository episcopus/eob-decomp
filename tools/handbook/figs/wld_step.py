import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 520)
c.section(0, 11, "ONE STEP OF THE PARTY (left) AND ONE PASS OF THE MAP LOOP THAT RUNS EVERYTHING ELSE (right)")
LW = 340
steps = [
    ("teal", "movement key", "MoveForward, MoveBackward, MoveLeft, MoveRight (0ACD:04F8 to 05FF): heading d = facing, +2, -1 or +1. The arrows move without turning; TurnLeft and TurnRight only change gPartyDir."),
    ("navy", "StepIfOpen(gPartyCell, d)", "target = StepCell; the wall of the target on side d ^ 2 needs flag bit 0; no monster in the target. Else -1: 'You can't go that way.', sound 0x1D, key buffer flushed."),
    ("navy", "MoveParty(target)  122E:0001", "AlertNearbyMonsters; gPartyCell = target; RunCellScript(old, 2); RunCellScript(new, 1); AlertNearbyMonsters; gStepCount++; gPickViewPos = 0xFF; gViewDirty = 1; a newline to the message area; UpdateDDEffects."),
    ("grey", "the next pass of the loop", "gViewDirty makes DrawViewFrame redraw the view; gStepCount feeds the level timer."),
]
y = 30
for k, t, b in steps:
    lines = wrap(b, LW - 20, 10.3)
    h = 28 + 12.5 * len(lines)
    c.rect(0, y, LW, h, fill=PAL[k][2], stroke=PAL[k][1], sw=1.1, rx=6)
    c.text(10, y + 17, t, size=11.5, weight=700, color=PAL[k][0], mono=(k == "navy"))
    yy = y + 31
    for l in lines:
        c.text(10, yy, l, size=10.3, color=INK2)
        yy += 12.5
    if k != "grey":
        c.arrow([(LW / 2, y + h + 1), (LW / 2, y + h + 12)], color=GREY)
    y += h + 13
# right: loop pass
RX = 372
RW = 388
c.text(RX, 40, "RunMapLoop (0ACD:0E8A), every pass", size=11.5, weight=700, color=PAL["plum"][0])
passes = [
    ("PollMapInput", "wait for one key or mouse event, dispatch it to the screen nodes, then HandleMapKey"),
    ("RunEffects  (1414:04E7)", "every effect of the 16 whose expiry has passed runs its handler; then TickLevelScriptTimer"),
    ("DrawViewFrame(1)", "only when gViewDirty is set"),
    ("ambient sound", "levels 1 to 3: every (4 to 13) x 18 ticks, sound 0x1E at a cell 0 to 11 from the party"),
    ("PlaySoundDefault(0), TurnUndead", "the turn-undead check runs on every pass (levels 2 and 7 only)"),
]
y = 52
for t, b in passes:
    lines = wrap(b, RW - 20, 10.3)
    h = 28 + 12.5 * len(lines) - 4
    c.rect(RX, y, RW, h, fill=PAL["plum"][2], stroke=PAL["plum"][1], sw=1.1, rx=6)
    c.text(RX + 10, y + 16, t, size=11, weight=700, color=PAL["plum"][0], mono=True)
    yy = y + 29
    for l in lines:
        c.text(RX + 10, yy, l, size=10.3, color=INK2)
        yy += 12.5
    y += h + 8

# effect table
ty = max(y, 330) + 10
c.line(0, ty - 8, 760, ty - 8, stroke=RULE)
c.section(0, ty + 6, "THE 16 TIMED EFFECTS (gEffects, ds:5BA8): HANDLER AND PERIOD IN TICKS")
eff = [
    ("0", "selected member blinks", "9", "grey"), ("1", "flying items and spells (MoveDDEffects)", "3", "red"),
    ("2 to 5", "monster groups 0 to 3 (UpdateMonsters)", "INF", "red"), ("6 to 11", "the six members' timed effects", "50", "grey"),
    ("12", "door animation (StepDoor)", "5", "ochre"), ("13", "animated wall kind 0x34 (teleporter?)", "10", "teal"),
    ("14", "food of each member", "1080", "grey"), ("15", "monster draw-offset jitter", "25", "red"),
]
yy = ty + 26
for i, (n, d, p, k) in enumerate(eff):
    col, row = divmod(i, 4)
    x = col * 380
    yv = yy + row * 20
    c.rect(x, yv - 12, 50, 17, fill=PAL[k][2], stroke=PAL[k][1], sw=1, rx=3)
    c.text(x + 25, yv + 1, n, size=10.5, weight=700, color=PAL[k][0], anchor="middle", mono=True)
    c.text(x + 60, yv + 1, d, size=10.8, color=INK2)
    c.text(x + 360, yv + 1, p, size=10.5, color=MUTED, anchor="end", mono=True)
c.h = yy + 4 * 20 + 6
c.save("figs/wld_step.svg")
