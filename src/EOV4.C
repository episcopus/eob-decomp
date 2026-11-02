/* Unit at overlay 4 of EOB.EXE (8259 bytes, 18 functions): the first-person view renderer. It
   draws the map objects, walls and creature sprites of the current page into the view region from
   the object records at ds:8A82 and the 9-byte tile records at ds:B894, clipped through the
   drawing wrappers in src/E082C.C. The unit is still being lifted. */

/*@flags -ml -O -Z */

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
   DRAFT: the body matches but the original keeps `2 * slot` in SI for the first test and spills a
   copy to a third local, where our build keeps the loop counter in SI; tried `register` and
   declaration orders. */
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

/* ovl4:036A: flips bit 0x4000 of a rectangle of word pixels in the view buffer at ds:ADB0, reading
   from the page buffer at ds:F43E. `src` starts at (g1879[page] - 1) * 0x35E + x2 * 2 + 0x294;
   for each of the `height` rows and `width` columns it reads the word at
   src[width * row + (width - 1 - col)], skips it when zero and stores it to
   ds:ADB0[(y + row) * 0x16 + x + col] with bit 0x4000 toggled, skipping columns outside 0..0x15.
   Called by Ovl4_044B at ovl4:04B4 and ovl4:04CD. Purpose unknown: what the bit marks.
   DRAFT: the body matches except the outer loop counter: the original holds it in CX, ours in DI/SI
   (tried declaration orders and `register`). */
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

extern int gFBCC; /*@ ds:FBCC */            /* the current page 0..3 (src/E08E6.C) */
extern int gSoundVolume; /*@ ds:FBCA */     /* the global sound volume (src/E08E6.C) */
extern char gFB45; /*@ ds:FB45 */           /* request flag, cleared here (src/E07A9.C) */
extern int g8102; /*@ ds:8102 */            /* the video mode/format (src/E07A9.C) */
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
   gSoundVolume and gFBCC), DrawTile blits it from gSegDCA6, DrawViewSlots(0) draws the slots, and
   when gFB45 and video mode 3 are set the palette gFE68 is loaded and gFB45 cleared. Then it waits
   for gCount849A to reach zero, and, when `full` is set and gF442 is clear, blanks the view box and
   clips a rectangle. It always plays sound 0 and, when gFlag is zero, runs Ovl4_1EDE; finally it
   clears gDF4C. Called from the game loop. */
/*@ ovl4:1F51 */ void DrawViewFrame(int full)
{
    Ovl4_044B(gSoundVolume, gFBCC);
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