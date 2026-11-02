/* Overlay 3 of EOB.EXE (6961 bytes, 23 functions). Purpose unknown at this point: the unit owns the
   wall set loader (ovl3:004F), the free list at ds:EFD4 and the 0x34-byte character/object records
   at ds:E06A. Placeholder names. */

/*@flags -ml -O -Z */

extern void farfree(void far *block); /*@ 0000:71EE */
extern void far *gEFD4[]; /*@ ds:EFD4 */
extern int gF538; /*@ ds:F538 */

extern void LoadOverlayScreen(char far *name, int a, int b); /*@ 082C:05EC */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *gSegDF4E; /*@ ds:DF4E */
extern void far *gE062; /*@ ds:E062 */

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

/*@ ovl3:01AA stub */ void Ovl3_01AA(void) {}
/*@ ovl3:01C8 stub */ void Ovl3_01C8(void) {}
/*@ ovl3:031A stub */ void Ovl3_031A(void) {}
/*@ ovl3:03A2 stub */ void Ovl3_03A2(void) {}
/*@ ovl3:05D2 stub */ void Ovl3_05D2(void) {}
/*@ ovl3:0624 stub */ void Ovl3_0624(void) {}
/*@ ovl3:069E stub */ void Ovl3_069E(void) {}
/*@ ovl3:0718 stub */ void Ovl3_0718(void) {}
/*@ ovl3:07A5 stub */ void Ovl3_07A5(void) {}
/*@ ovl3:0951 stub */ void Ovl3_0951(void) {}
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
