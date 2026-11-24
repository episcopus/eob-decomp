#!/usr/bin/env python3
"""level.py: one dungeon level of Eye of the Beholder, decoded from the pinned files in eob/.

Everything here is read the way the game reads it (whitepaper 5.3, 5.5; the functions named):

  LEVELn.MAZ   u16 32, u16 32, u16 4, then 1,024 cells of four sides in direction order 0 to 3
               (north, east, south, west); a side is a wall kind (LoadMaze, ovl3:09AA)
  LEVELn.INF   packed (wwfmt.unpack); LoadLevel (ovl3:0A7C, src/EOV3.C) reads:
                 +00 u16  offset of the trigger table
                 +02      maze file name (12 bytes), +0E wall set name (12), +1A wall picture name (12)
                 +26      four door picture bytes (LoadDoorShapes)
                 +2A      level timer: u8 mode, u16 period, u16 steps
                 +2F      two monster pictures: u8 loader (FF: none), name (12 bytes)
                 then     (u8 group, u8 period) pairs to FF: the monster update periods
                 then     30 monster records of 14 bytes (InitMonster, 08E6:131D)
                 then     u16 count, then wall records: EC (two 12-byte names: LoadWallSet) or
                          FB kind, picture set, decoration, handler, flags (DefineWallType, ovl3:01C8)
                 then     the scripts, up to the trigger table
                 table    u16 count; per entry u16 cell, u8 event mask, u16 script offset
  ITEM.DAT     the 500-record item table; a record whose level is n and whose key (cell) is
               positive lies on level n (RebuildItemList, ovl3:0951; eobfmt.items)

The script disassembler follows the handlers of src/E122E.C one by one (the operand reads of each
`...Op` function and of EvaluateScriptExpr, 122E:147F).

  python3 tools/maps/level.py 1            the level's header, monsters and every script as text
"""
from __future__ import annotations

import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'tools', 're'))
import eobfmt  # noqa: E402
import pak  # noqa: E402
import wwfmt  # noqa: E402

EOB = os.path.join(ROOT, 'eob')
N = 32
DIRS = 'NESW'
STEP = {0: (0, -1), 1: (1, 0), 2: (0, 1), 3: (-1, 0)}   # (dx, dy) of direction 0..3 (StepCell, gDirCellOffset)

# Monster type names: the sprite sheet each type is paired with in the level files (whitepaper 5.6.1),
# written as a player would say them.
MONSTER_NAMES = ['kobold', 'leech', 'skeleton', 'zombie', 'kuo-toa', 'flind', 'giant spider', 'dwarf',
                 'skeleton lord', 'displacer beast', 'mantis warrior', 'shindia', 'drow elf', 'stone golem',
                 'drow mage', 'drider', 'kenku', 'mind flayer', 'rust monster', 'xorn', 'hell hound', 'Xanathar']

_paks: dict[str, dict] = {}


def member(name: str) -> bytes:
    """a file from any of the six PAK archives in eob/"""
    if not _paks:
        for f in sorted(os.listdir(EOB)):
            if f.upper().endswith('.PAK'):
                for e in pak.read_pak(open(os.path.join(EOB, f), 'rb').read())['entries']:
                    _paks[e['name'].upper()] = e['data']
    return _paks[name.upper()]


def u16(d: bytes, p: int) -> int:
    return struct.unpack_from('<H', d, p)[0]


def s16(d: bytes, p: int) -> int:
    return struct.unpack_from('<h', d, p)[0]


def cstr(b: bytes) -> str:
    return b.split(b'\0', 1)[0].decode('latin-1')


def xy(cell: int) -> tuple[int, int]:
    return cell & 31, cell >> 5


def lab(cell: int) -> str:
    x, y = xy(cell)
    return '(%d,%d)' % (x, y)


# ---------------------------------------------------------------------------------------------
# items

_items = None


def _src(name: str) -> str:
    return open(os.path.join(ROOT, 'src', name), encoding='latin-1').read()


def _c_strings(src: str, name: str) -> list[str]:
    """a table of string literals (`char far *name[] = { "..", .. }`)"""
    m = re.search(r'\b%s\[\d+\]\s*=\s*\{(.*?)\};' % name, src, re.S)
    return re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))


def _c_bytes(src: str, name: str) -> list[int]:
    m = re.search(r'\b%s\[\d+\]\s*=\s*\{[^}]*?\*/(.*?)\};' % name, src, re.S)
    return [int(v, 0) & 0xFF for v in re.findall(r'-?\d+', m.group(1))]


def _skip_strings(table: list[int], n: int) -> str:
    """SkipStrings: the n-th NUL-terminated string of a packed table"""
    parts = bytes(table).split(b'\0')
    return parts[n].decode('latin-1') if n < len(parts) else ''


def items() -> dict:
    """ITEM.DAT decoded: records (dicts with the name a player sees once the item is identified)"""
    global _items
    if _items is None:
        raw = eobfmt.items(member('ITEM.DAT'))
        names = [n.decode('latin-1') for n in raw['names']]
        types = eobfmt.item_types(member('ITEMTYPE.DAT'))
        d07 = _src('D07C8.C')
        b2 = _src('E0BD2.C')
        mage, cleric = _c_strings(d07, 'gMageSpellNames'), _c_strings(d07, 'gClericSpellNames')
        rings, potions, wands = (_c_bytes(b2, t) for t in ('gRingNames', 'gPotionNames', 'gWandNames'))
        recs = []
        for i, r in enumerate(raw['items']):
            # ItemRecord (src/EOB.H): nameIndex, identifiedNameIndex, flags, icon, type, cellPos,
            # key u16, prevLink u16, nextLink u16, level, value
            first, second = names[r[0]], names[r[1]]
            typ, value = r[4], struct.unpack_from('<b', r, 13)[0]
            kind = types[typ][0x0E] & 0x7F if typ < len(types) else 0
            # the identified name, as DrawItemName (0BD2:05B5) builds it when flag 0x40 is set
            if second:
                shown = second
            elif kind in (0, 1, 2, 3):
                shown = first if value == 0 else ('Cursed %s %d' % (first, value) if value < 0 else '%s +%d' % (first, value))
            elif kind == 9:
                shown = 'Mage Scroll of %s' % mage[value]
            elif kind == 10:
                shown = 'Cleric Scroll of %s' % cleric[value]
            elif kind == 16:
                shown = 'Ring of %s' % _skip_strings(rings, value)
            elif kind == 14:
                shown = 'Potion of %s' % _skip_strings(potions, value)
            elif kind == 18:
                shown = 'Wand of %s' % _skip_strings(wands, value)
            else:
                shown = first
            recs.append(dict(index=i, name=first, shown=shown, flags=r[2], icon=r[3], type=typ, kind=kind,
                             pos=r[5], key=s16(r, 6), level=r[12], value=value))
        _items = dict(records=recs, names=names)
    return _items


def item_name(i: int) -> str:
    """the name a player sees once the item is identified"""
    return items()['records'][i]['shown']


# ---------------------------------------------------------------------------------------------
# scripts

OPS = {0xFF: 'SET WALL', 0xFE: 'TOGGLE WALL', 0xFD: 'OPEN DOOR', 0xFC: 'CLOSE DOOR', 0xFB: 'SPAWN',
       0xFA: 'MOVE', 0xF9: 'STEAL ITEM', 0xF8: 'TEXT', 0xF7: 'SET FLAG', 0xF6: 'SOUND', 0xF5: 'CLEAR FLAG',
       0xF4: 'HEAL', 0xF3: 'DAMAGE', 0xF2: 'JUMP', 0xF1: 'END', 0xF0: 'RETURN', 0xEF: 'CALL', 0xEE: 'IF',
       0xED: 'DRAIN', 0xEC: 'LOAD LEVEL', 0xEB: 'GIVE XP', 0xEA: 'NEW ITEM', 0xE9: 'LAUNCH', 0xE8: 'TURN',
       0xE7: 'IDENTIFY', 0xE6: 'OVERLAY'}


class Ins:
    """one decoded instruction: offset, opcode, mnemonic, operand dict, length"""
    __slots__ = ('off', 'op', 'name', 'a', 'size')

    def __init__(self, off, op, a, size):
        self.off, self.op, self.name, self.a, self.size = off, op, OPS[op], a, size

    def __repr__(self):
        return '%04X %s %s' % (self.off, self.name, ' '.join('%s=%s' % kv for kv in self.a.items()))


def expr(d: bytes, p: int) -> tuple[list, int]:
    """the postfix expression of an IF (EvaluateScriptExpr): tokens up to EE, returns (tokens, end)"""
    toks = []
    while True:
        b = d[p]
        p += 1
        if b == 0xEE:
            return toks, p
        if b < 0x80:
            toks.append(('lit', b))
        elif b == 0xE0:
            toks.append(('mode',))
        elif b == 0xED:
            toks.append(('facing',))
        elif b == 0xF7:
            toks.append(('wall', u16(d, p), 0)); p += 2
        elif b == 0xE9:
            toks.append(('wall', u16(d, p + 1), d[p])); p += 3
        elif b == 0xF5:
            toks.append(('items', d[p], u16(d, p + 1))); p += 3
        elif b == 0xF2:
            toks.append(('itemin', u16(d, p), u16(d, p + 2))); p += 4
        elif b == 0xF3:
            toks.append(('monsters', u16(d, p))); p += 2
        elif b == 0xF1:
            toks.append(('partyin', u16(d, p))); p += 2
        elif b == 0xF0:
            toks.append(('gflag', d[p])); p += 1
        elif b == 0xEF:
            toks.append(('lflag', d[p])); p += 1
        elif b == 0xE7:
            toks.append(('held', {0xE1: 'type', 0xF5: 'id'}.get(d[p], 'value'))); p += 1
        elif b == 0xDD:
            toks.append(('race', d[p])); p += 1
        elif b == 0xDC:
            toks.append(('class', d[p])); p += 1
        elif b == 0xDB:
            toks.append(('dice', d[p], d[p + 1], struct.unpack_from('<b', d, p + 2)[0])); p += 3
        elif b == 0xDA:
            toks.append(('alleffect',))
        elif b in (0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8):
            toks.append(('op', {0xFF: '==', 0xFE: '!=', 0xFD: '<', 0xFC: '<=', 0xFB: '>', 0xFA: '>=',
                                0xF9: '&&', 0xF8: '||'}[b]))
        else:
            toks.append(('nop', b))   # a negative byte not in the table pushes nothing


def decode(d: bytes, p: int) -> Ins:
    """the instruction at offset p of the unpacked file"""
    op = d[p]
    q = p + 1
    a = {}
    if op == 0xFF:                       # SetWallTypeOp
        sub = d[q]; q += 1
        if sub == 0xF7:
            a = dict(sub='all', cell=u16(d, q), kind=d[q + 2]); q += 3
        elif sub == 0xE9:
            a = dict(sub='side', cell=u16(d, q), side=d[q + 2], kind=d[q + 3]); q += 4
        elif sub == 0xED:
            a = dict(sub='facing', dir=d[q]); q += 1
        else:
            a = dict(sub='?%02X' % sub)
    elif op == 0xFE:                     # ToggleWallTypeOp
        sub = d[q]; q += 1
        if sub == 0xF7:
            a = dict(sub='all', cell=u16(d, q), a=d[q + 2], b=d[q + 3]); q += 4
        elif sub == 0xEA:
            a = dict(sub='door', cell=u16(d, q)); q += 2
        else:
            a = dict(sub='?%02X' % sub)
    elif op in (0xFD, 0xFC):             # OpenDoorOp, CloseDoorOp
        a = dict(cell=u16(d, q)); q += 2
    elif op == 0xFB:                     # SpawnMonsterOp: an unused byte, then the record's 13 bytes
        r = d[q + 1:q + 14]
        a = dict(group=r[0], cell=u16(r, 1), pos=r[3], dir=r[4], type=r[5], alt=r[6], mode=r[7],
                 timer=r[8], drop1=u16(r, 9), drop2=u16(r, 11)); q += 14
    elif op == 0xFA:                     # TeleportOp
        sub = d[q]; q += 1
        if sub != 0xF1:
            src = u16(d, q); q += 2
        else:
            src = None
        a = dict(sub={0xE8: 'party', 0xF3: 'monsters', 0xF5: 'items', 0xF1: 'redraw'}.get(sub, '?%02X' % sub),
                 src=src, dst=u16(d, q)); q += 2
    elif op == 0xF9:                     # MoveCarriedItemOp
        a = dict(who=d[q], cell=u16(d, q + 1), pos=d[q + 3]); q += 4
    elif op == 0xF8:                     # ShowTextOp: string, NUL, two colour bytes
        e = d.index(b'\0', q)
        a = dict(text=d[q:e].decode('latin-1'), c1=d[e + 1], c2=d[e + 2]); q = e + 3
    elif op == 0xF7:                     # SetFlagOp
        sub = d[q]; q += 1
        if sub in (0xEF, 0xF0):
            a = dict(sub='level' if sub == 0xEF else 'game', bit=d[q]); q += 1
        elif sub == 0xF3:
            a = dict(sub='monster', slot=d[q], bit=d[q + 1]); q += 2
        else:
            a = dict(sub='unread' if sub == 0xE4 else '?%02X' % sub)
    elif op == 0xF6:                     # PlaySoundOp
        a = dict(sound=d[q], cell=u16(d, q + 1)); q += 3
    elif op == 0xF5:                     # ClearFlagOp
        a = dict(sub='level' if d[q] == 0xEF else ('game' if d[q] == 0xF0 else '?'), bit=d[q + 1]); q += 2
    elif op == 0xF4:                     # HealPartyMember
        a = dict(who=d[q], amount=d[q + 1]); q += 2
    elif op == 0xF3:                     # DamagePartyMember
        a = dict(who=d[q], count=d[q + 1], sides=d[q + 2], bonus=struct.unpack_from('<b', d, q + 3)[0]); q += 4
    elif op in (0xF2, 0xEF):             # JumpToScriptOffset, PushScriptPointer
        a = dict(to=u16(d, q)); q += 2
    elif op in (0xF1, 0xF0):             # ScriptStop, PopScriptPointer
        pass
    elif op == 0xEE:                     # EvaluateScriptExpr
        toks, q = expr(d, q)
        a = dict(expr=toks, els=u16(d, q)); q += 2
    elif op == 0xED:                     # DrainItemOp
        sub = d[q]; q += 1
        if sub == 0xFF:
            a = dict(what='held')
        elif sub == 0xFE:
            a = dict(what='all', cell=u16(d, q)); q += 2
        else:
            a = dict(what='type', type=sub, cell=u16(d, q)); q += 2
    elif op == 0xEC:                     # LoadLevelOp: unused, level, cell, dir
        a = dict(level=d[q + 1], cell=u16(d, q + 2), dir=d[q + 4]); q += 5
    elif op == 0xEB:                     # AddExperienceOp
        if d[q] == 0xE2:
            a = dict(xp=u16(d, q + 1)); q += 3
        else:
            q += 1
    elif op == 0xEA:                     # CreateItemOp
        a = dict(item=u16(d, q), cell=u16(d, q + 2), pos=d[q + 4]); q += 5
    elif op == 0xE9:                     # LaunchProjectileOp
        a = dict(spell=d[q] == 0xDF, id=d[q + 1], cell=u16(d, q + 3), dir=d[q + 5], pos=d[q + 6]); q += 7
    elif op == 0xE8:                     # TurnOp
        a = dict(sub='party' if d[q] == 0xF1 else 'flying', n=d[q + 1]); q += 2
    elif op == 0xE7:                     # IdentifyItemsOp
        a = dict(cell=u16(d, q)); q += 2
    elif op == 0xE6:                     # CallOverlayOp
        a = dict(panel=d[q]); q += 1
    else:
        raise ValueError('byte %02X at %04X is not an opcode' % (op, p))
    return Ins(p, op, a, q - p)


# ---------------------------------------------------------------------------------------------
# the level

class Level:
    def __init__(self, n: int):
        self.n = n
        inf, _ = wwfmt.unpack(member('LEVEL%d.INF' % n))
        self.inf = inf
        self.maze_name = cstr(inf[2:14])
        self.wallset = cstr(inf[0x0E:0x1A])
        mz = wwfmt.maze(member(self.maze_name if '.' in self.maze_name else 'LEVEL%d.MAZ' % n))
        self.cells = [tuple(mz['cells'][i * 4:i * 4 + 4]) for i in range(N * N)]
        p = 0x2A
        self.timer_mode, self.timer_period, self.timer_steps = inf[p], u16(inf, p + 1), u16(inf, p + 3)
        p += 5
        self.monster_sheets = []
        for _ in range(2):
            if inf[p] != 0xFF:
                self.monster_sheets.append(cstr(inf[p + 1:p + 13]))
            p += 13
        self.periods = []
        while inf[p] != 0xFF:
            self.periods.append((inf[p], inf[p + 1]))
            p += 2
        p += 1
        self.monsters = []
        for i in range(30):
            r = inf[p:p + 14]
            p += 14
            cell = u16(r, 2)
            if cell:
                self.monsters.append(dict(slot=r[0], group=r[1], cell=cell, pos=r[4], dir=r[5], type=r[6],
                                          alt=r[7], mode=r[8], timer=r[9], drop1=u16(r, 10), drop2=u16(r, 12)))
        nrec = u16(inf, p)
        p += 2
        # kinds 0..22 are the program's own (whitepaper 5.3.2); the level redefines some and adds the rest
        self.kinds = {}
        self.wallsets = []
        for _ in range(nrec):
            if inf[p] == 0xEC:
                self.wallsets.append((cstr(inf[p + 1:p + 13]), cstr(inf[p + 13:p + 25])))
                p += 25
            else:
                k, vmp, dec, handler, flags = inf[p + 1:p + 6]
                self.kinds[k] = dict(vmp=vmp, decor=dec, handler=handler, flags=flags ^ 4)
                p += 6
        self.script_start = p
        tt = u16(inf, 0)
        self.triggers = []
        cnt = u16(inf, tt)
        for i in range(cnt):
            q = tt + 2 + i * 5
            self.triggers.append(dict(cell=u16(inf, q), mask=inf[q + 2], off=u16(inf, q + 3)))
        self.code = {}
        q = self.script_start
        while q < tt:
            ins = decode(inf, q)
            self.code[q] = ins
            q += ins.size
        if q != tt:
            raise ValueError('level %d: scripts end at %04X, the table is at %04X' % (n, q, tt))
        self.items = [r for r in items()['records'] if r['level'] == n and r['key'] > 0]

    # walls ---------------------------------------------------------------------------------
    def flags(self, kind: int) -> int:
        """the in-memory flag byte of a wall kind (bit 0 the party passes, bit 2 a monster, 3..5 door)"""
        if kind in self.kinds:
            return self.kinds[kind]['flags']
        return BUILTIN_FLAGS.get(kind, 0)

    def handler(self, kind: int) -> int:
        if kind in self.kinds:
            return self.kinds[kind]['handler']
        return BUILTIN_HANDLER.get(kind, 0)

    def side(self, cell: int, d: int) -> int:
        return self.cells[cell][d]

    def script_of(self, cell: int):
        for t in self.triggers:
            if t['cell'] == cell:
                return t
        return None

    def walk(self, off: int, seen=None):
        """every instruction reachable from offset off, following jumps, both arms of each IF, and calls"""
        seen = set() if seen is None else seen
        todo = [off]
        out = []
        while todo:
            q = todo.pop()
            while q in self.code and q not in seen:
                seen.add(q)
                ins = self.code[q]
                out.append(ins)
                if ins.op == 0xF2:
                    todo.append(ins.a['to'])
                    break
                if ins.op in (0xF1, 0xF0, 0xEC):
                    break
                if ins.op == 0xEE:
                    todo.append(ins.a['els'])
                if ins.op == 0xEF:
                    todo.append(ins.a['to'])
                q += ins.size
        out.sort(key=lambda i: i.off)
        return out

    def dump(self) -> str:
        lines = ['LEVEL %d  maze %s  walls %s  timer mode %d steps %d  monsters %s' % (
            self.n, self.maze_name, self.wallset, self.timer_mode, self.timer_steps, ', '.join(self.monster_sheets))]
        for m in self.monsters:
            lines.append('  monster %2d %-15s %s pos %d dir %d mode %d drops %d %d' % (
                m['slot'], MONSTER_NAMES[m['type']], lab(m['cell']), m['pos'], m['dir'], m['mode'], m['drop1'], m['drop2']))
        for k, v in sorted(self.kinds.items()):
            lines.append('  kind %2d vmp %d decor %d handler %d flags %02X' % (k, v['vmp'], v['decor'], v['handler'], v['flags']))
        for it in self.items:
            lines.append('  item %3d %-24s %s pos %d' % (it['index'], item_name(it['index']), lab(it['key']), it['pos']))
        starts = {t['off']: t for t in self.triggers}
        for q in sorted(self.code):
            if q in starts:
                t = starts[q]
                lines.append('-- cell %d %s mask %02X' % (t['cell'], lab(t['cell']), t['mask']))
            lines.append('  ' + repr(self.code[q]))
        return '\n'.join(lines)


def _c_array(name: str) -> list[int]:
    """a byte table of the program's data as src/D07C8.C defines it (matched to EOB.EXE by make match)"""
    src = open(os.path.join(ROOT, 'src', 'D07C8.C'), encoding='latin-1').read()
    m = re.search(r'\b%s\[\d+\]\s*=\s*\{[^}]*?\*/(.*?)\};' % name, src, re.S)
    return [int(v, 0) & 0xFF for v in re.findall(r'-?\d+', m.group(1))]


# the program's own wall kinds 0..22 (whitepaper 5.3.2, 5.4.3): 0 open, 1 and 2 walls, 3..22 four door
# families of five frames each (closed first); the level file adds and redefines the rest
BUILTIN_FLAGS = dict(enumerate(_c_array('gWallTypeFlags')[:23]))
BUILTIN_HANDLER = dict(enumerate(_c_array('gWallHandlerIndex')[:23]))


def main(argv):
    if len(argv) > 1:
        print(Level(int(argv[1])).dump())
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
