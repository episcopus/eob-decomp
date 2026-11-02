/* Unit at overlay 0 of EOB.EXE (10873 bytes, 35 functions): the game's character panel and its
   drawing helpers. It builds the 17-cell icon table (ovl0:0000), draws a character's panel and
   equipment (ovl0:065B, 135D), the two option-list screens (ovl0:207A, 2238), the item/text field
   editor (ovl0:23EC) and the status/rest screens, and loads the overlay's screens (ovl0:2547,
   28F9). Placeholder names where no evidence. */

/*@flags -ml -O -Z */

extern char gOverlayBuf[]; /*@ ds:113F */ /* the overlay screen buffer the cell slicer reads */
extern void far * (far *gFn8082)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void farfree(void far *block); /*@ 0000:71EE */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern int DispatchKeyWait(void far *list); /*@ 16A5:005A */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern unsigned char far g3A0; /*@ 0000:03A0 */ /* a flag in the resident segment (Purpose unknown) */

struct IconDef9 {               /* the 17 9-byte icon definitions at 2084 (17 * 9 = 153 bytes) */
    unsigned char x;            /* +0: the cell's left edge */
    unsigned char y;            /* +1: the cell's top edge */
    unsigned char w;            /* +2: the cell's width, also the "defined" flag */
    unsigned char h;            /* +3: the cell's height */
    unsigned char m04;
    unsigned char m05;
    unsigned char m06;
    unsigned char m07;
    unsigned char m08;
};
extern struct IconDef9 far gIcons2084[]; /*@ 2084:0000 */

struct IconDef4 {               /* 4-byte rectangle records at 2080 (the icon cell table) */
    unsigned char left;         /* +0 */
    unsigned char top;          /* +1 */
    unsigned char width;        /* +2 */
    unsigned char height;       /* +3 */
};
extern struct IconDef4 far gIcons2080[]; /*@ 2080:0000 */

extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */
extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern int g8115; /*@ ds:8115 */ /* nonzero while a status line is up (src/E082C.C) */
extern char gStr613D[]; /*@ ds:613D */ /* the string the party-status report draws (Purpose unknown) */
extern void DrawRowList(int index); /*@ 0D1C:0454 */

struct CharRec {                /* 243-byte character records at ds:F580 (src/E08E6.C) */
    char pad00;
    unsigned char m01;          /* +01: status bits */
    char name[0x0b];            /* +02: the character's name, NUL padded */
    char pad0d[0xf3 - 0x0d];
};
extern struct CharRec gChars[]; /*@ ds:F580 */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern char far *gFont1A47; /*@ ds:FE4E */ /* the current font (src/E1547.C) */
extern void far *gA9C0; /*@ ds:A9C0 */ /* the 6x6 font loaded from FONT6.FNT (src/E07A9.C) */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
extern char gFontWidth1A47; /*@ ds:8100 */ /* the current font's character width (src/E14EF.C) */
extern int gNamePosX[]; /*@ ds:5E44 */ /* the x of each panel name slot (Purpose unknown) */
extern int gNamePosY[]; /*@ ds:5E4C */ /* the y of each panel name slot (Purpose unknown) */
extern char gStr618C[]; /*@ ds:618C */ /* a fixed panel label (Purpose unknown) */
extern struct Node far *ListAdd(struct Node far *head, struct Node far *item); /*@ 1658:001E */
extern char g5E20; /*@ ds:5E20 */ /* nonzero when the mouse moved (src/E16A5.C) */

struct Node {                   /* the 60-byte panel row records (src/E0CF7.C) */
    struct Node far *next;      /* +00: next node, or null at the tail */
    unsigned key;               /* +04: the sort key ListAdd compares */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    unsigned char kind;         /* +0A: the drawing kind */
    unsigned char modeB;        /* +0B: the "on" drawing mode */
    unsigned char modeC;        /* +0C: the second drawing mode */
    unsigned char pad0D;        /* +0D: unknown (src/E0CF7.C leaves it unnamed) */
    unsigned flags;             /* +0E: the inactive bit and the layout bits */
    void far *src10;            /* +10: the default art pointer */
    void far *src14;            /* +14: the "on" art pointer */
    void far *src18;            /* +18: the second art pointer */
    int region;                 /* +1C: index into the region table ds:009F */
    int x;                      /* +1E: box left */
    int y;                      /* +20: box top */
    unsigned width;             /* +22: box width */
    unsigned height;            /* +24: box height */
    char pad26[0x3c - 0x26];
};
extern struct Node far *gNodePtrs[]; /*@ ds:FB7C */ /* the panel row nodes this unit lists and draws */
extern struct Node gNodeTemplate; /*@ ds:3A62 */ /* the template InitRowNode copies into each node */

struct RowDef8 {                /* 8-byte row definitions at 206B (41 of them) */
    unsigned char m00;
    unsigned char m01;
    unsigned char m02;
    unsigned char m03;
    unsigned char m04;
    char pad05[3];
};
extern struct RowDef8 far gRowDefs206B[]; /*@ 206B:0000 */

extern void far *g5E64[0x11]; /*@ ds:5E64 */ /* the 17 sliced icon pointers this unit builds and draws */

/* ovl0:0000: builds the 17-entry far-pointer table g5E64. For each of the 17 icon definitions in
   gIcons2084 whose +2 byte (the width) is nonzero, it slices a sprite cell out of the overlay buffer
   gOverlayBuf through ds:8082 (the definition's four leading bytes are the cell rectangle) and
   stores the pointer at g5E64[i]; every slot is cleared first. Called by ovl0:2547. */
/*@ ovl0:0000 */ void BuildIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        g5E64[i] = 0;
        if (gIcons2084[i].w)
            g5E64[i] = gFn8082(gIcons2084[i].x, gIcons2084[i].y, gIcons2084[i].w, gIcons2084[i].h,
                gOverlayBuf);
    }
}
/* ovl0:00AF: frees the 17 far pointers in g5E64 through farfree and clears every slot. Called by
   ovl0:28B0. */
/*@ ovl0:00AF */ void FreeIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        if (g5E64[i]) {
            farfree(g5E64[i]);
            g5E64[i] = 0;
        }
    }
}
/* ovl0:00F7: waits for a key through 16A5:005A with the caller's list. When the resident flag
   g3A0 (0000:03A0) is 1, it clears the flag and plays sound 0x14 first. Called by ovl0:0A5E, 0DDC,
   0EE5, 135D and 28F9. */
/*@ ovl0:00F7 */ void WaitKeyOrPause(void far *list)
{
    if (g3A0 == 1) {
        g3A0 = 0;
        PlaySound(0x14);
    }
    DispatchKeyWait(list);
}
/* ovl0:0128: draws one character-panel icon. It maps slot `index` (0..0x10) to the cell row
   gIcons2084[index].m06 + `row`, clips the panel rectangle gIcons2080[row], draws the icon cell
   g5E64[index] inside it through f1A29_000F (mode 2), and unless `mode` is 2 hides and shows the
   mouse pointer around the icon's live area. The rectangle's top-left is (0x14, 0) in mode 0,
   else the icon's own origin gIcons2084[index].m07 + 2 / m08 - 0x40. Called by ovl0:037A, 0504,
   0A5E, 11BA, 207A and 2238. */
/*@ ovl0:0128 */ void DrawPanelIcon(unsigned index, int rowBase, int mode)
{
    register int cellRow;
    int x;
    int y;

    if (index >= 0x11)
        return;
    cellRow = gIcons2084[index].m06 + rowBase;
    if (mode == 0) {
        x = 0x14;
        y = 0;
    } else {
        x = gIcons2084[index].m07 + 2;
        y = gIcons2084[index].m08 - 0x40;
    }
    ClipRect(gIcons2080[cellRow].left, gIcons2080[cellRow].top, x, y, gIcons2080[cellRow].width,
        gIcons2080[cellRow].height, 2, 2);
    if (gIcons2084[index].w)
        f1A29_000F(2, g5E64[index], (x << 3) + gIcons2084[index].m04, y + gIcons2084[index].m05, 0);
    if (mode == 2)
        return;
    HidePointerZone(gIcons2084[index].m07 << 3, gIcons2084[index].m08,
        (gIcons2084[index].m07 + gIcons2080[cellRow].width) << 3,
        gIcons2084[index].m08 + gIcons2080[cellRow].height);
    ClipRect(0x14, 0, gIcons2084[index].m07, gIcons2084[index].m08, gIcons2080[cellRow].width,
        gIcons2080[cellRow].height, 2, 0);
    ShowPointerZone();
}
/* ovl0:037A: flashes the panel icon `index`: draws it in its selected form (mode 1), plays sound
   0x4c, waits 5 frames and draws it again in its plain form (mode 0). Called by ovl0:0A5E, 0DDC,
   0EE5, 135D, 207A, 2238 and 28F9. */
/*@ ovl0:037A */ void FlashPanelIcon(int index)
{
    DrawPanelIcon(index, 1, 0);
    PlaySound(0x4c);
    WaitFrames(5);
    DrawPanelIcon(index, 0, 0);
}
/* ovl0:03B2: initialises one panel row node. It takes the far pointer gNodePtrs[index], copies the
   0x3c-byte template gNodeTemplate over it, then stores the node's key (index + 1), its two word
   keys `key`/`key` + 0x100, the box x (x << 3), y, width, height and the flags 0x1100. Returns the
   node. Called by ovl0:042F. */
/*@ ovl0:03B2 */ struct Node far *InitRowNode(int index, int x, int y, int width, int height, int key)
{
    struct Node far *node;

    node = gNodePtrs[index];
    movmem(&gNodeTemplate, node, 0x3c);
    node->key = index + 1;
    node->key06 = key;
    node->key08 = key + 0x100;
    node->x = x << 3;
    node->y = y;
    node->width = width;
    node->height = height;
    node->flags = 0x1100;
    return node;
}
/* ovl0:042F: builds the panel's linked row list. For i in 0..count-1 it initialises node i from the
   row definition gRowDefs206B[base + i] (the definition's five bytes are the node's x, y, width,
   height and key), then links nodes 1..count-1 onto node 0 with ListAdd and sets g5E20. Returns
   the list head (node 0). Called by ovl0:0A5E, 0DDC, 0EE5, 135D and 28F9. */
/*@ ovl0:042F */ struct Node far *BuildRowNodeList(int base, int count)
{
    struct Node far *list;
    int i;
    int idx;

    list = gNodePtrs[0];
    for (i = 0; i < count; i++) {
        idx = base + i;
        InitRowNode(i, gRowDefs206B[idx].m00, gRowDefs206B[idx].m01, gRowDefs206B[idx].m02,
            gRowDefs206B[idx].m03, gRowDefs206B[idx].m04);
    }
    for (i = 1; i < count; i++)
        list = ListAdd(list, gNodePtrs[i]);
    g5E20 = 1;
    return list;
}
/* ovl0:0504: draws the party-status panel when all four front party slots hold a named character
   (a nonzero first name byte at gChars[i]+2). It counts them, and when there are four it shows the
   gStr613D report over the status line (DrawShadowText, g8115 = 2), hides the pointer, clips the
   report box and draws panel icon 0xf; otherwise it draws icon 0xe. Called by ovl0:1071. */
/*@ ovl0:0504 */ void DrawPartyStatus(void)
{
    char i;
    char count;

    count = 0;
    for (i = 0; i < 4; i++)
        if (gChars[i].name[0])
            count++;
    count = (count == 4);
    if (count == 1) {
        g8115 = 2;
        DrawShadowText(gStr613D, 0xa8, 0x3d, 0xf, 0);
        g8115 = 0;
        LockHidePointer();
        ClipRect(0x15, 0x3d, 0x13, 0x7d, 0x11, 0x28, 2, 0);
        LockShowPointer();
        DrawPanelIcon(0xf, 0, 0);
    } else
        DrawPanelIcon(0xe, 0, 0);
}
/* ovl0:05BA: draws one panel name slot: switches the current font to the 6x6 gA9C0, computes the
   centred x offset (0x3c - width * gFontWidth1A47) / 2 for a name `len` characters wide, hides the
   pointer, draws the fixed label gStr618C at gNamePosX[index]/gNamePosY[index] and the character's
   name gChars[index].name centred on it in `color`, shows the pointer and restores the font.
   Called by ovl0:0EE5 and 23EC. */
/*@ ovl0:05BA */ void DrawCharName(int index, int len, int color)
{
    char far *savedFont;
    int x;

    savedFont = gFont1A47;
    f1A47_0169(gA9C0);
    x = (0x3c - gFontWidth1A47 * len) >> 1;
    LockHidePointer();
    gDrawText(gStr618C, gNamePosX[index], gNamePosY[index], 0xc, 0xc);
    gDrawText(gChars[index].name, gNamePosX[index] + x, gNamePosY[index], color, 0);
    LockShowPointer();
    f1A47_0169(savedFont);
}
/*@ ovl0:065B stub */ void Ovl0_065B(void) {}
/* ovl0:0A4F: draws the row list index 0xa. No direct caller found (reached through an overlay entry
   or jump table). */
/*@ ovl0:0A4F */ void DrawRowList10(void)
{
    DrawRowList(10);
}
/*@ ovl0:0A5E stub */ void Ovl0_0A5E(void) {}
/*@ ovl0:0D5B stub */ void Ovl0_0D5B(void) {}
/*@ ovl0:0DDC stub */ void Ovl0_0DDC(void) {}
/*@ ovl0:0EE5 stub */ void Ovl0_0EE5(void) {}
/*@ ovl0:1071 stub */ void Ovl0_1071(void) {}
/*@ ovl0:10F6 stub */ void Ovl0_10F6(void) {}
/*@ ovl0:1177 stub */ void Ovl0_1177(void) {}
/*@ ovl0:11BA stub */ void Ovl0_11BA(void) {}
/*@ ovl0:1224 stub */ void Ovl0_1224(void) {}
/*@ ovl0:135D stub */ void Ovl0_135D(void) {}
/*@ ovl0:1ADD stub */ void Ovl0_1ADD(void) {}
/*@ ovl0:1BF2 stub */ void Ovl0_1BF2(void) {}
/*@ ovl0:1EA9 stub */ void Ovl0_1EA9(void) {}
/*@ ovl0:1F66 stub */ void Ovl0_1F66(void) {}
/*@ ovl0:2051 stub */ void Ovl0_2051(void) {}
/*@ ovl0:207A stub */ void Ovl0_207A(void) {}
/*@ ovl0:2238 stub */ void Ovl0_2238(void) {}
/*@ ovl0:23EC stub */ void Ovl0_23EC(void) {}
/*@ ovl0:2547 stub */ void Ovl0_2547(void) {}
/*@ ovl0:26C6 stub */ void Ovl0_26C6(void) {}
/*@ ovl0:280D stub */ void Ovl0_280D(void) {}
/*@ ovl0:284A stub */ void Ovl0_284A(void) {}
/*@ ovl0:28B0 stub */ void Ovl0_28B0(void) {}
/*@ ovl0:28F9 stub */ void Ovl0_28F9(void) {}
