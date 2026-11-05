import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 470)
c.section(0, 11, "The game systems of section 5 and how they call each other")
# column of the loop
c.box(280, 28, 200, 62, "teal", "RunMapLoop", ["one pass: input, timed effects,", "redraw, sound, Turn Undead"], size=10.5, tsize=13.5, title_mono=True)
c.box(0, 28, 200, 62, "navy", "5.10 Input and interface", ["keys, mouse regions, menus,", "nodes, text windows"], size=10.5, tsize=12.5)
c.arrow([(202, 59), (278, 59)], color=SOLID["navy"], label="command", ldy=-6)
c.box(560, 28, 200, 62, "grey", "5.16 Random numbers, time", ["ticks at 18.2 Hz, two", "generators, RollDice"], size=10.5, tsize=12.5)
c.arrow([(558, 59), (482, 59)], color=GREY, label="ticks", ldy=-6)

def sysbox(x, y, w, h, kind, num, title, body):
    c.box(x, y, w, h, kind, f"{num} {title}", body, size=10, tsize=11.5)

# the rules layer
c.section(0, 118, "Rules and state", color=PAL["plum"][0], size=10)
sysbox(0, 126, 176, 72, "plum", "5.1", "Party and characters", ["243-byte record, six slots,", "classes, levels, AC, saves"])
sysbox(194, 126, 176, 72, "plum", "5.2", "Items", ["14 + 16 byte records,", "27 slots, the pointer item"])
sysbox(388, 126, 176, 72, "plum", "5.7", "Combat", ["real time, hand cooldowns,", "to-hit, damage, saves"])
sysbox(582, 126, 178, 72, "plum", "5.8", "Spells and effects", ["49 spells, timed slots,", "missiles, status"])
sysbox(0, 214, 176, 60, "plum", "5.9", "Camp, rest, food", ["ten-minute ticks,", "memorising, hunger"])

# the world layer
c.section(0, 296, "The world", color=PAL["ochre"][0], size=10)
sysbox(0, 304, 176, 72, "ochre", "5.3", "Mazes and level state", ["4-byte cells, level files,", "2,040-byte states"])
sysbox(194, 304, 176, 72, "ochre", "5.4", "Moving through the world", ["steps, doors, clicks,", "things in flight"])
sysbox(388, 304, 176, 72, "ochre", "5.5", "Cell scripts", ["26 opcodes and an", "expression language"])
sysbox(582, 304, 178, 72, "ochre", "5.6", "Monsters", ["22 types, four update", "groups, noticing, melee"])

# the output layer
c.section(0, 398, "Output and persistence", color=PAL["teal"][0], size=10)
sysbox(0, 406, 176, 56, "teal", "5.12", "The 3D view", ["18 slots, pre-drawn art"])
sysbox(194, 406, 176, 56, "teal", "5.13", "Graphics output", ["pages, text, pointer"])
sysbox(388, 406, 176, 56, "teal", "5.14", "Sound", ["AdLib scripts, speaker"])
sysbox(582, 406, 178, 56, "grey", "5.11", "Saving and loading", ["EOBDATA.SAV, LEVELS.TMP"])

# arrows from loop to layers
c.arrow([(380, 92), (380, 108), (88, 108), (88, 124)], color=GREY)
c.arrow([(380, 108), (671, 108), (671, 124)], color=GREY)
c.arrow([(380, 108), (282, 108), (282, 124)], color=GREY)
c.arrow([(380, 108), (476, 108), (476, 124)], color=GREY)
c.arrow([(466, 200), (466, 290), (282, 290), (282, 302)], color=GREY, dash="3 2")
c.arrow([(88, 276), (88, 302)], color=GREY, dash="3 2")
c.arrow([(580, 340), (566, 340)], color=GREY, both=True)
c.arrow([(372, 340), (386, 340)], color=GREY, both=True)
c.arrow([(178, 340), (192, 340)], color=GREY, both=True)
c.text(752, 296, "dashed: calls into the world", size=10, color=MUTED, anchor="end")
c.save("figs/sys_map.svg")
