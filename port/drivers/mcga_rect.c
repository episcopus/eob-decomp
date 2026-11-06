/* mcga_rect.c: MCGA.OVL in C, rectangles, cells, tiles and lines (playbook 80, M4; src/drivers/MCGA.ASM, mcga.h).

   The rectangle save and restore (SaveRect, RestoreRect and their page variants), the page to page copy
   (CopyRegion), FillRect and XorRect, the 8 by 8 cells (DrawCell8x8, ReadCell8x8, MirrorCell16), the
   indexed tiles (SetTileSet, SetTileFormat, DrawIndexedTile) and the clipped line (DrawLine).

   Each routine follows its PROC in the source, with 16-bit locals named as the registers it uses; the
   near helpers with register conventions take them as arguments or through a record of them. The
   driver's variables stay in the arena at their cs: offsets (mcga.h for 00C1 to 04F7; those only these
   routines use are below). The direction flag is clear in every routine (CopyRegion, RestoreRect,
   SaveRect, DrawCell8x8 and FillRect begin with `cld`; the others are entered with it clear). */
#include "mcga.h"

/* cs: offsets only this file uses */
#define G_TILE_DRAW_MODES 0x0780u   /* DrawIndexedTile's jump table: 0786, 0786, 078E */
#define TILE_DRAW 0x0786u           /* the table's targets: draw the tile */
#define TILE_NONE 0x078Eu           /* draw nothing */
#define G_LINE_COLOUR 0x09E9u       /* DrawLine's colour, kept inside the code */
#define G_BLIT_ROW_SKIP 0x2027u     /* cs:2027: DrawGlyph's and XorRect's state (shared with mcga_cell.c) */
#define G_XOR_BYTES 0x202Fu
#define G_BLIT_WIDTH 0x2031u        /* bytes */
#define G_BLIT_ROWS 0x2032u
#define G_BLIT_Y 0x2034u
#define G_BLIT_COLOUR 0x2035u
#define G_LEFT_MASK 0x2041u         /* bytes */
#define G_RIGHT_MASK 0x2042u
#define G_BIT_MASK_TABLE 0x2046u    /* 24 words: the mask set of each width, 1 to 24 */

#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (u16)(off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (u16)(off)), (u8)(v))
#define MOVSW(ds, si, es, di) x86_stosw((es), (di), x86_lodsw((ds), (si)))

/* the page number gDrawPage holds, as `lodsw` and `and ax,1Fh` read it through cs:gDrawPagePtr */
static u16 draw_page_number(void)
{
    return (u16)(tc_ld_u16(cs32(G_DRAW_PAGE_PTR)) & 0x1Fu);
}

/* ---- the saved rectangles -------------------------------------------------------------------------- */

/* MCGA.OVL 0543 (RestoreRectBody, inside RestoreRect): the body RestoreRect and RestoreRectToPage share,
   with the page number their [bp-2] holds. The column is clamped to 0 to 39 and the line to 0 to 199
   (signed); cols to 40 and rows to 200 (unsigned, so a negative count is the most); a count of 0 draws
   nothing. QUIRK 0588: the `js` after `or bx,bx` is never taken, bx being at most 40 there. */
static void RestoreRectBody(const u8 *arg, u16 page)
{
    u16 di = arg_word(arg, 6), dx, bx, cx, ax, es, ds, si;
    if ((s16)di < 0) {
        di = 0;
    }
    if ((s16)di >= (s16)SCREEN_COLS) {
        di = SCREEN_COLS - 1u;
    }
    di = (u16)(di << 3);
    dx = arg_word(arg, 8);
    if ((s16)dx < 0) {
        dx = 0;
    }
    if ((s16)dx >= (s16)SCREEN_HEIGHT) {
        dx = SCREEN_HEIGHT - 1u;
    }
    bx = arg_word(arg, 10);
    cx = arg_word(arg, 12);
    if (bx > SCREEN_COLS) {
        bx = SCREEN_COLS;
    }
    if (bx == 0) {
        return;
    }
    if (cx > SCREEN_HEIGHT) {
        cx = SCREEN_HEIGHT;
    }
    if (cx == 0) {
        return;
    }
    di = (u16)(di + row_offset(dx));
    dx = cx;
    bx = (u16)(bx << 2);
    es = mcga_PageSeg(page);
    si = arg_word(arg, 14);
    ds = arg_word(arg, 16);
    ax = (u16)((u16)(SCREEN_WIDTH / 2u - bx) << 1);
    do {
        cx = bx;
        x86_rep_movsw(ds, &si, es, &di, &cx);
        di = (u16)(di + ax);
    } while (--dx != 0);
}

/* MCGA.OVL 0516 (export 1, gRestoreRectToPage): RestoreRect onto the page gDrawPage names (AND 1Fh)
   instead of page 0. QUIRK 0534: the `pop bp` and `retf` after its jump are dead. */
static u32 RestoreRectToPage(const u8 *arg, tc_regs *r)
{
    (void)r;
    RestoreRectBody(arg, draw_page_number());
    return 0;
}

/* MCGA.OVL 0536 (export 2, gRestoreRect): RestoreRect(col, y, cols, rows, void far *buf) copies a
   buffer SaveRect filled back to page 0 at column col, line y, cols * 8 bytes a row */
static u32 RestoreRect(const u8 *arg, tc_regs *r)
{
    (void)r;
    RestoreRectBody(arg, 0);
    return 0;
}

/* MCGA.OVL 1AE8 (SaveRectBody, inside SaveRect): the body SaveRect and SaveRectFromPage share, with the
   page number of their [bp-2]; returns the bytes written (the buffer's end offset less its start), 0
   for cols or rows of 0 or less (signed). The column is clamped to 0 to 39, the line to 0 to 199, cols
   to 40 and rows to 200. QUIRK 1B48 to 1B60: the second, unsigned clamps and the zero tests of bx and
   cx are never taken, the signed ones before them having left them 1 to 40 and 1 to 200. */
static u16 SaveRectBody(const u8 *arg, u16 page)
{
    u16 si = arg_word(arg, 6), dx, bx, cx, di, di0, ds, es, ax;
    if ((s16)si < 0) {
        si = 0;
    }
    if ((s16)si >= (s16)SCREEN_COLS) {
        si = SCREEN_COLS - 1u;
    }
    si = (u16)(si << 3);
    dx = arg_word(arg, 8);
    if ((s16)dx < 0) {
        dx = 0;
    }
    if ((s16)dx >= (s16)SCREEN_HEIGHT) {
        dx = SCREEN_HEIGHT - 1u;
    }
    bx = arg_word(arg, 10);
    if ((s16)bx <= 0) {
        return 0;
    }
    if ((s16)bx > (s16)SCREEN_COLS) {
        bx = SCREEN_COLS;
    }
    cx = arg_word(arg, 12);
    if ((s16)cx <= 0) {
        return 0;
    }
    if ((s16)cx > (s16)SCREEN_HEIGHT) {
        cx = SCREEN_HEIGHT;
    }
    di = di0 = arg_word(arg, 14);
    es = arg_word(arg, 16);
    si = (u16)(si + row_offset(dx));
    dx = cx;
    bx = (u16)(bx << 2);
    ds = mcga_PageSeg(page);
    ax = (u16)((u16)(SCREEN_WIDTH / 2u - bx) << 1);
    do {
        cx = bx;
        x86_rep_movsw(ds, &si, es, &di, &cx);
        si = (u16)(si + ax);
    } while (--dx != 0);
    return (u16)(di - di0);
}

/* MCGA.OVL 1ABD (export 14, gSaveRectFromPage): SaveRect from the page gDrawPage names (AND 1Fh)
   instead of page 0; returns the bytes written. QUIRK 1ADA: the `pop bp` and `retf` after its jump are
   dead. */
static u32 SaveRectFromPage(const u8 *arg, tc_regs *r)
{
    (void)r;
    return SaveRectBody(arg, draw_page_number());
}

/* MCGA.OVL 1ADC (export 26, gSaveRect): SaveRect(col, y, cols, rows, void far *buf) copies a rectangle
   of page 0 (cols 8-pixel columns, rows lines) into buf for RestoreRect; returns the bytes written */
static u32 SaveRect(const u8 *arg, tc_regs *r)
{
    (void)r;
    return SaveRectBody(arg, 0);
}

/* MCGA.OVL 05D4 (export 3, gCopyRegion): CopyRegion(srcCol, srcY, dstCol, dstY, cols, rows, srcPage,
   dstPage) copies cols 8-pixel columns (1 to 40) of rows lines from one page to another. A column of 40
   or more or a line of 200 or more (signed) ends it; negative ones are taken as 0. The rows are cut
   against the destination line as given, unsigned: when dstY + rows passes 200, rows becomes
   rows - (dstY + rows - 199), so a cut copy stops at line 198, and a negative dstY can make a large
   count (dstY -5, rows 3: 204 rows from line 0). QUIRK 0673: a count of 0 is not refused (only a
   negative one): the loop runs 65536 times, the offsets wrapping within the segments. */
static u32 CopyRegion(const u8 *arg, tc_regs *r)
{
    u16 ax, si, di, cx, dx, bx, ds, es;
    (void)r;
    ax = arg_word(arg, 6);
    if ((s16)ax >= (s16)SCREEN_COLS) {
        return 0;
    }
    if ((s16)ax < 0) {
        ax = 0;
    }
    si = (u16)(ax << 3);
    ax = arg_word(arg, 8);
    if ((s16)ax >= (s16)SCREEN_HEIGHT) {
        return 0;
    }
    if ((s16)ax < 0) {
        ax = 0;
    }
    si = (u16)(si + row_offset(ax));
    ax = arg_word(arg, 10);
    if ((s16)ax >= (s16)SCREEN_COLS) {
        return 0;
    }
    if ((s16)ax < 0) {
        ax = 0;
    }
    di = (u16)(ax << 3);
    ax = arg_word(arg, 12);
    cx = ax;
    if ((s16)ax >= (s16)SCREEN_HEIGHT) {
        return 0;
    }
    if ((s16)ax < 0) {
        ax = 0;
    }
    di = (u16)(di + row_offset(ax));
    ds = mcga_PageSeg(arg_word(arg, 18));
    es = mcga_PageSeg(arg_word(arg, 20));
    dx = arg_word(arg, 14);
    if (dx > SCREEN_COLS) {
        return 0;
    }
    if ((s16)dx < 1) {
        return 0;
    }
    dx = (u16)(dx << 2);
    ax = arg_word(arg, 16);
    cx = (u16)(cx + ax);
    if (cx > SCREEN_HEIGHT) {
        cx = (u16)(cx - (SCREEN_HEIGHT - 1u));
        ax = (u16)(ax - cx);
    }
    if ((s16)ax < 0) {
        return 0;
    }
    bx = (u16)((u16)(SCREEN_WIDTH / 2u - dx) << 1);
    do {
        cx = dx;
        x86_rep_movsw(ds, &si, es, &di, &cx);
        si = (u16)(si + bx);
        di = (u16)(di + bx);
    } while (--ax != 0);
    return 0;
}

/* ---- the 8 by 8 cells ------------------------------------------------------------------------------ */

/* MCGA.OVL 06AF (export 5, gDrawCell8x8): DrawCell8x8(void far *src, col, y) copies 8 rows of 8 bytes
   from src to the draw page at column col (clamped to 0 to 39) and line y (clamped to 0 to 192) */
static u32 DrawCell8x8(const u8 *arg, tc_regs *r)
{
    u16 si = arg_word(arg, 6), ds = arg_word(arg, 8), ax, dx, di, es, bx, cx;
    (void)r;
    ax = arg_word(arg, 12);
    if ((s16)ax < 0) {
        ax = 0;
    }
    if ((s16)ax >= (s16)(SCREEN_HEIGHT - 7u)) {
        ax = SCREEN_HEIGHT - 8u;
    }
    ax = row_offset(ax);
    dx = arg_word(arg, 10);
    if ((s16)dx < 0) {
        dx = 0;
    }
    if ((s16)dx >= (s16)SCREEN_COLS) {
        dx = SCREEN_COLS - 1u;
    }
    di = (u16)(ax + (u16)(dx << 3));
    es = mcga_DrawPageSeg();
    bx = SCREEN_WIDTH - 8u;
    for (dx = 8; dx != 0; dx--) {
        cx = 4;
        x86_rep_movsw(ds, &si, es, &di, &cx);
        di = (u16)(di + bx);
    }
    return 0;
}

/* MCGA.OVL 0A97 (export 10, ds:806E): ReadCell8x8(page, void far *dst, col, y) copies the 8 by 8
   pixels at column col (clamped to 0 to 39), line y (clamped to 0 to 192) of a page into dst */
static u32 ReadCell8x8(const u8 *arg, tc_regs *r)
{
    u16 ds = mcga_PageSeg(arg_word(arg, 6)), di = arg_word(arg, 8), es = arg_word(arg, 10), si, bx, cx;
    (void)r;
    si = arg_word(arg, 12);
    if ((s16)si < 0) {
        si = 0;
    }
    if ((s16)si >= (s16)SCREEN_COLS) {
        si = SCREEN_COLS - 1u;
    }
    si = (u16)(si << 3);
    bx = arg_word(arg, 14);
    if ((s16)bx < 0) {
        bx = 0;
    }
    if ((s16)bx > (s16)(SCREEN_HEIGHT - 8u)) {
        bx = SCREEN_HEIGHT - 8u;
    }
    si = (u16)(si + row_offset(bx));
    for (cx = 8; cx != 0; cx--) {
        MOVSW(ds, &si, es, &di);
        MOVSW(ds, &si, es, &di);
        MOVSW(ds, &si, es, &di);
        MOVSW(ds, &si, es, &di);
        si = (u16)(si + (SCREEN_WIDTH - 8u));
    }
    return 0;
}

/* MCGA.OVL 0BD3 (near): bx = the four pixels of ax (two bytes of two nibbles) in the other order: each
   byte rotated by cl (4, as MirrorCell16 sets it: its nibbles swapped), then the bytes exchanged. Its
   tail (0BDC) is MirrorCell16's epilogue. */
static u16 MirrorPixelWord(u16 ax)
{
    u8 al = (u8)ax, ah = (u8)(ax >> 8);
    al = (u8)((al << 4) | (al >> 4));
    ah = (u8)((ah << 4) | (ah >> 4));
    return (u16)(((u16)al << 8) | ah);
}

/* MCGA.OVL 0B8F (export 12, ds:8076): MirrorCell16(void far *src, void far *dst) writes the 16 by 16
   cell of 4-bit pixels at src (16 rows of 8 bytes) mirrored left to right to dst: the row's four source
   words go, mirrored, to its words at di+6, +4, +2 and +0 */
static u32 MirrorCell16(const u8 *arg, tc_regs *r)
{
    u16 si = arg_word(arg, 6), ds = arg_word(arg, 8), di = arg_word(arg, 10), es = arg_word(arg, 12);
    u8 dl;
    (void)r;
    for (dl = 0x10; dl != 0; dl--) {
        tc_st_u16(TC_MKFP(es, (u16)(di + 6u)), MirrorPixelWord(x86_lodsw(ds, &si)));
        tc_st_u16(TC_MKFP(es, (u16)(di + 4u)), MirrorPixelWord(x86_lodsw(ds, &si)));
        tc_st_u16(TC_MKFP(es, (u16)(di + 2u)), MirrorPixelWord(x86_lodsw(ds, &si)));
        tc_st_u16(TC_MKFP(es, di), MirrorPixelWord(x86_lodsw(ds, &si)));
        di = (u16)(di + 8u);
    }
    return 0;
}

/* ---- the tiles ------------------------------------------------------------------------------------- */

/* MCGA.OVL 0C5C (export 13, ds:807A): SetTileSet(void far *pixels, void far *colourRows, void far
   *tileMap) names the tile set DrawIndexedTile draws from */
static u32 SetTileSet(const u8 *arg, tc_regs *r)
{
    (void)r;
    set_cs16(G_TILE_PIXELS_PTR, arg_word(arg, 6));
    set_cs16(G_TILE_PIXELS_PTR + 2u, arg_word(arg, 8));
    set_cs16(G_TILE_COLOURS_PTR, arg_word(arg, 10));
    set_cs16(G_TILE_COLOURS_PTR + 2u, arg_word(arg, 12));
    set_cs16(G_TILE_MAP_PTR, arg_word(arg, 14));
    set_cs16(G_TILE_MAP_PTR + 2u, arg_word(arg, 16));
    return 0;
}

/* MCGA.OVL 1C03 (export 30, ds:80BE): SetTileFormat(width, height, arg3, arg4) sets DrawIndexedTile's
   tile size in 8-pixel cells. A square size below 3 (signed) sets gTileMode = width AND 2, rows of
   width * 4 bytes, width * 8 rows, a requested size of 1 by 1; any other gives mode 4 (nothing drawn),
   8 rows of 4 bytes, cells 1 by 1, and keeps the size asked. arg3 and arg4 are stored and never read.
   QUIRK 1C3E: gTileParas comes from `rcr cx,1`, `shl cx,1`, `rcl cx,1`, `shl cx,1` with the carry
   `and cx,2` cleared: (width AND FFFEh) * 4, which is 0 for the 1 by 1 size (every tile then draws the
   first tile's pixels) and 8 for 2 by 2. */
static u32 SetTileFormat(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx;
    (void)r;
    set_cs16(G_TILE_FORMAT_ARG3, arg_word(arg, 10));
    set_cs16(G_TILE_FORMAT_ARG4, arg_word(arg, 12));
    ax = arg_word(arg, 6);
    bx = arg_word(arg, 8);
    if (ax == bx && (s16)ax < 3) {
        unsigned cf, out;
        set_cs16(G_TILE_REQ_WIDTH, 1);
        set_cs16(G_TILE_REQ_HEIGHT, 1);
        cx = (u16)(ax & 2u);                        /* and cx,2: the carry cleared */
        cf = 0;
        set_cs16(G_TILE_MODE, cx);
        cx = ax;
        out = cx & 1u;                              /* rcr cx,1 */
        cx = (u16)((cx >> 1) | (cf << 15));
        cf = out;
        cf = (cx >> 15) & 1u;                       /* shl cx,1 */
        cx = (u16)(cx << 1);
        out = (cx >> 15) & 1u;                      /* rcl cx,1 */
        cx = (u16)((cx << 1) | cf);
        cf = out;
        cx = (u16)(cx << 1);                        /* shl cx,1 */
        set_cs16(G_TILE_PARAS, cx);
        cx = (u16)(ax << 2);
        set_cs16(G_TILE_ROW_BYTES, cx);
        cx = (u16)(cx << 1);
        set_cs16(G_TILE_ROWS, cx);
        cx = (u16)(SCREEN_WIDTH - cx);
        set_cs16(G_TILE_DST_SKIP, cx);
    } else {
        set_cs16(G_TILE_MODE, 4);
        set_cs16(G_TILE_PARAS, 2);
        set_cs16(G_TILE_ROW_BYTES, 4);
        set_cs16(G_TILE_ROWS, 8);
        set_cs16(G_TILE_DST_SKIP, SCREEN_WIDTH - 8u);
        set_cs16(G_TILE_REQ_WIDTH, ax);
        set_cs16(G_TILE_REQ_HEIGHT, bx);
        ax = 1;
        bx = ax;
    }
    set_cs16(G_TILE_WIDTH_CELLS, ax);
    set_cs16(G_TILE_HEIGHT_CELLS, bx);
    return 0;
}

/* MCGA.OVL 0790 (near): draws the pixels of tile ax through gTileColourRow at es:di: the pixels at
   gTilePixelsPtr with ax * gTileParas (the low word of the product) added to its segment, gTileRows
   rows of gTileRowBytes bytes, two pixels a byte, high nibble first, gTileDstSkip after each row. Both
   counts are `loop` and `dec dx` counts: 0 is 65536. Its tail (07D8) is DrawIndexedTile's epilogue. */
static void DrawTilePixels(u16 ax, u16 es, u16 di)
{
    u16 si, ds, dx, cx;
    ax = (u16)((u32)ax * cs16(G_TILE_PARAS));
    si = cs16(G_TILE_PIXELS_PTR);
    ds = (u16)(ax + cs16(G_TILE_PIXELS_PTR + 2u));
    dx = cs16(G_TILE_ROWS);
    do {
        cx = cs16(G_TILE_ROW_BYTES);
        do {
            u8 al = x86_lodsb(ds, &si);
            x86_stosb(es, &di, cs8((u16)(G_TILE_COLOUR_ROW + (al >> 4))));
            x86_stosb(es, &di, cs8((u16)(G_TILE_COLOUR_ROW + (al & 0x0Fu))));
        } while (--cx != 0);
        di = (u16)(di + cs16(G_TILE_DST_SKIP));
    } while (--dx != 0);
}

/* MCGA.OVL 0717 (export 6, gDrawIndexedTile): DrawIndexedTile(tile, col, y) draws tile number tile of
   the set SetTileSet named: its kind (the tile map's byte at tile) picks a 16-colour row, copied into
   gTileColourRow, and the tile's pixels are drawn through it to page 0 at column col, line y. Neither
   is clamped: the line indexes gRowOffsets as a word index, wrapping in the driver's segment. The size
   is SetTileFormat's: `jmp cs:[gTileMode + gTileDrawModes]` reads its target from the table in the
   arena (0786 draws, 078E does not); a gTileMode that reads any other word jumps into code no C
   models, and the C stops there. */
static u32 DrawIndexedTile(const u8 *arg, tc_regs *r)
{
    u16 di, es, bx, si, target;
    (void)r;
    di = (u16)(cs16(G_TILE_MAP_PTR) + arg_word(arg, 6));
    es = cs16(G_TILE_MAP_PTR + 2u);
    bx = (u16)((u16)LD8(es, di) << 4);
    di = (u16)(bx + cs16(G_TILE_COLOURS_PTR));
    es = cs16(G_TILE_COLOURS_PTR + 2u);
    bx = 0x0F;
    do {
        set_cs8((u16)(G_TILE_COLOUR_ROW + bx), LD8(es, (u16)(bx + di)));
        bx--;
    } while ((s16)bx >= 0);
    di = row_offset(arg_word(arg, 10));
    di = (u16)(di + (u16)(arg_word(arg, 8) << 3));
    es = mcga_PageSeg(0);
    si = cs16(G_TILE_MODE);
    target = cs16((u16)(G_TILE_DRAW_MODES + si));
    if (target == TILE_DRAW) {
        DrawTilePixels(arg_word(arg, 6), es, di);
    } else if (target != TILE_NONE) {
        tc_fail(75, "DrawIndexedTile (MCGA.OVL 077B): gTileMode %04X jumps to %04X, no routine of the "
                "driver's C", si, target);
    }
    return 0;
}

/* ---- the line ---------------------------------------------------------------------------------------- */

/* the registers DrawClippedLine and its helpers share */
typedef struct {
    u16 ax, bx, cx, dx, si, di, bp;
} line_regs;

/* `imul dx` then `idiv bp`: ax = (ax * dx) / bp, signed, truncated. A zero divisor or a quotient past
   a word is the 8086's divide error (INT 0), which the original takes and the C does not model: it
   stops. With SetLineClip's rectangle the screen (the game never changes it) and coordinates whose
   differences fit a word, neither happens. */
static u16 imul_idiv(u16 ax, u16 dx, u16 bp, const char *where)
{
    s32 p = (s32)(s16)ax * (s32)(s16)dx, q;
    if (bp == 0) {
        tc_fail(75, "%s (MCGA.OVL): idiv by 0, the original's divide error (INT 0)", where);
    }
    q = p / (s32)(s16)bp;
    if (q > 32767 || q < -32768) {
        tc_fail(75, "%s (MCGA.OVL): idiv overflows (%ld / %d), the original's divide error (INT 0)",
                where, (long)p, (int)(s16)bp);
    }
    return (u16)q;
}

/* MCGA.OVL 08C3 (near): the x where the line from (ax, bx) to (cx, dx) meets y = si: ax = ax + (cx -
   ax) * (si - bx) / (dx - bx), bx = si; bp is left the divisor, cx, dx, si kept */
static void MoveToLineY(line_regs *g)
{
    u16 x = g->ax;
    g->bp = (u16)(g->dx - g->bx);
    g->ax = imul_idiv((u16)(g->cx - g->ax), (u16)(g->si - g->bx), g->bp, "MoveToLineY");
    g->ax = (u16)(g->ax + x);
    g->bx = g->si;
}

/* MCGA.OVL 08FA (near): the y where the line meets x = si: bx = bx + (dx - bx) * (si - ax) / (cx - ax),
   ax = si; bp is left the divisor */
static void MoveToLineX(line_regs *g)
{
    g->bp = (u16)(g->cx - g->ax);
    g->bx = (u16)(g->bx + imul_idiv((u16)(g->si - g->ax), (u16)(g->dx - g->bx), g->bp, "MoveToLineX"));
    g->ax = g->si;
}

/* MCGA.OVL 08A3 (near): outcode 0 and the impossible codes: nothing moved, carry clear */
static int ClipKeep(line_regs *g)
{
    (void)g;
    return 0;
}

/* MCGA.OVL 08A5 (near): moves the end (ax, bx) to the clip top along the line, carry set */
static int ClipToTop(line_regs *g)
{
    g->si = cs16(G_LINE_CLIP_TOP);
    MoveToLineY(g);
    return 1;
}

/* MCGA.OVL 08AF (near): moves the end to the clip bottom, by negating y (si, bx, dx) around
   MoveToLineY; carry set */
static int ClipToBottom(line_regs *g)
{
    g->si = (u16)-cs16(G_LINE_CLIP_BOTTOM);
    g->bx = (u16)-g->bx;
    g->dx = (u16)-g->dx;
    MoveToLineY(g);
    g->bx = (u16)-g->bx;
    g->dx = (u16)-g->dx;
    return 1;
}

/* MCGA.OVL 08DC (near): moves the end to the clip left edge, carry set */
static int ClipToLeft(line_regs *g)
{
    g->si = cs16(G_LINE_CLIP_LEFT);
    MoveToLineX(g);
    return 1;
}

/* MCGA.OVL 08E6 (near): moves the end to the clip right edge, by negating x (ax, cx, si) around
   MoveToLineX; carry set */
static int ClipToRight(line_regs *g)
{
    g->si = cs16(G_LINE_CLIP_RIGHT);
    g->ax = (u16)-g->ax;
    g->cx = (u16)-g->cx;
    g->si = (u16)-g->si;
    MoveToLineX(g);
    g->ax = (u16)-g->ax;
    g->cx = (u16)-g->cx;
    return 1;
}

/* MCGA.OVL 0883: gClipByOutcode, the edge each outcode moves an end to (above and left: the left edge;
   below and left: the bottom; above and right: the top; below and right: the right edge) */
static int (*const gClipByOutcode[16])(line_regs *) = {
    ClipKeep, ClipToTop, ClipToBottom, ClipKeep,
    ClipToLeft, ClipToLeft, ClipToBottom, ClipKeep,
    ClipToRight, ClipToTop, ClipToRight, ClipKeep,
    ClipKeep, ClipKeep, ClipKeep, ClipKeep,
};

/* MCGA.OVL 090F (near): the outcode of the point (ax, bx): 1 above the clip rectangle, 2 below, 4 left
   of it, 8 right of it (signed) */
static u16 LineOutcode(u16 ax, u16 bx)
{
    u16 si = 0;
    if ((s16)bx < (s16)cs16(G_LINE_CLIP_TOP)) {
        si |= 1u;
    }
    if ((s16)bx > (s16)cs16(G_LINE_CLIP_BOTTOM)) {
        si |= 2u;
    }
    if ((s16)ax < (s16)cs16(G_LINE_CLIP_LEFT)) {
        si |= 4u;
    }
    if ((s16)ax > (s16)cs16(G_LINE_CLIP_RIGHT)) {
        si |= 8u;
    }
    return si;
}

/* MCGA.OVL 093A (DrawLineInside, inside LineOutcode): the line from (ax, bx) to (cx, dx), inside the
   clip rectangle, in gLineColour on es. A horizontal line is a run of stosb and stosw (a byte first to
   an even offset, a byte last when the rest is odd), a vertical one a column from the upper end;
   others are Bresenham steps from the upper end, the error term starting at half the major length.
   The jumps after `sub` and `dec` are signed (jg, jl, jge), written here on the operands compared. */
static void DrawLineInside(line_regs *g, u16 es)
{
    u16 ax = g->ax, bx = g->bx, cx = g->cx, dx = g->dx, si, di, bp, t;
    if (dx == bx) {
        /* L_09A3: horizontal */
        if (!((s16)ax < (s16)cx)) {
            t = cx, cx = ax, ax = t;
        }
        cx = (u16)(cx - ax + 1u);
        bx = (u16)(row_offset(bx) + ax);
        di = bx;
        ax = cs16(G_LINE_COLOUR);
        ax = (u16)((ax & 0x00FFu) | (u16)(ax << 8));
        if (di & 1u) {
            x86_stosb(es, &di, (u8)ax);
            cx--;
        }
        dx = cx;
        for (cx = (u16)(cx >> 1); cx != 0; cx--) {
            x86_stosw(es, &di, ax);
        }
        if (dx & 1u) {
            x86_stosb(es, &di, (u8)ax);
        }
        return;
    }
    if ((s16)dx > (s16)bx) {
        dx = (u16)(dx - bx);
    } else {
        dx = (u16)(bx - dx);                        /* sub, neg */
        t = cx, cx = ax, ax = t;
        bx = (u16)(bx - dx);
    }
    bx = row_offset(bx);
    si = 1;
    if (cx == ax) {
        /* L_09D5: vertical */
        cx = (u16)(dx + 1u);
        bx = (u16)(bx + ax);
        ax = cs16(G_LINE_COLOUR);
        do {
            ST8(es, bx, ax);
            bx = (u16)(bx + SCREEN_WIDTH);
        } while (--cx != 0);
        return;
    }
    if ((s16)cx > (s16)ax) {
        cx = (u16)(cx - ax);
    } else {
        cx = (u16)(ax - cx);                        /* sub, neg */
        si = (u16)-si;
    }
    if (!((s16)cx >= (s16)dx)) {
        /* y-major: a step down every pixel, across when the error runs out */
        t = cx, cx = dx, dx = t;
        di = cx;
        bp = (u16)(cx >> 1);
        bx = (u16)(bx + ax);
        ax = cs16(G_LINE_COLOUR);
        for (;;) {
            int ge;
            ST8(es, bx, ax);
            if ((s16)cx < 1) {                      /* dec cx; jl */
                return;
            }
            cx--;
            bx = (u16)(bx + SCREEN_WIDTH);
            ge = (s16)bp >= (s16)dx;                /* sub bp,dx; jge */
            bp = (u16)(bp - dx);
            if (!ge) {
                bp = (u16)(bp + di);
                bx = (u16)(bx + si);
            }
        }
    } else {
        /* L_0983: x-major: a step across every pixel, down when the error runs out */
        di = cx;
        bp = (u16)(cx >> 1);
        bx = (u16)(bx + ax);
        ax = cs16(G_LINE_COLOUR);
        for (;;) {
            int ge;
            ST8(es, bx, ax);
            if ((s16)cx < 1) {
                return;
            }
            cx--;
            bx = (u16)(bx + si);
            ge = (s16)bp >= (s16)dx;
            bp = (u16)(bp - dx);
            if (!ge) {
                bp = (u16)(bp + di);
                bx = (u16)(bx + SCREEN_WIDTH);
            }
        }
    }
}

/* MCGA.OVL 080B (near): the line in ax = x1, bx = y1, cx = x2, dx = y2, colour di, on es (Cohen and
   Sutherland). The colour goes to gLineColour. A line with both ends inside is drawn as given.
   Otherwise, at each turn: the outcodes of both ends (the ends exchanged between the two, so the turn
   goes on with the second end in ax, bx); both 0 draws the line from there; a code in common ends it;
   else the second end is moved by its code's clip, and if that was ClipKeep (carry clear) the ends are
   exchanged back and the first end moved by its code. */
static void DrawClippedLine(line_regs *g, u16 es)
{
    u16 t;
    set_cs16(G_LINE_COLOUR, g->di);
    if ((s16)g->ax >= (s16)cs16(G_LINE_CLIP_LEFT) && (s16)g->ax <= (s16)cs16(G_LINE_CLIP_RIGHT)
        && (s16)g->bx >= (s16)cs16(G_LINE_CLIP_TOP) && (s16)g->bx <= (s16)cs16(G_LINE_CLIP_BOTTOM)
        && (s16)g->cx >= (s16)cs16(G_LINE_CLIP_LEFT) && (s16)g->cx <= (s16)cs16(G_LINE_CLIP_RIGHT)
        && (s16)g->dx >= (s16)cs16(G_LINE_CLIP_TOP) && (s16)g->dx <= (s16)cs16(G_LINE_CLIP_BOTTOM)) {
        DrawLineInside(g, es);
        return;
    }
    for (;;) {
        g->si = LineOutcode(g->ax, g->bx);
        t = g->cx, g->cx = g->ax, g->ax = t;
        t = g->bx, g->bx = g->dx, g->dx = t;
        g->di = g->si;
        g->si = LineOutcode(g->ax, g->bx);
        g->bp = (u16)(g->di | g->si);
        if (g->bp == 0) {
            DrawLineInside(g, es);
            return;
        }
        if (g->di & g->si) {
            return;
        }
        if (gClipByOutcode[g->si](g)) {
            continue;
        }
        t = g->cx, g->cx = g->ax, g->ax = t;
        t = g->bx, g->bx = g->dx, g->dx = t;
        gClipByOutcode[g->di](g);
    }
}

/* MCGA.OVL 07E3 (export 7, gDrawLine): DrawLine(x1, y1, x2, y2, colour) draws a line on the draw page,
   clipped to SetLineClip's rectangle */
static u32 DrawLine(const u8 *arg, tc_regs *r)
{
    line_regs g;
    u16 es = mcga_DrawPageSeg();
    (void)r;
    g.ax = arg_word(arg, 6);
    g.bx = arg_word(arg, 8);
    g.cx = arg_word(arg, 10);
    g.dx = arg_word(arg, 12);
    g.di = arg_word(arg, 14);
    g.si = 0;
    g.bp = 0;
    DrawClippedLine(&g, es);
    return 0;
}

/* ---- the filled and the XORed rectangles ------------------------------------------------------------- */

/* MCGA.OVL 09F5 (export 9, gFillRect): FillRect(x1, y1, x2, y2, colour) fills the rectangle on the draw
   page, corners included. x1 or y1 past the screen (signed) fills nothing, negative ones are 0; x2 and
   y2 negative are 0 and past the screen fill nothing; x1 > x2 or y1 > y2 fills nothing. The colour
   byte goes to gFillColour first. QUIRK 0A52 to 0A61: the clamps of x2 to 319 and y2 to 199 are never
   taken (the tests before them have ended the routine). */
static u32 FillRect(const u8 *arg, tc_regs *r)
{
    u16 ax, bx, cx, dx, si, di, bp, es;
    u8 al;
    (void)r;
    set_cs8(G_FILL_COLOUR, (u8)arg_word(arg, 14));
    es = mcga_DrawPageSeg();
    ax = arg_word(arg, 6);
    if ((s16)ax >= (s16)SCREEN_WIDTH) {
        return 0;
    }
    if ((s16)ax < 0) {
        ax = 0;
    }
    bx = arg_word(arg, 8);
    if ((s16)bx >= (s16)SCREEN_HEIGHT) {
        return 0;
    }
    if ((s16)bx < 0) {
        bx = 0;
    }
    cx = arg_word(arg, 10);
    if ((s16)cx < 0) {
        cx = 0;
    }
    if ((s16)cx >= (s16)SCREEN_WIDTH) {
        return 0;
    }
    dx = arg_word(arg, 12);
    if ((s16)dx < 0) {
        dx = 0;
    }
    if ((s16)dx >= (s16)SCREEN_HEIGHT) {
        return 0;
    }
    if (ax > cx || bx > dx) {
        return 0;
    }
    si = ax;
    di = (u16)(ax + row_offset(bx));
    bp = (u16)(dx - bx + 1u);
    cx = (u16)(cx + 1u - si);
    bx = (u16)(SCREEN_WIDTH - cx);
    dx = cx;
    al = cs8(G_FILL_COLOUR);
    for (;;) {
        x86_rep_stosb(es, &di, &cx, al);
        cx = dx;
        di = (u16)(di + bx);
        if (!((s16)bp > 1)) {                       /* dec bp; jg */
            break;
        }
        bp--;
    }
    return 0;
}

/* MCGA.OVL 2617 (export 8, gXorRect): XorRect(x1, y1, x2, y2, colour) XORs colour into every pixel of
   the rectangle on the draw page, corners included, each clamped to the screen (signed), the corners
   in either order. A row is a first group of gXorBytes pixels (the whole width when it is 8 or less,
   else to the end of x1's 8-pixel group and through x2's: (8 - (x1 AND 7)) + (x2 AND 7) + 1), then
   gBlitWidth groups of 8. The pixels outside those groups go by the masks gBitMaskTable has for a
   width of gXorBytes at bit 0: its first byte (gLeftMask) XORs from the left, MSB first, while its bits
   are set, before the groups; its second byte (gRightMask, when gXorBytes is over 1) after them. State
   at cs:2027 to 2042 (shared with DrawGlyph): gLeftMask is written x1 AND 7 first, then the mask. */
static u32 XorRect(const u8 *arg, tc_regs *r)
{
    u16 di, dx, ax, cx, bp, es, t;
    u8 bl, bh, cl, ch, dl, dh, al;
    (void)r;
    di = arg_word(arg, 6);
    if ((s16)di < 0) {
        di = 0;
    }
    if ((s16)di >= (s16)SCREEN_WIDTH) {
        di = SCREEN_WIDTH - 1u;
    }
    dx = arg_word(arg, 8);
    if ((s16)dx < 0) {
        dx = 0;
    }
    if ((s16)dx >= (s16)SCREEN_HEIGHT) {
        dx = SCREEN_HEIGHT - 1u;
    }
    ax = arg_word(arg, 10);
    if ((s16)ax < 0) {
        ax = 0;
    }
    if ((s16)ax >= (s16)SCREEN_WIDTH) {
        ax = SCREEN_WIDTH - 1u;
    }
    if ((s16)ax < (s16)di) {
        t = di, di = ax, ax = t;
    }
    bl = (u8)(di & 7u);
    set_cs8(G_LEFT_MASK, bl);
    bh = (u8)(ax & 7u);
    ax = (u16)(ax - di + 1u);
    cx = ax;
    if ((s16)ax > 8) {
        cx = (u8)(8u - bl + bh + 1u);               /* ch 0, cl the byte sum */
    }
    set_cs16(G_XOR_BYTES, cx);
    ax = (u16)((u16)(ax - cx) >> 3);
    set_cs8(G_BLIT_WIDTH, (u8)ax);
    ax = arg_word(arg, 12);
    if ((s16)ax < 0) {
        ax = 0;
    }
    if ((s16)ax >= (s16)SCREEN_HEIGHT) {
        ax = SCREEN_HEIGHT - 1u;
    }
    if ((s16)ax < (s16)dx) {
        t = dx, dx = ax, ax = t;
    }
    set_cs8(G_BLIT_Y, (u8)dx);
    ax = (u16)(ax - dx + 1u);
    set_cs8(G_BLIT_ROWS, (u8)ax);
    set_cs8(G_BLIT_COLOUR, (u8)arg_word(arg, 14));
    cx = (u16)(cs16(G_XOR_BYTES) - 1u);
    bp = cx;
    cl = (u8)((u8)((u8)cx + 7u) >> 3);
    bp = cs16((u16)(G_BIT_MASK_TABLE + (u16)(bp << 1)));
    bp = cs16(bp);
    set_cs8(G_LEFT_MASK, cs8(bp));
    al = 0;
    if (cl != 0) {
        bp = (u16)(bp + 1u);
        al = cs8(bp);
    }
    set_cs8(G_RIGHT_MASK, al);
    di = (u16)(di + row_offset(dx));
    ax = (u16)(SCREEN_WIDTH - cs16(G_XOR_BYTES) - (u16)((u16)cs8(G_BLIT_WIDTH) << 3));
    set_cs16(G_BLIT_ROW_SKIP, ax);
    es = mcga_DrawPageSeg();
    ch = cs8(G_BLIT_WIDTH);
    bh = cs8(G_BLIT_ROWS);
    bl = cs8(G_BLIT_COLOUR);
    dl = 0x80;
    do {
        al = cs8(G_LEFT_MASK);
        for (cl = dl; (al & cl) != 0;) {
            ST8(es, di, LD8(es, di) ^ bl);
            di = (u16)(di + 1u);
            cl = (u8)(cl >> 1);
        }
        if (ch != 0) {
            dh = ch;
            do {
                cl = dl;
                do {
                    ST8(es, di, LD8(es, di) ^ bl);
                    di = (u16)(di + 1u);
                    cl = (u8)(cl >> 1);
                } while (cl != 0);
            } while (--dh != 0);
        }
        al = cs8(G_RIGHT_MASK);
        if (al != 0) {
            for (cl = dl; (al & cl) != 0;) {
                ST8(es, di, LD8(es, di) ^ bl);
                di = (u16)(di + 1u);
                cl = (u8)(cl >> 1);
            }
        }
        di = (u16)(di + cs16(G_BLIT_ROW_SKIP));
    } while (--bh != 0);
    return 0;
}

/* ---- the table ------------------------------------------------------------------------------------- */

/* offset, name, C, argument bytes read, bytes of DX:AX returned, other registers answered */
const tc_driver_routine mcga_rect_routines[] = {
    {0x0516, "RestoreRectToPage", RestoreRectToPage, 12, 0, 0},
    {0x0536, "RestoreRect", RestoreRect, 12, 0, 0},
    {0x05D4, "CopyRegion", CopyRegion, 16, 0, 0},
    {0x06AF, "DrawCell8x8", DrawCell8x8, 8, 0, 0},
    {0x0717, "DrawIndexedTile", DrawIndexedTile, 6, 0, 0},
    {0x07E3, "DrawLine", DrawLine, 10, 0, 0},
    {0x09F5, "FillRect", FillRect, 10, 0, 0},
    {0x0A97, "ReadCell8x8", ReadCell8x8, 10, 0, 0},
    {0x0B8F, "MirrorCell16", MirrorCell16, 8, 0, 0},
    {0x0C5C, "SetTileSet", SetTileSet, 12, 0, 0},
    {0x1ABD, "SaveRectFromPage", SaveRectFromPage, 12, 2, 0},
    {0x1ADC, "SaveRect", SaveRect, 12, 2, 0},
    {0x1C03, "SetTileFormat", SetTileFormat, 8, 0, 0},
    {0x2617, "XorRect", XorRect, 10, 0, 0},
};
const unsigned mcga_rect_count = sizeof mcga_rect_routines / sizeof mcga_rect_routines[0];
