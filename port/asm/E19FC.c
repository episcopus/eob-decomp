/* E19FC.c: src/E19FC.ASM in C (playbook 80, M3): CompressLcw, the LCW encoder.

   Written from the TASM source, label by label, with 16-bit register variables named as the source
   names them. Its eight state words stay in its code segment, in the arena (19FC:019B to 01A9). The
   source's QUIRK at 19FC:007E and 19FC:00D7 (a run that reaches the end is one byte short) follows
   from the same string instructions here. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x19FCu
#define LIT_OPEN 0x19Bu
#define OUT_BASE 0x19Du
#define IN_BASE 0x19Fu
#define LIT_HDR 0x1A1u
#define OUT_SEG 0x1A3u
#define OUT_PTR 0x1A5u
#define BEST_LEN 0x1A7u
#define BEST_POS 0x1A9u
#define RD(v) tc_ld_u16(CS_VAR(SEG, (v)))
#define WR(v, x) tc_st_u16(CS_VAR(SEG, (v)), (u16)(x))

/*@ 19FC:0008 */ s16 CompressLcw(farptr src, farptr dst, u16 len)
{
    farptr p;
    u16 si, ds, di, es, bp, ax, bx, cx, dx;
    u8 al;

    p = x86_norm_far(src);
    si = TC_OFF(p);
    ds = TC_SEG(p);
    p = x86_norm_far(dst);
    di = TC_OFF(p);
    es = TC_SEG(p);
    dx = len;
    bp = (u16)(si + dx);                        /* the end of the source */
    WR(LIT_OPEN, 1);
    WR(OUT_BASE, di);
    WR(IN_BASE, si);
    WR(LIT_HDR, di);
    x86_stosb(es, &di, 0x81u);                  /* open the first literal run */
    x86_stosb(es, &di, x86_lodsb(ds, &si));
mainloop:
    WR(OUT_PTR, di);
    WR(OUT_SEG, es);
    es = ds;                                    /* es = the source segment for the searches */
    di = RD(IN_BASE);
    WR(BEST_LEN, 1);
search:
    al = tc_ld_u8(TC_MKFP(ds, si));
    if (al == tc_ld_u8(TC_MKFP(ds, (u16)(si + 0x40u)))) {
        /* a candidate fill run */
        bx = di;
        di = si;
        cx = (u16)(bp - di);
        (void)x86_rep_scasb(1, es, &di, &cx, al, cx == 0);
        di--;
        cx = (u16)(di - si);
        if (cx >= 0x41u) {
            WR(LIT_OPEN, 0);                    /* close the literal run */
            si = di;
            es = RD(OUT_SEG);
            di = RD(OUT_PTR);
            x86_stosb(es, &di, 0xFEu);          /* FEh fill: count word, byte */
            x86_stosw(es, &di, cx);
            x86_stosb(es, &di, al);
            WR(OUT_PTR, di);
            es = ds;
            di = bx;
            goto search;
        }
        di = bx;                                /* noRun */
    }
nextcand:
    cx = (u16)(si - di);
    if (cx == 0) {
        goto notfound;
    }
    if (!x86_rep_scasb(0, es, &di, &cx, al, 0)) {
        goto notfound;
    }
    bx = RD(BEST_LEN);
    /* a candidate can only beat gBestLen if it agrees gBestLen - 1 bytes ahead */
    if (tc_ld_u8(TC_MKFP(ds, (u16)(bx + si - 1u))) != tc_ld_u8(TC_MKFP(es, (u16)(bx + di - 2u)))) {
        goto nextcand;
    }
    dx = si;
    bx = di;
    di--;
    cx = (u16)(bp - si);
    (void)x86_repe_cmpsb(ds, &si, es, &di, &cx, 1);
    si = dx;
    ax = (u16)(di - bx);
    di = bx;
    if (ax < RD(BEST_LEN)) {
        goto search;                            /* jb: an equal length replaces the earlier match */
    }
    WR(BEST_LEN, ax);
    WR(BEST_POS, bx - 1u);
    goto search;
notfound:
    cx = RD(BEST_LEN);
    es = RD(OUT_SEG);
    di = RD(OUT_PTR);
    if (cx <= 2u) {
        goto litrun;
    }
    if (cx <= 0x0Au) {
        ax = (u16)(si - RD(BEST_POS));
        if (ax <= 0xFFFu) {
            /* 00h..7Fh: (length - 3) << 4 | distance >> 8, then the distance's low byte */
            u8 ah = (u8)((ax >> 8) + (u8)(((u8)cx - 3u) << 4));
            ax = (u16)((ah) | ((ax & 0xFFu) << 8));
            goto emitoff;
        }
    }
    if (cx > 0x40u) {                           /* absCopy */
        goto longmatch;
    }
    x86_stosb(es, &di, (u8)(((u8)cx - 3u) | 0xC0u));    /* C0h..FDh: (length - 3), the distance word */
    goto storeoff;
litrun:
    if (RD(LIT_OPEN) != 0) {
        goto putlit;
    }
newlit:
    WR(LIT_HDR, di);                            /* this run's count byte */
    x86_stosb(es, &di, 0x80u);
putlit:
    bx = RD(LIT_HDR);
    if (tc_ld_u8(TC_MKFP(es, bx)) == 0xBFu) {   /* a full 3Fh run starts a new one */
        goto newlit;
    }
    tc_st_u8(TC_MKFP(es, bx), (u8)(tc_ld_u8(TC_MKFP(es, bx)) + 1u));
    x86_stosb(es, &di, x86_lodsb(ds, &si));
    WR(LIT_OPEN, 1);
    goto loopend;
longmatch:
    x86_stosb(es, &di, 0xFFu);                  /* FFh: length word, distance word */
    x86_stosw(es, &di, RD(BEST_LEN));
storeoff:
    ax = (u16)(RD(BEST_POS) - RD(IN_BASE));
emitoff:
    x86_stosw(es, &di, ax);
    si = (u16)(si + RD(BEST_LEN));
    WR(LIT_OPEN, 0);
loopend:
    if (si < bp) {
        goto mainloop;
    }
    x86_stosb(es, &di, 0x80u);                  /* end of stream */
    return (s16)(u16)(di - RD(OUT_BASE));
}
