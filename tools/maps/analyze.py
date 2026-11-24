#!/usr/bin/env python3
"""analyze.py: what each cell script of a level does, in a player's words.

Each trigger's script is walked through every branch (level.Level.walk) and its instructions are
collected into an Event: how it is set off (the trigger mask, whitepaper 5.5.1), what it changes
(walls, doors, the party's place, items, monsters, flags) and what it says. The event's `kind`
picks the marker on the map; `title` and `body` are the note. Nothing here is guessed from the
look of the game: every statement is the instruction it came from.

  python3 tools/maps/analyze.py 3        every event of level 3 as text
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import level as lv  # noqa: E402

# The five events a cell can listen to (bits 3..7 of the mask enable modes 1, 2, 4, 8, 0x10);
# click, the activate key and the timer are always on (whitepaper 5.5.1).
MASK_WORDS = [(0x08, 'step'), (0x10, 'leave'), (0x20, 'drop'), (0x40, 'take'), (0x80, 'throw')]

# Messages that every lock and every stair prints; the notes say what they mean instead.
STOCK_TEXT = {"it doesn't fit.", 'failed lock pick attempt.', 'the lock picks break!.', 'only a thief can pick locks.',
              'this lock requires a key.', 'the lock appears jammed.', 'the lock has been picked!',
              "you can't go that way.", 'going up...', 'going down...', 'going up.', 'going down.', 'the key fits.',
              'the dagger fits.', 'the rock fits!', 'key taken.'}

RACES = ['human', 'elf', 'half-elf', 'dwarf', 'gnome', 'halfling']          # gRaceNames (src/D07C8.C), race >> 1
CLASS_BITS = {1: 'fighter', 2: 'mage', 4: 'cleric', 8: 'thief'}           # gClassBaseBits (whitepaper 5.1.3)
# CharOverlay panels (whitepaper 5.5.3); 8 is a picture shown while a portal takes the party
PANELS = {0: 'a temple scene', 1: 'an item scene', 2: 'the resurrection offer', 3: 'an encounter (attack or talk)',
          4: "the 'kill her' choice", 5: 'the surrender choice', 6: 'the quit choice', 7: "the 'free him' choice",
          8: 'the portal picture', 9: 'the ending', 10: 'the copy protection question (a word from the manual)'}
SPELL_OF_MODE = {0: 'magic missile', 1: 'flame arrow?', 2: 'fireball', 3: 'flame arrow', 4: 'hold person', 5: 'lightning bolt',
                 6: 'ice storm', 7: 'hold monster', 8: 'cause light wounds', 9: 'flame strike', 10: 'spell 51',
                 11: 'spell 52'}
DIRWORD = {0: 'north', 1: 'east', 2: 'south', 3: 'west'}


def _spell_modes():
    """gEffectModes (src/E0EF3.C): the spell each LAUNCH mode casts, by the spell table's names"""
    import re
    src = open(os.path.join(lv.ROOT, 'src', 'E0EF3.C'), encoding='latin-1').read()
    m = re.search(r'gEffectModes\[\d+\]\s*=\s*\{[^\n]*\n(.*?)\};', src, re.S)
    first = [int(t) for t in re.findall(r'\{\s*(-?\d+)\s*,', m.group(1))]
    d07 = open(os.path.join(lv.ROOT, 'src', 'D07C8.C'), encoding='latin-1').read()
    mage = lv._c_strings(d07, 'gMageSpellNames')
    cleric = lv._c_strings(d07, 'gClericSpellNames')
    out = {}
    for mode, sp in enumerate(first):
        if 0 < sp < len(mage):
            out[mode] = mage[sp].lower()
        elif 26 <= sp < 26 + len(cleric) - 1:
            out[mode] = cleric[sp - 25].lower()
        else:
            out[mode] = 'a magical attack'
    return out


SPELL_OF_MODE = _spell_modes()


def neighbours(cell):
    x, y = lv.xy(cell)
    out = {}
    for d, (dx, dy) in lv.STEP.items():
        out[d] = ((y + dy) % 32) * 32 + ((x + dx) % 32)
    return out


def key_names(t, v=None, limit=2):
    """the items a lock that wants type t (and value v) accepts: their names in ITEM.DAT"""
    recs = items_all()
    names = []
    for r in recs:
        if r['type'] == t and (v is None or r['value'] == v) and r['name'] not in ('NULL', ''):
            if r['shown'] not in names:
                names.append(r['shown'])
    if not names:
        return ''
    if v is None and len(names) > 1:     # the lock tests the type alone: any item of the kind does
        from collections import Counter
        base = Counter(r['name'] for r in recs if r['type'] == t and r['name'] not in ('NULL', '')).most_common(1)[0][0]
        return 'any ' + base.lower()
    if len(names) > limit:
        return ', '.join(names[:limit]) + ' or another'
    return ' or '.join(names)


def items_all():
    return lv.items()['records']


def type_word(t):
    """an item type in words: the names of the items of that type"""
    if t == 255:
        return 'any item'
    return key_names(t)


class Event:
    def __init__(self, L, t):
        self.L, self.t = L, t
        self.cell = t['cell']
        self.mask = t['mask']
        self.kind = 'other'
        self.title = ''
        self.body = []
        self.targets = []         # cells the event changes (for link lines)
        self.dest = None          # (level, cell) for a level change or a teleport
        self.solid = not any(L.flags(s) & 1 for s in L.cells[self.cell])
        self.analyse()

    @property
    def x(self):
        return lv.xy(self.cell)[0]

    @property
    def y(self):
        return lv.xy(self.cell)[1]

    # how the player sets it off -----------------------------------------------------------
    def how(self):
        words = [w for b, w in MASK_WORDS if self.mask & b]
        if self.cell == 0:
            return 'timer'
        if not words:
            return 'click'
        return '/'.join(words)

    def analyse(self):
        L = self.L
        ins = L.walk(self.t['off'])
        self.ins = ins
        A = lambda name: [i for i in ins if i.name == name]  # noqa: E731
        texts = []
        for i in A('TEXT'):
            if i.a['text'] not in texts:
                texts.append(i.a['text'])
        self.texts = texts
        self.special = [t for t in texts if t not in STOCK_TEXT]
        nb = set(neighbours(self.cell).values()) | {self.cell}
        levels = [(i.a['level'], i.a['cell'], i.a['dir']) for i in A('LOAD LEVEL')]
        party = [i.a['dst'] for i in A('MOVE') if i.a['sub'] == 'party']
        tele = [d for d in party if d not in nb]
        xp = sorted({i.a['xp'] for i in A('GIVE XP')})
        new_items = [(i.a['item'], i.a['cell']) for i in A('NEW ITEM')]
        spawns = [i.a for i in A('SPAWN')]
        dmg = [i.a for i in A('DAMAGE')]
        launch = [i.a for i in A('LAUNCH')]
        panels = [i.a['panel'] for i in A('OVERLAY')]
        steal = A('STEAL ITEM')
        turns = [i.a['n'] for i in A('TURN') if i.a['sub'] == 'party']
        doors = []
        for i in ins:
            if i.name in ('OPEN DOOR', 'CLOSE DOOR') or (i.name == 'TOGGLE WALL' and i.a['sub'] == 'door'):
                verb = {'OPEN DOOR': 'opens', 'CLOSE DOOR': 'closes'}.get(i.name, 'opens or closes')
                doors.append((verb, i.a['cell']))
        walls = []
        self_open = False
        for i in ins:
            if i.name == 'SET WALL' and i.a['sub'] == 'all' and i.a['cell'] == self.cell:
                self_open = self_open or bool(L.flags(i.a['kind']) & 1) and not self.passable_now()
            elif i.name == 'SET WALL' and i.a['sub'] == 'all':
                walls.append(('open' if L.flags(i.a['kind']) & 1 else 'wall', i.a['cell']))
            elif i.name == 'TOGGLE WALL' and i.a['sub'] == 'all' and i.a['cell'] != self.cell:
                walls.append(('toggle', i.a['cell']))
        self.self_open = self_open
        moved_monsters = [i.a for i in A('MOVE') if i.a['sub'] == 'monsters']
        held = []
        picks = False
        put_types = []
        for i in A('IF'):
            e = i.a['expr']
            for k, tok in enumerate(e):
                if tok == ('held', 'type') and k + 1 < len(e) and e[k + 1][0] == 'lit':
                    t = e[k + 1][1]
                    v = None
                    for k2 in range(k + 2, len(e) - 1):
                        if e[k2] == ('held', 'value') and e[k2 + 1][0] == 'lit':
                            v = e[k2 + 1][1]
                    if t == 28:
                        picks = True
                    elif (t, v) not in held:
                        held.append((t, v))
                if tok[0] == 'items' and tok[1] != 255 and tok[1] not in put_types:
                    put_types.append(tok[1])
                if tok[0] == 'itemin' and ('id', tok[1]) not in put_types:
                    put_types.append(('id', tok[1]))
        if any(v is not None for _, v in held):   # the "it doesn't fit" arm tests the type alone
            held = [(t, v) for t, v in held if v is not None]
        races = sorted({tok[1] for i in A('IF') for tok in i.a['expr'] if tok[0] == 'race'})
        classes = sorted({tok[1] for i in A('IF') for tok in i.a['expr'] if tok[0] == 'class' and tok[1] != 8})
        gflags = sorted({i.a['bit'] for i in A('SET FLAG') if i.a['sub'] == 'game'})
        self.doors, self.walls, self.xp, self.levels = doors, walls, xp, levels
        self.xp_once = True
        self.targets = sorted({c for _, c in doors} | {c for _, c in walls})

        # pick the kind, the most telling thing the script does
        b = self.body
        where = self.how()
        if levels:
            lvl, cell, d = levels[0]
            self.dest = (lvl, cell)
            if d == 255:
                self.kind = 'pit'
                self.title = 'Hole in the floor: falls to level %d %s' % (lvl, lv.lab(cell))
            elif held and abs(lvl - L.n) != 1 or (held and panels):
                self.kind = 'portal'
                self.title = 'Portal to level %d %s' % (lvl, lv.lab(cell))
                b.append('Click it holding %s.' % self.held_words(held))
            elif lvl < L.n:
                self.kind = 'up'
                self.title = 'Stairs up to level %d %s' % (lvl, lv.lab(cell))
            else:
                self.kind = 'down'
                self.title = 'Stairs down to level %d %s' % (lvl, lv.lab(cell))
            if len(levels) > 1:
                more = ['level %d %s' % (a, lv.lab(c)) for a, c, _ in levels[1:]]
                b.append('Also leads to ' + ', '.join(more) + ' (by what is held or the flags).')
            if 10 in panels:
                b.append('The first time down, the game asks a word from the manual.')
        elif tele:
            self.kind = 'teleport'
            self.dest = (L.n, tele[0])
            self.title = 'Teleporter to %s' % ', '.join(lv.lab(c) for c in dict.fromkeys(tele))
            if where != 'step':
                b.append('Set off by: %s.' % where)
        elif held and (doors or walls or new_items):
            self.kind = 'lock'
            self.title = 'Lock: %s' % self.held_words(held)
            if picks and 'the lock has been picked!' in texts:
                b.append('A thief with lock picks can open it.')
            if any(t in ('key taken.',) for t in texts):
                b.append('The key is taken back.')
        elif dmg or launch or (spawns and where in ('step', 'drop', 'take', 'throw')):
            self.kind = 'trap'
            parts = []
            for a in launch:
                parts.append(SPELL_OF_MODE.get(a['id'], 'a spell') if a['spell'] else 'a %s' % lv.item_name(a['id']).lower())
            if launch:
                srcs = sorted({lv.lab(a['cell']) for a in launch})
                self.title = 'Trap: %s from %s' % (' and '.join(dict.fromkeys(parts)), ', '.join(srcs))
            elif dmg:
                d = dmg[0]
                self.title = 'Trap: %dd%d damage%s' % (d['count'], d['sides'], ' to everyone' if d['who'] == 255 else '')
            else:
                self.title = 'Ambush: ' + self.spawn_words(spawns)
        elif spawns:
            self.kind = 'monster'
            self.title = 'Monsters appear: ' + self.spawn_words(spawns)
        elif any(p not in (8, 10) for p in panels):
            self.kind = 'npc'
            self.title = 'Encounter: ' + ', '.join(PANELS.get(p, 'panel %d' % p) for p in dict.fromkeys(panels) if p not in (8, 10))
        elif steal:
            self.kind = 'trap'
            self.title = 'Thief: an item from your packs lands at %s' % lv.lab(steal[0].a['cell'])
        elif turns:
            self.kind = 'spinner'
            self.title = 'Spinner: turns the party %s' % {1: 'right', 2: 'around', 3: 'left'}.get(turns[0] % 4, '')
        elif new_items or gflags or any('quest' in t for t in texts):
            self.kind = 'quest'
            self.title = 'Quest' if any('quest' in t for t in texts) else 'Reward'
        elif self_open and not doors and not walls:
            self.kind = 'switch'
            self.title = 'Wall that opens when %s' % {'throw': 'a spell or a thrown item hits it', 'step': 'you step on it'}.get(
                where.split('/')[-1], 'set off (%s)' % where)
        elif doors or walls:
            self.kind = 'switch'
            self.title = {'click': 'Button or lever', 'step': 'Pressure plate', 'drop': 'Place an item',
                          'take': 'Take the item'}.get(where.split('/')[0], 'Switch')
            if where == 'drop/take' or (put_types and where != 'click'):
                self.title = 'Item plate'
        elif self.special:
            self.kind = 'text'
            self.title = 'Writing' if self.solid else 'Message'
        elif xp:
            self.kind = 'secret'
            self.title = 'Secret'
        else:
            self.kind = 'other'

        # the body: what else it does
        if self.special and self.kind != 'text':
            b.append(' '.join('"%s"' % t for t in self.special))
        elif self.kind == 'text':
            b.append(' '.join('"%s"' % t for t in self.special))
        if put_types and self.kind not in ('lock', 'portal'):
            want = [lv.item_name(p[1]) if isinstance(p, tuple) else type_word(p) for p in put_types]
            want = [w_ for w_ in want if w_]
            if want:
                b.append('Wants %s.' % ' or '.join(want))
        if self.kind != 'trap' and launch:
            b.append('Fires %s.' % ', '.join(SPELL_OF_MODE.get(a['id'], 'a spell') if a['spell'] else lv.item_name(a['id']) for a in launch))
        if doors:
            byverb = {}
            for v, c in doors:
                byverb.setdefault(v, []).append(lv.lab(c))
            b.append('; '.join('%s the door at %s' % (v.capitalize(), ', '.join(dict.fromkeys(cs))) for v, cs in byverb.items()) + '.')
        if walls:
            byverb = {}
            for v, c in walls:
                byverb.setdefault(v, []).append(lv.lab(c))
            word = {'open': 'Opens the wall at', 'wall': 'Builds a wall at', 'toggle': 'Opens or closes the wall at'}
            b.append('; '.join('%s %s' % (word[v], ', '.join(dict.fromkeys(cs))) for v, cs in byverb.items()) + '.')
        if moved_monsters and self.kind not in ('trap', 'monster'):
            b.append('Moves the monsters at %s.' % ', '.join(sorted({lv.lab(a['src']) for a in moved_monsters})))
        if new_items:
            ws = []
            for it, c in new_items:
                ws.append('%s %s' % (lv.item_name(it), 'into your hand' if c == 0xFFFF else 'at ' + lv.lab(c)))
            b.append('Gives ' + '; '.join(dict.fromkeys(ws)) + '.')
        if spawns and self.kind not in ('trap', 'monster'):
            b.append('Monsters: %s.' % self.spawn_words(spawns))
        if dmg and self.kind != 'trap':
            b.append('Damage %dd%d.' % (dmg[0]['count'], dmg[0]['sides']))
        if races:
            rw = ' or '.join(RACES[r] for r in races if r < 6)
            b.append('%s %s in the party reads it.' % ('An' if rw[:1] in 'aeiou' else 'A', rw))
        if classes:
            b.append('Needs a %s.' % ' or '.join(CLASS_BITS.get(c, 'class %d' % c) for c in classes))
        if panels and self.kind not in ('portal', 'npc') and not (self.kind in ('up', 'down') and panels == [10]):
            b.append('Shows %s.' % ', '.join(PANELS.get(p, 'panel %d' % p) for p in dict.fromkeys(panels)))
        if xp:
            guarded = any(tok[0] in ('lflag', 'gflag') for i in A('IF') for tok in i.a['expr'])
            self.xp_once = guarded
            b.append('+%s XP%s.' % (' or '.join('{:,}'.format(x) for x in xp), ' the first time' if guarded else ''))
        if self.kind in ('switch', 'quest', 'secret', 'text', 'trap', 'monster', 'lock') and where not in ('click', 'step'):
            b.append('Set off by: %s.' % where)

    def passable_now(self):
        return any(self.L.flags(k) & 1 for k in self.L.cells[self.cell])

    def held_words(self, held):
        return ' or '.join(key_names(t, v) for t, v in held)

    @staticmethod
    def spawn_words(spawns):
        from collections import Counter
        c = Counter(lv.MONSTER_NAMES[s['type']] for s in spawns)
        return ', '.join('%d %s' % (n, plural(m) if n > 1 else m) for m, n in c.items())


def plural(m):
    if m.endswith('elf'):
        return m[:-3] + 'elves'
    if m.endswith('mage'):
        return m + 's'
    if m == 'Xanathar' or m.endswith('s'):
        return m
    return m + 's'


def events(L):
    return [Event(L, t) for t in L.triggers]


def main(argv):
    n = int(argv[1]) if len(argv) > 1 else 1
    L = lv.Level(n)
    from collections import Counter
    evs = events(L)
    print(Counter(e.kind for e in evs))
    for e in evs:
        print('%-9s %-8s %-9s %s | %s' % (e.kind, lv.lab(e.cell), e.how(), e.title, ' '.join(e.body)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
