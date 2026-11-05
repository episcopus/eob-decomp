/* Overlay 2 of EOB.EXE (3332 bytes, 11 functions, all entries of the overlay): loading the item
   tables and the item/creature sprite sheets, plus the "load a saved game" option panel. Called
   once at start-up: StartupAlloc (07A9:000A) reaches entry 2 (2106:002A, ovl2:09CC), which calls
   SetPointerShape, InitGameTables, LoadItems, LoadItemTypes, SetUiPalette, LoadIntroScreen and
   LoadGamePanel in turn; StartupAlloc also reaches entry 8 (2106:0048, ovl2:0577) for the ITEMRMP
   read (docs/design/WHITEPAPER.md 4). The art pages are loaded from the PAK data by
   LoadOverlayScreen/LoadScreenImage (082C:05EC/082C:051B, through LoadPacked) into page kind 3, and
   LoadOverlayScreen sets the draw page to 2. Cells are cut by the function pointer ds:8086 (gGetSpriteCell,
   graphics driver export 16) from the page after the draw page, so from the art just loaded (CellSource,
   src/drivers/MCGA.ASM:1485-1499). Its fifth argument is a 32-byte CGA colour map of values 0..3, not a
   source: MCGA does not use it (src/drivers/MCGA.ASM:1522-1525) and CGA reads it only for its dither tables
   (src/drivers/CGA.ASM:1666-1671). The PLAYFLD and item art have their own maps, one TLINK data segment each (PLAYFLD
   at 2069, ITEML1 at 20A3, ITEMS1 at 20A5, THROWN at 20A7, ITEMICN at 20A9); gCgaMapDefault (ds:113F) is the default
   map: DECORATE uses it for its six cells (LoadDecorateIcons), and LoadOverlayScreen passes it to LoadScreenImage as its `extra` argument (src/E082C.C:301-314, :329).
   LoadOverlayScreen passes mode -1, which makes LoadScreenImage return before the 19EC:002A blit, so only a
   load with another mode, such as the INTRO load below (ovl2:078D, mode 2), hands the map to 19EC:002A
   (src/E19EC.C:29-35). ITEMICN is loaded and cut twice: SetPointerShape takes the pointer cell with gCgaMapDefault
   as its map, LoadItemSheets the 0x58 icon cells with gCgaMapItemIcn. All 11 functions are at L2. */

/*@flags -ml -O -Z */

#include "EOB.H"

struct ItemRec {            /* the 14-byte ITEM.DAT records, at ds:8DD0 */
    char b0[6];
    int word6;              /* +06: set to -1 for all 500 slots before ITEM.DAT is read */
    char b8[6];
};

struct Pos2 {               /* a word pair, the per-cell origin tables */
    int x;
    int y;
};

struct Size2 {              /* a byte pair, the per-cell width/height tables */
    char w;
    char h;
};

struct WordCell {           /* the 4-byte "find the word" records at ds:65F0, 30 of them */
    char unused;            /* +0: always zero */
    char row;               /* +1: a row of gItemL1Icons */
    char line;              /* +2: the "Find line" number */
    char word;              /* +3: the "Enter word" number */
};

extern int gGraphicsChoice; /*@ ds:8102 */
extern int gDrawPage; /*@ ds:8115 */
extern int gTextAttr2; /*@ ds:811B */
extern int gTextAttr1; /*@ ds:811D */
extern int gTextRegion; /*@ ds:811F */
extern struct Region gRegions[]; /*@ ds:009F */
extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */
extern struct ItemRec gItems[]; /*@ ds:8DD0 */
extern struct Pos2 gCellPosL1[]; /*@ ds:64F8 */
extern struct Pos2 gCellPosS1[]; /*@ ds:6534 */
extern char gCellDX[]; /*@ ds:65AC */ /* four per-column y offsets added to a cell's y in LoadItemSheets (src/EOV2.C:269, :276, :283) */
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
extern struct WordCell gWordCells[]; /*@ ds:65F0 */
extern unsigned char gCompassRowHeights[]; /*@ ds:65C0 */ /* the three per-row cell heights of the gCompassIcons loop, indexed by the row i (0..2) (src/EOV2.C:366-368) */
extern unsigned char gCompassRowTops[]; /*@ ds:65C3 */ /* the three per-row cell y origins of the gCompassIcons loop, indexed by the row i (0..2) (src/EOV2.C:366-368) */
extern struct Cell4 gDecorateCellRects[]; /*@ ds:65C6 */ /* the six decoration-cell rectangles LoadDecorateIcons cuts from the DECORATE page (gCgaMapDefault is their colour map) */
extern void far *gTeleporterCells[]; /*@ ds:F4BE */ /* the six cells LoadDecorateIcons cuts from the DECORATE page (src/EOV2.C:314); their only reader, DrawSlotTeleporter (ovl4:1D20, src/EOV4.C:991), draws them for the animated wall type 0x34, alternating by gAnimWallPhase. That 0x34 is the teleporter is [H] (ScummVM's EOB1 wall type 52, re/renames.toml DrawSlotTeleporter), not proven on our bytes */
extern char far *gFlashFrames[]; /*@ ds:DD1C */ /* the flash-frame images src/E0EF3.C lays over a member, indexed by frame - 1 (src/E0EF3.C:356, :707, :773). The readers compute a frame 0 to 3 and then skip 0 (if (frame-- == 0) continue;, src/E0EF3.C:696, :771), so 0 means no frame and frames 1 to 3 index gFlashFrames[0..2]: DrawFlashFrame (0EF3:05BD) computes (gFlashPassMask[pass] & gFlashFrameMask[i]) >> gFlashFrameShift[i] from the bytes at ds:5621 (40 90 e4 b9 6e 1b 06 01), ds:5629 (c0 30 0c 03) and ds:562D (06 04 02 00), and FlashViewDamage (0EF3:06E7) does the same with gFlashPass2 (ds:564B), gFlashFrame2 (ds:5677) and gFlashShift2 (ds:56B7, shifts 0x1e to 0). So the array holds the three entries DD1C, DD20 and DD24 that LoadDecorateIcons fills; the next slot DD28 is the separate icon pointer gDisabledMemberIcon */
extern char far *gDisabledMemberIcon; /*@ ds:DD28 */ /* the icon table src/EOV1.ASM:87 draws member icons from, filled by LoadDecorateIcons at the slot after gFlashFrames */
extern void far *gDeadPortraitIcon; /*@ ds:F2F4 */
extern void far *gUnconsciousIcon; /*@ ds:DC94 */
extern void far *gHandShadeIcon; /*@ ds:8A7E */
extern void far *gCompassIcons[]; /*@ ds:A92A */
extern int gItemCount; /*@ ds:E066 */
extern int gItemNameCount; /*@ ds:F516 */
extern unsigned char far gItemNames[]; /*@ 1F28:0000 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern void far *gItemIcons[]; /*@ ds:DDE4 */
extern void far *gDefaultPointerIcon; /*@ ds:DD00 */ /* the mouse pointer image: gItemIcons[0], set by SetPointerShape (src/EOV2.C:214) and given to SetMousePointerShape (src/E1BB1.ASM:12) */
extern void far *gRemappedIcon; /*@ ds:DF48 */
extern void far *gStagingPage; /*@ ds:DF4E */
extern char far *gItemRemapTable; /*@ ds:F534 */
extern void far *gCurFont; /*@ ds:FE4E */
extern void far *gFont8x8; /*@ ds:FE5A */
extern int gTextHeight; /*@ ds:FE60 */
extern int gTextWidth; /*@ ds:FE62 */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern char far *gFont6x6; /*@ ds:A9C0 */
extern char gPanelFillColour; /*@ ds:F57E */ /* a UI colour (src/E0D1C.C:46), set by SetUiPalette */
extern char gPanelHighlight; /*@ ds:FB4C */ /* a UI colour (src/E0D1C.C:47), set by SetUiPalette */
extern char gPanelShadow; /*@ ds:F53A */ /* a UI colour (src/E0D1C.C:48), set by SetUiPalette */
extern char gUnusedPanelColour; /*@ ds:F53B */ /* a UI colour set by SetUiPalette; no reader in src/ yet */
extern char gBarEmptyColour; /*@ ds:FB7A */ /* a colour DrawRatioBar is passed (src/E1547.C:279), set by SetUiPalette */
extern char gAltPanelFill; /*@ ds:FB7B */ /* a colour DrawShadowText is passed (src/E1547.C:529), set by SetUiPalette */
extern char gUnselTabLightColour; /*@ ds:A9C4 */ /* a panel colour (src/E0EF3.C:1911), set by SetUiPalette */
extern char gUnselTabDarkColour; /*@ ds:A928 */ /* a panel colour (src/E0EF3.C:1912), set by SetUiPalette */
extern char gFailLabelLight; /*@ ds:A95A */ /* the top and right edge colour of the action label panel (src/E1547.C:412), set by SetUiPalette */
extern char gGameLoadState; /*@ ds:A998 */ /* nonzero tells EnterMapScreen (src/E0ACD.C:911) to show the inventory and the loaded map; the saved-game panel writes it in LoadGamePanel */
extern char gFailLabelShadow; /*@ ds:8A6C */ /* the left and bottom edge colour of the action label panel (src/E1547.C:412), set by SetUiPalette */
extern char gFailLabelFill; /*@ ds:8A7D */ /* the fill colour of the action label panel (src/E1547.C:412), set by SetUiPalette */
extern char gIconTable; /*@ ds:FB34 */ /* this unit, src/E0ACD.C:48, src/E1414.C:106 and src/E1547.C:128 read it as the party member being moved (-1 when none); the BlinkSelected comment (1414:0000, src/E1414.C:141-145) saves and restores it as that member. src/E0EF3.C:227 declares ds:FB34 as an array of far sprite pointers (a cross-unit conflict to settle) */
extern int gUnusedStartupWord; /*@ ds:EFC8 */ /* a word cleared by InitGameTables; no other reader in src/ yet */
extern int gUnreadInitWord; /*@ ds:EFCA */ /* a word cleared by InitGameTables; no other reader in src/ yet */
extern void far *gLoadedMonsterShapes[]; /*@ ds:B802 */ /* 36 far monster shape pointers (src/EOV3.C:146, freed by ovl3:05D2), cleared by InitGameTables */
extern long gDoorShapes[]; /*@ ds:FB5E */ /* six size-record pointers (src/EOV3.C:138 declares them longs; src/EOV4.C:756 declares them struct SlotSize far *), cleared by InitGameTables */
extern struct SlotPos gDoorButtons[]; /*@ ds:A99C */
extern void far *gDecorShapes[]; /*@ ds:EFD4 */ /* 200 far size-record pointers (src/EOV3.C:14, freed at :230-241; src/E122E.C:98 declares them struct RectKind far *), cleared by InitGameTables */
extern char gSelectFrameColours[]; /*@ ds:1CE7 */
extern char gBoxCycleColours16[]; /*@ ds:65EA */
extern char gCgaMapDefault[]; /*@ ds:113F */
extern char far *gOptionNames[]; /*@ ds:65DE */

/* the 32-byte colour maps (values 0..3) of the item and PLAYFLD art, one TLINK data segment each */
extern unsigned char far gCgaMapPlayfld[]; /*@ 2069:0000 */
extern unsigned char far gCgaMapItemL1[]; /*@ 20A3:0000 */
extern unsigned char far gCgaMapItemS1[]; /*@ 20A5:0000 */
extern unsigned char far gCgaMapThrown[]; /*@ 20A7:0000 */
extern unsigned char far gCgaMapItemIcn[]; /*@ 20A9:0000 */

/* graphics driver export 16: returns a new block holding the cell (x, y, w, h) cut from the page
   after the draw page: x and w in 8-pixel columns, y and h in lines (CellSource and CellSize,
   src/drivers/MCGA.ASM:1485-1510); the cells differ in size, the ITEMICN item icons are 2 by 0x10
   and the item and decoration cells use widths 3 to 5 and heights 8 to 0x20. `cgaMap` is the art's
   32-byte CGA colour map, which only the CGA driver reads (src/drivers/CGA.ASM:1666-1671). */
extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *cgaMap); /*@ ds:8086 */
extern int (far *gGetPixel)(int x, int y); /*@ ds:8072 */ /* graphics driver export 11: the colour of the draw page's pixel at (x, y), 0 off the screen (src/drivers/MCGA.ASM:1132-1165) */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int colour); /*@ ds:806A */ /* the rectangle fill hook (src/E082C.C:64) */
extern void (far *gSaveRectFromPage)(int x, int y, int w, int h, void far *buf); /*@ ds:807E */
extern void (far *gRestoreRectToPage)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern void LoadScreenImage(char far *name, char far *buf, int a, int b, int c); /*@ 082C:051B */
extern void DrawFilledPanel(int a, int b, int c, int d, int e, int f, int g); /*@ 082C:09D5 */
extern int EditTextField(char far *buf, char a, char b, char c, char d, char e, char f); /*@ 082C:000B */
extern void DrawShadowText(char far *text, int a, int b, int c, int d); /*@ 082C:0898 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void ClearTextRegion(void); /*@ 1DE7:0344 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void SetFont(char far *font); /*@ 1A47:0169 */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void DrawOptionList(int panel, char far * far *names, long mask, int shift, int spacing); /*@ 14EF:0154 */
extern int UpdateOptionList(int panel, char far * far *names, long counter, long mask, int shift); /*@ 14EF:0274 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void SetMousePointerShape(int hotX, int hotY, void far *icon); /*@ 1BB1:0002 */
extern void WaitGameKey(void); /*@ 1AAD:053A */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern char far *SkipStrings(char far *p, int n); /*@ 0D1C:07A7 */
extern char far *strcpy(char far *dst, char far *src); /*@ 0000:67CD */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern int strcmp(char far *a, char far *b); /*@ 0000:679D */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern void exit(int status); /*@ 0000:50F3 */
extern int strlwr(char far *s); /*@ 0000:6815 */ /* runtime-library strlwr: lowercases the far string
   in place ('A' to 'Z' get +0x20 and are stored back, 0000:6822-682C) and returns the pointer in
   dx:ax (0000:6815 decode: push bp, mov bp,sp, push si, cld, lds si,[bp+6], the loop, then
   0000:6832-6838: xchg dx,ax, mov dx,ds, pop ds, pop si, pop bp, retf) */
extern int LoadGame(void); /*@ 210C:0039 */ /* overlay 3 entry (its target is ovl3:15B9, named LoadGame by src/EOV3.C:863): returns nonzero when a saved game was loaded, otherwise the caller prints "Unable to load your saved game!" */
extern void RunCharacterPanel(void); /*@ 20F6:0052 */ /* overlay 0 entry (its target is ovl0:28F9, the character panel's main loop named by src/EOV0.C:1447-1455) */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */

/* ovl2:0000: loads the ITEMICN image, cuts one 2 by 0x10 cell out of it through gGetSpriteCell
   (gCgaMapDefault as the colour map) and makes the result (gItemIcons[0] and gDefaultPointerIcon) the mouse pointer shape at
   (0,0), then sets the draw page back to 0. Called first by LoadItemData. */
/*@ ovl2:0000 */ void SetPointerShape(void)
{
    LoadOverlayScreen("ITEMICN", 5, 3);
    gItemIcons[0] = gGetSpriteCell(0, 0, 2, 0x10, gCgaMapDefault);
    gDefaultPointerIcon = gItemIcons[0];
    SetMousePointerShape(0, 0, gDefaultPointerIcon);
    gDrawPage = 0;
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
   gThrownIcons, gItemIcons) and the five single THROWN cells, allocates the 0x12C-byte remapped
   icon image shown as the mouse pointer (gRemappedIcon), then reads ITEMRMP.VGA (gGraphicsChoice == 3) or ITEMRMP.EGA into gItemRemapTable.
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
                gCgaMapItemL1);

    LoadOverlayScreen("ITEMS1", 5, 3);
    for (i = 0; i < 0x17; i++)
        for (j = 0; j < 4; j++)
            gItemS1Icons[i][j] = gGetSpriteCell(gCellPosS1[i].x,
                gCellPosS1[i].y + gCellDX[j], gCellSize2[j].w, gCellSize2[j].h,
                gCgaMapItemS1);

    LoadOverlayScreen("THROWN", 5, 3);
    for (i = 0; i < 0xc; i++)
        for (j = 0; j < 4; j++)
            gThrownIcons[i][j] = gGetSpriteCell(gCellPosS1[i].x,
                gCellPosS1[i].y + gCellDX[j], gCellSize2[j].w, gCellSize2[j].h,
                gCgaMapThrown);

    gThrownIcon0 = gGetSpriteCell(0x10, 0, 4, 0x18, gCgaMapThrown);
    gThrownIcon1 = gGetSpriteCell(0x10, 0x18, 4, 0x18, gCgaMapThrown);
    gThrownIcon2 = gGetSpriteCell(0x10, 0x30, 3, 0x18, gCgaMapThrown);
    gThrownIcon3 = gGetSpriteCell(0x10, 0x90, 5, 0x18, gCgaMapThrown);
    gThrownIcon4 = gGetSpriteCell(0x10, 0xa8, 5, 0x10, gCgaMapThrown);

    LoadOverlayScreen("ITEMICN", 5, 3);
    for (i = 1; i < 0x59; i++)
        gItemIcons[i] = gGetSpriteCell((i % 20) * 2, (i / 20) * 16, 2, 0x10, gCgaMapItemIcn);

    gRemappedIcon = MemAlloc(0x12cL, 0);
    gDrawPage = 0;
    sprintf(gFileNameBuf, "ITEMRMP.%s", gGraphicsChoice == 3 ? "VGA" : "EGA");
    handle = FileOpen(gFileNameBuf, 1);
    FileRead(handle, gItemRemapTable, 0x640L);
    FileClose(handle);
}

/* ovl2:0496: resets the tables a new game starts from and allocates one 0x514-byte scratch buffer
   for each of the six character records: sets gIconTable (ds:FB34, the party member being moved,
   -1 when none; src/E0ACD.C:42) to -1, clears the two words gUnusedStartupWord and gUnreadInitWord, the 36 far pointers
   of gLoadedMonsterShapes (src/EOV3.C:146), the six longs of gDoorShapes (src/EOV3.C:138), the far size-record pointer
   of each of the six gDoorButtons records (src/E122E.C:112-117) and the 200 far pointers of gDecorShapes
   (src/E122E.C:98, freed by src/EOV3.C:230-241), then writes each character record's index and
   allocates its buffer (gChars[i].buf). Called by LoadItemData. */
/*@ ovl2:0496 */ void InitGameTables(void)
{
    int i;

    gIconTable = 0xff;
    gUnusedStartupWord = 0;
    gUnreadInitWord = 0;
    for (i = 0; i < 0x24; i++)
        gLoadedMonsterShapes[i] = 0;
    for (i = 0; i < 6; i++) {
        gDoorShapes[i] = 0;
        gDoorButtons[i].rec = 0;
    }
    for (i = 0; i < 0xc8; i++)
        gDecorShapes[i] = 0;
    for (i = 0; i < 6; i++) {
        gChars[i].index = i;
        gChars[i].portrait = MemAlloc(0x514L, 0);
    }
}

/* ovl2:0577: the overlay-8 entry (reached as 2106:0048), leads to the ITEMRMP read. First calls
   LoadItemSheets, then loads the DECORATE image into page 3 and cuts the creature/decoration
   icons out of it: six cells described by gDecorateCellRects (colour map gCgaMapDefault) into
   gTeleporterCells, the three flash frames gFlashFrames[0..2], three more single cells
   (gDeadPortraitIcon, gUnconsciousIcon, gHandShadeIcon), the icon pointer gDisabledMemberIcon, and the 3x4 gCompassIcons whose
   cells are 3 wide with y from gCompassRowTops and height from gCompassRowHeights. Called by StartupAlloc (07A9:000A). */
/*@ ovl2:0577 */ void LoadDecorateIcons(void)
{
    char i;
    char j;

    LoadItemSheets();
    LoadOverlayScreen("DECORATE", 5, 3);
    for (i = 0; i < 6; i++)
        gTeleporterCells[i] = gGetSpriteCell(gDecorateCellRects[i].x, gDecorateCellRects[i].y,
            gDecorateCellRects[i].w, gDecorateCellRects[i].h, gCgaMapDefault);
    gFlashFrames[0] = gGetSpriteCell(0x1d, 0, 2, 0x10, gCgaMapPlayfld);
    gFlashFrames[1] = gGetSpriteCell(0x1f, 0, 2, 0x10, gCgaMapPlayfld);
    gFlashFrames[2] = gGetSpriteCell(0x21, 0, 2, 0x10, gCgaMapPlayfld);
    gDeadPortraitIcon = gGetSpriteCell(0, 0x58, 4, 0x20, gCgaMapPlayfld);
    gUnconsciousIcon = gGetSpriteCell(4, 0x58, 4, 0x20, gCgaMapPlayfld);
    gHandShadeIcon = gGetSpriteCell(8, 0x58, 4, 0x10, gCgaMapPlayfld);
    gDisabledMemberIcon = gGetSpriteCell(9, 0x58, 2, 8, gCgaMapPlayfld);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 4; j++)
            gCompassIcons[i * 4 + j] = gGetSpriteCell(j * 3, gCompassRowTops[i], 3, gCompassRowHeights[i], gCgaMapPlayfld);
    gDrawPage = 0;
}

/* ovl2:078D: draws the INTRO screen for the "load a saved game" panel: loads the INTRO page into
   page 3 and copies it to page 2 (LoadScreenImage, mode 2, gCgaMapDefault as the colour map),
   draws to page 2 (gDrawPage), reads the colour of the pixel at (0, 0) with gGetPixel and fills
   the rectangle (0, 0x9f)-(0x13f, 0xc7) with that colour (gFillRect, src/E082C.C:64),
   draws the two panel frames, clips a 40 by 200 region at (0, 0) (0x28 and 0xc8
   are the width and height, the same rectangle src/E082C.C:317 makes) and sets the draw page back to 0.
   It cuts no sprite cells. Called by LoadItemData. */
/*@ ovl2:078D */ void LoadIntroScreen(void)
{
    int handle;

    LoadScreenImage("INTRO", gCgaMapDefault, 5, 3, 2);
    gDrawPage = 2;
    handle = gGetPixel(0, 0);
    gFillRect(0, 0x9f, 0x13f, 0xc7, handle);
    DrawFilledPanel(0x4d, 0xa5, 0xf9, 0xc1, 0xd, 0xe, 0xc);
    DrawFilledPanel(0x4c, 0xa4, 0xfa, 0xc2, 0xd, 0xe, -1);
    CopyRegion(0, 0, 0, 0, 0x28, 0xc8, 2, 0);
    gDrawPage = 0;
}

/* ovl2:0846: the title menu's "load a saved game" panel. Draws the panel (DrawOptionList) and waits
   (UpdateOptionList) until the pointer picks an entry. Entry 0 loads a saved game and marks the
   load in gGameLoadState: 0xff while it runs (src/EOV3.C:937 skips its map reload on that value) and 0xfe
   when it succeeded; both are nonzero, which is how EnterMapScreen (src/E0ACD.C:909-918) later
   shows the loaded map and the inventory. On failure it prints "Unable to load your saved game!",
   clears gGameLoadState and redraws the panel to wait again. Entry 1 sets gGameLoadState to 0, selects the 8x8 font
   (SetFont with gFont8x8, the Font8.fnt loaded at src/E07C8.C:376, src/E1C0E.C:57) and runs
   RunCharacterPanel (ovl0:28F9), then restores the 6x6 font (gFont6x6, the FONT6.FNT loaded at
   src/E07C8.C:377, src/EOV0.C:168, src/EOV3.C:42): the "Start a new party" entry [H]
   (docs/design/WHITEPAPER.md:137). Entry 2 shuts the game down with exit. Called by LoadItemData. */
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
            gGameLoadState = 0xff;
            if (!LoadGame()) {
                ClearTextRegion();
                DrawText("Unable to load your saved game!");
                WaitGameKey();
                ClearTextRegion();
                sel = 3;
                gGameLoadState = 0;
            } else {
                gGameLoadState = 0xfe;
            }
            break;
        case 1:
            SetFont(gFont8x8);
            RunCharacterPanel();
            SetFont(gFont6x6);
            gGameLoadState = 0;
            break;
        case 2:
            ShutdownGame();
            exit(0);
        }
        if (sel == 0 || sel == 1)
            return;
    }
}

/* ovl2:0937: sets the UI palette bytes (ds:F57E, FB4C, F53A, F53B, FB7A, FB7B, A9C4, A928, A95A,
   8A6C, 8A7D) for the VGA modes (gGraphicsChoice 4 or 3) or, for the other modes (0 CGA, 1 Tandy,
   2 EGA; src/E082C.C:66-67), sets different values and copies the 1CE7 table from ds:65EA. Called
   by LoadItemData. */
/*@ ovl2:0937 */ void SetUiPalette(void)
{
    if (gGraphicsChoice == 4 || gGraphicsChoice == 3) {
        gPanelFillColour = 0x84;
        gPanelHighlight = 0x82;
        gPanelShadow = 0x87;
        gUnusedPanelColour = 0x85;
        gBarEmptyColour = 0x85;
        gAltPanelFill = 0xb4;
        gUnselTabLightColour = 0xb1;
        gUnselTabDarkColour = 0xb8;
        gFailLabelLight = 0x11;
        gFailLabelShadow = 0x17;
        gFailLabelFill = 0x14;
    } else {
        gPanelFillColour = 2;
        gPanelHighlight = 9;
        gPanelShadow = 0xd;
        gUnusedPanelColour = 0x85;
        gBarEmptyColour = 2;
        gAltPanelFill = 0xe;
        gUnselTabLightColour = 0xf;
        gUnselTabDarkColour = 0xd;
        gFailLabelLight = 6;
        gFailLabelShadow = 0xd;
        gFailLabelFill = 8;
        strcpy(gSelectFrameColours, gBoxCycleColours16);
    }
}

/* ovl2:09CC, entry 2 (called as 2106:002A): load the item data and build the start-up UI, in order:
   the pointer shape, the game tables, the item and item-type tables, the UI palette, the INTRO
   screen and the "load a saved game" panel. Called by StartupAlloc (07A9:000A). */
/*@ ovl2:09CC */ void LoadItemData(void)
{
    SetPointerShape();
    InitGameTables();
    LoadItems();
    LoadItemTypes();
    SetUiPalette();
    LoadIntroScreen();
    LoadGamePanel();
}

/* ovl2:09ED: the "find the word" dungeon puzzle panel. Saves ds:811F and the font, switches to the
   8x8 font gFont8x8, sets the text colour gRegions[13].textAttr2 from gPanelFillColour, draws the panel frame with two
   DrawFilledPanel calls (082C:09D5) and insets the region by them, picks one of the 30 gWordCells
   records at random (RollDice), draws its item icon and the "On the page with this symbol... Find
   line %d / Enter word %d" text (082C:0898 DrawShadowText), reads the player's word through
   082C:000B EditTextField, strips its trailing spaces, lowercases it through the library strlwr at
   0000:6815 and compares it with the record's message through 0D1C:07A7 ("attacks" and the strings
   after it). Three wrong answers shut the game down; a correct one restores the region, the panel
   and the font. Called by overlay 5 (ovl5:1A82). */
/*@ ovl2:09ED */ void FindWordPanel(void)
{
    void far *font;
    int saveRegion;
    char input[0x1e];
    char text[0x96];
    int pick;
    int tries;

    saveRegion = gTextRegion;
    font = gCurFont;
    SetFont(gFont8x8);
    gRegions[13].textAttr2 = gPanelFillColour;
    gDrawPage = 0;
    SetTextWindow(0xd);
    gSaveRectFromPage(gTextLeft, gTextTop, gTextWidth, gTextHeight, gStagingPage);
    tries = 0;
    ClearTextRegion();
    DrawFilledPanel(gTextLeft * 8, gTextTop, (gTextLeft + gTextWidth) * 8 - 1, gTextTop + gTextHeight - 1,
        gPanelHighlight, gPanelShadow, -1);
    DrawFilledPanel(gTextLeft * 8 + 1, gTextTop + 1, (gTextLeft + gTextWidth) * 8 - 2, gTextTop + gTextHeight - 2,
        gPanelHighlight, gPanelShadow, -1);
    gRegions[gTextRegion].left++;
    gRegions[gTextRegion].top += 2;
    gRegions[gTextRegion].width -= 2;
    gRegions[gTextRegion].height -= 0x10;
    SetTextWindow(0xd);
    for (;;) {
        pick = RollDice(1, 0x1e, -1);
        ClearTextRegion();
        DrawShape(0, gItemL1Icons[gWordCells[pick].row][0], 0x64, 2, 0xd);
        sprintf(text, "\r\r\r\rOn the page with this symbol...\r\rFind line %d\rEnter word %d\r",
            gWordCells[pick].line, gWordCells[pick].word);
        DrawShadowText(text, (gTextLeft + 1) * 8, gTextTop, gTextAttr1, gTextAttr2);
        input[0] = 0;
        DrawFilledPanel(0x4c, 0x64, 0xd0, 0x71, gPanelShadow, gPanelHighlight, -1);
        DrawFilledPanel(0x4d, 0x65, 0xcf, 0x70, gPanelShadow, gPanelHighlight, -1);
        while (EditTextField(input, 0xa, 0x67, 0xf, (char)gTextAttr1, (char)gTextAttr2, 8) < 1)
            ;
        while (input[strlen(input) - 1] == ' ')
            input[strlen(input) - 1] = 0;
        strlwr(input);
        if (strcmp(SkipStrings("attacks", pick), input) == 0)
            break;
        tries++;
        if (tries == 3) {
            ShutdownGame();
            exit(0);
        }
    }
    gRegions[gTextRegion].left--;
    gRegions[gTextRegion].top -= 2;
    gRegions[gTextRegion].width += 2;
    gRegions[gTextRegion].height += 0x10;
    SetTextWindow(0xd);
    gRestoreRectToPage(gTextLeft, gTextTop, gTextWidth, gTextHeight, gStagingPage);
    SetTextWindow(saveRegion);
    SetFont(font);
}
