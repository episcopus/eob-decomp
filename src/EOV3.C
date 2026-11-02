/* Overlay 3 of EOB.EXE (6961 bytes, 23 functions). Purpose unknown at this point: the unit owns the
   wall set loader (ovl3:004F), the free list at ds:EFD4, the map/level screen setup and the table of
   0x34-byte records at ds:E06A. Placeholder names. */

/*@flags -ml -O -Z */

#include <dos.h>

extern void farfree(void far *block); /*@ 0000:71EE */
extern void far *gEFD4[]; /*@ ds:EFD4 */
extern int gF538; /*@ ds:F538 */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *gSegDF4E; /*@ ds:DF4E */
extern void far *gE062; /*@ ds:E062 */
extern long gFB5E[]; /*@ ds:FB5E */

struct Triple6 {            /* six bytes, the record InitGameTables clears */
    long v;
    char c[2];
};

extern struct Triple6 gA99C[]; /*@ ds:A99C */
extern void far *gB802[]; /*@ ds:B802 */
extern int g6930[]; /*@ ds:6930 */
extern int g69C0[]; /*@ ds:69C0 */
extern int g6A50[]; /*@ ds:6A50 */
extern int g8115; /*@ ds:8115 */
extern char gOverlayBuf[]; /*@ ds:113F */

struct Rect {               /* a four-word cell rectangle (x, y, w, h) */
    int x;
    int y;
    int w;
    int h;
};

extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern int gFBC8; /*@ ds:FBC8 */
extern char gB899[]; /*@ ds:B899 */        /* the nine-byte record list head (src/E0ACD.C) */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */

struct ItemRecord {         /* the 14-byte records at ds:8DD0 (src/E0BD2.C) */
    signed char m00;        /* +0 */
    signed char m01;        /* +1 */
    unsigned char flags;    /* +2 */
    signed char icon;       /* +3 */
    signed char m02;        /* +4 */
    signed char m03;        /* +5 */
    int m04;                /* +6: the key, -1 when free */
    int m06;                /* +8 */
    int m08;                /* +0xa */
    signed char m0A;        /* +0xc: gFBC8 or -1 */
    signed char m0B;        /* +0xd */
};

extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern unsigned char g6840[]; /*@ ds:6840 */
extern unsigned char g68A0[]; /*@ ds:68A0 */
extern unsigned char g6900[]; /*@ ds:6900 */
extern signed char far gMapTiles[]; /*@ 1F1D:0000 */

#define MAPSHEET ((void far *)((char far *)MK_FP(0x1d8, 0) + (gMapTiles[gFBC8 - 1] << 5)))

struct WallDec {            /* a 52-byte wall set decoration */
    char idx[0xc];
    int field0c[10];
    int field20[10];
};

/* ovl3:0000: frees every far pointer in the 200-entry table at ds:EFD4 (farfree), clears each slot
   and resets the counter ds:F538. Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:0000 */ void FreePtrTable(void)
{
    int i;

    for (i = 0; i < 0xc8; i++) {
        if (gEFD4[i]) {
            farfree(gEFD4[i]);
            gEFD4[i] = 0;
        }
    }
    gF538 = 0;
}

/* ovl3:004F: loads a wall set .DAT (whitepaper 4.3). It opens the screen file through
   LoadOverlayScreen, then opens the data file and reads a u16 decoration count and that many
   52-byte decorations into the scratch area at gSegDF4E+0x3E80; each decoration's two 10-word
   tables at +0xC and +0x20 are then stored to themselves word by word (the form the original
   compiles to, a load and a store of the same word). It points gE062 just past the decorations,
   reads a u16 rectangle count and that many 8-byte rectangles, walks the rectangle words the same
   way and closes the file. Called by ovl3:0A7C. */
/*@ ovl3:004F */ void LoadWallSet(char far *screen, char far *file)
{
    long decsBytes;
    int nDecs;
    int nRects;
    int handle;
    struct WallDec far *dec;
    int far *rects;
    int i;
    int j;

    LoadOverlayScreen(screen, 5, 3);
    handle = FileOpen(file, 1);
    FileRead(handle, (void far *)&nDecs, 2L);
    decsBytes = (long)nDecs * 0x34;
    FileRead(handle, (void far *)((char far *)gSegDF4E + 0x3e80), decsBytes);
    dec = (struct WallDec far *)((char far *)gSegDF4E + 0x3e80);
    for (i = 0; i < nDecs; i++) {
        for (j = 0; j < 10; j++) {
            dec->field0c[j] = dec->field0c[j];
            dec->field20[j] = dec->field20[j];
        }
        dec++;
    }
    gE062 = (void far *)((char far *)gSegDF4E + 0x3e80 + decsBytes);
    rects = (int far *)gE062;
    FileRead(handle, (void far *)&nRects, 2L);
    FileRead(handle, gE062, (long)nRects << 3);
    for (i = 0; i < nRects * 4; i++) {
        *rects = *rects;
        rects++;
    }
    FileClose(handle);
}

/* ovl3:01AA: Purpose unknown. Returns 1 when bit `bit` (the second argument, a char) is set in the
   signed char `value` (the first argument), else 0. */
/*@ ovl3:01AA */ int IsBitSet(signed char value, char bit)
{
    return (value & (1 << bit)) ? 1 : 0;
}
/*@ ovl3:01C8 stub */ void Ovl3_01C8(void) {}
/* ovl3:031A: frees and clears the six far pointers in the tables at ds:FB5E (stride 4) and the six
   in ds:A99C.v (stride 6). The counterpart of the allocation InitGameTables (ovl2:0496) makes.
   Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:031A */ void FreeEffectTables(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (gFB5E[i]) {
            farfree((void far *)gFB5E[i]);
            gFB5E[i] = 0;
        }
        if (gA99C[i].v) {
            farfree((void far *)gA99C[i].v);
            gA99C[i].v = 0;
        }
    }
}
/* ovl3:03A2: loads the overlay screen at ds:6A98 and slices map icon cells out of the map sheet
   (segment 0x1D8, offset from the tile byte table gMapTiles[gFBC8-1] shifted left 5). For `srcA`
   (when not -1) it slices three cells from the 4-byte rectangles at ds:6840 into gFB5E starting at
   `dstA` and three more from ds:68A0 into gA99C[dstA].v, and copies the two gA99C[dstA].c bytes
   from ds:6900. For `srcB` (when not -1) it does the same at `dstB`. Clears g8115. Called by
   ovl3:0A7C. blocked: cmatch cannot explain the far object's offset: the table at 1F1D:0000 is
   indexed [gFBC8-1], which the compiler folds to `es:[bx-1]` (disp16 FFFF), so our build's
   displacement is FAR_BASE-1 (7FFF) and cmatch only explains a displacement >= FAR_BASE
   (cmatch.py:370). The honest annotation (1F1D:0000) and index are kept. */
/*@ ovl3:03A2 draft */ void LoadMapCells(int srcA, int dstA, int srcB, int dstB)
{
    int i;
    int j;

    LoadOverlayScreen("DOOR", 5, 3);
    if (srcA == -1) goto second;
    for (i = 0; i < 3; i++) {
        j = srcA * 0xc + i * 4;
        gFB5E[dstA + i] = (long)gGetSpriteCell(g6840[j], g6840[j + 1], g6840[j + 2],
            g6840[j + 3], MAPSHEET);
        gA99C[dstA + i].v = (long)gGetSpriteCell(g68A0[j], g68A0[j + 1], g68A0[j + 2],
            g68A0[j + 3], MAPSHEET);
        j = srcA * 6 + i * 2;
        gA99C[dstA + i].c[0] = g6900[j];
        gA99C[dstA + i].c[1] = g6900[j + 1];
    }
second:
    if (srcB == -1) goto done;
    for (i = 0; i < 3; i++) {
        j = srcB * 0xc + i * 4;
        gFB5E[dstB + i] = (long)gGetSpriteCell(g6840[j], g6840[j + 1], g6840[j + 2],
            g6840[j + 3], MAPSHEET);
        gA99C[dstB + i].v = (long)gGetSpriteCell(g68A0[j], g68A0[j + 1], g68A0[j + 2],
            g68A0[j + 3], MAPSHEET);
        j = srcA * 6 + i * 2;
        gA99C[dstB + i].c[0] = g6900[j];
        gA99C[dstB + i].c[1] = g6900[j + 1];
    }
done:
    g8115 = 0;
}
/* ovl3:05D2: frees and clears the `count` map icon pointers in gB802 starting at index `base`.
   Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:05D2 */ void FreeMapIcons(int base, int count)
{
    int i;
    int end;

    end = base + count;
    for (i = base; i < end; i++) {
        if (gB802[i]) {
            farfree(gB802[i]);
            gB802[i] = 0;
        }
    }
}
/* ovl3:0624: loads the overlay screen `screen` and slices six map icon cells out of gOverlayBuf
   (ds:113F), one per 4-word rectangle in the table at ds:6930, into gB802 starting at index
   `dest`. Clears g8115. No direct caller found. */
/*@ ovl3:0624 */ void LoadMapIcons6(char far *screen, int dest)
{
    int n;
    int i;
    int si;

    LoadOverlayScreen(screen, 5, 3);
    n = 6;
    for (i = 0, si = 0; i < n; si += 4, i++)
        gB802[dest + i] = gGetSpriteCell(g6930[si], g6930[si + 1], g6930[si + 2],
            g6930[si + 3], gOverlayBuf);
    g8115 = 0;
}
/* ovl3:069E: the same as ovl3:0624 but slices the six cells from the 4-word rectangle table at
   ds:69C0 instead of ds:6930. No direct caller found. */
/*@ ovl3:069E */ void LoadMapIcons6b(char far *screen, int dest)
{
    int n;
    int i;
    int si;

    LoadOverlayScreen(screen, 5, 3);
    n = 6;
    for (i = 0, si = 0; i < n; si += 4, i++)
        gB802[dest + i] = gGetSpriteCell(g69C0[si], g69C0[si + 1], g69C0[si + 2],
            g69C0[si + 3], gOverlayBuf);
    g8115 = 0;
}
/* ovl3:0718: builds the screen name `name` + "1" through sprintf("%s1"), loads it through
   LoadOverlayScreen and slices six map icon cells out of gOverlayBuf using the 4-word rectangle
   table at ds:6A50 into gB802 starting at index `dest`. Clears g8115. No direct caller found. */
/*@ ovl3:0718 */ void LoadNamedIcons(char far *name, int dest)
{
    char buf[0x28];
    int i;
    int si;

    sprintf(buf, "%s1", name);
    LoadOverlayScreen(buf, 5, 3);
    for (i = 0, si = 0; i < 6; si += 4, i++)
        gB802[dest + i] = gGetSpriteCell(g6A50[si], g6A50[si + 1], g6A50[si + 2],
            g6A50[si + 3], gOverlayBuf);
    g8115 = 0;
}
/*@ ovl3:07A5 stub */ void Ovl3_07A5(void) {}
/* ovl3:0951: rebuilds the ds:B899 item list: walks the 500 items at ds:8DD0 and, for every item
   whose m0A is the current action gFBC8 and whose key m04 is positive, inserts it through
   InsertListItem. Called by ovl3:0A7C. */
/*@ ovl3:0951 */ void RebuildItemList(void)
{
    struct ItemRecord far *rec;
    int i;

    for (rec = gItems, i = 0; i < 0x1f4; rec++, i++) {
        if (rec->m0A == gFBC8 && rec->m04 > 0)
            InsertListItem((void far *)(gB899 + rec->m04 * 9), rec->m04, i, rec->m03);
    }
}
/*@ ovl3:09AA stub */ void Ovl3_09AA(void) {}
/*@ ovl3:0A7C stub */ void Ovl3_0A7C(void) {}
/*@ ovl3:0FD1 stub */ void Ovl3_0FD1(void) {}
/*@ ovl3:109D stub */ void Ovl3_109D(void) {}
/*@ ovl3:1147 stub */ void Ovl3_1147(void) {}
/*@ ovl3:11AF stub */ void Ovl3_11AF(void) {}
/*@ ovl3:123D stub */ void Ovl3_123D(void) {}
/*@ ovl3:12F1 stub */ void Ovl3_12F1(void) {}
/*@ ovl3:1368 stub */ void Ovl3_1368(void) {}
/*@ ovl3:15B9 stub */ void Ovl3_15B9(void) {}
/*@ ovl3:19F7 stub */ void Ovl3_19F7(void) {}
