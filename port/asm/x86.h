/* x86.h: what the assembly routines of port/asm need of the 8086, on the arena.

   The routines of port/asm are the assembly units of src/ written in C by hand (playbook 80,
   decision 3). Where a routine's result depends on how an instruction moves its registers (a string
   instruction's pointer steps, a word move, a shift through the carry), it is written with these, on
   16-bit register variables named as the source names them, so the C reads beside the TASM line by
   line. The direction flag is clear in every routine that uses them (each begins with `cld` or is
   entered with it clear, as Turbo C code leaves it). Offsets wrap at 64 KB within their segment. */
#ifndef PORT_X86_H
#define PORT_X86_H

#include "tcport.h"
#include "machine.h"

/* in, out and int: the machine's (machine.h), as the CPU would reach it */
static inline u8 x86_inb(u16 port) { return (u8)machine_port_in(port, 1); }
static inline void x86_outb(u16 port, u8 v) { machine_port_out(port, 1, v); }
static inline void x86_int(u8 n, tc_regs *r) { machine_int(n, r); }

/* a far jump or call to a saved vector (a handler chaining to the one it replaced) */
static inline void x86_chain(farptr vector)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    machine_call_handler(TC_SEG(vector), TC_OFF(vector), &r);
}

/* a word or byte of the routine's code segment (its `cs:` variables), in the arena */
#define CS_VAR(seg, off) TC_MKFP(tc_load_seg + (seg), (off))

static inline u8 x86_lodsb(u16 ds, u16 *si)
{
    u8 v = tc_ld_u8(TC_MKFP(ds, *si));
    *si = (u16)(*si + 1u);
    return v;
}

static inline u16 x86_lodsw(u16 ds, u16 *si)
{
    u16 v = tc_ld_u16(TC_MKFP(ds, *si));
    *si = (u16)(*si + 2u);
    return v;
}

static inline void x86_stosb(u16 es, u16 *di, u8 v)
{
    tc_st_u8(TC_MKFP(es, *di), v);
    *di = (u16)(*di + 1u);
}

static inline void x86_stosw(u16 es, u16 *di, u16 v)
{
    tc_st_u16(TC_MKFP(es, *di), v);
    *di = (u16)(*di + 2u);
}

/* rep movsb, rep movsw, rep stosb: CX ends at 0 */
static inline void x86_rep_movsb(u16 ds, u16 *si, u16 es, u16 *di, u16 *cx)
{
    for (; *cx != 0; (*cx)--) {
        x86_stosb(es, di, x86_lodsb(ds, si));
    }
}

static inline void x86_rep_movsw(u16 ds, u16 *si, u16 es, u16 *di, u16 *cx)
{
    for (; *cx != 0; (*cx)--) {
        x86_stosw(es, di, x86_lodsw(ds, si));
    }
}

static inline void x86_rep_stosb(u16 es, u16 *di, u16 *cx, u8 al)
{
    for (; *cx != 0; (*cx)--) {
        x86_stosb(es, di, al);
    }
}

/* repe scasb (equal: 1) and repne scasb (equal: 0) compare AL with ES:DI; they return ZF, which is
   zf_in when CX is 0 at the start (no compare, flags unchanged) */
static inline int x86_rep_scasb(int while_equal, u16 es, u16 *di, u16 *cx, u8 al, int zf_in)
{
    int zf = zf_in;
    while (*cx != 0) {
        (*cx)--;
        zf = tc_ld_u8(TC_MKFP(es, *di)) == al;
        *di = (u16)(*di + 1u);
        if (zf != while_equal) {
            break;
        }
    }
    return zf;
}

/* repe cmpsb: DS:SI with ES:DI while equal; returns ZF (zf_in when CX is 0 at the start) */
static inline int x86_repe_cmpsb(u16 ds, u16 *si, u16 es, u16 *di, u16 *cx, int zf_in)
{
    int zf = zf_in;
    while (*cx != 0) {
        (*cx)--;
        zf = tc_ld_u8(TC_MKFP(ds, *si)) == tc_ld_u8(TC_MKFP(es, *di));
        *si = (u16)(*si + 1u);
        *di = (u16)(*di + 1u);
        if (!zf) {
            break;
        }
    }
    return zf;
}

/* 1BA8:0081 NormFar (src/E1BA8.ASM): DX:AX to the same address with the offset below 16 */
static inline farptr x86_norm_far(farptr p)
{
    return TC_MKFP((u16)(TC_SEG(p) + (TC_OFF(p) >> 4)), (u16)(TC_OFF(p) & 0x0Fu));
}

#endif
