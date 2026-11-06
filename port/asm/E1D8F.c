/* E1D8F.c: src/E1D8F.ASM in C (playbook 80, M3): the INT 1Ch tick handler, its install and removal,
   and the atomic read of a tick counter.

   The unit's code segment begins with the saved INT 1Ch vector (offset at cs:0002, segment at
   cs:0004, zero at load); it stays in the arena there. The handler (1D8F:0055) works on DGROUP
   (`mov ax,212Eh`, relocated at load), whatever DS it is entered with. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1D8Fu
#define OLD_OFF CS_VAR(SEG, 0x0002u)
#define OLD_SEG CS_VAR(SEG, 0x0004u)
#define D(off) TC_MKFP(tc_load_seg + 0x212Eu, (off))

/* ds: 32-bit counters the handler steps: gTicksSinceEvent (8488, 16 bits), gTicks (848A), two gated by
   gTimerEnableFlags (849E) bits 0 and 1 (848E, gGameTicks at 8492), 8496 (ReadTickCount's), and the
   countdown gFrameWaitTicks (849A), which stops at 0 */
static void inc32(u16 off)
{
    u16 lo = (u16)(tc_ld_u16(D(off)) + 1u);
    tc_st_u16(D(off), lo);
    if (lo == 0) {
        tc_st_u16(D(off + 2u), (u16)(tc_ld_u16(D(off + 2u)) + 1u));
    }
}

/*@ 1D8F:0002 */ u32 ReadTickCount(void)
{
    /* cli; DX:AX = ds:8498:8496; sti (DS is the caller's, DGROUP) */
    return tc_ld_u32(TC_MKFP(tc_ds, 0x8496u));
}

/*@ 1D8F:0010 */ void InstallTickHandler(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x351Cu;                             /* get the INT 1Ch vector: ES:BX */
    x86_int(0x21, &r);
    tc_st_u16(OLD_SEG, r.es);
    tc_st_u16(OLD_OFF, r.bx);
    r.ax = 0x251Cu;                             /* set it to cs:0055 */
    r.ds = (u16)(tc_load_seg + SEG);
    r.dx = 0x0055u;
    x86_int(0x21, &r);
}

/*@ 1D8F:0038 */ void RemoveTickHandler(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x251Cu;                             /* set INT 1Ch to the saved vector */
    r.ds = tc_ld_u16(OLD_SEG);
    r.dx = tc_ld_u16(OLD_OFF);
    x86_int(0x21, &r);
}

/*@ 1D8F:0055 interrupt */ void TickIsr(void)
{
    u16 flags = tc_ld_u16(D(0x849Eu));
    if (flags & 1u) {
        inc32(0x848Eu);
    }
    if (flags & 2u) {
        inc32(0x8492u);
    }
    tc_st_u16(D(0x8488u), (u16)(tc_ld_u16(D(0x8488u)) + 1u));
    inc32(0x848Au);
    inc32(0x8496u);
    if (tc_ld_u32(D(0x849Au)) != 0) {
        tc_st_u32(D(0x849Au), tc_ld_u32(D(0x849Au)) - 1u);
    }
    /* chain to the saved vector unless its segment is 0 */
    if (tc_ld_u16(OLD_SEG) != 0) {
        x86_chain(TC_MKFP(tc_ld_u16(OLD_SEG), tc_ld_u16(OLD_OFF)));
    }
}
