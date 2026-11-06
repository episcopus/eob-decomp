/* mcga_mask.c: MCGA.OVL in C, the masked and the remapped shape draws (playbook 80, M4;
   src/drivers/MCGA.ASM, mcga.h).

   DrawShapeMasked (12F0) and its mirror DrawShapeMaskedFlipped (1006) draw an 8-bit shape (a header of
   four bytes, byte 1 its rows and byte 2 its width in 8-pixel columns; then its pixels, a 0 byte
   followed by a count being a run of transparent pixels) behind the scenery planes of page 4.
   DrawShapeRemapped (279C) and its mirror DrawShapeFlippedRemapped (298A) draw a 4-bit shape (byte 0
   its format, 0 drawing nothing; bytes 1 and 2 as above; its pixels from byte 20, two a byte, the high
   nibble first) through gRemapTable (cs:030C), colour 0 transparent.

   Each routine follows its labels in the source, with its registers as u16 locals named as the source
   names them, so the C reads beside the TASM line by line. The conditional jumps keep their exact
   flags: `jg` and `jle` after an add or a sub test the true signed result (SF = OF, ZF), `js` and
   `jns` the sign of the 16-bit result, `jae` an unsigned compare. Every routine saves the registers it
   uses (but see the format 0 notes of the remapped draws), so memory, its cs: variables included, is
   its only effect. */
#include "mcga.h"

#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (u16)(off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (u16)(off)), (u8)(v))

/* `add a, b` then `jg` (the true sum above 0; `jle` is its negation) */
static int add_gt0(u16 a, u16 b) { return (long)(s16)a + (long)(s16)b > 0; }
/* `sub a, b` then `jg`: a above b, signed */
static int sub_gt0(u16 a, u16 b) { return (long)(s16)a - (long)(s16)b > 0; }

/* ---- the masked draws' common parts ----------------------------------------------------------- */

/* The run skip of the masked draws (10C3, 119D, 12C2; 13AD, 1484, 15AC): passes cx pixels of a format
   0 shape at seg:*di. `xor ax,ax` sets ZF and `repne scasb` scans for the next 0 byte, each byte before
   it a pixel; with no 0 byte in cx bytes, cx is 0 and `jcxz` ends the skip (with cx 0 at the start,
   ZF stays set and the byte at di is taken as a run's length). Otherwise the byte after the 0 is the
   run's length: `inc cx` gives back the 0 byte's count, `sub cx, ax` takes the run. The skips of the
   rows above the region and of a row's end loop with `jne` until cx is exactly 0 (until_gt0 0); the
   one of a row's start loops with `jg` while cx stays above 0 (until_gt0 1). Returns cx. */
static u16 skip_pixels(u16 seg, u16 *di, u16 cx, int until_gt0)
{
    for (;;) {
        u16 ax;
        int more;
        if (!x86_rep_scasb(0, seg, di, &cx, 0, 1)) {
            return cx;                          /* jcxz */
        }
        ax = LD8(seg, *di);
        *di = (u16)(*di + 1u);
        cx = (u16)(cx + 1u);
        more = until_gt0 ? sub_gt0(cx, ax) : cx != ax;
        cx = (u16)(cx - ax);
        if (!more) {
            return cx;
        }
    }
}

/* The cover test of one pixel (11C9 to 1286; 14B0 to 156F): gMaskBit = 1 << (di & 7) for the pixel's
   bit and di >> 3 for its byte in each plane of page 4 (gMaskPlaneSeg); bl ORs the bit of planes 1 to
   6 that are above gMaskLevel (`cmp gMaskLevel, k; jge` passes plane k when the level, signed, is k or
   more) and always that of plane 7. Plane 0 (the first 8000 bytes) is never read. Returns bl. */
static u8 covered(u16 di)
{
    u16 es = cs16(G_MASK_PLANE_SEG), m;
    u8 bl = 0;
    int k;
    set_cs8(G_MASK_BIT, (u8)(1u << (di & 7u)));
    m = (u16)(di >> 3);
    for (k = 1; k <= 6; k++) {
        if ((s16)cs16(G_MASK_LEVEL) < k) {
            bl |= (u8)(LD8(es, m + (u16)k * MASK_PLANE) & cs8(G_MASK_BIT));
        }
    }
    bl |= (u8)(LD8(es, m + 7u * MASK_PLANE) & cs8(G_MASK_BIT));
    return bl;
}

/* One shape pixel al at es:di (1287 to 12A0; 1570 to 1588): page 2's pixel (gBackPageSeg) where the
   scenery covers it, else al */
static void masked_pixel(u16 es, u16 di, u8 al)
{
    if (covered(di) != 0) {
        u8 bl = LD8(cs16(G_BACK_PAGE_SEG), di);
        ST8(es, di, bl);
    } else {
        ST8(es, di, al);
    }
}

/* The masked draws' start and clip (1006 to 115B; 12F0 to 1445), which the two share instruction for
   instruction: the pages, the header, the clip to the region in lines, then in pixels. Sets *es (the
   page), *si (the first pixel byte of the first row drawn) and *di (the region's left edge, the page's
   offset before the row) and returns 0 where the source jumps to its end. */
static int masked_clip(const u8 *arg, u16 *es_out, u16 *si_out, u16 *di_out)
{
    u16 ax, bx, cx, dx, si, di, es;

    set_cs16(G_MASK_LEVEL, arg_word(arg, 0x10));
    set_cs16(G_MASK_PLANE_SEG, mcga_PageSeg(4));
    set_cs16(G_BACK_PAGE_SEG, mcga_PageSeg(2));
    di = 0;
    es = mcga_PageSeg(arg_word(arg, 6));
    si = arg_word(arg, 8);
    set_cs16(G_SHAPE_SEG, arg_word(arg, 0x0A));
    bx = arg_word(arg, 0x0C);
    cx = arg_word(arg, 0x0E);
    set_cs16(G_SHAPE_Y, cx);
    ax = (u16)(cs16(G_REGION_LEFT) << 3);
    di = (u16)(di + ax);
    bx = (u16)(bx - ax);
    set_cs16(G_SHAPE_X, bx);
    set_cs16(G_REGION_COLS, (u16)(cs16(G_REGION_RIGHT) - cs16(G_REGION_LEFT)));

    /* the header: `inc si; lodsw; inc si`, al the rows, ah the width in columns */
    si = (u16)(si + 1u);
    ax = x86_lodsw(cs16(G_SHAPE_SEG), &si);
    si = (u16)(si + 1u);
    set_cs16(G_SHAPE_ROWS, (u16)(ax & 0xFFu));
    ax = (u16)((u16)(ax >> 8) << 3);
    set_cs16(G_SHAPE_WIDTH, ax);
    set_cs16(G_SHAPE_FULL_WIDTH, ax);

    /* the top: above the region, the rows left are rows + (y - top); the rows above are passed run by
       run, gShapeWidth pixels each (gSkipLeft holds the count) */
    ax = (u16)(cs16(G_SHAPE_Y) - cs16(G_REGION_TOP));
    if ((s16)ax < 0) {
        u16 pushed_di = di;
        int gt = add_gt0(cs16(G_SHAPE_ROWS), ax);
        set_cs16(G_SHAPE_ROWS, (u16)(cs16(G_SHAPE_ROWS) + ax));
        if (!gt) {
            return 0;
        }
        ax = (u16)-ax;
        dx = ax;
        set_cs16(G_SKIP_LEFT, cs16(G_SHAPE_WIDTH));
        di = si;
        do {
            skip_pixels(cs16(G_SHAPE_SEG), &di, cs16(G_SKIP_LEFT), 0);
        } while (--dx != 0);
        si = di;
        di = pushed_di;
        set_cs16(G_SHAPE_Y, cs16(G_REGION_TOP));
    }

    /* the bottom */
    ax = (u16)(cs16(G_REGION_BOTTOM) - cs16(G_SHAPE_Y));
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < cs16(G_SHAPE_ROWS)) {
        set_cs16(G_SHAPE_ROWS, ax);
    }

    /* the left: gSkipLeft pixels of each row are off the region */
    set_cs16(G_SKIP_LEFT, 0);
    ax = cs16(G_SHAPE_X);
    if ((s16)ax < 0) {
        set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) + ax));
        ax = (u16)-ax;
        set_cs16(G_SKIP_LEFT, ax);
        if (ax >= cs16(G_SHAPE_FULL_WIDTH)) {
            return 0;
        }
        ax = 0;
        set_cs16(G_SHAPE_X, ax);
    }

    /* the right: gSkipRight pixels */
    set_cs16(G_SKIP_RIGHT, 0);
    ax = (u16)((u16)(cs16(G_REGION_COLS) << 3) - cs16(G_SHAPE_X));
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < cs16(G_SHAPE_WIDTH)) {
        set_cs16(G_SHAPE_WIDTH, ax);
        set_cs16(G_SKIP_RIGHT,
                 (u16)(cs16(G_SHAPE_FULL_WIDTH) - cs16(G_SKIP_LEFT) - cs16(G_SHAPE_WIDTH)));
    }
    *es_out = es;
    *si_out = si;
    *di_out = di;
    return 1;
}

/* ---- the masked draws ---------------------------------------------------------------------------- */

/* MCGA.OVL 12F0 (export 21, gDrawShapeMasked): DrawShapeMasked(page, void far *shape, x, y, depth)
   draws a format 0 shape clipped to the region, behind the scenery: page 4 holds bit planes of 8000
   bytes, and where a plane above depth (or plane 7) has the pixel's bit, page 2's pixel is drawn
   instead of the shape's. Each row: gSkipLeft pixels passed, gVisibleWidth drawn left to right (a
   transparent run moves di on), gSkipRight passed. QUIRK 151C: the `jmp short L_156F` after an
   unconditional jump is dead and needs no C. */
static u32 DrawShapeMasked(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx, dx, si, di, es, ds, bp;
    (void)r;
    if (!masked_clip(arg, &es, &si, &di)) {
        return 0;
    }
    /* L_1448 */
    ax = (u16)(row_offset(cs16(G_SHAPE_Y)) + cs16(G_SHAPE_X));
    di = (u16)(di + ax);
    bp = di;
    set_cs16(G_VISIBLE_WIDTH, cs16(G_SHAPE_WIDTH));
    dx = cs16(G_SHAPE_ROWS);
    ds = cs16(G_SHAPE_SEG);
    bx = es;
    do {
        /* L_1475: the pixels left of the region; cx ends at 0, or below it inside a transparent run */
        cx = cs16(G_SKIP_LEFT);
        if (cx != 0) {
            cx = skip_pixels(ds, &si, cx, 1);
        }
        /* L_149F */
        di = (u16)(di - cx);
        if (add_gt0(cx, cs16(G_VISIBLE_WIDTH))) {
            cx = (u16)(cx + cs16(G_VISIBLE_WIDTH));
            for (;;) {
                /* L_14A8 */
                ax = x86_lodsb(ds, &si);
                if (ax != 0) {
                    masked_pixel(bx, di, (u8)ax);
                    di = (u16)(di + 1u);
                    if (--cx == 0) {            /* loop L_154C */
                        break;
                    }
                } else {
                    /* L_1596: a transparent run */
                    int gt;
                    ax = x86_lodsb(ds, &si);
                    di = (u16)(di + ax);
                    gt = sub_gt0(cx, ax);
                    cx = (u16)(cx - ax);
                    if (!gt) {
                        break;
                    }
                }
            }
        } else {
            cx = (u16)(cx + cs16(G_VISIBLE_WIDTH));
        }
        /* L_159D: the pixels right of the region, from where the row's draw stopped */
        cx = (u16)(cx + cs16(G_SKIP_RIGHT));
        if (cx != 0) {
            skip_pixels(ds, &si, cx, 0);
        }
        /* L_15C7 */
        bp = (u16)(bp + SCREEN_WIDTH);
        di = bp;
    } while (--dx != 0);
    return 0;
}

/* MCGA.OVL 1006 (export 20, gDrawShapeMaskedFlipped): DrawShapeMasked mirrored left to right. The clip
   is DrawShapeMasked's; each row starts at its rightmost pixel (di + gVisibleWidth - 1) and is drawn
   right to left: gSkipRight pixels of the shape passed first (they fall right of the region), then
   gVisibleWidth drawn, then gSkipLeft passed. */
static u32 DrawShapeMaskedFlipped(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx, dx, si, di, es, ds, bp;
    (void)r;
    if (!masked_clip(arg, &es, &si, &di)) {
        return 0;
    }
    /* L_115E */
    ax = (u16)(row_offset(cs16(G_SHAPE_Y)) + cs16(G_SHAPE_X));
    di = (u16)(di + ax);
    ax = cs16(G_SHAPE_WIDTH);
    set_cs16(G_VISIBLE_WIDTH, ax);
    di = (u16)(di + ax - 1u);
    bp = di;
    dx = cs16(G_SHAPE_ROWS);
    ds = cs16(G_SHAPE_SEG);
    bx = es;
    do {
        /* L_118E: the pixels right of the region */
        cx = cs16(G_SKIP_RIGHT);
        if (cx != 0) {
            cx = skip_pixels(ds, &si, cx, 1);
        }
        /* L_11B8 */
        di = (u16)(di + cx);
        if (add_gt0(cx, cs16(G_VISIBLE_WIDTH))) {
            cx = (u16)(cx + cs16(G_VISIBLE_WIDTH));
            for (;;) {
                /* L_11C1 */
                ax = x86_lodsb(ds, &si);
                if (ax != 0) {
                    masked_pixel(bx, di, (u8)ax);
                    di = (u16)(di - 1u);
                    if (--cx == 0) {            /* loop L_1239 */
                        break;
                    }
                } else {
                    /* L_12AC: a transparent run */
                    int gt;
                    ax = x86_lodsb(ds, &si);
                    di = (u16)(di - ax);
                    gt = sub_gt0(cx, ax);
                    cx = (u16)(cx - ax);
                    if (!gt) {
                        break;
                    }
                }
            }
        } else {
            cx = (u16)(cx + cs16(G_VISIBLE_WIDTH));
        }
        /* L_12B3: the pixels left of the region */
        cx = (u16)(cx + cs16(G_SKIP_LEFT));
        if (cx != 0) {
            skip_pixels(ds, &si, cx, 0);
        }
        /* L_12DD */
        bp = (u16)(bp + SCREEN_WIDTH);
        di = bp;
    } while (--dx != 0);
    return 0;
}

/* ---- the remapped draws -------------------------------------------------------------------------- */

/* the colour of nibble n: `xlat byte ptr cs:[bx]` with bx = gRemapTable */
static u8 remap(u8 n) { return cs8((u16)(G_REMAP_TABLE + n)); }

/* MCGA.OVL 279C (export 36, gDrawShapeRemapped): DrawShapeRemapped(page, void far *shape, x, y) draws
   a 4-bit shape clipped to the region through gRemapTable. Its state is in locals of its stack frame
   ([bp-2] to [bp-18h], named below), not in the driver's variables. Where the region's left edge cuts
   the shape at an odd x, each row starts with a byte's low nibble ([bp-6] and [bp-2]); where its right
   edge does, each row ends with a byte's high nibble ([bp-4]). A format 0 shape returns at once
   through L_27EF, which pops the saved registers one word off (the `push ds` before the format's lodsb
   is still on the stack): SI comes back as the caller's DS, DI as SI, ES as DI, DS as ES, DX as DS, CX
   as DX, BX as CX and AX as BX; `mov sp, bp` unwinds the frame, so it returns to its caller with DS
   and the register variables of a C caller changed. The port cannot give a caller those registers,
   so the C stops there with the reason, as the port does at the original's other broken exits.
   QUIRK 2988: `pop bp; retf` after an
   unconditional jump is dead and needs no C. */
static u32 DrawShapeRemapped(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx, dx, si, di, es, ds;
    u16 shape_seg, y, x, cols, rows, width, full, left, right, odd, dst_skip, used;
    (void)r;

    es = mcga_PageSeg(arg_word(arg, 6));
    si = arg_word(arg, 8);
    shape_seg = arg_word(arg, 0x0A);            /* [bp-14h] */
    bx = arg_word(arg, 0x0C);
    y = arg_word(arg, 0x0E);                    /* [bp-12h] */
    ax = (u16)(cs16(G_REGION_LEFT) << 3);
    di = ax;
    bx = (u16)(bx - ax);
    x = bx;                                     /* [bp-10h] */
    cols = (u16)(cs16(G_REGION_RIGHT) - cs16(G_REGION_LEFT));   /* [bp-0Eh] */
    if (x86_lodsb(shape_seg, &si) == 0) {
        tc_fail(75, "DrawShapeRemapped (MCGA.OVL 27EF): a format 0 shape; the original pops its saved "
                "registers one word off and returns with the caller's DS, SI and DI changed");
    }
    /* L_27FB: the header; the pixels from byte 20 */
    ax = x86_lodsw(shape_seg, &si);
    si = (u16)(si + 0x11u);
    rows = (u16)(ax & 0xFFu);                   /* [bp-0Ch] */
    ax = (u16)((u16)(ax >> 8) << 3);
    width = ax;                                 /* [bp-0Ah] */
    full = ax;                                  /* [bp-8] */

    /* the top: the rows above the region passed, width / 2 bytes each */
    ax = (u16)(y - cs16(G_REGION_TOP));
    if ((s16)ax < 0) {
        ax = (u16)-ax;
        if (ax >= rows) {
            return 0;                           /* jae L_2882, jae L_285F */
        }
        dx = (u16)(width >> 1);
        si = (u16)(si + (u16)((u32)ax * dx));   /* mul dx: the low word */
        ax = (u16)(rows + y - cs16(G_REGION_TOP));
        if ((s16)ax < 0) {
            return 0;                           /* js L_2851, js L_285F */
        }
        rows = ax;
        y = cs16(G_REGION_TOP);
    }
    /* L_284A: the bottom */
    ax = (u16)(cs16(G_REGION_BOTTOM) - y);
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < rows) {
        rows = ax;
    }

    /* L_2861: the left */
    left = 0;                                   /* [bp-6] */
    ax = x;
    odd = (u16)(ax & 1u);                       /* [bp-2] */
    if ((s16)ax < 0) {
        width = (u16)(width + ax);
        ax = (u16)-ax;
        dx = (u16)(ax >> 1);
        si = (u16)(si + dx);
        if (ax >= full) {
            return 0;                           /* jae L_285F at L_2882 */
        }
        ax = 0;
        x = ax;
        left++;
    }
    /* L_288C: the right */
    right = 0;                                  /* [bp-4] */
    ax = (u16)(cols << 3);
    dx = ax;
    ax = (u16)(ax - x);
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < width) {
        dx = (u16)(dx - x);
        width = dx;
        right++;
    }

    /* L_28B3: the first pixel, the skips between rows; width and full become bytes */
    ax = (u16)(row_offset(y) + x);
    di = (u16)(di + ax);
    dst_skip = (u16)(SCREEN_WIDTH - width);     /* [bp-12h] */
    dx = shape_seg;
    width = (u16)(width >> 1);
    full = (u16)(full >> 1);
    used = width;                               /* [bp-18h]: the bytes a row reads */
    if (left != 0 && (odd & 1u) != 0) {
        used++;
        full++;
        if (right == 0) {
            dst_skip = (u16)(dst_skip + 2u);    /* then L_28FD finds [bp-4] 0 */
        } else {
            used++;                             /* L_2903, odd */
        }
    } else if (right != 0 && (odd & 1u) != 0) { /* L_28FD, L_2903 */
        used++;
    }

    do {
        /* L_290D */
        cx = width;
        ds = dx;
        if (left != 0 && odd != 0) {
            u8 al;
            if (cx == 0) {
                return 0;                       /* jcxz L_2970 */
            }
            al = x86_lodsb(ds, &si);            /* the first byte's low nibble only (L_2939) */
            al = remap((u8)(al & 0x0Fu));
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2983 */
            }
            cx--;                               /* loop L_2925 */
        }
        /* L_2925: two pixels a byte, a colour 0 passed */
        for (; cx != 0; cx--) {
            u8 ah = x86_lodsb(ds, &si), al;
            al = remap((u8)(ah >> 4));
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2973 */
            }
            al = remap((u8)(ah & 0x0Fu));       /* L_2939 */
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2983 */
            }
        }
        /* L_2944: the last byte's high nibble only */
        if (right != 0 && odd != 0) {
            u8 al = x86_lodsb(ds, &si);
            al = remap((u8)(al >> 4));          /* `mov cl, 4; shr al, cl` (cx is reloaded) */
            if (al != 0) {
                ST8(es, di, al);
            }
            di = (u16)(di + 1u);
        }
        /* L_295F: the next row */
        si = (u16)(si + full - used);
        di = (u16)(di + dst_skip);
    } while (--rows != 0);
    return 0;
}

/* MCGA.OVL 298A (export 37, gDrawShapeFlippedRemapped): DrawShapeRemapped mirrored left to right: each
   row is read from its last byte down, each byte's nibbles swapped through gNibbleSwap (cs:00F6). Its
   state is the driver's variables: gSkipLeft and gSkipRight are flags (that edge cuts the shape),
   gOddX the x's low bit, gShapeY becomes the screen's skip between rows, gShapeWidth and
   gShapeFullWidth bytes. A format 0 shape jumps to L_29F3 with the `push ds` before the format's lodsb
   still on the stack: its nine pops take the words one off, and `retf` returns to the caller's saved
   BP as offset and to the return offset as segment, out of the driver. The C stops there with the
   reason (the original does not come back). */
static u32 DrawShapeFlippedRemapped(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx, dx, si, di, es, ds;
    (void)r;

    set_cs16(G_SKIP_LEFT, 0);
    es = mcga_PageSeg(arg_word(arg, 6));
    set_cs16(G_DST_SEG, es);
    si = arg_word(arg, 8);
    set_cs16(G_SHAPE_SEG, arg_word(arg, 0x0A));
    bx = arg_word(arg, 0x0C);
    cx = arg_word(arg, 0x0E);
    set_cs16(G_SHAPE_Y, cx);
    ax = (u16)(cs16(G_REGION_LEFT) << 3);
    di = ax;
    set_cs16(G_SHAPE_UNUSED, 0);
    bx = (u16)(bx - ax);
    set_cs16(G_SHAPE_X, bx);
    set_cs16(G_REGION_COLS, (u16)(cs16(G_REGION_RIGHT) - cs16(G_REGION_LEFT)));
    if (x86_lodsb(cs16(G_SHAPE_SEG), &si) == 0) {
        tc_fail(75, "DrawShapeFlippedRemapped (MCGA.OVL 29F3): a format 0 shape; the original pops one "
                "word too many and its retf leaves the driver");
    }
    /* L_29FD: the header; the pixels from byte 20 */
    ax = x86_lodsw(cs16(G_SHAPE_SEG), &si);
    si = (u16)(si + 0x11u);
    set_cs16(G_SHAPE_ROWS, (u16)(ax & 0xFFu));
    ax = (u16)((u16)(ax >> 8) << 3);
    set_cs16(G_SHAPE_WIDTH, ax);
    set_cs16(G_SHAPE_FULL_WIDTH, ax);

    /* the top */
    ax = (u16)(cs16(G_SHAPE_Y) - cs16(G_REGION_TOP));
    if ((s16)ax < 0) {
        ax = (u16)-ax;
        if (ax >= cs16(G_SHAPE_ROWS)) {
            return 0;                           /* jae L_2A9B, jae L_2A73 */
        }
        dx = (u16)(cs16(G_SHAPE_WIDTH) >> 1);
        si = (u16)(si + (u16)((u32)ax * dx));   /* mul dx: the low word */
        ax = (u16)(cs16(G_SHAPE_ROWS) + cs16(G_SHAPE_Y) - cs16(G_REGION_TOP));
        if ((s16)ax < 0) {
            return 0;
        }
        set_cs16(G_SHAPE_ROWS, ax);
        set_cs16(G_SHAPE_Y, cs16(G_REGION_TOP));
    }
    /* L_2A59: the bottom */
    ax = (u16)(cs16(G_REGION_BOTTOM) - cs16(G_SHAPE_Y));
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < cs16(G_SHAPE_ROWS)) {
        set_cs16(G_SHAPE_ROWS, ax);
    }

    /* L_2A76: the left (the shape's last pixels: where the row starts does not move) */
    set_cs16(G_SKIP_LEFT, 0);
    ax = cs16(G_SHAPE_X);
    set_cs16(G_ODD_X, (u16)(ax & 1u));
    if ((s16)ax < 0) {
        set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) + ax));
        ax = (u16)-ax;
        if (ax >= cs16(G_SHAPE_FULL_WIDTH)) {
            return 0;
        }
        ax = 0;
        set_cs16(G_SHAPE_X, ax);
        set_cs16(G_SKIP_LEFT, (u16)(cs16(G_SKIP_LEFT) + 1u));
    }
    /* L_2AA8: the right (the shape's first pixels: si moves on by the bytes cut) */
    set_cs16(G_SKIP_RIGHT, 0);
    ax = (u16)((u16)(cs16(G_REGION_COLS) << 3) - cs16(G_SHAPE_X));
    if ((s16)ax <= 0) {
        return 0;
    }
    if (ax < cs16(G_SHAPE_WIDTH)) {
        dx = cs16(G_SHAPE_WIDTH);
        set_cs16(G_SHAPE_WIDTH, ax);
        dx = (u16)(dx >> 1);
        ax = (u16)(ax >> 1);
        si = (u16)(si + dx - ax);
        set_cs16(G_SKIP_RIGHT, (u16)(cs16(G_SKIP_RIGHT) + 1u));
    }

    /* L_2AE1: the first pixel; the skips; si one byte before the row, which is read backwards */
    ax = (u16)(row_offset(cs16(G_SHAPE_Y)) + cs16(G_SHAPE_X));
    di = (u16)(di + ax);
    set_cs16(G_SHAPE_Y, (u16)(SCREEN_WIDTH - cs16(G_SHAPE_WIDTH)));
    set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) >> 1));
    set_cs16(G_SHAPE_FULL_WIDTH, (u16)(cs16(G_SHAPE_FULL_WIDTH) >> 1));
    si = (u16)(si - 1u);
    ax = cs16(G_SHAPE_WIDTH);
    set_cs16(G_ROW_BYTES, ax);
    set_cs16(G_SRC_ROW_START, ax);
    if (cs16(G_SKIP_LEFT) != 0 && (cs16(G_ODD_X) & 1u) != 0) {
        set_cs16(G_SHAPE_FULL_WIDTH, (u16)(cs16(G_SHAPE_FULL_WIDTH) - 1u));
        set_cs16(G_SHAPE_Y, (u16)(cs16(G_SHAPE_Y) + 2u));
        set_cs16(G_SRC_ROW_START, (u16)(cs16(G_SRC_ROW_START) + 1u));
        if (cs16(G_SKIP_RIGHT) == 1u) {
            set_cs16(G_SHAPE_Y, (u16)(cs16(G_SHAPE_Y) - 2u));   /* then on to L_2B49 */
        }
    }
    /* L_2B49 (the jne L_2B60 above passes it only with gSkipRight 0, where it does nothing) */
    if (cs16(G_SKIP_RIGHT) != 0 && (cs16(G_ODD_X) & 1u) != 0) {
        set_cs16(G_SRC_ROW_START, (u16)(cs16(G_SRC_ROW_START) + 1u));
        si = (u16)(si - 1u);
    }
    /* L_2B60 */
    set_cs16(G_SRC_ROW_SKIP, cs16(G_SHAPE_FULL_WIDTH));
    set_cs16(G_DST_ROW_SKIP, cs16(G_SHAPE_Y));
    set_cs16(G_ROWS_LEFT, cs16(G_SHAPE_ROWS));
    es = cs16(G_DST_SEG);
    ds = cs16(G_SHAPE_SEG);
    dx = 0;                                     /* dl the byte, dh 0: cs:[dx + gNibbleSwap] */

    for (;;) {
        /* L_2B87 */
        cx = cs16(G_ROW_BYTES);
        si = (u16)(si + cs16(G_SRC_ROW_START));
        if (cs16(G_SKIP_LEFT) != 0 && cs16(G_ODD_X) != 0) {
            u8 al;
            if (cx == 0) {
                return 0;                       /* jcxz L_2BB1 */
            }
            dx = (u16)((dx & 0xFF00u) | LD8(ds, si));
            si = (u16)(si - 1u);
            al = cs8((u16)(G_NIBBLE_SWAP + dx));
            al = remap((u8)(al & 0x0Fu));       /* L_2BD7: the swapped byte's low nibble only */
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2BF4 */
            }
            cx--;                               /* loop L_2BB6 */
        }
        /* L_2BB6: two pixels a byte, a colour 0 passed */
        for (; cx != 0; cx--) {
            u8 ah, al = LD8(ds, si);
            si = (u16)(si - 1u);
            dx = (u16)((dx & 0xFF00u) | al);
            ah = cs8((u16)(G_NIBBLE_SWAP + dx));
            al = remap((u8)(ah >> 4));
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2BE4 */
            }
            al = remap((u8)(ah & 0x0Fu));       /* L_2BD7 */
            if (al != 0) {
                x86_stosb(es, &di, al);
            } else {
                di = (u16)(di + 1u);            /* L_2BF4 */
            }
        }
        /* L_2BF7: the swapped byte's high nibble only */
        if (cs16(G_SKIP_RIGHT) != 0 && cs16(G_ODD_X) != 0) {
            u8 al;
            dx = (u16)((dx & 0xFF00u) | LD8(ds, si));
            si = (u16)(si - 1u);
            al = cs8((u16)(G_NIBBLE_SWAP + dx));
            al = remap((u8)(al >> 4));          /* `mov cl, 4; shr al, cl` (cx is reloaded) */
            if (al != 0) {
                ST8(es, di, al);
            }
            di = (u16)(di + 1u);
        }
        /* L_2C21 */
        si = (u16)(si + cs16(G_SRC_ROW_SKIP));
        di = (u16)(di + cs16(G_DST_ROW_SKIP));
        set_cs16(G_ROWS_LEFT, (u16)(cs16(G_ROWS_LEFT) - 1u));
        if (cs16(G_ROWS_LEFT) == 0) {
            return 0;
        }
    }
}

/* ---- the table ------------------------------------------------------------------------------------ */

/* offset, name, C, argument bytes read, bytes of DX:AX returned, other registers answered */
const tc_driver_routine mcga_mask_routines[] = {
    {0x1006, "DrawShapeMaskedFlipped", DrawShapeMaskedFlipped, 12, 0, 0},
    {0x12F0, "DrawShapeMasked", DrawShapeMasked, 12, 0, 0},
    {0x279C, "DrawShapeRemapped", DrawShapeRemapped, 10, 0, 0},
    {0x298A, "DrawShapeFlippedRemapped", DrawShapeFlippedRemapped, 10, 0, 0},
};
const unsigned mcga_mask_count = sizeof mcga_mask_routines / sizeof mcga_mask_routines[0];
