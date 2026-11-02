/* Unit at overlay 5 of EOB.EXE (8094 bytes, 31 functions): the character/item overlay the map
   loop opens on the "inventory" key. Its entry ovl5:1A82 dispatches a menu (modes 0..7) to the
   seven panels 0924/0B58/0EB9/0F77/102C/1087/10A6/119E, and modes 8, 9 and 0xa to the two long
   animations 1204/17C8 and the "find the word" panel (ovl2:09ED). It reads the six 243-byte
   (0xF3) character records at ds:F580 (struct CharRec), draws the item grid through 0455 and the
   ds:7B89/7B94 position tables, and its 160A prints the winning-party password. Names are
   placeholders until evidence names them. */

/*@flags -ml -O -Z */

#include <stdlib.h>     /* abs, the inline __abs__ the select lists use */
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
    unsigned char flags;    /* +01: status bits (only bit 0 is read here) */    char name[0x0B];        /* +02: the character's name, NUL padded */
    char pad0D[0x1B - 0x0D];
    signed char hp;         /* +1B: current hit points */
    signed char maxhp;      /* +1C: maximum hit points */
    char pad1D[0x22 - 0x1D];
    unsigned char m22;      /* +22: compared with 0xf7 by 0AB1 (src/E0D1C.C:0415 uses it too) */
    char pad23[0x33 - 0x23];
    void far *m33;          /* +33: a far pointer to the character's picture buffer, preserved by
                                   the record swap at 070E (the movmem destination there) */
    char pad37[0x77 - 0x37];
    int m77[25];            /* +77: 25 item-slot words (the carried and equipped items) */
    char padA9[0xE3 - 0xA9];
    unsigned long mE3;      /* +E3: effect flags (0C9D clears bit 7 of the low word) */
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
    signed char m02;        /* +2: the item type */
    char pad03;
    int m04;                /* +4 */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB */
    char pad0C[0x0E - 0x0C];
};

struct ItemInfo {           /* 16-byte records at ds:A9F5 (src/E0D1C.C) */
    signed char m00;
    signed char m01;
    char pad02[0x09 - 0x02];
    int m09;                /* +9 */
    char pad0B[0x10 - 0x0B];
};

struct Obj28 {              /* 28-byte object records at ds:8A82 (src/E08E6.C) */
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
extern int gFBCC; /*@ ds:FBCC */
extern int gPartyCell; /*@ ds:FBCA */
extern signed char g1A1B[]; /*@ ds:1A1B */
extern char gB899[]; /*@ ds:B899 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct CharRec gCharPool[]; /*@ ds:7298 */  /* the pre-made character records 070E copies into gChars */
extern struct PoolNameRec gPoolNameBase[]; /*@ ds:71A7 */  /* ds:71A7 = gCharPool + 2 - 0xF3, the base of the
                                                              folded gCharPool[-n-1].name access */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern struct ItemInfo gA9F5[]; /*@ ds:A9F5 */
extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern int gCountDDE0; /*@ ds:DDE0 */
extern int gTableF4E6[]; /*@ ds:F4E6 */
extern char gOverlayBuf[]; /*@ ds:113F */
extern char far *gSegDF4E; /*@ ds:DF4E */
extern unsigned char gCellDef7B2B[]; /*@ ds:7B2B */
extern char gTable7B89[]; /*@ ds:7B89 */
extern char gTable7B94[]; /*@ ds:7B94 */
extern char gTable7B77[]; /*@ ds:7B77 */
extern char gTable7B7D[]; /*@ ds:7B7D */
extern char gTable7B83[]; /*@ ds:7B83 */
extern char gText7B23[]; /*@ ds:7B23 */
extern char gText7B9F[]; /*@ ds:7B9F */
extern char gText7BBA[]; /*@ ds:7BBA */
extern char gText7BC2[]; /*@ ds:7BC2 */
extern char gText7CB7[]; /*@ ds:7CB7 */
extern char gText7CBC[]; /*@ ds:7CBC */
extern char gText7D0E[]; /*@ ds:7D0E */
extern char gText7D77[]; /*@ ds:7D77 */
extern char gText7D9E[]; /*@ ds:7D9E */
extern char gText7E53[]; /*@ ds:7E53 */
extern char gText7E5C[]; /*@ ds:7E5C */
extern char gText7EDB[]; /*@ ds:7EDB */
extern char gText7EDD[]; /*@ ds:7EDD */
extern char gText7EEF[]; /*@ ds:7EEF */
extern char gText7EF7[]; /*@ ds:7EF7 */
extern char gText7EFF[]; /*@ ds:7EFF */
extern char gText7E43[]; /*@ ds:7E43 */
extern char gText7E4B[]; /*@ ds:7E4B */
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
extern void far *gFont1A47; /*@ ds:FE4E */
extern void far *gA9C0; /*@ ds:A9C0 */
extern char gFB44; /*@ ds:FB44 */
extern char gFlag; /*@ ds:F3B9 */
extern char gText7F07[]; /*@ ds:7F07 */
extern char gText7F0F[]; /*@ ds:7F0F */
extern char gText7F18[]; /*@ ds:7F18 */
extern char gText7F20[]; /*@ ds:7F20 */
extern char gText7F27[]; /*@ ds:7F27 */
extern char gText7F30[]; /*@ ds:7F30 */
extern char gText7F38[]; /*@ ds:7F38 */
extern int far *gBufDCFC; /*@ ds:DCFC */
extern int g169; /*@ ds:0169 */
extern int (far *gFn8072)(int a, int b); /*@ ds:8072 */
extern void (far *gFn809E)(int x, int y, int handle); /*@ ds:809E */
extern int RandomRange(int low, int high); /*@ 1AAA:0005 */
extern char gF542[]; /*@ ds:F542 */
extern int gScrollHook; /*@ ds:8117 */
extern int g8119; /*@ ds:8119 */
extern int g139; /*@ ds:0139 */
extern int gFBC8; /*@ ds:FBC8 */
extern unsigned char far gSeg1D0[]; /*@ 01D0:0000 */
extern unsigned char far gSeg1D8[]; /*@ 01D8:0000 */
extern unsigned char far gSeg1F8[]; /*@ 01F8:0000 */
extern unsigned char far gSeg1F0[]; /*@ 01F0:0000 */

extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
extern void far * (far *gFn8082)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int flag); /*@ ds:80CA */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */
extern void (far *gFn807E)(int x, int y, int w, int h, void far *buf); /*@ ds:807E */

extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
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
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
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
extern void fOvl4_1F51(int n); /*@ 2116:005C */  /* overlay 4 entry, ovl4:1F51 */
extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern void LoadScreenImage(char far *name, void far *buf, int a, int b, int c); /*@ 082C:051B */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void f1A47_0169(void far *font); /*@ 1A47:0169 */
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
void ShuffleGrid(int x, int y, unsigned cols, unsigned rows, int attr1, int attr2);

void PrintTableString(int index, int flag);

/* ovl5:0000: draws sprite cell `index` of the ds:7B2B table out of the overlay buffer gOverlayBuf
   at (x, y), moved left by the returned cell's byte +2 (times four) and up by its byte +1, into
   region 5. Sets gStatusLine for the draw and frees the sliced cell. Called by the avatar panel 00A1. */
/*@ ovl5:0000 */ void DrawSheetCell(int index, int x, int y)
{
    void far *cell;
    int c1;
    int c2;
    int c3;
    int c4;
    int i;

    gStatusLine = 2;
    i = index * 4;
    c1 = gCellDef7B2B[i];
    c2 = gCellDef7B2B[i + 1];
    c3 = gCellDef7B2B[i + 2];
    c4 = gCellDef7B2B[i + 3];
    cell = gGetSpriteCell(c1, c2, c3, c4, gOverlayBuf);
    gStatusLine = 0;
    y -= ((struct Cell far *)cell)->b1;
    x -= (((struct Cell far *)cell)->b2 << 3) >> 1;
    f1A29_000F(0, cell, x, y, 5);
    farfree(cell);
}

/* ovl5:00A1: draws the character portrait for the current panel. It clips the portrait region
   (0,0)-(0x16,0x78) and copies the three per-character cell-index tables ds:7B77, ds:7B7D and
   ds:7B83 (one six-entry row per party member) into locals, then draws the cells the `mode`
   selects through DrawSheetCell: the fixed cells 2, 7, 6, 0x12, 0x11 and 4, or, for modes 1 and
   2, the current character's own cells, indexed by the selected member gEFC6. Called by the
   overlay dispatcher 1A82 for modes 0..7 and by the resurrect list 0DBB with mode 2. */
/*@ ovl5:00A1 draft */ void DrawCharPortrait(int mode)
{
    char head[6] = { 9, 8, 8, 9, 9, 0x0A };         /* ds:7B77 */
    char body[6] = { 0x0D, 0x0B, 0x0C, 0x0E, 0x0F, 0x10 };  /* ds:7B7D */
    char feet[6] = { 0x34, 0x31, 0x31, 0x34, 0x34, 0x45 };  /* ds:7B83 */

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
            DrawSheetCell(head[gEFC6], 0x74, 0x68);
            DrawSheetCell(body[gEFC6], 0x74, feet[gEFC6]);
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
char gTable7B89[] = { 0x3B, 0xA6, 0x04, 0x70, 0xDC, 0x04, 0x70, 0xDC, 0x04, 0x70, 0xDC }; /*@ ds:7B89 */

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

/* ovl5:025F: the modal message box. It picks one of two rectangles from gFE42 (the two player
   windows), hides the pointer, fills the panel in the three colours, draws ds:7CB7 in colour 0xf
   and shows the pointer. It then waits for a key or a click: a click anywhere in the box, or one of
   the keys 0x2b, 0x34, 0x3d, dismisses it, as do the keys 0x41 and 0x42 only when the click lies
   inside the box. In the end it restores the pointer lock, refills the rectangle and clears the
   text region. Called by PrintTableString (03A9) after its message. */
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
        key = WaitKeyEvent() & 0x8FF;
        switch (key) {
        case 0x41:
        case 0x42:
            if (!IsPointInsideRect(gClickX, gClickY, left, top, right, bottom))
                break;
            /* fall through */
        case 0x2B:
        case 0x34:
        case 0x3D:
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
   message panel 025F. The strings are the overlay's own yes/no menu labels. */
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

/* the key codes this menu accepts; the switch table at ovl5:0657 lists them. The three confirm
   keys, the two mouse-button keys and up/down are the same game keys as src/E14EF.C's
   UpdateOptionList; 0x5c and 0x66 are this menu's own left/right (or prev/next) keys. */
enum {
    KEY_CONFIRM  = 0x2B,
    KEY_CONFIRM2 = 0x3D,
    KEY_CONFIRM3 = 0x61,
    KEY_CLICK    = 0x41,
    KEY_CLICK2   = 0x42,
    KEY_PREV     = 0x5C,
    KEY_UP       = 0x60,
    KEY_DOWN     = 0x62,
    KEY_NEXT     = 0x66
};

/* ovl5:0455: draws and runs one of the overlay's menus: the `count` rows starting at position
   `first` in the ds:7B89/ds:7B94 position tables, labelled with the far strings names[0..count-1].
   Each row is a DrawMenuRow panel; the highlighted row (i) is drawn in colour 9, the others in
   0xf. It redraws only when the highlight moved, then reads a key and also checks the mouse. The
   mouse-button keys 0x41/0x42 confirm the row under the last click (gClickX/gClickY); the confirm keys
   0x2b/0x3d/0x61 confirm the highlighted row; 0x5c/0x66 and 0x60/0x62 move the highlight by one
   or three (a three-row column-major grid, wrapping). Returns the confirmed row index, after
   clearing the panel. Called by every menu panel of the overlay (067B, 0828, 0924, 09C5, 0AB1,
   0B16, 0EB9, 0F77, 102C, 1087, 10A6, 119E). */
/*@ ovl5:0455 draft */ int SelectMenu(char first, char count, char far **names)
{
    int ybase;
    int sel;
    int key;
    int drawn;
    int j;
    int keycode;
    int i;

    ybase = gTextTop + (gTextRow + 1) * gFontHeight + 4;
    drawn = sel = -1;
    i = 0;
    do {
        LockHidePointer();
        if (drawn != i) {
            for (j = 0; j < count; j++) {
                DrawMenuRow(gTable7B89[first + j], ybase + gTable7B94[first + j],
                            i == j ? 9 : 0xf, names[j]);
            }
            drawn = i;
        }
        LockShowPointer();
        key = WaitKeyEvent() & 0x8ff;
        for (j = 0; j < count; j++) {
            if (IsPointInsideRect(gCursorX, gCursorY, gTable7B89[first + j],
                                  ybase + gTable7B94[first + j],
                                  gTable7B89[first + j] + 0x5e,
                                  ybase + gTable7B94[first + j] + 8)) {
                i = j;
                break;
            }
        }
        keycode = key;
        switch (keycode) {
        case KEY_CLICK:
        case KEY_CLICK2:
            for (j = 0; j < count; j++) {
                if (IsPointInsideRect(gClickX, gClickY, gTable7B89[first + j],
                                      ybase + gTable7B94[first + j],
                                      gTable7B89[first + j] + 0x5e,
                                      ybase + gTable7B94[first + j] + 8)) {
                    sel = j;
                    break;
                }
            }
            break;
        case KEY_PREV:
            i--;
            if (i < 0)
                i = count - 1 <= 2 ? count - 1 : 2;
            break;
        case KEY_NEXT:
            i++;
            if (i > 2 || count - 1 < i)
                i = 0;
            break;
        case KEY_DOWN:
            if (i + 3 < count)
                i += 3;
            else if (i > 3)
                i -= 3;
            /* falls through (the original has no break here) */
        case KEY_UP:
            if (i > 3)
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
        count += gChars[i].flags & 1;
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
   of gChars whose flags bit 0 is clear, saves that slot's picture pointer m33, copies the pool
   record over it through movmem, restores m33, recomputes the armour class, requeues every carried
   item through CopyItemToFreeSlot, then slices the character's sprite cell
   (gGetSpriteCell(which*4, 0xa0, 4, 0x20, gOverlayBuf)) into the picture buffer m33 and frees the
   slice. Called by AddCharPanel. */
/*@ ovl5:070E */ void AddPartyMember(int which)
{
    struct CharRec far *rec;
    void far *saved;   /* the slot's picture pointer, then the sliced sprite cell */
    int slot;
    int i;

    slot = i = 0;
    for (; i < 6; i++) {
        if ((gChars[i].flags & 1) == 0) {
            slot = i;
            break;
        }
    }
    rec = &gChars[slot];
    saved = rec->m33;
    movmem((void far *)&gCharPool[which], (void far *)rec, 0xF3);
    rec->m33 = saved;
    ComputeArmourClass(slot);
    for (i = 0; i < 0x19; i++) {
        if (rec->m77[i])
            rec->m77[i] = CopyItemToFreeSlot(rec->m77[i]);
    }
    gStatusLine = 2;
    saved = gFn8082(which * 4, 0xA0, 4, 0x20, gOverlayBuf);
    movmem(saved, rec->m33, 0x514);
    farfree(saved);
    gStatusLine = 0;
}
/* ovl5:0828: the character add panel. It clears the panel, draws message `which`, then shows the
   two-entry menu ds:7B23. Choice 0 ("join") either draws message `head` or, when `head` is -1,
   builds and draws "<name> joins the party." (ds:7D0E) for gCharPool[who] and shows the modal
   message 025F; it then removes the item class 0x21 from character `who`+1 and, when ChooseChar
   finds a free slot, adds the character through 070E. Choice 1 draws message `other`. Called by
   the panels 0924, 09C5, 0DBB and 119E. */
/*@ ovl5:0828 */ void AddCharPanel(int who, int which, int head, int other)
{
    int sel;

    ClearPanel();
    PrintTableString(which, 0);
    sel = SelectMenu(0, 2, (void far *)gText7B23);
    switch (sel) {
    case 0:
        if (head != -1) {
            PrintTableString(head, 1);
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
        PrintTableString(other, 1);
        break;
    }
}
/* ovl5:08D2: turns item `n` into a carried item (CopyItemToFreeSlot) and queues it on the list
   head gB899 for the current sound page (gPartyCell) with a random page sound. Called by the
   panels that add an item to a character. */
/*@ ovl5:08D2 */ void QueueItem(int item)
{
    int sound;

    item = CopyItemToFreeSlot(item);
    sound = g1A1B[gFBCC * 4 + RollDice(1, 2, -1)];
    InsertListItem(&gB899[gPartyCell * 9], gPartyCell, item, sound);
}

/* ovl5:0924: the temple/NPC panel. Clears the panel, draws message 8 and shows the menu ds:7BB2
   ("Heal Party"/"Leave") when gFC04 bit 0x2000 is set, otherwise ds:7BA6 ("Tend his wounds"/
   "Talk"/"Leave"). Choice 0 runs the character panel 0828 and sets bit 0x4000 and returns; choice
   1 draws message 1, sets bit 0x2000 and repeats; anything else returns. Called from the dispatcher
   1A82 (mode 0). */
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

/* ovl5:09C5: the "give / take an item" panel. On the first pass (gFC04 bit 0x8000 clear) it sets
   that bit and shows the two-entry menu ds:7BBA; choice 0 draws message 4, choice 1 records that
   the whole party is to be used (the local flag cleared) and the panel is left. Otherwise it draws
   message 0xd. When the flag is still set it shows the menu ds:7BC2; choice 0 queues the six
   healing items 0x37 and the item 0x3e on the current page, clears gFC04 bit 0x8000, sets gFC04
   bit 0x10000 and runs the character panel 0828(7, 7, 0x1d, 0x1e); choice 1 (or a free-party
   choice) draws message 5. Called by the panel dispatcher 0B58. */
/*@ ovl5:09C5 */ void GiveItemPanel(void)
{
    int whole;
    int sel;
    int i;

    whole = 1;
    if (!(gFC04 & 0x8000L)) {
        gFC04 |= 0x8000L;
        PrintTableString(3, 0);
        sel = SelectMenu(0, 2, (void far *)gText7BBA);
        if (sel == 0)
            PrintTableString(4, 0);
        else if (sel == 1)
            whole = 0;
    } else {
        PrintTableString(0xd, 0);
    }
    if (whole)
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
/* ovl5:0AB1: returns 1 when some party member (flags bit 0) has m22 == 0xf7, after drawing menu
   strings 0x19 and 0x1a and setting the 0x80000 bit of gFC04; otherwise 0. Called by the panel
   dispatcher 0B58. */
/*@ ovl5:0AB1 */ int CheckChar0F7(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (gChars[i].flags & 1) {
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

/* ovl5:0B16: when RemoveCarriedItems(6, -1) removes something, draws menu string 0x1c, queues
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

/* ovl5:0B58: the panel-dispatcher of the overlay's menu: when bit 0x10000 of gFC04 is clear it
   runs the item-select panel 09C5, when 0x80000 is clear it runs 0AB1, when 0x100000 is clear it
   runs 0B16; if none of them did anything it draws "Leave" and the message 025F. Called from the
   menu dispatcher 1A82. */
/*@ ovl5:0B58 */ void Menu1(void)
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
   of its 25 item slots whose item type's gA9F5[type].m09 bit 0..6 equals 8, as
   -gTable8DD2[item].m0B. Returns the count. Called by 0EB9. The counters are zeroed through one
   AX (`gCountDDE0 = i = 0`), the two flags tests are a `continue` pair (which makes the build
   reload the far pointer, ovl5:0BE9), and the three locals are declared type, kind, item (TC lays
   the frame out in declaration order, ovl5:0C4D/0C5E/0C37), all as the original has them. */
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
        if ((rec->flags & 1) == 0)
            continue;
        if (rec->hp != -10)
            continue;
        gTableF4E6[gCountDDE0++] = i;
    }
    for (i = 0; i < 6; i++) {
        rec = &gChars[i];
        if (rec->flags & 1) {
            for (j = 0; j < 0x19; j++) {
                item = rec->m77[j];
                if (!item)
                    continue;
                type = gTable8DD2[item].m02;
                kind = gA9F5[type].m09 & 0x7f;
                if (kind == 8)
                    gTableF4E6[gCountDDE0++] = -gTable8DD2[item].m0B;
            }
        }
    }
    return gCountDDE0;
}
/* ovl5:0C9D: heals the party. Rolls 1d3+2 "cures", then for each party member that f0D1C_0670
   accepts (mask 3) clears its flag bits 1..2, clears bit 7 of its effect flags, clears effect slot
   -0x22 and, while it is below its maximum hit points, spends one cure (1d8+9, clamped to the
   maximum) and heals it again. Called by the resurrect panel 0EB9. The only remaining difference
   is that the original keeps the two hit-point stores separate while ours merges their tails into
   one shared store; no source form tried keeps them apart, so this stays a draft. */
/*@ ovl5:0C9D draft */ void HealLoop(void)
{
    int i;
    int cures;
    int heal;

    i = 0;
    cures = RollDice(1, 3, 2);
    while (cures && i < 6) {
        if (!f0D1C_0670(i, 3)) {
            i++;
            continue;
        }
        gChars[i].flags &= 0xf9;
        gChars[i].mE3 &= ~0x80L;
        ClearCharEffectSlot(i, -0x22);
        if (gChars[i].hp < gChars[i].maxhp) {
            cures--;
            heal = RollDice(1, 8, 9);
            if (gChars[i].hp + heal > gChars[i].maxhp)
                gChars[i].hp = gChars[i].maxhp;
            else
                gChars[i].hp += heal;
            continue;
        }
        i++;
    }
}
/* ovl5:0DBB: the resurrect panel's "which one?" picker. It clears the panel, draws the title
   ds:7D9E, builds the far string list of the entries BuildDeathList left in gTableF4E6
   (gCountDDE0 of them): a non-negative entry is a party member (gChars[entry].name), a negative
   entry -k-1 is the pre-made character gCharPool[k].name. It then runs the 0455 menu (first row
   2) and acts on the chosen entry: a party member is healed to full (hp = maxhp); a pool character
   has its item class 0x21 removed, becomes the selected character gEFC6, redraws the portrait and
   runs the add-character panel with the head message suppressed (-1). Called by 0EB9.
   The pool-name branch reads through gPoolNameBase at ds:71A7, the base gCharPool[-k-1].name folds
   to (gCharPool ds:7298, its name field +2, one 0xF3 record back); the alias lets cmatch symbolize
   that folded offset, which lies before gCharPool and so misses the DGROUP heuristic's `off <= va`
   symbol pick. The original tests n < 0 first (jge to the positive branch), so the negative body
   comes first, and `(n < 0 ? -n : n) - 1` gives the original's branch-form abs (not abs()'s cdq). */
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
        gChars[n].hp = gChars[n].maxhp;
    }
}
/* ovl5:0EB9: the resurrect/heal NPC panel. When gFC04 bit 0x10000 is clear it draws message
   0x18; when bit 0x20000 is set it draws message 0xb; otherwise it draws message 9 and shows the
   menu ds:7BD2 (3 entries) when BuildDeathList found something, or ds:7BCA (2 entries) otherwise.
   Choice 0 runs 0C9D and choice 1 runs 0DBB, both setting gFC04 bit 0x20000. Called from the menu
   dispatcher 1A82 (mode 2). */
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
/* ovl5:0F77: the "attack / bribe / ..." NPC panel. Draws message 0x12 and shows the 2-entry menu
   ds:7BDE; choice 0 sets gFC04 bits 0x8000000 and 0x400000 and clears the type-8 objects to 5;
   choice 1 removes the item class from RemoveCarriedItems(0x31, -1) and draws message 0x14 and
   sets 0x400000 when something was removed, otherwise message 0x13. Called from the menu
   dispatcher 1A82 (mode 3). */
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
            if (gObjs[i].type == 8)
                gObjs[i].type = 5;
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
/* ovl5:1087: the "quit" menu entry: draws message 0x15 and sets gFC04 bit 0x1000000. Called from
   the menu dispatcher 1A82 (mode 5). */
/*@ ovl5:1087 */ void QuitPanel(void)
{
    PrintTableString(0x15, 1);
    gFC04 |= 0x1000000L;
}
/* ovl5:10A6: the "surrender / attack" NPC panel. Draws message 0x10 and shows the 2-entry menu
   ds:7BF2; choice 0 draws message 0x11 and, for every character's first two item slots whose item
   type gTable8DD2[item].m02 is outside 0x33..0x38, clears the slot and queues the item on the
   current page list. Always sets gFC04 bit 0x20000. Called from the menu dispatcher 1A82
   (mode 6). */
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
                if (item = gChars[i].m77[j]) {
                    if (gTable8DD2[item].m02 < 0x33 || gTable8DD2[item].m02 > 0x38) {
                        gChars[i].m77[j] = 0;
                        InsertListItem(&gB899[gPartyCell * 9], gPartyCell, item,
                                       g1A1B[gFBCC * 4 + RollDice(1, 2, -1)]);
                    }
                }
            }
        }
    }
    gFC04 |= 0x2000000L;
}
/* ovl5:119E: the "free him / kill him / leave" NPC panel. Draws message 0x16, shows the 3-entry
   menu ds:7BFA; choice 0 calls 0828(8, 0x1b, 0x2c, 0x2d) and choice 1 draws message 0x1f. Always
   sets gFC04 bit 0x40000. Called from the menu dispatcher 1A82 (mode 7). */
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
   ds:7E43 and slices fifteen cells out of gOverlayBuf (three per sprite: a 3x0x4b column, a
   3x0x4b column offset 0x50, and an 0xf x 0x12 cell) plus one 8 x 0x4d cell, blits ds:7E4B,
   fills the action field with f19EC_0002(5), clips the panel, draws the four first cells and
   shuffles the 0x18 x 0x90 grid. It then runs an animation script at ds:7C32 of (cell, step)
   byte pairs: each pair draws figures[i], barsTop[i] and barsBot[i], steps a 5x5 board cell (or
   the last cell when the step is 0), and plays the step sounds; the script ends at the first byte
   that is negative (0xff). Finally it frees the one extra cell and the three sprite arrays.
   Called from the menu dispatcher 1A82 (mode 8). */
/*@ ovl5:1204 */ void TileAnimation(void)
{
    char far *script;
    void far *barsTop[5];    /* the 3 x 0x4b strips at y 0 */
    void far *barsBot[5];    /* the 3 x 0x4b strips at y 0x50 */
    void far *figures[5];    /* the 0xf x 0x12 cells */
    void far *extra[11] = { 0 };
    int i;

    FreeEffectTables();
    FreeMapIcons(0, 0x24);
    FreePtrTable();
    LoadOverlayScreen(gText7E43, 5, 3);
    for (i = 0; i < 5; i++) {
        barsTop[i] = gGetSpriteCell(i * 3, 0, 3, 0x4b, gOverlayBuf);
        barsBot[i] = gGetSpriteCell(i * 3, 0x50, 3, 0x4b, gOverlayBuf);
        figures[i] = gGetSpriteCell(0xf, i * 0x12, 0xf, 0x12, gOverlayBuf);
    }
    extra[0] = gGetSpriteCell(0x1e, 0, 8, 0x4d, gOverlayBuf);
    LoadScreenImage(gText7E4B, gOverlayBuf, 5, 3, 2);
    PlaySound(0x21);
    PlaySound(0x13);
    gStatusLine = 2;
    gFn807E(3, 0, 0x12, 0x68, f19EC_0002(5));
    ClipRect(3, 0, 3, 0, 0x12, 0x68, 0, 2);
    f1A29_000F(2, figures[0], 0x1c, 9, 0);
    f1A29_000F(2, barsTop[0], 0x22, 0x1c, 0);
    f1A29_000F(2, barsBot[0], 0x78, 0x1c, 0);
    f1A29_000F(2, extra[0], 0x38, 0x1b, 0);
    ShuffleGrid(0x18, 0, 0x90, 0x68, 2, 0);
    gFn804A(3, 0, 0x12, 0x68, f19EC_0002(5));
    WaitTicks(0x1e);
    script = gText7C32;
    while (*script > -1) {
        i = *script++;
        f1A29_000F(0, figures[i], 0x1c, 9, 0);
        f1A29_000F(0, barsTop[i], 0x22, 0x1c, 0);
        f1A29_000F(0, barsBot[i], 0x78, 0x1c, 0);
        if (i == 1 && script[-3] == 0) {
            PlaySound(0x18);
            PlaySound(0x56);
        }
        i = *script++;
        if (i != 0) {
            i--;
            ClipRect((i % 5) * 8, (i / 5) * 0x4d, 7, 0x1b, 8, 0x4d, 2, 0);
        } else {
            f1A29_000F(0, extra[0], 0x38, 0x1b, 0);
        }
        if (i == 1)
            PlaySound(0x1f);
        if (i == 3 && script[-2] == 3)
            PlaySound(0x5a);
        WaitTicks(2);
    }
    farfree(extra[0]);
    for (i = 4; i >= 0; i--) {
        farfree(figures[i]);
        farfree(barsBot[i]);
        farfree(barsTop[i]);
    }
}
/* ovl5:160A: the winning-party password screen at the end of the game. It loads ds:7E53 into the
   string base gSegDF4E (the password text), sets up the panel, and when all six party bits of
   gFC04 are set (gFC04 & 0x1FFE == 0x1FFE) draws the winner title ds:7E5C, the first four party
   members' names (with a separator ds:7EDB) and then a checksum of every party member's name:
   the sum, over the name's characters, of the signed long square of each character (F_LXMUL@,
   0000:7737), formatted into gFileNameBuf through ds:7EDD. It then waits for a key and clears
   gScrollOn. Called from the menu dispatcher 1A82 (mode 0xb). */
/*@ ovl5:160A */ void WinnerPanel(void)
{
    int handle;
    long sum;
    int i;
    int j;

    handle = FileOpen(gText7E53, 1);
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
            if (gChars[i].flags & 1)
                DrawText(gChars[i].name);
        }
        sum = 0;
        for (i = 0; i < 6; i++) {
            if (gChars[i].flags & 1) {
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
   (FreeEffectTables, FreeMapIcons(0,0x24), FreePtrTable), loads ds:7EEF and builds the four
   sprite cells from gOverlayBuf (a 2x2 grid of 0xE x 0x58 cells), loads ds:7EF7 and slices one
   0x10 x 0x5f cell, blits ds:7EFF, then plays ten frames: six clipped cells (with the sounds
   0x48, 0x36, 0x36, 0x22) and four from the sliced cell (sound 0x2a). It then clears gStatusLine,
   fills the action field with f19EC_0002(5) (gFn807E), sets gF1 to 8 and runs the 19-step
   explosion: each step plays 0x77, blits the page (gFn804A), draws the sliced cell moving from
   y=-0x5a in steps of 5, clips the portrait region and waits 2 ticks. It finally plays sound 5,
   clears gF1, waits 0x3c ticks, shows the pointer, runs the winning-party panel (160A), frees the
   five cells and shuts the game down. Called from the menu dispatcher 1A82 (mode 9). */
/*@ ovl5:17C8 */ void EndingAnimation(void)
{
    void far *sprites[4];
    void far *cell;
    int i;

    FreeEffectTables();
    FreeMapIcons(0, 0x24);
    FreePtrTable();
    LoadOverlayScreen(gText7EEF, 5, 3);
    for (i = 0; i < 4; i++)
        sprites[i] = gFn8082((i % 2) * 0xe, (i / 2) * 0x58, 0xe, 0x58, gOverlayBuf);
    LoadOverlayScreen(gText7EF7, 5, 3);
    cell = gFn8082(0x16, 0, 0x10, 0x5f, gOverlayBuf);
    LoadScreenImage(gText7EFF, gOverlayBuf, 5, 3, -1);
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
            f1A29_000F(0, sprites[i - 6], 0x20, 0xa, 0);
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
        f1A29_000F(2, cell, 0x18, i * 5 + 0xffa6, 5);
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
   entry). It hides the pointer, saves the current font, handles the gFB44/gFlag message redraws
   and the overlay-4 call, then dispatches `mode`: 8 runs the 1204 animation, 9 the 17C8 one and
   0xa the "find the word" panel (ovl2:09ED); every other mode draws the item screen (loads the
   7F07 art, reads 7F0F into gSegDF4E, draws the panel and the 8x8 font) and runs one of the eight
   menu panels through the jump table at cs:1CCD. Finally it restores the font and the pointer,
   copies a byte table into overlay 3, loads the two 7F18/7F20 images, redraws the characters and
   resets the text window to 7. The only remaining difference is the constant-segment far object
   at 01D0 (and the two 01F8/01F0 screen buffers): the original reaches them with the literal
   paragraph (mov ax,0x1d0), while our link relocates the object's offset, so this stays a draft. */
/*@ ovl5:1A82 draft */ void CharOverlay(int mode)
{
    void far *font;
    register int handle;

    font = gFont1A47;
    gEFC6 = -1;
    gFE42 = 0;
    LockHidePointer();
    if (gFB44)
        Action01E3(gF542);
    if (gFlag)
        RedrawPartyPanel(gF542);
    fOvl4_1F51(1);
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
        LoadOverlayScreen(gText7F07, 5, 3);
        ClipRect(0, 0, 0, 0, 0x16, 0x78, 0, 6);
        DrawCharPortrait(mode);
        LockShowPointer();
        handle = FileOpen(gText7F0F, 1);
        FileRead(handle, gSegDF4E, 0x7d00L);
        FileClose(handle);
        f1A47_0169(gA9C0);
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
            Menu1();
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
        case 5:
            QuitPanel();
            break;
        case 6:
            SurrenderPanel();
            break;
        case 7:
            FreeHimPanel();
            break;
        }
        f1A47_0169(font);
        gScrollOn = 0;
        LockHidePointer();
        fOvl3_07A5(0, gSeg1D8 + (gSeg1D0[gFBC8 - 1] << 5));
        LoadScreenImage(gText7F18, gSeg1F8, 5, 3, 0);
        LoadScreenImage(gText7F20, gSeg1F0, 5, 3, 2);
        gStatusLine = 0;
        RedrawCharacters();
        fOvl4_1F51(1);
        break;
    }
    SetTextWindow(7);
    LockShowPointer();
}
/* ovl5:1CDD: the "use item" result panel. It hides the mouse cursor, runs the pending
   gFB44/gFlag messages, opens ds:7F27 (1: read) and reads it into the string base gSegDF4E, sets
   the text window and the panel colours, draws the panel (0,0)-(0xaf,0xae) and shows message `msg`
   through PrintTableString. It then restores the screen: frees the wall textures into gOverlayBuf,
   loads the two images ds:7F30 and ds:7F38, redraws the characters and the view frame and resets
   the text window to 7. Called by the item-slot handler 0BD2:1001. */
/*@ ovl5:1CDD draft */ void UseItemPanel(int msg)
{
    int handle;

    gFE42 = 1;
    if (gFB44)
        Action01E3(gF542);
    if (gFlag)
        RedrawPartyPanel(gF542);
    handle = FileOpen(gText7F27, 1);
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
    LoadScreenImage(gText7F30, gSeg1F8, 5, 3, 0);
    LoadScreenImage(gText7F38, gSeg1F0, 5, 3, 2);
    gStatusLine = 0;
    RedrawCharacters();
    DrawViewFrame(1);
    SetTextWindow(7);
    LockShowPointer();
}
/* ovl5:1E1B: shuffles an (cols x rows) grid and draws it. It fills two index arrays in the
   gBufDCFC buffer (the x indices and, at +0x280, the y indices) with 0..cols-1 and 0..rows-1,
   Fisher-Yates shuffles each with RandomRange, then for each row k and column i draws the cell
   at (x + xs[i], y + ys[j]) (j wrapping at rows) through the far pointers ds:8072 and ds:809E,
   with gStatusLine set to attr1 for the first call and attr2 for the second. Called by the 1204
   animation. */
/*@ ovl5:1E1B draft */ void ShuffleGrid(int x, int y, unsigned cols, unsigned rows, int attr1, int attr2)
{
    int far *xs;
    int far *ys;
    int swapped;
    int k;
    int pick;
    int j;
    int saved;
    register int i;
    register int cy;

    xs = gBufDCFC;
    ys = (int far *)((char far *)xs + 0x280);
    for (i = 0; i < cols; i++)
        xs[i] = i;
    for (i = 0; i < rows; i++)
        ys[i] = i;
    for (i = 0; i < cols; i++) {
        pick = RandomRange(0, cols - 1);
        swapped = xs[pick];
        xs[pick] = xs[i];
        xs[i] = swapped;
    }
    for (i = 0; i < rows; i++) {
        pick = RandomRange(0, rows - 1);
        swapped = ys[pick];
        ys[pick] = ys[i];
        ys[i] = swapped;
    }
    saved = gStatusLine;
    for (k = 0; k < rows; k++) {
        j = k;
        for (i = 0; i < cols; i++) {
            pick = x + xs[i];
            cy = y + ys[j];
            j++;
            if (j >= rows)
                j = 0;
            gStatusLine = attr1;
            swapped = gFn8072(pick, cy);
            gStatusLine = attr2;
            gFn809E(pick, cy, swapped);
        }
    }
    gStatusLine = saved;
}
