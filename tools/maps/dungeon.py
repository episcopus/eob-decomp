#!/usr/bin/env python3
"""dungeon.py: one page, how the twelve levels of Eye of the Beholder connect.

Each level is a small plan (rock and floor from LEVELn.MAZ) with its stairs, holes and portals
marked, in two rows by wall set. Under the plans: every way between the levels (from the LOAD LEVEL
instructions of the twelve scripts, analyze.py) and where the eight stone items that work the
portals lie (ITEM.DAT, item type 37: the portals test the type and the item's value).
"""
from __future__ import annotations

import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyze  # noqa: E402
import level as lv  # noqa: E402
import render as R  # noqa: E402

from reportlab.lib.colors import HexColor, white  # noqa: E402
from reportlab.lib.styles import ParagraphStyle  # noqa: E402
from reportlab.platypus import Paragraph  # noqa: E402

W, H, M = R.W, R.H, R.M
SETS = {1: 'brick', 2: 'brick', 3: 'brick', 4: 'blue stone', 5: 'blue stone', 6: 'blue stone', 7: 'drow',
        8: 'drow', 9: 'drow', 10: 'green stone', 11: 'green stone', 12: 'Xanathar'}


def thumb(c, P, x0, ytop, size):
    s = size / 32.0
    for cell in range(1024):
        x, y = lv.xy(cell)
        if P.is_solid(cell):
            col = R.ROCK
        elif cell not in P.reach:
            col = HexColor('#cbc5ba')
        else:
            col = R.FLOOR
        c.setFillColor(col)
        c.rect(x0 + x * s, ytop - (y + 1) * s, s + 0.05, s + 0.05, fill=1, stroke=0)
    for e in P.events:
        x, y = lv.xy(e.cell)
        X, Y = x0 + (x + 0.5) * s, ytop - (y + 0.5) * s
        if e.kind in ('up', 'down'):
            c.setFillColor(R.GREEN if e.kind == 'up' else HexColor('#0f5132'))
            c.setStrokeColor(white)
            c.setLineWidth(0.4)
            c.circle(X, Y, 2.3, fill=1, stroke=1)
        elif e.kind == 'pit':
            c.setFillColor(R.INK)
            c.circle(X, Y, 1.1, fill=1, stroke=0)
        elif e.kind == 'portal':
            c.setFillColor(R.PURPLE)
            c.setStrokeColor(white)
            c.setLineWidth(0.4)
            c.circle(X, Y, 2.3, fill=1, stroke=1)
    c.setStrokeColor(R.INK)
    c.setLineWidth(0.6)
    c.rect(x0, ytop - size, size, size, fill=0, stroke=1)


def draw(c):
    pages = {n: R.Page(n, __import__('build').spec_of(n)) for n in range(1, 13)}
    c.setFillColor(R.INK)
    c.setFont(R.FONTB, 22)
    c.drawString(M, H - M - 19, 'THE DUNGEON')
    c.setFont(R.FONT, 8.6)
    c.setFillColor(R.MUTED)
    tx = M + R.stringWidth('THE DUNGEON', R.FONTB, 22) + 12
    c.drawString(tx, H - M - 8, 'Twelve levels under Waterdeep, and every way between them')
    c.drawString(tx, H - M - 19, 'Eye of the Beholder (Westwood, DOS)  |  player maps  |  SPOILERS')
    c.setFont(R.FONTI, 6.3)
    c.drawRightString(W - M, H - M - 8, 'Read from the game\'s own level files: walls, doors, scripts, items, monsters.')
    c.drawRightString(W - M, H - M - 16, 'Each level has its own map: level-01.pdf to level-12.pdf.')
    c.setStrokeColor(R.INK)
    c.setLineWidth(1.1)
    c.line(M, H - M - 26, W - M, H - M - 26)

    size = 104
    gap = (W - 2 * M - 6 * size) / 5
    tops = [H - M - 52, H - M - 52 - size - 40]
    for n in range(1, 13):
        row, col = divmod(n - 1, 6)
        x0 = M + col * (size + gap)
        ytop = tops[row]
        c.setFillColor(R.INK)
        c.setFont(R.FONTB, 9)
        c.drawString(x0, ytop + 5, 'LEVEL %d' % n)
        c.setFont(R.FONT, 6.2)
        c.setFillColor(R.MUTED)
        c.drawRightString(x0 + size, ytop + 5, SETS[n])
        thumb(c, pages[n], x0, ytop, size)
        evs = pages[n].events
        ups = sorted({e.dest[0] for e in evs if e.kind == 'up'})
        downs = sorted({e.dest[0] for e in evs if e.kind == 'down'})
        pits = sorted({e.dest[0] for e in evs if e.kind == 'pit'})
        portals = sorted({e.dest[0] for e in evs if e.kind == 'portal'})
        bits = []
        if ups:
            bits.append('up to %s' % ', '.join(map(str, ups)))
        if downs:
            bits.append('down to %s' % ', '.join(map(str, downs)))
        if pits:
            bits.append('holes to %s' % ', '.join(map(str, pits)))
        if portals:
            bits.append('portals to %s' % ', '.join(map(str, portals)))
        c.setFillColor(R.INK)
        c.setFont(R.FONT, 6.0)
        line = '; '.join(bits)
        words = R.wrap_words(line, size, 6.0)
        for i, w_ in enumerate(words[:3]):
            c.drawString(x0, ytop - size - 8 - i * 7, w_)

    # the key of the small plans
    ky = tops[1] - size - 34
    c.setFont(R.FONTB, 7)
    c.setFillColor(R.INK)
    c.drawString(M, ky, 'KEY')
    x = M + 24
    for col, text, kind in ((R.GREEN, 'stairs up', 'c'), (HexColor('#0f5132'), 'stairs down', 'c'),
                            (R.PURPLE, 'portal', 'c'), (R.INK, 'hole', 'd'), (HexColor('#cbc5ba'), 'floor no way leads to', 's')):
        c.setFillColor(col)
        if kind == 's':
            c.rect(x, ky - 1, 7, 7, fill=1, stroke=0)
        else:
            c.circle(x + 3.5, ky + 2.5, 3 if kind == 'c' else 1.6, fill=1, stroke=0)
        c.setFillColor(R.INK)
        c.setFont(R.FONT, 6.4)
        c.drawString(x + 10, ky, text)
        x += 16 + R.stringWidth(text, R.FONT, 6.4) + 10

    # the text columns: ways between levels, the stone items, how to read the maps
    st = ParagraphStyle('t', fontName=R.FONT, fontSize=6.5, leading=7.9, textColor=R.INK)
    cols = 3
    cgap = 16
    colw = (W - 2 * M - (cols - 1) * cgap) / cols
    top = ky - 12
    blocks = []
    blocks.append(('head', 'EVERY WAY BETWEEN THE LEVELS'))
    links = defaultdict(list)
    for n in range(1, 13):
        for e in pages[n].events:
            if e.kind in ('up', 'down', 'portal'):
                links[(n, e.dest[0], e.kind)].append(e)
            elif e.kind == 'pit':
                links[(n, e.dest[0], 'pit')].append(e)
    for (a, b, kind), es in sorted(links.items()):
        word = {'up': 'stairs up', 'down': 'stairs down', 'portal': 'portal', 'pit': 'hole' if len(es) == 1 else 'holes'}[kind]
        cells = ', '.join(lv.lab(e.cell) for e in es[:6])
        if kind == 'pit' and len(es) > 6:
            cells = '%d cells, each falls to the same cell below' % len(es)
        extra = ' (%s)' % R.esc(es[0].body[0]) if kind == 'portal' and es[0].body else ''
        blocks.append(('line', '<b>%d to %d</b> %s: %s%s' % (a, b, word, cells, extra)))
    blocks.append(('head', 'THE STONE ITEMS THAT WORK THE PORTALS'))
    stones = [r for r in lv.items()['records'] if r['type'] == 37 and r['level'] and r['key'] > 0]
    first = {}
    for r in sorted(stones, key=lambda r: (r['level'] == 12, r['level'], r['index'])):
        first.setdefault(r['shown'], r)
    blocks.append(('line', '; '.join('<b>%s</b> level %d %s' % (R.esc(nm), r['level'], lv.lab(r['key']))
                                     for nm, r in first.items()) + '.'))
    set12 = sorted({r['shown'] for r in stones if r['level'] == 12})
    blocks.append(('line', 'Hold the stone and click the portal\'s wall. Level 12 at (28,18) holds %d of them, one of each but the %s.' % (
        len(set12), ', '.join(sorted(set(first) - set(set12))) or 'none')))
    blocks.append(('head', 'HOW TO READ THE MAPS'))
    for t in (
        'Every map is the level\'s 32 by 32 grid, north up, cells numbered (x,y) from 0 at the top left as the game '
        'numbers them. Dark cells are solid rock; light cells are floor. A wall face that does something is a small bar '
        'on the rock beside the floor: blue for a button or lever, red for a keyhole, amber for a niche, pale for writing.',
        'Numbered circles are the notes beside the map or on its second page. Dotted blue lines join a switch, plate or '
        'lock to the door or wall it moves; purple arrows are teleporters; stars give experience the first time.',
        'Everything is read from the game\'s own files: the walls from LEVELn.MAZ, the scripts that make every switch, '
        'trap, message and quest from LEVELn.INF, the items from ITEM.DAT. A note says what a script does; it does not '
        'guess at what the designers meant. Grey floor is floor no stairs, door, switch or teleporter reaches.',
        'The copy read is the fan patch "1.9" of the DOS game.',
    ):
        blocks.append(('line', t))

    R.flow(c, blocks, [(M + i * (colw + cgap), top, colw, M) for i in range(cols)], 6.5, lambda i: None)


def build(out):
    from reportlab.pdfgen import canvas
    c = canvas.Canvas(out, pagesize=(W, H))
    c.setTitle('The dungeon - how the levels connect (Eye of the Beholder)')
    c.setAuthor('eob-decomp')
    draw(c)
    c.showPage()
    c.save()
