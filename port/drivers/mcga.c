/* mcga.c: MCGA.OVL in C, its entry and link, the page helpers, the small exports and the routine table
   (playbook 80, M4; src/drivers/MCGA.ASM is the reference, mcga.h says how the C follows it).

   The other exports are in mcga_rect.c (rectangles, cells, tiles, lines), mcga_cell.c (the sprite
   cutters and the text), mcga_shape.c (DrawShape and its mirror) and mcga_mask.c (the masked and the
   remapped shape draws). */
#include "mcga.h"

u16 mcga_cs;

/* ---- the page helpers ---------------------------------------------------------------------------- */

static u16 page_segs_entry(u16 page)
{
    farptr segs = cs32(G_PAGE_SEGS_PTR);
    return tc_ld_u16(tc_fp_add(segs, (u16)((page & 0x1Fu) << 1)));
}

static u16 draw_page(void)
{
    return tc_ld_u16(cs32(G_DRAW_PAGE_PTR));
}

/* MCGA.OVL 0AFA (near): the segment of the page after the draw page, gPageSegs[(gDrawPage + 1) & 1Fh] */
u16 mcga_NextPageSeg(void)
{
    return page_segs_entry((u16)(draw_page() + 1u));
}

/* MCGA.OVL 0B1D (near): the segment of the draw page, gPageSegs[gDrawPage & 1Fh] */
u16 mcga_DrawPageSeg(void)
{
    return page_segs_entry(draw_page());
}

/* MCGA.OVL 0B3F (near): the segment of page ax, gPageSegs[ax & 1Fh] */
u16 mcga_PageSeg(u16 page)
{
    return page_segs_entry(page);
}

/* ---- the game's routines, through the import slots ----------------------------------------------- */

/* `call dword ptr cs:[gMemAllocPtr]` with size low, size high and flags pushed (in that order from the
   top of the stack) and DS as given: MemAlloc(unsigned long size, int flags), dx:ax the block. The
   game's MemAlloc is C (src/E19BC.C, `farptr MemAlloc(u32, s16)` in the port); code with no C runs
   as tc_code_far sends it. */
farptr mcga_mem_alloc(u16 ds, u16 size_low, u16 size_high, u16 flags, const char *site)
{
    farptr fn = cs32(G_MEM_ALLOC_PTR), p;
    tc_fn c = tc_code_find(fn);
    u16 caller_ds = tc_ds;
    tc_ds = ds;
    if (c != 0) {
        p = ((farptr (*)(u32, s16))c)((u32)size_low | ((u32)size_high << 16), (s16)flags);
    } else {
        u8 b[6];
        tc_va16(b, size_low);
        tc_va16(b + 2, size_high);
        tc_va16(b + 4, flags);
        p = tc_code_far(fn, b, 6, site);
    }
    tc_ds = caller_ds;
    return p;
}

/* ---- entry and link ------------------------------------------------------------------------------- */

#define DRIVER_END 0x2C38u          /* the end of the image */
#define LINK_DRIVER 0x0BE6u

/* MCGA.OVL 0C3F, reached by the jump at 0000: DriverEntry returns bx = the driver's size in paragraphs
   and ax = the offset of LinkDriver. The carry is the last bit `shr bx,cl` shifted out (bit 3 of
   2C38h, set), which the game does not read. QUIRK 0C45: es:di = A000:0000 is set and popped. */
static u32 mcga_DriverEntry(const u8 *arg, tc_regs *r)
{
    (void)arg;
    r->bx = (u16)((DRIVER_END >> 4) + 1u);
    r->flags = (DRIVER_END >> 3) & 1u ? TC_FLAG_CF : 0;
    r->ax = LINK_DRIVER;
    return LINK_DRIVER;
}

/* MCGA.OVL 0BE6: LinkDriver(void far *table) walks the game's slot table and the driver's (cs:0003),
   SLOT_COUNT slots: a game slot that holds a pointer is copied into the driver (an import); a zero one
   gets the driver's pointer, its segment the driver's (an export). Carry clear, or set when an export
   slot of the driver is zero (the walk stops there). AX comes back as it went in. */
static u32 mcga_LinkDriver(const u8 *arg, tc_regs *r)
{
    u16 si = TC_OFF(arg_far(arg, 6)), ds = TC_SEG(arg_far(arg, 6)), di = 3, cx = cs16(G_SLOT_COUNT);
    u16 es = mcga_cs;
    r->flags = 0;
    do {
        u16 lo = x86_lodsw(ds, &si);
        u16 hi = x86_lodsw(ds, &si);
        if ((lo | hi) != 0) {
            x86_stosw(es, &di, lo);             /* an import */
            x86_stosw(es, &di, hi);
        } else {
            u16 dx;
            si = (u16)(si - 4u);
            dx = tc_ld_u16(TC_MKFP(es, di));
            di = (u16)(di + 4u);
            if (dx == 0) {
                r->flags = TC_FLAG_CF;          /* an export the driver does not have */
                return 0;
            }
            tc_st_u16(TC_MKFP(ds, si), dx);     /* an export: the offset, then the driver's segment */
            si = (u16)(si + 2u);
            tc_st_u16(TC_MKFP(ds, si), es);
            si = (u16)(si + 2u);
        }
    } while (--cx != 0);
    return 0;
}

/* ---- the small exports ---------------------------------------------------------------------------- */

/* MCGA.OVL 04F8 (export 0, gCopyPage): CopyPage(srcSeg, dstSeg) copies a whole 64000-byte page */
static u32 mcga_CopyPage(const u8 *arg, tc_regs *r)
{
    u16 ds = arg_word(arg, 6), si = 0, es = arg_word(arg, 8), di = 0, cx = PAGE_WORDS;
    (void)r;
    x86_rep_movsw(ds, &si, es, &di, &cx);
    return 0;
}

/* MCGA.OVL 0693 (export 4, gClearPage): fills the draw page with colour 0 */
static u32 mcga_ClearPage(const u8 *arg, tc_regs *r)
{
    u16 es = mcga_DrawPageSeg(), di = 0, cx;
    (void)arg;
    (void)r;
    for (cx = PAGE_WORDS; cx != 0; cx--) {
        x86_stosw(es, &di, 0);
    }
    return 0;
}

/* MCGA.OVL 0B56 (export 11, gGetPixel): GetPixel(x, y), the draw page's pixel at (x, y), 0 off the
   screen */
static u32 mcga_GetPixel(const u8 *arg, tc_regs *r)
{
    s16 x = (s16)arg_word(arg, 6), y = (s16)arg_word(arg, 8);
    (void)r;
    if (y < 0 || y >= (s16)SCREEN_HEIGHT || x < 0 || x >= (s16)SCREEN_WIDTH) {
        return 0;
    }
    return tc_ld_u8(TC_MKFP(mcga_DrawPageSeg(), (u16)(row_offset((u16)y) + (u16)x)));
}

/* MCGA.OVL 15DA (export 22, gPutPixel): PutPixel(x, y, colour) sets a pixel of the draw page; off the
   screen it does nothing */
static u32 mcga_PutPixel(const u8 *arg, tc_regs *r)
{
    s16 x = (s16)arg_word(arg, 6), y = (s16)arg_word(arg, 8);
    (void)r;
    if (y < 0 || y >= (s16)SCREEN_HEIGHT || x < 0 || x >= (s16)SCREEN_WIDTH) {
        return 0;
    }
    tc_st_u8(TC_MKFP(mcga_DrawPageSeg(), (u16)(row_offset((u16)y) + (u16)x)), (u8)arg_word(arg, 10));
    return 0;
}

/* MCGA.OVL 0FCD (export 18, gCopyToPlanar): nothing to do in a chunky mode */
static u32 mcga_CopyToPlanar(const u8 *arg, tc_regs *r)
{
    (void)arg;
    (void)r;
    return 0;
}

/* MCGA.OVL 0FD2 (export 19, ds:8092): SwapPageWithScreen(page) exchanges a page and page 0, word by
   word (the page's word to the screen, the screen's to the page). Not called by the game. */
static u32 mcga_SwapPageWithScreen(const u8 *arg, tc_regs *r)
{
    u16 ds = mcga_PageSeg(arg_word(arg, 6)), es = mcga_PageSeg(0), si = 0, di = 0, cx;
    (void)r;
    for (cx = PAGE_WORDS; cx != 0; cx--) {
        u16 ax = tc_ld_u16(TC_MKFP(es, di));
        x86_stosw(es, &di, x86_lodsw(ds, &si));
        tc_st_u16(TC_MKFP(ds, (u16)(si - 2u)), ax);
    }
    return 0;
}

/* MCGA.OVL 1612 (export 23, gRestoreDrawState): puts back the region SaveDrawState kept */
static u32 mcga_RestoreDrawState(const u8 *arg, tc_regs *r)
{
    (void)arg;
    (void)r;
    set_cs16(G_REGION_LEFT, cs16(G_SAVED_LEFT));
    set_cs16(G_REGION_RIGHT, cs16(G_SAVED_RIGHT));
    set_cs16(G_REGION_TOP, cs16(G_SAVED_TOP));
    set_cs16(G_REGION_BOTTOM, cs16(G_SAVED_BOTTOM));
    return 0;
}

/* MCGA.OVL 1A7A (export 25, gSaveDrawState): keeps the region and sets it to the whole screen (0, 40,
   0, 200) */
static u32 mcga_SaveDrawState(const u8 *arg, tc_regs *r)
{
    (void)arg;
    (void)r;
    set_cs16(G_SAVED_LEFT, cs16(G_REGION_LEFT));
    set_cs16(G_REGION_LEFT, 0);
    set_cs16(G_SAVED_RIGHT, cs16(G_REGION_RIGHT));
    set_cs16(G_REGION_RIGHT, SCREEN_COLS);
    set_cs16(G_SAVED_TOP, cs16(G_REGION_TOP));
    set_cs16(G_REGION_TOP, 0);
    set_cs16(G_SAVED_BOTTOM, cs16(G_REGION_BOTTOM));
    set_cs16(G_REGION_BOTTOM, SCREEN_HEIGHT);
    return 0;
}

/* MCGA.OVL 1B9D (export 27, ds:80B2): SetLineClip(left, top, right, bottom), DrawLine's rectangle in
   pixels. Not called by the game. */
static u32 mcga_SetLineClip(const u8 *arg, tc_regs *r)
{
    (void)r;
    set_cs16(G_LINE_CLIP_LEFT, arg_word(arg, 6));
    set_cs16(G_LINE_CLIP_TOP, arg_word(arg, 8));
    set_cs16(G_LINE_CLIP_RIGHT, arg_word(arg, 10));
    set_cs16(G_LINE_CLIP_BOTTOM, arg_word(arg, 12));
    return 0;
}

/* MCGA.OVL 1BC0 (export 28, gSetRegion): SetRegion(left, right, top, bottom), the region the shape
   draws clip to: columns, then lines */
static u32 mcga_SetRegion(const u8 *arg, tc_regs *r)
{
    (void)r;
    set_cs16(G_REGION_LEFT, arg_word(arg, 6));
    set_cs16(G_REGION_RIGHT, arg_word(arg, 8));
    set_cs16(G_REGION_TOP, arg_word(arg, 10));
    set_cs16(G_REGION_BOTTOM, arg_word(arg, 12));
    return 0;
}

/* MCGA.OVL 1BE3 (export 29, gSetPalette): SetPalette(void far *rgb) loads the 256 DAC registers from
   768 bytes, INT 10h AX=1012h */
static u32 mcga_SetPalette(const u8 *arg, tc_regs *r)
{
    tc_regs v = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    (void)r;
    v.ax = 0x1012u;
    v.bx = 0;
    v.cx = 0x100u;
    v.dx = TC_OFF(arg_far(arg, 6));
    v.es = TC_SEG(arg_far(arg, 6));
    v.ds = tc_ds;
    x86_int(0x10, &v);
    return 0;
}

/* MCGA.OVL 1FF1 (export 32, gSaveRectSize): SaveRectSize(cols, rows), the bytes SaveRect needs:
   cols * rows * 8, cols clamped to 1 to 40 and rows to 1 to 200 (the product's low word, shifted) */
static u32 mcga_SaveRectSize(const u8 *arg, tc_regs *r)
{
    s16 cols = (s16)arg_word(arg, 6), rows = (s16)arg_word(arg, 8);
    (void)r;
    if (cols < 1) {
        cols = 1;
    }
    if (cols > (s16)SCREEN_COLS) {
        cols = SCREEN_COLS;
    }
    if (rows < 1) {
        rows = 1;
    }
    if (rows > (s16)SCREEN_HEIGHT) {
        rows = SCREEN_HEIGHT;
    }
    return (u16)((u16)((u16)cols * (u16)rows) << 3);
}

/* MCGA.OVL 277B (export 35, gSetRemapTable): SetRemapTable(void far *colours) copies 16 colours into
   gRemapTable */
static u32 mcga_SetRemapTable(const u8 *arg, tc_regs *r)
{
    farptr p = arg_far(arg, 6);
    u16 ds = TC_SEG(p), si = TC_OFF(p), di = G_REMAP_TABLE, cx = 8;
    (void)r;
    x86_rep_movsw(ds, &si, mcga_cs, &di, &cx);
    return 0;
}

/* ---- the table ------------------------------------------------------------------------------------ */

/* offset, name, C, argument bytes read, bytes of DX:AX returned, other registers answered */
const tc_driver_routine mcga_core_routines[] = {
    {0x0000, "DriverEntry", mcga_DriverEntry, 0, 2, TC_ANSWERS_BX | TC_ANSWERS_CF},
    {0x0BE6, "LinkDriver", mcga_LinkDriver, 4, 0, TC_ANSWERS_CF},
    {0x04F8, "CopyPage", mcga_CopyPage, 4, 0, 0},
    {0x0693, "ClearPage", mcga_ClearPage, 0, 0, 0},
    {0x0B56, "GetPixel", mcga_GetPixel, 4, 2, 0},
    {0x0FCD, "CopyToPlanar", mcga_CopyToPlanar, 0, 0, 0},
    {0x0FD2, "SwapPageWithScreen", mcga_SwapPageWithScreen, 2, 0, 0},
    {0x15DA, "PutPixel", mcga_PutPixel, 6, 0, 0},
    {0x1612, "RestoreDrawState", mcga_RestoreDrawState, 0, 0, 0},
    {0x1A7A, "SaveDrawState", mcga_SaveDrawState, 0, 0, 0},
    {0x1B9D, "SetLineClip", mcga_SetLineClip, 8, 0, 0},
    {0x1BC0, "SetRegion", mcga_SetRegion, 8, 0, 0},
    {0x1BE3, "SetPalette", mcga_SetPalette, 4, 0, 0},
    {0x1FF1, "SaveRectSize", mcga_SaveRectSize, 4, 2, 0},
    {0x277B, "SetRemapTable", mcga_SetRemapTable, 4, 0, 0},
};
const unsigned mcga_core_count = sizeof mcga_core_routines / sizeof mcga_core_routines[0];

static const struct {
    const tc_driver_routine *t;
    const unsigned *n;
} tables[] = {
    {mcga_core_routines, &mcga_core_count},
    {mcga_rect_routines, &mcga_rect_count},
    {mcga_cell_routines, &mcga_cell_count},
    {mcga_shape_routines, &mcga_shape_count},
    {mcga_mask_routines, &mcga_mask_count},
};

/* routine i of all the files' tables, in order (the per-function test lists them); 0 past the end */
const tc_driver_routine *mcga_routine(unsigned i)
{
    unsigned k;
    for (k = 0; k < sizeof tables / sizeof tables[0]; k++) {
        if (i < *tables[k].n) {
            return &tables[k].t[i];
        }
        i -= *tables[k].n;
    }
    return 0;
}

void mcga_register(u16 seg)
{
    const tc_driver_routine *d;
    unsigned i;
    mcga_cs = seg;
    for (i = 0; (d = mcga_routine(i)) != 0; i++) {
        tc_code_register_args(TC_MKFP(seg, d->off), d->fn, d->name);
    }
}
