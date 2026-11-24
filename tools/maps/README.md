# tools/maps: player maps of the twelve levels

`make maps` writes US Letter landscape PDFs to `docs/maps/`, read from the game's own files:

| File | What |
|---|---|
| `docs/maps/dungeon.pdf` | One page: the twelve levels as small plans, every way between them (stairs, holes, portals), where the stone items that work the portals lie, and how to read the maps |
| `docs/maps/level-01.pdf` to `level-12.pdf` | One level each: the plan, the key, the ways in and out, the numbered notes; a second page when the notes do not fit beside the plan |
| `docs/maps/eye-of-the-beholder-maps.pdf` | All of the above in one file |

They are player aids made from the original data, not part of the decompilation proof. Alone: `uv run --with reportlab python tools/maps/build.py 7` (one level) or `... build.py dungeon`. `reportlab` is not a project dependency; `uv run --with` supplies it. The fonts are Arial from macOS (`/System/Library/Fonts/Supplemental`); elsewhere the built-in Helvetica is used. Building takes about a second.

## What is drawn, and where it comes from

Every statement on a page is an instruction of a level script or a byte of a data file; nothing is copied from walkthroughs. The whitepaper sections cited are where each format is proven against the code.

- **The plan** is `LEVELn.MAZ` (whitepaper 5.3.1): 32 by 32 cells, each with four sides in the order north, east, south, west, a side being a wall kind. A cell none of whose sides lets the party in is drawn as rock. Coordinates are the game's: cell number `y * 32 + x`, written `(x,y)`, both from 0 at the top left, north up.
- **Wall kinds** (whitepaper 5.3.2): kinds 0 to 22 are the program's own (`gWallTypeFlags`, `gWallHandlerIndex` in `src/D07C8.C`, read by `level.py`); the level file defines the rest (the `FB` records of `LEVELn.INF`, `DefineWallType`). The flag byte decides rock, floor and doors (bits 3 to 5); the handler decides what a click does: 1 a door button, 2 the cell's script, 3 and 4 a lever, 5 a stuck door to force, 10 a niche. A face the party can walk through that still draws a wall picture is drawn as an illusion (level 5's walls with the "rune of safe passage" are these).
- **Doors** (whitepaper 5.4.3) are cells whose two opposite sides carry a door kind; the bar is drawn across the passage. Its colour is the closed frame's handler: a button (families A and C), "opened from elsewhere" (B and D, handler 6: a lever, a key or a plate), stuck (handler 5).
- **Scripts** (whitepaper 5.5): `level.py` disassembles every byte of `LEVELn.INF` between the wall records and the trigger table with the operand formats of the handlers in `src/E122E.C`, and checks that the last script ends exactly at the table. `analyze.py` walks each trigger's script through every branch and call, collects what it does (level changes, party moves, door and wall changes, text, experience, new items, monsters, damage, spells launched, panels, the item a lock or plate tests) and names the event: stairs (a `LOAD LEVEL` that sets the facing), a hole (one that keeps it, `FF`), a portal (one guarded by a held item), a teleporter (a party `MOVE` that is not to a neighbouring cell), a lock, a trap, a switch, writing. How it is set off comes from the trigger's mask: click, step, leave, drop, take, throw.
- **Stairs push**: 54 scripts end in four `IF literal == 0..3` groups that move the party (or a dropped item) to a neighbouring cell (whitepaper 5.5.5); moves to a neighbour are therefore not teleporters.
- **Items** are `ITEM.DAT` records whose level is the level and whose key (cell) is positive (`RebuildItemList`). Names are what the game shows once an item is identified (`DrawItemName`, `src/E0BD2.C`: "Mage Scroll of", "Potion of", the `+n`).
- **Monsters** are the 30 records of the level file (`InitMonster`); names are the sprite sheets the level pairs with each type (whitepaper 5.6.1, not names the game prints). The first drop item is dropped half the time, the second always (`KillMonster`, `src/E08E6.C`). The level timer's script (cell 0) adds monsters every so many steps.
- **Grey floor** is floor that no route reaches from the cells the party arrives in (the `LOAD LEVEL` targets of the twelve files, and cell 490 on level 1), counting every door as openable, every wall a script opens as open and every teleporter. It marks unused space and closed rooms; a cell reached only by a portal from another level counts as reached.
- **The tips** in the box at the top of each page are hand-written in `build.py`, each checked against the script of the cell it names (level 11's button order was found by searching the flips the eight button scripts make).

```
python3 tools/maps/level.py 7       # the level's header, monsters, wall kinds, items and every script as text
python3 tools/maps/analyze.py 7     # every event of the level, as the notes word it
```

## Files

- `level.py`: reads a level (`LEVELn.MAZ`, `LEVELn.INF`, `ITEM.DAT` from the PAKs in `eob/`), the item names, and disassembles the scripts.
- `analyze.py`: what each cell script does, in a player's words.
- `render.py`: draws a level's pages (the plan, the key, the notes), with the reachability shading.
- `dungeon.py`: the overview page.
- `build.py`: the per-level titles and tips, the box at the top of each page, and the entry point.

## Known gaps

- What a wall decoration looks like is not drawn: a face is marked by what it does (its handler and script), not by its picture.
- A script that tests flags set elsewhere is described by what it can do, not by when it does it; the notes say "the first time" only for experience guarded by a level flag.
- Some scripts share subroutines (`CALL`); an event lists everything its calls can do, so cells that share a quest subroutine repeat its reward.
- Monster names are sheet names, and the game's text is quoted as it is written (lower case, the authors' spelling).
