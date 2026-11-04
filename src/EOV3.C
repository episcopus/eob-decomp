/* Overlay 3 of EOB.EXE (6961 bytes, 23 functions, the map and level overlay). It loads a wall set
   .DAT (ovl3:004F), slices map icon cells (ovl3:03A2, 0624, 069E, 0718), builds the ds:B899 item
   list (ovl3:0951) and mounts a level's map screen (ovl3:0A7C), which loads an object table
   (ovl3:09AA) and, for a level that was saved, reads its level-state slot from LEVELS.TMP
   (ovl3:1147) and unpacks it (ovl3:109D); it packs the level state (ovl3:0FD1) and saves it to
   LEVELS.TMP (ovl3:11AF), ages and advances the character timers (ovl3:123D, 12F1) and resets the
   level (ovl3:19F7). The entry at 210C:0039 (ovl3:15B9) loads a saved game. All 23 functions are
   decompiled to L2. Placeholder names where no evidence. */

/*@flags -ml -O -Z */

#include <dos.h>

extern void farfree(void far *block); /*@ 0000:71EE */
extern struct RectKind far *gRectKinds[]; /*@ ds:EFD4, the per-kind size records (src/E122E.C:91) */
extern int gWallRecCount; /*@ ds:F538 */            /* the wall-record slot count RegisterWallObject advances; Reset by FreePtrTable */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void LoadPacked(char far *name, int srcKind, int dstKind); /*@ 1B82:0041 */
extern void SetPlaneTables(void far *t1, void far *t2); /*@ 17A6:037E */
extern void ExpandNibbles(void far *src, void far *dst); /*@ 17A6:0F1F */
extern void BuildMask(void far *p); /*@ 17A6:18EA */
extern void (far *gCopyToPlanar)(void far *buf, int a, int b); /*@ ds:808E */
extern char gWallPlaneTableA[16]; /*@ ds:F4D6 */       /* a 16-byte plane table (src/E17A6.ASM) */
extern char gWallPlaneTableB[16]; /*@ ds:FB4D */       /* a 16-byte plane table (src/E17A6.ASM) */
extern unsigned char far gSheetINVENT[]; /*@ 2067:0000 */  /* the INVENT sheet (src/E0ACD.C) */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */  /* the PLAYFLD sheet (src/E0ACD.C) */
extern int gTunesOn; /*@ ds:80E8 */            /* toggled by Ctrl-key handling (src/E1AAD.ASM) */
extern int gSoundOn; /*@ ds:80E6 */
extern char gBarGraphsOn; /*@ ds:F43C */
extern char gSpellItemTypes[]; /*@ ds:AD20 */         /* a 96-byte saved-game buffer (Purpose unknown) */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern void far *gWallBlocks; /*@ ds:DCA6 */   /* the far pointer f19EC_0002(7) returns (src/E07A9.C) */
extern int gGraphicsChoice; /*@ ds:8102 */            /* the video mode the game initialised (src/E082C.C) */
extern char gWallNameBuf[]; /*@ ds:FE2E */  /* the wall texture name kept between LoadWallTextures calls (ovl3:07A5) */
extern void far *(far *gGetSpriteCellRle)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void DrawViewRows(void); /*@ 2116:0034 */  /* overlay 4 entry, ovl4:1EDE */
extern void SetFont(char far *font); /*@ 1A47:0169 */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern void SetMouseItem(int item); /*@ 07A9:010C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void far *gFont6x6; /*@ ds:A9C0 */      /* the current 6x6 font (src/E07A9.C) */
extern char gGameLoadState; /*@ ds:A998 */           /* src/E0ACD.C */
extern signed char gViewToCellPos[]; /*@ ds:1A1B */  /* per-page sound table (src/E0ACD.C:131) */
extern int gPartyCell; /*@ ds:FBCA */
extern int gPartyDir; /*@ ds:FBCC */
extern int gFBCE; /*@ ds:FBCE */
extern int gMouseDisabled; /*@ ds:814A */
extern unsigned gBlockSizes[]; /*@ ds:6A8C */  /* the six EOBDATA.SAV block sizes (ovl3:1368 saves them in this order) */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern long FileRead(int handle, char far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void *memset(void far *s, int c, unsigned n); /*@ 0000:6384 */
extern void far *gStagingPage; /*@ ds:DF4E */
extern struct Cell8 far *gDecorationRects; /*@ ds:E062 */  /* the 8-byte map cell rectangles (struct Cell8 below) */
extern char gMaze[]; /*@ ds:B894 */
extern int gSavedLevels; /*@ ds:FBD0 */            /* the saved-level bit mask: bit level-1 is set by SaveLevelState and SaveGame and tested by LoadObjects and LoadMapScreen */
extern void far *gBufDCFC; /*@ ds:DCFC */
extern unsigned char gWallVmpSet[]; /*@ ds:1879 */  /* the four parallel per-kind tables at ds:1879..0x1990 */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */

struct Cell8 {              /* the 8-byte sprite rectangles gDecorationRects points at (ovl3:01C8, 03A2) */
    int w0;
    int w1;
    int w2;
    int w3;
};

struct WallRec {            /* a 0x34-byte slot at ds:E040 (ovl3:01C8): RegisterWallObject copies a
                               0x34-byte wall-set decoration into cells (+0x2a), so the decoration's
                               byte 0x0a lands at +0 of the following slot */
    signed char next;       /* +0: the copied decoration's byte 0x0a, read back as the chain link after
                               the slot is filled (then set to index+1) */
    char pad01[0x29];
    unsigned char cells[0xa]; /* +0x2a: the copied decoration's first ten bytes, ten map cell indices,
                                 0xFF = none */
};
extern struct WallRec gWallRecs[]; /*@ ds:E040 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */

struct Monster {              /* 28-byte records, 30 of them at ds:8A82 (src/E08E6.C) */
    char typeIndex;                /* +0 */
    signed char kind;       /* +1 */
    int cell;                 /* +2 */
    char pad04[6];          /* +4 */
    char m0A;               /* +A: cleared by ovl3:19F7 */
    char pad0B[0x1c - 0x0b];/* +B */
};

extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern unsigned long gGameTicks; /*@ ds:8492 */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern void RearmCharEffects(void); /*@ 1414:0B2E */
extern signed char gWallTypeFlags[]; /*@ ds:194B */
extern signed char gWallDecorHead[]; /*@ ds:18BF */  /* per-object-kind table (src/E122E.C) */
extern unsigned char gWallHandlerIndex[]; /*@ ds:1905 */     /* a 256-byte table gMaze indexes into (src/E0DE8.C) */
extern int gDragMember; /*@ ds:5C74 */
extern signed char gDoorMoveDir; /*@ ds:5C76 */
extern char gDragActive; /*@ ds:5C77 */

struct Projectile {              /* 18-byte effect records, 10 of them at ds:DD2C (src/E1414.C) */
    char active;            /* +0: 1 or 2 when live */
    char pad01[7];
    int item;                /* +8: an index into gItems */
    char pad0A[8];
};



extern struct Projectile gProjectiles[]; /*@ ds:DD2C */

struct CharRec {            /* 243-byte character records at ds:F580 (src/E08E6.C) */
    char pad00;
    unsigned char status;     /* +1: status bits (src/E08E6.C); 0 when the slot is empty */
    char pad02[0x22 - 0x02];
    signed char portraitIndex;          /* +0x22: the character's cell row/column (src/E0D1C.C) */
    char pad23[0x33 - 0x23];
    void far *portrait;       /* +0x33: the character's packed sprite buffer */
    char pad37[0xad - 0x37];
    unsigned long timers[10]; /* +0xAD: ten 32-bit countdowns */
    char padD5[0xf3 - 0xd5];
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern void LoadLevelState(int action); /*@ ovl3:1147 */
extern void ResetLevel(void); /*@ ovl3:19F7 */
extern long SeekFile(int index, long offset, int whence); /*@ 1C0E:06E5 */
extern void far *gArtPage; /*@ ds:EFC2 */
extern void far *NormalizePtr(void far *p); /*@ 1BA8:0072 */
extern long FileWrite(int index, char far *buf, unsigned long len); /*@ 1C0E:0578 */
extern int CompressLcw(char far *src, char far *dst, unsigned len); /*@ 19FC:0008 */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern void WaitKeyEvent(void); /*@ 1AAD:05D3 */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern struct SlotSize far *gDoorShapes[]; /*@ ds:FB5E (the per-column size records, src/E122E.C:106) */

struct SlotPos {            /* the six-byte slot-position records at ds:A99C (src/E122E.C:112) */
    struct SlotSize far *rec; /* +0: the far size record */
    unsigned char x;        /* +4 */
    unsigned char y;        /* +5 */
};
extern struct SlotPos gDoorButtons[]; /*@ ds:A99C */
extern void far *gLoadedMonsterShapes[]; /*@ ds:B802 */
extern int gMonsterRectsA[]; /*@ ds:6930 */
extern int gMonsterRectsB[]; /*@ ds:69C0 */
extern int gMonsterRectsC[]; /*@ ds:6A50 */
extern int gDrawPage; /*@ ds:8115 */
extern char gOverlayBuf[]; /*@ ds:113F */

struct Rect {               /* a four-word cell rectangle (x, y, w, h) */
    int x;
    int y;
    int w;
    int h;
};

extern void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *sheet); /*@ ds:8086 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern int gCurLevel; /*@ ds:FBC8 */
extern char gMazeItems[]; /*@ ds:B899 */        /* the nine-byte record list head (src/E0ACD.C) */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */

struct ItemRecord {         /* the 14-byte records at ds:8DD0 (src/E0BD2.C) */
    signed char nameIndex;        /* +0 */
    signed char identifiedNameIndex;        /* +1 */
    unsigned char flags;    /* +2 */
    signed char icon;       /* +3 */
    signed char type;        /* +4 */
    signed char cellPos;        /* +5: the sound InsertListItem plays for the item (src/E0BD2.C:420) */
    int key;                /* +6: the key, -1 when free */
    int prevLink;                /* +8 */
    int nextLink;                /* +0xa */
    signed char m0A;        /* +0xc: gCurLevel or -1 */
    signed char value;        /* +0xd */
};

extern struct ItemRecord gItems[]; /*@ ds:8DD0 */

struct ObjRec {             /* nine-byte object records at ds:B894 (src/E0D1C.C) */
    void far *ptr;          /* +0: the far pointer ovl3:09AA fills */
    char rest[5];           /* +4 */
};
extern unsigned char gDoorRects[]; /*@ ds:6840 */
extern unsigned char gDoorButtonRects[]; /*@ ds:68A0 */
extern unsigned char gDoorButtonPos[]; /*@ ds:6900 */
extern signed char far gMapTiles[]; /*@ 1F1D:0000 */  /* the map tile byte table, read by the map screen
                                                         sheet computation (the overlay stores the
                                                         segment-table index 01D0, which resolves to
                                                         paragraph 1F1D; src/EOV5.C calls it gSeg1D0) */

#define MAPSHEET ((void far *)((char far *)MK_FP(0x1d8, 0) + (gMapTiles[gCurLevel - 1] << 5)))

extern int gTimerEnableFlags; /*@ ds:849E */
extern char gLevelTimerMode; /*@ ds:F51B */
extern int gLevelTimerPeriod; /*@ ds:F510 */
extern int gLevelTimerSteps; /*@ ds:FB32 */
extern int gStepCount; /*@ ds:F540 */
extern unsigned long gDeadline; /*@ ds:F512 */
extern char gLevelPalettePending; /*@ ds:FB45 */
extern char far *gLevelInf; /*@ ds:8DCA */   /* the 7500-byte unpacked level script (src/E07A9.C) */
extern char far *gWallBlockMaps; /*@ ds:F43E */   /* the 6000-byte level graphics file (src/E07A9.C) */
extern unsigned char far *gPaletteBuf; /*@ ds:FE68 */  /* the 0x300-byte palette file (src/E07C8.C) */
extern void FormatName(char far *prefix, int number, char far *suffix); /*@ 082C:0AE8 */
extern void Unpack(char far *src, void far *dst); /*@ 1B82:00C7 */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 1B82:0009 */
extern void InitMonster(int index, int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14); /*@ 08E6:131D */
extern void PlaySound(int sound); /*@ 177D:0070 */

struct Effect {                     /* the 16-byte timed effect records at ds:5BA8 (src/E1414.C) */
    void (far *fn)(int);            /* +00: the handler */
    unsigned long period;           /* +04: the interval added to the tick on re-arm */
    unsigned long expiry;           /* +08: the tick the effect next runs at */
    int arg;                        /* +0C: the handler's argument */
    char sub;                       /* +0E */
    char active;                    /* +0F: nonzero while the effect is armed */
};
extern struct Effect gEffects[]; /*@ ds:5BA8 */

/* ds:6A80 is the initialiser template for ovl3:0A7C's local `handlers` array: the three map icon
   loaders LoadMapIcons6 (stub entry 0x25), LoadNamedIcons (0x2A) and LoadMapIcons6b (0x34), which
   ovl3:0A7C copies into the array through F_SCOPY (0000:78C9). Its far pointers are overlay stub
   entries, which `make match` compares by the function they name. */

struct WallDec {            /* the 52-byte wall-set decorations LoadWallSet reads into
                               gStagingPage+0x3E80; RegisterWallObject copies one into a gWallRec slot */
    char idx[0xc];          /* +0x00: idx[0..9] are the ten cell indices, idx[0xa] the chain link */
    int field0c[10];        /* +0x0C: ten words the self-assignment loop passes through */
    int field20[10];        /* +0x20: ten words the self-assignment loop passes through */
};

/* ovl3:0000: frees every far pointer in the 200-entry table at ds:EFD4 (farfree), clears each slot
   and resets the wall-record slot count ds:F538. gWallRecs itself is not cleared: it is only read
   through gWallRecCount (RegisterWallObject, :326-340), so the old records stay in memory until the next
   RegisterWallObject overwrites them from slot 0. Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:0000 */ void FreePtrTable(void)
{
    int i;

    for (i = 0; i < 0xc8; i++) {
        if (gRectKinds[i]) {
            farfree(gRectKinds[i]);
            gRectKinds[i] = 0;
        }
    }
    gWallRecCount = 0;
}

/* ovl3:004F: loads a wall set .DAT (whitepaper 4.3). It opens the screen file through
   LoadOverlayScreen, then opens the data file and reads a u16 decoration count and that many
   52-byte decorations into the scratch area at gStagingPage+0x3E80; each decoration's two 10-word
   tables at +0xC and +0x20 are then stored to themselves word by word (the form the original
   compiles to, a load and a store of the same word). It points gDecorationRects just past the decorations,
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
    FileRead(handle, (void far *)((char far *)gStagingPage + 0x3e80), decsBytes);
    dec = (struct WallDec far *)((char far *)gStagingPage + 0x3e80);
    for (i = 0; i < nDecs; i++) {
        for (j = 0; j < 10; j++) {
            dec->field0c[j] = dec->field0c[j];
            dec->field20[j] = dec->field20[j];
        }
        dec++;
    }
    gDecorationRects = (void far *)((char far *)gStagingPage + 0x3e80 + decsBytes);
    rects = (int far *)gDecorationRects;
    FileRead(handle, (void far *)&nRects, 2L);
    FileRead(handle, gDecorationRects, (long)nRects << 3);
    for (i = 0; i < nRects * 4; i++) {
        *rects = *rects;
        rects++;
    }
    FileClose(handle);
}

/* ovl3:01AA: Purpose unknown: returns 1 when bit `bit` (the second argument, a char) is set in the
   signed char `value` (the first argument), else 0. */
/*@ ovl3:01AA */ int IsBitSet(signed char value, char bit)
{
    return (value & (1 << bit)) ? 1 : 0;
}
/* ovl3:01C8: registers a placed wall object `kind` (the first byte of a five-byte level record,
   the rest `field1879`, `recordIndex`, `field1905`, `objectFlags`). It stores the four per-kind
   tables at ds:1879 (field1879 into the first, gWallRecCount+1 into ds:18BF, field1905 into ds:1905 and
   objectFlags^4 into ds:194B), then, unless `recordIndex` is -1 (clearing ds:18BF[kind]), walks
   the chain of 0x34-byte wall decorations starting at `recordIndex`: it copies the 0x34 bytes at
   gStagingPage+0x3E80+recordIndex*0x34 into gWallRecs[gWallRecCount].cells, 52 bytes at +0x2a of the slot, so
   the decoration's byte 0x0a lands at +0 of the next slot. For each of the ten cell indices in
   those first ten bytes that has no sprite yet in ds:EFD4, it slices the cell's 8-byte rectangle
   out of gDecorationRects through ds:8086 with the map sheet (gMapTiles[gCurLevel-1] cells). gWallRecCount (the slot
   count) is bumped and the link is the decoration byte 0x0a now at gWallRecs[gWallRecCount].next: when it
   is nonzero it is set to gWallRecCount+1 and the walk continues, otherwise the loop ends. Called by
   ovl3:0A7C. */
/*@ ovl3:01C8 */ void RegisterWallObject(int kind, int field1879, int recordIndex, int field1905, int objectFlags)
{
    int i;
    int cell;
    struct Cell8 far *rect;

    gWallVmpSet[kind] = field1879;
    gWallDecorHead[kind] = gWallRecCount + 1;
    gWallHandlerIndex[kind] = field1905;
    gWallTypeFlags[kind] = objectFlags;
    gWallTypeFlags[kind] ^= 4;
    if (recordIndex != -1) {
        while (recordIndex != -1) {
            movmem((void far *)((char far *)gStagingPage + recordIndex * 0x34 + 0x3e80),
                   (void far *)gWallRecs[gWallRecCount].cells, 0x34);
            for (i = 0; i < 0xa; i++) {
                cell = gWallRecs[gWallRecCount].cells[i];
                if (cell == 0xff)
                    continue;
                if (gRectKinds[cell])
                    continue;
                rect = &((struct Cell8 far *)gDecorationRects)[cell];
                gRectKinds[cell] = gGetSpriteCell(rect->w0, rect->w1, rect->w2, rect->w3, MAPSHEET);
            }
            gWallRecCount++;
            recordIndex = gWallRecs[gWallRecCount].next;
            if (recordIndex)
                gWallRecs[gWallRecCount].next = gWallRecCount + 1;
            else
                recordIndex = -1;
        }
    } else
        gWallDecorHead[kind] = 0;
}
/* ovl3:031A: frees and clears the six far pointers in the per-slot size-record table ds:FB5E and
   the six size-record pointers in the SlotPos records at ds:A99C. The counterpart of the
   allocation InitGameTables (ovl2:0496) makes. Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:031A */ void FreeSlotTables(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (gDoorShapes[i]) {
            farfree((void far *)gDoorShapes[i]);
            gDoorShapes[i] = 0;
        }
        if (gDoorButtons[i].rec) {
            farfree((void far *)gDoorButtons[i].rec);
            gDoorButtons[i].rec = 0;
        }
    }
}
/* ovl3:03A2: loads the overlay screen "DOOR" (LoadOverlayScreen) and slices map icon cells out of
   the map sheet (segment 0x1D8, offset from the tile byte table gMapTiles[gCurLevel-1] shifted left 5).
   For `srcA` (when not -1) it slices three cells from the 4-byte rectangles at ds:6840 into gDoorShapes
   starting at `dstA` and three more from ds:68A0 into gDoorButtons[dstA].rec, and copies the x and y bytes of
   gDoorButtons[dstA] from ds:6900. For `srcB` (when not -1) it slices the three cells at `dstB` the same way,
   but reads its ds:6900 x/y pair with srcA's index (QUIRK ovl3:0597). Clears gDrawPage. Called by
   ovl3:0A7C. */
/*@ ovl3:03A2 */ void LoadMapCells(int srcA, int dstA, int srcB, int dstB)
{
    int i;
    int j;

    LoadOverlayScreen("DOOR", 5, 3);
    if (srcA == -1) goto second;
    for (i = 0; i < 3; i++) {
        j = srcA * 0xc + i * 4;
        gDoorShapes[dstA + i] = (struct SlotSize far *)gGetSpriteCell(gDoorRects[j], gDoorRects[j + 1], gDoorRects[j + 2],
            gDoorRects[j + 3], MAPSHEET);
        gDoorButtons[dstA + i].rec = (struct SlotSize far *)gGetSpriteCell(gDoorButtonRects[j], gDoorButtonRects[j + 1], gDoorButtonRects[j + 2],
            gDoorButtonRects[j + 3], MAPSHEET);
        j = srcA * 6 + i * 2;
        gDoorButtons[dstA + i].x = gDoorButtonPos[j];
        gDoorButtons[dstA + i].y = gDoorButtonPos[j + 1];
    }
second:
    if (srcB == -1) goto done;
    for (i = 0; i < 3; i++) {
        j = srcB * 0xc + i * 4;
        gDoorShapes[dstB + i] = (struct SlotSize far *)gGetSpriteCell(gDoorRects[j], gDoorRects[j + 1], gDoorRects[j + 2],
            gDoorRects[j + 3], MAPSHEET);
        gDoorButtons[dstB + i].rec = (struct SlotSize far *)gGetSpriteCell(gDoorButtonRects[j], gDoorButtonRects[j + 1], gDoorButtonRects[j + 2],
            gDoorButtonRects[j + 3], MAPSHEET);
        j = srcA * 6 + i * 2;   /* QUIRK ovl3:0597: the srcB branch reads srcA's ds:6900 pair too */
        gDoorButtons[dstB + i].x = gDoorButtonPos[j];
        gDoorButtons[dstB + i].y = gDoorButtonPos[j + 1];
    }
done:
    gDrawPage = 0;
}
/* ovl3:05D2: frees and clears the `count` map icon pointers in gLoadedMonsterShapes starting at
   index `base`.
   Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:05D2 */ void FreeMapIcons(int base, int count)
{
    int i;
    int end;

    end = base + count;
    for (i = base; i < end; i++) {
        if (gLoadedMonsterShapes[i]) {
            farfree(gLoadedMonsterShapes[i]);
            gLoadedMonsterShapes[i] = 0;
        }
    }
}
/* ovl3:0624: loads the overlay screen `screen` and slices six map icon cells out of gOverlayBuf
   (ds:113F), one per 4-word rectangle in the table at ds:6930, into gLoadedMonsterShapes starting at index
   `dest`. Clears gDrawPage. Called through the `handlers` table LoadMapScreen builds (ovl3:0A7C,
   ds:6A80 initialiser). */
/*@ ovl3:0624 */ void LoadMapIcons6(char far *screen, int dest)
{
    int n;
    int i;
    int si;

    LoadOverlayScreen(screen, 5, 3);
    n = 6;
    for (i = 0, si = 0; i < n; si += 4, i++)
        gLoadedMonsterShapes[dest + i] = gGetSpriteCell(gMonsterRectsA[si], gMonsterRectsA[si + 1], gMonsterRectsA[si + 2],
            gMonsterRectsA[si + 3], gOverlayBuf);
    gDrawPage = 0;
}
/* ovl3:069E: the same as ovl3:0624 but slices the six cells from the 4-word rectangle table at
   ds:69C0 instead of ds:6930. Called through the `handlers` table LoadMapScreen builds (ds:6A80
   initialiser). */
/*@ ovl3:069E */ void LoadMapIcons6b(char far *screen, int dest)
{
    int n;
    int i;
    int si;

    LoadOverlayScreen(screen, 5, 3);
    n = 6;
    for (i = 0, si = 0; i < n; si += 4, i++)
        gLoadedMonsterShapes[dest + i] = gGetSpriteCell(gMonsterRectsB[si], gMonsterRectsB[si + 1], gMonsterRectsB[si + 2],
            gMonsterRectsB[si + 3], gOverlayBuf);
    gDrawPage = 0;
}
/* ovl3:0718: builds the screen name `name` + "1" through sprintf("%s1"), loads it through
   LoadOverlayScreen and slices six map icon cells out of gOverlayBuf using the 4-word rectangle
   table at ds:6A50 into gLoadedMonsterShapes starting at index `dest`. Clears gDrawPage. Called through the
   `handlers` table LoadMapScreen builds (ds:6A80 initialiser). */
/*@ ovl3:0718 */ void LoadNamedIcons(char far *name, int dest)
{
    char buf[0x28];
    int i;
    int si;

    sprintf(buf, "%s1", name);
    LoadOverlayScreen(buf, 5, 3);
    for (i = 0, si = 0; i < 6; si += 4, i++)
        gLoadedMonsterShapes[dest + i] = gGetSpriteCell(gMonsterRectsC[si], gMonsterRectsC[si + 1], gMonsterRectsC[si + 2],
            gMonsterRectsC[si + 3], gOverlayBuf);
    gDrawPage = 0;
}
/* ovl3:07A5: loads a wall texture set for `name`. It builds the file name "<name>.VCN" (VGA mode
   3) or "<name>.ECN" (otherwise) into gFileNameBuf; when `name` is NULL it reuses the name kept
   in gWallNameBuf. It loads the file packed with LoadPacked, then Unpacks (Unpack0, 1BA8:0006)
   the two 16-byte plane tables at gArtPage+2 and +0x12 into ds:F4D6 and ds:FB4D and the
   0xCB20-byte texture block at gArtPage+0x22 into gWallBlocks, then sets the plane tables
   (17A6:037E). In EGA mode 2 it expands the planes into gStagingPage (17A6:0F1F) and blits them to
   the A400 planar window through the driver ds:808E; in CGA mode 0 it slices a 1x8 cell from
   `sheet`, frees it and build-masks gWallBlocks (17A6:18EA). At the end it keeps the name just used
   by copying gFileNameBuf into gWallNameBuf. Called by ovl3:0A7C. */
/*@ ovl3:07A5 */ void LoadWallTextures(char far *name, char far *sheet)
{
    void far *cell;

    if (name)
        sprintf(gFileNameBuf, "%s.%s", name, gGraphicsChoice == 3 ? "VCN" : "ECN");
    else
        strcpy(gFileNameBuf, gWallNameBuf);
    LoadPacked(gFileNameBuf, 7, 3);
    Unpack0((char far *)gArtPage + 2, (void far *)gWallPlaneTableA, 0x10L);
    Unpack0((char far *)gArtPage + 0x12, (void far *)gWallPlaneTableB, 0x10L);
    Unpack0((char far *)gArtPage + 0x22, gWallBlocks, 0xcb20L);
    SetPlaneTables((void far *)gWallPlaneTableA, (void far *)gWallPlaneTableB);
    if (gGraphicsChoice == 2) {
        gCopyToPlanar(gWallBlocks, 0xa400, 0xfa00);
        ExpandNibbles(gWallBlocks, gStagingPage);
        gCopyToPlanar(gStagingPage, 0xaa00, 0x7d00);
        ExpandNibbles((char far *)gWallBlocks + 0x7d00, gStagingPage);
        gCopyToPlanar(gStagingPage, 0xabf4, 0x7d00);
    } else if (gGraphicsChoice == 0) {
        cell = gGetSpriteCell(0, 0, 1, 8, sheet);
        farfree(cell);
        BuildMask(gWallBlocks);
    }
    strcpy(gWallNameBuf, gFileNameBuf);
}
/* ovl3:0951: rebuilds the ds:B899 item list: walks the 500 items at ds:8DD0 and, for every item
   whose m0A is the current action gCurLevel and whose key key is positive, inserts it through
   InsertListItem. Called by ovl3:0A7C. */
/*@ ovl3:0951 */ void RebuildItemList(void)
{
    struct ItemRecord far *rec;
    int i;

    for (rec = gItems, i = 0; i < 0x1f4; rec++, i++) {
        if (rec->m0A == gCurLevel && rec->key > 0)
            InsertListItem((void far *)(gMazeItems + rec->key * 9), rec->key, i, rec->cellPos);
    }
}
/* ovl3:09AA: loads an object table. It clears the 0x2400 bytes at gMaze, opens `name`, reads 6
   bytes into gMaze and 0x1000 bytes into gStagingPage and closes it. When bit gCurLevel-1 of gSavedLevels is set
   it hands gCurLevel to the sibling ovl3:1147; otherwise it copies the 0x400 far pointers of gStagingPage
   into the +0 field of the gMaze records. Called by ovl3:0A7C. */
/*@ ovl3:09AA */ void LoadObjects(char far *name)
{
    int handle;
    char far *src;
    struct ObjRec far *dst;
    int i;

    memset(gMaze, 0, 0x2400);
    handle = FileOpen(name, 1);
    FileRead(handle, (void far *)gMaze, 6L);
    FileRead(handle, gStagingPage, 0x1000L);
    FileClose(handle);
    if (gSavedLevels & (1 << (gCurLevel - 1))) {
        LoadLevelState(gCurLevel);
    } else {
        src = (char far *)gStagingPage;
        dst = (struct ObjRec far *)gMaze;
        for (i = 0; i < 0x400; i++) {
            dst->ptr = *(void far * far *)src;
            src += 4;
            dst = (struct ObjRec far *)((unsigned long)dst + 9);
        }
    }
}
/* ovl3:0A7C: mounts the map screen for level `level`. It clears bit 2 of gTimerEnableFlags, reads the
   level's packed script "LEVEL<n>.INF" (FormatName, FileOpen, FileRead into gStagingPage, Unpack into
   gLevelInf), names its two object/screen files through LoadObjects and a "%s.%s" sprite name, loads
   the sprite file into gWallBlockMaps and, in VGA mode, the "%s.PAL" palette, then frees the slot,
   map-icon and pointer tables (FreeSlotTables, FreeMapIcons and FreePtrTable).
   The rest of the script: four LoadMapCells arguments, the header bytes gLevelTimerMode/gLevelTimerPeriod/gLevelTimerSteps and the
   gDeadline/gStepCount timers, two far function-pointer records (the local `handlers` list of the icon
   loaders, called with 0 and 0x12), a run of (index, value) timer pairs stored as the
   periods of the timed effects gEffects[index + 2] and gEffects[index + 3], the four deadlines
   gEffects[2..5].expiry derived from those periods plus the tick counter gGameTicks, the 30
   14-byte object records handed to InitMonster (or skipped by 0x1a4 when the level was already
   unpacked), the wall/object records (0xEC introduces a 0x18-byte wall set through LoadWallSet,
   otherwise a five-byte RegisterWallObject record), the item records keyed against gMaze, and
   finally the wall textures (LoadWallTextures with the gMapTiles[gCurLevel-1] sheet) and the INVENT
   screen. It clears the 0x400 ds:B899 item-list heads, rebuilds the list, sets bit 2 of gTimerEnableFlags,
   calls PlaySound(0) and sets gLevelPalettePending to 1. Called by EnterMapScreen (0ACD:0F72), ResetMapScreen
   (0ACD:1033), LoadLevelOp (122E:1AAB, through the 210C:007A stub) and ovl3:15B9 LoadGame. */
/*@ ovl3:0A7C */ void LoadMapScreen(int level)
{
    int scriptWord;
    int itemIdx;
    int itemCount;
    int handlerIdx;
    int objType;
    int objTimer;
    int objM12;
    int objM14;
    int timerIdx;
    int objIndex;
    int objId;
    int objFrame;
    int objOrient;
    int objM07;
    int objB0;
    int objKind;
    int scriptCount;
    int payloadLen;
    int nObjects;             /* QUIRK ovl3:0A7C: counted but never read (dead result) */
    char far *script;
    void (far *handlers[3])(char far *, int) = { LoadMapIcons6, LoadMapIcons6b, LoadNamedIcons };
    int i;
    int handle;

    gTimerEnableFlags &= ~2;
    gCurLevel = level;
    FormatName("LEVEL", level, "INF");
    handle = FileOpen(gFileNameBuf, 1);
    FileRead(handle, (char far *)&payloadLen, 2L);
    FileRead(handle, (char far *)gStagingPage, (long)payloadLen);
    FileClose(handle);
    Unpack((char far *)gStagingPage, gLevelInf);
    script = gLevelInf + 2;
    LoadObjects(script);
    script += 12;
    sprintf(gFileNameBuf, "%s.%s", script, gGraphicsChoice == 3 ? "VMP" : "EMP");
    handle = FileOpen(gFileNameBuf, 1);
    FileRead(handle, gWallBlockMaps, 2L);
    FileRead(handle, gWallBlockMaps, 0x1770L);
    FileClose(handle);
    if (gGraphicsChoice == 3) {
        sprintf(gFileNameBuf, "%s.PAL", script);
        LoadFile(gFileNameBuf, gPaletteBuf, 0x300);
    }
    script += 0x18;
    FreeSlotTables();
    FreeMapIcons(0, 0x24);
    FreePtrTable();
    LoadMapCells(script[0], script[1], script[2], script[3]);
    script += 4;
    gLevelTimerMode = script[0];
    script++;
    gLevelTimerPeriod = *(int far *)script;
    script += 2;
    gLevelTimerSteps = *(int far *)script;
    script += 2;
    gDeadline = gGameTicks + gLevelTimerPeriod;
    gStepCount = 0;
    handlerIdx = script[0];
    script++;
    if (handlerIdx != -1)
        handlers[handlerIdx](script, 0);
    script += 12;
    handlerIdx = script[0];
    script++;
    if (handlerIdx != -1)
        handlers[handlerIdx](script, 0x12);
    script += 12;
    while ((timerIdx = *script++) != -1) {
        timerIdx <<= 1;
        gEffects[timerIdx + 2].period = (unsigned long)script[0];
        gEffects[timerIdx + 3].period = (unsigned long)script[0];
        script++;
    }
    for (i = 0; i < 4; i++) {
        if (i & 1)
            gEffects[i + 2].expiry = gGameTicks + (gEffects[i + 2].period >> 1);
        else
            gEffects[i + 2].expiry = gGameTicks + gEffects[i + 2].period;
    }
    if (!(gSavedLevels & (1 << (level - 1)))) {
        memset(gMonsters, 0, 0x348);
        for (i = 0; i < 0x1e; i++) {
            objIndex = script[0];
            script++;
            objKind = script[0];
            script++;
            objId = *(int far *)script;
            script += 2;
            objFrame = script[0];
            script++;
            objOrient = script[0];
            script++;
            objB0 = script[0];
            script++;
            objM07 = script[0];
            script++;
            objType = script[0];
            script++;
            objTimer = script[0];
            script++;
            objM12 = *(int far *)script;
            script += 2;
            objM14 = *(int far *)script;
            script += 2;
            InitMonster(objIndex, objKind, objId, objFrame, objOrient, objB0, objM07, objType, objTimer,
                objM12, objM14);
        }
    } else
        script += 0x1a4;
    scriptCount = *(int far *)script;
    script += 2;
    nObjects = 0;
    for (i = 0; i < scriptCount; i++) {
        if (*script++ == (char)0xec) {   /* 0xEC: a 0x18-byte wall set, else a wall object record */
            LoadWallSet(script, script + 0xc);
            script += 0x18;
        } else {
            nObjects++;
            RegisterWallObject(script[0], script[1], script[2], script[3], script[4]);
            script += 5;
        }
    }
    gDrawPage = 0;
    scriptWord = *(int far *)gLevelInf;
    script = gLevelInf + scriptWord;
    itemCount = *(int far *)script;
    script += 2;
    for (i = 0; i < 0x400; i++)
        *(unsigned short far *)(gMazeItems + i * 9) = 0;
    RebuildItemList();
    for (i = 0; i < itemCount; i++) {
        itemIdx = *(int far *)script;
        script += 2;
        ((unsigned char far *)gMaze)[itemIdx * 9 + 4] |= script[0];
        script++;
        scriptWord = *(int far *)script;
        script += 2;
        *(unsigned short far *)((char far *)gMaze + itemIdx * 9 + 7) = scriptWord;
    }
    LoadWallTextures(gLevelInf + 0x1a, MAPSHEET);
    LoadScreenImage("INVENT", (char far *)gSheetINVENT, 5, 3, 2);
    gTimerEnableFlags |= 2;
    PlaySound(0);
    gLevelPalettePending = 1;
}
/* ovl3:0FD1: packs the level state into `buf`. It copies the four gMaze bytes of each of the 0x400
   records into gBufDCFC, compresses gBufDCFC (0x1000 bytes) into `buf` with CompressLcw and, when
   the stream is longer than 0x4B0, prints "Map compression failure: size of map = %d" and shuts the
   game down. It finishes by Unpack0ing gMonsters into `buf`+0x4B0. Called by ovl3:11AF. */
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
            *src++ = gMaze[i * 9 + j];
    len = CompressLcw((char far *)gBufDCFC, (char far *)buf, 0x1000);
    if (len > 0x4b0) {
        gDrawPage = 0;
        sprintf(gFileNameBuf, "Map compression failure: size of map = %d", len);
        gDrawText(gFileNameBuf, 0, 0, 0xf, 0);
        WaitKeyEvent();
        ShutdownGame();
    }
    Unpack0(gMonsters, dst, 0x348L);
}
/* ovl3:109D: unpacks a level state. `buf` is the 0x7F8-byte level state ovl3:1147 read. It
   Unpack4s 0x1000 bytes of `buf` into gBufDCFC and Unpack0s the 0x348 bytes at `buf`+0x4B0 into
   gMonsters, copies the four gBufDCFC bytes of each of the 0x400 gMaze records into its +0 field, then
   bumps the +4 byte of every gMaze record the 30 gMonsters objects index by id. Called by ovl3:1147. */
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
            gMaze[i * 9 + j] = *src++;
    Unpack0(dst, gMonsters, 0x348L);
    for (i = 0; i < 0x1e; i++)
        gMaze[gMonsters[i].cell * 9 + 4]++;
}
/* ovl3:1147: reads the level state `action` from LEVELS.TMP: opens the file, seeks to
   (action-1)*0x7F8 and reads 0x7F8 bytes into gArtPage, closes it and hands gArtPage to ovl3:109D.
   Called by ovl3:09AA. */
/*@ ovl3:1147 */ void LoadLevelState(int action)
{
    int handle;

    handle = FileOpen("LEVELS.TMP", 1);
    SeekFile(handle, (long)(action - 1) * 0x7f8, 0);
    FileRead(handle, gArtPage, 0x7f8L);
    FileClose(handle);
    UnpackLevelState(gArtPage);
}
/* ovl3:11AF: saves the level state `level` to LEVELS.TMP: normalizes gArtPage+0xC800 into a far
   pointer, hands it to ovl3:0FD1, opens LEVELS.TMP for writing (mode 3), seeks to
   (level-1)*0x7F8, writes 0x7F8 bytes from the buffer, closes it and sets bit level-1 of gSavedLevels.
   Called by LoadLevelOp (122E:1AAB, through the 210C:002F stub). */
/*@ ovl3:11AF */ void SaveLevelState(int level)
{
    char far *state;
    int handle;

    state = (char far *)NormalizePtr((char far *)gArtPage + 0xc800);
    PackLevelState((void far *)state);
    handle = FileOpen("LEVELS.TMP", 3);
    SeekFile(handle, (long)(level - 1) * 0x7f8, 0);
    FileWrite(handle, state, 0x7f8L);
    FileClose(handle);
    gSavedLevels |= 1 << (level - 1);
}/* ovl3:123D: ages the ten 32-bit countdowns at gChars+0xAD for every character that passes
   MemberIs(i, 1): a countdown greater than the tick counter gGameTicks loses gGameTicks, and a nonzero
   countdown not greater than gGameTicks is set to 1. Called by ovl3:1368. */
/*@ ovl3:123D */ void AgeCharTimers(void)
{
    struct CharRec far *rec;
    int i;
    int j;

    for (rec = gChars, i = 0; i < 6; i++, rec++) {
        if (MemberIs(i, 1)) {
            for (j = 0; j < 0xa; j++) {
                if (rec->timers[j] > gGameTicks)
                    rec->timers[j] -= gGameTicks;
                else if (rec->timers[j])
                    rec->timers[j] = 1;
            }
        }
    }
}
/* ovl3:12F1: the counterpart of ovl3:123D: adds the tick counter gGameTicks to every nonzero countdown
   at gChars+0xAD for each character that passes MemberIs(i, 1), then calls RearmCharEffects.
   Called by ovl3:1368 and ovl3:15B9. */
/*@ ovl3:12F1 */ void AdvanceCharTimers(void)
{
    struct CharRec far *rec;
    int i;
    int j;

    for (rec = gChars, i = 0; i < 6; i++, rec++) {
        if (MemberIs(i, 1)) {
            for (j = 0; j < 0xa; j++) {
                if (rec->timers[j])
                    rec->timers[j] += gGameTicks;
            }
        }
    }
    RearmCharEffects();
}
/* ovl3:1368: saves the game. It runs ResetLevel, reads the whole 0x5FA0-byte LEVELS.TMP image
   into gArtPage (mode 1), packs the level state into the in-memory image at gArtPage +
   (gCurLevel-1)*0x7F8 through ovl3:0FD1, sets bit gCurLevel-1 in ds:FBD0, probes EOBDATA.SAV (a mode-1
   FileOpen/FileClose), then deletes and recreates EOBDATA.SAV (mode 2) and writes, checking each
   length: the six 243-byte characters ds:F580 (after AgeCharTimers and before AdvanceCharTimers),
   the 0x40-byte header at ds:FBC8, the 500 14-byte items ds:8DD0, the 0x5FA0-byte image
   gArtPage, the single bytes ds:80E8, ds:80E6 and ds:F43C, and the 0x60-byte buffer ds:AD20.
   Returns 1 when every write wrote its full length, else 0. The `if (ok)` at :854 follows `ok = 1`,
   so the INVENT reload at :871 runs after the writes whether or not they succeeded; the `ok &=`
   writes at :857-869 only compute the return value. Called by overlay 1 (ovl1:1ACD). */
/*@ ovl3:1368 */ int SaveGame(void)
{
    int slot;
    int handle;
    int ok;
    long len;

    slot = gCurLevel - 1;
    ResetLevel();
    handle = FileOpen("LEVELS.TMP", 1);
    FileRead(handle, gArtPage, 0x5fa0L);
    FileClose(handle);
    PackLevelState((char far *)gArtPage + (long)slot * 0x7f8);
    gSavedLevels |= 1 << slot;
    handle = FileOpen("EOBDATA.SAV", 1);
    FileClose(handle);
    handle = FileOpen("EOBDATA.SAV", 2);
    ok = 1;
    if (ok) {
        AgeCharTimers();
        len = 0x5b2;
        ok &= (FileWrite(handle, (char far *)gChars, len) == len);
        AdvanceCharTimers();
        len = 0x40;
        ok &= (FileWrite(handle, (char far *)&gCurLevel, len) == len);
        len = 0x1b58;
        ok &= (FileWrite(handle, (char far *)gItems, len) == len);
        len = 0x5fa0;
        ok &= (FileWrite(handle, gArtPage, len) == len);
        ok &= (FileWrite(handle, (char far *)&gTunesOn, 1L) == 1L);
        ok &= (FileWrite(handle, (char far *)&gSoundOn, 1L) == 1L);
        ok &= (FileWrite(handle, (char far *)&gBarGraphsOn, 1L) == 1L);
        len = 0x60;
        ok &= (FileWrite(handle, (char far *)gSpellItemTypes, len) == len);
        FileClose(handle);
        LoadScreenImage("INVENT", (char far *)gSheetINVENT, 5, 3, 2);
    }
    return ok;
}

/* ovl3:15B9: loads a saved game from EOBDATA.SAV. It reads the six blocks whose sizes are the
   table at ds:6A8C into gArtPage (checking each length), then resets the level, unpacks the
   character block into gChars (restoring the six sprite pointers around it, then
   AdvanceCharTimers), the 0x40-byte header into ds:FBC8 and the item block into gItems. It
   recreates LEVELS.TMP from block 3 (mode 2 deletes and recreates the file; FileWrite starts at
   offset 0 and carries the whole gBlockSizes[3] image). It stores the three single bytes ds:80E8
   (the Ctrl flag), ds:80E6 (gSoundOn) and ds:F43C, unpacks the 0x60 bytes into ds:AD20, loads
   the CHARGENA screen and, for each character whose identifiedNameIndex is nonzero and whose portraitIndex is >= 0, cuts its
   4 by 32 cell out of gOverlayBuf (through ds:8082), unpacks it into its 0x514-byte sprite buffer
   and frees it (a character with identifiedNameIndex nonzero and portraitIndex < 0 sets `flag` at :953 instead);
   when any slot is empty or any occupied character has portraitIndex < 0 it then loads the OUTTAKE screen
   and cuts the portraitIndex < 0 characters' 4 by 32 cells out of it the same way. Unless ds:A998 is -1 it then rebuilds the
   map screen (ovl3:0A7C), reloads PLAYFLD and INVENT, hands gFont6x6 to SetFont, redraws the
   characters, and either restores the mouse item when the mouse is enabled (gMouseDisabled is 0)
   or, when the mouse is disabled and gFBCE is nonzero, inserts it with a sound from the gViewToCellPos table
   picked by RollDice(1, 2, -1) (the item is keyed by gPartyCell). Sets ds:A998 to 1 on success, 0 on
   (ovl1:1ACD) and overlay 2 (ovl2:0846). */
/*@ ovl3:15B9 */ int LoadGame(void)
{
    int handle;
    int ok;
    int flag;
    int idx;
    char far *buf;
    void far *cell;
    struct CharRec far *rec;
    void far *sprites[6];
    int i;
    int j;

    buf = (char far *)gArtPage;
    handle = FileOpen("EOBDATA.SAV", 1);
    ok = 1;
    if (ok) {
        for (i = 0; i < 6; i++) {
            ok &= (FileRead(handle, buf, gBlockSizes[i]) == gBlockSizes[i]);
            buf += gBlockSizes[i];
        }
        FileClose(handle);
    }
    if (ok) {
        ResetLevel();
        buf = (char far *)gArtPage;
        for (j = 0; j < 6; j++)
            sprites[j] = gChars[j].portrait;
        Unpack0(buf, (char far *)gChars, gBlockSizes[0]);
        buf += gBlockSizes[0];
        for (j = 0; j < 6; j++)
            gChars[j].portrait = sprites[j];
        AdvanceCharTimers();
        Unpack0(buf, (char far *)&gCurLevel, gBlockSizes[1]);
        buf += gBlockSizes[1];
        Unpack0(buf, (char far *)gItems, gBlockSizes[2]);
        buf += gBlockSizes[2];
        handle = FileOpen("LEVELS.TMP", 1);
        FileClose(handle);
        handle = FileOpen("LEVELS.TMP", 2);
        FileWrite(handle, buf, gBlockSizes[3]);
        FileClose(handle);
        buf += gBlockSizes[3];
        gTunesOn = buf[0];
        buf++;
        gSoundOn = buf[0];
        buf++;
        gBarGraphsOn = buf[0];
        buf++;
        Unpack0(buf, (char far *)gSpellItemTypes, gBlockSizes[5]);
        buf += gBlockSizes[5];
        LoadOverlayScreen("CHARGENA", 5, 3);
        flag = 0;
        for (i = 0; i < 6; i++) {
            rec = &gChars[i];
            if (rec->status != 0 && rec->portraitIndex >= 0) {
                cell = gGetSpriteCellRle((rec->portraitIndex % 10) * 4, (rec->portraitIndex / 10) * 32, 4, 0x20, gOverlayBuf);
                Unpack0(cell, rec->portrait, 0x514L);
                farfree(cell);
            } else
                flag = 1;
        }
        if (flag) {
            LoadOverlayScreen("OUTTAKE", 5, 3);
            flag = 0;
            for (i = 0; i < 6; i++) {
                rec = &gChars[i];
                if (rec->status != 0 && rec->portraitIndex < 0) {
                    cell = gGetSpriteCellRle(-(rec->portraitIndex + 1) * 4, 0xa0, 4, 0x20, gOverlayBuf);
                    Unpack0(cell, rec->portrait, 0x514L);
                    farfree(cell);
                }
            }
        }
        if (gGameLoadState != (char)0xff) {
            LoadMapScreen(gCurLevel);
            LoadScreenImage("PLAYFLD", (char far *)gSheetDECORATE, 5, 3, 0);
            DrawViewRows();
            LoadScreenImage("INVENT", (char far *)gSheetINVENT, 5, 3, 2);
            SetFont((char far *)gFont6x6);
            RedrawCharacters();
            if (gMouseDisabled == 0)
                SetMouseItem(gFBCE);
            else if (gFBCE) {
                idx = gViewToCellPos[gPartyDir * 4 + RollDice(1, 2, -1)];
                InsertListItem((void far *)(gMazeItems + gPartyCell * 9), gPartyCell, gFBCE, idx);
            }
        }
        gGameLoadState = 1;
    } else
        gGameLoadState = 0;
    return ok;
}
/* ovl3:19F7: resets the level. It clears the m0A byte of the 30 gMonsters objects, clears every live
   ds:DD2C effect record (for an active 1, keeping only the low two bits of the m03 byte of the
   item record its item indexes, then clearing its active byte; an active 2 is just cleared),
   then, when gDragActive is set, walks the gMaze record of character gDragMember adding the mode
   byte gDoorMoveDir to two bytes of its slot until the gWallTypeFlags flag of that slot has bit 0x30 set (the
   loop has no other bound), and clears gDragActive. Called by ovl3:1368 and ovl3:15B9. */
/*@ ovl3:19F7 */ void ResetLevel(void)
{
    int i;
    int who;
    int sel;
    int slot;

    for (i = 0; i < 0x1e; i++)
        gMonsters[i].m0A = 0;
    for (i = 0; i < 0xa; i++) {
        if (gProjectiles[i].active == 1) {
            gItems[gProjectiles[i].item].cellPos &= 3;
            gProjectiles[i].active = 0;
        }
        if (gProjectiles[i].active == 2)
            gProjectiles[i].active = 0;
    }
    if (gDragActive) {
        who = gDragMember;
        sel = gDoorMoveDir;
        slot = (gWallTypeFlags[gMaze[who * 9]] & 8) ? 0 : 1;
        while ((gWallTypeFlags[gMaze[who * 9 + slot]] & 0x30) == 0) {
            gMaze[who * 9 + slot] += sel;
            gMaze[who * 9 + slot + 2] += sel;
        }
        gDragActive = 0;
    }
}
