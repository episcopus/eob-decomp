/* E1BA8.c: src/E1BA8.ASM in C (playbook 80, M3): the far copy with normalized pointers. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

/* Copies len bytes: the low word's count as words then an odd byte, then the high word's count of
   64 KB blocks (8000h words each), both pointers normalized (NormFar, 1BA8:0081) first and before
   each block. Word moves, as `rep movsw`: an overlapping copy moves two bytes at a time. */
/*@ 1BA8:0006 */ void Unpack0(farptr src, farptr dst, s32 len)
{
    farptr p;
    u16 si, ds, di, es, cx, bx, odd;

    p = x86_norm_far(dst);
    di = TC_OFF(p);
    es = TC_SEG(p);
    p = x86_norm_far(src);
    si = TC_OFF(p);
    ds = TC_SEG(p);
    cx = (u16)len;
    bx = (u16)((u32)len >> 16);
    odd = cx & 1u;                              /* shr cx,1: CF */
    cx >>= 1;
    x86_rep_movsw(ds, &si, es, &di, &cx);
    if (odd) {
        x86_stosb(es, &di, x86_lodsb(ds, &si));
    }
    for (; bx != 0; bx--) {
        p = x86_norm_far(TC_MKFP(es, di));
        di = TC_OFF(p);
        es = TC_SEG(p);
        p = x86_norm_far(TC_MKFP(ds, si));
        si = TC_OFF(p);
        ds = TC_SEG(p);
        cx = 0x8000u;
        x86_rep_movsw(ds, &si, es, &di, &cx);
    }
}

/*@ 1BA8:0072 */ farptr NormalizePtr(farptr p)
{
    return x86_norm_far(p);
}
