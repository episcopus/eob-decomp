/* Unit at overlay 5 of EOB.EXE (8094 bytes, 31 functions): the character/item overlay reached
   from CallOverlayOp (122E:1D76) and the overlay's stub entry. Its entry ovl5:1A82 dispatches a
   menu (modes 0..7) to the eight panels 0924/0B58/0EB9/0F77/102C/1087/10A6/119E, and modes 8, 9
   and 0xa to the two long animations 1204/17C8 and the "find the word" panel (ovl2:09ED). It
   reads the six 243-byte (0xF3) character records at ds:F580 (struct CharRec), draws its menus
   through 0455 and the ds:7B89/7B94 position tables, and its 160A prints the winning-party
   password. Names are placeholders until evidence names them. */

/*@flags -ml -O -Z -v */

#include <dos.h>        /* FP_OFF/FP_SEG, the blit calls split a far pointer */

struct Cell4 {              /* a four-byte sprite-cell definition, x, y, w, h (src/EOV2.C) */
    unsigned char x;
    unsigned char y;
    unsigned char w;
    unsigned char h;
};

struct Cell {               /* the header gGetSpriteCell returns; bytes +1 and +2 are read here */
    char b0;                /* +0 */
    signed char b1;         /* +1: a vertical size the caller subtracts */
    signed char b2;         /* +2: a horizontal size, times four */
};

struct CharRec {            /* 243-byte (0xF3) character records at ds:F580 (src/EOV6.C) */
    char index;             /* +00 */
    unsigned char status;   /* +01: status bits (only bit 0 is read here) */
    char name[0x0B];        /* +02: the character's name, NUL padded */
    char pad0D[0x1B - 0x0D];
    signed char hp;         /* +1B: current hit points */
    signed char maxHp;      /* +1C: maximum hit points */
    char pad1D[0x22 - 0x1D];
    unsigned char m22;      /* +22: compared with 0xf7 by 0AB1 (0D1C:0415 uses it too) */
    char pad23[0x33 - 0x23];
    void far *portrait;     /* +33: a far pointer to the character's picture buffer, preserved by
                                   the record swap at 070E (the movmem destination there) */
    char pad37[0x77 - 0x37];
    int items[25];          /* +77: 25 item-slot words (the carried and equipped items) */
    char padA9[0xE3 - 0xA9];
    unsigned long effectFlags;      /* +E3: effect flags (0C9D clears bit 7 of the low word) */
    char padE7[0xF3 - 0xE7];
};

struct PoolNameRec {        /* a view of one 243-byte pre-made character record from its name field:
                               the base gCharPool[-n-1].name folds to (gCharPool + 2 - 0xF3 = ds:71A7) */
    char name[0x0B];        /* +0: the name bytes that gCharPool's name field (+2) holds; as an array,
                               `.name` is its address (a char far pointer), as in struct CharRec */
    char rest[0xE8];        /* +0x0B: the rest of the record, not read here */
};

struct Entry14 {            /* the 14-byte item records at ds:8DD2 (src/E0D1C.C) */
    char pad00;
    signed char icon;       /* +1 */
    signed char type;        /* +2: the item type */
    char pad03;
    int key;                /* +4 */
    char pad06[0x0B - 0x06];
    signed char value;        /* +0xB */
    char pad0C[0x0E - 0x0C];
};

struct ItemInfo {           /* 16-byte records at ds:A9F5 (src/E0D1C.C) */
    signed char m00;
    signed char m01;
    char pad02[0x09 - 0x02];
    int m09;                /* +9 */
    char pad0B[0x10 - 0x0B];
};

struct Monster {              /* 28-byte object records at ds:8A82 (src/E08E6.C) */
    char pad00[8];
    signed char type;       /* +8 */
    char pad09[0x1C - 0x09];
};

extern int gStatusLine; /*@ ds:8115 */
extern signed char gF57E; /*@ ds:F57E */
extern signed char gF53A; /*@ ds:F53A */
extern signed char gFB4C; /*@ ds:FB4C */
extern char gFontHeight; /*@ ds:8101 */
extern int gTextRow; /*@ ds:FE54 */
extern int gTextTop; /*@ ds:FE66 */
extern int gCursorX; /*@ ds:8138 */
extern int gCursorY; /*@ ds:813A */
extern int gClickX; /*@ ds:8140 */
extern int gClickY; /*@ ds:8142 */
extern int gMouseDisabled; /*@ ds:814A */
extern int gHideDepth; /*@ ds:8146 */
extern int gEFC6; /*@ ds:EFC6 */
extern int gFE42; /*@ ds:FE42 */
extern char gScrollOn; /*@ ds:8110 */
extern unsigned long gFC04; /*@ ds:FC04 */
extern int gPartyDir; /*@ ds:FBCC */
extern int gPartyCell; /*@ ds:FBCA */
extern signed char g1A1B[]; /*@ ds:1A1B */
extern char gMazeItems[]; /*@ ds:B899 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct CharRec gCharPool[]; /*@ ds:7298 */  /* the pre-made character records 070E copies into gChars */
extern struct PoolNameRec gPoolNameBase[]; /*@ ds:71A7 */  /* ds:71A7 = gCharPool + 2 - 0xF3, the base of the
                                                              folded gCharPool[-n-1].name access */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern struct ItemInfo gA9F5[]; /*@ ds:A9F5 */
extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern int gCountDDE0; /*@ ds:DDE0 */
extern int gTableF4E6[]; /*@ ds:F4E6 */
extern char gOverlayBuf[]; /*@ ds:113F */
extern char far *gSegDF4E; /*@ ds:DF4E */
extern unsigned char gCellDef7B2B[]; /*@ ds:7B2B */
extern unsigned char gTable7B89[]; /*@ ds:7B89 */
extern char gTable7B94[]; /*@ ds:7B94 */
extern char far *gText7B23[]; /*@ ds:7B23 */  /* the two-entry "yes"/"no" menu table */
extern char gText7B9F[]; /*@ ds:7B9F */
extern char gText7BBA[]; /*@ ds:7BBA */
extern char gText7BC2[]; /*@ ds:7BC2 */
extern char gText7CB7[]; /*@ ds:7CB7 */
extern char gText7CBC[]; /*@ ds:7CBC */
extern char gText7D0E[]; /*@ ds:7D0E */
extern char gText7D77[]; /*@ ds:7D77 */
extern char gText7D9E[]; /*@ ds:7D9E */
extern char gFileNameText[]; /*@ ds:7E53 */
extern char gText7E5C[]; /*@ ds:7E5C */
extern char gText7EDB[]; /*@ ds:7EDB */
extern char gText7EDD[]; /*@ ds:7EDD */
extern char gFileNameXDeath2[]; /*@ ds:7EEF */
extern char gFileNameXDeath3[]; /*@ ds:7EF7 */
extern char gFileNameXDeath1[]; /*@ ds:7EFF */
extern char gFileNamePortalA[]; /*@ ds:7E43 */
extern char gFileNamePortalB[]; /*@ ds:7E4B */
extern char gText7C32[]; /*@ ds:7C32 */
extern char gText7BA6[]; /*@ ds:7BA6 */
extern char gText7BB2[]; /*@ ds:7BB2 */
extern char gText7BCA[]; /*@ ds:7BCA */
extern char gText7BD2[]; /*@ ds:7BD2 */
extern char gText7BDE[]; /*@ ds:7BDE */
extern char gText7BE6[]; /*@ ds:7BE6 */
extern char gText7BEE[]; /*@ ds:7BEE */
extern char gText7BF2[]; /*@ ds:7BF2 */
extern char gText7BFA[]; /*@ ds:7BFA */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern void far *gCurFont; /*@ ds:FE4E */
extern void far *gFont6x6; /*@ ds:A9C0 */
extern char gPanelMode; /*@ ds:FB44 */
extern char gFlag; /*@ ds:F3B9 */
extern char gFileNameOuttake[]; /*@ ds:7F07 */
extern char gFileNameText2[]; /*@ ds:7F0F */
extern char gFileNamePlayfld[]; /*@ ds:7F18 */
extern char gFileNameInvent[]; /*@ ds:7F20 */
extern char gFileNameText3[]; /*@ ds:7F27 */
extern char gFileNamePlayfld2[]; /*@ ds:7F30 */
extern char gFileNameInvent2[]; /*@ ds:7F38 */
extern int far *gBufDCFC; /*@ ds:DCFC */
extern int g169; /*@ ds:0169 */
extern int (far *gFn8072)(int x, int y); /*@ ds:8072 */
extern void (far *gFn809E)(int x, int y, int handle); /*@ ds:809E */
extern int RandomRange(int low, int high); /*@ 1AAA:0005 */
extern char gF542[]; /*@ ds:F542 */
extern int gScrollHook; /*@ ds:8117 */
extern int g8119; /*@ ds:8119 */
extern int g139; /*@ ds:0139 */
extern int gFBC8; /*@ ds:FBC8 */
extern signed char far gMapTiles[]; /*@ 1F1D:0000 */  /* the level's tile-index table (src/EOV3.C) */
extern unsigned char far gMapSheet[]; /*@ 1F1E:0000 */  /* the sheet gMapTiles indexes into (src/EOV3.C's MAPSHEET) */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */  /* the PLAYFLD sheet (src/E0ACD.C, src/EOV3.C) */
extern unsigned char far gSheet2067[]; /*@ 2067:0000 */  /* the INVENT sheet (src/E0ACD.C, src/EOV3.C) */

extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
extern void far * (far *gFn8082)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int flag); /*@ ds:80CA */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */
extern void (far *gFn807E)(int x, int y, int w, int h, void far *buf); /*@ ds:807E */

extern void DrawShape(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void ClearTextRegion(void); /*@ 1DE7:0344 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern int IsPointInsideRect(int x, int y, int left, int top, int right, int bottom); /*@ 082C:0613 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern int WaitKeyEvent(void); /*@ 1AAD:05D3 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern void farfree(void far *p); /*@ 0000:71EE */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int RemoveCarriedItems(int field2, int fieldB); /*@ 0D1C:01CA */
extern void f0D1C_035A(int who); /*@ 0D1C:035A */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void ClearCharEffectSlot(int who, int what); /*@ 1414:0C2D */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern void Action01E3(char far *text); /*@ 0ACD:01E3 */
extern void RedrawPartyPanel(char far *text); /*@ 0EF3:2541 */
extern void fOvl2_09ED(void); /*@ 2106:0039 */  /* overlay 2 entry, ovl2:09ED */
extern void fOvl3_07A5(char far *name, char far *sheet); /*@ 210C:0052 */  /* overlay 3 entry, ovl3:07A5 */
extern void FreePtrTable(void); /*@ 210C:0048 */  /* overlay 3 entry, ovl3:0000 */
extern void FreeEffectTables(void); /*@ 210C:003E */  /* overlay 3 entry, ovl3:031A */
extern void FreeMapIcons(int base, int count); /*@ 210C:0061 */  /* overlay 3 entry, ovl3:05D2 */
extern void DrawViewFrame(int n); /*@ 2116:005C */  /* overlay 4 entry, ovl4:1F51 */
extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern void LoadScreenImage(char far *name, void far *buf, int a, int b, int c); /*@ 082C:051B */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void SetFont(void far *font); /*@ 1A47:0169 */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void f19EC_002A(int a, int b, int c, int d); /*@ 19EC:002A */
extern int gF1; /*@ ds:00F1 */

/* siblings not decompiled yet: their real signatures, so callers compile */
int SelectMenu(char first, char count, char far **names);
void Ovl5_0924(void);
void Ovl5_0BBE(void);
void Ovl5_0C9D(void);
void ResurrectPicker(void);
void Ovl5_0EB9(void);
void Ovl5_0F77(void);
void Ovl5_10A6(void);
void Ovl5_119E(void);
void TileAnimation(void);
void EndingAnimation(void);
void Ovl5_1A82(int mode);
void ShuffleGrid(int x, int y, unsigned cols, unsigned rows, int saveStatus, int restoreStatus);

void PrintTableString(int index, int flag);

/* ovl5:0000: draws sprite cell `index` of the ds:7B2B table out of the overlay buffer gOverlayBuf
   at (x, y), moved left by the returned cell's byte +2 (times four) and up by its byte +1, into
   region 5. Sets gStatusLine for the draw and frees the sliced cell. Called by DrawCharPortrait
   (00A1). */
/*@ ovl5:0000 */ void DrawSheetCell(int index, int x, int y)
{
    void far *cell;
    int cx;                 /* the cell's source x in gOverlayBuf, ds:7B2B[index*4 + 0] */
    int cy;                 /* the cell's source y, ds:7B2B[index*4 + 1] */
    int cw;                 /* the cell's width, ds:7B2B[index*4 + 2] */
    int ch;                 /* the cell's height, ds:7B2B[index*4 + 3] */
    int i;

    gStatusLine = 2;
    i = index * 4;
    cx = gCellDef7B2B[i];
    cy = gCellDef7B2B[i + 1];
    cw = gCellDef7B2B[i + 2];
    ch = gCellDef7B2B[i + 3];
    cell = gGetSpriteCell(cx, cy, cw, ch, gOverlayBuf);
    gStatusLine = 0;
    y -= ((struct Cell far *)cell)->b1;
    x -= (((struct Cell far *)cell)->b2 << 3) >> 1;
    DrawShape(0, cell, x, y, 5);
    farfree(cell);
}

/* ovl5:00A1: draws the character portrait for the current panel. It clips the portrait region
   (0,0)-(0x16,0x78) and copies three six-entry, per-party-member tables into locals: two cell
   index tables (ds:7B77 and ds:7B7D) and a y-offset table (ds:7B83). Through DrawSheetCell it
   draws the fixed cells 2 (mode 0), 0 and 5 (mode 1), 3 (mode 2), 7 (mode 3), 6 (mode 4), 0x12
   (mode 5), 0x11 (mode 6) and 4 (mode 7); for mode 2 with a selected member (gEFC6 != -1) it
   also draws that member's cellIdxA and cellIdxB cells at x 0x74, the cellIdxB cell at the
   ds:7B83 y offset. Called by the overlay dispatcher 1A82 for modes 0..7 and by the resurrect
   picker 0DBB with mode 2. */
/*@ ovl5:00A1 */ void DrawCharPortrait(int mode)
{
    char cellIdxA[6] = { 9, 8, 8, 9, 9, 0x0A };              /* ds:7B77 */
    char cellIdxB[6] = { 0x0D, 0x0B, 0x0C, 0x0E, 0x0F, 0x10 };  /* ds:7B7D */
    char yOff[6] = { 0x34, 0x31, 0x31, 0x34, 0x34, 0x45 };   /* ds:7B83 */

    ClipRect(0, 0, 0, 0, 0x16, 0x78, 6, 0);
    switch (mode) {
    case 0:
        DrawSheetCell(2, 0x58, 0x68);
        break;
    case 1:
        if (gEFC6 == -1) {
            DrawSheetCell(0, 0x58, 0x68);
        } else {
            DrawSheetCell(0, 0x3C, 0x68);
            DrawSheetCell(5, 0x74, 0x68);
        }
        break;
    case 2:
        if (gEFC6 == -1) {
            DrawSheetCell(3, 0x58, 0x68);
        } else {
            DrawSheetCell(3, 0x3C, 0x68);
            DrawSheetCell(cellIdxA[gEFC6], 0x74, 0x68);
            DrawSheetCell(cellIdxB[gEFC6], 0x74, yOff[gEFC6]);
        }
        break;
    case 3:
        DrawSheetCell(7, 0x58, 0x68);
        break;
    case 4:
        DrawSheetCell(6, 0x58, 0x68);
        break;
    case 5:
        DrawSheetCell(0x12, 0x58, 0x58);
        break;
    case 6:
        DrawSheetCell(0x11, 0x58, 0x68);
        break;
    case 7:
        DrawSheetCell(4, 0x58, 0x68);
        break;
    }
}

/* The menu x-position table ds:7B89, defined here because the original lays it out right after
   00A1's three array initialisers (the matcher compares that block by content); 0455 reads it. */
unsigned char gTable7B89[] = { 0x3B, 0xA6, 0x04, 0x70, 0xDC, 0x04, 0x70, 0xDC, 0x04, 0x70, 0xDC }; /*@ ds:7B89 */

/* ovl5:021C: clears the character panel (1, 0x7a)-(0x13e, 0xc7): hides the pointer, clears the
   text region, fills the rectangle with gF57E and shows the pointer again. Called before every
   panel of the overlay draws itself. */
/*@ ovl5:021C */ void ClearPanel(void)
{
    HidePointerZone(1, 0x7a, 0x13e, 0xc7);
    ClearTextRegion();
    gFillRect(1, 0x7a, 0x13e, 0xc7, gF57E);
    ShowPointerZone();
}

/* the key codes these panels accept; SelectMenu's switch table at ovl5:0657 lists them. The mask,
   the three confirm keys, the two mouse-button keys and up/down are the same game keys as
   UpdateOptionList's (14EF:0274); 0x5c and 0x66 are SelectMenu's own prev/next keys, and 0x34 is
   a fourth dismiss key of the message box 025F. */
enum {
    KEY_CODE_MASK = 0x8FF,      /* the filter both menu loops apply: bits 0-7 and bit 11 of the key word */
    KEY_CONFIRM   = 0x2B,       /* confirm: dismisses the message box and picks the highlighted row */
    KEY_CONFIRM2  = 0x3D,       /* second confirm code */
    KEY_CONFIRM3  = 0x61,       /* third confirm code */
    KEY_CONFIRM4  = 0x34,       /* a fourth dismiss key of the message box 025F */
    KEY_CLICK     = 0x41,       /* mouse button 0 changed (1AAD:0B0E) */
    KEY_CLICK2    = 0x42,       /* mouse button 1 changed (1AAD:0B0E) */
    KEY_PREV      = 0x5C,       /* SelectMenu: previous row */
    KEY_UP        = 0x60,
    KEY_DOWN      = 0x62,
    KEY_NEXT      = 0x66        /* SelectMenu: next row */
};

/* ovl5:025F: the modal message box. It picks one of two rectangles from gFE42: 0 (CharOverlay)
   uses (0x11D,0xBD)-(0x13B,0xC5), 1 (UseItemPanel, WinnerPanel) uses (0x8B,0xA2)-(0xA9,0xAA). It
   hides the pointer, fills the panel in the three colours, draws the "MORE" prompt ds:7CB7 in
   colour 0xf and shows the pointer. It then waits for a key or a click: KEY_CONFIRM, KEY_CONFIRM4
   and KEY_CONFIRM2 dismiss it, as do KEY_CLICK and KEY_CLICK2 only when the click lies inside the
   box. In the end it restores the pointer lock, refills the rectangle and clears the text region.
   Called by PrintTableString (03A9) after its message. */
/*@ ovl5:025F */ void MessagePanel(void)
{
    int done;
    int redraws;
    int key;
    int right;
    int bottom;
    int left;
    int top;

    if (gFE42) {
        left = 0x8B;
        top = 0xA2;
        right = 0xA9;
        bottom = 0xAA;
    } else {
        left = 0x11D;
        top = 0xBD;
        right = 0x13B;
        bottom = 0xC5;
    }
    HidePointerZone(left, top, right, bottom);
    DrawFilledPanel(left, top, right, bottom, gFB4C, gF53A, gF57E);
    gDrawText(gText7CB7, left + 4, top + 2, 0xF, 0);
    ShowPointerZone();
    FlushKeyBuffer();
    redraws = gHideDepth;
    if (!gMouseDisabled) {
        while (gHideDepth)
            LockShowPointer();
    }
    done = 1;
    do {
        key = WaitKeyEvent() & KEY_CODE_MASK;
        switch (key) {
        case KEY_CLICK:
        case KEY_CLICK2:
            if (!IsPointInsideRect(gClickX, gClickY, left, top, right, bottom))
                break;
            /* fall through */
        case KEY_CONFIRM:
        case KEY_CONFIRM4:
        case KEY_CONFIRM2:
            done = 0;
            break;
        }
    } while (done);
    if (!gMouseDisabled) {
        while (redraws--)
            LockHidePointer();
    }
    LockHidePointer();
    gFillRect(left, top, right, bottom, gF57E);
    ClearTextRegion();
    LockShowPointer();
}
/* ovl5:03A9: draws string `index` (1-based) of the string-offset table pointed at by gSegDF4E
   (the word at index-1 is an offset from that base), then, when `flag` is nonzero, the modal
   message panel 025F. The indexed strings are the overlay's message texts (a TEXT.DAT page read
   into gSegDF4E); the menu labels are the ds:7B tables, not these. */
/*@ ovl5:03A9 */ void PrintTableString(int index, int flag)
{
    char far *offsets;
    int off;

    offsets = gSegDF4E;
    index--;
    off = ((int far *)offsets)[index];
    DrawText(gSegDF4E + off);
    if (flag)
        MessagePanel();
}

/* ovl5:03EF: one row of the overlay's menu: fills a bevelled panel (x, y)-(x+0x5e, y+8) in the
   three panel colours and draws `text` centred in it (three pixels per character, at y+2) in
   `colour`. Called by the grid 0455 for every entry it draws. */
/*@ ovl5:03EF */ void DrawMenuRow(int x, int y, int colour, char far *text)
{
    DrawFilledPanel(x, y, x + 0x5e, y + 8, gFB4C, gF53A, gF57E);
    gDrawText(text, x + 0x30 - strlen(text) * 3, y + 2, colour, 0);
}

/* ovl5:0455: draws and runs one of the overlay's menus: the `count` rows starting at position
   `first` in the ds:7B89/ds:7B94 position tables, labelled with the far strings names[0..count-1].
   Each row is a DrawMenuRow panel; the highlighted row (i) is drawn in colour 9, the others in
   0xf. It redraws only when the highlight moved, then reads a key and also checks the mouse. The
   mouse-button keys 0x41/0x42 confirm the row under the last click (gClickX/gClickY); the confirm keys
   0x2b/0x3d/0x61 confirm the highlighted row; 0x5c/0x66 and 0x60/0x62 move the highlight by one
   or three (a three-row column-major grid, wrapping). Returns the confirmed row index, after
   clearing the panel. Called by the overlay's menu panels ChooseChar (067B), AddCharPanel (0828),
   TemplePanel (0924), GiveItemPanel (09C5), ResurrectPicker (0DBB), ResurrectPanel (0EB9),
   AttackPanel (0F77), KillHerPanel (102C), SurrenderPanel (10A6) and FreeHimPanel (119E).
   Row `n`'s x column is the unsigned byte ds:7B89[n] and its y offset the signed byte
   ds:7B94[n] (the code zero-extends one and sign-extends the other); `row` holds first+j so the
   two lookups can share it. */
/*@ ovl5:0455 */ int SelectMenu(char first, char count, char far **names)
{
    int ybase;
    int sel;
    int key;
    int drawn;
    int j;
    int row;
    int i;

    ybase = gTextTop + (gTextRow + 1) * gFontHeight + 4;
    drawn = sel = -1;
    i = 0;
    do {
        LockHidePointer();
        if (drawn != i) {
            for (j = 0; j < count; j++) {
                row = first + j;
                DrawMenuRow(gTable7B89[row], ybase + gTable7B94[row],
                            i == j ? 9 : 0xf, names[j]);
            }
            drawn = i;
        }
        LockShowPointer();
        key = WaitKeyEvent() & KEY_CODE_MASK;
        for (j = 0; j < count; j++) {
            row = first + j;
            if (IsPointInsideRect(gCursorX, gCursorY, gTable7B89[row],
                                  gTable7B94[row] + ybase,
                                  gTable7B89[row] + 0x5e,
                                  gTable7B94[row] + ybase + 8)) {
                i = j;
                break;
            }
        }
        switch (key) {
        case KEY_CLICK:
        case KEY_CLICK2:
            for (j = 0; j < count; j++) {
                row = first + j;
                if (IsPointInsideRect(gClickX, gClickY, gTable7B89[row],
                                      gTable7B94[row] + ybase,
                                      gTable7B89[row] + 0x5e,
                                      gTable7B94[row] + ybase + 8)) {
                    sel = j;
                    break;
                }
            }
            break;
        case KEY_PREV:
            i--;
            if (i < 0)
                i = count - 1 > 2 ? 2 : count - 1;
            break;
        case KEY_NEXT:
            i++;
            if (i > 2 || count - 1 < i)
                i = 0;
            break;
        case KEY_DOWN:
            if (i + 3 < count)
                i += 3;
            else if (i - 3 > 0)
                i -= 3;
            /* falls through (the original has no break here) */
        case KEY_UP:
            if (i - 3 > 0)
                i -= 3;
            else if (i + 3 < count)
                i += 3;
            break;
        case KEY_CONFIRM:
        case KEY_CONFIRM2:
        case KEY_CONFIRM3:
            sel = i;
            break;
        }
    } while (sel == -1);
    ClearPanel();
    return sel;
}
/* ovl5:067B: the "which character?" selector of the 0828 panel. It fills the far-pointer array
   names[] with the six party members' names (gChars[i].name, ds + i*0xF3 + 2) and appends the
   "CANCEL" string ds:7B9F, at the same time counting how many members are present (flags bit 0).
   When every slot is taken it clears the panel, draws the title ds:7CBC and shows the seven-entry
   menu table through SelectMenu; choosing CANCEL (index 6) returns 0, any other choice is applied
   through f0D1C_035A. A free slot returns 1 at once. Called by 0828. */
/*@ ovl5:067B */ int ChooseChar(void)
{
    char far *names[7];
    int count;
    int i;
    int sel;

    count = i = 0;
    for (; i < 6; i++) {
        count += gChars[i].status & 1;
        names[i] = gChars[i].name;
    }
    names[6] = gText7B9F;
    if (count > 5) {
        ClearPanel();
        DrawText(gText7CBC);
        sel = SelectMenu(2, 7, names);
        if (sel == 6)
            return 0;
        f0D1C_035A(sel);
    }
    return 1;
}
/* ovl5:070E: adds the character `which` of the pool ds:7298 to the party. It finds the first slot
   of gChars whose flags bit 0 is clear, saves that slot's picture pointer portrait, copies the pool
   record over it through movmem, restores portrait, recomputes the armour class, requeues every carried
   item through CopyItemToFreeSlot, then slices the character's sprite cell through gFn8082
   (ds:8082: which*4, 0xa0, 4, 0x20 out of gOverlayBuf) into the picture buffer portrait and frees the
   slice. Called by AddCharPanel. */
/*@ ovl5:070E */ void AddPartyMember(int which)
{
    struct CharRec far *rec;
    void far *saved;   /* the slot's picture pointer, then the sliced sprite cell */
    int slot;
    int i;

    slot = i = 0;
    for (; i < 6; i++) {
        if ((gChars[i].status & 1) == 0) {
            slot = i;
            break;
        }
    }
    rec = &gChars[slot];
    saved = rec->portrait;
    movmem((void far *)&gCharPool[which], (void far *)rec, 0xF3);
    rec->portrait = saved;
    ComputeArmourClass(slot);
    for (i = 0; i < 0x19; i++) {
        if (rec->items[i])
            rec->items[i] = CopyItemToFreeSlot(rec->items[i]);
    }
    gStatusLine = 2;
    saved = gFn8082(which * 4, 0xA0, 4, 0x20, gOverlayBuf);
    movmem(saved, rec->portrait, 0x514);
    farfree(saved);
    gStatusLine = 0;
}
/* ovl5:0828: the character add panel. It clears the panel, draws message `which`, then shows the
   two-entry menu ds:7B23 ("yes"/"no"). Choice 0 ("yes") either draws message `yesMsg` or, when
   `yesMsg` is -1, builds and draws "<name> joins the party." (ds:7D0E) for gCharPool[who] and
   shows the modal message 025F; it then removes the carried items of type 0x21 whose value is
   `who`+1 (RemoveCarriedItems matches type == class and value == fieldB; what value means is not
   established, 0D1C:01CA) and, when ChooseChar finds a free slot, adds the character through 070E.
   Choice 1 ("no") draws message `noMsg`. Called by the panels 0924, 09C5, 0DBB and 119E. */
/*@ ovl5:0828 */ void AddCharPanel(int who, int which, int yesMsg, int noMsg)
{
    int sel;

    ClearPanel();
    PrintTableString(which, 0);
    sel = SelectMenu(0, 2, gText7B23);
    switch (sel) {
    case 0:
        if (yesMsg != -1) {
            PrintTableString(yesMsg, 1);
        } else {
            sprintf(gFileNameBuf, gText7D0E, gCharPool[who].name);
            DrawText(gFileNameBuf);
            MessagePanel();
        }
        RemoveCarriedItems(0x21, who + 1);
        if (ChooseChar())
            AddPartyMember(who);
        break;
    case 1:
        PrintTableString(noMsg, 1);
        break;
    }
}
/* ovl5:08D2: turns item `n` into a carried item (CopyItemToFreeSlot) and queues it on the list
   head gMazeItems keyed by gPartyCell (the party's map cell), with a random sound from the current
   sound page gPartyDir. Called by the panels that add an item to a character. */
/*@ ovl5:08D2 */ void QueueItem(int item)
{
    int sound;

    item = CopyItemToFreeSlot(item);
    sound = g1A1B[gPartyDir * 4 + RollDice(1, 2, -1)];
    InsertListItem(&gMazeItems[gPartyCell * 9], gPartyCell, item, sound);
}

/* ovl5:0924: the temple/NPC panel. Clears the panel, draws message 8 and shows the menu ds:7BB2
   ("Tend his wounds"/"Leave") when gFC04 bit 0x2000 is set, otherwise ds:7BA6 ("Tend his
   wounds"/"Talk"/"Leave"). Choice 0 runs the character panel 0828 and sets bit 0x4000 and returns;
   choice 1 draws message 1, sets bit 0x2000 and repeats; anything else returns. Called from the
   dispatcher 1A82 (mode 0). */
/*@ ovl5:0924 */ void TemplePanel(void)
{
    int sel;

    for (;;) {
        ClearPanel();
        PrintTableString(8, 0);
        if (gFC04 & 0x2000L) {
            sel = SelectMenu(0, 2, (void far *)gText7BB2);
            if (sel == 1)
                sel = 2;
        } else {
            sel = SelectMenu(2, 3, (void far *)gText7BA6);
        }
        switch (sel) {
        case 0:
            AddCharPanel(6, 0xc, 0x17, 2);
            gFC04 |= 0x4000L;
            break;
        case 1:
            PrintTableString(1, 1);
            gFC04 |= 0x2000L;
            continue;
        }
        break;
    }
}

/* ovl5:09C5: the "give an item" panel. On the first pass (gFC04 bit 0x8000 clear) it sets that
   bit and shows the two-entry menu ds:7BBA ("Hear proposal"/"Leave"); choice 0 draws message 4,
   choice 1 skips the second menu (showSecond = 0). Otherwise it draws message 0xd. When the second
   menu is shown (showSecond) it runs ds:7BC2 ("Help him"/"Leave"); choice 0 queues six healing
   items 0x37 and the item 0x3e through QueueItem, sets gFC04 bit 0x10000, draws message 6 and
   runs the character panel 0828(7, 7, 0x1d, 0x1e); choice 1 (or a skipped second menu) draws
   message 5 and returns. Called by the panel dispatcher 0B58. */
/*@ ovl5:09C5 */ void GiveItemPanel(void)
{
    int showSecond;
    int sel;
    int i;

    showSecond = 1;
    if (!(gFC04 & 0x8000L)) {
        gFC04 |= 0x8000L;
        PrintTableString(3, 0);
        sel = SelectMenu(0, 2, (void far *)gText7BBA);
        if (sel == 0)
            PrintTableString(4, 0);
        else if (sel == 1)
            showSecond = 0;
    } else {
        PrintTableString(0xd, 0);
    }
    if (showSecond)
        sel = SelectMenu(0, 2, (void far *)gText7BC2);
    else
        sel = 1;
    if (sel == 0) {
        for (i = 0; i < 6; i++)
            QueueItem(0x37);
        QueueItem(0x3e);
        gFC04 |= 0x10000L;
        PrintTableString(6, 1);
        AddCharPanel(7, 7, 0x1d, 0x1e);
    } else {
        PrintTableString(5, 1);
    }
}
/* ovl5:0AB1: returns 1 when some party member (flags bit 0) has m22 == 0xf7, after drawing
   messages 0x19 and 0x1a and setting the 0x80000 bit of gFC04; otherwise 0. Called by the panel
   dispatcher 0B58. */
/*@ ovl5:0AB1 */ int CheckChar0F7(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (gChars[i].status & 1) {
            if (gChars[i].m22 == 0xf7)
                break;
        }
    }
    if (i == 6)
        return 0;
    PrintTableString(0x19, 1);
    PrintTableString(0x1a, 1);
    gFC04 |= 0x80000L;
    return 1;
}

/* ovl5:0B16: when RemoveCarriedItems(6, -1) removes something, draws message 0x1c, queues
   item 0x20 and sets the 0x100000 bit of gFC04, returning 1; otherwise 0. Called by 0B58. */
/*@ ovl5:0B16 */ int CheckRemove6(void)
{
    if (RemoveCarriedItems(6, -1)) {
        PrintTableString(0x1c, 1);
        QueueItem(0x20);
        gFC04 |= 0x100000L;
        return 1;
    }
    return 0;
}

/* ovl5:0B58: the item-panel dispatcher. When bit 0x10000 of gFC04 is clear it runs the give-item
   panel 09C5, when 0x80000 is clear it runs 0AB1, when 0x100000 is clear it runs 0B16; if none of
   them did anything it draws the farewell line gText7D77 ("I wish you luck in your travels.") and
   the modal message panel 025F. Called from the menu dispatcher 1A82. */
/*@ ovl5:0B58 */ void ItemPanelDispatch(void)
{
    int done;

    done = 0;
    if (!(gFC04 & 0x10000L)) {
        GiveItemPanel();
        done = 1;
    }
    if (!(gFC04 & 0x80000L))
        done |= CheckChar0F7();
    if (!(gFC04 & 0x100000L))
        done |= CheckRemove6();
    if (!done) {
        DrawText(gText7D77);
        MessagePanel();
    }
}

/* ovl5:0BBE: builds the list at ds:F4E6 (count in gCountDDE0) of the characters and items the
   resurrect/heal panel 0EB9 may act on: first every character whose flags bit 0 is set and whose
   hit points are -10 (0xf6, a dead member); then, for every character with flags bit 0 set, each
   of its 25 item slots whose item type's gA9F5[type].m09 low 7 bits equal 8, stored as
   -gTable8DD2[item].value. Returns the count. ResurrectPicker reads a negative entry n back as the
   pool index -(n + 1): that reading is an inference from ResurrectPicker and the
   RemoveCarriedItems(0x21, who + 1) calls; what value itself means is not established (0D1C:01CA).
   Called by 0EB9. */
/*@ ovl5:0BBE */ int BuildDeathList(void)
{
    struct CharRec far *rec;
    int type;
    int kind;
    int item;
    int i;
    int j;

    gCountDDE0 = i = 0;
    for (; i < 6; i++) {
        rec = &gChars[i];
        if ((rec->status & 1) == 0)
            continue;
        if (rec->hp != -10)
            continue;
        gTableF4E6[gCountDDE0++] = i;
    }
    for (i = 0; i < 6; i++) {
        rec = &gChars[i];
        if (rec->status & 1) {
            for (j = 0; j < 0x19; j++) {
                item = rec->items[j];
                if (!item)
                    continue;
                type = gTable8DD2[item].type;
                kind = gA9F5[type].m09 & 0x7f;
                if (kind == 8)
                    gTableF4E6[gCountDDE0++] = -gTable8DD2[item].value;
            }
        }
    }
    return gCountDDE0;
}
/* ovl5:0C9D: heals the party. Rolls 1d3+2 "cures", then for each party member that MemberIs
   accepts (mask 3) clears its flag bits 1..2, clears bit 7 of its effect flags, clears effect slot
   -0x22 and, while it is below its maximum hit points, spends one cure (1d8+9, clamped to the
   maximum) and heals it again. A member that is already at full hit points and one the effect
   cannot be applied to are both skipped with i++, written as two separate branches so the two
   increment tails stay apart (the original emits both). Called by the resurrect panel 0EB9. */
/*@ ovl5:0C9D */ void HealLoop(void)
{
    int i;
    int cures;
    int heal;

    i = 0;
    cures = RollDice(1, 3, 2);
    while (cures && i < 6) {
        if (MemberIs(i, 3)) {
            gChars[i].status &= 0xf9;
            gChars[i].effectFlags &= ~0x80L;
            ClearCharEffectSlot(i, -0x22);
            if (gChars[i].hp < gChars[i].maxHp) {
                cures--;
                heal = RollDice(1, 8, 9);
                if (gChars[i].hp + heal > gChars[i].maxHp)
                    gChars[i].hp = gChars[i].maxHp;
                else
                    gChars[i].hp += heal;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
}
/* ovl5:0DBB: the resurrect panel's "which one?" picker. It clears the panel, draws the title
   ds:7D9E, builds the far string list of the entries BuildDeathList left in gTableF4E6
   (gCountDDE0 of them): a non-negative entry is a party member (gChars[entry].name), a negative
   entry -k-1 is the pre-made character gCharPool[k].name, read through gPoolNameBase at ds:71A7
   (gCharPool ds:7298, its name field +2, one 0xF3 record back). It then runs the 0455 menu (first
   row 2) and acts on the chosen entry: a party member is healed to full (hp = maxHp); a pool
   character has its carried items of type 0x21 with value == k+1 removed, becomes the selected
   character gEFC6, redraws the portrait and runs the add-character panel with the yes message
   suppressed (-1). Called by 0EB9. `names[20]` holds gCountDDE0 entries: the match fixes the
   array at 20 entries, nothing in the code caps gCountDDE0 at 20 (BuildDeathList can add up to
   6 + 6*25 = 156), and whether the original's data can exceed 20 is not established. */
/*@ ovl5:0DBB */ void ResurrectPicker(void)
{
    char far *names[20];
    int sel;
    int i;
    int n;

    ClearPanel();
    DrawText(gText7D9E);
    for (i = 0; i < gCountDDE0; i++) {
        if (gTableF4E6[i] < 0)
            names[i] = gPoolNameBase[-gTableF4E6[i]].name;
        else
            names[i] = gChars[gTableF4E6[i]].name;
    }
    sel = SelectMenu(2, gCountDDE0, names);
    n = gTableF4E6[sel];
    if (n < 0) {
        n = (n < 0 ? -n : n) - 1;
        RemoveCarriedItems(0x21, n + 1);
        gEFC6 = n;
        DrawCharPortrait(2);
        AddCharPanel(n, n * 2 + 0x20, -1, n * 2 + 0x21);
    } else {
        gChars[n].hp = gChars[n].maxHp;
    }
}
/* ovl5:0EB9: the resurrect/heal NPC panel. When gFC04 bit 0x10000 is clear it draws message
   0x18; when bit 0x20000 is set it draws message 0xb; otherwise it draws message 9 and shows the
   menu ds:7BD2 ("Heal Party"/"Resurrect Dead"/"Leave") when BuildDeathList found something, or
   ds:7BCA ("Heal Party"/"Leave") otherwise. Choice 0 runs the heal loop 0C9D and sets gFC04 bit
   0x20000; choice 1 runs the picker 0DBB and sets the bit, but in the two-entry branch choice 1 is
   remapped to 2 and does nothing. Called from the menu dispatcher 1A82 (mode 2). */
/*@ ovl5:0EB9 */ void ResurrectPanel(void)
{
    int sel;

    if (gFC04 & 0x10000L) {
        if (gFC04 & 0x20000L) {
            PrintTableString(0xb, 1);
        } else {
            PrintTableString(9, 0);
            if (BuildDeathList()) {
                sel = SelectMenu(2, 3, (void far *)gText7BD2);
            } else {
                sel = SelectMenu(0, 2, (void far *)gText7BCA);
                if (sel == 1)
                    sel = 2;
            }
            switch (sel) {
            case 0:
                HealLoop();
                gFC04 |= 0x20000L;
                break;
            case 1:
                ResurrectPicker();
                gFC04 |= 0x20000L;
                break;
            }
        }
    } else {
        PrintTableString(0x18, 1);
    }
}
/* ovl5:0F77: the "attack / bribe" NPC panel. Draws message 0x12 and shows the 2-entry menu
   ds:7BDE ("Attack"/"Bribe"); choice 0 sets gFC04 bits 0x8000000 and 0x400000 and changes every
   type-8 object to 5; choice 1 removes the item class from RemoveCarriedItems(0x31, -1) and draws
   message 0x14 and sets 0x400000 when something was removed, otherwise message 0x13. Called from
   the menu dispatcher 1A82 (mode 3). */
/*@ ovl5:0F77 */ void AttackPanel(void)
{
    int sel;
    int i;

    PrintTableString(0x12, 0);
    sel = SelectMenu(0, 2, (void far *)gText7BDE);
    switch (sel) {
    case 0:
        gFC04 |= 0x8000000L;
        gFC04 |= 0x400000L;
        for (i = 0; i < 0x1e; i++) {
            if (gMonsters[i].type == 8)
                gMonsters[i].type = 5;
        }
        break;
    case 1:
        if (RemoveCarriedItems(0x31, -1)) {
            PrintTableString(0x14, 1);
            gFC04 |= 0x400000L;
        } else {
            PrintTableString(0x13, 1);
        }
        break;
    }
}
/* ovl5:102C: the "kill her / hear her out / let her go" NPC panel. Draws message 0xe, shows the
   3-entry menu ds:7BE6; choice 0 sets gFC04 bit 0x200000, choice 1 draws message 0xf. Always sets
   bit 0x800000. Called from the menu dispatcher 1A82 (mode 4). */
/*@ ovl5:102C */ void KillHerPanel(void)
{
    int sel;

    PrintTableString(0xe, 0);
    sel = SelectMenu(2, 3, (void far *)gText7BE6);
    switch (sel) {
    case 0:
        gFC04 |= 0x200000L;
        break;
    case 1:
        PrintTableString(0xf, 1);
        break;
    }
    gFC04 |= 0x800000L;
}
/* ovl5:1087: the quit panel. It has no menu: it draws message 0x15 and sets gFC04 bit 0x1000000.
   Called from the menu dispatcher 1A82 (mode 6). */
/*@ ovl5:1087 */ void QuitPanel(void)
{
    PrintTableString(0x15, 1);
    gFC04 |= 0x1000000L;
}
/* ovl5:10A6: the "surrender / attack" NPC panel. Draws message 0x10 and shows the 2-entry menu
   ds:7BF2 ("Surrender"/"Attack"); choice 0 draws message 0x11 and, for every character's first two
   item slots whose item type gTable8DD2[item].type is outside 0x33..0x38, clears the slot and
   queues the item on the list keyed by gPartyCell, with the sound from gPartyDir. Sets gFC04 bit
   0x2000000. Called from the menu dispatcher 1A82 (mode 5). */
/*@ ovl5:10A6 */ void SurrenderPanel(void)
{
    int sel;
    int item;
    int i;
    int j;

    PrintTableString(0x10, 0);
    sel = SelectMenu(0, 2, (void far *)gText7BF2);
    if (sel == 0) {
        PrintTableString(0x11, 1);
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 2; j++) {
                if (item = gChars[i].items[j]) {
                    if (gTable8DD2[item].type < 0x33 || gTable8DD2[item].type > 0x38) {
                        gChars[i].items[j] = 0;
                        InsertListItem(&gMazeItems[gPartyCell * 9], gPartyCell, item,
                                       g1A1B[gPartyDir * 4 + RollDice(1, 2, -1)]);
                    }
                }
            }
        }
    }
    gFC04 |= 0x2000000L;
}
/* ovl5:119E: the "free him / kill him / leave" NPC panel. Draws message 0x16, shows the 3-entry
   menu ds:7BFA ("Free Him"/"Kill Him"/"Leave"); choice 0 calls 0828(8, 0x1b, 0x2c, 0x2d) and
   choice 1 draws message 0x1f. Choices 0 and 1 set gFC04 bit 0x4000000; the default (choice 2)
   returns before it. Called from the menu dispatcher 1A82 (mode 7). */
/*@ ovl5:119E */ void FreeHimPanel(void)
{
    int sel;

    PrintTableString(0x16, 0);
    sel = SelectMenu(2, 3, (void far *)gText7BFA);
    switch (sel) {
    case 0:
        AddCharPanel(8, 0x1b, 0x2c, 0x2d);
        break;
    case 1:
        PrintTableString(0x1f, 1);
        break;
    default:
        return;
    }
    gFC04 |= 0x4000000L;
}
/* ovl5:1204: the first ending animation (mode 8). It tears down the dungeon resources, loads
   PORTALA ds:7E43 and slices fifteen cells out of gOverlayBuf (three per sprite: a 3x0x4b column, a
   3x0x4b column offset 0x50, and an 0xf x 0x12 cell) plus one 8 x 0x4d cell, blits PORTALB
   ds:7E4B, fills the action field with f19EC_0002(5), clips the panel, draws block[0], stripA[0],
   stripB[0] and extra[0] and shuffles the grid at origin (0x18, 0) of 0x90 columns by 0x68 rows.
   It then runs an animation script at ds:7C32 of (cell, step) byte pairs: each pair draws
   block[i], stripA[i] and stripB[i], then draws the board cell the step selects (or the extra
   8 x 0x4d cell when the step is 0), and plays the step sounds; the script ends at the first byte
   that is negative (0xff). Finally it frees the one extra cell and the three sprite arrays.
   Called from the menu dispatcher 1A82 (mode 8). */
/*@ ovl5:1204 */ void TileAnimation(void)
{
    char far *script;
    void far *stripA[5];     /* 3 x 0x4b strips sliced at sheet y 0 */
    void far *stripB[5];     /* 3 x 0x4b strips sliced at sheet y 0x50 */
    void far *block[5];      /* 0xf x 0x12 cells sliced at sheet x 0xf */
    void far *extra[11] = { 0 };  /* only extra[0] holds the one 8 x 0x4d cell; the array size
                                     is what the original frame reserves */
    int i;

    FreeEffectTables();
    FreeMapIcons(0, 0x24);
    FreePtrTable();
    LoadOverlayScreen(gFileNamePortalA, 5, 3);
    for (i = 0; i < 5; i++) {
        stripA[i] = gGetSpriteCell(i * 3, 0, 3, 0x4b, gOverlayBuf);
        stripB[i] = gGetSpriteCell(i * 3, 0x50, 3, 0x4b, gOverlayBuf);
        block[i] = gGetSpriteCell(0xf, i * 0x12, 0xf, 0x12, gOverlayBuf);
    }
    extra[0] = gGetSpriteCell(0x1e, 0, 8, 0x4d, gOverlayBuf);
    LoadScreenImage(gFileNamePortalB, gOverlayBuf, 5, 3, 2);
    PlaySound(0x21);
    PlaySound(0x13);
    gStatusLine = 2;
    gFn807E(3, 0, 0x12, 0x68, f19EC_0002(5));
    ClipRect(3, 0, 3, 0, 0x12, 0x68, 0, 2);
    DrawShape(2, block[0], 0x1c, 9, 0);
    DrawShape(2, stripA[0], 0x22, 0x1c, 0);
    DrawShape(2, stripB[0], 0x78, 0x1c, 0);
    DrawShape(2, extra[0], 0x38, 0x1b, 0);
    ShuffleGrid(0x18, 0, 0x90, 0x68, 2, 0);
    gFn804A(3, 0, 0x12, 0x68, f19EC_0002(5));
    WaitTicks(0x1e);
    script = gText7C32;
    while (*script > -1) {
        i = *script++;
        DrawShape(0, block[i], 0x1c, 9, 0);
        DrawShape(0, stripA[i], 0x22, 0x1c, 0);
        DrawShape(0, stripB[i], 0x78, 0x1c, 0);
        if (i == 1 && script[-3] == 0) {
            PlaySound(0x18);
            PlaySound(0x56);
        }
        i = *script++;
        if (i != 0) {
            i--;
            ClipRect((i % 5) * 8, (i / 5) * 0x4d, 7, 0x1b, 8, 0x4d, 2, 0);
        } else {
            DrawShape(0, extra[0], 0x38, 0x1b, 0);
        }
        if (i == 1)
            PlaySound(0x1f);
        if (i == 3 && script[-2] == 3)
            PlaySound(0x5a);
        WaitTicks(2);
    }
    farfree(extra[0]);
    for (i = 4; i >= 0; i--) {
        farfree(block[i]);
        farfree(stripB[i]);
        farfree(stripA[i]);
    }
}
/* ovl5:160A: the winning-party password screen at the end of the game. It loads TEXT.DAT
   (ds:7E53) into the string base gSegDF4E, sets up the panel, and when gFC04 bits 1..12 are all
   set (gFC04 & 0x1FFE == 0x1FFE) draws the winner title ds:7E5C, the first four party members'
   names (separated by ds:7EDB) and then a checksum of every party member's name: the sum, over the
   name's characters, of the signed long square of each character (F_LXMUL@, 0000:7737), formatted
   into gFileNameBuf through the "\r\rPassword: %04x\r" format at ds:7EDD. It then waits for a key
   and clears gScrollOn. Called by EndingAnimation (17C8) and by 08E6:0EC3 Obj0EC3. */
/*@ ovl5:160A */ void WinnerPanel(void)
{
    int handle;
    long sum;
    int i;
    int j;

    handle = FileOpen(gFileNameText, 1);
    FileRead(handle, gSegDF4E, 0x7d00L);
    FileClose(handle);
    PlaySound(0x14);
    gStatusLine = 0;
    gScrollOn = 1;
    gFE42 = 1;
    g169 = gF57E;
    SetTextWindow(0xc);
    gScrollHook = 0xac;
    g8119 = 0x2d0;
    LockHidePointer();
    DrawFilledPanel(0, 0, 0xaf, 0xae, gFB4C, gF53A, gF57E);
    LockShowPointer();
    PrintTableString(0x33, 1);
    if ((gFC04 & 0x1ffe) == 0x1ffe) {
        DrawText(gText7E5C);
        for (i = 0; i < 4; i++) {
            DrawText(gText7EDB);
            if (gChars[i].status & 1)
                DrawText(gChars[i].name);
        }
        sum = 0;
        for (i = 0; i < 6; i++) {
            if (gChars[i].status & 1) {
                for (j = 0; j < strlen(gChars[i].name); j++)
                    sum += (long)gChars[i].name[j] * gChars[i].name[j];
            }
        }
        sprintf(gFileNameBuf, gText7EDD, sum);
        DrawText(gFileNameBuf);
        WaitTicks(0xa);
        FlushKeyBuffer();
        WaitKeyEvent();
    }
    gScrollOn = 0;
}
/* ovl5:17C8: the second ending animation (mode 9). It tears down the dungeon resources
   (FreeEffectTables, FreeMapIcons(0,0x24), FreePtrTable), loads XDEATH2 (ds:7EEF) and builds the
   four sprite cells from gOverlayBuf (a 2x2 grid of 0xE x 0x58 cells), loads XDEATH3 (ds:7EF7)
   and slices one 0x10 x 0x5f cell, blits XDEATH1 (ds:7EFF), then plays ten frames: six clipped
   cells (with the sounds 0x48, 0x36, 0x36, 0x22) and, for the last four, the 2x2 sprite grid
   (sprites[i-6], sound 0x2a). It then clears gStatusLine, fills the action field with
   f19EC_0002(5) (gFn807E), sets gF1 to 8 and runs the 19-step explosion: each step plays 0x77,
   blits the page (gFn804A), draws the sliced cell moving from y=-0x5a in steps of 5, clips the
   portrait region and waits 2 ticks. It finally plays sound 5, clears gF1, waits 0x3c ticks, shows
   the pointer, runs the winning-party panel (160A), frees the four sprites and the sliced cell and
   shuts the game down. Called from the menu dispatcher 1A82 (mode 9). */
/*@ ovl5:17C8 */ void EndingAnimation(void)
{
    void far *sprites[4];
    void far *cell;
    int i;

    FreeEffectTables();
    FreeMapIcons(0, 0x24);
    FreePtrTable();
    LoadOverlayScreen(gFileNameXDeath2, 5, 3);
    for (i = 0; i < 4; i++)
        sprites[i] = gFn8082((i % 2) * 0xe, (i / 2) * 0x58, 0xe, 0x58, gOverlayBuf);
    LoadOverlayScreen(gFileNameXDeath3, 5, 3);
    cell = gFn8082(0x16, 0, 0x10, 0x5f, gOverlayBuf);
    LoadScreenImage(gFileNameXDeath1, gOverlayBuf, 5, 3, -1);
    f19EC_002A(3, 2, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    for (i = 0; i < 0xa; i++) {
        if (i == 2)
            PlaySound(0x48);
        if (i == 4)
            PlaySound(0x36);
        if (i == 6)
            PlaySound(0x36);
        PlaySound(0x22);
        if (i < 6) {
            ClipRect((i % 3) * 0xd, (i / 3) * 0x58, 4, 0xa, 0xd, 0x58, 2, 0);
        } else {
            PlaySound(0x2a);
            DrawShape(0, sprites[i - 6], 0x20, 0xa, 0);
        }
        WaitTicks(4);
    }
    gStatusLine = 0;
    gFn807E(0, 0, 0x16, 0x78, f19EC_0002(5));
    gF1 = 8;
    for (i = 0; i < 0x13; i++) {
        PlaySound(0x77);
        gStatusLine = 2;
        gFn804A(0, 0, 0x16, 0x78, f19EC_0002(5));
        DrawShape(2, cell, 0x18, i * 5 + 0xffa6, 5);
        ClipRect(0, 0, 0, 0, 0x16, 0x78, 2, 0);
        WaitTicks(2);
    }
    PlaySound(5);
    gF1 = 0;
    WaitTicks(0x3c);
    LockShowPointer();
    WinnerPanel();
    for (i = 0; i < 4; i++)
        farfree(sprites[i]);
    farfree(cell);
    ShutdownGame();
}
/* ovl5:1A82: the overlay's entry and dispatcher (reached from 122E:1D76 and the overlay's stub
   entry). It hides the pointer, saves the current font, handles the gPanelMode/gFlag message redraws
   and the overlay-4 call, then dispatches `mode`: 8 runs the 1204 animation, 9 the 17C8 one and
   0xa the "find the word" panel (ovl2:09ED); every other mode draws the item screen (loads OUTTAKE
   ds:7F07, reads TEXT.DAT ds:7F0F into gSegDF4E, draws the panel and the 8x8 font) and runs one of
   the eight menu panels through the jump table at cs:1CCD. Finally it restores the font and the
   pointer, loads a wall texture set (fOvl3_07A5, LoadWallTextures) for the sheet
   gMapSheet + (gMapTiles[gFBC8 - 1] << 5), with a NULL name so the current wall name is reused,
   loads the PLAYFLD (ds:7F18) and INVENT (ds:7F20) images,
   redraws the characters and resets the text window to 7. The eight panel cases are laid out in
   the source order 0,1,2,3,4,6,5,7 (the original's bodies are 0924, 0B58, 0EB9, 0F77, 102C, 1087,
   10A6, 119E, so the jump table maps mode 5 to the 10A6 body and mode 6 to the 1087 body). */
/*@ ovl5:1A82 */ void CharOverlay(int mode)
{
    void far *font;
    register int handle;

    font = gCurFont;
    gEFC6 = -1;
    gFE42 = 0;
    LockHidePointer();
    if (gPanelMode)
        Action01E3(gF542);
    if (gFlag)
        RedrawPartyPanel(gF542);
    DrawViewFrame(1);
    switch (mode) {
    case 8:
        TileAnimation();
        break;
    case 9:
        EndingAnimation();
        break;
    case 0xa:
        fOvl2_09ED();
        break;
    default:
        LoadOverlayScreen(gFileNameOuttake, 5, 3);
        ClipRect(0, 0, 0, 0, 0x16, 0x78, 0, 6);
        DrawCharPortrait(mode);
        LockShowPointer();
        handle = FileOpen(gFileNameText2, 1);
        FileRead(handle, gSegDF4E, 0x7d00L);
        FileClose(handle);
        SetFont(gFont6x6);
        HidePointerZone(0, 0x79, 0x13f, 0xc8);
        DrawFilledPanel(0, 0x79, 0x13f, 0xc8, gFB4C, gF53A, gF57E);
        ShowPointerZone();
        gScrollOn = 1;
        g139 = gF57E;
        SetTextWindow(9);
        gScrollHook = 0xac;
        g8119 = 0x2d0;
        switch (mode) {
        case 0:
            TemplePanel();
            break;
        case 1:
            ItemPanelDispatch();
            break;
        case 2:
            ResurrectPanel();
            break;
        case 3:
            AttackPanel();
            break;
        case 4:
            KillHerPanel();
            break;
        case 6:
            QuitPanel();
            break;
        case 5:
            SurrenderPanel();
            break;
        case 7:
            FreeHimPanel();
            break;
        }
        SetFont(font);
        gScrollOn = 0;
        LockHidePointer();
        fOvl3_07A5(0, gMapSheet + (gMapTiles[gFBC8 - 1] << 5));
        LoadScreenImage(gFileNamePlayfld, gSheetDECORATE, 5, 3, 0);
        LoadScreenImage(gFileNameInvent, gSheet2067, 5, 3, 2);
        gStatusLine = 0;
        RedrawCharacters();
        DrawViewFrame(1);
        break;
    }
    SetTextWindow(7);
    LockShowPointer();
}
/* ovl5:1CDD: the "use item" result panel. It hides the mouse cursor, runs the pending
   gPanelMode/gFlag messages, opens TEXT.DAT (ds:7F27, 1: read) and reads it into the string base
   gSegDF4E, sets the text window and the panel colours, draws the panel (0,0)-(0xaf,0xae) and
   shows message `msg` through PrintTableString. It then restores the screen: loads the wall
   textures through fOvl3_07A5 (LoadWallTextures, sheet gOverlayBuf, name NULL reuses the current
   wall name), loads the PLAYFLD (ds:7F30) and INVENT (ds:7F38) images, redraws the characters and
   the view frame and resets the text window to 7. Called by the item-slot handler 0BD2:1001. */
/*@ ovl5:1CDD */ void UseItemPanel(int msg)
{
    int handle;

    gFE42 = 1;
    if (gPanelMode)
        Action01E3(gF542);
    if (gFlag)
        RedrawPartyPanel(gF542);
    handle = FileOpen(gFileNameText3, 1);
    FileRead(handle, gSegDF4E, 0x7d00L);
    FileClose(handle);
    gScrollOn = 1;
    g169 = gF57E;
    SetTextWindow(0xc);
    gScrollHook = 0xac;
    g8119 = 0x2d0;
    DrawFilledPanel(0, 0, 0xaf, 0xae, gFB4C, gF53A, gF57E);
    LockShowPointer();
    PrintTableString(msg, 1);
    gScrollOn = 0;
    LockHidePointer();
    fOvl3_07A5(0, gOverlayBuf);
    LoadScreenImage(gFileNamePlayfld2, gSheetDECORATE, 5, 3, 0);
    LoadScreenImage(gFileNameInvent2, gSheet2067, 5, 3, 2);
    gStatusLine = 0;
    RedrawCharacters();
    DrawViewFrame(1);
    SetTextWindow(7);
    LockShowPointer();
}
/* ovl5:1E1B: shuffles a cols x rows grid. It fills two index arrays in the gBufDCFC buffer (the
   x indices and, at +0x280, the y indices) with 0..cols-1 and 0..rows-1, Fisher-Yates shuffles
   each with RandomRange, then for each row k and column i saves the pixels under the cell at
   (x + xs[i], y + ys[j]) (j wrapping at rows) through the far pointer ds:8072 and restores them
   through ds:809E, with gStatusLine set to saveStatus for the save and restoreStatus for the
   restore. Called by the 1204 animation. */
/*@ ovl5:1E1B */ void ShuffleGrid(int x, int y, unsigned cols, unsigned rows, int saveStatus, int restoreStatus)
{
    int far *xs;
    int far *ys;
    int handle;                 /* the gFn8072 handle, and the Fisher-Yates swap temp: the original
                                   reuses one slot (a second local grows the frame and breaks the match) */
    int k;
    int pick;
    int j;
    int saved;
    register int i;
    register int idx;           /* the register local: the swap index and the draw-loop y coordinate */

    xs = gBufDCFC;
    ys = (int far *)((char far *)xs + 0x280);
    for (i = 0; i < cols; i++)
        xs[i] = i;
    for (i = 0; i < rows; i++)
        ys[i] = i;
    for (i = 0; i < cols; i++) {
        pick = RandomRange(0, cols - 1);
        idx = i;
        handle = xs[pick];
        xs[pick] = xs[idx];
        xs[idx] = handle;
    }
    for (i = 0; i < rows; i++) {
        pick = RandomRange(0, rows - 1);
        idx = i;
        handle = ys[pick];
        ys[pick] = ys[idx];
        ys[idx] = handle;
    }
    saved = gStatusLine;
    for (k = 0; k < rows; k++) {
        j = k;
        for (i = 0; i < cols; i++) {
            pick = x + xs[i];
            idx = y + ys[j];
            j++;
            if (j >= rows)
                j = 0;
            gStatusLine = saveStatus;
            handle = gFn8072(pick, idx);
            gStatusLine = restoreStatus;
            gFn809E(pick, idx, handle);
        }
    }
    gStatusLine = saved;
}
