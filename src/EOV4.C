/* Unit at overlay 4 of EOB.EXE (8259 bytes, 18 functions): the first-person view renderer. It
   draws the map objects, walls and creature sprites of the current page into the view region from
   the object records at ds:8A82 and the 9-byte tile records at ds:B894, clipped through the
   drawing wrappers in src/E082C.C. */

/*@flags -ml -O -Z -v */

extern signed char gViewCellOffsets[]; /*@ ds:199D */  /* rows of 18 signed heading offsets */
extern signed char gMaze[]; /*@ ds:B894 */    /* 9-byte tile records (src/E08E6.C) */
extern signed char far *gViewCells[]; /*@ ds:EF6A */ /* 18 far pointers into gMaze (src/E1414.C) */
extern signed char gViewFrontSide; /*@ ds:A9C6 */        /* an offset added into gViewCells's targets (src/E1414.C:101; src/E0ACD.C:78, src/E122E.C:99 differ) */
extern signed char gWallTypeFlags[]; /*@ ds:194B */      /* per-slot byte flags (src/E08E6.C) */
extern signed char gWallVmpSet[]; /*@ ds:1879 */      /* per-object count-1 (this unit) */
extern int gViewCellIds[]; /*@ ds:A9C8 */         /* 18 heading words, the masked sum (this unit) */
extern int gViewSlots[]; /*@ ds:DCB2 */         /* 18 {left,right} view-slot pairs (this unit) */

/* ovl4:0000: for each of the 18 view slots, compute a heading word from `base` plus the slot's
   signed offset in the 18-byte row `row` of ds:199D (row is scaled by 18 first), masked to 10 bits,
   store it in the heading table at ds:A9C8, point the slot's ds:EF6A entry at the 9-byte tile
   record for that heading in ds:B894, and reset all 36 words of the view-slot array at ds:DCB2 to
   -1 (word i and word i + 18 for each i < 18, i.e. both words of all 18 {left, right} pairs).
   Called by BuildViewBlockMap at ovl4:04D3. Purpose unknown: the meaning of `row` and of the 18 slots. */
/*@ ovl4:0000 */ void InitViewSlots(int base, int row)
{
    int heading;
    int i;

    row *= 18;
    for (i = 0; i < 18; i++) {
        heading = gViewCellIds[i] = (base + gViewCellOffsets[row + i]) & 0x3FF;
        gViewCells[i] = (signed char far *)&gMaze[heading * 9];
        gViewSlots[i] = gViewSlots[i + 18] = -1;
    }
}

extern void SetRegionExtents(int region, int left, int top, int width, int height); /*@ 1DE7:02FE */

/* ovl4:0066: wrapper for SetRegionExtents: builds a region record from a box whose left and right
   edges are in 8-pixel columns (so the width, right - left, is in columns too and is not scaled)
   and whose top and bottom are in units of 8 pixel rows (top is scaled by 8, height is
   (bottom - top) * 8). Called by ComputeViewSpan at ovl4:0211 and DrawSlotDoor at ovl4:19A0 to fit the
   view region to the visible tile span. */
/*@ ovl4:0066 */ void SetRegionRect(int region, int left, int top, int right, int bottom)
{
    SetRegionExtents(region, left, top * 8, right - left, (bottom - top) * 8);
}

extern signed char gSpanWallClip[]; /*@ ds:6B82 */   /* per slot,i signed column adjustment (this unit) */
extern signed char gSpanDoorClip[]; /*@ ds:6CC6 */   /* per slot,i signed {left,right} bounds (this unit) */

#define VIEW_SLOTS 18       /* the ds:EF6A tile records a slot's span is built from */
#define VIEW_RIGHT 0x16     /* the rightmost column of the view, in 8-pixel columns */
#define VIEW_BOTTOM 0xf     /* the bottom of the view region, in 8-pixel rows */
#define ADJUST_SKIP -40     /* a ds:6B82 adjustment that leaves the span unchanged */
#define ADJUST_EMPTY -41    /* a ds:6B82 adjustment that forces an empty span */
#define TILE_FLAG 8         /* ds:194B bit 3: the record carries ds:6CC6 bounds, not an adjustment */

/* ovl4:0097: computes the visible left and right columns of one view slot and fits region `region`
   to them. `slot` indexes the view slots and gViewCells; `left`/`right` receive the span in 8-pixel
   columns (their two words are passed straight to SetRegionRect, which takes 8-pixel columns). When
   the slot's cached pair in gViewSlots (at ds:DCB2, whose 36 words are 18 {left, right} pairs) is
   not -1 it returns that pair. Otherwise it starts at 0..VIEW_RIGHT and walks the VIEW_SLOTS tile
   records gViewCells[i][gViewFrontSide]: a record whose ds:194B heading flag has bit 3 clear contributes its
   ds:6B82 column adjustment (a positive value pulls *right left, a negative one pulls *left right);
   the record is ignored when its ds:1879 count is zero or its adjustment is ADJUST_SKIP, and an
   adjustment of ADJUST_EMPTY forces an empty span. A record whose bit 3 is set narrows the span
   with its ds:6CC6 {left,right} bounds; the walk stops when *right falls below *left. The pair is
   cached back into gViewSlots and SetRegionRect fits `region` to (left, right). Called by
   DrawViewSlots at ovl4:1E3C and DrawSlotItems at ovl4:0C41 and ovl4:0CFB. Purpose unknown: what the
   records and adjustments mean. */
/*@ ovl4:0097 */ void ComputeViewSpan(int slot, int far *left, int far *right, int region)
{
    int i;
    int pair;
    int heading;
    register int adjust;
    register int j;

    j = 2 * slot;
    if (gViewSlots[j] != -1) {
        *left = gViewSlots[j];
        *right = gViewSlots[j + 1];
    } else {
        *left = 0;
        *right = VIEW_RIGHT;
        pair = 2 * slot;
        slot *= VIEW_SLOTS;
        for (i = 0; i < VIEW_SLOTS; i++) {
            heading = gViewCells[i][gViewFrontSide];
            if (!(gWallTypeFlags[heading] & TILE_FLAG)) {
                adjust = gSpanWallClip[slot + i];
                if (gWallVmpSet[heading] == 0 || adjust == ADJUST_SKIP)
                    continue;
                if (adjust == ADJUST_EMPTY) {
                    *left = VIEW_RIGHT;
                    *right = 0;
                    break;
                }
                if (adjust > 0 && *right > adjust)
                    *right = adjust;
                if (adjust < 0 && -adjust > *left)
                    *left = -adjust;
            } else {
                j = (slot + i) * 2;
                if (gSpanDoorClip[j] > *left)
                    *left = gSpanDoorClip[j];
                j++;
                if (gSpanDoorClip[j] < *right)
                    *right = gSpanDoorClip[j];
            }
            if (*right < *left)
                break;
        }
        gViewSlots[pair] = *left;
        pair++;
        gViewSlots[pair] = *right;
    }
    SetRegionRect(region, *left, 0, *right, VIEW_BOTTOM);
}
struct Region {             /* the 16-byte region records at ds:009F (src/E1DE7.C) */
    int left;               /* +0 */
    int top;                /* +2 */
    int width;              /* +4 */
    int height;             /* +6 */
    int textAttr1;
    int textAttr2;
    int textCol;
    int textRow;
};

extern struct Region gRegions[]; /*@ ds:009F */
extern int gCurLevel; /*@ ds:FBC8 */                 /* the current action (src/E0D1C.C) */
extern int gPartyDir; /*@ ds:FBCC */                 /* the party's facing, 0..3 north, east, south, west */
extern signed char gDoorClipY[]; /*@ ds:6F4E */       /* row value table this unit indexes */
extern signed char gDoorClipTopRow[]; /*@ ds:6F6D */       /* per-page index into ds:6F4E */
extern signed char gDoorClipBottomRow[]; /*@ ds:6F79 */       /* per-page index into ds:6F4E */
extern signed char gViewSlotRow[]; /*@ ds:1A09 */       /* per-object index into ds:6F4E */

/* ovl4:021E: computes the top and bottom pixel rows of one column of the current page's view
   and fits region `region` to them. The column value comes from ds:6F4E at the row index
   ds:6F6D[gCurLevel] + ds:1A09[col] for the top and ds:6F79[gCurLevel] + ds:1A09[col] for the bottom;
   then SetRegionExtents rewrites the region with its current left and width and the new top and
   height. Called by DrawSlotDoor at ovl4:149B and ovl4:179E. Purpose unknown: what the tables
   mean. */
/*@ ovl4:021E */ void SetRegionColumnExtent(char col, int far *top, int far *bottom, int region)
{
    char topRow = gDoorClipTopRow[gCurLevel];
    char bottomRow = gDoorClipBottomRow[gCurLevel];

    *top = gDoorClipY[topRow + gViewSlotRow[col]];
    *bottom = gDoorClipY[bottomRow + gViewSlotRow[col]];
    SetRegionExtents(region, gRegions[region].left, *top, gRegions[region].width,
                     *bottom - *top);
}

extern unsigned char far *gWallBlockMaps; /*@ ds:F43E */   /* 6000-byte start-up buffer (src/E07A9.C) */
extern unsigned char gViewBlockMap[]; /*@ ds:ADB0 */  /* the view pixel buffer (this unit) */

/* ovl4:02A5: copies a rectangle of word pixels out of the page buffer pointed to by ds:F43E into
   the view buffer at ds:ADB0. `src` starts at (gWallVmpSet[page] - 1) * 0x35E + x2 * 2 + 0x294 in
   ds:F43E; each row walks `width` words and each of the `height` rows advances the destination by
   a 0x16-word row. Words outside columns 0..0x15 are skipped, and only nonzero source words are
   stored. Called by BuildViewBlockMap at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the buffers hold.
   The three duplicated increment tails are kept apart by the file's -v (debug information), as
   the original shows; without it our build merges the continue tails (whitepaper 2.1.1). */
/*@ ovl4:02A5 */ void CopyViewColumn(char x, char y, char page, int x2, char width, char height)
{
    unsigned int far *dst;
    unsigned int far *src;
    int row;
    int col;

    if (gWallVmpSet[page] == 0)
        return;
    src = (unsigned int far *)(gWallBlockMaps + (gWallVmpSet[page] - 1) * 0x35E + x2 * 2);
    src += 0x14A;
    for (row = 0; row < height; row++) {
        dst = (unsigned int far *)(gViewBlockMap + ((y + row) * 0x16 + x) * 2);
        for (col = 0; col < width; col++) {
            if (x + col >= 0 && x + col < 0x16) {
                if (*src != 0) {
                    *dst = *src;
                    src++;
                    dst++;
                } else {
                    dst++;
                    src++;
                }
            } else {
                dst++;
                src++;
            }
        }
    }
}

/* ovl4:036A: flips bit 0x4000 of a rectangle of word pixels in the view buffer at ds:ADB0, reading
   from the page buffer at ds:F43E. The far pointer `src` starts at
   (gWallVmpSet[page] - 1) * 0x35E + x2 * 2 + 0x294; for each of the `height` rows and `width` columns it
   reads the word at (src + width * row)[width - 1 - col], skips it when zero and stores it to
   ds:ADB0[(y + row) * 0x16 + x + col] with bit 0x4000 toggled, skipping columns outside 0..0x15.
   Called by BuildViewBlockMap at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the bit marks. */
/*@ ovl4:036A */ void ToggleViewColumn(char x, char y, char page, int x2, char width, char height)
{
    unsigned int far *src;
    int row;
    int col;
    int pixel;

    if (gWallVmpSet[page] == 0)
        return;
    src = (unsigned int far *)(gWallBlockMaps + (gWallVmpSet[page] - 1) * 0x35E + x2 * 2);
    src += 0x14A;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            if (x + col < 0 || x + col >= 0x16)
                continue;
            pixel = (src + width * row)[width - 1 - col];
            if (pixel == 0)
                continue;
            if (pixel & 0x4000)
                pixel -= 0x4000;
            else
                pixel |= 0x4000;
            ((unsigned int *)gViewBlockMap)[(y + row) * 0x16 + x + col] = pixel;
        }
    }
}

/* ovl4:042C: returns 1 when `slot` is nonzero and bit 3 of its ds:194B flag byte is clear, else 0.
   The callers use it to choose between the slot's own ds:EF6A record and a second record; bit 3
   itself is read with other meanings in src/E1414.C:321 and src/EOV3.C:1012. Called by BuildViewBlockMap
   at ovl4:0563 and ovl4:05D8. */
/*@ ovl4:042C */ int IsSlotLive(int slot)
{
    return slot != 0 && !(gWallTypeFlags[slot] & 8);
}

extern signed char gNearSideByDir[]; /*@ ds:1991 */   /* per-page index into a gViewCells target (src/E08E6.C) */
extern signed char gRightSideByDir[]; /*@ ds:1995 */   /* per-page index into a gViewCells target (this unit) */
extern signed char gLeftSideByDir[]; /*@ ds:1999 */   /* per-page index into a gViewCells target (this unit) */
extern signed char gViewRightSide; /*@ ds:EFB9 */     /* the second per-page gViewCells target index (this unit) */
extern signed char gViewLeftSide; /*@ ds:EFD0 */     /* the third per-page gViewCells target index (this unit) */
extern int gViewMirrorParity; /*@ ds:F4BC */             /* the cell and page parity (this unit) */

/* ovl4:044B: draws the whole view page. It caches the three per-page gViewCells target offsets
   (gNearSideByDir, gRightSideByDir and gLeftSideByDir at the page index, into gViewFrontSide, gViewRightSide and gViewLeftSide), clears the 0x14a-word
   view buffer at ds:ADB0, computes a parity from the cell (`base`) and the page into ds:F4BC, copies
   or toggles the top strip (page 1, x2 -0x14a), initialises the 18 view slots through InitViewSlots,
   and then walks a fixed list of rectangles. Each rectangle selects one of the page's tile records
   (gViewCells[i] at one of the three offsets) and, when the byte is a live slot (or a nonzero record,
   for the ones that are always drawn), copies or toggles its column (x, y, width, height, source x2)
   from the page buffer into the view buffer. The records with a ds:194B bit 3 also pick a second
   record and draw whichever of the two the flag selects. Called by DrawViewFrame at ovl4:1F5D.
   Purpose unknown: what the tile records and the many rectangles are. */
/*@ ovl4:044B */ void BuildViewBlockMap(int base, int row)
{
    int i;
    int slot;
    int slot2;

    gViewFrontSide = gNearSideByDir[gPartyDir];
    gViewRightSide = gRightSideByDir[gPartyDir];
    gViewLeftSide = gLeftSideByDir[gPartyDir];
    for (i = 0; i < 0x14a; i++)
        ((unsigned int *)gViewBlockMap)[i] = 0;
    gViewMirrorParity = ((base >> 5) + (base & 0x1f) + gPartyDir) & 1;
    if (gViewMirrorParity == 0)
        CopyViewColumn(0, 0xf, 1, 0xfeb6, 0x16, 0xf);
    else
        ToggleViewColumn(0, 0xf, 1, 0xfeb6, 0x16, 0xf);
    InitViewSlots(base, row);

    slot = gViewCells[0][gViewRightSide];
    if (slot)
        CopyViewColumn(0xfe, 3, slot, 0x66, 3, 5);
    slot = gViewCells[6][gViewLeftSide];
    if (slot)
        ToggleViewColumn(0x15, 3, slot, 0x66, 3, 5);
    slot = gViewCells[1][gViewRightSide];
    slot2 = gViewCells[2][gViewFrontSide];
    if (IsSlotLive(slot) && !(gWallTypeFlags[slot2] & 8))
        CopyViewColumn(2, 3, slot, 0x66, 3, 5);
    else if (slot != 0 && (gWallTypeFlags[slot2] & 8))
        CopyViewColumn(2, 3, slot2, 0x66, 3, 5);
    slot = gViewCells[5][gViewLeftSide];
    slot2 = gViewCells[4][gViewFrontSide];
    if (IsSlotLive(slot) && !(gWallTypeFlags[slot2] & 8))
        ToggleViewColumn(0x11, 3, slot, 0x66, 3, 5);
    else if (slot != 0 && (gWallTypeFlags[slot2] & 8))
        ToggleViewColumn(0x11, 3, slot2, 0x66, 3, 5);
    slot = gViewCells[2][gViewRightSide];
    if (slot)
        CopyViewColumn(8, 3, slot, 0x61, 1, 5);
    slot = gViewCells[4][gViewLeftSide];
    if (slot)
        ToggleViewColumn(0xd, 3, slot, 0x61, 1, 5);
    if (IsSlotLive(gViewCells[1][gViewFrontSide]))
        CopyViewColumn(0xfc, 3, gViewCells[1][gViewFrontSide], 0x81, 6, 5);
    if (IsSlotLive(gViewCells[5][gViewFrontSide]))
        CopyViewColumn(0x14, 3, gViewCells[5][gViewFrontSide], 0x81, 6, 5);
    if (IsSlotLive(gViewCells[2][gViewFrontSide]))
        CopyViewColumn(2, 3, gViewCells[2][gViewFrontSide], 0x81, 6, 5);
    if (IsSlotLive(gViewCells[4][gViewFrontSide]))
        CopyViewColumn(0xe, 3, gViewCells[4][gViewFrontSide], 0x81, 6, 5);
    slot = gViewCells[3][gViewFrontSide];
    if (slot)
        CopyViewColumn(8, 3, slot, 0x81, 6, 5);
    slot = gViewCells[7][gViewRightSide];
    if (slot)
        CopyViewColumn(0, 3, slot, 0x75, 2, 6);
    slot = gViewCells[11][gViewLeftSide];
    if (slot)
        ToggleViewColumn(0x14, 3, slot, 0x75, 2, 6);
    slot = gViewCells[8][gViewRightSide];
    if (slot)
        CopyViewColumn(6, 2, slot, 0x51, 2, 8);
    slot = gViewCells[10][gViewLeftSide];
    if (slot)
        ToggleViewColumn(0xe, 2, slot, 0x51, 2, 8);
    if (IsSlotLive(gViewCells[8][gViewFrontSide]))
        CopyViewColumn(0xfc, 2, gViewCells[8][gViewFrontSide], 0x9f, 0xa, 8);
    if (IsSlotLive(gViewCells[10][gViewFrontSide]))
        CopyViewColumn(0x10, 2, gViewCells[10][gViewFrontSide], 0x9f, 0xa, 8);
    slot = gViewCells[9][gViewFrontSide];
    if (slot)
        CopyViewColumn(6, 2, slot, 0x9f, 0xa, 8);
    slot = gViewCells[12][gViewRightSide];
    if (slot)
        CopyViewColumn(3, 1, slot, 0x2d, 3, 0xc);
    slot = gViewCells[14][gViewLeftSide];
    if (slot)
        ToggleViewColumn(0x10, 1, slot, 0x2d, 3, 0xc);
    slot = gViewCells[12][gViewFrontSide];
    slot2 = gViewCells[13][gViewFrontSide];
    if (!(gWallTypeFlags[slot] & 8))
        CopyViewColumn(0xf3, 1, slot, 0xef, 0x10, 0xc);
    slot = gViewCells[14][gViewFrontSide];
    if (!(gWallTypeFlags[slot] & 8))
        CopyViewColumn(0x13, 1, slot, 0xef, 0x10, 0xc);
    slot = gViewCells[13][gViewFrontSide];
    if (slot)
        CopyViewColumn(3, 1, slot, 0xef, 0x10, 0xc);
    slot = gViewCells[15][gViewRightSide];
    slot2 = gViewCells[17][gViewLeftSide];
    if (gCurLevel > 6 && gCurLevel < 0xa) {
        if (slot2 == 0x17)
            slot2 = 0;
        if (slot2 == 0x18)
            slot2 = 0;
    } else if (gCurLevel > 3) {
        if (slot == 0x18)
            slot = 0;
        if (slot2 == 0x17)
            slot2 = 0;
    }
    if (slot)
        CopyViewColumn(0, 0, slot, 0, 3, 0xf);
    if (slot2)
        ToggleViewColumn(0x13, 0, slot2, 0, 3, 0xf);
}

/* A 14-byte item record at ds:8DD0 (src/E0BD2.C declares the same layout). */
struct ItemRecord {
    signed char nameIndex;
    signed char identifiedNameIndex;
    unsigned char flags;
    signed char icon;       /* +3: indexes ds:181F */
    signed char type;
    signed char cellPos;        /* +5: the kind tested here */
    int key;
    int prevLink;                /* +8: one link of the circular list */
    int nextLink;
    signed char m0A;
    signed char value;
};
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern signed char gViewRelativePos[]; /*@ ds:1A2B */   /* per-page sprite base (src/E0DE8.C) */
extern signed char gIconPictureIndex[]; /*@ ds:181F */   /* byte table indexed by an item icon (src/E122E.C) */
extern signed char gItemClipSlot[]; /*@ ds:6F86 */   /* per-slot byte (this unit) */
extern signed char gItemScaleFrame[]; /*@ ds:6F98 */   /* per-slot, per-part byte (this unit) */
extern int gNicheItemY[]; /*@ ds:6FA8 */           /* per-slot y base (this unit) */
extern int gItemJitter[]; /*@ ds:6FAE */           /* per-part x/y nudge (this unit) */
extern int gViewPosX[]; /*@ ds:1D61 */           /* per-slot, per-part x table (this unit) */
extern int gViewPosY[]; /*@ ds:1D63 */           /* per-slot, per-part y table (this unit) */
extern void far *gItemL1Icons[][4]; /*@ ds:EC9A */  /* far sprite cells, one 4-cell row per icon (this unit) */
extern int gViewSlotCenterX[]; /*@ ds:19E5 */           /* per-slot x base (this unit) */
extern int gViewColLeft; /*@ ds:FE3E */      /* the visible left column (this unit) */
extern int gViewColRight; /*@ ds:FE40 */     /* the visible right column (this unit) */
extern void DrawShapeClipped(int page, void far *src, int x, int y, int region); /*@ 082C:063B */

/* ovl4:0AD0: draws the map object that one view slot's tile record names at word +5, as a run of
   sprite cells taken from the circular item list at ds:8DD0. `slot` is the view slot, `field` the
   index of the object: it starts at gItems[field]'s +8 link (the item's list), remembers that link
   as the head and walks the list back to it. For each item, the kind +5 picks the sprite base
   ds:1A2B[gPartyDir * 4 + (kind & 3)]; the cell position comes from ds:1D61/ds:1D63 (or, when the kind
   is 8, from gViewSlotCenterX[slot] and gNicheItemY[gViewSlotRow[slot]] with sprite 0) and is nudged by ds:6FAE at
   `field & 7` and `(field >> 1) & 7`; the cell itself is ds:EC9A[gIconPictureIndex[icon]][frame], its byte +2
   (in 8-pixel units) and byte +1 shift the position. Items whose kind is 8 or below 4 are drawn;
   the rest are skipped. The tile's ds:194B flags (forced to 0x80 when slot is 0x10) and its ds:1879
   count gate the slot: it returns when the count is nonzero and bit 7 of the flags is clear, so it
   proceeds when the count is zero or bit 7 is set. Bit 3 of the flags, when sprite < 2 and frame is
   nonzero, also recomputes the span of ds:6F86[slot] (the view slot's span slot), when that byte is
   not -1, and pulls the object up 4 pixels. After each item, when that span slot was set, the slot's
   own span is recomputed (the second call is inside the loop). Called by DrawViewSlots when the tile record's
   word +5 is nonzero. Purpose unknown: what the tables and the object are.
   QUIRK ovl4:0AD0: `spanSlot` is set only inside the drawn-item branch; a first item that is
   skipped leaves the check after the item reading this slot's indeterminate stack value, and a
   later skipped item reads the previous item's value. The original reads the same stack slot. */
/*@ ovl4:0AD0 */ void DrawSlotItems(int slot, int field)
{
    int x;
    int frame;
    int sprite;
    int head;
    int k;
    int flags;
    signed char first;
    void far *cell;
    signed char is8;
    signed char tile;
    signed char spanSlot;
    struct ItemRecord far *rec;
    register int slotReg;   /* the slot number, the original keeps it in a register */
    register int y;

    slotReg = slot;
    tile = gMaze[gViewCellIds[slotReg] * 9 + gViewFrontSide];
    if (slotReg == 0x10)
        flags = 0x80;
    else
        flags = gWallTypeFlags[tile];
    if (gWallVmpSet[tile] != 0 && !(flags & 0x80))
        return;

    field = gItems[field].prevLink;
    head = field;
    first = 1;
    while (field != head || first) {
        rec = &gItems[field];
        is8 = (rec->cellPos == 8);
        if (is8 == 1 || rec->cellPos < 4) {
            spanSlot = -1;
            sprite = gViewRelativePos[gPartyDir * 4 + (rec->cellPos & 3)];
            if (is8)
                sprite = 4;
            k = (slotReg * 5 + sprite) * 2;
            x = gViewPosX[k] + 0x58;
            if (is8) {
                x = gViewSlotCenterX[slotReg];
                y = gNicheItemY[gViewSlotRow[slotReg]];
                sprite = 0;
            } else {
                y = gViewPosY[k] + 0x7c;
            }
            frame = gItemScaleFrame[gViewSlotRow[slotReg] * 4 + sprite];
            if ((flags & 8) && sprite < 2 && frame != 0) {
                spanSlot = gItemClipSlot[slotReg];
                if (spanSlot != -1)
                    ComputeViewSpan(spanSlot, &gViewColLeft, &gViewColRight, 5);
                y -= 4;
            }
            if (frame >= 0) {
                cell = gItemL1Icons[gIconPictureIndex[rec->icon]][frame];
                x -= (((signed char far *)cell)[2] << 3) >> 1;
                x += gItemJitter[field & 7] * 2;
                y -= ((signed char far *)cell)[1];
                y += gItemJitter[(field >> 1) & 7];
                DrawShapeClipped(2, cell, x, y, 5);
            }
        }
        field = rec->prevLink;
        first = 0;
        if (spanSlot != -1)
            ComputeViewSpan(slotReg, &gViewColLeft, &gViewColRight, 5);
    }
}

/* The 28-byte object records at ds:8A82 and the 27-byte records at ds:1A3B (src/E08E6.C). */
struct Monster {
    char typeIndex;                /* +0: index into the ds:1A3B table */
    signed char kind;       /* +1 */
    int cell;                 /* +2 */
    char cellPos;             /* +4 */
    char orient;            /* +5 */
    char toggle;            /* +6 */
    char m07;               /* +7 */
    signed char type;       /* +8 */
    signed char state;      /* +9 */
    char m0A;               /* +A */
    char timer;             /* +B */
    int hitPoints;          /* +C */
    int count;              /* +E */
    int targetCell;
    int dropItem1;
    int dropItem2;
    char m16;               /* +16: flag byte */
    char m17;               /* +17 */
    char actionDelay;
    char actionsLeft;
    char m1A;
    char m1B;
};
struct MonsterType {
    char pad00;
    char m01;
    char m02;
    char m03;
    char m04[3][3];
    char pad0D;
    char m0E;               /* +0xE: bits tested here */
    char pad0F[0x13 - 0x0F];
    int m13;
    char m15;               /* +0x15: tested here */
    char m16;
    char m17;
    char m18;
    char pad19[0x1B - 0x19];
};
extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern struct MonsterType gMonsterTypes[]; /*@ ds:1A3B */
extern signed char gMonsterShrinkSteps[]; /*@ ds:6FBE */   /* byte table indexed by gViewSlotRow[slot] (this unit) */
extern signed char gMonsterDrawOrder[]; /*@ ds:7001 */   /* per-page, per-frame byte (this unit) */
extern signed char gMonsterFramesMirrorStep[]; /*@ ds:6FC1 */   /* sprite table A (this unit) */
extern signed char gMonsterFrames[]; /*@ ds:6FE1 */   /* sprite table B (this unit) */
extern signed char gMonsterYAdjust[]; /*@ ds:7015 */   /* per-row y nudge (this unit) */
extern void far *gMonsterShapes[]; /*@ ds:B7FE */     /* far sprite cells indexed by sprite plus nudge (this unit) */
extern void far *gBufDCFC; /*@ ds:DCFC */    /* start-up far allocation, 4096 bytes (src/E07A9.C) */
extern int gGraphicsChoice; /*@ ds:8102 */  /* the graphics choice: 0 CGA, 1 Tandy, 2 EGA, 3 MCGA/VGA, 4 VGA (re/globals.toml ds:8102; src/E07C8.C:42-45) */
extern int gPartyCell; /*@ ds:FBCA */        /* the current map cell id (src/E08E6.C) */
extern void (far *gSetRemapTable)(char far *map); /*@ ds:80D2 */  /* the far colour-table callback (src/E07A9.C) */
extern char gHitFlashRemap[]; /*@ ds:1CD3 */  /* a 16-byte colour row the colour callback ds:80D2 is passed (src/E08E6.C:110, :752) */
extern char gHitFlashRemapCga[]; /*@ ds:1CE3 */  /* the 16-byte colour row ds:80D2 is passed when gGraphicsChoice is 0 (src/E08E6.C:111, :750) */
extern void BlitSpriteEga(void far *src, void far *dst); /*@ 17A6:1715 */
extern void BlitSpriteCga(void far *src, void far *dst); /*@ 17A6:11CD */
extern void BlitSpriteDefault(void far *src, void far *dst); /*@ 17A6:0FFA */
extern void DrawShapeClippedFlipped(int page, void far *src, int x, int y, int region); /*@ 082C:0681 */
extern void DrawShapeClippedRemapped(int page, void far *src, int x, int y, int region); /*@ 082C:06C7 */
extern void DrawShapeClippedRemapFlipped(int page, void far *src, int x, int y, int region); /*@ 082C:070D */

/* ovl4:0D1B: draws the map objects whose ds:8A82 record id matches the cell the player's facing
   points at. The cell id is (gPartyCell + gViewCellOffsets[gPartyDir * 18 + slot]) & 0x3ff; the 30 object
   records are scanned and each matching one is put in a 5-entry far-pointer slot indexed by
   gMonsterDrawOrder[gPartyDir * 5 + obj->frame]. Each non-empty slot is then blitted gMonsterShrinkSteps[gViewSlotRow[slot]] times into
   ds:DCFC through the sprite blitter the video mode selects, positioned by ds:1D61/ds:1D63 nudged
   by the object's ds:1A3B record and its +0x17 byte, and drawn by one of four DrawRegion wrappers
   (which one depends on the sprite's sign and the +0x16 flag bit 2, which also runs the colour
   callback ds:80D2 with ds:1CD3 or ds:1CE3). Called by DrawViewSlots when the tile record's byte
   +4 has any of bits 0..2. Purpose unknown: what the counters and the four wrappers mean.
   Four source forms TCC is picky about, all confirmed by the match: the object byte +0xA is tested
   against (char)0xfe/(char)0xff so the compare is a byte compare (against int constants it would
   sign-extend first); the absolute sprite is an explicit if/else (abs() compiles to a library
   call); the +7 nudge is an if/else assigning the local (a ?: keeps the value in AX and stores
   once); and the ds:1A3B record is indexed directly for the m15 test (only the m0E test keeps the
   far-pointer local ds:1A3B `info`). */
/*@ ovl4:0D1B */ void DrawSlotMonsters(int slot)
{
    struct MonsterType far *info;
    struct Monster far *obj;
    int sprite;
    int x;
    int k;
    int spriteAbs;
    int tileRow;
    int cellId;
    int spriteBase;
    int page5;
    int n;
    int page4;
    int nudge;
    char far *cell;
    struct Monster far *arr[5];
    register int i;
    register int y;

    tileRow = gMonsterShrinkSteps[gViewSlotRow[slot]];
    cellId = (gPartyCell + gViewCellOffsets[gPartyDir * 18 + slot]) & 0x3ff;
    page5 = gPartyDir * 5;
    page4 = gPartyDir * 4;
    for (i = 0; i < 5; i++)
        arr[i] = 0;
    obj = gMonsters;
    i = 0;
    while (i < 0x1e) {
        if (obj->cell == cellId)
            arr[gMonsterDrawOrder[page5 + obj->cellPos]] = obj;
        i++;
        obj++;
    }
    for (page5 = 0; page5 < 5; page5++) {
        if (arr[page5] == 0)
            continue;
        obj = arr[page5];
        info = &gMonsterTypes[obj->typeIndex];
        if (info->m0E & 2)
            sprite = gMonsterFramesMirrorStep[(obj->toggle << 4) + page4 + obj->orient];
        else
            sprite = gMonsterFrames[(obj->toggle << 4) + page4 + obj->orient];
        if (tileRow == 0) {
            if (obj->m0A == (char)0xfe)
                sprite = 5;
            else if (obj->m0A == (char)0xff)
                sprite = 6;
        }
        if (sprite < 0)
            spriteAbs = -sprite;
        else
            spriteAbs = sprite;
        if (obj->m07)
            nudge = 0x12;
        else
            nudge = 0;
        cell = gMonsterShapes[spriteAbs + nudge];
        for (n = tileRow; n != 0; n--) {
            if (gGraphicsChoice == 2)
                BlitSpriteEga(cell, gBufDCFC);
            else if (gGraphicsChoice == 0)
                BlitSpriteCga(cell, gBufDCFC);
            else
                BlitSpriteDefault(cell, gBufDCFC);
            cell = gBufDCFC;
        }
        if (obj->cellPos != 4)
            spriteBase = gViewRelativePos[page4 + obj->cellPos];
        else
            spriteBase = 4;
        k = (slot * 5 + spriteBase) * 2;
        x = gViewPosX[k] + 0x58;
        y = gViewPosY[k] + 0x7f;
        if (gMonsterTypes[obj->typeIndex].m15 == 1 && (spriteAbs == 1 || spriteAbs > 3))
            y += gMonsterYAdjust[tileRow];
        x -= (((signed char far *)cell)[2] << 3) >> 1;
        y -= ((signed char far *)cell)[1];
        x += (obj->m17 & 0xf0) >> 4;
        y += obj->m17 & 0xf;
        if (obj->m16 & 2) {
            if (gGraphicsChoice == 0)
                gSetRemapTable(gHitFlashRemapCga);
            else
                gSetRemapTable(gHitFlashRemap);
            if (sprite >= 0)
                DrawShapeClippedRemapped(2, cell, x, y, 5);
            else
                DrawShapeClippedRemapFlipped(2, cell, x, y, 5);
        } else {
            if (sprite >= 0)
                DrawShapeClipped(2, cell, x, y, 5);
            else
                DrawShapeClippedFlipped(2, cell, x, y, 5);
        }
    }
}
extern signed char gWallDecorHead[]; /*@ ds:18BF */   /* per-object-kind table (src/E122E.C) */
extern void far *gWallFaces[]; /*@ ds:7018 */      /* per slot, two far pointers to a wall face record (this unit) */
extern signed char gWallOffsets[]; /*@ ds:70A8 */  /* per wall face, its signed segment offset (this unit) */
extern int gWallXBias[]; /*@ ds:70CC */            /* per slot, the x base when the face flag bit 2 is clear (this unit) */

struct RectKind {           /* the per-kind size record gRectKinds points at (src/E122E.C) */
    char pad00;
    signed char height;     /* +1 */
    signed char width;      /* +2: base width in eighths */
};
struct ViewWallRec {        /* the 0x34-byte wall records at ds:E06A this unit walks (src/EOV3.C) */
    unsigned char cells[0xa]; /* +00: ten cell indices, 0xff = none */
    signed char next;       /* +0A: the next record, nonpositive ends the walk */
    char flags;             /* +0B: bits 0 and 1 reflect the offset, bit 2 selects the x base */
    int x[0xa];             /* +0C: the per-cell x, the cell index doubled apart */
    int y[0xa];             /* +20: the per-cell y, the cell index doubled apart */
};
extern struct RectKind far *gRectKinds[]; /*@ ds:EFD4 */  /* per-cell far size records (src/E122E.C) */
extern struct ViewWallRec gViewWalls[]; /*@ ds:E06A */

/* ovl4:1091: draws the wall faces of one view slot. The slot's two faces are ds:7018[slot*2+1],
   drawn first (ovl4:10AD), and ds:7018[slot*2], drawn second (ovl4:11FD); each face, when its far
   pointer is set, starts a chain of 0x34-byte wall records at ds:E06A. The chain head is
   gWallDecorHead of the byte the face's first byte selects in the slot's ds:EF6A tile record, and
   `next` (which is read after the index is decremented) walks it. The face's signed offset
   ds:70A8[face] picks the cell within each record (b = |offset| - 1) and its sign picks the
   DrawRegion wrapper: a nonnegative offset draws the cell at its x, a negative one mirrors it about
   the 0xb0-pixel screen. The face at slot*2 (drawn second) also reads the record's flags: bit 0 (or
   bit 1 with the parity ds:F4BC) reflects the offset, and bit 2 adds either gViewPosX[(slot*5+4)*2] or
   ds:70CC[slot] to x. Called by DrawViewSlots at ovl4:1E50. Purpose unknown: what the wall faces
   and the ds:E06A records are. */
/*@ ovl4:1091 */ void DrawSlotDecorations(int slot)
{
    signed char offA;
    signed char offB;
    char sideA;
    char sideB;
    char seg;
    int y;
    int delta;
    int flags;
    unsigned char spr;
    int n;
    int x;

    sideA = slot * 2;
    sideB = sideA + 1;
    if (gWallFaces[sideB]) {
        offB = *(unsigned char far *)gWallFaces[sideB];
        for (n = gWallDecorHead[gViewCells[slot][offB]]; n > 0; n = gViewWalls[n].next) {
            n--;
            delta = gWallOffsets[sideB];
            seg = (delta < 0 ? -delta : delta) - 1;
            spr = gViewWalls[n].cells[seg];
            if (spr != 0xff) {
                if (delta >= 0) {
                    x = gViewWalls[n].x[seg];
                } else {
                    x = 0xb0 - gViewWalls[n].x[seg];
                    x -= gRectKinds[spr]->width << 3;
                }
                y = gViewWalls[n].y[seg];
                if (delta >= 0)
                    DrawShapeClipped(2, gRectKinds[spr], x, y, 5);
                else
                    DrawShapeClippedFlipped(2, gRectKinds[spr], x, y, 5);
            }
        }
    }
    if (gWallFaces[sideA]) {
        offA = *(unsigned char far *)gWallFaces[sideA];
        for (n = gWallDecorHead[gViewCells[slot][offA]]; n > 0; n = gViewWalls[n].next) {
            n--;
            flags = gViewWalls[n].flags;
            delta = gWallOffsets[sideA];
            if ((flags & 1) || ((flags & 2) && gViewMirrorParity))
                delta = -delta;
            seg = (delta < 0 ? -delta : delta) - 1;
            spr = gViewWalls[n].cells[seg];
            if (spr != 0xff) {
                if (delta >= 0) {
                    if (flags & 4)
                        x = gViewWalls[n].x[seg] + gViewPosX[(slot * 5 + 4) * 2];
                    else
                        x = gViewWalls[n].x[seg] + gWallXBias[slot];
                } else {
                    if (flags & 4)
                        x = 0xb0 - gViewWalls[n].x[seg] + gViewPosX[(slot * 5 + 4) * 2];
                    else
                        x = 0xb0 - gViewWalls[n].x[seg] + gWallXBias[slot];
                    x -= gRectKinds[spr]->width << 3;
                }
                y = gViewWalls[n].y[seg];
                if (delta >= 0)
                    DrawShapeClipped(2, gRectKinds[spr], x, y, 5);
                else
                    DrawShapeClippedFlipped(2, gRectKinds[spr], x, y, 5);
            }
        }
    }
}
/* The 3-byte size record a gDoorShapes entry points at (src/E122E.C declares the same layout). */
struct SlotSize {
    char pad00;
    signed char height;     /* +1: base height in pixels */
    signed char width;      /* +2: base width in eighths */
};
/* The 6-byte slot-position records at ds:A99C: a far size record, then x and y (src/E122E.C). */
struct SlotPos {
    struct SlotSize far *rec;   /* +0 */
    unsigned char x;            /* +4 */
    unsigned char y;            /* +5 */
};
extern struct SlotSize far *gDoorShapes[]; /*@ ds:FB5E */  /* per column, a far size record (ovl3:031A frees and clears the entries, src/EOV3.C:356-358; src/EOV2.C:155 and src/EOV3.C:143 declare it long, but the ->width use here needs a far pointer) */
extern struct SlotPos gDoorButtons[]; /*@ ds:A99C */
extern int gF1; /*@ ds:00F1 */               /* the view region's top (src/E0EF3.C:395) */
extern int gF5; /*@ ds:00F5 */               /* the view region's height (src/E0EF3.C:397) */
extern signed char gDoorShapeSet[]; /*@ ds:716E */   /* per heading, a column base (this unit) */
extern signed char gDoorYLevels1to3[]; /*@ ds:70F0 */   /* per index y base, action 1..3 (this unit) */
extern signed char gDoorYLevels4to6[]; /*@ ds:70F4 */   /* per index y base, action 4..6 (this unit) */
extern signed char gDoorYLevels7to9[]; /*@ ds:70F8 */   /* per index y base, action 7..9 (this unit) */
extern signed char gDoorYLevels10to11[]; /*@ ds:70FC */   /* per index y base, action 10..11 (this unit) */
extern signed char gDoorBackYLevels10to11[]; /*@ ds:70FF */   /* per index y base, action 10..11, second draw (this unit) */
extern signed char gDoorYLevel12[]; /*@ ds:7102 */   /* per index y base, action 12 (this unit) */
extern int gDoorClipLeftX[]; /*@ ds:7106 */           /* per slot x, action 4..6 (this unit) */
extern int gDoorClipRightX[]; /*@ ds:7108 */           /* per slot y, action 4..6 (this unit) */
extern signed char gDoorStepBase[]; /*@ ds:714E */   /* per heading nudge base (this unit) */
extern signed char gDoorLiftStep[]; /*@ ds:718E */   /* per index nudge a (this unit) */
extern signed char gDoorLiftFrom30[]; /*@ ds:7192 */   /* per index nudge b (this unit) */
extern signed char gDoorSlideStep[]; /*@ ds:7196 */   /* per index nudge c (this unit) */
extern signed char gDoorSlideFrom30[]; /*@ ds:719A */   /* per index nudge d (this unit) */
extern signed char gDoorSplitStep[]; /*@ ds:719E */   /* per index nudge e (this unit) */
extern signed char gDoorSplitFrom30[]; /*@ ds:71A2 */   /* per index nudge f (this unit) */

#define NO_RECORD -1        /* a ds:18BF byte that means "this heading has no size record" */

/* ovl4:1411: draws the wall face(s) one view slot shows for the current action (ds:FBC8). It selects
   the slot's size record gDoorShapes[col] (col = gDoorShapeSet[heading] + 2 - gViewSlotRow[slot]) and its x base
   gViewPosX[(slot*5+4)*2]; unless the action is 7..9 it first fits region 5 to the column extent
   (SetRegionColumnExtent). Actions 1..3 and 12 draw the record once, at a y base from ds:70F0 (12:
   ds:7102), nudged below heading 0x1e by (heading - ds:714E[heading]) * ds:718E and at or above it by
   ds:7192 with no heading term. Actions 4..6 draw the record twice (region 000F and 004D, mirrored)
   at y base ds:70F4, with an x shift from ds:7196 below 0x1e and ds:719A at or above, and fit region
   5 once (ovl4:1614) to ds:7106/ds:7108 shifted right 3 and clamped to gViewColLeft/gViewColRight,
   with gF1/gF5. Actions 7..9 draw gDoorShapes[col+3] first, then fit the column extent and draw the record
   at y base ds:70F8, with the same nudge pair as 1..3. Actions 10..11 draw gDoorShapes[col] and
   gDoorShapes[col+3] at y bases ds:70FC/ds:70FF with a nudge from ds:719E/ds:71A2. Every case, when
   ds:18BF of the heading is NO_RECORD (0xff), also draws the ds:A99C[col] position record, which
   needs its own x and y. Finally SetRegionRect (ovl4:19A0) fits region 5 to the visible span for
   every case. Called by DrawViewSlots at ovl4:1EA2 when the slot is below 15 and the record's
   ds:194B bit 3 is set. Purpose unknown: what the actions, the size records and the nudges mean, and
   what the 0x1e heading threshold selects.
   QUIRK ovl4:1411: in cases 10 and 11 the slot parameter is dead and the compiler reuses its stack
   slot for the case's y nudge, so `slot` is reassigned there and no longer holds the slot number. */
/*@ ovl4:1411 */ void DrawSlotDoor(int slot)
{
    int top;
    int bottom;
    int heading;
    int x;
    int left;
    int right;
    int backX;
    int col;
    int shift;
    int xBase;
    struct SlotSize far *rec;
    int idx;
    register int y;

    idx = gViewSlotRow[slot];
    heading = gViewCells[slot][gViewFrontSide];
    col = gDoorShapeSet[heading] + 2 - idx;
    rec = gDoorShapes[col];
    xBase = gViewPosX[(slot * 5 + 4) * 2];
    x = xBase + 0x58;
    if (gCurLevel < 7 || gCurLevel > 9)
        SetRegionColumnExtent(slot, &top, &bottom, 5);
    switch (gCurLevel) {
    case 1:
    case 2:
    case 3:
    case 12:
        if (gCurLevel != 0xc)
            y = gDoorYLevels1to3[idx] - rec->height;
        else
            y = gDoorYLevel12[idx] - rec->height;
        x -= (rec->width << 3) >> 1;
        if (gViewCells[slot][gViewFrontSide] < 0x1e)
            y -= (heading - gDoorStepBase[heading]) * gDoorLiftStep[idx];
        else
            y -= gDoorLiftFrom30[gViewSlotRow[slot]];
        DrawShapeClipped(2, rec, x, y, 5);
        if (gWallDecorHead[heading] == NO_RECORD) {
            x = gDoorButtons[col].x + xBase;
            y = gDoorButtons[col].y;
            DrawShapeClipped(2, gDoorButtons[col].rec, x, y, 5);
        }
        break;
    case 4:
    case 5:
    case 6:
        y = gDoorYLevels4to6[idx] - rec->height;
        left = gDoorClipLeftX[slot * 2] >> 3;
        right = gDoorClipRightX[slot * 2] >> 3;
        if (gViewColLeft > left)
            left = gViewColLeft;
        if (gViewColRight < right)
            right = gViewColRight;
        SetRegionExtents(5, left, gF1, right - left, gF5);
        if (gViewCells[slot][gViewFrontSide] < 0x1e)
            shift = -((heading - gDoorStepBase[heading]) * gDoorSlideStep[idx]);
        else
            shift = -gDoorSlideFrom30[gViewSlotRow[slot]];
        shift -= rec->width << 3;
        DrawShapeClipped(2, rec, x + shift, y, 5);
        shift += rec->width << 3;
        DrawShapeClippedFlipped(2, rec, x - shift, y, 5);
        if (gWallDecorHead[heading] == NO_RECORD) {
            x = gDoorButtons[col].x + xBase - shift;
            y = gDoorButtons[col].y;
            DrawShapeClipped(2, gDoorButtons[col].rec, x, y, 5);
        }
        break;
    case 7:
    case 8:
    case 9:
        y = gDoorYLevels7to9[idx];
        backX = x - ((gDoorShapes[col + 3]->width << 3) >> 1);
        x -= (rec->width << 3) >> 1;
        y -= gDoorShapes[col + 3]->height;
        DrawShapeClipped(2, gDoorShapes[col + 3], backX, y, 5);
        SetRegionColumnExtent(slot, &top, &bottom, 5);
        y = gDoorYLevels7to9[idx];
        if (gViewCells[slot][gViewFrontSide] < 0x1e)
            y -= (heading - gDoorStepBase[heading]) * gDoorLiftStep[idx];
        else
            y -= gDoorLiftFrom30[gViewSlotRow[slot]];
        DrawShapeClipped(2, rec, x, y, 5);
        if (gWallDecorHead[heading] == NO_RECORD) {
            x = gDoorButtons[col].x + xBase;
            y = gDoorButtons[col].y;
            DrawShapeClipped(2, gDoorButtons[col].rec, x, y, 5);
        }
        break;
    case 10:
    case 11:
        if (gViewCells[slot][gViewFrontSide] < 0x1e)
            slot = -((heading - gDoorStepBase[heading]) * gDoorSplitStep[idx]);
        else
            slot = -gDoorSplitFrom30[gViewSlotRow[slot]];
        x -= (rec->width << 3) >> 1;
        y = gDoorYLevels10to11[idx];
        DrawShapeClipped(2, gDoorShapes[col], x, y + slot, 5);
        slot = (slot >> 2) + (slot >> 3);
        y = gDoorBackYLevels10to11[idx];
        DrawShapeClipped(2, gDoorShapes[col + 3], x, y - slot, 5);
        if (gWallDecorHead[heading] == NO_RECORD) {
            x = gDoorButtons[col].x + xBase;
            y = gDoorButtons[col].y;
            DrawShapeClipped(2, gDoorButtons[col].rec, x, y, 5);
        }
        break;
    }
    SetRegionRect(5, gViewColLeft, 0, gViewColRight, 0xf);
}
/* The 18-byte records at ds:DD2C (src/E1414.C declares the same layout). */
struct Projectile {
    char active;              /* +00: the record state, 1 selects the ds:F2F8 path here */
    char pad01;
    int m02;                  /* +02 */
    int cell;                  /* +04: the slot heading the record belongs to */
    char m06;                 /* +06: its low two bits index ds:7246; ds:7262 is indexed by the whole byte (m07 * 4 + m06) */
    signed char dir;          /* +07: the page the record is seen from */
    int item;                  /* +08: indexes the ds:8DD0 item records */
    char m0A;                 /* +0A: row in the ds:F2F8 cell table */
    char m0B;                 /* +0B */
    char m0C;                 /* +0C: bit 6 gates a position recompute */
    unsigned char m0D;        /* +0D */
    int (far *fn)(struct Projectile far *rec); /* +0E */
};
extern struct Projectile gProjectiles[]; /*@ ds:DD2C */
extern signed char gProjectileDrawOrder[]; /*@ ds:7246 */   /* per page and low-2-bit index, the list slot (this unit) */
extern unsigned char gProjectileX[]; /*@ ds:71A6 */ /* per slot, per sprite, the x base (this unit) */
extern signed char gProjectileY[]; /*@ ds:71EE */   /* per gViewSlotRow slot, per sprite, the y base (this unit) */
extern signed char gProjectileFrame[]; /*@ ds:71FE */   /* per slot, per sprite, the frame (this unit) */
extern signed char gProjectileFixWidth[]; /*@ ds:725A */   /* per frame, the cell +2 value a nudge needs (this unit) */
extern signed char gProjectileFixX[]; /*@ ds:725E */   /* per frame, the x nudge added on that match (this unit) */
extern signed char gProjectileFlip[]; /*@ ds:7262 */   /* per record +7, per record +6, the draw wrapper (this unit) */
extern signed char gItemThrownShape[]; /*@ ds:1D07 */   /* per kind plus 0x2d side, a ds:F2F8 row or -1 (this unit) */
extern void far *gThrownIcons[][4]; /*@ ds:F2F8 */  /* far sprite cells, four per icon row (this unit) */

#define DD_SIDE_STRIDE 0x2d
#define NO_DD_ROW -1

/* ovl4:19C4: draws the map objects whose ds:DD2C record is seen from the current page. Only when
   gWallVmpSet[tile] is nonzero and gWallTypeFlags[tile] bit 7 is clear does it run (tile is the slot's gViewCells
   record byte, as in DrawSlotMonsters). It scans the ten 0x12-byte ds:DD2C records and, for each whose
   byte +0 is set and word +4 equals the slot's heading (gViewCellIds[slot]), puts its far pointer in
   the list slot gProjectileDrawOrder[gPartyDir * 5 + (record +6 & 3)]. Each non-empty slot picks a cell from the
   ds:F2F8 or ds:EC9A far sprite tables and a draw wrapper from ds:7262, at an x from ds:71A6 and a
   y from ds:71EE selected by the sprite base ds:1A2B[gPartyDir * 4 + (+6 & 3)] and the frame ds:71FE:
   a record whose byte +0 is 1 picks the ds:F2F8 row ds:1D07[kind + side * 0x2d] (kind is the
   ds:8DD0 item icon through ds:181F, side -1 when the record +7 matches neither the page nor the
   page xor 2) and shifts x by ds:725E[frame] when the sign-extended cell +2 byte matches
   ds:725A[frame];
   any other record uses the row byte +0xA directly and, when its +0xC has bit 6, recomputes x from
   ds:1D61 and y from the cell. A wrapper of 0 draws with DrawShapeClipped, any other with
   DrawShapeClippedFlipped. Called by DrawViewSlots for every slot. Purpose unknown: what the records, the
   sides and the wrappers mean. Three source forms TCC pins down: the ds:194B bit 7 test is written
   `gWallVmpSet[tile] == 0 || (gWallTypeFlags[tile] & 0x80)` with an empty then, the only form that tests the byte
   without sign-extending it; the frame guard is `if (frame >= 0) { body }` (an `if (frame < 0)
   continue;` gives the same bytes but reloads ES before the record check, a -Z difference); and the
   cell +2 test is a raw `<< 3 >> 3`, which for a signed char in a 16-bit int leaves the value
   unchanged, so it just compares the sign-extended byte with ds:725A[frame] (a `% 8` would compile
   to idiv). */
/*@ ovl4:19C4 */ void DrawSlotProjectiles(int slot)
{
    int tile;
    int page;
    int kind;
    int side;
    int spriteBase;
    int x;
    int y;
    int heading;
    int wrapper;
    struct Projectile far *rec;
    char far *cell;
    struct Projectile far *arr[5];
    int i;
    int frame;

    heading = gViewCellIds[slot];
    tile = gMaze[heading * 9 + gViewFrontSide];
    if (gWallVmpSet[tile] == 0 || (gWallTypeFlags[tile] & 0x80))
        ;
    else
        return;
    page = gPartyDir * 5;
    for (i = 0; i < 5; i++)
        arr[i] = 0;
    rec = gProjectiles;
    for (i = 0; i < 0xa; i++, rec++) {
        if (rec->active != 0 && rec->cell == heading)
            arr[gProjectileDrawOrder[page + (rec->m06 & 3)]] = rec;
    }
    for (page = 0; page < 5; page++) {
        if (arr[page] == 0)
            continue;
        rec = arr[page];
        spriteBase = gViewRelativePos[gPartyDir * 4 + (rec->m06 & 3)];
        x = gProjectileX[slot * 4 + spriteBase];
        y = gProjectileY[gViewSlotRow[slot] * 4 + spriteBase];
        frame = gProjectileFrame[slot * 4 + spriteBase];
        wrapper = 0;
        if (frame >= 0) {
            if (rec->active == 1) {
                kind = gIconPictureIndex[gItems[rec->item].icon];
                if (rec->dir == gPartyDir)
                    side = 0;
                else if (rec->dir == (gPartyDir ^ 2))
                    side = 1;
                else
                    side = -1;
                if (side != -1 && gItemThrownShape[kind + side * DD_SIDE_STRIDE] != NO_DD_ROW) {
                    cell = (char far *)gThrownIcons[gItemThrownShape[side * DD_SIDE_STRIDE + kind]][frame];
                    wrapper = gProjectileFlip[rec->dir * 4 + rec->m06];
                } else {
                    cell = (char far *)gItemL1Icons[kind][frame];
                    wrapper = (rec->dir == ((gPartyDir + 1) & 3)) ? 1 : 0;
                }
                if ((((signed char far *)cell)[2] << 3 >> 3) == gProjectileFixWidth[frame])
                    x += gProjectileFixX[frame];
            } else {
                cell = (char far *)gThrownIcons[rec->m0A][frame];
                wrapper = gProjectileFlip[rec->dir * 4 + rec->m06];
                if (rec->m0C & 0x40) {
                    x = gViewPosX[(slot * 5 + 4) * 2] + 0x58;
                    x -= (((signed char far *)cell)[2] << 3) >> 1;
                    y = 0x2c - (((signed char far *)cell)[1] >> 1);
                }
            }
            if (wrapper == 0)
                DrawShapeClipped(2, cell, x, y, 5);
            else
                DrawShapeClippedFlipped(2, cell, x, y, 5);
        }
    }
}

extern signed char gTeleporterLeftOffset[]; /*@ ds:7272 */   /* per-column x base correction (this unit) */
extern signed char gTeleporterTopY[]; /*@ ds:7275 */   /* per-column y base (this unit) */
extern void far *gIntroCells[]; /*@ ds:F4BE */  /* sprite cell table (src/EOV2.C) */
extern int gAnimWallPhase; /*@ ds:FB78 */            /* toggled by 1414:04B0 (src/E1414.C) */
extern signed char far gCellBytes[]; /*@ 20AB:0000 */  /* the cell bytes drawn here */

#define CELL_ROW_BYTES 0x1a   /* one sprite row: CELL_COLS cells of two bytes (x then y) */
#define CELL_COLS 13

/* ovl4:1D20: draws the "0x34" object the view shows for one view slot: two columns of 13 sprite
   cells through DrawShapeClipped(mode 2, cell, x, y, region 5). The column pair is ds:1A09[slot]
   counted back from 2 (a slot with a larger ds:1A09 draws fewer). Each column i uses the far
   sprite cell ds:F4BE[(col * 2 + i) ^ ds:FB78] and reads its cells from the far bytes at
   20AB:0000, row (col*2+i) * CELL_ROW_BYTES, two bytes a cell: byte 0 shifts x and byte 1 shifts y,
   on top of the per-column base ds:19E5[slot] - ds:7272[col] (x) and ds:7275[col] (y). The first
   cell (idx 0) is nudged by -4 on both axes. Called by DrawViewSlots at ovl4:1ECB when the slot's
   heading is 0x34. Purpose unknown: what the object is and what the 20AB:0000 bytes mean. */
/*@ ovl4:1D20 */ void DrawSlotTeleporter(int slot)
{
    int col;
    int j;
    int xbase;
    int ybase;
    int xoff;
    int yoff;
    int i;
    int idx;
    void far *cells;

    col = 2 - gViewSlotRow[slot];
    if (col < 0)
        return;
    xbase = gViewSlotCenterX[slot] - gTeleporterLeftOffset[col];
    ybase = gTeleporterTopY[col];
    for (i = 0; i < 2; i++) {
        idx = col * 2 + i;
        xoff = yoff = 0;
        if (idx == 0) {
            xoff = -4;
            yoff = -4;
        }
        cells = gIntroCells[idx ^ gAnimWallPhase];
        for (j = 0; j < CELL_COLS; j++) {
            DrawShapeClipped(2, cells,
                           xoff + xbase + gCellBytes[idx * CELL_ROW_BYTES + j * 2],
                           yoff + ybase + gCellBytes[idx * CELL_ROW_BYTES + j * 2 + 1],
                           5);
        }
    }
}

extern signed char gViewDrawOrder[]; /*@ ds:7278 */     /* per-view-slot byte (this unit) */

/* ovl4:1E18: walks the view slots from `start` to 17. For each, ds:7278 gives the slot number,
   ComputeViewSpan computes the visible column span into ds:FE3E/ds:FE40 and the slot is drawn only
   if the span is not empty: DrawSlotDecorations draws the slot, DrawSlotItems draws the sprite at word +5 of the
   slot's ds:EF6A tile record, and for slots below 15 Ovl4_1411 (when ds:194B of the record's
   heading byte has bit 3) and DrawSlotMonsters (when byte +4 of the record has any of bits 0..2).
   DrawSlotProjectiles always runs, and DrawSlotTeleporter only for the heading value 0x34. Called by DrawViewFrame at
   ovl4:1F75. Purpose unknown: what the slot numbers and the 0x34 heading mean. */
/*@ ovl4:1E18 */ void DrawViewSlots(int start)
{
    int i;
    int slot;
    int heading;

    for (i = start; i < 18; i++) {
        slot = gViewDrawOrder[i];
        ComputeViewSpan(slot, &gViewColLeft, &gViewColRight, 5);
        if (gViewColRight < gViewColLeft)
            continue;
        DrawSlotDecorations(slot);
        heading = gViewCells[slot][gViewFrontSide];
        if (*(int far *)(gViewCells[slot] + 5) != 0)
            DrawSlotItems(slot, *(int far *)(gViewCells[slot] + 5));
        if (slot < 0xf) {
            if (gWallTypeFlags[heading] & 8)
                DrawSlotDoor(slot);
            if (*(gViewCells[slot] + 4) & 7)
                DrawSlotMonsters(slot);
        }
        DrawSlotProjectiles(slot);
        if (heading == 0x34)
            DrawSlotTeleporter(slot);
    }
}

extern int gCompassDir; /*@ ds:7296 */              /* the page the rows were last drawn for (this unit) */
extern int gCompassY[]; /*@ ds:7290 */             /* per-row y (this unit) */
extern int gCompassX[]; /*@ ds:728A */             /* per-row x (this unit) */
extern void far *gCompassIcons[]; /*@ ds:A92A */    /* per-page, per-row far image pointers (this unit) */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */

/* ovl4:1EDE: redraws the three view rows for the current page. It returns early when ds:7296
   already holds gPartyDir; otherwise it hides the pointer below the view box (0x46, 0x79, 0xb0, 0xaf),
   draws the three rows through DrawShape(0, gCompassIcons[gPartyDir + 4 * i], gCompassX[i], gCompassY[i], 0),
   shows the pointer and remembers gPartyDir in ds:7296. Called by DrawViewFrame at ovl4:202F. Purpose
   unknown: what the rows are. */
/*@ ovl4:1EDE */ void DrawCompass(void)
{
    int i;

    if (gCompassDir == gPartyDir)
        return;
    HidePointerZone(0x46, 0x79, 0xb0, 0xaf);
    for (i = 0; i < 3; i++)
        DrawShape(0, gCompassIcons[gPartyDir + 4 * i], gCompassX[i], gCompassY[i], 0);
    ShowPointerZone();
    gCompassDir = gPartyDir;
}

extern char gLevelPalettePending; /*@ ds:FB45 */           /* request flag, cleared here (src/E07A9.C) */
extern char gQuietObjectUpdate; /*@ ds:F442 */           /* gate checked here (src/E08E6.C) */
extern char gFlag; /*@ ds:F3B9 */           /* selects a table set (src/E0ACD.C) */
extern char gViewDirty; /*@ ds:DF4C */           /* a notify flag, cleared here (src/E08E6.C) */
extern unsigned long gFrameWaitTicks; /*@ ds:849A */  /* the frame/effect busy counter (src/E08E6.C) */
extern void far *gWallBlocks; /*@ ds:DCA6 */   /* the far pointer DrawTile draws (src/E07A9.C) */
extern unsigned char far *gPaletteBuf; /*@ ds:FE68 */  /* the loaded palette buffer (src/E07C8.C) */
extern void (far *gSetPalette)(void far *table); /*@ ds:80BA */  /* palette setter (src/E07C8.C) */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void DrawTile(void far *src); /*@ 17A6:03E7 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */

/* ovl4:1F51: draws one frame of the view: BuildViewBlockMap draws the objects into the page buffer (with
   gPartyCell and gPartyDir), DrawTile blits it from gWallBlocks, DrawViewSlots(0) draws the slots, and
   when gLevelPalettePending is set and gGraphicsChoice is 3 (MCGA/VGA, re/globals.toml ds:8102) it loads the
   palette gPaletteBuf and clears gLevelPalettePending; when `full` is also set, it first blanks the view box (gFillRect
   with pattern 0xc). Then it waits for gFrameWaitTicks
   to reach zero, and, when `full` is set and gQuietObjectUpdate is clear, clips a rectangle. It always plays
   sound 0 and, when gFlag is zero, runs DrawCompass; finally it clears gViewDirty. Called from the game
   loop. */
/*@ ovl4:1F51 */ void DrawViewFrame(int full)
{
    BuildViewBlockMap(gPartyCell, gPartyDir);
    DrawTile(gWallBlocks);
    DrawViewSlots(0);
    if (gLevelPalettePending && gGraphicsChoice == 3) {
        if (full) {
            HidePointerZone(0, 0, 0xb0, 0x78);
            gFillRect(0, 0, 0xb0, 0x78, 0xc);
            ShowPointerZone();
        }
        gSetPalette(gPaletteBuf);
        gLevelPalettePending = 0;
    }
    while (gFrameWaitTicks != 0)
        ;
    if (full && !gQuietObjectUpdate) {
        HidePointerZone(0, 0, 0xb0, 0x78);
        CopyRegion(0, 0, 0, 0, 0x16, 0x78, 2, 0);
        ShowPointerZone();
    }
    PlaySoundDefault(0);
    if (!gFlag)
        DrawCompass();
    gViewDirty = 0;
}
/* ovl4:2039: discards its work: si = 0 then si++ and return. Purpose unknown. */
/*@ ovl4:2039 */ void PreloadViewOverlay(void)
{
    int si;

    si = 0;
    si++;
}