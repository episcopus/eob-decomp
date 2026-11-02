/* Unit at overlay 0 of EOB.EXE (10873 bytes, 35 functions): the game's character panel and its
   drawing helpers. It builds the 17-cell icon table (ovl0:0000), draws a character's panel and
   equipment (ovl0:065B, 135D), the two option-list screens (ovl0:207A, 2238), the item/text field
   editor (ovl0:23EC) and the status/rest screens, and loads the overlay's screens (ovl0:2547,
   28F9). Placeholder names where no evidence. */

/*@flags -ml -O -Z -v */

#include <dos.h>

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

struct CharRec {                /* 243-byte character records at ds:F580 (src/E0A72.C) */
    char pad00;
    unsigned char m01;          /* +01: status bits */
    char name[0x0b];            /* +02: the character's name, NUL padded */
    char m0D;                   /* +0D: the source of 284A's byte copy */
    char m0E;
    char m0F;
    char m10;
    char m11;
    char m12;
    char m13;
    char m14;
    char m15;
    char m16;
    char m17;
    char m18;
    char m19;
    char m1A;
    char m1B;
    char m1C;                   /* +1C: the destination of 284A's byte copy */
    char m1D;
    char m1E;
    signed char m1F;            /* +1F: half of it indexes gTable5F35 (src/E0A72.C) */
    signed char m20;            /* +20: indexes g1231 (src/E0DE8.C) */
    char pad21[0x23 - 0x21];
    signed char m23;            /* +23: set to 0x64 by ovl0:26C6 (Purpose unknown) */
    signed char m24;            /* +24: read by ovl0:065B (Purpose unknown) */
    signed char m25;            /* +25: read by ovl0:065B (Purpose unknown) */
    signed char m26;            /* +26: read by ovl0:065B (Purpose unknown) */
    char pad27[0x33 - 0x27];
    void far *sprite;           /* +33: the character's packed sprite buffer (src/EOV3.C) */
    char pad37[0x73 - 0x37];
    long m7375;                 /* +73: a 32-bit value 280D sets to 0x26c (Purpose unknown) */
    int m77[27];                /* +77: 27 carried-item slots (src/E0DE8.C) */
    char padAD[0xf3 - 0xAD];
};
extern struct CharRec gChars[]; /*@ ds:F580 */

struct ItemSlot {               /* 4-byte records at ds:60D5 (ovl0:26C6) */
    int item;                   /* +0: an item type, or -1 for "no item" */
    int slot;                   /* +2: the carrying slot it prefers */
};
extern struct ItemSlot g60D5[]; /*@ ds:60D5 */ /* the per-class item/slot table ovl0:26C6 walks (Purpose unknown) */
extern char g6101[]; /*@ ds:6101 */ /* the per-class four-entry item table ovl0:26C6 walks (Purpose unknown) */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern signed char g1231[]; /*@ ds:1231 */ /* byte table indexed by gChars[].m20 (src/E0BD2.C) */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void far *gPtrsFC8C[]; /*@ ds:FC8C */ /* the 0x43 overlay buffers 28B0 frees */
extern int gDCB0; /*@ ds:DCB0 */ /* a character index read by ovl0:11BA (Purpose unknown) */
extern int gPanelX5E54[]; /*@ ds:5E54 */ /* per-character x read by ovl0:11BA (Purpose unknown) */
extern int gPanelY5E5C[]; /*@ ds:5E5C */ /* per-character y read by ovl0:11BA (Purpose unknown) */
extern char gStr61CB[]; /*@ ds:61CB */ /* a fixed report label (Purpose unknown) */
extern char gStr6207[]; /*@ ds:6207 */ /* a fixed report label (Purpose unknown) */
extern int gTable5FC5[][6]; /*@ ds:5FC5 */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gTable5F35[][6]; /*@ ds:5F35 */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gTable5F7D[][6]; /*@ ds:5F7D */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gOutDD04[6]; /*@ ds:DD04 */ /* six words ovl0:1EA9 writes (Purpose unknown) */
extern int gOutDC98[6]; /*@ ds:DC98 */ /* six words ovl0:1EA9 writes (Purpose unknown) */
extern signed char g116E[]; /*@ ds:116E */ /* per-class byte (src/E0A72.C) */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void *LoadOverlayScreen(char far *name, int srcKind, int dstKind); /*@ 082C:05EC */
extern void *LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void f19EC_002A(int a, int b, int c, int d); /*@ 19EC:002A */
extern int g8102; /*@ ds:8102 */ /* the video mode the game initialised (src/E082C.C) */
extern void far *gPtrsFD54[]; /*@ ds:FD54 */ /* the ten overlay buffers ovl0:2547 slices (Purpose unknown) */
extern char gStr6272[]; /*@ ds:6272 */ /* a screen name (Purpose unknown) */
extern char gStr627B[]; /*@ ds:627B */ /* a screen name (Purpose unknown) */
extern char gStr6283[]; /*@ ds:6283 */ /* a screen name (Purpose unknown) */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void DrawOptionList(int panel, char far * far *names, long mask, int shift, int spacing); /*@ 14EF:0154 */
extern int UpdateOptionList(int panel, char far * far *names, long counter, long mask, int shift); /*@ 14EF:0274 */
extern char far *g11AD[]; /*@ ds:11AD */ /* the race names (src/E1547.C) */
extern char far *g11DD[]; /*@ ds:11DD */ /* the class names (src/E1547.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */ /* the buffer sprintf formats into (src/E082C.C) */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern int strlen(char far *s); /*@ 0000:67F6 */
extern char *FormatStrengthPercent(char value, char percent); /*@ 0A72:0468 */
extern char gStr6197[]; /*@ ds:6197 */ /* a fixed panel label (Purpose unknown) */
extern char gStr61A1[]; /*@ ds:61A1 */ /* a format string ovl0:065B uses (Purpose unknown) */
extern char gStr61B3[]; /*@ ds:61B3 */ /* a format string ovl0:065B uses (Purpose unknown) */
extern char gStr61B9[]; /*@ ds:61B9 */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char gStr61BC[]; /*@ ds:61BC */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char gStr61C2[]; /*@ ds:61C2 */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char far *g115D[]; /*@ ds:115D */ /* six far string pointers ovl0:065B draws in a column (Purpose unknown) */
extern int g197; /*@ ds:0197 */ /* a panel field cleared by ovl0:1F66 (Purpose unknown) */
extern int gFC8A; /*@ ds:FC8A */ /* the parity of the key ovl0:1F66 last handled (Purpose unknown) */
extern unsigned long gTicks; /*@ ds:848A */ /* the 32-bit tick counter (src/E082C.C) */
extern signed char g5EB0; /*@ ds:5EB0 */ /* the character ovl0:1224 last drew (Purpose unknown) */
extern signed char gFC84; /*@ ds:FC84 */ /* the character panel's animation frame 0..9 (Purpose unknown) */
extern signed char gFC85; /*@ ds:FC85 */ /* the frame ovl0:1224 last drew (Purpose unknown) */
extern unsigned long gFC86; /*@ ds:FC86 */ /* the tick ovl0:1224 next advances the frame at (Purpose unknown) */
extern unsigned char g6079[]; /*@ ds:6079 */ /* per-character x read by ovl0:1F66 (Purpose unknown) */
extern unsigned char g607D[]; /*@ ds:607D */ /* per-character y read by ovl0:1F66 (Purpose unknown) */
extern char gStr623F[]; /*@ ds:623F */ /* a fixed panel label (Purpose unknown) */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern char far *gFont1A47; /*@ ds:FE4E */ /* the current font (src/E1547.C) */
extern void far *gA9C0; /*@ ds:A9C0 */ /* the 6x6 font loaded from FONT6.FNT (src/E07A9.C) */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
extern char gFontWidth; /*@ ds:8100 */ /* the current font's character width (src/E14EF.C) */
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
/* ovl0:00F7: waits for a key through 16A5:005A with the caller's list and returns its code. When the
   resident flag g3A0 (0000:03A0) is 1, it clears the flag and plays sound 0x14 first. Called by
   ovl0:0A5E, 0DDC, 0EE5, 135D and 28F9. */
/*@ ovl0:00F7 */ int WaitKeyOrPause(void far *list)
{
    if (g3A0 == 1) {
        g3A0 = 0;
        PlaySound(0x14);
    }
    return DispatchKeyWait(list);
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
   centred x offset (0x3c - width * gFontWidth) / 2 for a name `len` characters wide, hides the
   pointer, draws the fixed label gStr618C at gNamePosX[index]/gNamePosY[index] and the character's
   name gChars[index].name centred on it in `color`, shows the pointer and restores the font.
   Called by ovl0:0EE5 and 23EC. */
/*@ ovl0:05BA */ void DrawCharName(int index, int len, int color)
{
    char far *savedFont;
    int x;

    savedFont = gFont1A47;
    f1A47_0169(gA9C0);
    x = (0x3c - gFontWidth * len) >> 1;
    LockHidePointer();
    gDrawText(gStr618C, gNamePosX[index], gNamePosY[index], 0xc, 0xc);
    gDrawText(gChars[index].name, gNamePosX[index] + x, gNamePosY[index], color, 0);
    LockShowPointer();
    f1A47_0169(savedFont);
}
/* ovl0:065B: draws the current character's panel page for gChars[index]. It clips the page and,
   unless `mode` is 4, draws the character's sprite, then the name, race g11AD[m1F] and class
   g11DD[m20] centred over the page and six labels g115D[8..13] down the left column. It formats
   three stat lines into gFileNameBuf (the ratio of m0D against m0F with m11, m13, m15, m17, m19, the
   m1D/m1C pair and the m24/m25/m26 triple, the third picking one of three gStr61xx formats from
   m25 and m26) and draws each; the mode selects which panel icons are highlighted. Called by
   ovl0:0A5E, 0DDC, 0EE5, 135D and 23EC. Purpose unknown: the mode-specific icons. */
/*@ ovl0:065B draft */ void Ovl0_065B(int index, char mode)
{
    char far *text;
    int i;

    LockHidePointer();
    ClipRect(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    g8115 = 2;
    if (mode != 4)
        f1A29_000F(2, gChars[index].sprite, 0xe0, 2, 0);
    text = gChars[index].name;
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x23, 0xf, 0);
    text = g11AD[gChars[index].m1F];
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x2d, 0xf, 0);
    text = g11DD[gChars[index].m20];
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x36, 0xf, 0);
    for (i = 8; i < 0xe; i++)
        DrawShadowText(g115D[i], 0xa3, i * 8, 0xf, 0);
    DrawShadowText(gStr6197, 0xf8, 0x40, 0xf, 0);
    sprintf(gFileNameBuf, gStr61A1, FormatStrengthPercent(gChars[index].m0D, gChars[index].m0F),
        gChars[index].m11, gChars[index].m13, gChars[index].m15, gChars[index].m17,
        gChars[index].m19);
    DrawShadowText(gFileNameBuf, 0xc0, 0x40, 0xf, 0);
    sprintf(gFileNameBuf, gStr61B3, gChars[index].m1D, gChars[index].m1C);
    DrawShadowText(gFileNameBuf, 0x118, 0x40, 0xf, 0);
    text = gStr61B9;
    if (gChars[index].m25 > 0)
        text = gStr61BC;
    if (gChars[index].m26 > 0)
        text = gStr61C2;
    sprintf(gFileNameBuf, text, gChars[index].m24, gChars[index].m25, gChars[index].m26);
    DrawShadowText(gFileNameBuf, 0x118, 0x50, 0xf, 0);
    switch (mode) {
    case 1:
        DrawPanelIcon(4, 0, 2);
        DrawPanelIcon(7, 0, 2);
        DrawPanelIcon(8, 0, 2);
        DrawPanelIcon(6, 0, 2);
        break;
    case 2:
        DrawPanelIcon(0x10, 0, 2);
        break;
    case 3:
        DrawPanelIcon(0xa, 0, 2);
        DrawPanelIcon(0xb, 0, 2);
        DrawPanelIcon(9, 0, 2);
        break;
    }
    ClipRect(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
    g8115 = 0;
}
/* ovl0:0A4F: draws the row list index 0xa. No direct caller found (reached through an overlay entry
   or jump table). */
/*@ ovl0:0A4F */ void DrawRowList10(void)
{
    DrawRowList(10);
}
/*@ ovl0:0A5E stub */ void Ovl0_0A5E(void) {}
/* ovl0:0D5B: walks the panel slot indices modulo 0x2c (44) to find one that is free. Starting from
   `start` (a negative start is folded to 0x2b), it loops: a slot is rejected when one of the four
   named characters gChars[0..3] has `taken[di]` equal to it, when `flag` is nonzero and the slot is
   below 0x1a, or when `flag` is zero and the slot is above 0x1c; a rejected slot advances by
   `delta` (folded and wrapped) and the search repeats. Returns the free slot. Called by ovl0:0A5E. */
/*@ ovl0:0D5B */ int FindSlot(int start, int flag, int delta, char far *taken)
{
    int slot;
    int found;
    int di;

    slot = start;
    if (slot < 0)
        slot = 0x2b;
    slot %= 0x2c;
    do {
        found = 0;
        di = 0;
        for (; di < 4; di++) {
            if (gChars[di].name[0] && taken[di] == slot)
                found = 1;
        }
        if (flag != 0 && slot < 0x1a)
            found = 1;
        if (flag == 0 && slot > 0x1c)
            found = 1;
        if (found) {
            slot += delta;
            if (slot < 0)
                slot = 0x2b;
            slot %= 0x2c;
        }
    } while (found);
    return slot;
}
void Ovl0_1224(void);              /* ovl0:1224, defined below */
void Ovl0_1ADD(void);              /* ovl0:1ADD, defined below */
void Ovl0_1BF2(int who);           /* ovl0:1BF2, defined below */
void DrawPanelRow(int index);      /* ovl0:11BA, defined below */
/* ovl0:0DDC: the character panel's function-key loop. It opens the panel page, builds the 0x1b-based
   four-row list, then until a key ends it redraws the animation and waits: the enter key (0x61) and
   key 0x8004 flash icon 6 and keep looping, keys 0x8001/0x8002/0x8003 flash icons 4/7/8 and run the
   combat tables (ovl0:1BF2), the race picker (Ovl0_1ADD) or the item list (ovl0:0A5E), each redrawing
   the page and list, and any other key exits after drawing the current row list. Called by no direct
   caller found (reached through an overlay entry). Purpose unknown: the 0x8001..0x8004 key codes. */
/*@ ovl0:0DDC */ int Ovl0_0DDC(void)
{
    struct Node far *list;
    register int key;

    Ovl0_0A5E();
    Ovl0_065B(gDCB0, 1);
    list = BuildRowNodeList(0x1b, 4);
    FlushKeyBuffer();
    for (;;) {
        Ovl0_1224();
        key = WaitKeyOrPause(list);
        switch (key) {
        case 0x8001:
            FlashPanelIcon(4);
            Ovl0_1224();
            Ovl0_1BF2(gDCB0);
            key = -1;
            break;
        case 0x8002:
            FlashPanelIcon(7);
            Ovl0_1ADD();
            key = -1;
            break;
        case 0x8003:
            FlashPanelIcon(8);
            Ovl0_0A5E();
            key = -1;
            break;
        case 0x61:
        case 0x8004:
            FlashPanelIcon(6);
            key = 1;
            break;
        default:
            key = 0;
            break;
        }
        if (key < 0) {
            Ovl0_065B(gDCB0, 1);
            list = BuildRowNodeList(0x1b, 4);
            key = 0;
        }
        if (key == 0)
            continue;
        DrawRowList(gDCB0 + 6);
        DrawRowList(-1);
        return 0;
    }
}
/* ovl0:0EE5: Purpose unknown: the character-editor loop. It builds a seven-row list, draws the page
   ovl0:065B(mode 2), then until the current character has a name and no exit was asked for it waits
   for a key: the four 0x8001..0x8004 keys pick the character (key & 0xff) - 1, 0x8005 returns 1
   (leaving the editor), 0x8006 and the enter/left/right keys flash icon 9 and clear the result,
   0x8007 erases the current character's name (icon 0x10, DrawCharName, DrawPanelRow), and 0x5c/0x66
   and 0x60/0x62 flip the character index's bit 0 and bit 1. Every pass redraws the row list and, when
   the selection moved to a named character, the page. Returns the result. Called by ovl0:28F9. */
/*@ ovl0:0EE5 */ int Ovl0_0EE5(void)
{
    int key;
    int result;
    struct Node far *list;
    register int pos;
    register int flag;

    list = BuildRowNodeList(0, 7);
    FlushKeyBuffer();
    DrawRowList(-1);
    result = 0;
    flag = 0;
    Ovl0_065B(gDCB0, 2);
    while (gChars[gDCB0].name[0] != 0 && !flag) {
        DrawRowList(gDCB0 + 6);
        key = WaitKeyOrPause(list);
        pos = gDCB0;
        switch (key) {
        case 0x8001:
        case 0x8002:
        case 0x8003:
        case 0x8004:
            pos = (key & 0xff) - 1;
            break;
        case 0x8005:
            result = 1;
            flag = 1;
            break;
        case 0x2b:
        case 0x3d:
        case 0x61:
        case 0x6e:
        case 0x8006:
            FlashPanelIcon(9);
            result = 0;
            flag = 1;
            break;
        case 0x8007:
            if (gChars[gDCB0].name) {
                FlashPanelIcon(0x10);
                gChars[gDCB0].name[0] = 0;
                DrawCharName(gDCB0, 1, 0xc);
                DrawPanelRow(gDCB0 + 0x32);
            }
            break;
        case 0x5c:
        case 0x66:
            pos ^= 1;
            break;
        case 0x60:
        case 0x62:
            pos ^= 2;
            break;
        }
        if (flag)
            DrawRowList(-1);
        if (gChars[pos].name[0] == 0)
            flag = 1;
        if (pos != gDCB0) {
            gDCB0 = pos;
            DrawRowList(-1);
            if (!flag)
                Ovl0_065B(gDCB0, 2);
        }
    }
    return result;
}
/* ovl0:1071: draws the panel report line gStr61CB at (0xa8, 0x10) over a fresh region (g8115 = 2
   while it draws), restores g8115, redraws the report box and then calls DrawPartyStatus. Called
   by ovl0:28F9. Purpose unknown. */
/*@ ovl0:1071 */ void ShowReportLine1(void)
{
    int saved;

    ClipRect(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    saved = g8115;
    g8115 = 2;
    DrawShadowText(gStr61CB, 0xa8, 0x10, 0xf, 0);
    g8115 = saved;
    LockHidePointer();
    ClipRect(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
    DrawPartyStatus();
}

/* ovl0:10F6: the same as ovl0:1071 but draws gStr6207 at (0xa8, 0x20) and does not call
   DrawPartyStatus. Called by ovl0:28F9. Purpose unknown. */
/*@ ovl0:10F6 */ void ShowReportLine2(void)
{
    int saved;

    ClipRect(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    saved = g8115;
    g8115 = 2;
    DrawShadowText(gStr6207, 0xa8, 0x20, 0xf, 0);
    g8115 = saved;
    LockHidePointer();
    ClipRect(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
}
/* ovl0:1177: rolls an ability score: four 1d6 rolls summed with the lowest dropped. Returns the
   sum as a byte. Called by ovl0:1BF2. */
/*@ ovl0:1177 */ char RollAbilityScore(void)
{
    int sum;
    int roll;
    int i;
    int min;

    min = 0xa;
    sum = 0;
    for (i = 0; i < 4; i++) {
        roll = RollDice(1, 6, 0);
        sum += roll;
        if (roll < min)
            min = roll;
    }
    sum -= min;
    return sum;
}
/* ovl0:11BA: draws the panel row the icon index `index` maps to. It clears the row list (-1), hides
   the pointer, then either draws panel icon index - 0x32 when `index` is above 0x30, or slices the
   current character gDCB0's sprite over the row through f1A29_000F (mode 0) at gPanelX5E54/gDCB0,
   gPanelY5E5C/gDCB0 + 1. Shows the pointer afterwards. Called by ovl0:0EE5 and 23EC. */
/*@ ovl0:11BA */ void DrawPanelRow(int index)
{
    int row;

    row = index;
    DrawRowList(-1);
    LockHidePointer();
    if (row > 0x30) {
        row -= 0x32;
        DrawPanelIcon(row, 0, 0);
    } else
        f1A29_000F(0, gChars[gDCB0].sprite, gPanelX5E54[gDCB0], gPanelY5E5C[gDCB0] + 1, 0);
    LockShowPointer();
}
/* ovl0:1224: advances and draws the current character's animation frame. When the selected character
   changed it resets the frame timer gFC86; when gFC86 is behind gTicks it steps gFC84 through 0..9
   and sets gFC86 two ticks ahead. If the frame did not change it returns, else it clips the character
   cell, blits cell gPtrsFD54[gFC84] at (0x120, 0x80) through f1A29_000F, hides the pointer over the
   character's panel box, clips it and shows the pointer. Called by ovl0:0A5E, 0DDC, 135D, 1F66,
   207A and 2238. Purpose unknown: which animation the ten cells are. */
/*@ ovl0:1224 */ void Ovl0_1224(void)
{
    if (g5EB0 != gDCB0) {
        gFC86 = 0;
        g5EB0 = gDCB0;
    }
    if (gFC86 < gTicks) {
        gFC84++;
        if (gFC84 > 9)
            gFC84 = 0;
        gFC86 = gTicks + 2;
    }
    if (gFC85 == gFC84)
        return;
    ClipRect(gDCB0 * 4, 0x80, 0x24, 0x80, 4, 0x20, 2, 2);
    f1A29_000F(2, gPtrsFD54[gFC84], 0x120, 0x80, 0);
    HidePointerZone(gPanelX5E54[gDCB0], gPanelY5E5C[gDCB0] + 1,
        gPanelX5E54[gDCB0] + 0x20, gPanelY5E5C[gDCB0] + 0x20);
    ClipRect(0x24, 0x80, gPanelX5E54[gDCB0] >> 3, gPanelY5E5C[gDCB0] + 1,
        4, 0x20, 2, 0);
    ShowPointerZone();
    gFC85 = gFC84;
}
/*@ ovl0:135D stub */ int Ovl0_135D(int i, char far *field, char far *other) { return 0; }
/* ovl0:1ADD: Purpose unknown: edits one of the seven ability/stat fields of the current character. It draws the page
   through ovl0:065B(mode 3) and loops over the field index i (0..6); for each it opens the field with
   Ovl0_135D(i, &gChars[gDCB0].m? , second) and redraws the page. Field 0 passes both m0D and m0F;
   fields 1..6 pass m11, m13, m15, m17, m19 and m1B with no second field. A return of -2 leaves, a
   negative return wraps i to 6, then i is taken modulo 7. Finally it redraws the page through
   ovl0:065B(mode 1). Called by ovl0:0DDC and ovl0:0EE5. */
/*@ ovl0:1ADD */ void Ovl0_1ADD(void)
{
    int i;

    i = 0;
    FlushKeyBuffer();
    Ovl0_065B(gDCB0, 3);
    while (i >= 0 && i <= 6) {
        switch (i) {
        case 0:
            i = Ovl0_135D(i, &gChars[gDCB0].m0D, &gChars[gDCB0].m0F);
            break;
        case 1:
            i = Ovl0_135D(i, &gChars[gDCB0].m11, (char far *)0);
            break;
        case 2:
            i = Ovl0_135D(i, &gChars[gDCB0].m13, (char far *)0);
            break;
        case 3:
            i = Ovl0_135D(i, &gChars[gDCB0].m15, (char far *)0);
            break;
        case 4:
            i = Ovl0_135D(i, &gChars[gDCB0].m17, (char far *)0);
            break;
        case 5:
            i = Ovl0_135D(i, &gChars[gDCB0].m19, (char far *)0);
            break;
        case 6:
            i = Ovl0_135D(i, &gChars[gDCB0].m1B, (char far *)0);
            break;
        }
        Ovl0_065B(gDCB0, 3);
        if (i == -2)
            break;
        if (i < 0)
            i = 6;
        i %= 7;
    }
    Ovl0_065B(gDCB0, 1);
}
/*@ ovl0:1BF2 stub */ void Ovl0_1BF2(int who) {}
/* ovl0:1EA9: builds two six-word combat tables for the current character gDCB0. For each of the
   six slots it takes the larger of gTable5FC5[m20][i] and gTable5F35[m1F>>1][i] into gOutDD04[i],
   copies gTable5F7D[m1F>>1][i] into gOutDC98[i], and when the class byte g116E[m20] is nonzero
   forces gOutDC98[0] to 0x12. Called by ovl0:1BF2. Purpose unknown. */
/*@ ovl0:1EA9 */ void BuildCombatTables(void)
{
    int cls;
    int row;
    int i;

    cls = gChars[gDCB0].m1F >> 1;
    row = gChars[gDCB0].m20;
    for (i = 0; i < 6; i++) {
        if (gTable5FC5[row][i] > gTable5F35[cls][i])
            gOutDD04[i] = gTable5FC5[row][i];
        else
            gOutDD04[i] = gTable5F35[cls][i];
        gOutDC98[i] = gTable5F7D[cls][i];
        if (g116E[row])
            gOutDC98[0] = 0x12;
    }
}/* ovl0:1F66: opens the option list for the current character's race and returns the chosen value.
   It draws the character's panel box (DrawRect), clips and labels it, then repeatedly runs
   UpdateOptionList(1, g11AD, 0L, 0xffffffffL, 0) until it returns a value other than -1, redrawing
   the panel row ovl0:1224 after each -1, and stores the value's parity in gFC8A. Called by
   ovl0:23EC. */
/*@ ovl0:1F66 */ int PickOption(void)
{
    int choice;

    g8115 = 0;
    LockHidePointer();
    DrawRect(g6079[gDCB0], g607D[gDCB0], g6079[gDCB0] + 0x20, g607D[gDCB0] + 0x21, 0xc);
    ClipRect(0, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    DrawShadowText(gStr623F, 0x93, 0x43, 9, 0);
    LockShowPointer();
    FlushKeyBuffer();
    g197 = 0;
    DrawOptionList(1, g11AD, 0xffffffffL, 0, 0);
    while ((choice = UpdateOptionList(1, g11AD, 0L, 0xffffffffL, 0)) == -1)
        Ovl0_1224();
    gFC8A = choice % 2;
    return choice;
}
/* ovl0:2051: returns the index (0-based) of the `n`th set bit of `mask`, scanning up to `count`
   bits; when fewer than `n` bits are set it returns `count`. No direct caller found. */
/*@ ovl0:2051 */ int FindNthSetBit(int mask, int n, int count)
{
    int remaining;
    int i;

    remaining = n;
    for (i = 0; i < count; i++) {
        if (mask & (1 << i)) {
            remaining--;
            if (remaining == 0)
                break;
        }
    }
    return i;
}
/*@ ovl0:207A stub */ void Ovl0_207A(void) {}
/*@ ovl0:2238 stub */ void Ovl0_2238(void) {}
/*@ ovl0:23EC stub */ void Ovl0_23EC(void) {}
/* ovl0:2547: loads the character panel's screens and slices its icon buffers. It allocates the 0x13
   0x3c-byte gNodePtrs nodes, loads three named screens (gStr6272, gStr627B, gStr6283) and slices
   the 0x2c panel cells gPtrsFC8C ((i%10)*4, (i/10)*0x20) and ten row cells gPtrsFD54 (0, i*4) out
   of gOverlayBuf through ds:8082, builds the 17-icon table and, in video modes 0 and 1, blits
   gOverlayBuf with f19EC_002A and clips the panel. Called by ovl0:28F9. */
/*@ ovl0:2547 */ void InitPanelScreens(void)
{
    int i;

    for (i = 0; i < 0x13; i++)
        gNodePtrs[i] = (struct Node far *)MemAlloc(0x3cL, 0);
    LoadOverlayScreen(gStr6272, 5, 3);
    for (i = 0; i < 0x2c; i++)
        gPtrsFC8C[i] = gFn8082((i % 10) * 4, (i / 10) * 0x20, 4, 0x20, gOverlayBuf);
    g8115 = 0;
    LoadScreenImage(gStr627B, gOverlayBuf, 5, 3, 0);
    LoadOverlayScreen(gStr6283, 5, 3);
    for (i = 0; i < 0xa; i++)
        gPtrsFD54[i] = gFn8082(i * 4, 0, 4, 0x20, gOverlayBuf);
    BuildIconTable();
    if (g8102 == 0 || g8102 == 1) {
        f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
        ClipRect(0, 0, 0, 0, 0x28, 0xc8, 4, 2);
    } else
        f19EC_002A(3, 2, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    g8115 = 0;
    ClipRect(0x12, 0x40, 0, 0, 0x14, 0x80, 0, 2);
    LockShowPointer();
}
/* ovl0:26C6: gives each of the first four characters its class starting equipment. It copies item
   0x23 into gChars[0]'s slot 2, then for each character sets m01 = 1, m23 = 0x64, pad00 = i and slot
   3 = item 0xa; for each of the class's four table entries g6101[m20][j] it looks up g60D5[idx]'s
   item and slot: an item of -1 is skipped, a negative slot takes the first free of slots 0..2, an
   occupied slot is skipped, item 5 for m1F >> 1 == 3 becomes 0x24, the item is copied into a free
   slot (the first free from slot 2) and stored. Finally it rebuilds the character's armour class
   through ComputeArmourClass. Called by ovl0:28F9. Purpose unknown: the item numbers. */
/*@ ovl0:26C6 */ void Ovl0_26C6(void)
{
    int j;
    int i;
    int idx;
    struct CharRec far *rec;
    register int item;
    register int slot;

    item = CopyItemToFreeSlot(0x23);
    gChars[0].m77[2] = item;
    for (i = 0; i < 4; i++) {
        rec = &gChars[i];
        rec->m01 = 1;
        rec->m23 = 0x64;
        rec->pad00 = i;
        item = CopyItemToFreeSlot(0xa);
        rec->m77[3] = item;
        for (j = 0; j < 4; j++) {
            idx = g6101[rec->m20 * 4 + j];
            item = g60D5[idx].item;
            slot = g60D5[idx].slot;
            if (item == -1)
                continue;
            if (slot < 0) {
                slot = 0;
                if (rec->m77[slot] != 0)
                    slot = 1;
                if (rec->m77[slot] != 0)
                    slot = 2;
            }
            if (slot != 2 && rec->m77[slot] != 0)
                continue;
            if (item == 5 && (rec->m1F >> 1) == 3)
                item = 0x24;
            item = CopyItemToFreeSlot(item);
            if (slot != 2) {
                rec->m77[slot] = item;
            } else {
                slot = 2;
                while (rec->m77[slot] != 0)
                    slot++;
                rec->m77[slot] = item;
            }
        }
        ComputeArmourClass(i);
    }
}
/* ovl0:280D: for each of the first four characters whose class byte g1231[gChars[i].m20] has bit 2
   set, sets the 32-bit gChars[i].m7375 to 0x26c. Called by ovl0:28F9. Purpose unknown. */
/*@ ovl0:280D */ void InitCharSlotValues(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (g1231[gChars[i].m20] & 2)
            gChars[i].m7375 = 0x26c;
    }
}

/* ovl0:284A: for each of the first four character records, copies the nine bytes at +0x0D, +0x0F,
   +0x11, +0x13, +0x15, +0x17, +0x19 and +0x1B one byte up (to +0x0E, +0x10, ..., +0x1C) through a
   far pointer walked by 0xf3. Called by ovl0:28F9. Purpose unknown. */
/*@ ovl0:284A */ void ShiftCharFields(void)
{
    struct CharRec far *rec;
    int i;

    rec = (struct CharRec far *)gChars;
    for (i = 0; i < 4; i++, rec++) {
        rec->m0E = rec->m0D;
        rec->m10 = rec->m0F;
        rec->m12 = rec->m11;
        rec->m14 = rec->m13;
        rec->m16 = rec->m15;
        rec->m18 = rec->m17;
        rec->m1A = rec->m19;
        rec->m1C = rec->m1B;
    }
}

/* ovl0:28B0: releases the panel's buffers: every far pointer in the 0x43-entry table gPtrsFC8C,
   then the 17-icon table through FreeIconTable, then every far pointer in the 0x13-entry table
   gNodePtrs. Called by ovl0:28F9. */
/*@ ovl0:28B0 */ void FreePanelBuffers(void)
{
    int i;

    for (i = 0; i < 0x43; i++)
        farfree(gPtrsFC8C[i]);
    FreeIconTable();
    for (i = 0; i < 0x13; i++)
        farfree(gNodePtrs[i]);
}
/* ovl0:28F9: the character panel's main loop. It loads the screens, sets text window 2, shows a
   report line and the five-row list, then until a finished flag `flag` is set it draws the current
   character's row list, waits for a key through ovl0:00F7, applies the key: the two function-key
   toggles (gDCB0 ^= 1, ^= 2) on the 0x5c..0x66 range, the negative-key character switch
   ((key & 0xf) - 1; 4 sets `flag`), and the enter/left/right keys (0x61, 0x2b, 0x3d) which open the
   character editor ovl0:0EE5/23EC. When `flag` is set it flashes icon 0xf, shows report line 2,
   runs Ovl0_26C6, refreshes the character fields (ovl0:280D, 284A), frees the panel buffers and
   hides the pointer. Called by LoadGamePanel (ovl2:0846). */
/*@ ovl0:28F9 */ void RunCharacterPanel(void)
{
    int i;
    int save;
    struct Node far *list;
    register int flag;
    register int key;

    InitPanelScreens();
    flag = 0;
    SetTextWindow(2);
    ShowReportLine1();
    list = BuildRowNodeList(0, 5);
    PlaySound(0x14);
    do {
        DrawRowList(gDCB0 + 6);
        key = WaitKeyOrPause(list);
        if (key != 0) {
            switch (key) {
            case 0x5c:
            case 0x66:
                gDCB0 ^= 1;
                break;
            case 0x60:
            case 0x62:
                gDCB0 ^= 2;
                break;
            }
            DrawRowList(-1);
        }
        if (key < 0) {
            key = (key & 0xf) - 1;
            if (key == 4)
                flag = 1;
            else {
                gDCB0 = key;
                key = 0x2b;
            }
        }
        if (key == 0x61 || key == 0x2b || key == 0x3d) {
            DrawRowList(-1);
            if (gChars[gDCB0].name[0]) {
                save = gDCB0;
                if (Ovl0_0EE5())
                    flag = 1;
                if (gChars[gDCB0].name[0] == 0 && save != gDCB0)
                    Ovl0_23EC();
            } else
                Ovl0_23EC();
            list = BuildRowNodeList(0, 5);
            ShowReportLine1();
        }
        if (flag) {
            for (i = 0; i < 4; i++)
                if (gChars[i].name[0] == 0)
                    flag = 0;
        }
    } while (flag == 0);
    FlashPanelIcon(0xf);
    ShowReportLine2();
    Ovl0_26C6();
    InitCharSlotValues();
    ShiftCharFields();
    FreePanelBuffers();
    LockHidePointer();
}
