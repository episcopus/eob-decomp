/* Unit at EOB.EXE 1A29:000F (470 bytes, 7 functions): the region front ends of the screen
   driver, plus the routine that selects the current region. The region table at ds:009F holds
   one rectangle per region in a 16-byte record; the rectangles may overlap (record 0 is the
   whole screen, 0,0,40,200, and records 1 to 3 lie inside it; the initial table is at EOB.EXE
   file offset 0x23B7F). The table is read by many front ends, for example 082C:0650/0696/06DC/071E,
   0D1C:A0F7, 0EF3:8387, 1547:1E47, 16A5:0867, 1DE7:005F and SelectRegion 1A29:019A, and written
   by the setter 1DE7:02FE (words 0 to 3) and, for words 4 to 7, by 1DE7:0009 SetTextWindow; each
   record gives the region's left edge in 8-pixel columns, its top line, its width in 8-pixel
   columns and its height in lines. A front end's `region` argument is
   an index into the table, not a byte offset. A front end adds the selected region's origin
   (ds:FE4A in pixels, ds:FE46 in lines) to the caller's region-relative coordinates and, when
   its region argument is not the one selected, selects it first, then
   calls one of the driver's far drawing pointers. SelectRegion expands a record into the globals
   ds:FE48/FE4A/FE4C/FE46/FE44 and hands the four edges to the driver's region setter.
   The driver pointer table is filled outside the resident code [H]. Compiled with -G
   (`inc sp; inc sp` clean-ups). */

/*@flags -ml -O -Z -G */

struct Region {             /* 16 bytes: left, top, width, height, then four words of saved state */
    int left;               /* left edge, in 8-pixel columns */
    int top;                /* top edge, in lines */
    int width;              /* width, in 8-pixel columns */
    int height;             /* height, in lines */
    /* words 4..7: per-record text-window state, copied out by 1DE7:0009 (SetTextWindow) for the
       window it switches away from (src/E1DE7.C:80-83) and copied in for the window it switches to
       (src/E1DE7.C:88-95). textAttr1 and textAttr2 are the window's two text attributes (gTextAttr1,
       gTextAttr2, src/E1DE7.C:38-40; their game meaning is not established [H]); textCol and textRow
       are its cursor column and row (gTextCol, gTextRow, src/E1DE7.C:41-42). This unit does not
       use them. */
    int textAttr1;
    int textAttr2;
    int textCol;
    int textRow;
};

extern struct Region gRegions[]; /*@ ds:009F */  /* the region table, read by 082C (four sites, 082C:0650/0696/06DC/071E), 1DE7:005F and SelectRegion (1A29:019A); indexed by a region number, so gRegions[region] is region * 16 */
extern int gCurRegion;           /*@ ds:8108 */  /* index of the region selected last, set by SelectRegion and compared by the front ends and by 1DE7:0333; 1DE7 keeps its own record index at ds:811F, and nothing here says the two agree */
extern int gRegionLeft;          /*@ ds:FE48 */  /* selected region's left edge, in 8-pixel columns */
extern int gRegionLeftPx;        /*@ ds:FE4A */  /* selected region's left edge, in pixels (left * 8) */
extern int gRegionRight;         /*@ ds:FE4C */  /* selected region's right edge (left + width), in 8-pixel columns */
extern int gRegionTop;           /*@ ds:FE46 */  /* selected region's top line */
extern int gRegionBottom;        /*@ ds:FE44 */  /* selected region's bottom line (top + height), in lines */

/* The driver's drawing entry points. No writer of these words exists in the resident code, so
   they are filled at load time [H]; their meanings are unknown and their first argument `mode`
   is passed straight through. This unit calls each of the five pointers other than ds:80C2
   (80A6, 80D6, 80DA, 809A, 8096) once, from its own front end (for example 1A29:0082 `lcall
   [0x80a6]`), and calls ds:80C2 once from DrawShape (1A29:0044 `lcall [0x80c2]`). It is not
   the only caller of ds:80C2: ShowPointer (1BBE:00ED, src/E1BBE.ASM:252-258) calls the same
   pointer at 1BBE:01F4 (`mov ax, 0` at 1BBE:01F0) with a literal 0 for `mode`, bypassing this
   unit. gSetRegion below is called once, in SelectRegion (1A29:01DB `lcall [0x80b6]`).
   The `gDraw` prefix is a family guess: only ds:80C2 is evidenced as drawing (ShowPointer
   draws the mouse shape through it, src/E1BBE.ASM:252-258); ds:8062, another pointer of the
   same kind, is gDrawLine (src/E1A1F.C:6), which supports the family but is not evidence for
   these six. */
extern void (far *gDrawShape)(int mode, void far *src, int x, int y);        /*@ ds:80C2 */
extern void (far *gDraw80A6)(int mode, void far *src, int x, int y);        /*@ ds:80A6 */
extern void (far *gDraw80D6)(int mode, void far *src, int x, int y);        /*@ ds:80D6 */
extern void (far *gDraw80DA)(int mode, void far *src, int x, int y);        /*@ ds:80DA */
extern void (far *gDraw809A)(int mode, void far *src, int x, int y, int extra); /*@ ds:809A */
extern void (far *gDraw8096)(int mode, void far *src, int x, int y, int extra); /*@ ds:8096 */
/* called with (left, right, top, bottom) after a region is selected */
extern void (far *gSetRegion)(int left, int right, int top, int bottom);    /*@ ds:80B6 */

void SelectRegion(int region);

/* 1A29:000F: draws the shape record `src` through the driver entry gDrawShape (ds:80C2). It
   does nothing when the source far pointer is null, selects `region` when it is not the current
   one, then calls the driver with x and y offset by the region's origin: its left edge in pixels
   for x, its top line for y. The mouse pointer
   bypasses this front end: ShowPointer (1BBE:00ED) calls the driver pointer ds:80C2 directly at
   1BBE:01F4 (`mov ax, 0` at 1BBE:01F0; src/E1BBE.ASM:252-258), passing the shape record
   ([816E]:[8170], gPointerShapeOff and gPointerShapeSeg, re/renames.toml) as `src` at the
   unclipped absolute position gShapeX/gShapeY, with no region origin added, and a literal 0 as
   `mode`. This front end is called from 20 functions (9 in the resident code, 11 in the
   overlays; for example 082C:063B, the `lcall 1A29:000F` at 082C:0675), which pass
   region-relative coordinates. */
/*@ 1A29:000F */ void DrawShape(int mode, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShape(mode, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:004D: Purpose unknown: a front end over the driver pointer ds:80A6, identical to
   1A29:000F otherwise (null check, lazy region select, region-relative x and y). */
/*@ 1A29:004D */ void f1A29_004D(int mode, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDraw80A6(mode, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:008B: Purpose unknown: a front end over the driver pointer ds:80D6, identical to
   1A29:000F otherwise (null check, lazy region select, region-relative x and y). */
/*@ 1A29:008B */ void f1A29_008B(int mode, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDraw80D6(mode, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:00C9: Purpose unknown: a front end over the driver pointer ds:80DA, identical to
   1A29:000F otherwise (null check, lazy region select, region-relative x and y). */
/*@ 1A29:00C9 */ void f1A29_00C9(int mode, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDraw80DA(mode, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:0107: Purpose unknown: a front end over the driver pointer ds:809A, like 1A29:000F but
   with one extra word forwarded to the driver. No caller of it was found in EOB.EXE or its
   overlays (make packet), so both its role and the meaning of `extra` are unknown. */
/*@ 1A29:0107 */ void f1A29_0107(int mode, void far *src, int x, int y, int region, int extra)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDraw809A(mode, src, x + gRegionLeftPx, y + gRegionTop, extra);
}

/* 1A29:0148: Purpose unknown: a front end over the driver pointer ds:8096, like 1A29:000F but
   with one extra word forwarded to the driver. No caller of it was found in EOB.EXE or its
   overlays (make packet), so both its role and the meaning of `extra` are unknown. */
/*@ 1A29:0148 */ void f1A29_0148(int mode, void far *src, int x, int y, int region, int extra)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDraw8096(mode, src, x + gRegionLeftPx, y + gRegionTop, extra);
}

/* 1A29:0189: makes `region` the current one. `region` is an index into gRegions (the record is
   at region * 16, the shift 1DE7:0305..0309 computes): loads the record, expands it into the
   left, right, top and bottom globals (right and bottom being left + width and top + height),
   then calls the driver's region setter gSetRegion(left, right, top, bottom). Called by every
   front end above and by 1DE7:02FE when the record it rewrote is the current one (1DE7:0333
   `cmp si, [8108]`; 1DE7:0337 `jne` skips the call unless it is). */
/*@ 1A29:0189 */ void SelectRegion(int region)
{
    gCurRegion = region;
    gRegionLeft = gRegions[region].left;
    gRegionLeftPx = gRegionLeft * 8;
    gRegionRight = gRegionLeft + gRegions[region].width;
    gRegionTop = gRegions[region].top;
    gRegionBottom = gRegionTop + gRegions[region].height;
    gSetRegion(gRegionLeft, gRegionRight, gRegionTop, gRegionBottom);
}
