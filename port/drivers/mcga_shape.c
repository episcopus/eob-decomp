/* mcga_shape.c: MCGA.OVL in C, DrawShape and its mirror (playbook 80, M4; src/drivers/MCGA.ASM, mcga.h).

   DrawShape(page, void far *shape, x, y) draws a shape at pixel (x, y) of a page, clipped to the
   region SetRegion sets (cs:00CA: columns of 8 pixels, then lines); DrawShapeFlipped draws it mirrored
   left to right. A shape begins with four bytes: its format, its rows, its width / 8, one byte not
   read. Format 0 (first byte 0): rows of 8-bit pixels, where 0 and a count is a run of transparent
   pixels. Any other first byte: a 16-byte colour table of the shape's own, then two pixels a byte (the
   high nibble first), a pixel transparent when the table gives it colour 0.

   Each routine follows the source's labels: clip (top, bottom, left, right), set up, then the row loop
   and the pixel loop, on 16-bit variables named for what the source keeps in its registers. DrawShape
   keeps its state on its stack ([bp-2] to [bp-18h]), which is C locals here; DrawShapeFlipped keeps it
   in the driver's variables (cs:00D2 to cs:00F5), written here where the source writes them, since the
   game and the per-function test see them. Every pointer is an offset that wraps within its segment;
   a run's count or a clip that does not line up with the rows walks on as the source walks. */
#include "mcga.h"

/* the format-0 run walker (L_1D45, L_1DF6, L_1E36 and the mirror's): passes over cx pixels of a
   format-0 shape at seg:*p, a 0 and its count being count pixels. `xor ax,ax` sets ZF before `repne
   scasb`, so a cx of 0 (no compare) takes `je` to the count as if a 0 were found. Each loop goes on
   while the count left is nonzero (`jne`), or while it is above 0 (`jg`, signed: the left skip), and
   leaves the count left (0, or below 0 when a run goes past the pixels skipped). */
static u16 skip_pixels(u16 seg, u16 *p, u16 cx, int while_above_zero)
{
    for (;;) {
        u16 count;
        int zf = x86_rep_scasb(0, seg, p, &cx, 0, 1);
        if (!zf && cx == 0) {               /* jcxz: no 0 in the pixels skipped */
            return cx;
        }
        count = x86_lodsb(seg, p);          /* mov al,es:[di]; inc di */
        cx = (u16)(cx + 1u);                /* the 0 was not a pixel */
        if (while_above_zero) {
            int above = (s16)cx > (s16)count;  /* jg after sub cx,ax */
            cx = (u16)(cx - count);
            if (!above) {
                return cx;
            }
        } else {
            cx = (u16)(cx - count);
            if (cx == 0) {
                return cx;
            }
        }
    }
}

/* a 4-bit pixel: `xlat` through the shape's colour table at seg:table (the offset wraps), colour 0
   transparent (`inc di`), else `stosb` */
static void put_nibble(u16 seg, u16 table, u16 es, u16 *di, u8 nibble)
{
    u8 c = tc_ld_u8(TC_MKFP(seg, (u16)(table + nibble)));
    if (c != 0) {
        x86_stosb(es, di, c);
    } else {
        *di = (u16)(*di + 1u);
    }
}

/* `jle` after `add cx,ax`: the sum, as signed words, is 0 or below */
static int sum_at_most_zero(u16 a, u16 b)
{
    return (s32)(s16)a + (s32)(s16)b <= 0;
}

/* ---- DrawShape ------------------------------------------------------------------------------------- */

/* MCGA.OVL 1CAB (export 31, gDrawShape): DrawShape(page, void far *shape, x, y), clipped to the
   region. QUIRK 1FEF: `pop bp; retf` after the last jump is never reached. QUIRKs the C keeps: a shape
   of 0 rows that is not clipped at the top draws 65536 rows (`dec; jne`); in format 4, a row of 0
   bytes with an odd x clipped on the left ends the draw (`jcxz L_1FD8`), and the line is found by `mul`
   (y * 320, its low word), not by the row table. */
static u32 DrawShape(const u8 *arg, tc_regs *r)
{
    farptr shape = arg_far(arg, 8);
    u16 es, di = 0, si = TC_OFF(shape), seg = TC_SEG(shape);
    u16 x, y = arg_word(arg, 14), cols, ax;
    u16 rows, width, full_width, skip_left, skip_right;
    u16 top = cs16(G_REGION_TOP), bottom = cs16(G_REGION_BOTTOM);
    (void)r;

    es = mcga_PageSeg(arg_word(arg, 6));
    ax = (u16)(cs16(G_REGION_LEFT) << 3);
    di = (u16)(di + ax);                                    /* the region's left edge */
    x = (u16)(arg_word(arg, 12) - ax);                      /* [bp-10h]: x from it */
    cols = (u16)(cs16(G_REGION_RIGHT) - cs16(G_REGION_LEFT));  /* [bp-0Eh] */

    if (x86_lodsb(seg, &si) == 0) {
        /* ---- format 0 (L_1D01) */
        u16 visible, row_start, dx;
        ax = x86_lodsw(seg, &si);
        si = (u16)(si + 1u);
        rows = (u16)(ax & 0xFFu);
        width = full_width = (u16)((ax >> 8) << 3);

        /* the top: skip the rows above it, run by run */
        ax = (u16)(y - top);
        if (ax & 0x8000u) {
            rows = (u16)(rows + ax);
            if ((s16)rows <= 0) {                           /* jg after add */
                return 0;
            }
            dx = (u16)-ax;
            do {
                skip_pixels(seg, &si, width, 0);            /* [bp-6] = the width */
            } while (--dx != 0);
            y = top;
        }
        /* the bottom */
        ax = (u16)(bottom - y);
        if ((s16)ax <= 0) {                                 /* js, je */
            return 0;
        }
        if (ax < rows) {
            rows = ax;
        }
        /* the left */
        skip_left = 0;
        if (x & 0x8000u) {
            width = (u16)(width + x);
            ax = (u16)-x;
            skip_left = ax;
            if (ax >= full_width) {
                return 0;
            }
            x = 0;
        }
        /* the right */
        skip_right = 0;
        ax = (u16)((u16)(cols << 3) - x);
        if ((s16)ax <= 0) {
            return 0;
        }
        if (ax < width) {
            width = ax;
            skip_right = (u16)(full_width - skip_left - width);
        }

        /* set up (L_1DC3) */
        di = (u16)(di + (u16)(row_offset(y) + x));
        row_start = di;                                     /* [bp-16h] */
        visible = width;                                    /* [bp-2] */
        dx = rows;

        /* the rows (L_1DE9) */
        do {
            u16 cx = skip_left;
            if (cx != 0) {
                cx = skip_pixels(seg, &si, cx, 1);
            }
            di = (u16)(di - cx);                            /* a run past the skip: its pixels on */
            {
                int none = sum_at_most_zero(cx, visible);
                cx = (u16)(cx + visible);
                if (!none) {
                    /* the pixels (L_1E18) */
                    for (;;) {
                        u8 al = x86_lodsb(seg, &si);
                        if (al != 0) {
                            x86_stosb(es, &di, al);
                            if (--cx == 0) {                /* loop */
                                break;
                            }
                        } else {
                            u16 count = x86_lodsb(seg, &si);
                            int above = (s16)cx > (s16)count;
                            di = (u16)(di + count);
                            cx = (u16)(cx - count);
                            if (!above) {                   /* jg */
                                break;
                            }
                        }
                    }
                }
            }
            cx = (u16)(cx + skip_right);                    /* L_1E29: a run past the edge counts */
            if (cx != 0) {
                skip_pixels(seg, &si, cx, 0);
            }
            row_start = (u16)(row_start + SCREEN_WIDTH);
            di = row_start;
        } while (--dx != 0);
        return 0;
    }

    /* ---- format 4 (L_1E68) */
    {
        u16 table, odd_x, dst_skip, row_bytes, used;
        ax = x86_lodsw(seg, &si);
        si = (u16)(si + 1u);
        table = si;                                         /* bx: the colour table */
        si = (u16)(si + 0x10u);
        rows = (u16)(ax & 0xFFu);
        width = full_width = (u16)((ax >> 8) << 3);

        /* the top: skip whole rows, width / 2 bytes each */
        ax = (u16)(y - top);
        if (ax & 0x8000u) {
            ax = (u16)-ax;
            if (ax >= rows) {
                return 0;
            }
            si = (u16)(si + (u16)(ax * (u16)(width >> 1)));
            ax = (u16)((u16)(rows + y) - top);
            if (ax & 0x8000u) {
                return 0;
            }
            rows = ax;
            y = top;
        }
        /* the bottom */
        ax = (u16)(bottom - y);
        if ((s16)ax <= 0) {
            return 0;
        }
        if (ax < rows) {
            rows = ax;
        }
        /* the left: skip_left is 1 when clipped; the bytes skipped are added before the test */
        skip_left = 0;
        odd_x = (u16)(x & 1u);                              /* [bp-2] */
        if (x & 0x8000u) {
            width = (u16)(width + x);
            ax = (u16)-x;
            si = (u16)(si + (u16)(ax >> 1));
            if (ax >= full_width) {
                return 0;
            }
            x = 0;
            skip_left++;
        }
        /* the right: skip_right is 1 when clipped */
        skip_right = 0;
        ax = (u16)((u16)(cols << 3) - x);
        if ((s16)ax <= 0) {
            return 0;
        }
        if (ax < width) {
            width = ax;
            skip_right++;
        }

        /* set up (L_1F23) */
        di = (u16)(di + (u16)((u16)(y * SCREEN_WIDTH) + x));
        dst_skip = (u16)(SCREEN_WIDTH - width);             /* [bp-12h] */
        row_bytes = (u16)(width >> 1);                      /* [bp-0Ah] */
        full_width = (u16)(full_width >> 1);                /* [bp-8] */
        used = row_bytes;                                   /* [bp-18h]: the bytes a row reads */
        if (skip_left != 0 && (odd_x & 1u)) {
            used++;
            full_width++;
            if (skip_right == 0) {
                dst_skip = (u16)(dst_skip + 2u);
            } else {
                used++;                                     /* L_1F6E */
            }
        } else if (skip_right != 0 && (odd_x & 1u)) {
            used++;
        }

        /* the rows (L_1F78) */
        for (;;) {
            u16 cx = row_bytes;
            if (skip_left != 0 && odd_x != 0) {
                u8 b;
                if (cx == 0) {
                    return 0;                               /* jcxz L_1FD8: the draw ends */
                }
                b = x86_lodsb(seg, &si);                    /* its low nibble only */
                put_nibble(seg, table, es, &di, (u8)(b & 0x0Fu));
                cx--;
            }
            while (cx != 0) {                               /* L_1F90 */
                u8 b = x86_lodsb(seg, &si);
                put_nibble(seg, table, es, &di, (u8)(b >> 4));
                put_nibble(seg, table, es, &di, (u8)(b & 0x0Fu));
                cx--;
            }
            if (skip_right != 0 && odd_x != 0) {            /* L_1FAD: a last high nibble */
                u8 b = x86_lodsb(seg, &si);
                u8 c = tc_ld_u8(TC_MKFP(seg, (u16)(table + (u8)(b >> 4))));
                if (c != 0) {
                    tc_st_u8(TC_MKFP(es, di), c);
                }
                di = (u16)(di + 1u);
            }
            si = (u16)(si + full_width - used);
            di = (u16)(di + dst_skip);
            if (--rows == 0) {
                return 0;
            }
        }
    }
}

/* ---- DrawShapeFlipped ------------------------------------------------------------------------------ */

/* MCGA.OVL 1639 (export 24, gDrawShapeFlipped): DrawShape mirrored left to right, both formats; its
   state in the driver's variables. A row is read from its start and drawn from its right end leftward,
   so the pixels skipped first are those past the region's right edge (gSkipRight) and the last those
   past its left (gSkipLeft). gSkipLeft holds the width while the top rows are skipped, and gShapeY
   becomes the line step in format 4. QUIRKs the C keeps: as in DrawShape, 0 rows draws 65536 and a
   row of 0 bytes with an odd x clipped on the left ends the draw (`jcxz L_19F7`); format 4 reads each
   byte through the nibble-swap table (cs:00F6) and finds the line by the row table. */
static u32 DrawShapeFlipped(const u8 *arg, tc_regs *r)
{
    farptr shape = arg_far(arg, 8);
    u16 es, di = 0, si = TC_OFF(shape), seg, ax, dx;
    (void)r;

    set_cs16(G_SKIP_LEFT, 0);
    es = mcga_PageSeg(arg_word(arg, 6));
    set_cs16(G_DST_SEG, es);
    set_cs16(G_SHAPE_SEG, TC_SEG(shape));
    set_cs16(G_SHAPE_Y, arg_word(arg, 14));
    ax = (u16)(cs16(G_REGION_LEFT) << 3);
    di = (u16)(di + ax);
    set_cs16(G_SHAPE_UNUSED, 0);
    set_cs16(G_SHAPE_X, (u16)(arg_word(arg, 12) - ax));
    set_cs16(G_REGION_COLS, (u16)(cs16(G_REGION_RIGHT) - cs16(G_REGION_LEFT)));
    seg = cs16(G_SHAPE_SEG);

    if (x86_lodsb(seg, &si) == 0) {
        /* ---- format 0 (L_16A5) */
        u16 bp;
        ax = x86_lodsw(seg, &si);
        si = (u16)(si + 1u);
        set_cs16(G_SHAPE_ROWS, (u16)(ax & 0xFFu));
        set_cs16(G_SHAPE_WIDTH, (u16)((ax >> 8) << 3));
        set_cs16(G_SHAPE_FULL_WIDTH, (u16)((ax >> 8) << 3));

        /* the top */
        ax = (u16)(cs16(G_SHAPE_Y) - cs16(G_REGION_TOP));
        if (ax & 0x8000u) {
            u16 rows = (u16)(cs16(G_SHAPE_ROWS) + ax);
            set_cs16(G_SHAPE_ROWS, rows);
            if ((s16)rows <= 0) {
                return 0;
            }
            dx = (u16)-ax;
            set_cs16(G_SKIP_LEFT, cs16(G_SHAPE_WIDTH));
            do {
                skip_pixels(seg, &si, cs16(G_SKIP_LEFT), 0);
            } while (--dx != 0);
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
        /* the left */
        set_cs16(G_SKIP_LEFT, 0);
        ax = cs16(G_SHAPE_X);
        if (ax & 0x8000u) {
            set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) + ax));
            ax = (u16)-ax;
            set_cs16(G_SKIP_LEFT, ax);
            if (ax >= cs16(G_SHAPE_FULL_WIDTH)) {
                return 0;
            }
            set_cs16(G_SHAPE_X, 0);
        }
        /* the right */
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

        /* set up (L_178F): di at the row's last visible pixel */
        di = (u16)(di + (u16)(row_offset(cs16(G_SHAPE_Y)) + cs16(G_SHAPE_X)));
        set_cs16(G_VISIBLE_WIDTH, cs16(G_SHAPE_WIDTH));
        di = (u16)(di + cs16(G_SHAPE_WIDTH) - 1u);
        bp = di;
        dx = cs16(G_SHAPE_ROWS);

        /* the rows (L_17BF) */
        do {
            u16 cx = cs16(G_SKIP_RIGHT);
            if (cx != 0) {
                cx = skip_pixels(seg, &si, cx, 1);
            }
            di = (u16)(di + cx);                            /* a run past the skip: its pixels on */
            {
                u16 visible = cs16(G_VISIBLE_WIDTH);
                int none = sum_at_most_zero(cx, visible);
                cx = (u16)(cx + visible);
                if (!none) {
                    /* the pixels (L_17F2), leftward */
                    for (;;) {
                        u8 al = x86_lodsb(seg, &si);
                        if (al != 0) {
                            tc_st_u8(TC_MKFP(es, di), al);
                            di = (u16)(di - 1u);
                            if (--cx == 0) {
                                break;
                            }
                        } else {
                            u16 count = x86_lodsb(seg, &si);
                            int above = (s16)cx > (s16)count;
                            di = (u16)(di - count);
                            cx = (u16)(cx - count);
                            if (!above) {
                                break;
                            }
                        }
                    }
                }
            }
            cx = (u16)(cx + cs16(G_SKIP_LEFT));             /* L_1806 */
            if (cx != 0) {
                skip_pixels(seg, &si, cx, 0);
            }
            bp = (u16)(bp + SCREEN_WIDTH);
            di = bp;
        } while (--dx != 0);
        return 0;
    }

    /* ---- format 4 (L_1843): si walks each row backward from its last byte */
    {
        u16 table;
        ax = x86_lodsw(seg, &si);
        si = (u16)(si + 1u);
        table = si;
        si = (u16)(si + 0x10u);
        set_cs16(G_SHAPE_ROWS, (u16)(ax & 0xFFu));
        set_cs16(G_SHAPE_WIDTH, (u16)((ax >> 8) << 3));
        set_cs16(G_SHAPE_FULL_WIDTH, (u16)((ax >> 8) << 3));

        /* the top */
        ax = (u16)(cs16(G_SHAPE_Y) - cs16(G_REGION_TOP));
        if (ax & 0x8000u) {
            ax = (u16)-ax;
            if (ax >= cs16(G_SHAPE_ROWS)) {
                return 0;
            }
            si = (u16)(si + (u16)(ax * (u16)(cs16(G_SHAPE_WIDTH) >> 1)));
            ax = (u16)((u16)(cs16(G_SHAPE_ROWS) + cs16(G_SHAPE_Y)) - cs16(G_REGION_TOP));
            if (ax & 0x8000u) {
                return 0;
            }
            set_cs16(G_SHAPE_ROWS, ax);
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
        /* the left (the row's end, nothing to skip in si) */
        set_cs16(G_SKIP_LEFT, 0);
        ax = cs16(G_SHAPE_X);
        set_cs16(G_ODD_X, (u16)(ax & 1u));
        if (ax & 0x8000u) {
            set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) + ax));
            ax = (u16)-ax;
            if (ax >= cs16(G_SHAPE_FULL_WIDTH)) {
                return 0;
            }
            set_cs16(G_SHAPE_X, 0);
            set_cs16(G_SKIP_LEFT, (u16)(cs16(G_SKIP_LEFT) + 1u));
        }
        /* the right (the row's start: skip its bytes) */
        set_cs16(G_SKIP_RIGHT, 0);
        ax = (u16)((u16)(cs16(G_REGION_COLS) << 3) - cs16(G_SHAPE_X));
        if ((s16)ax <= 0) {
            return 0;
        }
        if (ax < cs16(G_SHAPE_WIDTH)) {
            dx = cs16(G_SHAPE_WIDTH);
            set_cs16(G_SHAPE_WIDTH, ax);
            si = (u16)(si + (u16)(dx >> 1) - (u16)(ax >> 1));
            set_cs16(G_SKIP_RIGHT, (u16)(cs16(G_SKIP_RIGHT) + 1u));
        }

        /* set up (L_192A) */
        di = (u16)(di + row_offset(cs16(G_SHAPE_Y)));
        di = (u16)(di + cs16(G_SHAPE_X));
        set_cs16(G_SHAPE_Y, (u16)(SCREEN_WIDTH - cs16(G_SHAPE_WIDTH)));   /* the line step */
        set_cs16(G_SHAPE_WIDTH, (u16)(cs16(G_SHAPE_WIDTH) >> 1));
        set_cs16(G_SHAPE_FULL_WIDTH, (u16)(cs16(G_SHAPE_FULL_WIDTH) >> 1));
        si = (u16)(si - 1u);
        set_cs16(G_ROW_BYTES, cs16(G_SHAPE_WIDTH));
        set_cs16(G_SRC_ROW_START, cs16(G_SHAPE_WIDTH));
        if (cs16(G_SKIP_LEFT) != 0 && (cs16(G_ODD_X) & 1u)) {
            set_cs16(G_SHAPE_FULL_WIDTH, (u16)(cs16(G_SHAPE_FULL_WIDTH) - 1u));
            set_cs16(G_SHAPE_Y, (u16)(cs16(G_SHAPE_Y) + 2u));
            set_cs16(G_SRC_ROW_START, (u16)(cs16(G_SRC_ROW_START) + 1u));
            if (cs16(G_SKIP_RIGHT) != 1u) {
                goto rows;
            }
            set_cs16(G_SHAPE_Y, (u16)(cs16(G_SHAPE_Y) - 2u));
        }
        if (cs16(G_SKIP_RIGHT) != 0 && (cs16(G_ODD_X) & 1u)) {     /* L_198F */
            set_cs16(G_SRC_ROW_START, (u16)(cs16(G_SRC_ROW_START) + 1u));
            si = (u16)(si - 1u);
        }
    rows:
        set_cs16(G_SRC_ROW_SKIP, cs16(G_SHAPE_FULL_WIDTH));       /* L_19A6 */
        set_cs16(G_DST_ROW_SKIP, cs16(G_SHAPE_Y));
        set_cs16(G_ROWS_LEFT, cs16(G_SHAPE_ROWS));
        es = cs16(G_DST_SEG);
        seg = cs16(G_SHAPE_SEG);

        /* the rows (L_19CD): each byte's nibbles swapped (cs:00F6), so the low one is drawn first */
        for (;;) {
            u16 cx = cs16(G_ROW_BYTES);
            u8 b;
            si = (u16)(si + cs16(G_SRC_ROW_START));
            if (cs16(G_SKIP_LEFT) != 0 && cs16(G_ODD_X) != 0) {
                if (cx == 0) {
                    return 0;                               /* jcxz L_19F7: the draw ends */
                }
                b = cs8((u16)(G_NIBBLE_SWAP + tc_ld_u8(TC_MKFP(seg, si))));
                si = (u16)(si - 1u);
                put_nibble(seg, table, es, &di, (u8)(b & 0x0Fu));
                cx--;
            }
            while (cx != 0) {                               /* L_19FC */
                b = cs8((u16)(G_NIBBLE_SWAP + tc_ld_u8(TC_MKFP(seg, si))));
                si = (u16)(si - 1u);
                put_nibble(seg, table, es, &di, (u8)(b >> 4));
                put_nibble(seg, table, es, &di, (u8)(b & 0x0Fu));
                cx--;
            }
            if (cs16(G_SKIP_RIGHT) != 0 && cs16(G_ODD_X) != 0) {    /* L_1A3A: a last pixel */
                u8 c;
                b = cs8((u16)(G_NIBBLE_SWAP + tc_ld_u8(TC_MKFP(seg, si))));
                si = (u16)(si - 1u);
                c = tc_ld_u8(TC_MKFP(seg, (u16)(table + (u8)(b >> 4))));
                if (c != 0) {
                    tc_st_u8(TC_MKFP(es, di), c);
                }
                di = (u16)(di + 1u);
            }
            si = (u16)(si + cs16(G_SRC_ROW_SKIP));
            di = (u16)(di + cs16(G_DST_ROW_SKIP));
            set_cs16(G_ROWS_LEFT, (u16)(cs16(G_ROWS_LEFT) - 1u));
            if (cs16(G_ROWS_LEFT) == 0) {
                return 0;
            }
        }
    }
}

/* ---- the table ------------------------------------------------------------------------------------ */

/* offset, name, C, argument bytes read, bytes of DX:AX returned, other registers answered */
const tc_driver_routine mcga_shape_routines[] = {
    {0x1639, "DrawShapeFlipped", DrawShapeFlipped, 10, 0, 0},
    {0x1CAB, "DrawShape", DrawShape, 10, 0, 0},
};
const unsigned mcga_shape_count = sizeof mcga_shape_routines / sizeof mcga_shape_routines[0];
