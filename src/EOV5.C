/* Unit at overlay 5 of EOB.EXE (8094 bytes, 31 functions): the character/item overlay the map
   loop opens on the "inventory" key. Its entry ovl5:1A82 dispatches a menu (modes 0..7) to the
   seven panels 0924/0B58/0EB9/0F77/102C/1087/10A6/119E, and modes 8, 9 and 0xa to the two long
   animations 1204/17C8 and the "find the word" panel (ovl2:09ED). It reads the six 243-byte
   (0xF3) character records at ds:F580 (struct CharRec), draws the item grid through 0455 and the
   ds:7B89/7B94 position tables, and its 160A prints the winning-party password. Names are
   placeholders until evidence names them. */

/*@flags -ml -O -Z */

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
    char flags;             /* +01: status bits (only bit 0 is read here) */    char name[0x0B];        /* +02: the character's name, NUL padded */
    char pad0D[0x1B - 0x0D];
    signed char hp;         /* +1B: current hit points */
    signed char maxhp;      /* +1C: maximum hit points */
    char pad1D[0x22 - 0x1D];
    unsigned char m22;      /* +22: compared with 0xf7 by 0AB1 (src/E0D1C.C:0415 uses it too) */
    char pad23[0x33 - 0x23];
    long m33;               /* +33: a value the record swap at 070E preserves */
    char pad37[0x77 - 0x37];
    int m77[25];            /* +77: 25 item-slot words (the carried and equipped items) */
    char padA9[0xE3 - 0xA9];
    unsigned long mE3;      /* +E3: effect flags (0C9D clears bit 7 of the low word) */
    char padE7[0xF3 - 0xE7];
};

struct Entry14 {            /* the 14-byte item records at ds:8DD2 (src/E0D1C.C) */
    char pad00;
    signed char icon;       /* +1 */
    char pad02[0xE - 2];
};

extern int g8115; /*@ ds:8115 */
extern signed char gF57E; /*@ ds:F57E */
extern signed char gF53A; /*@ ds:F53A */
extern signed char gFB4C; /*@ ds:FB4C */
extern char gFontHeight1A47; /*@ ds:8101 */
extern int gTextRow; /*@ ds:FE54 */
extern int gTextTop; /*@ ds:FE66 */
extern int gCursorX; /*@ ds:8138 */
extern int gCursorY; /*@ ds:813A */
extern int g8140; /*@ ds:8140 */
extern int g8142; /*@ ds:8142 */
extern int gMouseDisabled; /*@ ds:814A */
extern int gEFC6; /*@ ds:EFC6 */
extern int gFE42; /*@ ds:FE42 */
extern char gScrollOn; /*@ ds:8110 */
extern unsigned long gFC04; /*@ ds:FC04 */
extern int gFBCC; /*@ ds:FBCC */
extern int gSoundVolume; /*@ ds:FBCA */
extern signed char g1A1B[]; /*@ ds:1A1B */
extern char gB899[]; /*@ ds:B899 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
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
extern char gText7D77[]; /*@ ds:7D77 */
extern char gFileNameBuf[]; /*@ ds:FE6E */

extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
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
extern int IsPointInsideRect(int x, int y, int left, int top, int right, int bottom); /*@ 082C:0613 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern int WaitKeyEvent(void); /*@ 1AAD:05D3 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern void farfree(void far *p); /*@ 0000:71EE */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int RemoveCarriedItems(int field2, int fieldB); /*@ 0D1C:01CA */
extern void f0D1C_035A(int who); /*@ 0D1C:035A */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void ClearCharEffectSlot(int who, int what); /*@ 1414:0C2D */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */

/* siblings not decompiled yet: their real signatures, so callers compile */
void Ovl5_00A1(int mode);
void Ovl5_025F(void);
void Ovl5_0455(int offset, int count, void far *names);
void Ovl5_067B(void);
void Ovl5_070E(int who);
void Ovl5_0828(int who, int which, int head, int other);
void Ovl5_0924(void);
void Ovl5_09C5(void);
void Ovl5_0BBE(void);
void Ovl5_0C9D(void);
void Ovl5_0DBB(void);
void Ovl5_0EB9(void);
void Ovl5_0F77(void);
void Ovl5_10A6(void);
void Ovl5_119E(void);
void Ovl5_1204(void);
void Ovl5_17C8(void);
void Ovl5_1A82(int mode);
void Ovl5_1CDD(int n);
void Ovl5_1E1B(int a, int b, int x, int y, int c, int d);

void PrintTableString(int index, int flag);

/* ovl5:0000: draws sprite cell `index` of the ds:7B2B table out of the overlay buffer gOverlayBuf
   at (x, y), moved left by the returned cell's byte +2 (times four) and up by its byte +1, into
   region 5. Sets g8115 for the draw and frees the sliced cell. Called by the avatar panel 00A1. */
/*@ ovl5:0000 */ void DrawSheetCell(int index, int x, int y)
{
    void far *cell;
    int c1;
    int c2;
    int c3;
    int c4;
    int i;

    g8115 = 2;
    i = index * 4;
    c1 = gCellDef7B2B[i];
    c2 = gCellDef7B2B[i + 1];
    c3 = gCellDef7B2B[i + 2];
    c4 = gCellDef7B2B[i + 3];
    cell = gGetSpriteCell(c1, c2, c3, c4, gOverlayBuf);
    g8115 = 0;
    y -= ((struct Cell far *)cell)->b1;
    x -= (((struct Cell far *)cell)->b2 << 3) >> 1;
    f1A29_000F(0, cell, x, y, 5);
    farfree(cell);
}

/*@ ovl5:00A1 stub */ void Ovl5_00A1(int mode) {}
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

/*@ ovl5:025F stub */ void Ovl5_025F(void) {}
/* ovl5:03A9: draws string `index` (1-based) of the string-offset table pointed at by gSegDF4E
   (the word at index-1 is an offset from that base), then, when `flag` is nonzero, the modal
   message panel 025F. The strings are the overlay's own yes/no menu labels. */
/*@ ovl5:03A9 */ void PrintTableString(int index, int flag)
{
    char far *p;
    int off;

    p = gSegDF4E;
    index--;
    off = ((int far *)p)[index];
    DrawText(gSegDF4E + off);
    if (flag)
        Ovl5_025F();
}

/* ovl5:03EF: one row of the overlay's menu: fills a bevelled panel (x, y)-(x+0x5e, y+8) in the
   three panel colours and draws `text` centred in it (three pixels per character, at y+2) in
   `colour`. Called by the grid 0455 for every entry it draws. */
/*@ ovl5:03EF */ void DrawMenuRow(int x, int y, int colour, char far *text)
{
    DrawFilledPanel(x, y, x + 0x5e, y + 8, gFB4C, gF53A, gF57E);
    gDrawText(text, x + 0x30 - strlen(text) * 3, y + 2, colour, 0);
}

/*@ ovl5:0455 stub */ void Ovl5_0455(int offset, int count, void far *names) {}
/*@ ovl5:067B stub */ void Ovl5_067B(void) {}
/*@ ovl5:070E stub */ void Ovl5_070E(int who) {}
/*@ ovl5:0828 stub */ void Ovl5_0828(int who, int which, int head, int other) {}
/* ovl5:08D2: turns item `n` into a carried item (CopyItemToFreeSlot) and queues it on the list
   head gB899 for the current sound page (gSoundVolume) with a random page sound. Called by the
   panels that add an item to a character. */
/*@ ovl5:08D2 */ void QueueItem(int item)
{
    int sound;

    item = CopyItemToFreeSlot(item);
    sound = g1A1B[gFBCC * 4 + RollDice(1, 2, -1)];
    InsertListItem(&gB899[gSoundVolume * 9], gSoundVolume, item, sound);
}

/*@ ovl5:0924 stub */ void Ovl5_0924(void) {}
/*@ ovl5:09C5 stub */ void Ovl5_09C5(void) {}
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
        Ovl5_09C5();
        done = 1;
    }
    if (!(gFC04 & 0x80000L))
        done |= CheckChar0F7();
    if (!(gFC04 & 0x100000L))
        done |= CheckRemove6();
    if (!done) {
        DrawText(gText7D77);
        Ovl5_025F();
    }
}

/*@ ovl5:0BBE stub */ void Ovl5_0BBE(void) {}
/*@ ovl5:0C9D stub */ void Ovl5_0C9D(void) {}
/*@ ovl5:0DBB stub */ void Ovl5_0DBB(void) {}
/*@ ovl5:0EB9 stub */ void Ovl5_0EB9(void) {}
/*@ ovl5:0F77 stub */ void Ovl5_0F77(void) {}
/*@ ovl5:102C stub */ void Ovl5_102C(void) {}
/*@ ovl5:1087 stub */ void Ovl5_1087(void) {}
/*@ ovl5:10A6 stub */ void Ovl5_10A6(void) {}
/*@ ovl5:119E stub */ void Ovl5_119E(void) {}
/*@ ovl5:1204 stub */ void Ovl5_1204(void) {}
/*@ ovl5:160A stub */ void Ovl5_160A(void) {}
/*@ ovl5:17C8 stub */ void Ovl5_17C8(void) {}
/*@ ovl5:1A82 stub */ void Ovl5_1A82(int mode) {}
/*@ ovl5:1CDD stub */ void Ovl5_1CDD(int n) {}
/*@ ovl5:1E1B stub */ void Ovl5_1E1B(int a, int b, int x, int y, int c, int d) {}
