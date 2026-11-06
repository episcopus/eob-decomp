/* E1990.c: src/E1990.ASM in C (playbook 80, M3): the DOS memory chain. CheckMCB merges free blocks
   along the MCB chain; CheckMemoryChain walks the chain from a one-paragraph block it allocates,
   moves the environment block when it finds it and frees that block; ReserveMemoryBelowVideo and
   TrimMemoryToVideo size the program's memory below the probed segment (the Tandy graphics choice).

   Written from the TASM source, which stays the reference: each routine follows its labels, with
   16-bit register variables named as the source names them. The MCBs are read and written in the
   arena exactly where the original reads and writes them (type byte at +0, owner at +1, size at +3),
   so the chain the routines leave is the original's. The unit's code-segment words stay in the arena
   at their offsets in segment 1990 (CS_VAR): gMemTopSeg (cs:02), gVideoTopSeg (cs:04), gMovedSeg
   (cs:06), gMovedParas (cs:08), gNewMemTopSeg (cs:0C), gMcbSeg (cs:0E). DOS is the machine's
   (x86_int: INT 21h 48h and 49h). CheckMCB is near and register-only: a static function. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1990u
#define RD(off) tc_ld_u16(CS_VAR(SEG, (off)))
#define WR(off, v) tc_st_u16(CS_VAR(SEG, (off)), (u16)(v))
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))
#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (off)), (u8)(v))

/* the code-segment words (src/E1990.ASM names) */
#define G_MEM_TOP_SEG 0x0002u
#define G_VIDEO_TOP_SEG 0x0004u
#define G_MOVED_SEG 0x0006u
#define G_MOVED_PARAS 0x0008u
#define G_NEW_MEM_TOP_SEG 0x000Cu
#define G_MCB_SEG 0x000Eu

#define MCB_M 0x4Du
#define MCB_Z 0x5Au

/* INT 21h AH=48h with BX paragraphs: AX (the block's segment, or DOS's error code), and CF */
static u16 dos_alloc(u16 bx, int *cf)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x4800u;
    r.bx = bx;
    x86_int(0x21, &r);
    if (cf != 0) {
        *cf = (r.flags & TC_FLAG_CF) != 0;
    }
    return r.ax;
}

/* INT 21h AH=49h with ES the block's segment (the result is not tested anywhere in the unit) */
static void dos_free(u16 es)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x4900u;
    r.es = es;
    x86_int(0x21, &r);
}

/* `mov cx,8; rep movsw; dec bx; jne` over bx paragraphs, DS:SI to ES:DI with the offsets wrapping in
   their segments; a count of 0 runs 65536 times, as `dec bx; jne` does */
static void copy_paras(u16 ds, u16 si, u16 es, u16 di, u16 bx)
{
    u16 cx;
    do {
        cx = 8;
        x86_rep_movsw(ds, &si, es, &di, &cx);
        bx--;
    } while (bx != 0);
}

/* 1990:0010 CheckMCB: walk the chain from gMcbSeg, merging two neighbours whose owners are both zero;
   AX = 0 when it ends at a 'Z' block, 1 when a block's type is neither 'M' nor 'Z' */
static u16 check_mcb(void)
{
    u16 es, ds, bx, ax;

    es = RD(G_MCB_SEG);
    if (LD8(es, 0) == MCB_Z) {
        return 0;                                   /* ok */
    }
    for (;;) {                                      /* again */
        bx = LD16(es, 1);
        ds = es;
        ax = (u16)(es + LD16(es, 3) + 1u);
        es = ax;
        if (LD8(es, 0) != MCB_M && LD8(es, 0) != MCB_Z) {
            return 1;                               /* bad */
        }
        bx |= LD16(es, 1);                          /* owner */
        if (bx == 0) {
            ax = (u16)(LD16(es, 3) + 1u);
            ST16(ds, 3, LD16(ds, 3) + ax);
            ST8(ds, 0, LD8(es, 0));
            es = ds;
        }
        if (LD8(es, 0) == MCB_Z) {                  /* next */
            return 0;
        }
    }
}

/*@ 1990:0071 */ s16 CheckMemoryChain(void)
{
    u16 ax, bx, es, ds, mcb;
    int cf;

    ax = dos_alloc(1, &cf);
    if (cf) {
        return 1;                                   /* allocFail: nothing to free */
    }
    ax--;                                           /* allocOk */
    es = ax;
    WR(G_MCB_SEG, ax);
    ds = LD16(es, 1);                               /* the owner: the PSP */
    bx = (u16)(LD16(ds, 0x2C) - 1u);                /* the environment block's MCB */
    for (;;) {                                      /* walk */
        es = (u16)(es + LD16(es, 3) + 1u);
        if (LD8(es, 0) != MCB_M && LD8(es, 0) != MCB_Z) {
            ax = 1;                                 /* chainError */
            goto freeTiny;
        }
        if (LD16(es, 1) != 0 && ds == LD16(es, 1) && bx == es) {     /* checkOwner */
            bx = (u16)(LD16(es, 3) + 1u);
            WR(G_MOVED_PARAS, bx);
            bx--;
            ax = dos_alloc(bx, &cf);
            if (cf) {
                ax = 1;                             /* chainError */
                goto freeTiny;
            }
            ST16(ds, 0x2C, ax);
            WR(G_MOVED_SEG, ax);
            mcb = es;                               /* push ds; push es; ds = the block's MCB, es = the copy */
            copy_paras(mcb, 0x10, ax, 0, LD16(mcb, 3));
            bx = 0;                                 /* the copy's count ends at 0, and BX is the walk's
                                                       comparison from here on: no later block matches */
            ST16(es, 1, 0);                         /* the old block is free */
        }
        if (LD8(es, 0) == MCB_Z) {                  /* nextBlock */
            break;
        }
    }
    ax = check_mcb();
freeTiny:
    dos_free((u16)(RD(G_MCB_SEG) + 1u));
    return (s16)ax;
}

/*@ 1990:0133 */ u16 ReserveMemoryBelowVideo(s16 steps)
{
    u16 ax, bx, es, ds, tiny;
    u8 bl, bh;

    WR(G_VIDEO_TOP_SEG, 0x9800u);                   /* overwritten before use (1990:01B6) */
    bl = LD8(0xB800u, 0);
    ST8(0xB800u, 0, 0xC3u);
    es = 0xA000u;
    for (;;) {                                      /* probeDown */
        ax = (u16)(es - 0x400u);
        if (ax <= 0x3C00u) {
            ST8(0xB800u, 0, bl);                    /* probeDone */
            es = 0xA000u;
            break;
        }
        es = ax;
        bh = LD8(es, 0);
        if (bh != 0xC3u) {
            continue;
        }
        ST8(0xB800u, 0, 0x3Cu);
        bh = LD8(es, 0);
        ST8(0xB800u, 0, 0xC3u);
        if (bh != 0x3Cu) {
            continue;
        }
        ST8(es, 0, bl);
        break;                                      /* probeEnd */
    }
    bx = es;
    ax = (u16)(steps - 1);
    ax = (u16)(((ax & 0xFFu) << 8) | (ax >> 8));    /* xchg al,ah */
    ax = (u16)(ax << 3);                            /* shl ax,1 three times */
    bx = (u16)(bx - ax);
    WR(G_VIDEO_TOP_SEG, bx);
    ax = dos_alloc(1, 0);                           /* the carry is not tested */
    tiny = ax;                                      /* push ax */
    ax--;
    es = ax;
    if (LD8(es, 0) != MCB_M) {
        /* QUIRK 1990:01CE: the original jumps to retTop with the pushed word still on the stack, so its
           pops and its far return take the wrong words; there is no defined result to give */
        tc_fail(75, "ReserveMemoryBelowVideo (1990:01CE): the block at %04X:0000 is not 'M'; the original "
                    "returns through the wrong stack words",
                es);
    }
    do {                                            /* chainHead, lastBlock */
        es = (u16)(LD16(es, 3) + 1u + es);
    } while (LD8(es, 0) != MCB_Z);
    ax = (u16)(es + LD16(es, 3) + 1u);
    WR(G_MEM_TOP_SEG, ax);
    bx = (u16)(RD(G_VIDEO_TOP_SEG) - RD(G_MOVED_PARAS));
    if (ax <= bx) {
        WR(G_VIDEO_TOP_SEG, ax + RD(G_MOVED_PARAS));
    }
    bx = (u16)(es + 1u);                            /* noGrow */
    ax = (u16)(RD(G_VIDEO_TOP_SEG) - RD(G_MOVED_PARAS));
    WR(G_NEW_MEM_TOP_SEG, ax);
    ax = (u16)(ax - bx);
    ST16(es, 3, ax);
    bx = (u16)(es + ax + 1u);
    dos_free(tiny);                                 /* pop es; INT 21h 49h */
    ax = RD(G_MOVED_SEG);
    if (ax != 0) {
        ax--;
        ds = ax;
        es = bx;
        copy_paras(ds, 0, es, 0, (u16)(LD16(ds, 3) + 1u));
        ST8(es, 0, MCB_Z);
        ax = LD16(es, 1);
        bx = (u16)(es + 1u);
        ds = ax;
        ST16(ds, 0x2C, bx);
        ST16(ds, 2, RD(G_NEW_MEM_TOP_SEG));
        dos_free(RD(G_MOVED_SEG));
        (void)check_mcb();                          /* its result is overwritten below */
    }
    return RD(G_VIDEO_TOP_SEG);                     /* retTop */
}

/*@ 1990:027D */ void TrimMemoryToVideo(void)
{
    u16 ax, bx, es, tiny;

    ax = dos_alloc(1, 0);                           /* the carry is not tested */
    tiny = ax;                                      /* push ax */
    ax--;
    es = ax;
    if (LD8(es, 0) != MCB_M) {
        /* QUIRK 1990:0296: as ReserveMemoryBelowVideo's, the exit pops one word out */
        tc_fail(75, "TrimMemoryToVideo (1990:0296): the block at %04X:0000 is not 'M'; the original "
                    "returns through the wrong stack words",
                es);
    }
    do {                                            /* trimLast, walkLast */
        es = (u16)(LD16(es, 3) + 1u + es);
    } while (LD8(es, 0) != MCB_Z);
    bx = (u16)(es + 1u);
    ax = (u16)(RD(G_MEM_TOP_SEG) - bx);
    ST16(es, 3, ax);
    dos_free(tiny);                                 /* pop es; INT 21h 49h */
}
