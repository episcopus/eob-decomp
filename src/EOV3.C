/* Overlay 3 of EOB.EXE (6961 bytes, 23 functions, the map and level overlay). It loads a wall set
   .DAT (ovl3:004F), slices map icon cells (ovl3:03A2, 0624, 069E, 0718), builds the ds:B899 item
   list (ovl3:0951), loads and unpacks a level state from LEVELS.TMP (ovl3:09AA, 109D, 1147) and
   packs and saves it (ovl3:0FD1, 11AF), ages and advances the character timers (ovl3:123D, 12F1)
   and resets the level (ovl3:19F7). The entry at 210C:0039 (ovl3:15B9) loads a saved game. 17 of
   the 23 functions are decompiled to L2, ovl3:03A2 is a blocked draft, and 01C8, 07A5, 0A7C,
   1368, 15B9 are stubs. Placeholder names where no evidence. */

/*@flags -ml -O -Z */

#include <dos.h>

extern void farfree(void far *block); /*@ 0000:71EE */
extern void far *gEFD4[]; /*@ ds:EFD4 */
extern int gF538; /*@ ds:F538 */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void *memset(void far *s, int c, unsigned n); /*@ 0000:6384 */
extern void far *gSegDF4E; /*@ ds:DF4E */
extern void far *gE062; /*@ ds:E062 */
extern char gB894[]; /*@ ds:B894 */
extern int gFBD0; /*@ ds:FBD0 */
extern void far *gBufDCFC; /*@ ds:DCFC */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */

struct Obj28 {              /* 28-byte records, 30 of them at ds:8A82 (src/E08E6.C) */
    char b0;                /* +0 */
    signed char kind;       /* +1 */
    int id;                 /* +2 */
    char pad04[6];          /* +4 */
    char m0A;               /* +A: cleared by ovl3:19F7 */
    char pad0B[0x1c - 0x0b];/* +B */
};

extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern unsigned long g8492; /*@ ds:8492 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void RearmCharEffects(void); /*@ 1414:0B2E */
extern signed char g194B[]; /*@ ds:194B */
extern int g5C74; /*@ ds:5C74 */
extern signed char g5C76; /*@ ds:5C76 */
extern char g5C77; /*@ ds:5C77 */

struct DDRec {              /* 18-byte effect records, 10 of them at ds:DD2C (src/E1414.C) */
    char active;            /* +0: 1 or 2 when live */
    char pad01[7];
    int m08;                /* +8: an index into the ds:8DD2 records */
    char pad0A[8];
};

struct Entry14 {            /* 14-byte records at ds:8DD2 (src/E0D1C.C) */
    char pad00[3];
    unsigned char m03;      /* +3: its low two bits are cleared here */
    char pad04[0xa];
};

extern struct DDRec gDD2C[]; /*@ ds:DD2C */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */

struct CharRec {            /* 243-byte character records at ds:F580 (src/E08E6.C) */
    char pad00[0xad];
    unsigned long timer[10]; /* +0xAD: ten 32-bit countdowns */
    char padD5[0xf3 - 0xd5];
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern void LoadLevelState(int action); /*@ ovl3:1147 */
extern long SeekFile(int index, long offset, int whence); /*@ 1C0E:06E5 */
extern void far *gSegEFC2; /*@ ds:EFC2 */
extern void far *f1BA8_0072(void far *p); /*@ 1BA8:0072 */
extern long FileWrite(int index, char far *buf, unsigned long len); /*@ 1C0E:0578 */
extern int CompressLcw(char far *src, char far *dst, unsigned len); /*@ 19FC:0008 */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern void WaitKeyEvent(void); /*@ 1AAD:05D3 */
extern void ShutdownGame(void); /*@ 07C8:058A */
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

struct ObjRec {             /* nine-byte object records at ds:B894 (src/E0D1C.C) */
    void far *ptr;          /* +0: the far pointer ovl3:09AA fills */
    char rest[5];           /* +4 */
};
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
/* ovl3:09AA: loads an object table. It clears the 0x2400 bytes at gB894, opens `name`, reads 6
   bytes into gB894 and 0x1000 bytes into gSegDF4E and closes it. When bit gFBC8-1 of gFBD0 is set
   it hands gFBC8 to the sibling ovl3:1147; otherwise it copies the 0x400 far pointers of gSegDF4E
   into the +0 field of the gB894 records. Called by ovl3:0A7C. */
/*@ ovl3:09AA */ void LoadObjects(char far *name)
{
    int handle;
    char far *src;
    struct ObjRec far *dst;
    int i;

    memset(gB894, 0, 0x2400);
    handle = FileOpen(name, 1);
    FileRead(handle, (void far *)gB894, 6L);
    FileRead(handle, gSegDF4E, 0x1000L);
    FileClose(handle);
    if (gFBD0 & (1 << (gFBC8 - 1))) {
        LoadLevelState(gFBC8);
    } else {
        src = (char far *)gSegDF4E;
        dst = (struct ObjRec far *)gB894;
        for (i = 0; i < 0x400; i++) {
            dst->ptr = *(void far * far *)src;
            src += 4;
            dst = (struct ObjRec far *)((unsigned long)dst + 9);
        }
    }
}
/*@ ovl3:0A7C stub */ void Ovl3_0A7C(void) {}
/* ovl3:0FD1: packs the level state into `buf`. It copies the four gB894 bytes of each of the 0x400
   records into gBufDCFC, compresses gBufDCFC (0x1000 bytes) into `buf` with CompressLcw and, when
   the stream is longer than 0x4B0, prints "Map compression failure: size of map = %d" and shuts the
   game down. It finishes by Unpack0ing gObjs into `buf`+0x4B0. Called by ovl3:11AF. */
/*@ ovl3:0FD1 */ void PackLevelState(void far *buf)
{
    char far *dst;
    char far *src;
    int i;
    int j;
    int len;

    dst = (char far *)buf + 0x4b0;
    src = (char far *)gBufDCFC;
    for (i = 0; i < 0x400; i++)
        for (j = 0; j < 4; j++)
            *src++ = gB894[i * 9 + j];
    len = CompressLcw((char far *)gBufDCFC, (char far *)buf, 0x1000);
    if (len > 0x4b0) {
        g8115 = 0;
        sprintf(gFileNameBuf, "Map compression failure: size of map = %d", len);
        gDrawText(gFileNameBuf, 0, 0, 0xf, 0);
        WaitKeyEvent();
        ShutdownGame();
    }
    Unpack0(gObjs, dst, 0x348L);
}
/* ovl3:109D: unpacks a level state. `buf` is the 0x7F8-byte level state ovl3:1147 read. It
   Unpack4s 0x1000 bytes of `buf` into gBufDCFC and Unpack0s the 0x348 bytes at `buf`+0x4B0 into
   gObjs, copies the four gBufDCFC bytes of each of the 0x400 gB894 records into its +0 field, then
   bumps the +4 byte of every gB894 record the 30 gObjs objects index by id. Called by ovl3:1147. */
/*@ ovl3:109D */ void UnpackLevelState(void far *buf)
{
    char far *dst;
    char far *src;
    int i;
    int j;

    src = (char far *)gBufDCFC;
    dst = (char far *)buf + 0x4b0;
    Unpack4(buf, src, 0x1000);
    for (i = 0; i < 0x400; i++)
        for (j = 0; j < 4; j++)
            gB894[i * 9 + j] = *src++;
    Unpack0(dst, gObjs, 0x348L);
    for (i = 0; i < 0x1e; i++)
        gB894[gObjs[i].id * 9 + 4]++;
}
/* ovl3:1147: reads the level state `action` from LEVELS.TMP: opens the file, seeks to
   (action-1)*0x7F8 and reads 0x7F8 bytes into gSegEFC2, closes it and hands gSegEFC2 to ovl3:109D.
   Called by ovl3:09AA. */
/*@ ovl3:1147 */ void LoadLevelState(int action)
{
    int handle;

    handle = FileOpen("LEVELS.TMP", 1);
    SeekFile(handle, (long)(action - 1) * 0x7f8, 0);
    FileRead(handle, gSegEFC2, 0x7f8L);
    FileClose(handle);
    UnpackLevelState(gSegEFC2);
}
/* ovl3:11AF: saves the level state `level` to LEVELS.TMP: normalizes gSegEFC2+0xC800 into a far
   pointer, hands it to ovl3:0FD1, opens LEVELS.TMP for writing (mode 3), seeks to
   (level-1)*0x7F8, writes 0x7F8 bytes from the buffer, closes it and sets bit level-1 of gFBD0.
   Called by overlay 0 (122E:1AAB). */
/*@ ovl3:11AF */ void SaveLevelState(int level)
{
    char far *state;
    int handle;

    state = (char far *)f1BA8_0072((char far *)gSegEFC2 + 0xc800);
    PackLevelState((void far *)state);
    handle = FileOpen("LEVELS.TMP", 3);
    SeekFile(handle, (long)(level - 1) * 0x7f8, 0);
    FileWrite(handle, state, 0x7f8L);
    FileClose(handle);
    gFBD0 |= 1 << (level - 1);
}/* ovl3:123D: ages the ten 32-bit countdowns at gChars+0xAD for every character that passes
   f0D1C_0670(i, 1): a countdown greater than the tick counter g8492 loses g8492, and a smaller
   nonzero one is set to 1. Called by ovl3:1368. */
/*@ ovl3:123D */ void AgeCharTimers(void)
{
    struct CharRec far *rec;
    int i;
    int j;

    for (rec = gChars, i = 0; i < 6; i++, rec++) {
        if (f0D1C_0670(i, 1)) {
            for (j = 0; j < 0xa; j++) {
                if (rec->timer[j] > g8492)
                    rec->timer[j] -= g8492;
                else if (rec->timer[j])
                    rec->timer[j] = 1;
            }
        }
    }
}
/* ovl3:12F1: the counterpart of ovl3:123D: adds the tick counter g8492 to every nonzero countdown
   at gChars+0xAD for each character that passes f0D1C_0670(i, 1), then calls RearmCharEffects.
   Called by ovl3:1368 and ovl3:15B9. */
/*@ ovl3:12F1 */ void AdvanceCharTimers(void)
{
    struct CharRec far *rec;
    int i;
    int j;

    for (rec = gChars, i = 0; i < 6; i++, rec++) {
        if (f0D1C_0670(i, 1)) {
            for (j = 0; j < 0xa; j++) {
                if (rec->timer[j])
                    rec->timer[j] += g8492;
            }
        }
    }
    RearmCharEffects();
}
/*@ ovl3:1368 stub */ void Ovl3_1368(void) {}
/*@ ovl3:15B9 stub */ void Ovl3_15B9(void) {}
/* ovl3:19F7: resets the level. It clears the m0A byte of the 30 gObjs objects, clears every live
   ds:DD2C effect record (disarming the low two bits of its item's m03 when it was active), then,
   when g5C77 is set, walks the gB894 record of character g5C74 adding the mode byte g5C76 to the
   two bytes of its slot until the g194B flag has bit 0x30, and clears g5C77. Called by ovl3:1368
   and ovl3:15B9. */
/*@ ovl3:19F7 */ void ResetLevel(void)
{
    int i;
    int who;
    int sel;
    int slot;

    for (i = 0; i < 0x1e; i++)
        gObjs[i].m0A = 0;
    for (i = 0; i < 0xa; i++) {
        if (gDD2C[i].active == 1) {
            gTable8DD2[gDD2C[i].m08].m03 &= 3;
            gDD2C[i].active = 0;
        }
        if (gDD2C[i].active == 2)
            gDD2C[i].active = 0;
    }
    if (g5C77) {
        who = g5C74;
        sel = g5C76;
        slot = (g194B[gB894[who * 9]] & 8) ? 0 : 1;
        while ((g194B[gB894[who * 9 + slot]] & 0x30) == 0) {
            gB894[who * 9 + slot] += sel;
            gB894[who * 9 + slot + 2] += sel;
        }
        g5C77 = 0;
    }
}
