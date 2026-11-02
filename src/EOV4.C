/* Unit at overlay 4 of EOB.EXE (8259 bytes, 18 functions): the first-person view renderer. It
   draws the map objects, walls and creature sprites of the current page into the view region from
   the object records at ds:8A82 and the 9-byte tile records at ds:B894, clipped through the
   drawing wrappers in src/E082C.C. The unit is still being lifted. */

/*@flags -ml -O -Z -v */

extern signed char gHead199D[]; /*@ ds:199D */  /* rows of 18 signed heading offsets */
extern signed char gB894[]; /*@ ds:B894 */    /* 9-byte tile records (src/E08E6.C) */
extern signed char far *gEF6A[]; /*@ ds:EF6A */ /* 18 far pointers into gB894 (src/E1414.C) */
extern signed char gA9C6; /*@ ds:A9C6 */        /* offset into the gEF6A target (src/E0ACD.C) */
extern signed char g194B[]; /*@ ds:194B */      /* per-slot byte flags (src/E08E6.C) */
extern signed char g1879[]; /*@ ds:1879 */      /* per-object count-1 (this unit) */
extern int gAngleA9C8[]; /*@ ds:A9C8 */         /* 18 heading words, the masked sum (this unit) */
extern int gViewSlots[]; /*@ ds:DCB2 */         /* 18 {left,right} view-slot pairs (this unit) */

/* ovl4:0000: for each of the 18 view slots, compute a heading word from `base` plus the slot's
   signed offset in the 18-byte row `row` of ds:199D (row is scaled by 18 first), masked to 10 bits,
   store it in the heading table at ds:A9C8, point the slot's ds:EF6A entry at the 9-byte tile
   record for that heading in ds:B894, and reset the slot to -1 in both halves of the view-slot
   array at ds:DCB2. Called by Ovl4_044B at ovl4:04D3. Purpose unknown: the meaning of `row` and of
   the 18 slots. */
/*@ ovl4:0000 */ void InitViewSlots(int base, int row)
{
    int heading;
    int i;

    row *= 18;
    for (i = 0; i < 18; i++) {
        heading = gAngleA9C8[i] = (base + gHead199D[row + i]) & 0x3FF;
        gEF6A[i] = (signed char far *)&gB894[heading * 9];
        gViewSlots[i] = gViewSlots[i + 18] = -1;
    }
}

extern void SetRegionExtents(int region, int left, int top, int width, int height); /*@ 1DE7:02FE */

/* ovl4:0066: wrapper for SetRegionExtents: builds a region record from a box whose left and right
   edges are in 8-pixel columns and whose top and bottom are in units of 8 pixel rows (top is
   scaled by 8, height is (bottom - top) * 8). Called by Ovl4_0097 at ovl4:0211 and Ovl4_1411 at
   ovl4:19A0 to fit the view region to the visible tile span. */
/*@ ovl4:0066 */ void SetRegionRect(int region, int left, int top, int right, int bottom)
{
    SetRegionExtents(region, left, top * 8, right - left, (bottom - top) * 8);
}

extern signed char g6B82[]; /*@ ds:6B82 */   /* per slot,i signed column adjustment (this unit) */
extern signed char g6CC6[]; /*@ ds:6CC6 */   /* per slot,i signed {left,right} bounds (this unit) */

#define VIEW_SLOTS 18       /* the ds:EF6A tile records a slot's span is built from */
#define VIEW_RIGHT 0x16     /* the rightmost column of the view, in 8-pixel columns */
#define VIEW_BOTTOM 0xf     /* the bottom of the view region, in 8-pixel rows */
#define ADJUST_SKIP -40     /* a ds:6B82 adjustment that leaves the span unchanged */
#define ADJUST_EMPTY -41    /* a ds:6B82 adjustment that forces an empty span */
#define TILE_FLAG 8         /* ds:194B bit 3: the record carries ds:6CC6 bounds, not an adjustment */

/* ovl4:0097: computes the visible left and right columns of one view slot and fits region `region`
   to them. `slot` indexes the view slots and gEF6A; `left`/`right` receive the span (2-pixel
   columns). When the slot's cached pair in gViewSlots (at ds:DCB2, two words per slot) is not -1 it
   returns that pair. Otherwise it starts at 0..VIEW_RIGHT and walks the VIEW_SLOTS tile records
   gEF6A[i][gA9C6]: a record whose ds:194B heading flag has bit 3 clear contributes its ds:6B82
   column adjustment (a positive value pulls *right left, a negative one pulls *left right; an
   adjustment of ADJUST_SKIP on a record with a zero ds:1879 count is ignored and ADJUST_EMPTY
   forces an empty span), a record whose bit 3 is set narrows the span with its ds:6CC6
   {left,right} bounds; the walk stops when *right falls below *left. The pair is cached back into
   gViewSlots and SetRegionRect fits `region` to (left, right). Called by Ovl4_1E18 at ovl4:1E23.
   Purpose unknown: what the records and adjustments mean.
   QUIRK ovl4:0097: the local `j` is used as the pair index before the loop and as the ds:6CC6
   column index inside it; both are `2 * slot`, so the value is reused rather than recomputed. */
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
            heading = gEF6A[i][gA9C6];
            if (!(g194B[heading] & TILE_FLAG)) {
                adjust = g6B82[slot + i];
                if (g1879[heading] == 0 || adjust == ADJUST_SKIP)
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
                if (g6CC6[j] > *left)
                    *left = g6CC6[j];
                j++;
                if (g6CC6[j] < *right)
                    *right = g6CC6[j];
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
    int saved0;
    int saved1;
    int saved2;
    int saved3;
};

extern struct Region gRegions[]; /*@ ds:009F */
extern int gFBC8; /*@ ds:FBC8 */                 /* the current action (src/E0D1C.C) */
extern int gFBCC; /*@ ds:FBCC */                 /* the current page 0..3 (src/E08E6.C) */
extern signed char g6F4E[]; /*@ ds:6F4E */       /* row value table this unit indexes */
extern signed char g6F6D[]; /*@ ds:6F6D */       /* per-page index into ds:6F4E */
extern signed char g6F79[]; /*@ ds:6F79 */       /* per-page index into ds:6F4E */
extern signed char g1A09[]; /*@ ds:1A09 */       /* per-object index into ds:6F4E */

/* ovl4:021E: computes the top and bottom pixel rows of one column of the current page's view
   and fits region `region` to them. The column value comes from ds:6F4E at the row index
   ds:6F6D[gFBC8] + ds:1A09[col] for the top and ds:6F79[gFBC8] + ds:1A09[col] for the bottom;
   then SetRegionExtents rewrites the region with its current left and width and the new top and
   height. Called by Ovl4_1411 at ovl4:149B and ovl4:179E. Purpose unknown: what the tables
   mean. */
/*@ ovl4:021E */ void SetRegionColumnExtent(char col, int far *top, int far *bottom, int region)
{
    char topRow = g6F6D[gFBC8];
    char bottomRow = g6F79[gFBC8];

    *top = g6F4E[topRow + g1A09[col]];
    *bottom = g6F4E[bottomRow + g1A09[col]];
    SetRegionExtents(region, gRegions[region].left, *top, gRegions[region].width,
                     *bottom - *top);
}

extern unsigned char far *gBufF43E; /*@ ds:F43E */   /* 6000-byte start-up buffer (src/E07A9.C) */
extern signed char g1879[]; /*@ ds:1879 */           /* per-object count-1 (this unit) */
extern unsigned char gViewBufADB0[]; /*@ ds:ADB0 */  /* the view pixel buffer (this unit) */

/* ovl4:02A5: copies a rectangle of word pixels out of the page buffer pointed to by ds:F43E into
   the view buffer at ds:ADB0. `src` starts at (g1879[page] - 1) * 0x35E + x2 * 2 + 0x294 in
   ds:F43E; each row walks `width` words and each of the `height` rows advances the destination by
   a 0x16-word row. Words outside columns 0..0x15 are skipped, and only nonzero source words are
   stored. Called by Ovl4_044B at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the buffers hold.
   The three duplicated increment tails are kept apart by the file's -v (debug information), as
   the original shows; without it our build merges the continue tails (whitepaper 2.1.1). */
/*@ ovl4:02A5 */ void CopyViewColumn(char x, char y, char page, int x2, char width, char height)
{
    unsigned int far *dst;
    unsigned int far *src;
    int row;
    int col;

    if (g1879[page] == 0)
        return;
    src = (unsigned int far *)(gBufF43E + (g1879[page] - 1) * 0x35E + x2 * 2);
    src += 0x14A;
    for (row = 0; row < height; row++) {
        dst = (unsigned int far *)(gViewBufADB0 + ((y + row) * 0x16 + x) * 2);
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
   (g1879[page] - 1) * 0x35E + x2 * 2 + 0x294; for each of the `height` rows and `width` columns it
   reads the word at (src + width * row)[width - 1 - col], skips it when zero and stores it to
   ds:ADB0[(y + row) * 0x16 + x + col] with bit 0x4000 toggled, skipping columns outside 0..0x15.
   Called by Ovl4_044B at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the bit marks. */
/*@ ovl4:036A */ void ToggleViewColumn(char x, char y, char page, int x2, char width, char height)
{
    unsigned int far *src;
    int row;
    int col;
    int pixel;

    if (g1879[page] == 0)
        return;
    src = (unsigned int far *)(gBufF43E + (g1879[page] - 1) * 0x35E + x2 * 2);
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
            ((unsigned int *)gViewBufADB0)[(y + row) * 0x16 + x + col] = pixel;
        }
    }
}

extern signed char g194B[]; /*@ ds:194B */  /* per-slot byte flags (src/E08E6.C) */

/* ovl4:042C: returns 1 when `slot` is nonzero and the bit 3 of its ds:194B flag byte is clear
   (the slot is a live map object), else 0. Called by Ovl4_044B at ovl4:0563 and ovl4:05D8. */
/*@ ovl4:042C */ int IsSlotLive(int slot)
{
    return slot != 0 && !(g194B[slot] & 8);
}

extern signed char g1991[]; /*@ ds:1991 */   /* per-page index into a gEF6A target (src/E08E6.C) */
extern signed char g1995[]; /*@ ds:1995 */   /* per-page index into a gEF6A target (this unit) */
extern signed char g1999[]; /*@ ds:1999 */   /* per-page index into a gEF6A target (this unit) */
extern signed char gEFB9; /*@ ds:EFB9 */     /* the second per-page gEF6A target index (this unit) */
extern signed char gEFD0; /*@ ds:EFD0 */     /* the third per-page gEF6A target index (this unit) */
extern int gF4BC; /*@ ds:F4BC */             /* the cell and page parity (this unit) */

/* ovl4:044B: draws the whole view page. It caches the three per-page gEF6A target offsets
   (g1991, g1995 and g1999 at the page index, into gA9C6, gEFB9 and gEFD0), clears the 0x14a-word
   view buffer at ds:ADB0, computes a parity from the cell (`base`) and the page into ds:F4BC, copies
   or toggles the top strip (page 1, x2 -0x14a), initialises the 18 view slots through InitViewSlots,
   and then walks a fixed list of rectangles. Each rectangle selects one of the page's tile records
   (gEF6A[i] at one of the three offsets) and, when the byte is a live slot (or a nonzero record,
   for the ones that are always drawn), copies or toggles its column (x, y, width, height, source x2)
   from the page buffer into the view buffer. The records with a ds:194B bit 3 also pick a second
   record and draw whichever of the two the flag selects. Called by DrawViewFrame at ovl4:1F6B.
   Purpose unknown: what the tile records and the many rectangles are. */
/*@ ovl4:044B */ void Ovl4_044B(int base, int row)
{
    int i;
    int slot;
    int slot2;

    gA9C6 = g1991[gFBCC];
    gEFB9 = g1995[gFBCC];
    gEFD0 = g1999[gFBCC];
    for (i = 0; i < 0x14a; i++)
        ((unsigned int *)gViewBufADB0)[i] = 0;
    gF4BC = ((base >> 5) + (base & 0x1f) + gFBCC) & 1;
    if (gF4BC == 0)
        CopyViewColumn(0, 0xf, 1, 0xfeb6, 0x16, 0xf);
    else
        ToggleViewColumn(0, 0xf, 1, 0xfeb6, 0x16, 0xf);
    InitViewSlots(base, row);

    slot = gEF6A[0][gEFB9];
    if (slot)
        CopyViewColumn(0xfe, 3, slot, 0x66, 3, 5);
    slot = gEF6A[6][gEFD0];
    if (slot)
        ToggleViewColumn(0x15, 3, slot, 0x66, 3, 5);
    slot = gEF6A[1][gEFB9];
    slot2 = gEF6A[2][gA9C6];
    if (IsSlotLive(slot) && !(g194B[slot2] & 8))
        CopyViewColumn(2, 3, slot, 0x66, 3, 5);
    else if (slot != 0 && (g194B[slot2] & 8))
        CopyViewColumn(2, 3, slot2, 0x66, 3, 5);
    slot = gEF6A[5][gEFD0];
    slot2 = gEF6A[4][gA9C6];
    if (IsSlotLive(slot) && !(g194B[slot2] & 8))
        ToggleViewColumn(0x11, 3, slot, 0x66, 3, 5);
    else if (slot != 0 && (g194B[slot2] & 8))
        ToggleViewColumn(0x11, 3, slot2, 0x66, 3, 5);
    slot = gEF6A[2][gEFB9];
    if (slot)
        CopyViewColumn(8, 3, slot, 0x61, 1, 5);
    slot = gEF6A[4][gEFD0];
    if (slot)
        ToggleViewColumn(0xd, 3, slot, 0x61, 1, 5);
    if (IsSlotLive(gEF6A[1][gA9C6]))
        CopyViewColumn(0xfc, 3, gEF6A[1][gA9C6], 0x81, 6, 5);
    if (IsSlotLive(gEF6A[5][gA9C6]))
        CopyViewColumn(0x14, 3, gEF6A[5][gA9C6], 0x81, 6, 5);
    if (IsSlotLive(gEF6A[2][gA9C6]))
        CopyViewColumn(2, 3, gEF6A[2][gA9C6], 0x81, 6, 5);
    if (IsSlotLive(gEF6A[4][gA9C6]))
        CopyViewColumn(0xe, 3, gEF6A[4][gA9C6], 0x81, 6, 5);
    slot = gEF6A[3][gA9C6];
    if (slot)
        CopyViewColumn(8, 3, slot, 0x81, 6, 5);
    slot = gEF6A[7][gEFB9];
    if (slot)
        CopyViewColumn(0, 3, slot, 0x75, 2, 6);
    slot = gEF6A[11][gEFD0];
    if (slot)
        ToggleViewColumn(0x14, 3, slot, 0x75, 2, 6);
    slot = gEF6A[8][gEFB9];
    if (slot)
        CopyViewColumn(6, 2, slot, 0x51, 2, 8);
    slot = gEF6A[10][gEFD0];
    if (slot)
        ToggleViewColumn(0xe, 2, slot, 0x51, 2, 8);
    if (IsSlotLive(gEF6A[8][gA9C6]))
        CopyViewColumn(0xfc, 2, gEF6A[8][gA9C6], 0x9f, 0xa, 8);
    if (IsSlotLive(gEF6A[10][gA9C6]))
        CopyViewColumn(0x10, 2, gEF6A[10][gA9C6], 0x9f, 0xa, 8);
    slot = gEF6A[9][gA9C6];
    if (slot)
        CopyViewColumn(6, 2, slot, 0x9f, 0xa, 8);
    slot = gEF6A[12][gEFB9];
    if (slot)
        CopyViewColumn(3, 1, slot, 0x2d, 3, 0xc);
    slot = gEF6A[14][gEFD0];
    if (slot)
        ToggleViewColumn(0x10, 1, slot, 0x2d, 3, 0xc);
    slot = gEF6A[12][gA9C6];
    slot2 = gEF6A[13][gA9C6];
    if (!(g194B[slot] & 8))
        CopyViewColumn(0xf3, 1, slot, 0xef, 0x10, 0xc);
    slot = gEF6A[14][gA9C6];
    if (!(g194B[slot] & 8))
        CopyViewColumn(0x13, 1, slot, 0xef, 0x10, 0xc);
    slot = gEF6A[13][gA9C6];
    if (slot)
        CopyViewColumn(3, 1, slot, 0xef, 0x10, 0xc);
    slot = gEF6A[15][gEFB9];
    slot2 = gEF6A[17][gEFD0];
    if (gFBC8 > 6 && gFBC8 < 0xa) {
        if (slot2 == 0x17)
            slot2 = 0;
        if (slot2 == 0x18)
            slot2 = 0;
    } else if (gFBC8 > 3) {
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
    signed char m00;
    signed char m01;
    unsigned char flags;
    signed char icon;       /* +3: indexes ds:181F */
    signed char m02;
    signed char m03;        /* +5: the kind tested here */
    int m04;
    int m06;                /* +8: one link of the circular list */
    int m08;
    signed char m0A;
    signed char m0B;
};
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern signed char g1A2B[]; /*@ ds:1A2B */   /* per-page sprite base (src/E0DE8.C) */
extern signed char g181F[]; /*@ ds:181F */   /* byte table indexed by an item icon (src/E122E.C) */
extern signed char g6F86[]; /*@ ds:6F86 */   /* per-slot byte (this unit) */
extern signed char g6F98[]; /*@ ds:6F98 */   /* per-slot, per-part byte (this unit) */
extern int g6FA8[]; /*@ ds:6FA8 */           /* per-slot y base (this unit) */
extern int g6FAE[]; /*@ ds:6FAE */           /* per-part x/y nudge (this unit) */
extern int g1D61[]; /*@ ds:1D61 */           /* per-slot, per-part x table (this unit) */
extern int g1D63[]; /*@ ds:1D63 */           /* per-slot, per-part y table (this unit) */
extern void far *gItemL1Icons[][4]; /*@ ds:EC9A */  /* far sprite cells, one 4-cell row per icon (this unit) */
extern int g19E5[]; /*@ ds:19E5 */           /* per-slot x base (this unit) */
extern int gViewColLeft; /*@ ds:FE3E */      /* the visible left column (this unit) */
extern int gViewColRight; /*@ ds:FE40 */     /* the visible right column (this unit) */
extern void DrawRegion000F(int mode, void far *src, int x, int y, int region); /*@ 082C:063B */

/* ovl4:0AD0: draws the map object that one view slot's tile record names at word +5, as a run of
   sprite cells taken from the circular item list at ds:8DD0. `slot` is the view slot, `field` the
   index of the object: it starts at gItems[field]'s +8 link (the item's list), remembers that link
   as the head and walks the list back to it. For each item, the kind +5 picks the sprite base
   ds:1A2B[gFBCC * 4 + (kind & 3)]; the cell position comes from ds:1D61/ds:1D63 (or, when the kind
   is 8, from g19E5[slot] and g6FA8[g1A09[slot]] with sprite 0) and is nudged by ds:6FAE at
   `field & 7` and `(field >> 1) & 7`; the cell itself is ds:EC9A[g181F[icon]][frame], its byte +2
   (in 8-pixel units) and byte +1 shift the position. Items whose kind is 8 or below 4 are drawn;
   the rest are skipped. The tile's ds:194B flags and its ds:1879 count gate the slot; bit 3 of the
   flags also recomputes the span of ds:6F86[slot] and pulls the object up 4 pixels. After the list,
   when that span slot was set, the slot's own span is recomputed. Called by DrawViewSlots when the
   tile record's word +5 is nonzero. Purpose unknown: what the tables and the object are. */
/*@ ovl4:0AD0 */ void Ovl4_0AD0(int slot, int field)
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
    tile = gB894[gAngleA9C8[slotReg] * 9 + gA9C6];
    if (slotReg == 0x10)
        flags = 0x80;
    else
        flags = g194B[tile];
    if (g1879[tile] != 0 && !(flags & 0x80))
        return;

    field = gItems[field].m06;
    head = field;
    first = 1;
    while (field != head || first) {
        rec = &gItems[field];
        is8 = (rec->m03 == 8);
        if (is8 == 1 || rec->m03 < 4) {
            spanSlot = -1;
            sprite = g1A2B[gFBCC * 4 + (rec->m03 & 3)];
            if (is8)
                sprite = 4;
            k = (slotReg * 5 + sprite) * 2;
            x = g1D61[k] + 0x58;
            if (is8) {
                x = g19E5[slotReg];
                y = g6FA8[g1A09[slotReg]];
                sprite = 0;
            } else {
                y = g1D63[k] + 0x7c;
            }
            frame = g6F98[g1A09[slotReg] * 4 + sprite];
            if ((flags & 8) && sprite < 2 && frame != 0) {
                spanSlot = g6F86[slotReg];
                if (spanSlot != -1)
                    ComputeViewSpan(spanSlot, &gViewColLeft, &gViewColRight, 5);
                y -= 4;
            }
            if (frame >= 0) {
                cell = gItemL1Icons[g181F[rec->icon]][frame];
                x -= (((signed char far *)cell)[2] << 3) >> 1;
                x += g6FAE[field & 7] * 2;
                y -= ((signed char far *)cell)[1];
                y += g6FAE[(field >> 1) & 7];
                DrawRegion000F(2, cell, x, y, 5);
            }
        }
        field = rec->m06;
        first = 0;
        if (spanSlot != -1)
            ComputeViewSpan(slotReg, &gViewColLeft, &gViewColRight, 5);
    }
}

/* The 28-byte object records at ds:8A82 and the 27-byte records at ds:1A3B (src/E08E6.C). */
struct Obj28 {
    char b0;                /* +0: index into the ds:1A3B table */
    signed char kind;       /* +1 */
    int id;                 /* +2 */
    char frame;             /* +4 */
    char orient;            /* +5 */
    char toggle;            /* +6 */
    char m07;               /* +7 */
    signed char type;       /* +8 */
    signed char state;      /* +9 */
    char m0A;               /* +A */
    char timer;             /* +B */
    int hitPoints;          /* +C */
    int count;              /* +E */
    int m10;
    int m12;
    int m14;
    char m16;               /* +16: flag byte */
    char m17;               /* +17 */
    char m18;
    char m19;
    char m1A;
    char m1B;
};
struct Entry27 {
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
extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern struct Entry27 gTable1A3B[]; /*@ ds:1A3B */
extern signed char g6FBE[]; /*@ ds:6FBE */   /* byte table indexed by g1A09[slot] (this unit) */
extern signed char g7001[]; /*@ ds:7001 */   /* per-page, per-frame byte (this unit) */
extern signed char g6FC1[]; /*@ ds:6FC1 */   /* sprite table A (this unit) */
extern signed char g6FE1[]; /*@ ds:6FE1 */   /* sprite table B (this unit) */
extern signed char g7015[]; /*@ ds:7015 */   /* per-row y nudge (this unit) */
extern void far *gB7FE[]; /*@ ds:B7FE */     /* far sprite cells indexed by sprite plus nudge (this unit) */
extern void far *gBufDCFC; /*@ ds:DCFC */    /* start-up far allocation, 4096 bytes (src/E07A9.C) */
extern int gGraphicsChoice; /*@ ds:8102 */             /* the video mode/format (src/E07A9.C) */
extern int gPartyCell; /*@ ds:FBCA */        /* the current map cell id (src/E08E6.C) */
extern void (far *gFn80D2)(char far *map); /*@ ds:80D2 */  /* the far colour-table callback (src/E07A9.C) */
extern char g1CD3[]; /*@ ds:1CD3 */
extern char g1CE3[]; /*@ ds:1CE3 */
extern void BlitSpriteEga(void far *src, void far *dst); /*@ 17A6:1715 */
extern void BlitSpriteCga(void far *src, void far *dst); /*@ 17A6:11CD */
extern void BlitSpriteDefault(void far *src, void far *dst); /*@ 17A6:0FFA */
extern void DrawRegion004D(int mode, void far *src, int x, int y, int region); /*@ 082C:0681 */
extern void DrawRegion008B(int mode, void far *src, int x, int y, int region); /*@ 082C:06C7 */
extern void DrawRegion00C9(int mode, void far *src, int x, int y, int region); /*@ 082C:070D */

/* ovl4:0D1B: draws the map objects whose ds:8A82 record id matches the cell the player's facing
   points at. The cell id is (gPartyCell + gHead199D[gFBCC * 18 + slot]) & 0x3ff; the 30 object
   records are scanned and each matching one is put in a 5-entry far-pointer slot indexed by
   g7001[gFBCC * 5 + obj->frame]. Each non-empty slot is then blitted g6FBE[g1A09[slot]] times into
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
/*@ ovl4:0D1B */ void Ovl4_0D1B(int slot)
{
    struct Entry27 far *info;
    struct Obj28 far *obj;
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
    struct Obj28 far *arr[5];
    register int i;
    register int y;

    tileRow = g6FBE[g1A09[slot]];
    cellId = (gPartyCell + gHead199D[gFBCC * 18 + slot]) & 0x3ff;
    page5 = gFBCC * 5;
    page4 = gFBCC * 4;
    for (i = 0; i < 5; i++)
        arr[i] = 0;
    obj = gObjs;
    i = 0;
    while (i < 0x1e) {
        if (obj->id == cellId)
            arr[g7001[page5 + obj->frame]] = obj;
        i++;
        obj++;
    }
    for (page5 = 0; page5 < 5; page5++) {
        if (arr[page5] == 0)
            continue;
        obj = arr[page5];
        info = &gTable1A3B[obj->b0];
        if (info->m0E & 2)
            sprite = g6FC1[(obj->toggle << 4) + page4 + obj->orient];
        else
            sprite = g6FE1[(obj->toggle << 4) + page4 + obj->orient];
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
        cell = gB7FE[spriteAbs + nudge];
        for (n = tileRow; n != 0; n--) {
            if (gGraphicsChoice == 2)
                BlitSpriteEga(cell, gBufDCFC);
            else if (gGraphicsChoice == 0)
                BlitSpriteCga(cell, gBufDCFC);
            else
                BlitSpriteDefault(cell, gBufDCFC);
            cell = gBufDCFC;
        }
        if (obj->frame != 4)
            spriteBase = g1A2B[page4 + obj->frame];
        else
            spriteBase = 4;
        k = (slot * 5 + spriteBase) * 2;
        x = g1D61[k] + 0x58;
        y = g1D63[k] + 0x7f;
        if (gTable1A3B[obj->b0].m15 == 1 && (spriteAbs == 1 || spriteAbs > 3))
            y += g7015[tileRow];
        x -= (((signed char far *)cell)[2] << 3) >> 1;
        y -= ((signed char far *)cell)[1];
        x += (obj->m17 & 0xf0) >> 4;
        y += obj->m17 & 0xf;
        if (obj->m16 & 2) {
            if (gGraphicsChoice == 0)
                gFn80D2(g1CE3);
            else
                gFn80D2(g1CD3);
            if (sprite >= 0)
                DrawRegion008B(2, cell, x, y, 5);
            else
                DrawRegion00C9(2, cell, x, y, 5);
        } else {
            if (sprite >= 0)
                DrawRegion000F(2, cell, x, y, 5);
            else
                DrawRegion004D(2, cell, x, y, 5);
        }
    }
}
extern signed char gTable18BF[]; /*@ ds:18BF */   /* per-object-kind table (src/E122E.C) */
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

/* ovl4:1091: draws the wall faces of one view slot. The slot has two faces (ds:7018[slot*2] and
   ds:7018[slot*2+1]); each face, when its far pointer is set, starts a chain of 0x34-byte wall
   records at ds:E06A. The chain head is gTable18BF of the byte the face's first byte selects in the
   slot's ds:EF6A tile record, and `next` (which is read after the index is decremented) walks it.
   The face's signed offset ds:70A8[face] picks the cell within each record (b = |offset| - 1) and
   its sign picks the DrawRegion wrapper: a nonnegative offset draws the cell at its x, a negative
   one mirrors it about the 0xb0-pixel screen. The second face (slot*2) also reads the record's
   flags: bit 0 (or bit 1 with the parity ds:F4BC) reflects the offset, and bit 2 adds either
   g1D61[(slot*5+4)*2] or ds:70CC[slot] to x. Called by DrawViewSlots at ovl4:1E2E. Purpose unknown:
   what the wall faces and the ds:E06A records are. */
/*@ ovl4:1091 */ void Ovl4_1091(int slot)
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
        for (n = gTable18BF[gEF6A[slot][offB]]; n > 0; n = gViewWalls[n].next) {
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
                    DrawRegion000F(2, gRectKinds[spr], x, y, 5);
                else
                    DrawRegion004D(2, gRectKinds[spr], x, y, 5);
            }
        }
    }
    if (gWallFaces[sideA]) {
        offA = *(unsigned char far *)gWallFaces[sideA];
        for (n = gTable18BF[gEF6A[slot][offA]]; n > 0; n = gViewWalls[n].next) {
            n--;
            flags = gViewWalls[n].flags;
            delta = gWallOffsets[sideA];
            if ((flags & 1) || ((flags & 2) && gF4BC))
                delta = -delta;
            seg = (delta < 0 ? -delta : delta) - 1;
            spr = gViewWalls[n].cells[seg];
            if (spr != 0xff) {
                if (delta >= 0) {
                    if (flags & 4)
                        x = gViewWalls[n].x[seg] + g1D61[(slot * 5 + 4) * 2];
                    else
                        x = gViewWalls[n].x[seg] + gWallXBias[slot];
                } else {
                    if (flags & 4)
                        x = 0xb0 - gViewWalls[n].x[seg] + g1D61[(slot * 5 + 4) * 2];
                    else
                        x = 0xb0 - gViewWalls[n].x[seg] + gWallXBias[slot];
                    x -= gRectKinds[spr]->width << 3;
                }
                y = gViewWalls[n].y[seg];
                if (delta >= 0)
                    DrawRegion000F(2, gRectKinds[spr], x, y, 5);
                else
                    DrawRegion004D(2, gRectKinds[spr], x, y, 5);
            }
        }
    }
}
/* The 3-byte size record a gFB5E entry points at (src/E122E.C declares the same layout). */
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
extern struct SlotSize far *gFB5E[]; /*@ ds:FB5E */  /* per column, a far size record (src/EOV3.C) */
extern struct SlotPos gA99C[]; /*@ ds:A99C */
extern int gF1; /*@ ds:00F1 */               /* a region top (src/EOV5.C) */
extern int gF5; /*@ ds:00F5 */               /* a region height (this unit) */
extern signed char g716E[]; /*@ ds:716E */   /* per heading, a column base (this unit) */
extern signed char g70F0[]; /*@ ds:70F0 */   /* per index y base, action 1..3 (this unit) */
extern signed char g70F4[]; /*@ ds:70F4 */   /* per index y base, action 4..6 (this unit) */
extern signed char g70F8[]; /*@ ds:70F8 */   /* per index y base, action 7..9 (this unit) */
extern signed char g70FC[]; /*@ ds:70FC */   /* per index y base, action 10..11 (this unit) */
extern signed char g70FF[]; /*@ ds:70FF */   /* per index y base, action 10..11, second draw (this unit) */
extern signed char g7102[]; /*@ ds:7102 */   /* per index y base, action 12 (this unit) */
extern int g7106[]; /*@ ds:7106 */           /* per slot x, action 4..6 (this unit) */
extern int g7108[]; /*@ ds:7108 */           /* per slot y, action 4..6 (this unit) */
extern signed char g714E[]; /*@ ds:714E */   /* per heading nudge base (this unit) */
extern signed char g718E[]; /*@ ds:718E */   /* per index nudge a (this unit) */
extern signed char g7192[]; /*@ ds:7192 */   /* per index nudge b (this unit) */
extern signed char g7196[]; /*@ ds:7196 */   /* per index nudge c (this unit) */
extern signed char g719A[]; /*@ ds:719A */   /* per index nudge d (this unit) */
extern signed char g719E[]; /*@ ds:719E */   /* per index nudge e (this unit) */
extern signed char g71A2[]; /*@ ds:71A2 */   /* per index nudge f (this unit) */

#define NO_RECORD -1        /* a ds:18BF byte that means "this heading has no size record" */

/* ovl4:1411: draws the wall face(s) one view slot shows for the current action (ds:FBC8). It selects the
   slot's size record gFB5E[col] (col = g716E[heading] + 2 - g1A09[slot]) and its x base
   g1D61[(slot*5+4)*2]; unless the action is 7..9 it first fits region 5 to the column extent. For
   actions 1..3 and 12 it draws one region at the record with a y base from ds:70F0/ds:7102 nudged by
   ds:718E/ds:7192 when the heading is below 0x1e; 4..6 draws the record twice (region 000F and
   004D, mirrored) at a y base ds:70F4 and fits the region to ds:7106/ds:7108, then redraws region 5
   at the column extent; 7..9 draws gFB5E[col+3] first, then fits the column extent and draws the
   record at y base ds:70F8; 10..11 draws gFB5E[col] and gFB5E[col+3] at y bases ds:70FC/ds:70FF
   with a nudge from ds:719E/ds:71A2. Every case, when ds:18BF of the heading is NO_RECORD (0xff),
   also draws the ds:A99C[col] position record, which needs its own x and y. Finally SetRegionRect
   fits region 5 to the visible span. Called by DrawViewSlots at ovl4:1E2E when the slot is below 15
   and the record's ds:194B bit 3 is set. Purpose unknown: what the actions, the size records and the
   nudges mean, and what the 0x1e heading threshold selects.
   QUIRK ovl4:1411: in cases 10 and 11 the slot parameter is dead and the compiler reuses its stack
   slot for the case's y nudge, so `slot` is reassigned there and no longer holds the slot number. */
/*@ ovl4:1411 */ void Ovl4_1411(int slot)
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

    idx = g1A09[slot];
    heading = gEF6A[slot][gA9C6];
    col = g716E[heading] + 2 - idx;
    rec = gFB5E[col];
    xBase = g1D61[(slot * 5 + 4) * 2];
    x = xBase + 0x58;
    if (gFBC8 < 7 || gFBC8 > 9)
        SetRegionColumnExtent(slot, &top, &bottom, 5);
    switch (gFBC8) {
    case 1:
    case 2:
    case 3:
    case 12:
        if (gFBC8 != 0xc)
            y = g70F0[idx] - rec->height;
        else
            y = g7102[idx] - rec->height;
        x -= (rec->width << 3) >> 1;
        if (gEF6A[slot][gA9C6] < 0x1e)
            y -= (heading - g714E[heading]) * g718E[idx];
        else
            y -= g7192[g1A09[slot]];
        DrawRegion000F(2, rec, x, y, 5);
        if (gTable18BF[heading] == NO_RECORD) {
            x = gA99C[col].x + xBase;
            y = gA99C[col].y;
            DrawRegion000F(2, gA99C[col].rec, x, y, 5);
        }
        break;
    case 4:
    case 5:
    case 6:
        y = g70F4[idx] - rec->height;
        left = g7106[slot * 2] >> 3;
        right = g7108[slot * 2] >> 3;
        if (gViewColLeft > left)
            left = gViewColLeft;
        if (gViewColRight < right)
            right = gViewColRight;
        SetRegionExtents(5, left, gF1, right - left, gF5);
        if (gEF6A[slot][gA9C6] < 0x1e)
            shift = -((heading - g714E[heading]) * g7196[idx]);
        else
            shift = -g719A[g1A09[slot]];
        shift -= rec->width << 3;
        DrawRegion000F(2, rec, x + shift, y, 5);
        shift += rec->width << 3;
        DrawRegion004D(2, rec, x - shift, y, 5);
        if (gTable18BF[heading] == NO_RECORD) {
            x = gA99C[col].x + xBase - shift;
            y = gA99C[col].y;
            DrawRegion000F(2, gA99C[col].rec, x, y, 5);
        }
        break;
    case 7:
    case 8:
    case 9:
        y = g70F8[idx];
        backX = x - ((gFB5E[col + 3]->width << 3) >> 1);
        x -= (rec->width << 3) >> 1;
        y -= gFB5E[col + 3]->height;
        DrawRegion000F(2, gFB5E[col + 3], backX, y, 5);
        SetRegionColumnExtent(slot, &top, &bottom, 5);
        y = g70F8[idx];
        if (gEF6A[slot][gA9C6] < 0x1e)
            y -= (heading - g714E[heading]) * g718E[idx];
        else
            y -= g7192[g1A09[slot]];
        DrawRegion000F(2, rec, x, y, 5);
        if (gTable18BF[heading] == NO_RECORD) {
            x = gA99C[col].x + xBase;
            y = gA99C[col].y;
            DrawRegion000F(2, gA99C[col].rec, x, y, 5);
        }
        break;
    case 10:
    case 11:
        if (gEF6A[slot][gA9C6] < 0x1e)
            slot = -((heading - g714E[heading]) * g719E[idx]);
        else
            slot = -g71A2[g1A09[slot]];
        x -= (rec->width << 3) >> 1;
        y = g70FC[idx];
        DrawRegion000F(2, gFB5E[col], x, y + slot, 5);
        slot = (slot >> 2) + (slot >> 3);
        y = g70FF[idx];
        DrawRegion000F(2, gFB5E[col + 3], x, y - slot, 5);
        if (gTable18BF[heading] == NO_RECORD) {
            x = gA99C[col].x + xBase;
            y = gA99C[col].y;
            DrawRegion000F(2, gA99C[col].rec, x, y, 5);
        }
        break;
    }
    SetRegionRect(5, gViewColLeft, 0, gViewColRight, 0xf);
}
/* The 18-byte records at ds:DD2C (src/E1414.C declares the same layout). */
struct DDRec {
    char active;              /* +00: the record state, 1 selects the ds:F2F8 path here */
    char pad01;
    int m02;                  /* +02 */
    int m04;                  /* +04: the slot heading the record belongs to */
    char m06;                 /* +06: low two bits index ds:7246 and ds:7262 */
    signed char m07;          /* +07: the page the record is seen from */
    int m08;                  /* +08: indexes the ds:8DD0 item records */
    char m0A;                 /* +0A: row in the ds:F2F8 cell table */
    char m0B;                 /* +0B */
    char m0C;                 /* +0C: bit 6 gates a position recompute */
    unsigned char m0D;        /* +0D */
    int (far *fn)(struct DDRec far *rec); /* +0E */
};
extern struct DDRec gDD2C[]; /*@ ds:DD2C */
extern signed char g7246[]; /*@ ds:7246 */   /* per page and low-2-bit index, the list slot (this unit) */
extern unsigned char g71A6[]; /*@ ds:71A6 */ /* per slot, per sprite, the x base (this unit) */
extern signed char g71EE[]; /*@ ds:71EE */   /* per g1A09 slot, per sprite, the y base (this unit) */
extern signed char g71FE[]; /*@ ds:71FE */   /* per slot, per sprite, the frame (this unit) */
extern signed char g725A[]; /*@ ds:725A */   /* per frame, the cell +2 value a nudge needs (this unit) */
extern signed char g725E[]; /*@ ds:725E */   /* per frame, the x nudge added on that match (this unit) */
extern signed char g7262[]; /*@ ds:7262 */   /* per record +7, per record +6, the draw wrapper (this unit) */
extern signed char g1D07[]; /*@ ds:1D07 */   /* per kind plus 0x2d side, a ds:F2F8 row or -1 (this unit) */
extern void far *gThrownIcons[][4]; /*@ ds:F2F8 */  /* far sprite cells, four per icon row (this unit) */

#define DD_SIDE_STRIDE 0x2d
#define NO_DD_ROW -1

/* ovl4:19C4: draws the map objects whose ds:DD2C record is seen from the current page. Only when
   g1879[tile] is nonzero and g194B[tile] bit 7 is clear does it run (tile is the slot's gEF6A
   record byte, as in Ovl4_0D1B). It scans the ten 0x12-byte ds:DD2C records and, for each whose
   byte +0 is set and word +4 equals the slot's heading (gAngleA9C8[slot]), puts its far pointer in
   the list slot g7246[gFBCC * 5 + (record +6 & 3)]. Each non-empty slot picks a cell from the
   ds:F2F8 or ds:EC9A far sprite tables and a draw wrapper from ds:7262, at an x from ds:71A6 and a
   y from ds:71EE selected by the sprite base ds:1A2B[gFBCC * 4 + (+6 & 3)] and the frame ds:71FE:
   a record whose byte +0 is 1 picks the ds:F2F8 row ds:1D07[kind + side * 0x2d] (kind is the
   ds:8DD0 item icon through ds:181F, side -1 when the record +7 matches neither the page nor the
   page xor 2) and shifts x by ds:725E[frame] when the cell +2 modulo 8 matches ds:725A[frame];
   any other record uses the row byte +0xA directly and, when its +0xC has bit 6, recomputes x from
   ds:1D61 and y from the cell. A wrapper of 0 draws with DrawRegion000F, any other with
   DrawRegion004D. Called by DrawViewSlots for every slot. Purpose unknown: what the records, the
   sides and the wrappers mean. Three source forms TCC pins down: the ds:194B bit 7 test is written
   `g1879[tile] == 0 || (g194B[tile] & 0x80)` with an empty then, the only form that tests the byte
   without sign-extending it; the frame guard is `if (frame >= 0) { body }` (an `if (frame < 0)
   continue;` gives the same bytes but reloads ES before the record check, a -Z difference); and the
   cell +2 test is a raw `<< 3 >> 3` (a `% 8` compiles to idiv). */
/*@ ovl4:19C4 */ void Ovl4_19C4(int slot)
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
    struct DDRec far *rec;
    char far *cell;
    struct DDRec far *arr[5];
    int i;
    int frame;

    heading = gAngleA9C8[slot];
    tile = gB894[heading * 9 + gA9C6];
    if (g1879[tile] == 0 || (g194B[tile] & 0x80))
        ;
    else
        return;
    page = gFBCC * 5;
    for (i = 0; i < 5; i++)
        arr[i] = 0;
    rec = gDD2C;
    for (i = 0; i < 0xa; i++, rec++) {
        if (rec->active != 0 && rec->m04 == heading)
            arr[g7246[page + (rec->m06 & 3)]] = rec;
    }
    for (page = 0; page < 5; page++) {
        if (arr[page] == 0)
            continue;
        rec = arr[page];
        spriteBase = g1A2B[gFBCC * 4 + (rec->m06 & 3)];
        x = g71A6[slot * 4 + spriteBase];
        y = g71EE[g1A09[slot] * 4 + spriteBase];
        frame = g71FE[slot * 4 + spriteBase];
        wrapper = 0;
        if (frame >= 0) {
            if (rec->active == 1) {
                kind = g181F[gItems[rec->m08].icon];
                if (rec->m07 == gFBCC)
                    side = 0;
                else if (rec->m07 == (gFBCC ^ 2))
                    side = 1;
                else
                    side = -1;
                if (side != -1 && g1D07[kind + side * DD_SIDE_STRIDE] != NO_DD_ROW) {
                    cell = (char far *)gThrownIcons[g1D07[side * DD_SIDE_STRIDE + kind]][frame];
                    wrapper = g7262[rec->m07 * 4 + rec->m06];
                } else {
                    cell = (char far *)gItemL1Icons[kind][frame];
                    wrapper = (rec->m07 == ((gFBCC + 1) & 3)) ? 1 : 0;
                }
                if ((((signed char far *)cell)[2] << 3 >> 3) == g725A[frame])
                    x += g725E[frame];
            } else {
                cell = (char far *)gThrownIcons[rec->m0A][frame];
                wrapper = g7262[rec->m07 * 4 + rec->m06];
                if (rec->m0C & 0x40) {
                    x = g1D61[(slot * 5 + 4) * 2] + 0x58;
                    x -= (((signed char far *)cell)[2] << 3) >> 1;
                    y = 0x2c - (((signed char far *)cell)[1] >> 1);
                }
            }
            if (wrapper == 0)
                DrawRegion000F(2, cell, x, y, 5);
            else
                DrawRegion004D(2, cell, x, y, 5);
        }
    }
}

extern signed char g7272[]; /*@ ds:7272 */   /* per-column x base correction (this unit) */
extern signed char g7275[]; /*@ ds:7275 */   /* per-column y base (this unit) */
extern void far *gIntroCells[]; /*@ ds:F4BE */  /* sprite cell table (src/EOV2.C) */
extern int gFB78; /*@ ds:FB78 */            /* toggled by 1414:04B0 (src/E1414.C) */
extern signed char far gCellBytes[]; /*@ 20AB:0000 */  /* the cell bytes drawn here */

#define CELL_ROW_BYTES 0x1a   /* one sprite row: CELL_COLS cells of two bytes (x then y) */
#define CELL_COLS 13

/* ovl4:1D20: draws the "0x34" object the view shows for one view slot: two columns of 13 sprite
   cells through DrawRegion000F(mode 2, cell, x, y, region 5). The column pair is ds:1A09[slot]
   counted back from 2 (a slot with a larger ds:1A09 draws fewer). Each column i uses the far
   sprite cell ds:F4BE[(col * 2 + i) ^ ds:FB78] and reads its cells from the far bytes at
   20AB:0000, row (col*2+i) * CELL_ROW_BYTES, two bytes a cell: byte 0 shifts x and byte 1 shifts y,
   on top of the per-column base ds:19E5[slot] - ds:7272[col] (x) and ds:7275[col] (y). Column 0 is
   nudged by -4 on both axes. Called by Ovl4_1E18 at ovl4:1E2A when the slot's heading is 0x34.
   Purpose unknown: what the object is and what the 20AB:0000 bytes mean. */
/*@ ovl4:1D20 */ void Ovl4_1D20(int slot)
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

    col = 2 - g1A09[slot];
    if (col < 0)
        return;
    xbase = g19E5[slot] - g7272[col];
    ybase = g7275[col];
    for (i = 0; i < 2; i++) {
        idx = col * 2 + i;
        xoff = yoff = 0;
        if (idx == 0) {
            xoff = -4;
            yoff = -4;
        }
        cells = gIntroCells[idx ^ gFB78];
        for (j = 0; j < CELL_COLS; j++) {
            DrawRegion000F(2, cells,
                           xoff + xbase + gCellBytes[idx * CELL_ROW_BYTES + j * 2],
                           yoff + ybase + gCellBytes[idx * CELL_ROW_BYTES + j * 2 + 1],
                           5);
        }
    }
}

extern signed char g7278[]; /*@ ds:7278 */     /* per-view-slot byte (this unit) */
extern signed char gA9C6; /*@ ds:A9C6 */       /* offset into the gEF6A target (src/E0ACD.C) */

/* ovl4:1E18: walks the view slots from `start` to 17. For each, ds:7278 gives the slot number,
   Ovl4_0097 computes the visible column span into ds:FE3E/ds:FE40 and the slot is drawn only if
   the span is not empty: Ovl4_1091 draws the slot, Ovl4_0AD0 draws the sprite at word +5 of the
   slot's ds:EF6A tile record, and for slots below 15 Ovl4_1411 (when ds:194B of the record's
   heading byte has bit 3) and Ovl4_0D1B (when byte +4 of the record has any of bits 0..2).
   Ovl4_19C4 always runs, and Ovl4_1D20 only for the heading value 0x34. Called by Ovl4_1F51 at
   ovl4:1F75. Purpose unknown: what the slot numbers and the 0x34 heading mean. */
/*@ ovl4:1E18 */ void DrawViewSlots(int start)
{
    int i;
    int slot;
    int heading;

    for (i = start; i < 18; i++) {
        slot = g7278[i];
        ComputeViewSpan(slot, &gViewColLeft, &gViewColRight, 5);
        if (gViewColRight < gViewColLeft)
            continue;
        Ovl4_1091(slot);
        heading = gEF6A[slot][gA9C6];
        if (*(int far *)(gEF6A[slot] + 5) != 0)
            Ovl4_0AD0(slot, *(int far *)(gEF6A[slot] + 5));
        if (slot < 0xf) {
            if (g194B[heading] & 8)
                Ovl4_1411(slot);
            if (*(gEF6A[slot] + 4) & 7)
                Ovl4_0D1B(slot);
        }
        Ovl4_19C4(slot);
        if (heading == 0x34)
            Ovl4_1D20(slot);
    }
}

extern int g7296; /*@ ds:7296 */              /* the page the rows were last drawn for (this unit) */
extern int g7290[]; /*@ ds:7290 */             /* per-row y (this unit) */
extern int g728A[]; /*@ ds:728A */             /* per-row x (this unit) */
extern void far *gFarA92A[]; /*@ ds:A92A */    /* per-page, per-row far image pointers (this unit) */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */

/* ovl4:1EDE: redraws the three view rows for the current page. It returns early when ds:7296
   already holds gFBCC; otherwise it hides the pointer over the view box, draws the three rows
   through f1A29_000F(0, ds:A92A[gFBCC + 4 * i], ds:728A[i], ds:7290[i], 0), shows the pointer and
   remembers gFBCC in ds:7296. Called by Ovl4_1F51 at ovl4:202F. Purpose unknown: what the rows
   are. */
/*@ ovl4:1EDE */ void DrawViewRows(void)
{
    int i;

    if (g7296 == gFBCC)
        return;
    HidePointerZone(0x46, 0x79, 0xb0, 0xaf);
    for (i = 0; i < 3; i++)
        f1A29_000F(0, gFarA92A[gFBCC + 4 * i], g728A[i], g7290[i], 0);
    ShowPointerZone();
    g7296 = gFBCC;
}

extern char gFB45; /*@ ds:FB45 */           /* request flag, cleared here (src/E07A9.C) */
extern char gF442; /*@ ds:F442 */           /* gate checked here (src/E08E6.C) */
extern char gFlag; /*@ ds:F3B9 */           /* selects a table set (src/E0ACD.C) */
extern char gDF4C; /*@ ds:DF4C */           /* a notify flag, cleared here (src/E08E6.C) */
extern unsigned long gCount849A; /*@ ds:849A */  /* the frame/effect busy counter (src/E08E6.C) */
extern void far *gSegDCA6; /*@ ds:DCA6 */   /* the far pointer DrawTile draws (src/E07A9.C) */
extern unsigned char far *gFE68; /*@ ds:FE68 */  /* the loaded palette buffer (src/E07C8.C) */
extern void (far *gFn80BA)(void far *table); /*@ ds:80BA */  /* palette setter (src/E07C8.C) */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void DrawTile(void far *src); /*@ 17A6:03E7 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */

/* ovl4:1F51: draws one frame of the view: Ovl4_044B draws the objects into the page buffer (with
   gPartyCell and gFBCC), DrawTile blits it from gSegDCA6, DrawViewSlots(0) draws the slots, and
   when gFB45 and video mode 3 are set the palette gFE68 is loaded and gFB45 cleared. Then it waits
   for gCount849A to reach zero, and, when `full` is set and gF442 is clear, blanks the view box and
   clips a rectangle. It always plays sound 0 and, when gFlag is zero, runs Ovl4_1EDE; finally it
   clears gDF4C. Called from the game loop. */
/*@ ovl4:1F51 */ void DrawViewFrame(int full)
{
    Ovl4_044B(gPartyCell, gFBCC);
    DrawTile(gSegDCA6);
    DrawViewSlots(0);
    if (gFB45 && gGraphicsChoice == 3) {
        if (full) {
            HidePointerZone(0, 0, 0xb0, 0x78);
            gFillRect(0, 0, 0xb0, 0x78, 0xc);
            ShowPointerZone();
        }
        gFn80BA(gFE68);
        gFB45 = 0;
    }
    while (gCount849A != 0)
        ;
    if (full && !gF442) {
        HidePointerZone(0, 0, 0xb0, 0x78);
        ClipRect(0, 0, 0, 0, 0x16, 0x78, 2, 0);
        ShowPointerZone();
    }
    PlaySoundDefault(0);
    if (!gFlag)
        DrawViewRows();
    gDF4C = 0;
}
/* ovl4:2039: discards its work: si = 0 then si++ and return. Purpose unknown. */
/*@ ovl4:2039 */ void NoOp2039(void)
{
    int si;

    si = 0;
    si++;
}