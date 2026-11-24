#!/usr/bin/env python3
"""Build the player maps of Eye of the Beholder into docs/maps/.

  uv run --with reportlab python tools/maps/build.py          # all: the dungeon page, the twelve levels, the book
  uv run --with reportlab python tools/maps/build.py 3        # level 3 only
  uv run --with reportlab python tools/maps/build.py dungeon  # the overview page only

Reads the pinned originals in eob/ (the PAK archives) and the program's own tables from src/
(item, spell and wall kind tables, matched to EOB.EXE). Writes docs/maps/level-NN.pdf, one or two
US Letter landscape pages each, docs/maps/dungeon.pdf (how the levels connect) and
docs/maps/eye-of-the-beholder-maps.pdf (all of them in one file). See tools/maps/README.md."""
from __future__ import annotations

import os
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
OUT = os.path.join(os.path.dirname(os.path.dirname(HERE)), 'docs', 'maps')

import analyze  # noqa: E402
import level as lv  # noqa: E402

WALLS = {'brick': 'brick', 'blue': 'blue stone', 'drow': 'drow', 'green': 'green stone', 'xanatha': 'Xanathar'}

# The subtitle of each level names its wall set and what its own writing calls it; `tip` is a
# sentence a player can use, each one checked against the script it describes (cell given).
LEVELS = {
    1: dict(subtitle='Brick walls: the sewers under Waterdeep', start=[490],
            tip='A new party starts at (10,15) facing north. Put any dagger in the niche at (12,24) '
                'and it becomes Dagger +4 \'Guinsoo\'. The kobold at (11,19) carries the Silver Key for the lock at (1,4).'),
    2: dict(subtitle='Brick walls: the sewers, "entry level" and the "correction facility"',
            tip='Each of the four "odd carvings" takes a dagger; with all four filled the special quest leaves '
                'four rations. The portal at (7,1) takes the Stone Ring.'),
    3: dict(subtitle='Brick walls: "city of Waterdeep, rapid access transport system"',
            tip='The four holes at (7,5), (11,7), (9,11) and (5,9) take the Blue Gem (the special quest): they open the '
                'walls at (6,6) and (10,10), leave potions and hand the gem back.'),
    4: dict(subtitle='Blue stone: the dwarven kingdom ("Kruen, king under the mountain")',
            tip='Seven keyholes take the Dwarven Key; lock picks also work at (2,3) and (2,10).'),
    5: dict(subtitle='Blue stone: the dwarven kingdom, with its holes and the dwarves\' camp',
            tip='The pantry at (10,14): five rations left there complete the special quest and come back as iron rations. '
                'The walls marked with the rune of safe passage are illusions you can walk through.'),
    6: dict(subtitle='Blue stone: the lowest dwarven level and the kenku nest',
            tip='Ten kenku eggs at (13,17) complete the special quest and open the nest walls around it.'),
    7: dict(subtitle='Drow walls: the portal hub',
            tip='The five portals at (11,28) to (15,28) each need a stone item held in the hand: the Stone Holy Symbol, '
                'Necklace, Dagger, Medallion and Gem. Levels 2, 4, 5, 9 and 11 have the portals that lead back here.'),
    8: dict(subtitle='Drow walls: the drider and hell hound level'),
    9: dict(subtitle='Drow walls: the "hall of thieves"',
            tip='Something thrown at (25,18) completes the special quest and opens the wall at (23,18): '
                '"the key lies on the other side".'),
    10: dict(subtitle='Green stone: the mantis hive',
             tip='A kenku egg in each of (27,25), (27,27) and (27,29) completes the special quest: four mantis warriors '
                 'appear, each carrying a ring.'),
    11: dict(subtitle='Green stone: "the celestial star of navigation"',
             tip='The eight buttons at (25,9) to (25,16) each flip themselves and some of the others. Put any scroll '
                 'in the niche at (24,17), then, from the start, press (25,10), (25,11) and (25,13): the special quest '
                 'gives 10,000 experience. Pressing (25,13), (25,14) and (25,16) instead sets all eight alike and '
                 'leaves a Wand of Frost in that niche.'),
    12: dict(subtitle='Xanathar\'s lair',
             tip='You arrive in a closed room. The lever at (13,9) fires a fireball and a lightning bolt; the corner '
                 'cells turn them until one hits the wall at (12,11), which then opens.'),
}


def glance(n, spec):
    """the box at the top of the page: facts from the data, then the level's tip"""
    L = lv.Level(n)
    evs = analyze.events(L)
    mc = Counter(lv.MONSTER_NAMES[m['type']] for m in L.monsters)
    mons = ', '.join('%d %s' % (k, analyze.plural(m) if k > 1 else m) for m, k in mc.most_common())
    timer = [e for e in evs if e.cell == 0 and e.kind == 'monster']
    parts = ['<b>MONSTERS</b> %s%s.' % (mons, (', and the level adds %s every %d steps' % (
        timer[0].title.split(': ', 1)[1], L.timer_steps)) if timer else '')]
    ways = []
    for kind, word in (('up', 'up'), ('down', 'down')):
        tgt = sorted({e.dest[0] for e in evs if e.kind == kind})
        k = sum(1 for e in evs if e.kind == kind)
        if k:
            ways.append('%d stairs %s (to %s)' % (k, word, ', '.join(map(str, tgt))))
    pits = [e for e in evs if e.kind == 'pit']
    if pits:
        ways.append('%d hole%s' % (len(pits), 's' if len(pits) > 1 else ''))
    portals = [e for e in evs if e.kind == 'portal']
    if portals:
        ways.append('%d portal%s (to %s)' % (len(portals), 's' if len(portals) > 1 else '',
                                            ', '.join(str(e.dest[0]) for e in portals)))
    tele = [e for e in evs if e.kind == 'teleport']
    if tele:
        ways.append('%d teleporter%s' % (len(tele), 's' if len(tele) > 1 else ''))
    parts.append('<b>WAYS</b> ' + '; '.join(ways) + '.')
    quest = [e for e in evs if any('quest' in t for t in e.texts)]
    if quest:
        parts.append('<b>SPECIAL QUEST</b> at %s (the game says "special quest for this level!").' % ', '.join(
            lv.lab(e.cell) for e in quest[:6]))
    if spec.get('tip'):
        parts.append('<b>TIP</b> ' + spec['tip'])
    return ' '.join(parts)


def spec_of(n):
    s = dict(LEVELS[n])
    s['glance'] = glance(n, s)
    return s


def main(argv):
    import render
    import dungeon
    os.makedirs(OUT, exist_ok=True)
    args = argv[1:]
    which = [int(a) for a in args if a.isdigit()] or (list(range(1, 13)) if not args else [])
    sizes = {}
    for n in which:
        out = os.path.join(OUT, 'level-%02d.pdf' % n)
        sizes[n] = render.build(n, spec_of(n), out)
        print('level %d: %s (body %.1f pt)' % (n, os.path.relpath(out), sizes[n]))
    if not args or 'dungeon' in args:
        out = os.path.join(OUT, 'dungeon.pdf')
        dungeon.build(out)
        print('dungeon: %s' % os.path.relpath(out))
    if not args:
        from reportlab.pdfgen import canvas
        out = os.path.join(OUT, 'eye-of-the-beholder-maps.pdf')
        c = canvas.Canvas(out, pagesize=render.landscape(render.letter))
        c.setTitle('Eye of the Beholder - player maps')
        c.setAuthor('eob-decomp')
        dungeon.draw(c)
        c.showPage()
        for n in range(1, 13):
            render.render(n, spec_of(n), c, sizes[n])
        c.save()
        print('book: %s' % os.path.relpath(out))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
