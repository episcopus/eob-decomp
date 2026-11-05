/* E1D9A.c: src/E1D9A.ASM in C (playbook 80, M3): the four packed-file decoders the loader calls
   (E1B82.C Unpack, by the format byte).

   Written from the TASM source, which stays the reference: each routine follows its labels, with
   16-bit register variables named as the source names them. The scratch the decoders keep in their
   own code segment stays in the arena at its offsets in segment 1D9A (the state digests see it): the
   output limit (cs:0Ch), the packed source segment (cs:10h), Unpack1's code phase (cs:12h, a byte),
   Unpack4's output start, bytes left and end (cs:109h, 10Bh, 10Dh), the string lengths and offset
   (cs:185h, 187h, 189h) and Unpack2's reader state (cs:2C9h, an address in this segment). */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1D9Au
#define W(off) CS_VAR(SEG, (off))
#define RD(off) tc_ld_u16(W(off))
#define WR(off, v) tc_st_u16(W(off), (u16)(v))

/* 1D9A:0013: an LCW stream to len bytes; the bytes written are left in AX (the callers' prototype
   returns nothing). */
/*@ 1D9A:0013 */ void Unpack4(farptr src, farptr dst, s16 len)
{
    farptr p;
    u16 si, ds, di, es, ax, bx, cx, dx, bp;
    u8 al;

    p = x86_norm_far(src);
    si = TC_OFF(p);
    ds = TC_SEG(p);
    p = x86_norm_far(dst);
    di = TC_OFF(p);
    es = TC_SEG(p);
    dx = (u16)len;
    WR(0x109, di);                              /* output start offset */
    dx = (u16)(dx + di);
    WR(0x10D, dx);                              /* output end offset */
    bx = si;
    dx = ds;
    bp = es;
    for (;;) {                                  /* u4_loop */
        ax = (u16)(RD(0x10D) - di);
        if (ax == 0) {
            break;
        }
        WR(0x10B, ax);                          /* bytes left to fill */
        si = bx;
        ds = dx;
        al = x86_lodsb(ds, &si);
        if (!(al & 0x80u)) {
            /* a back reference: (byte >> 4) + 3 bytes from a 12-bit distance behind DI */
            u16 ah = al & 0x0Fu;
            cx = (u16)((al >> 4) + 3u);
            if (cx > RD(0x10B)) {
                cx = RD(0x10B);
            }
            ax = (u16)((ah << 8) | x86_lodsb(ds, &si));
            bx = si;
            si = (u16)(di - ax);
            ds = bp;
            x86_rep_movsb(ds, &si, es, &di, &cx);
            continue;
        }
        if (!(al & 0x40u)) {                    /* u4_control */
            if (al == 0x80u) {
                break;
            }
            cx = al & 0x3Fu;                    /* a literal run */
            if (cx > RD(0x10B)) {
                cx = RD(0x10B);
            }
            x86_rep_movsb(ds, &si, es, &di, &cx);
            bx = si;
            continue;
        }
        cx = (u16)((al & 0x3Fu) + 3u);          /* u4_fill */
        if (al == 0xFEu) {
            cx = x86_lodsw(ds, &si);
            al = x86_lodsb(ds, &si);
            bx = si;
            if (cx > RD(0x10B)) {
                cx = RD(0x10B);
            }
            x86_rep_stosb(es, &di, &cx, al);
            continue;
        }
        if (al == 0xFFu) {                      /* u4_absref */
            cx = x86_lodsw(ds, &si);
        }
        ax = x86_lodsw(ds, &si);                /* u4_havecount */
        bx = si;
        si = (u16)(ax + RD(0x109));
        ds = bp;
        if (cx > RD(0x10B)) {
            cx = RD(0x10B);
        }
        x86_rep_movsb(ds, &si, es, &di, &cx);
    }
}

/* 1D9A:0162 _NormDest: the count about to be emitted off the 32-bit remainder BX:DX, ES:DI
   renormalized */
static void norm_dest(u16 cx, u16 *bx, u16 *dx, u16 *es, u16 *di)
{
    u16 borrow = *bx < cx;
    *bx = (u16)(*bx - cx);
    *dx = (u16)(*dx - borrow);
    *es = (u16)(*es + (*di >> 4));
    *di &= 0x0Fu;
}

/*@ 1D9A:0113 */ void Unpack3(u16 srcSeg, u16 dstSeg)
{
    u16 es = dstSeg, ds = srcSeg, cx = 0, di = 0, si = 2, bx, dx, ax;
    u8 cl;

    bx = tc_ld_u16(TC_MKFP(ds, si));            /* 32-bit output length */
    dx = tc_ld_u16(TC_MKFP(ds, (u16)(si + 2u)));
    si = (u16)(si + 4u);
    ax = x86_lodsw(ds, &si);
    si = (u16)(si + ax);                        /* skip the header word */
    do {                                        /* u3_read */
        cl = tc_ld_u8(TC_MKFP(ds, si));
        si++;
        cx = (u16)((cx & 0xFF00u) | cl);
        if (cl != 0 && !(cl & 0x80u)) {
            norm_dest(cx, &bx, &dx, &es, &di);
            x86_rep_movsb(ds, &si, es, &di, &cx);
            continue;
        }
        if (cl == 0) {                          /* u3_zerocount: a big-endian word */
            u16 w = tc_ld_u16(TC_MKFP(ds, si));
            cx = (u16)((w >> 8) | (w << 8));
            si = (u16)(si + 2u);
        } else {                                /* u3_negcount: neg cl */
            cx = (u16)((cx & 0xFF00u) | (u8)(0u - cl));
        }
        norm_dest(cx, &bx, &dx, &es, &di);      /* u3_emit */
        {
            u8 al = x86_lodsb(ds, &si);
            x86_rep_stosb(es, &di, &cx, al);
        }
    } while ((bx | dx) != 0);                   /* u3_next */
}

/* 1D9A:0299 _ReadCode1: the next 12-bit code; two codes in every three bytes, by the phase byte
   cs:12h */
static u16 read_code1(u16 *si)
{
    u16 ds = RD(0x10), ax;
    u8 phase = (u8)(tc_ld_u8(W(0x12)) + 1u);
    tc_st_u8(W(0x12), phase);
    if (phase & 1u) {                           /* u1r_odd */
        ax = tc_ld_u16(TC_MKFP(ds, *si));
        ax = (u16)((ax >> 8) | (ax << 8));
        ax >>= 4;
        *si = (u16)(*si + 1u);
        return ax;
    }
    ax = x86_lodsw(ds, si);
    ax = (u16)((ax >> 8) | (ax << 8));
    return ax & 0x0FFFu;
}

/*@ 1D9A:018B */ void Unpack1(u16 srcSeg, u16 dstSeg, u16 workSeg)
{
    u16 es = dstSeg, ds = srcSeg, bp, di = 0, si = 2, ax, bx, cx, dx;
    u8 ch;

    bp = workSeg;
    WR(0x0C, x86_lodsw(ds, &si));               /* output limit */
    (void)x86_lodsw(ds, &si);
    ax = x86_lodsw(ds, &si);
    si = (u16)(si + ax);
    WR(0x10, ds);                               /* packed source segment */
    ds = bp;
    tc_st_u8(W(0x12), 0);                       /* code phase counter */
    bp = 0;                                     /* output table offset */
    ax = read_code1(&si);
    dx = di;
    WR(0x185, 1);
    WR(0x189, 1);
    ch = (u8)ax;
    x86_stosb(es, &di, (u8)ax);
    while (di < RD(0x0C)) {                     /* u1_test */
        ax = read_code1(&si);
        if ((ax >> 8) == 0) {                   /* u1_literal */
            x86_stosb(es, &di, (u8)ax);
            ch = (u8)ax;
            if (bp < 0x3C00u) {
                tc_st_u16(TC_MKFP(ds, bp), dx);
                WR(0x185, RD(0x185) + 1u);
                tc_st_u16(TC_MKFP(ds, (u16)(bp + 2u)), RD(0x185));
                bp = (u16)(bp + 4u);
            }
            dx = (u16)(di - 1u);                /* u1_reset */
            WR(0x185, 1);
            WR(0x189, 1);
            continue;
        }
        ax = (u16)(ax - 0x100u);                /* dec ah */
        bx = (u16)(ax << 2);
        if (bx < bp) {                          /* a dictionary string */
            u16 s2 = tc_ld_u16(TC_MKFP(ds, bx));
            cx = tc_ld_u16(TC_MKFP(ds, (u16)(bx + 2u)));
            WR(0x189, cx);
            WR(0x187, di);
            x86_rep_movsb(es, &s2, es, &di, &cx);
            ch = tc_ld_u8(TC_MKFP(es, RD(0x187)));
        } else {                                /* u1_copybytes: the previous string and its first byte */
            u16 s2 = dx;
            u8 al = ch;
            dx = di;
            WR(0x187, dx);
            cx = RD(0x189);
            WR(0x185, cx);
            x86_rep_movsb(es, &s2, es, &di, &cx);
            x86_stosb(es, &di, al);
            ch = al;
            WR(0x189, RD(0x189) + 1u);
        }
        if (bp < 0x3C00u) {                     /* u1_store */
            tc_st_u16(TC_MKFP(ds, bp), dx);
            WR(0x185, RD(0x185) + 1u);
            tc_st_u16(TC_MKFP(ds, (u16)(bp + 2u)), RD(0x185));
            bp = (u16)(bp + 4u);
        }
        dx = RD(0x187);                         /* u1_skip */
        WR(0x185, RD(0x189));
    }
}

/* 1D9A:03CC _ReadCode2: the next 14-bit code. cs:2C9h holds the address of the next of four reader
   states; they cycle 400h, 412h, 3D8h, 3F1h (the labels' offsets in this segment), taking one or two
   bytes in turn. */
static u16 read_code2(u16 *si)
{
    u16 ds = RD(0x10), ax;
    u8 cl, cf;
    switch (RD(0x2C9)) {
    case 0x3D8u:                                /* u2r_word */
        WR(0x2C9, 0x3F1u);
        ax = x86_lodsw(ds, si);
        ax = (u16)((ax >> 8) | (ax << 8));
        cl = tc_ld_u8(TC_MKFP(ds, *si));
        cf = cl >> 7;                           /* shl cl,1 / rcl ax,1, twice */
        cl = (u8)(cl << 1);
        ax = (u16)((ax << 1) | cf);
        cf = cl >> 7;
        ax = (u16)((ax << 1) | cf);
        return ax & 0x3FFFu;
    case 0x3F1u:                                /* u2r_pair */
        WR(0x2C9, 0x400u);
        ax = x86_lodsw(ds, si);
        ax = (u16)((ax >> 8) | (ax << 8));
        return ax & 0x3FFFu;
    case 0x400u:                                /* u2r_low */
        WR(0x2C9, 0x412u);
        ax = x86_lodsb(ds, si);
        ax = (u16)((ax << 8) | tc_ld_u8(TC_MKFP(ds, *si)));
        return (u16)(ax >> 2);
    case 0x412u:                                /* u2r_split */
        WR(0x2C9, 0x3D8u);
        ax = x86_lodsw(ds, si);
        cl = (u8)ax;
        ax = (u16)((ax & 0xFF00u) | tc_ld_u8(TC_MKFP(ds, *si)));
        cf = cl & 1u;                           /* shr cl,1 / rcr ax,1, twice */
        cl >>= 1;
        ax = (u16)((ax >> 1) | (cf << 15));
        cf = cl & 1u;
        ax = (u16)((ax >> 1) | (cf << 15));
        return (u16)(ax >> 2);
    default:
        tc_fail(70, "Unpack2: reader state %04X is none of the four", RD(0x2C9));
    }
}

/*@ 1D9A:02CB */ void Unpack2(u16 srcSeg, u16 dstSeg, u16 workSeg)
{
    u16 es = dstSeg, ds = srcSeg, bp, di = 0, si = 2, ax, bx = 0, cx, dx;
    u8 ch;

    bp = workSeg;
    WR(0x0C, x86_lodsw(ds, &si));               /* output limit */
    (void)x86_lodsw(ds, &si);
    ax = x86_lodsw(ds, &si);
    si = (u16)(si + ax);
    WR(0x10, ds);                               /* packed source segment */
    ds = bp;
    WR(0x2C9, 0x400u);                          /* first reader state */
    bp = 0;
    ax = read_code2(&si);
    dx = di;
    WR(0x185, 1);
    WR(0x189, 1);
    ch = (u8)ax;
    x86_stosb(es, &di, (u8)ax);
    while (di < RD(0x0C)) {                     /* u2_test */
        ax = read_code2(&si);
        if ((ax >> 8) == 0) {                   /* u2_literal */
            x86_stosb(es, &di, (u8)ax);
            ch = (u8)ax;
            if (bp < 0x7E00u) {
                tc_st_u16(TC_MKFP(ds, bp), dx);
                WR(0x185, RD(0x185) + 1u);
                tc_st_u16(TC_MKFP(ds, (u16)(bp + 0x7E00u)), RD(0x185));
                bp = (u16)(bp + 2u);
            }
            dx = (u16)(di - 1u);                /* u2_reset */
            WR(0x185, 1);
            WR(0x189, 1);
            continue;
        }
        ax = (u16)(ax - 0x100u);                /* dec ah */
        bx = (u16)(ax << 1);
        if (bx < bp) {                          /* a dictionary string */
            u16 s2 = tc_ld_u16(TC_MKFP(ds, bx));
            cx = tc_ld_u16(TC_MKFP(ds, (u16)(bx + 0x7E00u)));
            WR(0x189, cx);
            bx = di;
            x86_rep_movsb(es, &s2, es, &di, &cx);
            ch = tc_ld_u8(TC_MKFP(es, bx));
        } else {                                /* u2_copybytes */
            u16 s2 = dx;
            u8 al = ch;
            dx = di;
            bx = dx;
            cx = RD(0x189);
            WR(0x185, cx);
            x86_rep_movsb(es, &s2, es, &di, &cx);
            x86_stosb(es, &di, al);
            ch = al;
            WR(0x189, RD(0x189) + 1u);
        }
        if (bp < 0x7E00u) {                     /* u2_store */
            tc_st_u16(TC_MKFP(ds, bp), dx);
            WR(0x185, RD(0x185) + 1u);
            tc_st_u16(TC_MKFP(ds, (u16)(bp + 0x7E00u)), RD(0x185));
            bp = (u16)(bp + 2u);
        }
        dx = bx;                                /* u2_skip */
        WR(0x185, RD(0x189));
    }
}
