/* Unit at overlay 4 of EOB.EXE (8259 bytes, 18 functions): the first-person view renderer. It
   draws the map objects, walls and creature sprites of the current page into the view region from
   the object records at ds:8A82 and the 9-byte tile records at ds:B894, clipped through the
   drawing wrappers in src/E082C.C. The unit is still being lifted. */

/*@flags -ml -O -Z */

extern signed char gHead199D[]; /*@ ds:199D */  /* rows of 18 signed heading offsets */
extern unsigned char gB894[]; /*@ ds:B894 */    /* 9-byte tile records (src/E08E6.C) */
extern signed char far *gEF6A[]; /*@ ds:EF6A */ /* 18 far pointers into gB894 (src/E1414.C) */
extern int gAngleA9C8[]; /*@ ds:A9C8 */         /* 18 heading words, the masked sum (this unit) */
extern int gViewDCD6[]; /*@ ds:DCD6 */          /* 18 view slots, reset to -1 (this unit) */
extern int gViewDCB2[]; /*@ ds:DCB2 */          /* 18 view slots, reset to -1 (this unit) */

/* ovl4:0000: for each of the 18 view slots, compute a heading word from `base` plus the slot's
   signed offset in the 18-byte row `row` of ds:199D, masked to 10 bits, store it in the heading
   table at ds:A9C8, point the slot's ds:EF6A entry at the 9-byte tile record for that heading in
   ds:B894, and reset the slot to -1 in the two view-slot arrays at ds:DCD6 and ds:DCB2. Called by
   Ovl4_044B at ovl4:04D3. Purpose unknown: the meaning of `row` and of the 18 slots. DRAFT: the
   shape matches but the register allocator puts `off` in DI where the original uses CX (tried
   `off = row * 18` inline, `off = row; off *= 18`, `register`, and declaration orders). */
/*@ ovl4:0000 draft */ void InitViewSlots(int base, int row)
{
    int heading;
    int si;
    int off = row * 18;

    for (si = 0; si < 18; si++) {
        gAngleA9C8[si] = heading = (base + gHead199D[si + off]) & 0x3FF;
        gEF6A[si] = (signed char far *)&gB894[heading * 9];
        gViewDCD6[si] = -1;
        gViewDCB2[si] = -1;
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

/*@ ovl4:0097 stub */ void Ovl4_0097(int slot, int far *left, int far *right, int region) {}

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
   DRAFT: the body matches but our build merges the two `continue` increment blocks into one where
   the original keeps three duplicated tails; tried `if/else`, `continue` order and pointer order. */
/*@ ovl4:02A5 draft */ void CopyViewColumn(char x, char y, char page, int x2, char width, char height)
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
            if (x + col < 0 || x + col >= 0x16) {
                dst++;
                src++;
                continue;
            }
            if (*src == 0) {
                src++;
                dst++;
                continue;
            }
            *dst = *src;
            src++;
            dst++;
        }
    }
}

/*@ ovl4:036A stub */ void Ovl4_036A(void) {}

extern signed char g194B[]; /*@ ds:194B */  /* per-slot byte flags (src/E08E6.C) */

/* ovl4:042C: returns 1 when `slot` is nonzero and the bit 3 of its ds:194B flag byte is clear
   (the slot is a live map object), else 0. Called by Ovl4_044B at ovl4:0563 and ovl4:05D8.
   DRAFT: the original loads the ds:194B byte and sign-extends it (cwde) before the bit-3 test;
   our build folds it to a byte test. Tried `&&`, `!`, a cast to int and a ternary. */
/*@ ovl4:042C draft */ int IsSlotLive(int slot)
{
    return slot != 0 && (g194B[slot] & 8) == 0;
}

/*@ ovl4:044B stub */ void Ovl4_044B(void) {}
/*@ ovl4:0AD0 stub */ void Ovl4_0AD0(int slot, int field) {}
/*@ ovl4:0D1B stub */ void Ovl4_0D1B(int slot) {}
/*@ ovl4:1091 stub */ void Ovl4_1091(int slot) {}
/*@ ovl4:1411 stub */ void Ovl4_1411(int slot) {}
/*@ ovl4:19C4 stub */ void Ovl4_19C4(int slot) {}
/*@ ovl4:1D20 stub */ void Ovl4_1D20(int slot) {}

extern signed char g7278[]; /*@ ds:7278 */     /* per-view-slot byte (this unit) */
extern signed char gA9C6; /*@ ds:A9C6 */       /* offset into the gEF6A target (src/E0ACD.C) */
extern int gViewColLeft; /*@ ds:FE3E */        /* the visible left column (this unit) */
extern int gViewColRight; /*@ ds:FE40 */       /* the visible right column (this unit) */

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
        Ovl4_0097(slot, &gViewColLeft, &gViewColRight, 5);
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

/*@ ovl4:1EDE stub */ void Ovl4_1EDE(void) {}
/*@ ovl4:1F51 stub */ void Ovl4_1F51(void) {}
/* ovl4:2039: discards its work: si = 0 then si++ and return. Purpose unknown. */
/*@ ovl4:2039 */ void NoOp2039(void)
{
    int si;

    si = 0;
    si++;
}