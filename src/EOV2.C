/* Overlay 2 of EOB.EXE (3332 bytes, 11 functions, all entries of the overlay): loading the item
   tables and the item/creature sprite sheets, plus the "load a saved game" option panel. Called
   once at start-up: StartupAlloc (07A9:000A) reaches entry 2 (2106:002A, ovl2:09CC) which calls
   SetPointerShape, InitGameTables, LoadItems, LoadItemTypes, SetUiPalette, LoadIntroIcons and
   LoadGamePanel in turn, and
   reaches entry 8 (2106:0048, ovl2:0577) for the ITEMRMP read (docs/design/WHITEPAPER.md 4). The
   sprite sheets are at fixed paragraphs of the overlay's own data (ITEML1 at 20A3, ITEMS1 at
   20A5, THROWN at 20A7, ITEMICN at 20A9 and DECORATE at 2069, from TLINK's segment table), loaded
   through 082C:05EC/082C:051B and sliced into cells through the function pointer ds:8086. All 11
   functions are decompiled to L2; several names are placeholders. */

/*@flags -ml -O -Z */

struct Region {             /* 16-byte records of the region table at ds:009F (src/E1A29.C) */
    int left;
    int top;
    int width;
    int height;
    int saved0;
    int saved1;
    int saved2;
    int saved3;
};

struct ItemType {           /* 16-byte records of ITEMTYPE.DAT, at ds:A9F0 */
    char data[16];
};

struct ItemRec {            /* the 14-byte ITEM.DAT records, at ds:8DD0 */
    char b0[6];
    int word6;              /* +06: set to -1 for all 500 slots before ITEM.DAT is read */
    char b8[6];
};

struct CharRec {            /* 243-byte character records at ds:F580 (src/E0A72.C) */
    unsigned char index;    /* +00: the record's own index, written by InitGameTables */
    char pad01[0x32];
    void far *buf;          /* +33: the 0x514-byte scratch buffer MemAlloc'd per record */
    char pad37[0xF3 - 0x37];
};

struct Pair {               /* two words, the record InitGameTables clears */
    int a;
    int b;
};

struct Triple6 {            /* six bytes, the record InitGameTables clears */
    long v;
    char c[2];
};

struct Pos2 {               /* a word pair, the per-cell origin tables */
    int x;
    int y;
};

struct Size2 {              /* a byte pair, the per-cell width/height tables */
    char w;
    char h;
};

struct Cell4 {              /* a four-byte cell definition, x, y, w, h */
    unsigned char x;
    unsigned char y;
    unsigned char w;
    unsigned char h;
};

struct WordCell {           /* the 4-byte "find the word" records at ds:65F0, 30 of them */
    char unused;            /* +0: always zero */
    char row;               /* +1: a row of gItemL1Icons */
    char line;              /* +2: the "Find line" number */
    char word;              /* +3: the "Enter word" number */
};

extern int g8102; /*@ ds:8102 */
extern int g8115; /*@ ds:8115 */
extern int gTextAttr2; /*@ ds:811B */
extern int gTextAttr1; /*@ ds:811D */
extern int g811F; /*@ ds:811F */
extern struct Region gRegions[]; /*@ ds:009F */
extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */
extern struct ItemRec gItems[]; /*@ ds:8DD0 */
extern struct Pos2 gCellPosL1[]; /*@ ds:64F8 */
extern struct Pos2 gCellPosS1[]; /*@ ds:6534 */
extern char gCellDX[]; /*@ ds:65AC */
extern struct Size2 gCellSize1[]; /*@ ds:65B0 */
extern struct Size2 gCellSize2[]; /*@ ds:65B8 */
extern void far *gItemL1Icons[][4]; /*@ ds:EC9A */
extern void far *gItemS1Icons[][4]; /*@ ds:ED8A */
extern void far *gThrownIcons[][4]; /*@ ds:F2F8 */
extern void far *gThrownIcon0; /*@ ds:FB38 */
extern void far *gThrownIcon1; /*@ ds:FB3C */
extern void far *gThrownIcon2; /*@ ds:FB40 */
extern void far *gThrownIcon3; /*@ ds:EFB4 */
extern void far *gThrownIcon4; /*@ ds:EFBE */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern int gColour179; /*@ ds:0179 */
extern struct WordCell gWordCells[]; /*@ ds:65F0 */
extern unsigned char g65C0[]; /*@ ds:65C0 */
extern unsigned char g65C3[]; /*@ ds:65C3 */
extern struct Cell4 gCellDef65C6[]; /*@ ds:65C6 */
extern void far *gIntroCells[]; /*@ ds:F4BE */
extern void far *gIconDD1C; /*@ ds:DD1C */
extern void far *gIconDD20; /*@ ds:DD20 */
extern void far *gIconDD24; /*@ ds:DD24 */
extern void far *gIconDD28; /*@ ds:DD28 */
extern void far *gIconF2F4; /*@ ds:F2F4 */
extern void far *gIconDC94; /*@ ds:DC94 */
extern void far *gIcon8A7E; /*@ ds:8A7E */
extern void far *gIconTableA92A[]; /*@ ds:A92A */
extern int gItemCount; /*@ ds:E066 */
extern int gItemNameCount; /*@ ds:F516 */
extern unsigned char far gItemNames[]; /*@ 1F28:0000 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */
extern void far *gPtrDD00; /*@ ds:DD00 */
extern void far *gDF48; /*@ ds:DF48 */
extern void far *gSegDF4E; /*@ ds:DF4E */
extern char far *gBufF534; /*@ ds:F534 */
extern void far *gFont1A47; /*@ ds:FE4E */
extern void far *gFE5A; /*@ ds:FE5A */
extern int gTextHeight; /*@ ds:FE60 */
extern int gTextWidth; /*@ ds:FE62 */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern char far *gA9C0; /*@ ds:A9C0 */
extern char gF57E; /*@ ds:F57E */
extern char gFB4C; /*@ ds:FB4C */
extern char gF53A; /*@ ds:F53A */
extern char gF53B; /*@ ds:F53B */
extern char gFB7A; /*@ ds:FB7A */
extern char gFB7B; /*@ ds:FB7B */
extern char gA9C4; /*@ ds:A9C4 */
extern char gA928; /*@ ds:A928 */
extern char gA95A; /*@ ds:A95A */
extern char gA998; /*@ ds:A998 */
extern char g8A6C; /*@ ds:8A6C */
extern char g8A7D; /*@ ds:8A7D */
extern char gFB34; /*@ ds:FB34 */
extern int gEFC8; /*@ ds:EFC8 */
extern int gEFCA; /*@ ds:EFCA */
extern long gB802[]; /*@ ds:B802 */
extern long gFB5E[]; /*@ ds:FB5E */
extern struct Triple6 gA99C[]; /*@ ds:A99C */
extern long gEFD4[]; /*@ ds:EFD4 */
extern char gBuf1CE7[]; /*@ ds:1CE7 */
extern char gData65EA[]; /*@ ds:65EA */
extern char gOverlayBuf[]; /*@ ds:113F */
extern char far *gOptionNames[]; /*@ ds:65DE */

/* the loaded sprite sheets, at fixed paragraphs of the overlay's own data (TLINK segment table) */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */
extern unsigned char far gSheetITEML1[]; /*@ 20A3:0000 */
extern unsigned char far gSheetITEMS1[]; /*@ 20A5:0000 */
extern unsigned char far gSheetTHROWN[]; /*@ 20A7:0000 */
extern unsigned char far gSheetITEMICN[]; /*@ 20A9:0000 */

/* a far function pointer called with a 16x16 cell position on a sprite sheet (src/E07A9.C); it
   returns the cell's far address, filled in at load time [H]. */
extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
extern int (far *gFn8072)(int a, int b); /*@ ds:8072 */
extern void (far *gFillRect)(int a, int b, int c, int d, int e); /*@ ds:806A */
extern void (far *gFn807E)(int x, int y, int w, int h, void far *buf); /*@ ds:807E */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern void LoadScreenImage(char far *name, char far *buf, int a, int b, int c); /*@ 082C:051B */
extern void DrawFilledPanel(int a, int b, int c, int d, int e, int f, int g); /*@ 082C:09D5 */
extern int EditTextField(char far *buf, char a, char b, char c, char d, char e, char f); /*@ 082C:000B */
extern void DrawShadowText(char far *text, int a, int b, int c, int d); /*@ 082C:0898 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void f19EC_002A(int a, int b, char far *c); /*@ 19EC:002A */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void ClearTextRegion(void); /*@ 1DE7:0344 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void DrawOptionList(int panel, char far * far *names, long mask, int shift, int spacing); /*@ 14EF:0154 */
extern int UpdateOptionList(int panel, char far * far *names, long counter, long mask, int shift); /*@ 14EF:0274 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void SetMousePointerShape(int hotX, int hotY, void far *icon); /*@ 1BB1:0002 */
extern void WaitGameKey(void); /*@ 1AAD:053A */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern char far *SkipStrings(char far *p, int n); /*@ 0D1C:07A7 */
extern char far *strcpy(char far *dst, char far *src); /*@ 0000:67CD */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern int strcmp(char far *a, char far *b); /*@ 0000:679D */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern void exit(int status); /*@ 0000:50F3 */
extern int f6815(char far *s); /*@ 0000:6815 */
extern int LoadSavedGame(void); /*@ 210C:0039 */ /* overlay 3 entry (ovl3:15B9): returns nonzero when a saved game was loaded, the caller prints "Unable to load your saved game!" otherwise */
extern void fOVL0_28F9(void); /*@ 20F6:0052 */ /* overlay 0 entry (ovl0:28F9): Purpose unknown, case 1 of the option panel redraws the fonts around it */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */

/* ovl2:0000: Purpose unknown. Loads the ITEMICN sheet, slices one 2x16 cell from it through
   gGetSpriteCell and makes the result (gIconsDDE4[0] and gPtrDD00) the mouse pointer shape at
   (0,0). Called first by LoadItemData. */
/*@ ovl2:0000 */ void SetPointerShape(void)
{
    LoadOverlayScreen("ITEMICN", 5, 3);
    gIconsDDE4[0] = gGetSpriteCell(0, 0, 2, 0x10, gOverlayBuf);
    gPtrDD00 = gIconsDDE4[0];
    SetMousePointerShape(0, 0, gPtrDD00);
    g8115 = 0;
}

/* ovl2:005E: reads ITEM.DAT (whitepaper 4): a u16 item count (432), one 14-byte item record per
   count into gItems (whose word at +6 is first set to -1 for all 500 slots), then a u16 name count
   (98) and one 35-byte name per count into the far buffer gItemNames at 1F28:0000. Called by
   LoadItemData. */
/*@ ovl2:005E */ void LoadItems(void)
{
    int handle;
    int i;

    handle = FileOpen("ITEM.DAT", 1);
    FileRead(handle, (void far *)&gItemCount, 2L);
    for (i = 0; i < 500; i++)
        gItems[i].word6 = -1;
    for (i = 0; i < gItemCount; i++)
        FileRead(handle, (void far *)&gItems[i], 14L);
    FileRead(handle, (void far *)&gItemNameCount, 2L);
    for (i = 0; i < gItemNameCount; i++)
        FileRead(handle, (void far *)(gItemNames + i * 0x23), 0x23L);
    FileClose(handle);
}

/* ovl2:0110: read ITEMTYPE.DAT, a count then 16-byte records */
/*@ ovl2:0110 */ void LoadItemTypes(void)
{
    int n;
    int i;
    int handle;

    handle = FileOpen("ITEMTYPE.DAT", 1);
    FileRead(handle, (void far *)&n, 2L);
    for (i = 0; i < n; i++)
        FileRead(handle, (void far *)&gItemTypes[i], 16L);
    FileClose(handle);
}

/* ovl2:0171: loads the four item icon sheets (ITEML1, ITEMS1, THROWN, ITEMICN), slices every cell
   out of them through gGetSpriteCell into the icon tables (gItemL1Icons, gItemS1Icons,
   gThrownIcons, gIconsDDE4) and the five single THROWN cells, allocates the 0x12C-byte item
   description buffer (gDF48), then reads ITEMRMP.VGA (g8102 == 3) or ITEMRMP.EGA into gBufF534.
   Called by LoadDecorateIcons right before it loads DECORATE. */
/*@ ovl2:0171 */ void LoadItemSheets(void)
{
    int handle;
    int i;
    int j;

    LoadOverlayScreen("ITEML1", 5, 3);
    for (i = 0; i < 0xe; i++)
        for (j = 0; j < 4; j++)
            gItemL1Icons[i][j] = gGetSpriteCell(gCellPosL1[i].x,
                gCellPosL1[i].y + gCellDX[j], gCellSize1[j].w, gCellSize1[j].h,
                gSheetITEML1);

    LoadOverlayScreen("ITEMS1", 5, 3);
    for (i = 0; i < 0x17; i++)
        for (j = 0; j < 4; j++)
            gItemS1Icons[i][j] = gGetSpriteCell(gCellPosS1[i].x,
                gCellPosS1[i].y + gCellDX[j], gCellSize2[j].w, gCellSize2[j].h,
                gSheetITEMS1);

    LoadOverlayScreen("THROWN", 5, 3);
    for (i = 0; i < 0xc; i++)
        for (j = 0; j < 4; j++)
            gThrownIcons[i][j] = gGetSpriteCell(gCellPosS1[i].x,
                gCellPosS1[i].y + gCellDX[j], gCellSize2[j].w, gCellSize2[j].h,
                gSheetTHROWN);

    gThrownIcon0 = gGetSpriteCell(0x10, 0, 4, 0x18, gSheetTHROWN);
    gThrownIcon1 = gGetSpriteCell(0x10, 0x18, 4, 0x18, gSheetTHROWN);
    gThrownIcon2 = gGetSpriteCell(0x10, 0x30, 3, 0x18, gSheetTHROWN);
    gThrownIcon3 = gGetSpriteCell(0x10, 0x90, 5, 0x18, gSheetTHROWN);
    gThrownIcon4 = gGetSpriteCell(0x10, 0xa8, 5, 0x10, gSheetTHROWN);

    LoadOverlayScreen("ITEMICN", 5, 3);
    for (i = 1; i < 0x59; i++)
        gIconsDDE4[i] = gGetSpriteCell((i % 20) * 2, (i / 20) * 16, 2, 0x10, gSheetITEMICN);

    gDF48 = MemAlloc(0x12cL, 0);
    g8115 = 0;
    sprintf(gFileNameBuf, "ITEMRMP.%s", g8102 == 3 ? "VGA" : "EGA");
    handle = FileOpen(gFileNameBuf, 1);
    FileRead(handle, gBufF534, 0x640L);
    FileClose(handle);
}

/* ovl2:0496: clears the item, spell and creature tables and allocates one 0x514-byte scratch
   buffer for each of the six character records (gChars[i].buf). Called by LoadItemData. */
/*@ ovl2:0496 */ void InitGameTables(void)
{
    int i;

    gFB34 = 0xff;
    gEFC8 = 0;
    gEFCA = 0;
    for (i = 0; i < 0x24; i++)
        gB802[i] = 0;
    for (i = 0; i < 6; i++) {
        gFB5E[i] = 0;
        gA99C[i].v = 0;
    }
    for (i = 0; i < 0xc8; i++)
        gEFD4[i] = 0;
    for (i = 0; i < 6; i++) {
        gChars[i].index = i;
        gChars[i].buf = MemAlloc(0x514L, 0);
    }
}

/* ovl2:0577: the overlay-8 entry (reached as 2106:0048), leads to the ITEMRMP read. First calls
   LoadItemSheets, then loads the DECORATE sheet and slices the creature/decoration icons out of
   it: six cells from the INTRO sheet (ds:113F) into gIntroCells, seven single cells, and the 3x4
   gIconTableA92A whose cell width comes from g65C0 and y from g65C3. Called by StartupAlloc
   (07A9:000A). */
/*@ ovl2:0577 */ void LoadDecorateIcons(void)
{
    char i;
    char j;

    LoadItemSheets();
    LoadOverlayScreen("DECORATE", 5, 3);
    for (i = 0; i < 6; i++)
        gIntroCells[i] = gGetSpriteCell(gCellDef65C6[i].x, gCellDef65C6[i].y,
            gCellDef65C6[i].w, gCellDef65C6[i].h, gOverlayBuf);
    gIconDD1C = gGetSpriteCell(0x1d, 0, 2, 0x10, gSheetDECORATE);
    gIconDD20 = gGetSpriteCell(0x1f, 0, 2, 0x10, gSheetDECORATE);
    gIconDD24 = gGetSpriteCell(0x21, 0, 2, 0x10, gSheetDECORATE);
    gIconF2F4 = gGetSpriteCell(0, 0x58, 4, 0x20, gSheetDECORATE);
    gIconDC94 = gGetSpriteCell(4, 0x58, 4, 0x20, gSheetDECORATE);
    gIcon8A7E = gGetSpriteCell(8, 0x58, 4, 0x10, gSheetDECORATE);
    gIconDD28 = gGetSpriteCell(9, 0x58, 2, 8, gSheetDECORATE);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 4; j++)
            gIconTableA92A[i * 4 + j] = gGetSpriteCell(j * 3, g65C3[i], 3, g65C0[i], gSheetDECORATE);
    g8115 = 0;
}

/* ovl2:078D: Purpose unknown. Loads the INTRO sheet into gOverlayBuf and fills the DROW/creature
   icon tables (through gGetSpriteCell, at ds:F4BE and ds:A92A) after drawing the "intro" panels;
   the sprite cells it slices use the byte tables at ds:65C0/65C3/65C6 and the sheet segments
   DECORATE (2069) and 20A9. Called by LoadItemData. */
/*@ ovl2:078D */ void LoadIntroIcons(void)
{
    int handle;

    LoadScreenImage("INTRO", gOverlayBuf, 5, 3, 2);
    g8115 = 2;
    handle = gFn8072(0, 0);
    gFillRect(0, 0x9f, 0x13f, 0xc7, handle);
    DrawFilledPanel(0x4d, 0xa5, 0xf9, 0xc1, 0xd, 0xe, 0xc);
    DrawFilledPanel(0x4c, 0xa4, 0xfa, 0xc2, 0xd, 0xe, -1);
    ClipRect(0, 0, 0, 0, 0x28, 0xc8, 2, 0);
    g8115 = 0;
}

/* ovl2:0846: the "load a saved game" option panel. Draws the panel (DrawOptionList), then loops
   UpdateOptionList until the pointer picks an entry: entry 0 loads a saved game (3 0FFh/0FEh/0
   status byte at ds:A998, print "Unable to load your saved game!" and give up after three tries),
   entry 1 restores the fonts and redraws, entry 2 shuts the game down. Called by LoadItemData. */
/*@ ovl2:0846 */ void LoadGamePanel(void)
{
    int sel;

    SetTextWindow(6);
    for (;;) {
        ClearTextRegion();
        DrawOptionList(6, gOptionNames, 0xffffffffL, 0, 0);
        LockShowPointer();
        do {
            sel = UpdateOptionList(6, gOptionNames, 0L, 0xffffffffL, 0);
        } while (sel == -1);
        LockHidePointer();
        switch (sel) {
        case 0:
            gA998 = 0xff;
            if (!LoadSavedGame()) {
                ClearTextRegion();
                DrawText("Unable to load your saved game!");
                WaitGameKey();
                ClearTextRegion();
                sel = 3;
                gA998 = 0;
            } else {
                gA998 = 0xfe;
            }
            break;
        case 1:
            f1A47_0169(gFE5A);
            fOVL0_28F9();
            f1A47_0169(gA9C0);
            gA998 = 0;
            break;
        case 2:
            ShutdownGame();
            exit(0);
        }
        if (sel == 0 || sel == 1)
            return;
    }
}

/* ovl2:0937: sets the UI palette entries (ds:F57E, FB4C, F53A, FB7B, FB7A, A9C4, A928, A95A,
   8A6C, 8A7D) for the VGA/Tandy modes (g8102 4 or 3) or the other modes, and on the other modes
   copies the 1CE7 table from ds:65EA. Called by LoadItemData. */
/*@ ovl2:0937 */ void SetUiPalette(void)
{
    if (g8102 == 4 || g8102 == 3) {
        gF57E = 0x84;
        gFB4C = 0x82;
        gF53A = 0x87;
        gF53B = 0x85;
        gFB7A = 0x85;
        gFB7B = 0xb4;
        gA9C4 = 0xb1;
        gA928 = 0xb8;
        gA95A = 0x11;
        g8A6C = 0x17;
        g8A7D = 0x14;
    } else {
        gF57E = 2;
        gFB4C = 9;
        gF53A = 0xd;
        gF53B = 0x85;
        gFB7A = 2;
        gFB7B = 0xe;
        gA9C4 = 0xf;
        gA928 = 0xd;
        gA95A = 6;
        g8A6C = 0xd;
        g8A7D = 8;
        strcpy(gBuf1CE7, gData65EA);
    }
}

/* ovl2:09CC, entry 2 (called as 2106:002A): load all the item tables */
/*@ ovl2:09CC */ void LoadItemData(void)
{
    SetPointerShape();
    InitGameTables();
    LoadItems();
    LoadItemTypes();
    SetUiPalette();
    LoadIntroIcons();
    LoadGamePanel();
}

/* ovl2:09ED: the "find the word" dungeon puzzle panel. Saves ds:811F and the font, switches to the
   8x8 font gFE5A, draws the panel frame (through 082C:09D5, 082C:0898 and the region changed by
   the panel rectangle), picks one of the 30 gWordCells records at random (RollDice), draws its item
   icon and the "On the page with this symbol... Find line %d / Enter word %d" text, reads the
   player's word through 082C:000B and compares it with the record's message through 0D1C:07A7
   ("attacks" and the strings after it). Three wrong answers shut the game down; a correct one
   restores the region, the panel and the font. Called by overlay 5 (ovl5:1A82). */
/*@ ovl2:09ED */ void FindWordPanel(void)
{
    void far *font;
    int saveRegion;
    char input[0x1e];
    char text[0x96];
    int pick;
    int tries;

    saveRegion = g811F;
    font = gFont1A47;
    f1A47_0169(gFE5A);
    gColour179 = gF57E;
    g8115 = 0;
    SetTextWindow(0xd);
    gFn807E(gTextLeft, gTextTop, gTextWidth, gTextHeight, gSegDF4E);
    tries = 0;
    ClearTextRegion();
    DrawFilledPanel(gTextLeft * 8, gTextTop, (gTextLeft + gTextWidth) * 8 - 1, gTextTop + gTextHeight - 1,
        gFB4C, gF53A, -1);
    DrawFilledPanel(gTextLeft * 8 + 1, gTextTop + 1, (gTextLeft + gTextWidth) * 8 - 2, gTextTop + gTextHeight - 2,
        gFB4C, gF53A, -1);
    gRegions[g811F].left++;
    gRegions[g811F].top += 2;
    gRegions[g811F].width -= 2;
    gRegions[g811F].height -= 0x10;
    SetTextWindow(0xd);
    for (;;) {
        pick = RollDice(1, 0x1e, -1);
        ClearTextRegion();
        f1A29_000F(0, gItemL1Icons[gWordCells[pick].row][0], 0x64, 2, 0xd);
        sprintf(text, "\r\r\r\rOn the page with this symbol...\r\rFind line %d\rEnter word %d\r",
            gWordCells[pick].line, gWordCells[pick].word);
        DrawShadowText(text, (gTextLeft + 1) * 8, gTextTop, gTextAttr1, gTextAttr2);
        input[0] = 0;
        DrawFilledPanel(0x4c, 0x64, 0xd0, 0x71, gF53A, gFB4C, -1);
        DrawFilledPanel(0x4d, 0x65, 0xcf, 0x70, gF53A, gFB4C, -1);
        while (EditTextField(input, 0xa, 0x67, 0xf, (char)gTextAttr1, (char)gTextAttr2, 8) < 1)
            ;
        while (input[strlen(input) - 1] == ' ')
            input[strlen(input) - 1] = 0;
        f6815(input);
        if (strcmp(SkipStrings("attacks", pick), input) == 0)
            break;
        tries++;
        if (tries == 3) {
            ShutdownGame();
            exit(0);
        }
    }
    gRegions[g811F].left--;
    gRegions[g811F].top -= 2;
    gRegions[g811F].width += 2;
    gRegions[g811F].height += 0x10;
    SetTextWindow(0xd);
    gFn804A(gTextLeft, gTextTop, gTextWidth, gTextHeight, gSegDF4E);
    SetTextWindow(saveRegion);
    f1A47_0169(font);
}
