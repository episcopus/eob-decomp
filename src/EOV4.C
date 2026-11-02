/* Unit at overlay 4 of EOB.EXE (8259 bytes, 18 functions): the first-person view renderer. It
   draws the map objects, walls and creature sprites of the current page into the view region from
   the object records at ds:8A82 and the 9-byte tile records at ds:B894, clipped through the
   drawing wrappers in src/E082C.C. The unit is still being lifted. */

/*@flags -ml -O -Z -B */

extern signed char gHead199D[]; /*@ ds:199D */  /* rows of 18 signed heading offsets */
extern unsigned char gB894[]; /*@ ds:B894 */    /* 9-byte tile records (src/E08E6.C) */
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

/* ovl4:0097: computes the visible left and right columns of one view slot. When the slot's pair in
   gViewSlots (at ds:DCB2) is not -1 it just returns that pair. Otherwise it starts from 0..0x16 and
   walks the 18 tile records (gEF6A[i][gA9C6]): a record whose ds:194B flag has bit 3 clear
   contributes its ds:6B82 column adjustment (a positive value pulls *right left, a negative one
   pulls *left right, and -40 is skipped while -41 forces an empty left), a record whose bit 3 is
   set contributes its ds:6CC6 {left,right} bounds; the walk stops when *right falls below *left.
   The pair is cached back into gViewSlots and SetRegionRect fits region `region` to the span.
   Called by Ovl4_1E18 at ovl4:1E23. Purpose unknown: what the records and adjustments mean.
   DRAFT: the register allocation differs. The original has sub sp,6 (bp-2 i, bp-4 pair, bp-6
   heading), keeps 2*slot in SI across the `if` (the shared index of the condition, the two loads
   and the final stores) and adjust in DI; ours promotes `slot` to DI (the index recomputed per use)
   or the loop counter to SI, giving sub sp,4. A local `j = (slot + i) * 2` is needed so the else's
   index takes SI; with it every local but `adjust` is in memory. `register int pair` then reproduces
   the whole if/then and the entry (sub sp,6, pair in SI), but the compiler drops the else's
   redundant `pair = 2 * slot`, so the end label (and the then tail `jmp`) lands one byte early
   (+0037); recomputing pair in the else adds 7 bytes instead. Tried all declaration orders of
   {i,pair,heading,adjust,j} with every `register` subset, `pair` vs inline `2 * slot` in the
   condition, `slot *= 18` vs `slot = slot * 18`, a `while` loop, -B, -G and -r-; none changes it. */
/*@ ovl4:0097 draft */ void ComputeViewSpan(int slot, int far *left, int far *right, int region)
{
    int pair;
    int i;
    int heading;
    int adjust;

    pair = 2 * slot;
    if (gViewSlots[pair] != -1) {
        *left = gViewSlots[pair];
        *right = gViewSlots[pair + 1];
    } else {
        *left = 0;
        *right = 0x16;
        slot = slot * 18;
        for (i = 0; i < 18; i++) {
            heading = gEF6A[i][gA9C6];
            if ((g194B[heading] & 8) == 0) {
                adjust = g6B82[slot + i];
                if (g1879[heading] == 0)
                    continue;
                if (adjust == -40)
                    continue;
                if (adjust == -41) {
                    *left = 0x16;
                    *right = 0;
                    break;
                }
                if (adjust > 0 && *right > adjust)
                    *right = adjust;
                if (adjust < 0 && -adjust > *left)
                    *left = -adjust;
            } else {
                if (g6CC6[(slot + i) * 2] > *left)
                    *left = g6CC6[(slot + i) * 2];
                if (g6CC6[(slot + i) * 2 + 1] < *right)
                    *right = g6CC6[(slot + i) * 2 + 1];
            }
            if (*right < *left)
                break;
        }
        gViewSlots[pair] = *left;
        gViewSlots[pair + 1] = *right;
    }
    SetRegionRect(region, *left, 0, *right, 0xf);
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
extern int gFBC8; /*@ ds:FBC8 */                 /* the current page 0..3 (src/E0ACD.C) */
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
   The three duplicated increment tails are kept apart only through the assembler (-B), as the
   original shows (TCC -S then TASM; without it our build merges the continue tails). */
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
   from the page buffer at ds:F43E. `src` starts at (g1879[page] - 1) * 0x35E + x2 * 2 + 0x294;
   for each of the `height` rows and `width` columns it reads the word at
   src[width * row + (width - 1 - col)], skips it when zero and stores it to
   ds:ADB0[(y + row) * 0x16 + x + col] with bit 0x4000 toggled, skipping columns outside 0..0x15.
   Called by Ovl4_044B at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the bit marks.
   DRAFT: the body and the pre-loop setup match; only the loop registers differ. The original keeps
   the outer counter (row) in CX, the inner (col) in DI and the pixel in SI; ours puts row in DI, col
   in SI and the pixel in AX (so it pushes width*row instead of using CX). Tried all 6 declaration
   orders of {row,col,pixel} with every `register` subset, int and unsigned types, while/do-while
   loops, -B, -G and -r-; the allocation does not move. The difference is TCC's choice of which of
   the three live values takes SI/DI, not a source form found yet. */
/*@ ovl4:036A draft */ void ToggleViewColumn(char x, char y, char page, int x2, char width, char height)
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
            pixel = src[width * row + (width - 1 - col)];
            if (pixel == 0)
                continue;
            if (pixel & 0x4000)
                pixel -= 0x4000;
            else
                pixel |= 0x4000;
            gViewBufADB0[(y + row) * 0x16 + x + col] = pixel;
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

/*@ ovl4:044B stub */ void Ovl4_044B(int arg0, int arg1) {}

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
extern int g8102; /*@ ds:8102 */             /* the video mode/format (src/E07A9.C) */
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
   DRAFT: the prologue, frame (sub sp,0x36) and body match through the object scan; the first
   difference is the far-pointer store into the local `arr` (0xD1B+0079: ours `93 xchg bx,ax`,
   original `87 D8 xchg ax,bx`). TCC -S emits `xchg bx,ax`, and TASM 2.0, 2.01, 1.0 and 1.01 all
   assemble both spellings as the one-byte 93, so no source form gives the original's two-byte
   87 D8 under this file's -B flag; without -B (TCC's own assembler) the byte is 87 D8, but then
   CopyViewColumn's three separate continue tails merge. That per-function assembler split (0D1B
   and 19C4 want TCC's assembler, CopyViewColumn wants TASM) is a matcher question, not a source
   form (playbook 30). */
/*@ ovl4:0D1B draft */ void Ovl4_0D1B(int slot)
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
            if (obj->m0A == 0xfe)
                sprite = 5;
            else if (obj->m0A == 0xff)
                sprite = 6;
        }
        spriteAbs = abs(sprite);
        nudge = obj->m07 ? 0x12 : 0;
        cell = gB7FE[spriteAbs + nudge];
        for (n = tileRow; n != 0; n--) {
            if (g8102 == 2)
                BlitSpriteEga(cell, gBufDCFC);
            else if (g8102 == 0)
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
        info = &gTable1A3B[obj->b0];
        if (info->m15 == 1 && (spriteAbs == 1 || spriteAbs > 3))
            y += g7015[tileRow];
        x -= (((signed char far *)cell)[2] << 3) >> 1;
        y -= ((signed char far *)cell)[1];
        x += (obj->m17 & 0xf0) >> 4;
        y += obj->m17 & 0xf;
        if (obj->m16 & 2) {
            if (g8102 == 0)
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
/*@ ovl4:1091 stub */ void Ovl4_1091(int slot) {}
/*@ ovl4:1411 stub */ void Ovl4_1411(int slot) {}
/*@ ovl4:19C4 stub */ void Ovl4_19C4(int slot) {}

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
    if (gFB45 && g8102 == 3) {
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