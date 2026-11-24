#!/usr/bin/env python3
"""render.py: the player map of one level of Eye of the Beholder, two US Letter landscape pages.

Page 1 is the plan: the 32 by 32 cells as the game stores them (LEVELn.MAZ), drawn as blocks, with
the doors, the wall faces that do something (buttons, levers, locks, niches, writing), the stairs,
holes and teleporters, the switches joined to what they move, and where the items and the monsters
are at the start. Page 2 is the notes: every cell script that does something worth knowing
(analyze.py), the items on the floor and the monsters.

Needs reportlab (not a project dependency): `uv run --with reportlab python tools/maps/build.py`.
"""
from __future__ import annotations

import os
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyze  # noqa: E402
import level as lv  # noqa: E402

from reportlab import rl_config  # noqa: E402
from reportlab.lib.colors import HexColor, white  # noqa: E402
from reportlab.lib.pagesizes import landscape, letter  # noqa: E402
from reportlab.lib.styles import ParagraphStyle  # noqa: E402
from reportlab.pdfbase import pdfmetrics  # noqa: E402
from reportlab.pdfbase.pdfmetrics import registerFontFamily, stringWidth  # noqa: E402
from reportlab.pdfbase.ttfonts import TTFont  # noqa: E402
from reportlab.platypus import Paragraph  # noqa: E402

rl_config.invariant = 1      # no date or random id in the files: the same data give the same bytes
_F = '/System/Library/Fonts/Supplemental/'
try:
    for _n, _f in [('Arial', 'Arial.ttf'), ('Arial-Bold', 'Arial Bold.ttf'), ('Arial-Italic', 'Arial Italic.ttf'),
                   ('Arial-BoldItalic', 'Arial Bold Italic.ttf')]:
        pdfmetrics.registerFont(TTFont(_n, _F + _f))
    registerFontFamily('Arial', normal='Arial', bold='Arial-Bold', italic='Arial-Italic', boldItalic='Arial-BoldItalic')
    FONT, FONTB, FONTI = 'Arial', 'Arial-Bold', 'Arial-Italic'
except Exception:  # not on macOS: the built-in Helvetica (not embedded)
    FONT, FONTB, FONTI = 'Helvetica', 'Helvetica-Bold', 'Helvetica-Oblique'

INK = HexColor('#2b2118')
ROCK = HexColor('#5b4a3c')
ROCKLINE = HexColor('#47392d')
FLOOR = HexColor('#f6efdc')
GRID = HexColor('#e2d8bf')
MUTED = HexColor('#6d6256')
BOX = HexColor('#efe6cf')
BOXLINE = HexColor('#b9aa86')
RED = HexColor('#b3261e')
BLUE = HexColor('#1f5fa8')
AMBER = HexColor('#c98a1b')
PURPLE = HexColor('#6b3fa0')
GREEN = HexColor('#2f7d3a')
TEAL = HexColor('#0f766e')
GREY = HexColor('#8c8478')
ORANGE = HexColor('#c2410c')
HEX = {'red': '#b3261e', 'blue': '#1f5fa8', 'amber': '#a86f0f', 'purple': '#6b3fa0', 'green': '#2f7d3a',
       'teal': '#0f766e', 'grey': '#6d6256', 'orange': '#c2410c'}

# the colour and the note heading of each kind of numbered event
KIND = {
    'lock': ('red', 'Lock'), 'portal': ('purple', 'Portal'), 'trap': ('red', 'Trap'), 'quest': ('amber', 'Quest'),
    'npc': ('teal', 'Encounter'), 'monster': ('red', 'Monsters'), 'spinner': ('purple', 'Spinner'),
    'text': ('grey', 'Writing'), 'switch': ('blue', 'Switch'), 'secret': ('amber', 'Secret'),
}
NUMBERED = ('lock', 'portal', 'trap', 'quest', 'npc', 'monster', 'spinner', 'text')

W, H = landscape(letter)
M = 22
CELL = 16.0
N = 32
MAPX = M + 12
MAPTOP = H - M - 40
MAPW = CELL * N
MAPY = MAPTOP - MAPW


def cx(col):
    return MAPX + col * CELL


def cy(row):
    return MAPTOP - row * CELL


def centre(cell):
    x, y = lv.xy(cell)
    return cx(x + 0.5), cy(y + 0.5)


def tag(color, text):
    return '<font color="%s"><b>%s</b></font>' % (HEX[color], text)


def esc(s):
    return s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')


_ALL = []


def all_level_events():
    """the events of all twelve levels (for the arrival cells)"""
    if not _ALL:
        for n in range(1, 13):
            _ALL.extend(analyze.events(lv.Level(n)))
    return _ALL


class Page:
    """the drawing state of one level: the cells' classes and the events"""

    def __init__(self, n, spec):
        self.L = L = lv.Level(n)
        self.spec = spec
        self.events = analyze.events(L)
        self.by_cell = defaultdict(list)
        for e in self.events:
            self.by_cell[e.cell].append(e)
        self.number_events()
        self.reach = self.reachable()

    def arrivals(self):
        """the cells the party arrives in from other levels (every LOAD LEVEL of the twelve files that
        names this level), and the new party's cell on level 1 (ResetMapScreen, 0ACD:1033)"""
        out = set(self.spec.get('start', ()))
        for e in all_level_events():
            for lvl, cell, _ in e.levels:
                if lvl == self.L.n and e.L.n != self.L.n:
                    out.add(cell)
        return out

    def reachable(self):
        """breadth first from the arrival cells. A step into a cell is allowed by that cell's side facing
        back (StepIfOpen, 0D1C:0080); every door is taken as openable, every wall a script opens or
        toggles as open, and teleporters as one more step"""
        L = self.L
        opened = {t for e in self.events for v, t in e.walls if v in ('open', 'toggle')}
        opened |= {e.cell for e in self.events if e.self_open}
        tele = defaultdict(list)
        for e in self.events:
            if e.kind == 'teleport':
                tele[e.cell].append(e.dest[1])
        seen = set(self.arrivals())
        todo = list(seen)
        while todo:
            a = todo.pop()
            nxt = list(tele.get(a, ()))
            for d, b in analyze.neighbours(a).items():
                k = L.cells[b][(d + 2) % 4]
                if L.flags(k) & 1 or self.is_door(b) or b in opened:
                    nxt.append(b)
            for b in nxt:
                if b not in seen:
                    seen.add(b)
                    todo.append(b)
        return seen

    def anchor(self, cell):
        """where a marker for a cell goes: the centre, or for a wall face the edge it faces"""
        X, Y = centre(cell)
        if not self.is_solid(cell):
            return X, Y
        for d in range(4):
            h = self.L.handler(self.L.cells[cell][d])
            nb = analyze.neighbours(cell)[d]
            if h in (2, 3, 4, 10) and not self.is_solid(nb):
                dx, dy = lv.STEP[d]
                return X + dx * CELL * 0.42, Y - dy * CELL * 0.42
        return X, Y

    # cell classes --------------------------------------------------------------------------
    def passable(self, cell, d):
        return bool(self.L.flags(self.L.cells[cell][d]) & 1)

    def is_door(self, cell):
        return any(self.L.flags(s) & 0x38 for s in self.L.cells[cell])

    def is_solid(self, cell):
        return not self.is_door(cell) and not any(self.passable(cell, d) for d in range(4))

    def is_stairs(self, cell):
        return any(e.kind in ('up', 'down') for e in self.by_cell.get(cell, ()))

    def number_events(self):
        """number the events worth a note; equal notes share a number"""
        self.numbers = {}
        self.notes = []
        seen = {}
        for e in self.events:
            if e.kind not in NUMBERED or e.cell == 0:   # cell 0 is the level timer's script
                continue
            key = (e.kind, e.title, ' '.join(e.body))
            if key not in seen:
                seen[key] = len(self.notes) + 1
                self.notes.append((seen[key], e, []))
            self.notes[seen[key] - 1][2].append(e.cell)
            self.numbers[e.cell] = seen[key]


def draw_map(c, P):
    L = P.L
    c.setLineWidth(0.25)
    for cell in range(N * N):
        x, y = lv.xy(cell)
        if P.is_solid(cell):
            c.setFillColor(ROCK)
            c.setStrokeColor(ROCKLINE)
        elif cell not in P.reach:
            c.setFillColor(HexColor('#cbc5ba'))
            c.setStrokeColor(HexColor('#bdb6aa'))
        else:
            c.setFillColor(FLOOR)
            c.setStrokeColor(GRID)
        c.rect(cx(x), cy(y + 1), CELL, CELL, fill=1, stroke=1)
    # illusionary faces: a solid-looking block the party can walk into from that side
    for cell in range(N * N):
        x, y = lv.xy(cell)
        sides = L.cells[cell]
        for d in range(4):
            k = sides[d]
            if P.passable(cell, d) and k in L.kinds and L.kinds[k]['vmp'] and not (L.flags(k) & 0x38) \
                    and not P.is_stairs(cell):
                c.setFillColor(HexColor('#d9c8ec'))
                c.rect(cx(x) + 1, cy(y + 1) + 1, CELL - 2, CELL - 2, fill=1, stroke=0)
                edge(c, x, y, d, PURPLE, 2.0, dash=(2, 1.5))
    # doors
    for cell in range(N * N):
        if not P.is_door(cell):
            continue
        x, y = lv.xy(cell)
        sides = L.cells[cell]
        ns = L.flags(sides[0]) & 0x38 or L.flags(sides[2]) & 0x38
        k = sides[0] if ns else sides[1]
        fl, hd = L.flags(k), L.handler(k)
        if fl & 0x10 and not fl & 0x20:
            col, kindw = AMBER, 'open'
        elif hd == 5:
            col, kindw = ORANGE, 'stuck'
        elif hd == 1:
            col, kindw = AMBER, 'button'
        else:
            col, kindw = RED, 'remote'
        X, Y = cx(x + 0.5), cy(y + 0.5)
        c.setStrokeColor(col)
        c.setLineWidth(3.0)
        c.setLineCap(0)
        if ns:   # passage runs north to south: the door is an east-west bar
            p0, p1 = (cx(x) + 1.5, Y), (cx(x + 1) - 1.5, Y)
        else:
            p0, p1 = (X, cy(y) - 1.5), (X, cy(y + 1) + 1.5)
        if kindw == 'open':
            c.setLineWidth(1.0)
            c.setDash(1.5, 1.2)
        c.line(p0[0], p0[1], p1[0], p1[1])
        c.setDash()
        if kindw == 'stuck':
            c.setStrokeColor(white)
            c.setLineWidth(0.9)
            if ns:
                c.line(X, Y - 2.5, X, Y + 2.5)
            else:
                c.line(X - 2.5, Y, X + 2.5, Y)
        if kindw == 'button':
            c.setFillColor(INK)
            c.circle(X, Y, 1.1, fill=1, stroke=0)

    # wall faces that do something
    for cell in range(N * N):
        if not P.is_solid(cell):
            continue
        x, y = lv.xy(cell)
        evs = P.by_cell.get(cell, [])
        ekinds = {e.kind for e in evs}
        for d in range(4):
            k = L.cells[cell][d]
            h = L.handler(k)
            nb = analyze.neighbours(cell)[d]
            if P.is_solid(nb):
                continue
            if h == 10:
                face(c, x, y, d, AMBER, 'niche')
            elif h in (3, 4):
                face(c, x, y, d, BLUE, 'lever')
            elif h == 2:
                if 'lock' in ekinds or 'portal' in ekinds:
                    face(c, x, y, d, RED if 'lock' in ekinds else PURPLE, 'lock')
                elif 'switch' in ekinds or 'teleport' in ekinds or 'quest' in ekinds or 'npc' in ekinds:
                    face(c, x, y, d, BLUE, 'button')
                elif evs:
                    face(c, x, y, d, GREY, 'writing')
            elif h == 5:
                pass

    # links: switches to what they move
    c.setLineWidth(0.45)
    for e in P.events:
        if e.kind not in ('switch', 'quest', 'lock', 'trap', 'secret', 'text', 'spinner', 'teleport', 'npc'):
            continue
        X0, Y0 = P.anchor(e.cell)
        if len(e.targets) > 6:      # a switch that moves many cells: the note lists them
            continue
        for t in e.targets:
            if t == e.cell:
                continue
            X1, Y1 = centre(t)
            c.setStrokeColor(HexColor('#3d78c2'))
            c.setDash(1.2, 1.4)
            c.line(X0, Y0, X1, Y1)
            c.setDash()

    # teleporters
    for e in P.events:
        if e.kind != 'teleport' or not e.dest:
            continue
        X0, Y0 = centre(e.cell)
        X1, Y1 = centre(e.dest[1])
        arrow(c, X0, Y0, X1, Y1, PURPLE)
        c.setFillColor(PURPLE)
        c.setStrokeColor(white)
        c.setLineWidth(0.8)
        c.circle(X0, Y0, 3.6, fill=1, stroke=1)

    # items and monsters at the start
    icells = Counter(it['key'] for it in L.items)
    for cell, n in icells.items():
        X, Y = centre(cell)
        c.setFillColor(AMBER)
        c.setStrokeColor(white)
        c.setLineWidth(0.6)
        c.rect(X + 2.2, Y + 2.2, 4.2, 4.2, fill=1, stroke=1)
    mcells = Counter(m['cell'] for m in L.monsters)
    for cell, n in mcells.items():
        X, Y = centre(cell)
        c.setFillColor(RED)
        c.setStrokeColor(white)
        c.setLineWidth(0.6)
        c.circle(X - 4.2, Y - 4.2, 2.9, fill=1, stroke=1)
        if n > 1:
            c.setFillColor(white)
            c.setFont(FONTB, 4.2)
            c.drawCentredString(X - 4.2, Y - 5.7, str(n))

    # stairs, holes, portals
    for e in P.events:
        X, Y = centre(e.cell)
        if e.kind in ('up', 'down'):
            stairs(c, X, Y, e.kind == 'up', e.dest[0])
        elif e.kind == 'pit':
            c.setFillColor(INK)
            c.setStrokeColor(white)
            c.setLineWidth(0.6)
            c.circle(X, Y, 4.0, fill=1, stroke=1)
    # experience stars
    for e in P.events:
        if e.xp and e.kind not in ('up', 'down', 'pit'):
            X, Y = centre(e.cell)
            star(c, X + 4.3, Y - 4.3, 2.4, AMBER)

    # numbered markers
    for num, e, cells in P.notes:
        col = HexColor(HEX[KIND[e.kind][0]])
        for cell in cells:
            X, Y = P.anchor(cell)
            c.setFillColor(col)
            c.setStrokeColor(white)
            c.setLineWidth(0.8)
            c.circle(X, Y, 5.0, fill=1, stroke=1)
            c.setFillColor(white)
            c.setFont(FONTB, 5.6 if num < 10 else 4.8)
            c.drawCentredString(X, Y - 1.9, str(num))

    # the party's start
    for cell in P.spec.get('start', ()):
        X, Y = centre(cell)
        c.setFillColor(GREEN)
        c.setStrokeColor(white)
        c.setLineWidth(1.0)
        c.rect(X - 5, Y - 5, 10, 10, fill=1, stroke=1)
        c.setFillColor(white)
        c.setFont(FONTB, 6)
        c.drawCentredString(X, Y - 2.1, 'S')

    # axes
    c.setFillColor(MUTED)
    c.setFont(FONT, 5.4)
    for k in range(N):
        c.drawCentredString(cx(k + 0.5), MAPTOP + 2.5, str(k))
        c.drawCentredString(cx(k + 0.5), MAPY - 7, str(k))
        c.drawRightString(MAPX - 2.5, cy(k + 0.5) - 1.9, str(k))
        c.drawString(MAPX + MAPW + 2.5, cy(k + 0.5) - 1.9, str(k))
    c.setStrokeColor(INK)
    c.setLineWidth(0.8)
    c.rect(MAPX, MAPY, MAPW, MAPW, fill=0, stroke=1)


def edge(c, x, y, d, col, lw, dash=None):
    seg = {0: (cx(x), cy(y), cx(x + 1), cy(y)), 2: (cx(x), cy(y + 1), cx(x + 1), cy(y + 1)),
           3: (cx(x), cy(y), cx(x), cy(y + 1)), 1: (cx(x + 1), cy(y), cx(x + 1), cy(y + 1))}[d]
    c.setStrokeColor(col)
    c.setLineWidth(lw)
    if dash:
        c.setDash(*dash)
    c.line(*seg)
    c.setDash()


def face(c, x, y, d, col, what):
    """a mark inside a solid block, on its face toward the open cell in direction d"""
    t = 3.4   # depth into the block
    L0, L1 = 4.0, CELL - 4.0
    if d == 0:
        r = (cx(x) + L0, cy(y) - t, L1 - L0, t)
    elif d == 2:
        r = (cx(x) + L0, cy(y + 1), L1 - L0, t)
    elif d == 3:
        r = (cx(x), cy(y + 1) + L0, t, L1 - L0)
    else:
        r = (cx(x + 1) - t, cy(y + 1) + L0, t, L1 - L0)
    c.setFillColor(col)
    c.setStrokeColor(white)
    c.setLineWidth(0.5)
    if what == 'niche':
        c.rect(*r, fill=1, stroke=1)
    elif what == 'writing':
        c.setFillColor(HexColor('#cfc6b4'))
        c.rect(*r, fill=1, stroke=0)
    else:
        c.roundRect(*r, 1.2, fill=1, stroke=1)
        if what == 'lever':
            mx, my = r[0] + r[2] / 2, r[1] + r[3] / 2
            c.setFillColor(white)
            c.circle(mx, my, 0.9, fill=1, stroke=0)


def arrow(c, x0, y0, x1, y1, col):
    import math
    dx, dy = x1 - x0, y1 - y0
    L = math.hypot(dx, dy) or 1
    ux, uy = dx / L, dy / L
    ex, ey = x1 - ux * 3.5, y1 - uy * 3.5
    c.setStrokeColor(col)
    c.setLineWidth(0.8)
    c.setDash(2.2, 1.4)
    c.line(x0, y0, ex, ey)
    c.setDash()
    p = c.beginPath()
    px, py = -uy * 2.4, ux * 2.4
    p.moveTo(x1, y1)
    p.lineTo(ex + px, ey + py)
    p.lineTo(ex - px, ey - py)
    p.close()
    c.setFillColor(col)
    c.drawPath(p, fill=1, stroke=0)


def stairs(c, X, Y, up, lvl):
    c.setFillColor(GREEN if up else HexColor('#0f5132'))
    c.setStrokeColor(white)
    c.setLineWidth(0.7)
    p = c.beginPath()
    if up:
        p.moveTo(X, Y + 6)
        p.lineTo(X + 6, Y - 4.5)
        p.lineTo(X - 6, Y - 4.5)
    else:
        p.moveTo(X, Y - 6)
        p.lineTo(X + 6, Y + 4.5)
        p.lineTo(X - 6, Y + 4.5)
    p.close()
    c.drawPath(p, fill=1, stroke=1)
    c.setFillColor(white)
    c.setFont(FONTB, 5.2)
    c.drawCentredString(X, Y - (3.6 if up else 0.2), str(lvl))


def star(c, X, Y, r, col):
    import math
    p = c.beginPath()
    for i in range(10):
        a = math.pi / 2 + i * math.pi / 5
        rr = r if i % 2 == 0 else r * 0.45
        (p.moveTo if i == 0 else p.lineTo)(X + rr * math.cos(a), Y + rr * math.sin(a))
    p.close()
    c.setFillColor(col)
    c.setStrokeColor(white)
    c.setLineWidth(0.4)
    c.drawPath(p, fill=1, stroke=1)


def header(c, P, page):
    spec = P.spec
    c.setFillColor(INK)
    title = 'LEVEL %d' % P.L.n
    c.setFont(FONTB, 22)
    c.drawString(M, H - M - 19, title)
    tw = stringWidth(title, FONTB, 22)
    c.setFont(FONT, 8.6)
    c.setFillColor(MUTED)
    c.drawString(M + tw + 12, H - M - 8, spec['subtitle'])
    c.drawString(M + tw + 12, H - M - 19, 'Eye of the Beholder (Westwood, DOS)  |  player map%s  |  SPOILERS' % (
        '' if page == 1 else ', notes'))
    c.setFont(FONTI, 6.3)
    c.drawRightString(W - M, H - M - 8, 'Read from the game\'s own level files: walls, doors, scripts, items, monsters.')
    c.drawRightString(W - M, H - M - 16, 'Cells are (x,y) from 0, top left, north up, as the game numbers them.')
    c.setStrokeColor(INK)
    c.setLineWidth(1.1)
    c.line(M, H - M - 26, W - M, H - M - 26)


def para(c, html, x, ytop, w, style):
    p = Paragraph(html, style)
    _, h = p.wrap(w, 2000)
    p.drawOn(c, x, ytop - h)
    return h


def legend(c, x, y, w, P):
    """the key, drawn top-down from y; returns the y below it"""
    items = [
        ('rock', 'solid rock'), ('floor', 'floor'), ('closed', 'floor no way leads to'),
        ('illusion', 'wall you can walk through'),
        ('door_button', 'door with a button'), ('door_remote', 'door opened from elsewhere (lever, key, plate)'),
        ('door_stuck', 'stuck door: force it'), ('door_open', 'open door'),
        ('button', 'button or switch on the wall'), ('lever', 'lever'), ('lock', 'keyhole (red) or portal (purple)'),
        ('niche', 'niche in the wall'), ('writing', 'writing or carving'),
        ('up', 'stairs up (to level)'), ('down', 'stairs down'), ('pit', 'hole in the floor: you fall'),
        ('tele', 'teleporter and where it sends you'), ('link', 'what a switch or plate moves'),
        ('item', 'items lying here'), ('monster', 'monsters at the start (number)'), ('star', 'experience, first time'),
        ('num', 'see the note with this number'),
    ]
    if P.spec.get('start'):
        items.append(('start', 'where a new party starts'))
    c.setFillColor(INK)
    c.setFont(FONTB, 7)
    c.drawString(x, y - 7, 'KEY')
    y -= 12
    col_w = w / 2
    rows = (len(items) + 1) // 2
    for i, (k, text) in enumerate(items):
        col, row = divmod(i, rows)
        X = x + col * col_w
        Y = y - row * 10.2
        sym(c, k, X + 5, Y - 3)
        c.setFillColor(INK)
        c.setFont(FONT, 5.9)
        lines = wrap_words(text, col_w - 14, 5.9)
        c.drawString(X + 12, Y - 5, lines[0])
    return y - rows * 10.2


def wrap_words(text, width, size):
    words, lines, cur = text.split(), [], ''
    for w_ in words:
        t = (cur + ' ' + w_).strip()
        if stringWidth(t, FONT, size) > width and cur:
            lines.append(cur)
            cur = w_
        else:
            cur = t
    lines.append(cur)
    return lines


def sym(c, k, X, Y):
    s = 4.5
    if k == 'rock':
        c.setFillColor(ROCK); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=0)
    elif k == 'floor':
        c.setFillColor(FLOOR); c.setStrokeColor(BOXLINE); c.setLineWidth(0.4); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=1)
    elif k == 'closed':
        c.setFillColor(HexColor('#cbc5ba')); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=0)
    elif k == 'illusion':
        c.setFillColor(HexColor('#d9c8ec')); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=0)
        c.setStrokeColor(PURPLE); c.setLineWidth(1.6); c.setDash(2, 1.5); c.line(X - s, Y + s, X + s, Y + s); c.setDash()
    elif k.startswith('door_'):
        c.setFillColor(FLOOR); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=0)
        col = {'door_button': AMBER, 'door_remote': RED, 'door_stuck': ORANGE, 'door_open': AMBER}[k]
        c.setStrokeColor(col)
        c.setLineWidth(1.0 if k == 'door_open' else 3.0)
        if k == 'door_open':
            c.setDash(1.5, 1.2)
        c.line(X - s + 1, Y, X + s - 1, Y)
        c.setDash()
        if k == 'door_stuck':
            c.setStrokeColor(white); c.setLineWidth(0.9); c.line(X, Y - 2.5, X, Y + 2.5)
        if k == 'door_button':
            c.setFillColor(INK); c.circle(X, Y, 1.1, fill=1, stroke=0)
    elif k in ('button', 'lever', 'lock', 'niche', 'writing'):
        c.setFillColor(ROCK); c.rect(X - s, Y - s, 2 * s, 2 * s, fill=1, stroke=0)
        col = {'button': BLUE, 'lever': BLUE, 'lock': RED, 'niche': AMBER, 'writing': HexColor('#cfc6b4')}[k]
        c.setFillColor(col); c.setStrokeColor(white); c.setLineWidth(0.5)
        c.roundRect(X - s + 1.5, Y - s, 2 * s - 3, 3.4, 1, fill=1, stroke=0 if k == 'writing' else 1)
        if k == 'lever':
            c.setFillColor(white); c.circle(X, Y - s + 1.7, 0.9, fill=1, stroke=0)
    elif k in ('up', 'down'):
        stairs(c, X, Y, k == 'up', 'n')
    elif k == 'pit':
        c.setFillColor(INK); c.circle(X, Y, 4, fill=1, stroke=0)
    elif k == 'tele':
        arrow(c, X - 5, Y, X + 6, Y, PURPLE)
        c.setFillColor(PURPLE); c.setStrokeColor(white); c.circle(X - 5, Y, 3, fill=1, stroke=1)
    elif k == 'link':
        c.setStrokeColor(HexColor('#3d78c2')); c.setLineWidth(0.6); c.setDash(1.2, 1.4); c.line(X - 5, Y, X + 5, Y); c.setDash()
    elif k == 'item':
        c.setFillColor(AMBER); c.rect(X - 2.2, Y - 2.2, 4.4, 4.4, fill=1, stroke=0)
    elif k == 'monster':
        c.setFillColor(RED); c.circle(X, Y, 3, fill=1, stroke=0)
        c.setFillColor(white); c.setFont(FONTB, 4.2); c.drawCentredString(X, Y - 1.5, '2')
    elif k == 'star':
        star(c, X, Y, 3, AMBER)
    elif k == 'num':
        c.setFillColor(GREY); c.circle(X, Y, 4.5, fill=1, stroke=0)
        c.setFillColor(white); c.setFont(FONTB, 5.4); c.drawCentredString(X, Y - 1.9, '7')
    elif k == 'start':
        c.setFillColor(GREEN); c.rect(X - 4.5, Y - 4.5, 9, 9, fill=1, stroke=0)
        c.setFillColor(white); c.setFont(FONTB, 5.5); c.drawCentredString(X, Y - 2, 'S')


def exits_lines(P):
    """the ways in and out: stairs, holes, portals, teleporters"""
    out = []
    groups = defaultdict(list)
    for e in P.events:
        if e.kind in ('up', 'down', 'portal'):
            out.append((e.kind, e))
        elif e.kind == 'pit':
            groups[e.dest[0]].append(e)
    lines = []
    for k, e in out:
        word = {'up': 'Stairs up', 'down': 'Stairs down', 'portal': 'Portal'}[k]
        lines.append('<b>%s</b> %s to level %d %s%s' % (
            word, lv.lab(e.cell), e.dest[0], lv.lab(e.dest[1]),
            ('  (%s)' % esc(' '.join(e.body))) if k == 'portal' else ''))
    for lvl, es in sorted(groups.items()):
        dmg = ' '.join(es[0].body)
        if len(es) > 4:
            lines.append('<b>%d holes</b> (black dots): fall to level %d, same place%s' % (len(es), lvl, ' (%s)' % esc(dmg) if dmg else ''))
        else:
            lines.append('<b>%s</b> %s: fall to level %d, same place%s' % ('Holes' if len(es) > 1 else 'Hole',
                         ', '.join(lv.lab(e.cell) for e in es), lvl, ' (%s)' % esc(dmg) if dmg else ''))
    return lines


def note_blocks(P, spec):
    """the notes, as a list of ('head', text), ('note', (num, colour, html)) and ('line', html)"""
    L = P.L
    blocks = []

    def head(t):
        blocks.append(('head', t))

    if P.notes:
        head('NOTES (NUMBERS ON THE MAP)')
    for num, e, cells in P.notes:
        color = KIND[e.kind][0]
        where = ', '.join(lv.lab(x) for x in cells)
        body = esc(' '.join(e.body))
        blocks.append(('note', (num, color, '<b>%s</b>  %s%s' % (esc(e.title), where, '<br/>' + body if body else ''))))
    sw = [e for e in P.events if e.kind == 'switch']
    if sw:
        head('SWITCHES AND PLATES (DOTTED LINES)')
        for e in sw:
            blocks.append(('line', '<b>%s</b> %s: %s' % (esc(e.title), lv.lab(e.cell), esc(' '.join(e.body)))))
    xpcells = [e for e in P.events if e.xp and e.kind not in ('up', 'down', 'pit')]
    if xpcells:
        head('EXPERIENCE (STARS)')
        blocks.append(('line', 'Shared by the party; once unless marked: ' + '; '.join(
            '%s +%s%s' % (lv.lab(e.cell), '/'.join('{:,}'.format(x) for x in e.xp), '' if e.xp_once else ' (every time)')
            for e in xpcells) + '.'))
    head('ITEMS AT THE START (SQUARES)')
    bycell = defaultdict(list)
    for it in L.items:
        bycell[it['key']].append(it)
    for cell in sorted(bycell, key=lambda k: (lv.xy(k)[1], lv.xy(k)[0])):
        its = bycell[cell]
        names = Counter(lv.item_name(i['index']) for i in its)
        where = ' in a niche' if all(i['pos'] == 8 for i in its) else ''
        blocks.append(('line', '<b>%s</b>%s: %s' % (lv.lab(cell), where, esc(', '.join(
            ('%d %s' % (k, nm) if k > 1 else nm) for nm, k in names.items())))))
    head('MONSTERS AT THE START (RED DOTS)')
    mc = Counter(lv.MONSTER_NAMES[m['type']] for m in L.monsters)
    blocks.append(('line', ', '.join('%d %s' % (k, analyze.plural(m) if k > 1 else m) for m, k in mc.most_common()) + '.'))
    # KillMonster (08E6): a copy of the first drop item half the time, of the second always
    half = Counter()
    for m in L.monsters:
        if m['drop1']:
            half[(lv.MONSTER_NAMES[m['type']], lv.item_name(m['drop1']))] += 1
    for (mon, item), k in half.most_common():
        blocks.append(('line', '%s: each drops a %s half the time.' % (
            ('%d %s' % (k, analyze.plural(mon))) if k > 1 else 'One ' + mon, esc(item))))
    for m in L.monsters:
        if m['drop2']:
            blocks.append(('line', 'The %s at %s carries %s.' % (
                lv.MONSTER_NAMES[m['type']], lv.lab(m['cell']), esc(lv.item_name(m['drop2'])))))
    timer = [e for e in P.events if e.cell == 0 and e.kind == 'monster']
    if timer:
        blocks.append(('line', 'Every %d steps the level may add %s (the level timer).' % (
            L.timer_steps, timer[0].title.split(': ', 1)[1])))
    for t in spec.get('extra', ()):
        head(t[0])
        blocks.append(('line', t[1]))
    return blocks


def flow(c, blocks, frames, fs, on_new_page):
    """draw the blocks down the frames (x, top, width, bottom) in order; on_new_page(i) is called
    before the first block lands in frame i when that frame starts a new page"""
    st = ParagraphStyle('n', fontName=FONT, fontSize=fs, leading=fs * 1.2, textColor=INK)
    fi = 0
    x, y, w, bottom = frames[0]
    started = {0}

    def advance():
        nonlocal fi, x, y, w, bottom
        fi += 1
        if fi >= len(frames):
            raise OverflowError('notes do not fit')
        x, y, w, bottom = frames[fi]
        if fi not in started:
            started.add(fi)
            on_new_page(fi)

    for i, (kind, data) in enumerate(blocks):
        if kind == 'head':
            nxt = blocks[i + 1] if i + 1 < len(blocks) else None
            need = 14 + (measure(nxt, w, st) if nxt else 0)
            if y - need < bottom:
                advance()
            c.setFillColor(INK)
            c.setFont(FONTB, 7)
            c.drawString(x, y - 8, data)
            c.setStrokeColor(BOXLINE)
            c.setLineWidth(0.5)
            c.line(x, y - 10.5, x + w, y - 10.5)
            y -= 14
            continue
        h = measure((kind, data), w, st)
        if y - h < bottom:
            advance()
        if kind == 'note':
            num, color, html = data
            p = Paragraph(html, st)
            p.wrap(w - 13, 2000)
            c.setFillColor(HexColor(HEX[color]))
            c.circle(x + 4.6, y - 4.4, 4.6, fill=1, stroke=0)
            c.setFillColor(white)
            c.setFont(FONTB, 5.6 if num < 10 else 4.8)
            c.drawCentredString(x + 4.6, y - 6.4, str(num))
            p.drawOn(c, x + 13, y - h + 2.6)
        else:
            p = Paragraph(data, st)
            p.wrap(w, 2000)
            p.drawOn(c, x, y - h + 1.6)
        y -= h
    return fi


def measure(block, w, st):
    kind, data = block
    if kind == 'head':
        return 14
    if kind == 'note':
        p = Paragraph(data[2], st)
        return p.wrap(w - 13, 2000)[1] + 2.6
    p = Paragraph(data, st)
    return p.wrap(w, 2000)[1] + 1.6


def render(n, spec, c, fs=6.4):
    P = Page(n, spec)
    # ----- page 1: the plan, the key, the ways out, then the notes as far as they go
    header(c, P, 1)
    draw_map(c, P)
    RX = MAPX + MAPW + 24
    RW = W - M - RX
    base = ParagraphStyle('b', fontName=FONT, fontSize=fs, leading=fs * 1.22, textColor=INK)
    y = MAPTOP + 10
    gh = Paragraph(spec['glance'], base)
    _, h = gh.wrap(RW - 10, 1000)
    c.setFillColor(BOX)
    c.setStrokeColor(BOXLINE)
    c.setLineWidth(0.6)
    c.roundRect(RX, y - h - 8, RW, h + 9, 3, fill=1, stroke=1)
    gh.drawOn(c, RX + 5, y - h - 3.5)
    y -= h + 16
    y = legend(c, RX, y, RW, P) - 6
    c.setFillColor(INK)
    c.setFont(FONTB, 7)
    c.drawString(RX, y - 8, 'WAYS IN AND OUT')
    c.setStrokeColor(BOXLINE)
    c.setLineWidth(0.5)
    c.line(RX, y - 10.5, RX + RW, y - 10.5)
    y -= 14
    small = ParagraphStyle('s', parent=base, fontSize=fs, leading=fs * 1.2)
    for ln in exits_lines(P):
        y -= para(c, ln, RX, y, RW, small) + 1.6
    tele = [e for e in P.events if e.kind == 'teleport']
    if tele:
        ln = '<b>Teleporters</b> ' + '; '.join('%s to %s' % (lv.lab(e.cell), lv.lab(e.dest[1])) for e in tele) + '.'
        y -= para(c, ln, RX, y, RW, small) + 1.6
    y -= 4
    # page 2 has three columns
    cols, gap = 3, 14
    colw = (W - 2 * M - (cols - 1) * gap) / cols
    top2 = H - M - 34
    frames = [(RX, y, RW, M)] + [(M + i * (colw + gap), top2, colw, M) for i in range(cols)]
    blocks = note_blocks(P, spec)

    def new_page(i):
        if i == 1:   # frames 1 to 3 are the columns of page 2
            c.showPage()
            header(c, P, 2)

    flow(c, blocks, frames, fs, new_page)
    c.showPage()


def build(n, spec, out):
    from reportlab.pdfgen import canvas
    last = None
    for fs in (6.4, 6.2, 6.0, 5.8, 5.6, 5.4, 5.2, 5.0):
        try:
            c = canvas.Canvas(out, pagesize=(W, H))
            c.setTitle('Level %d - player map (Eye of the Beholder)' % n)
            c.setAuthor('eob-decomp')
            c.setSubject('Eye of the Beholder (DOS) level %d map' % n)
            render(n, spec, c, fs)
            c.save()
            return fs
        except OverflowError as e:
            last = e
    raise RuntimeError('level %d: %s' % (n, last))
