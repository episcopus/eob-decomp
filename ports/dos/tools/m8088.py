"""m8088.py: what a basic block costs on an 8088 (Tandy 1000 EX: 7.16 MHz, 8-bit bus).

Clocks are the Intel 8088 tables (iAPX 88 Book, "Instruction Set Reference"), which already count the 8-bit bus for
memory operands. What the tables leave out is the time an instruction waits for its own bytes: the 8088 fetches one
byte per bus cycle into a 4-byte queue, so on tight code it is the bus, not the execution unit, that sets the pace.
Each block is therefore simulated with a queue: empty at the start of the block (a taken jump flushes it), filled by
the idle clocks of the instructions before.

A conditional jump's cost depends on whether it was taken (16 clocks, or 4), which a block cannot know: the block
keeps both and the next block's address decides (Costs.jump). A shift by CL costs 4 clocks per bit: CL is taken from
a `mov cl, imm` earlier in the block, or read at run time when the block does not set it.

BUS is the clocks of one bus cycle: 4, plus the wait states. The Tandy 1000 HX technical reference (same design as the
EX) says RAM shared with video inserts "typically one or two" wait cycles, so the model's default is 5; run with
EOB_BUS=4 or 6 for the two ends (results in docs: the ratios barely move).
"""
from __future__ import annotations

import os

from capstone import CS_ARCH_X86, CS_MODE_16, Cs, x86

BUS = int(os.environ.get("EOB_BUS", "5"))
MHZ = 7.15909

ALU = {"add", "adc", "sub", "sbb", "and", "or", "xor"}
JCC = {"jo", "jno", "jb", "jae", "je", "jne", "jbe", "ja", "js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"}
SHIFT = {"shl", "sal", "shr", "sar", "rol", "ror", "rcl", "rcr"}
# name: (clocks per repetition under REP, clocks alone, bytes on the bus per repetition)
STRING = {"movsb": (17, 18, 2), "movsw": (25, 26, 4), "stosb": (10, 11, 1), "stosw": (14, 15, 2),
          "lodsb": (13, 12, 1), "lodsw": (17, 16, 2), "scasb": (15, 15, 1), "scasw": (19, 19, 2),
          "cmpsb": (22, 22, 2), "cmpsw": (30, 30, 4)}
SIMPLE = {"clc": 2, "stc": 2, "cmc": 2, "cld": 2, "std": 2, "cli": 2, "sti": 2, "nop": 3, "cbw": 2, "cwd": 5,
          "sahf": 4, "lahf": 4, "xlatb": 11, "aaa": 4, "aas": 4, "daa": 4, "das": 4, "aam": 83, "aad": 60, "hlt": 2,
          "wait": 4, "into": 4}

md = Cs(CS_ARCH_X86, CS_MODE_16)
md.detail = True


def _ea(op, seg_prefix: bool) -> int:
    m = op.mem
    base, index, disp = m.base != 0, m.index != 0, m.disp != 0
    if not base and not index:
        c = 6
    elif base and index:
        slow = (m.base, m.index) in ((x86.X86_REG_BP, x86.X86_REG_SI), (x86.X86_REG_BX, x86.X86_REG_DI))
        c = (8 if slow else 7) + (4 if disp else 0)
    else:
        c = 5 + (4 if disp else 0)
    return c + (2 if seg_prefix else 0)


class Insn:
    __slots__ = ("clocks", "busbytes", "length", "taken", "not_taken", "cl_bits", "per_rep", "mn")


def cost(i, known_cl: int | None) -> Insn:
    """clocks (table), bytes the instruction moves on the bus, length; for jumps taken/not taken; `cl_bits` is 1 when
    the clocks still need 4 * CL added at run time."""
    r = Insn()
    r.length, r.busbytes, r.taken, r.not_taken, r.cl_bits, r.per_rep = i.size, 0, 0, 0, 0, False
    mn = i.mnemonic
    rep = mn.startswith("rep")
    if rep:
        mn = mn.split()[-1]
    r.mn = mn
    if mn in STRING:
        per, one, n = STRING[mn]
        r.clocks, r.busbytes, r.per_rep = (per if rep else one), n, rep
        return r
    if mn in SIMPLE:
        r.clocks = SIMPLE[mn]
        return r
    ops = i.operands
    mem = [o for o in ops if o.type == x86.X86_OP_MEM]
    ea = _ea(mem[0], i.prefix[1] != 0) if mem else 0
    size = max((o.size for o in ops), default=2)
    w = size >= 2
    dst_mem = bool(ops) and ops[0].type == x86.X86_OP_MEM
    imm = bool(ops) and ops[-1].type == x86.X86_OP_IMM
    m_rw = 0
    if mn == "mov":
        moffs = mem and mem[0].mem.base == 0 and mem[0].mem.index == 0 and any(
            o.type == x86.X86_OP_REG and o.reg in (x86.X86_REG_AX, x86.X86_REG_AL) for o in ops)
        if moffs:
            c = 14 if w else 10
            ea = 0
        elif mem and dst_mem and imm:
            c = 14 if w else 10
        elif mem and dst_mem:
            c = 13 if w else 9
        elif mem:
            c = 12 if w else 8
        else:
            c = 4 if imm else 2
        m_rw = 1
    elif mn in ALU or mn in ("cmp", "test"):
        ro = mn in ("cmp", "test")
        if mem:
            if dst_mem and imm:
                c = ({True: 14, False: 10} if ro else {True: 25, False: 17})[w]
                if mn == "test":
                    c += 1
            elif dst_mem and not ro:
                c = 24 if w else 16
            else:
                c = 13 if w else 9
            m_rw = 1 if ro else 2
        else:
            c = 3 if not imm else (5 if mn == "test" else 4)
    elif mn in ("inc", "dec"):
        c, m_rw = (23 if w else 15, 2) if mem else (2 if w else 3, 0)
    elif mn in ("neg", "not"):
        c, m_rw = (24 if w else 16, 2) if mem else (3, 0)
    elif mn in SHIFT:
        by_cl = len(ops) > 1 and ops[1].type == x86.X86_OP_REG
        if by_cl:
            c, m_rw = (28 if w else 20, 2) if mem else (8, 0)
            if known_cl is not None:
                c += 4 * known_cl
            else:
                r.cl_bits = 1
        else:
            n = ops[1].imm if len(ops) > 1 and ops[1].type == x86.X86_OP_IMM else 1
            c, m_rw = (23 if w else 15, 2) if mem else (2, 0)
            c *= n                                # the assembler's shl r, n is n one-bit shifts on a real 8088
    elif mn == "push":
        c, m_rw = (24, 1) if mem else (14 if ops[0].type == x86.X86_OP_REG and ops[0].reg in (x86.X86_REG_CS, x86.X86_REG_DS, x86.X86_REG_ES, x86.X86_REG_SS) else 15, 0)
        r.busbytes = 2
    elif mn == "pop":
        c, m_rw = (25, 1) if mem else (12, 0)
        r.busbytes = 2
    elif mn in ("pushf", "popf"):
        c = 14 if mn == "pushf" else 12
        r.busbytes = 2
    elif mn == "call":
        if mem:
            c, r.busbytes, m_rw = 29, 2, 1
        elif ops[0].type == x86.X86_OP_REG:
            c, r.busbytes = 20, 2
        else:
            c, r.busbytes = 23, 2
    elif mn == "lcall":
        c, r.busbytes, m_rw = (53, 4, 1) if mem else (36, 4, 0)
    elif mn == "ret":
        c, r.busbytes = (24 if ops else 20), 2
    elif mn == "retf":
        c, r.busbytes = (33 if ops else 34), 4
    elif mn == "iret":
        c, r.busbytes = 44, 6
    elif mn == "int":
        c, r.busbytes = 71, 10
    elif mn == "jmp":
        if mem:
            c, m_rw = 18, 1
        elif ops[0].type == x86.X86_OP_REG:
            c = 11
        else:
            c = 15
    elif mn == "ljmp":
        c, m_rw = (24, 1) if mem else (15, 0)
    elif mn in JCC:
        c, r.taken, r.not_taken = 0, 16, 4
    elif mn == "jcxz":
        c, r.taken, r.not_taken = 0, 18, 6
    elif mn == "loop":
        c, r.taken, r.not_taken = 0, 17, 5
    elif mn == "loope":
        c, r.taken, r.not_taken = 0, 18, 6
    elif mn == "loopne":
        c, r.taken, r.not_taken = 0, 19, 5
    elif mn == "mul":
        c, m_rw = (118 + 6 + 8, 1) if (w and mem) else (125, 0) if w else (80, 1) if mem else (74, 0)
    elif mn == "imul":
        c, m_rw = (146, 1) if (w and mem) else (140, 0) if w else (95, 1) if mem else (89, 0)
    elif mn == "div":
        c, m_rw = (168, 1) if (w and mem) else (153, 0) if w else (90, 1) if mem else (85, 0)
    elif mn == "idiv":
        c, m_rw = (180, 1) if (w and mem) else (175, 0) if w else (112, 1) if mem else (106, 0)
    elif mn == "lea":
        c = 2
    elif mn in ("les", "lds"):
        c, m_rw, r.busbytes = 24, 0, 4
    elif mn == "xchg":
        if mem:
            c, m_rw = (25 if w else 17), 2
        else:
            c = 3 if any(o.reg == x86.X86_REG_AX for o in ops) else 4
    elif mn in ("in", "out"):
        c = 14 if w else 10
        if any(o.type == x86.X86_OP_REG and o.reg == x86.X86_REG_DX for o in ops):
            c -= 2
        r.busbytes = 2 if w else 1
    else:
        c = 8
    r.busbytes += m_rw * (2 if w else 1) if mem else 0
    r.clocks = c + ea
    return r


class Block:
    __slots__ = ("cycles", "bus", "taken", "not_taken", "fall", "routine", "cl_bits", "size")


def block_cost(code: bytes, addr: int, routine: int) -> Block:
    """Simulate the queue over the block. Returns its fixed cost; the last jump's two costs stay separate."""
    q = 0                     # bytes in the queue
    cycles = bus = cl_bits = 0
    known_cl = None
    b = Block()
    b.taken = b.not_taken = 0
    n = 0
    insns = list(md.disasm(code, addr & 0xFFFF))
    for k, i in enumerate(insns):
        n += i.size
        c = cost(i, known_cl)
        if i.mnemonic == "mov" and i.operands[0].type == x86.X86_OP_REG and i.operands[0].reg in (x86.X86_REG_CL, x86.X86_REG_CX):
            known_cl = i.operands[1].imm & 0xFF if i.operands[1].type == x86.X86_OP_IMM else None
        elif i.operands and i.operands[0].type == x86.X86_OP_REG and i.operands[0].reg in (x86.X86_REG_CL, x86.X86_REG_CX, x86.X86_REG_CH):
            known_cl = None
        miss = max(0, c.length - q)
        q = max(0, q - c.length)
        cycles += miss * BUS
        bus += miss
        if c.per_rep:
            # REP: Unicorn runs each repetition as a block of its own, so a repetition is charged here;
            # the 9 clocks of set-up are charged only when other instructions lead to it
            lead = 9 if k else 0
            cycles += (c.clocks + c.busbytes * (BUS - 4)) + lead - (miss * BUS if not k else 0)
            bus += c.busbytes - (miss if not k else 0)
            q = 0
            continue
        cycles += c.clocks + c.busbytes * (BUS - 4)
        bus += c.busbytes
        cl_bits += c.cl_bits
        idle = c.clocks - c.busbytes * 4
        if idle > 0:
            q = min(4, q + idle // BUS)
        if c.taken:
            b.taken, b.not_taken = c.taken, c.not_taken
    b.cycles, b.bus, b.fall, b.routine, b.cl_bits, b.size = cycles, bus, (addr + n) & 0xFFFFF, routine, cl_bits, len(code)
    return b
