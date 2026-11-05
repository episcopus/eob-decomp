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

#include "EOB.H"

extern struct Region gRegions[]; /*@ ds:009F */  /* the region table, read by 082C (four sites, 082C:0650/0696/06DC/071E), 1DE7:005F and SelectRegion (1A29:019A); indexed by a region number, so gRegions[region] is region * 16 */
extern int gCurRegion;           /*@ ds:8108 */  /* index of the region selected last, set by SelectRegion and compared by the front ends and by 1DE7:0333; 1DE7 keeps its own record index at ds:811F, and nothing here says the two agree */
extern int gRegionLeft;          /*@ ds:FE48 */  /* selected region's left edge, in 8-pixel columns */
extern int gRegionLeftPx;        /*@ ds:FE4A */  /* selected region's left edge, in pixels (left * 8) */
extern int gRegionRight;         /*@ ds:FE4C */  /* selected region's right edge (left + width), in 8-pixel columns */
extern int gRegionTop;           /*@ ds:FE46 */  /* selected region's top line */
extern int gRegionBottom;        /*@ ds:FE44 */  /* selected region's bottom line (top + height), in lines */

/* The graphics driver's drawing entry points: slots of the driver's export table at ds:8046,
   which the driver fills when it loads (the overlay request at ds:8121 points at ds:802A; MCGA.OVL
   0BF2 to 0C2F copies the game's imports and writes its exports). Their first argument is the
   destination page, which the driver turns into a segment through gPageSegs (MCGA.OVL 0B3F).
   This unit calls each of them once from its own front end (for example 1A29:0082 `lcall
   [0x80a6]`), DrawShape calls ds:80C2 (1A29:0044); ShowPointer (1BBE:00ED, src/E1BBE.ASM:252-258)
   also calls ds:80C2, at 1BBE:01F4, with page 0. gSetRegion below is called once, in SelectRegion
   (1A29:01DB `lcall [0x80b6]`). */
extern void (far *gDrawShape)(int page, void far *src, int x, int y);        /*@ ds:80C2 */
extern void (far *gDrawShapeFlipped)(int page, void far *src, int x, int y);        /*@ ds:80A6 */
extern void (far *gDrawShapeRemapped)(int page, void far *src, int x, int y);        /*@ ds:80D6 */
extern void (far *gDrawShapeFlippedRemapped)(int page, void far *src, int x, int y);        /*@ ds:80DA */
extern void (far *gDrawShapeMasked)(int page, void far *src, int x, int y, int depth); /*@ ds:809A */
extern void (far *gDrawShapeMaskedFlipped)(int page, void far *src, int x, int y, int depth); /*@ ds:8096 */
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
   unclipped absolute position gShapeX/gShapeY, with no region origin added, and page 0. This front end is called from 20 functions (9 in the resident code, 11 in the
   overlays; for example 082C:063B, the `lcall 1A29:000F` at 082C:0675), which pass
   region-relative coordinates. */
/*@ 1A29:000F */ void DrawShape(int page, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShape(page, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:004D: draws a shape mirrored left to right: the front end over gDrawShapeFlipped
   (ds:80A6, driver export 24), whose MCGA body (mcga.ovl 1639) writes each pixel then decrements
   DI (mcga.ovl 17F7 to 17FA). Identical to 1A29:000F otherwise (null check, lazy region select,
   region-relative x and y). Callers use it for the negative-delta side of wall faces
   (src/EOV4.C:653, :684, :791) and for negative icon indexes (CastBurningHands, 0EF3:0F99, src/E0EF3.C:925). */
/*@ 1A29:004D */ void DrawShapeFlipped(int page, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShapeFlipped(page, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:008B: Draws a shape like DrawShape but through the driver's colour-remapping entry ds:80D6,
   after the caller has set the remap row through ds:80D2. A front end over the driver pointer
   ds:80D6, identical to
   1A29:000F otherwise (null check, lazy region select, region-relative x and y). */
/*@ 1A29:008B */ void DrawShapeRemapped(int page, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShapeRemapped(page, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:00C9: draws a shape through the colour remap and mirrored: the front end over ds:80DA
   (driver export 37), whose MCGA body (mcga.ovl 298A) draws the 4-bit packed shape through the
   remap buffer (mcga.ovl 030C) and swaps the nibbles of each pixel pair (the table at mcga.ovl
   00F6). Identical to 1A29:000F otherwise (null check, lazy region select, region-relative x and
   y). Its only caller picks it for sprite < 0 after gSetRemapTable, where the unremapped branch
   picks DrawShapeFlipped (src/EOV4.C:580-589); the mirroring rests on that pairing and the nibble
   swap. */
/*@ 1A29:00C9 */ void DrawShapeFlippedRemapped(int page, void far *src, int x, int y, int region)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShapeFlippedRemapped(page, src, x + gRegionLeftPx, y + gRegionTop);
}

/* 1A29:0107: draws a shape behind masked layers: the front end over gDrawShapeMasked (ds:809A,
   driver export 21), like 1A29:000F plus the word `depth` it forwards, the mask level. The MCGA
   body (mcga.ovl 12F0, src/drivers/MCGA.ASM:2133) tests up to seven mask planes in page 4 below
   that depth (mcga.ovl 14DE to 1568) and puts the page 2 pixel where a plane covers it (mcga.ovl
   1576 to 157F). No caller of
   it was found in EOB.EXE or its overlays (make packet). */
/*@ 1A29:0107 */ void DrawShapeMasked(int page, void far *src, int x, int y, int region, int depth)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShapeMasked(page, src, x + gRegionLeftPx, y + gRegionTop, depth);
}

/* 1A29:0148: DrawShapeMasked mirrored left to right: the front end over gDrawShapeMaskedFlipped
   (ds:8096, driver export 20), whose MCGA body (mcga.ovl 1006) is the masked draw with dec di
   (mcga.ovl 117C). `depth` as in 1A29:0107. No caller of it was found in EOB.EXE or its
   overlays (make packet). */
/*@ 1A29:0148 */ void DrawShapeMaskedFlipped(int page, void far *src, int x, int y, int region, int depth)
{
    if (!src)
        return;
    if (region != gCurRegion)
        SelectRegion(region);
    gDrawShapeMaskedFlipped(page, src, x + gRegionLeftPx, y + gRegionTop, depth);
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
