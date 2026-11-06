import sys; sys.path.insert(0, '.')
from figlib import *

c = Canvas(760, 390)
c.section(0, 11, "Where an item can be, and what moves it (mouse enabled)")

# three places
A = (270, 30, 220, 92)
B = (0, 230, 230, 96)
C = (530, 230, 230, 96)
c.box(*A, kind="red", title="the pointer", body=["gMouseItem (ds:FBCE): one item id, 0 for none.", "SetMouseItem also makes it the cursor image."], size=10.5, tsize=13)
c.box(*B, kind="teal", title="a character slot", body=["gChars[who].items[slot], 27 per character.", "Slot 16 is the head of a list (quiver)."], size=10.5, tsize=13)
c.box(*C, kind="navy", title="a cell's item list", body=["Head word at gMaze + cell * 9 + 5,", "then the ring of records (next figure)."], size=10.5, tsize=13)

# pointer <-> slot
c.arrow([(300, 122), (160, 230)], color=SOLID["teal"], both=True)
c.text(110, 160, "click a slot: UseSelectedItem", size=10.5, weight=700, color=PAL["teal"][0], anchor="middle")
c.text(110, 174, "0BD2:0BA0 swaps them if the slot", size=10, color=INK2, anchor="middle")
c.text(110, 187, "takes the pointer's item", size=10, color=INK2, anchor="middle")
c.text(110, 200, "(CanPlaceItemInSlot), then", size=10, color=INK2, anchor="middle")
c.text(110, 213, "ComputeArmourClass", size=10, color=INK2, anchor="middle")
# pointer <-> cell
c.arrow([(460, 122), (600, 230)], color=SOLID["navy"], both=True)
c.text(650, 160, "click the floor: ClickFloorItem", size=10.5, weight=700, color=PAL["navy"][0], anchor="middle")
c.text(650, 174, "0ACD:0644. Pointer full: insert", size=10, color=INK2, anchor="middle")
c.text(650, 187, "into the list, run the cell script", size=10, color=INK2, anchor="middle")
c.text(650, 200, "with 4 (drop). Pointer empty: remove,", size=10, color=INK2, anchor="middle")
c.text(650, 213, "script 8 (take)", size=10, color=INK2, anchor="middle")
# slot <-> cell
c.arrow([(230, 290), (530, 290)], color=GREY, both=True)
c.text(380, 282, "keys (no mouse): MoveListItem, PickListItem", size=10.5, color=INK2, anchor="middle")
c.text(380, 312, "member removed: RemovePartyMember drops all items", size=10, color=MUTED, anchor="middle")
c.text(380, 325, "on the party's cell, a random front position", size=10, color=MUTED, anchor="middle")
# thrown
c.box(280, 140, 200, 54, "ochre", "thrown or shot", ["AddDDEffect: a projectile record flies and the item travels in the cell's list."], size=9.5, tsize=11.5)
c.arrow([(380, 122), (380, 140)], color=SOLID["ochre"])
# bottom notes
c.text(0, 360, "With the mouse disabled the keyboard cursor stands in for the pointer: GetCurrentItem and SetCurrentItem read and write", size=10.5, color=INK2)
c.text(0, 374, "items[gCursorSlot] of the selected member instead of gMouseItem.", size=10.5, color=INK2)
c.save("figs/chr_itemflow.svg")
