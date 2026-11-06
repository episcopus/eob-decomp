/* mcga_cell.c: MCGA.OVL in C, the sprite cutters and the text (playbook 80, M4; src/drivers/MCGA.ASM, mcga.h).

   The two cutters (GetSpriteCellRle 0C8D, GetSpriteCell 0DD9) with their near helpers (CellSource 0D9F,
   CellSize 0DBA, ClearColourMap 0DC9), the masked 16 by 16 cell draw (DrawMaskedCell16 0F56), and the
   text (DrawText 2543 with its near DrawGlyph 241E). Each follows the source's labels, on 16-bit
   register variables named as the source names them; the driver's variables are read and written in
   the arena at their cs: offsets, where the original keeps them (the cutters' at cs:01FD to cs:0327,
   mcga.h; the text state at cs:2027 to cs:2045 and the bit mask table at cs:2046, below). */
#include "mcga.h"

/* cs:2027: DrawGlyph's and XorRect's state (src/drivers/MCGA.ASM, after SaveRectSize). XorRect
   (mcga_rect.c) uses gBlitRowSkip, gXorBytes, gBlitWidth, gBlitRows, gBlitColour and the two masks;
   this file the ones the text uses. 2029 (6 bytes) and 2037 (4 bytes) are not used. */
#define G_BLIT_ROW_SKIP 0x2027u     /* screen bytes from the end of one row to the next */
#define G_XOR_BYTES 0x202Fu         /* XorRect's */
#define G_BLIT_WIDTH 0x2031u        /* byte: the glyph's width */
#define G_BLIT_ROWS 0x2032u         /* byte: rows left */
#define G_BLIT_X 0x2033u            /* byte */
#define G_BLIT_Y 0x2034u            /* byte */
#define G_BLIT_COLOUR 0x2035u       /* byte: the text colour */
#define G_BLIT_BACK 0x2036u         /* byte: the text background (0: transparent) */
#define G_TEXT_LEFT 0x203Bu         /* DrawText: the line start x */
#define G_LINE_HEIGHT 0x203Du
#define G_CHAR_WIDTH 0x203Fu
#define G_LEFT_MASK 0x2041u         /* XorRect's */
#define G_RIGHT_MASK 0x2042u        /* XorRect's */
#define G_GLYPH_BYTES 0x2043u       /* byte: the glyph's bytes in a row, less one */
#define G_FONT_SEG 0x2044u          /* the font's segment */
#define G_BIT_MASK_TABLE 0x2046u    /* 24 words: for width w, the set of eight mask pointers */

#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (u16)(off)))
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (u16)(off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (u16)(off)), (u8)(v))

/* ---- the sprite cutters' helpers ------------------------------------------------------------------ */

/* MCGA.OVL 0D9F (near): CellSource, ds:si = the source rectangle's corner in the page after the draw
   page: si = col * 8 + gRowOffsets[y], from the caller's col ([bp+6]) and y ([bp+8]). It leaves ax, bx
   and cl changed, which no caller reads. */
static void CellSource(const u8 *arg, u16 *ds, u16 *si)
{
    *ds = mcga_NextPageSeg();
    *si = (u16)((u16)(arg_word(arg, 6) << 3) + row_offset(arg_word(arg, 8)));
}

/* MCGA.OVL 0DBA (near): CellSize, dh = rows and dl = cols (the low bytes of [bp+0Ch] and [bp+0Ah]);
   gCellRows = rows. Returns ax, the cols word. */
static u16 CellSize(const u8 *arg, u8 *dh, u8 *dl)
{
    u16 ax = arg_word(arg, 12);
    *dh = (u8)ax;
    set_cs8(G_CELL_ROWS, (u8)ax);
    ax = arg_word(arg, 10);
    *dl = (u8)ax;
    return ax;
}

/* MCGA.OVL 0DC9 (near): ClearColourMap, every screen colour not yet given a 4-bit code (the 256 bytes
   of gColourMap set to FFh). Its es = cs and di = cs:030C on return are not read. */
static void ClearColourMap(void)
{
    u16 di = G_COLOUR_MAP, cx;
    for (cx = 0x80u; cx != 0; cx--) {
        x86_stosw(mcga_cs, &di, 0xFFFFu);
    }
}

/* ---- the sprite cutters ------------------------------------------------------------------------- */

/* MCGA.OVL 0C8D (export 15, gGetSpriteCellRle): GetSpriteCellRle(col, y, cols, rows, void far *sheet)
   cuts the rectangle at column col, line y of the page after the draw page into a block from MemAlloc:
   a header (0, rows, cols, rows), then each row's 8 * cols pixels, a run of zero bytes written as 0
   and its length; a run of 256 or more as 0, FFh, 0 and the low byte of its length less 255 (one split
   only). The rows and cols are the arguments' low bytes, a count of 0 being 256 rows or a row of 65536
   bytes (the `dec dh` and the `cmp bp,si` after the byte is read). Two passes: the first counts the
   bytes in gCellBytes (16 bits), the second writes them. MemAlloc gets gCellBytes + 4, with DS as the
   caller's (the source keeps it in es). The sheet is not read. Returns dx:ax the block. */
static u32 GetSpriteCellRle(const u8 *arg, tc_regs *r)
{
    u16 es = tc_ds, ds, si, di, cx, ax, bp;
    u8 dh, dl, al;
    farptr blk;
    (void)r;
    CellSource(arg, &ds, &si);
    CellSize(arg, &dh, &dl);
    set_cs16(G_CELL_BYTES, 0);
    do {                                                /* L_0CA8: count */
        set_cs16(G_CELL_ROW_START, si);
        bp = (u16)(8u * dl + si);
        do {                                            /* L_0CB5 */
            al = x86_lodsb(ds, &si);
            if (al == 0) {
                set_cs16(G_CELL_BYTES, (u16)(cs16(G_CELL_BYTES) + 1u));
                cx = si;
                while (bp != si && LD8(ds, si) == 0) {  /* L_0CC1 */
                    si++;
                }
                ax = (u16)(si - cx + 1u);               /* L_0CCD */
                if ((ax >> 8) != 0) {
                    set_cs16(G_CELL_BYTES, (u16)(cs16(G_CELL_BYTES) + 2u));
                }
            }
            set_cs16(G_CELL_BYTES, (u16)(cs16(G_CELL_BYTES) + 1u));     /* L_0CEE */
        } while (bp != si);
        si = (u16)(cs16(G_CELL_ROW_START) + SCREEN_WIDTH);
    } while (--dh != 0);

    blk = mcga_mem_alloc(es, (u16)(cs16(G_CELL_BYTES) + 4u), 0, 0, "MCGA GetSpriteCellRle");
    di = TC_OFF(blk);
    set_cs16(G_CELL_OFF, di);
    es = TC_SEG(blk);
    set_cs16(G_CELL_SEG, es);
    x86_stosb(es, &di, 0);
    dh = (u8)arg_word(arg, 12);
    x86_stosb(es, &di, dh);
    dl = (u8)arg_word(arg, 10);
    x86_stosb(es, &di, dl);
    x86_stosb(es, &di, dh);
    CellSource(arg, &ds, &si);
    do {                                                /* L_0D42: write */
        set_cs16(G_CELL_ROW_START, si);
        bp = (u16)(8u * dl + si);
        do {                                            /* L_0D4F */
            al = x86_lodsb(ds, &si);
            if (al == 0) {
                x86_stosb(es, &di, 0);
                cx = si;
                while (bp != si && LD8(ds, si) == 0) {  /* L_0D57 */
                    si++;
                }
                ax = (u16)(si - cx + 1u);               /* L_0D63 */
                if ((ax >> 8) != 0) {
                    ax = (u16)(ax - 0xFFu);
                    x86_stosb(es, &di, 0xFFu);
                    x86_stosb(es, &di, 0);
                }
                al = (u8)ax;
            }
            x86_stosb(es, &di, al);                     /* L_0D7C */
        } while (bp != si);
        si = (u16)(cs16(G_CELL_ROW_START) + SCREEN_WIDTH);
    } while (--dh != 0);
    return ((u32)cs16(G_CELL_SEG) << 16) | cs16(G_CELL_OFF);
}

/* MCGA.OVL 0DD9 (export 16, gGetSpriteCell): GetSpriteCell(col, y, cols, rows, void far *sheet) cuts the
   rectangle into a 4-bit cell from MemAlloc: a header (1, rows, cols, rows), 16 bytes of colours (the
   screen colour of each code in the order met; the codes not given are left as MemAlloc gives them),
   then two pixels a byte, the first in the high nibble, the nibbles running on across the rows (an
   odd count leaves the last low nibble 0). A colour past the sixteenth gets code 0. The first pass
   counts the bytes (16, then one for each second pixel: gCellBytes, 16 bits) and sets the colour map
   as it goes; the map is cleared again before the second, and stays as the second pass leaves it.
   rows and cols are low bytes, 0 being 256 rows or a row of 65536 pixels (`loop` with cx 0). MemAlloc
   gets gCellBytes + 4, with DS as the caller's (kept in gCellSeg). The sheet is not read. The first
   pass's bx (the caller's di stepped) is counted and never stored through. Returns dx:ax the block. */
static u32 GetSpriteCell(const u8 *arg, tc_regs *r)
{
    u16 ds, si, es, di, bx, cx, dx, bp;
    u8 dh, dl, al, ah, n;
    farptr blk;
    (void)r;
    set_cs16(G_CELL_SEG, tc_ds);
    ClearColourMap();
    CellSource(arg, &ds, &si);
    set_cs16(G_CELL_SRC_ROW, si);
    dx = (u8)CellSize(arg, &dh, &dl);
    set_cs16(G_CELL_BYTES, 0x10u);
    dx = (u16)(dx << 3);
    set_cs8(G_NIBBLE_HIGH, 0);
    set_cs8(G_COLOURS_USED, 0);
    do {                                                /* L_0E1A: count */
        cx = dx;
        do {                                            /* L_0E1C */
            al = x86_lodsb(ds, &si);
            bp = al;
            ah = cs8((u16)(bp + G_COLOUR_MAP));
            if (ah == 0xFFu && cs8(G_COLOURS_USED) < 0x10u) {   /* L_0E45 */
                n = cs8(G_COLOURS_USED);
                set_cs8(G_COLOURS_USED, (u8)(n + 1u));
                set_cs8((u16)(bp + G_COLOUR_MAP), n);
            }
            set_cs8(G_NIBBLE_HIGH, (u8)(cs8(G_NIBBLE_HIGH) + 1u));     /* L_0E2B */
            if ((cs8(G_NIBBLE_HIGH) & 1u) == 0) {
                set_cs16(G_CELL_BYTES, (u16)(cs16(G_CELL_BYTES) + 1u)); /* L_0E3C */
            }
        } while (--cx != 0);
        set_cs16(G_CELL_SRC_ROW, (u16)(cs16(G_CELL_SRC_ROW) + SCREEN_WIDTH));   /* L_0E63 */
        si = cs16(G_CELL_SRC_ROW);
        set_cs8(G_CELL_ROWS, (u8)(cs8(G_CELL_ROWS) - 1u));
    } while (cs8(G_CELL_ROWS) != 0);
    ClearColourMap();

    blk = mcga_mem_alloc(cs16(G_CELL_SEG), (u16)(cs16(G_CELL_BYTES) + 4u), 0, 0, "MCGA GetSpriteCell");
    di = TC_OFF(blk);
    set_cs16(G_CELL_OFF, di);
    es = TC_SEG(blk);
    set_cs16(G_CELL_SEG, es);
    x86_stosb(es, &di, 1);
    dh = (u8)arg_word(arg, 12);
    x86_stosb(es, &di, dh);
    dl = (u8)arg_word(arg, 10);
    x86_stosb(es, &di, dl);
    x86_stosb(es, &di, dh);
    CellSource(arg, &ds, &si);
    set_cs8(G_CELL_ROWS, dh);
    dx = dl;
    set_cs16(G_CELL_SRC_ROW, si);
    bx = di;
    di = (u16)(di + 0x10u);
    dx = (u16)(dx << 3);
    set_cs8(G_NIBBLE_HIGH, 0);
    set_cs8(G_COLOURS_USED, 0);
    do {                                                /* L_0EDC: write */
        cx = dx;
        do {                                            /* L_0EDE */
            al = x86_lodsb(ds, &si);
            bp = al;
            ah = cs8((u16)(bp + G_COLOUR_MAP));
            if (ah == 0xFFu) {                          /* L_0F11 */
                if (cs8(G_COLOURS_USED) < 0x10u) {
                    ST8(es, bx, al);
                    bx++;
                    ah = cs8(G_COLOURS_USED);
                    set_cs8(G_COLOURS_USED, (u8)(ah + 1u));
                    set_cs8((u16)(bp + G_COLOUR_MAP), ah);
                } else {
                    ah = 0;                             /* L_0F2E */
                }
            }
            set_cs8(G_NIBBLE_HIGH, (u8)(cs8(G_NIBBLE_HIGH) + 1u));     /* L_0EED */
            if ((cs8(G_NIBBLE_HIGH) & 1u) != 0) {
                ST8(es, di, (u8)(ah << 4));
            } else {
                ST8(es, di, (u8)(LD8(es, di) | ah));    /* L_0F09 */
                di++;
            }
        } while (--cx != 0);
        set_cs16(G_CELL_SRC_ROW, (u16)(cs16(G_CELL_SRC_ROW) + SCREEN_WIDTH));   /* L_0F32 */
        si = cs16(G_CELL_SRC_ROW);
        set_cs8(G_CELL_ROWS, (u8)(cs8(G_CELL_ROWS) - 1u));
    } while (cs8(G_CELL_ROWS) != 0);
    return ((u32)cs16(G_CELL_SEG) << 16) | cs16(G_CELL_OFF);
}

/* MCGA.OVL 0F56 (export 17, ds:808A): DrawMaskedCell16(maskCell, cell, void far *sheet, col, y) draws the
   16 by 16 cell number cell of a sheet of 128-byte 4-bit cells to the draw page at column col, line y,
   through cell maskCell: for each sheet byte, ah starts FFh and loses its high nibble where the mask
   byte's high nibble is set, its low where the low is; the drawn byte is mask | (ah & cell), written as
   two pixels, its high nibble left in place (byte & F0h) then its low (byte & 0Fh), one word. The cell
   offsets are cell << 7 and maskCell << 7 from the sheet's offset, each wrapping at 64 KB. Returns
   nothing (ax is saved). */
static u32 DrawMaskedCell16(const u8 *arg, tc_regs *r)
{
    u16 si = arg_word(arg, 10), ds = arg_word(arg, 12), di, bx, ax, es, cx;
    u8 dl, al, ah;
    (void)r;
    di = (u16)(arg_word(arg, 14) << 3);
    di = (u16)(di + row_offset(arg_word(arg, 16)));
    bx = (u16)((u16)(arg_word(arg, 6) << 7) + si);
    si = (u16)(si + (u16)(arg_word(arg, 8) << 7));
    es = mcga_DrawPageSeg();
    dl = 0x10u;
    do {                                                /* L_0F98 */
        for (cx = 8; cx != 0; cx--) {                   /* L_0F9B */
            al = LD8(ds, bx);
            ah = 0xFFu;
            if ((al & 0xF0u) != 0) {
                ah &= 0x0Fu;
            }
            if ((al & 0x0Fu) != 0) {
                ah &= 0xF0u;
            }
            ah &= LD8(ds, si);
            al |= ah;
            ax = (u16)(((u16)al << 8) | al);
            x86_stosw(es, &di, (u16)(ax & 0x0FF0u));
            si++;
            bx++;
        }
        di = (u16)(di + SCREEN_WIDTH - 16u);
    } while (--dl != 0);
    return 0;
}

/* ---- the text ---------------------------------------------------------------------------------- */

/* MCGA.OVL 241E (near): DrawGlyph(ch, x, y) draws character ch of the current font (gCurFont, cs:0003,
   points at a record whose first far pointer is the font: +2 the glyph offsets, a word for ch at
   2 * (ch + 1); +102h the height; +103h the width) at (x, y) on the draw page: the set bits of each
   glyph row in gBlitColour (unless it is 0), the clear bits in gBlitBack (unless it is 0), the row's
   bits counted out by the width's mask (gBitMaskTable, the start bit 0 set) and its bytes by
   gGlyphBytes, (width - 1) / 8 more. Not drawn when x is past 319 or x + width past 320, y past 199 or
   y + height past 200 (unsigned), or the width is 0 or over 24; gBlitX, gBlitWidth, gBlitY and
   gBlitRows are set as far as the checks get. A height of 0 draws 256 rows (`dec byte`). At 24A0
   gGlyphBytes is first set to (width + 6) / 8 and at once set again; only the second value counts.
   The registers it leaves changed (ax, cx, dx, ds) are not read by DrawText. */
static void DrawGlyph(u16 ch, u16 x, u16 y)
{
    farptr font = cs32(G_CUR_FONT_PTR);
    u16 es = TC_SEG(font), si = TC_OFF(font), bx, dx, cx, ax, di, bp, mask_row, ds;
    u8 bl, bh, al, ah, cl, chb;
    ax = LD16(es, (u16)(si + 2u));
    si = LD16(es, si);
    es = ax;
    bx = (u16)((u16)(ch + 1u) << 1);
    bx = LD16(es, (u16)(bx + si));
    dx = x;
    if (dx >= SCREEN_WIDTH) {
        return;
    }
    ax = dx;
    set_cs8(G_BLIT_X, (u8)ax);
    cx = LD8(es, (u16)(si + FONT_WIDTH));
    set_cs8(G_BLIT_WIDTH, (u8)cx);
    ax = (u16)(ax + cx);
    if (ax > SCREEN_WIDTH) {
        return;
    }
    di = dx;
    dx = y;
    if (dx >= SCREEN_HEIGHT) {
        return;
    }
    ax = dx;
    set_cs8(G_BLIT_Y, (u8)ax);
    cx = LD8(es, (u16)(si + FONT_HEIGHT));
    set_cs8(G_BLIT_ROWS, (u8)cx);
    ax = (u16)(ax + cx);
    if (ax > SCREEN_HEIGHT) {
        return;
    }
    si = (u16)(si + bx);
    al = (u8)(cs8(G_BLIT_WIDTH) - 1u);
    if (al >= 0x18u) {
        return;
    }
    bp = al;
    set_cs8(G_GLYPH_BYTES, (u8)((u8)(al + 7u) >> 3));  /* 24A0: overwritten below */
    bp = cs16((u16)((u16)(bp << 1) + G_BIT_MASK_TABLE));
    set_cs8(G_GLYPH_BYTES, (u8)((u8)(cs8(G_BLIT_WIDTH) - 1u) >> 3));
    bp = cs16(bp);
    di = (u16)(di + row_offset(dx));
    set_cs16(G_BLIT_ROW_SKIP, (u16)(SCREEN_WIDTH - cs8(G_BLIT_WIDTH)));
    ds = es;
    es = mcga_DrawPageSeg();
    bl = cs8(G_BLIT_COLOUR);
    bh = cs8(G_BLIT_BACK);
    do {                                                /* L_24EF */
        chb = cs8(G_GLYPH_BYTES);
        mask_row = bp;
        for (;;) {                                      /* L_24F6 */
            al = x86_lodsb(ds, &si);
            ah = cs8(bp);
            cl = 0x80u;
            while ((ah & cl) != 0) {                    /* L_24FD */
                if ((al & cl) != 0) {
                    if (bl != 0) {
                        ST8(es, di, bl);
                    }
                } else if (bh != 0) {                   /* L_251A */
                    ST8(es, di, bh);
                }
                di++;                                   /* L_2521 */
                cl >>= 1;
                if (cl == 0) {
                    break;
                }
            }
            if (cl != 0 || chb == 0) {                  /* a clear mask bit, or the row's last byte */
                break;
            }
            bp++;
            chb--;
        }
        bp = mask_row;                                  /* L_252F */
        di = (u16)(di + cs16(G_BLIT_ROW_SKIP));
        set_cs8(G_BLIT_ROWS, (u8)(cs8(G_BLIT_ROWS) - 1u));
    } while (cs8(G_BLIT_ROWS) != 0);
}

/* MCGA.OVL 2543 (export 33, gDrawText): DrawText(char far *text, x, y, colour, background) draws a string
   in the current font through DrawGlyph, each character & 7Fh. x and y below 0 are taken as 0 (signed);
   x past 319 (unsigned) or y past 199 (signed) draws nothing. CR (0Dh) goes back to the first x and down
   a line height (the font's height byte) with no check of y until the next character; a character that
   would pass x 320 does the same before it is drawn; the string ends at its 0 or when y reaches 200
   (unsigned). gFontSeg, gLineHeight, gTextLeft, gCharWidth, gBlitColour and gBlitBack are left as set.
   Returns nothing (ax is saved). */
static u32 DrawText(const u8 *arg, tc_regs *r)
{
    farptr font = cs32(G_CUR_FONT_PTR);
    u16 es = TC_SEG(font), di = TC_OFF(font), ax, si, bx, cx, dx;
    u8 c;
    (void)r;
    ax = LD16(es, (u16)(di + 2u));
    di = LD16(es, di);
    es = ax;
    set_cs16(G_FONT_SEG, es);
    set_cs16(G_LINE_HEIGHT, LD8(es, (u16)(di + FONT_HEIGHT)));
    si = arg_word(arg, 6);
    es = arg_word(arg, 8);
    bx = arg_word(arg, 10);
    if ((s16)bx < 0) {
        bx = 0;
    }
    if (bx >= SCREEN_WIDTH) {
        return 0;
    }
    set_cs16(G_TEXT_LEFT, bx);
    cx = arg_word(arg, 12);
    if ((s16)cx < 0) {
        cx = 0;
    }
    if ((s16)cx >= (s16)SCREEN_HEIGHT) {
        return 0;
    }
    set_cs8(G_BLIT_COLOUR, arg[0x0E - 6]);
    set_cs8(G_BLIT_BACK, arg[0x10 - 6]);
    for (;;) {                                          /* L_25AF */
        c = LD8(es, si);
        if (c == 0) {
            break;
        }
        if (c == 0x0Du) {
            bx = cs16(G_TEXT_LEFT);
            cx = (u16)(cx + cs16(G_LINE_HEIGHT));
            si++;
            continue;
        }
        ax = LD8(cs16(G_FONT_SEG), (u16)(di + FONT_WIDTH));    /* L_25C8 */
        set_cs16(G_CHAR_WIDTH, ax);
        dx = (u16)(bx + ax);
        if (dx > SCREEN_WIDTH) {
            bx = cs16(G_TEXT_LEFT);
            cx = (u16)(cx + cs16(G_LINE_HEIGHT));
        }
        if (cx >= SCREEN_HEIGHT) {                      /* L_25EE */
            break;
        }
        DrawGlyph((u16)(c & 0x7Fu), bx, cx);
        bx = (u16)(bx + cs16(G_CHAR_WIDTH));
        si++;
    }
    return 0;
}

/* ---- the table ------------------------------------------------------------------------------------ */

/* offset, name, C, argument bytes read, bytes of DX:AX returned, other registers answered */
const tc_driver_routine mcga_cell_routines[] = {
    {0x0C8D, "GetSpriteCellRle", GetSpriteCellRle, 8, 4, 0},
    {0x0DD9, "GetSpriteCell", GetSpriteCell, 8, 4, 0},
    {0x0F56, "DrawMaskedCell16", DrawMaskedCell16, 12, 0, 0},
    {0x2543, "DrawText", DrawText, 11, 0, 0},
};
const unsigned mcga_cell_count = sizeof mcga_cell_routines / sizeof mcga_cell_routines[0];
