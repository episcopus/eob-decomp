/* Unit at overlay 0 of INTRO.EXE (11741 bytes, 22 functions): the Westwood introduction
   sequence. RunIntro (ovl0:0000) is the overlay's entry, called from the front end at
   0892:0161 through stub entry 16; PlayIntroSequence (ovl0:003A) walks the scenes, each a
   function of this file named after the .CMP/.COL files it loads (TITLE-E/TITLE-V, TEXT,
   WTRDP1..3, ORB, TOWRMAGE, KING, ADVENTUR, HANDS, TUNNEL, ZOOMTUNL, AVALANCH, SSI, PRESENT,
   DAND, WESTWOOD). The whole unit is drawn through the driver pointers the front end fills at
   load time (ds:1EE2 and its neighbours), which this page only partly names.
   The seven scene functions are still stubs: ovl0:015D (WTRDP1..3), 06E2 (ORB, TOWRMAGE, a
   four sprite array it blits with BlitImage and draws through f1A29_000F), 0B20 (TOWRMAGE),
   1265, 15F9, 1DF3 and 23A3. They are long ClipRect/frame animations; every utility they use
   (LoadPackedImage, LoadPalette, PlaySound, Dissolve, BlitImage, DrawGrid, GrowFrame,
   WaitFrameDelay, f19EC_002A) is matched. */

/*@flags -ml -O -Z -G */

#include <dos.h>

/* The driver entry points. The front end fills these words at load time (i0892 sets ds:1F46
   from its palette setup at 0892:011D..0142); their DGROUP neighbours ds:1E98..1ED0 hold far
   pointers into this program's own segments, so the block is a driver vtable [H]. Only the
   entries this unit calls are declared. */
extern void (far *gUnknown1EE2)(void);                              /*@ ds:1EE2 */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int color); /*@ ds:1EEE */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int color); /*@ ds:1EF6 */
extern int (far *gGetPixel)(int x, int y);                          /*@ ds:1EFE */
extern void far *(far *gMakeImage)(int x, int y, int width, int height, char far *base); /*@ ds:1F0E */
extern void (far *gPutPixel)(int x, int y, int color);              /*@ ds:1F2A */
extern void (far *gSetPalette)(char far *palette);                  /*@ ds:1F46 */

/* Globals of this unit's DGROUP (INTRO.EXE, paragraph 0E9F). */
extern int gGraphicsChoice;             /*@ ds:1F8E */  /* the graphics choice: argv[1][0] + 1 then 0x31 less (0892:0010, 0892:0258) */
extern int gDrawPage;                   /*@ ds:1FA1 */
extern long gFrameDelay;                /*@ ds:2326 */
extern int gAbort;                      /*@ ds:2D1C */

/* The keyboard routines of the library (i0a19, the twin of EOB.EXE's 1AAD input unit). */
extern int ReadKey(void);               /*@ 0A19:0382 */
extern int WaitGameKey(void);           /*@ 0A19:052E */
extern void WaitFrame(void);            /*@ 098F:0023 */

/* 097F:002A: hands a copy job to the far routine at ds:8046 when two buffer slots hold
   distinct non-zero segments (src/E19EC.C:29). Its third and fourth arguments are a far
   pointer the handler uses as the source mask/scratch buffer. */
extern void f19EC_002A(int dstKind, int mode, int extraOff, int extraSeg); /*@ 097F:002A */

/* 0AED:000D: reads a named file into a far buffer, the EOB LoadFile at 1B82:0009 linked at
   another address (the twin unit is i0aed, no EOB twin). */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 0AED:000D */
extern void far *f19EC_0002(int kind); /*@ 097F:0002 */
extern int FileOpen(char far *name, int mode); /*@ 0B5D:00D1 */
extern long FileRead(int index, char far *buf, unsigned long len); /*@ 0B5D:03F6 */
extern void FileClose(int index); /*@ 0B5D:03B1 */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 0CE9:0009 */

/* The sound driver. The INTRO addresses differ from EOB's (0000:400C -> 0000:43E7, 1948:0418
   -> 084A:0417). The routine names follow EOB's 177D:0070 and 1948:0418; gSoundBuf and
   gSoundChoice are the names the front end gives them (0892:06D1, 0892:06A9). */
extern unsigned char far *gSoundBuf;    /*@ ds:28A0 */  /* the game-number to driver-number map */
extern int gSoundChoice;                  /*@ ds:28C6 */  /* 1 AdLib, 2 PC speaker, 3 Tandy */
extern unsigned char gTandyOn;          /*@ ds:289B */  /* the PC speaker / Tandy driver select */
extern void QueueSound(int sound);      /*@ 0000:43E7 */
extern void far *GetSoundData(int idx); /*@ 0000:03E3 */
extern void StartSound(void far *routine); /*@ 084A:0417 */
extern void SoundDriverResume(void);    /*@ 084A:0457 */

/* The palette and image buffers of this unit's DGROUP. gPaletteBuf is a far pointer the front end
   allocates and fills (i0892:05A2..061A); gOverlayBuf and gCopyBuf are the DGROUP buffers
   f19EC_002A hands to its copy handler, EOB.EXE's gOverlayBuf at ds:113F: ovl0:2A99 passes
   gOverlayBuf and ovl0:003A passes gCopyBuf (ds:1BEA) for the title copy and gOverlayBuf
   (ds:1C0A, 32 bytes into the same zeroed region) for the text copy. */
extern unsigned char far *gPaletteBuf;     /*@ ds:2D42 */
extern unsigned char gOverlayBuf[];     /*@ ds:1C0A */
extern unsigned char gCopyBuf[];        /*@ ds:1BEA */
extern unsigned char gEgaPalette[16];   /*@ ds:19E7 */
extern int gDriverState;                /*@ ds:1F76 */  /* Purpose unknown: a driver flag; ovl0:003A sets bit 0 only */

/* the delay of the Westwood logo card, in timer ticks: 50, read by PlayLogoSequence (ds:1D28) */
extern int gWestwoodDelay;              /*@ ds:1D28 */
/* the delays of the four SSI logo cards (100, 50, 140, 140), read by PlayLogoSequence (ds:1D2A) */
extern int gLogoDelays[4];              /*@ ds:1D2A */
/* the CMP images of the four SSI logo cards (AND, SSI, PRESENT, DAND), read by PlayLogoSequence */
extern char far *gLogoFiles[4];         /*@ ds:1D32 */

/* 0CDA:0006: the pseudo-random byte generator of i0cda. It advances the 32-bit seed at
   ds:2310 (0x98761234 at load) with a shift and xor sequence and returns a byte; 0FD9 calls
   it and discards the result, the animations use it to pick star positions and colours. */
extern unsigned char RandomByte(void);  /*@ 0CDA:0006 */
extern int RandomRange(int low, int high); /*@ 0A15:0009 */

/* the permutation tables Dissolve shuffles (ovl0:2C0D): one word per screen column and row */
extern int gOrderX[320];                /*@ ds:290C */
extern int gOrderY[200];                /*@ ds:2B8C */

/* the key code of the escape key, the one key the wait loop reacts to */
#define KEY_ESC 0x1B

/* Functions of this file, in address order (a call to one defined later compiles through the
   overlay's far call, which TLINK rewrites to nop; push cs; call near, the form the original
   shows as 90 0E E8). */
void PlayIntroSequence(void);
void Ovl0_015D(void);
void Ovl0_06E2(void);
void BlitImage(int x1, int y1, int x2, int y2, int width, int height, int mode);
void Ovl0_0B20(void);
void ClearBottomBar(void);
void WaitFrameDelay(void);
void DelayFrames(unsigned frames);
void GrowFrame(int left, int top, int dstLeft, int dstTop, int x, int y, int width, int height, int curWidth, int curHeight, int addW, int addH);
int SignOf(int value);
void Ovl0_1265(void);
void Ovl0_15F9(void);
void Ovl0_1DF3(void);
void Ovl0_23A3(void);
void RevealInStrips(void);
void DrawGrid(void);
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 0986:000E */
void LoadPalette(char far *name);
void LoadPackedImage(char far *name, int srcKind, int dstKind);
void Dissolve(int x, int y, unsigned width, unsigned height, int fromPage, int toPage);
void PlaySound(int sound);

/* Functions of this file, in address order (forward declarations: a call to one defined
   later compiles through the overlay's far call, which TLINK rewrites to nop; push cs; call
   near, the form the original shows as 90 0E E8). */
void PlayIntroSequence(void);
void PlayLogoSequence(void);

/* ovl0:0000: the introduction overlay's entry point. Clears the abort flag, for the two
   non-VGA modes (gGraphicsChoice 1 and 2) starts the driver through ds:1EE2 and applies the
   fixed palette at ds:19E7, plays the logo sequence (ovl0:2A99), then the scenes
   (ovl0:003A) unless a key aborted the intro. */
/*@ ovl0:0000 */ void RunIntro(void)
{
    gAbort = 0;
    if (gGraphicsChoice == 1 || gGraphicsChoice == 2) {
        gUnknown1EE2();
        gSetPalette(gEgaPalette);
    }
    PlayLogoSequence();
    if (!gAbort)
        PlayIntroSequence();
}

/* ovl0:003A: plays the title card and then every scene. It sets the intro's driver flag
   (ds:1F76 bit 0), draws on page 0 and holds the title for 120 ticks: the TITLE-V card on the
   VGA mode (gGraphicsChoice 3), TITLE-E otherwise, then the WESTWOOD palette, sound 2, the
   buffer copies and the dissolve from page 2 to page 0. Then TEXT.CMP, another copy and a
   6-tick wait. Each of the eight scene functions follows guarded by the abort flag, so a key
   during a wait skips the rest of the intro. */
/*@ ovl0:003A */ void PlayIntroSequence(void)
{
    gDriverState |= 1;
    gDrawPage = 0;
    gFrameDelay = 0x78;
    if (gGraphicsChoice != 3)
        LoadPackedImage("TITLE-E.CMP", 3, 5);
    else
        LoadPackedImage("TITLE-V.CMP", 3, 5);
    WaitFrame();
    LoadPalette("WESTWOOD.COL");
    PlaySound(2);
    f19EC_002A(5, 2, FP_OFF(gCopyBuf), FP_SEG(gCopyBuf));
    Dissolve(0, 0, 320, 200, 2, 0);
    LoadPackedImage("TEXT.CMP", 3, 5);
    f19EC_002A(5, 6, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    if (!gAbort)
        Ovl0_0B20();
    if (!gAbort)
        Ovl0_06E2();
    if (!gAbort)
        Ovl0_015D();
    if (!gAbort)
        Ovl0_1265();
    if (!gAbort)
        Ovl0_15F9();
    if (!gAbort)
        Ovl0_1DF3();
    if (!gAbort)
        Ovl0_23A3();
    if (!gAbort)
        DrawGrid();
}
/*@ ovl0:015D stub */ void Ovl0_015D(void) {}
/*@ ovl0:06E2 stub */ void Ovl0_06E2(void) {}
/* ovl0:09AD: copies an image between two rectangles of the same video buffer (slot 3, the
   segment f19EC_0002(3) returns), from the rectangle at (x1,y1) to the one at (x2,y2), each
   row `width` bytes wide and `height` lines high. `mode` selects the byte interleave:
     0: nothing;
     1: two source rows per pass, the low bytes (i even) and the high bytes (i odd) written
        into alternate destination rows, the other byte cleared;
     2: one source row per pass in the low bytes, the destination's second row cleared, the
        source advancing two rows;
     3: every third source row is written with three clears per byte, the two rows between
        are cleared.
   Used by the scene functions to build the layered sprites (ovl0:015D, ovl0:06E2). */
/*@ ovl0:09AD */ void BlitImage(int x1, int y1, int x2, int y2, int width, int height, int mode)
{
    int i;
    char far *src;
    char far *dst;
    int row;

    src = dst = (char far *)f19EC_0002(3);
    src += x1 + y1 * 320;
    dst += x2 + y2 * 320;
    switch (mode) {
    case 1:
        while (height > 0) {
            for (i = 0; i < width; i += 2) {
                dst[i] = src[i];
                dst[i + 1] = 0;
            }
            dst += 320;
            src += 320;
            for (i = 0; i < width; i += 2) {
                dst[i] = 0;
                dst[i + 1] = src[i + 1];
            }
            dst += 320;
            src += 320;
            height -= 2;
        }
        break;
    case 2:
        while (height > 0) {
            for (i = 0; i < width; i += 2) {
                dst[i] = src[i];
                dst[i + 1] = 0;
            }
            dst += 320;
            for (i = 0; i < width; i++)
                dst[i] = 0;
            dst += 320;
            src += 640;
            height -= 2;
        }
        break;
    case 3:
        row = 0;
        while (row < height) {
            if (row % 3) {
                for (i = 0; i < width; i++)
                    dst[i] = 0;
            } else {
                for (i = 0; i < width - 3; i += 3) {
                    dst[i] = src[i];
                    dst[i + 1] = 0;
                    dst[i + 2] = 0;
                }
                for (; i < width; i++)
                    dst[i] = 0;
            }
            row++;
            dst += 320;
            src += 320;
        }
        break;
    }
}
/*@ ovl0:0B20 stub */ void Ovl0_0B20(void) {}
/* ovl0:0FAA: clears the bottom bar of the screen, the rectangle from (0,168) to (319,199),
   with colour 12 on the page the caller wants visible (page 0), then puts the driver's
   current page (gDrawPage) back. Called by the scene functions between frames (ovl0:015D at
   0217 and 04CF, and ovl0:15F9, ovl0:1DF3 and ovl0:23A3) to wipe the text on the bar. */
/*@ ovl0:0FAA */ void ClearBottomBar(void)
{
    int page;

    page = gDrawPage;
    gDrawPage = 0;
    gFillRect(0, 0xA8, 0x13F, 0xC7, 0xC);
    gDrawPage = page;
}

/* ovl0:0FD9: waits out gFrameDelay, one pass per timer tick, polling the keyboard. A pending
   key is taken: the rest of the key ring is drained (esc sets the abort flag gAbort, so the
   caller skips the rest of the intro) and the delay is cleared, so any key skips the wait.
   RandomByte is called once per pass even when a key was drained; it advances the seed at
   ds:2310 and its result is not used here. */
/*@ ovl0:0FD9 */ void WaitFrameDelay(void)
{
    if (!gAbort)
        while (gFrameDelay) {
            if (ReadKey()) {
                while (ReadKey()) {
                    if (WaitGameKey() == KEY_ESC)
                        gAbort = 1;
                }
                gFrameDelay = 0;
            }
            RandomByte();
        }
}

/* ovl0:1025: waits `frames` timer ticks by setting gFrameDelay and calling WaitFrameDelay
   (unless a key already aborted the intro). The high word is cleared, so `frames` is zero
   extended: it cannot be made negative. */
/*@ ovl0:1025 */ void DelayFrames(unsigned frames)
{
    gFrameDelay = frames;
    if (!gAbort)
        WaitFrameDelay();
}

/* ovl0:1041: grows and moves a framed rectangle, one frame at a time, with the clip rectangle
   passed to the driver each frame. The frame starts at (left, top) and walks towards
   (dstLeft, dstTop) a pixel at a time (SignOf), while its size (growW, growH) grows by 2 up to
   (width, height) and the inner rect (curWidth, curHeight) grows to the destination; the
   edges (right, bottom) shrink until the frame reaches them, then the loop ends. Called twice
   by the ovl0:15F9 scene (16E6, 18DF) and twice by ovl0:1DF3 (1B1E, 1CCA). */
/*@ ovl0:1041 */ void GrowFrame(int left, int top, int dstLeft, int dstTop, int x, int y, int width, int height,
                               int curWidth, int curHeight, int addW, int addH)
{
    int growW;
    int growH;
    int done;
    int right;
    int bottom;

    right = curWidth + addW;
    bottom = curHeight + addH;
    if (bottom >= 0x15)
        bottom = 0x15;
    growW = growH = 1;
    done = 0;
    while (!done) {
        gFrameDelay = 1;
        ClipRect(x, y * 8, left, top * 8, growW, growH * 8, 2, 0);
        if (curWidth < left)
            ClipRect(0x27, 0, curWidth, 0, 1, 0xB0, 0, 0);
        if (curHeight < top)
            ClipRect(0, 0xC0, 0, curHeight * 8, 0x28, 8, 0, 0);
        if (dstLeft + width <= right)
            ClipRect(0x27, 0, right, 0, 1, 0xB0, 0, 0);
        if (dstTop + height <= bottom)
            ClipRect(0, 0xC0, 0, bottom * 8, 0x28, 8, 0, 0);
        if (left == dstLeft && top == dstTop && growW == width && growH == height &&
            curWidth >= dstLeft && curHeight >= dstTop &&
            dstLeft + width >= right && dstTop + height >= bottom)
            done = 1;
        left += SignOf(dstLeft - left);
        top += SignOf(dstTop - top);
        if (growW != width)
            growW += 2;
        if (growW > width)
            growW = width;
        if (growH != height)
            growH += 2;
        if (growH > height)
            growH = height;
        if (++curWidth > dstLeft)
            curWidth = dstLeft;
        if (++curHeight > dstTop)
            curHeight = dstTop;
        if (dstLeft + width < right)
            right--;
        if (dstTop + height < bottom)
            bottom--;
        WaitFrameDelay();
    }
}

/* ovl0:1248: the sign of `value` as -1, 0 or 1. Used by the animations to turn a direction
   delta into a step. */
/*@ ovl0:1248 */ int SignOf(int value)
{
    if (value < 0)
        return -1;
    if (value > 0)
        return 1;
    return 0;
}
/*@ ovl0:1265 stub */ void Ovl0_1265(void) {}
/*@ ovl0:15F9 stub */ void Ovl0_15F9(void) {}
/*@ ovl0:1DF3 stub */ void Ovl0_1DF3(void) {}
/*@ ovl0:23A3 stub */ void Ovl0_23A3(void) {}
/* ovl0:2913: reveals the screen in horizontal strips. For each step 0..6 it walks the 25
   eight-line strips top to bottom and clips each strip twice: the whole strip (height
   7 - step, so the bottom part of it) and its single moving line at si*8 + 7 - step; a frame
   is waited after each step, and the screen is finally unclipped (the whole 40 by 200 region
   again). No caller of this function was found in INTRO.EXE (make packet). */
/*@ ovl0:2913 */ void RevealInStrips(void)
{
    int step;
    int strip;

    for (step = 0; step < 7; step++) {
        gFrameDelay = 1;
        for (strip = 0; strip < 25; strip++) {
            ClipRect(0, strip * 8 + 1, 0, strip * 8, 40, 7 - step, 0, 0);
            ClipRect(0, strip * 8 + 7 - step, 0, strip * 8 + 7 - step, 40, 1, 2, 0);
        }
        WaitFrameDelay();
    }
    ClipRect(0, 0, 0, 0, 40, 200, 2, 0);
}

/* ovl0:29C9: draws a grid over the whole screen in colour 12. In each of two passes, the odd
   lines of the eight-line band are vertical lines every 8 pixels starting at x = line + pass,
   the even ones horizontal lines starting at y = line + pass; the second pass fills the pixels
   the first left out. Called by the title scene (ovl0:015D), the TOWRMAGE scene (ovl0:0B20,
   twice) and ovl0:23A3. */
/*@ ovl0:29C9 */ void DrawGrid(void)
{
    int line;
    int pass;
    int x;
    int y;

    for (pass = 0; pass < 2; pass++)
        for (line = 0; line < 8; line++) {
            if (line & 1) {
                x = line + pass;
                while (x < 320) {
                    gDrawLine(x, 0, x, 199, 12);
                    x += 8;
                }
            } else {
                y = line + pass;
                while (y < 200) {
                    gDrawLine(0, y, 319, y, 12);
                    y += 8;
                }
            }
        }
}
/* ovl0:2A4E: reads a 768-byte .COL palette file into gPaletteBuf and applies it, only on the VGA
   mode (gGraphicsChoice 3). The first colour (black) is cleared and one frame is waited before
   the driver is given the palette, so the screen blanks first. */
/*@ ovl0:2A4E */ void LoadPalette(char far *name)
{
    if (gGraphicsChoice == 3) {
        LoadFile(name, gPaletteBuf, 0x300);
        gPaletteBuf[0] = gPaletteBuf[1] = gPaletteBuf[2] = 0;
        WaitFrame();
        gSetPalette(gPaletteBuf);
    }
}
/* ovl0:2A99: plays the Westwood logo card: loads WESTWOOD.CMP into the buffer slots 5/3,
   starts the driver, applies the WESTWOOD.COL palette, copies the buffers through
   f19EC_002A, makes sound 1 and shows the card for gWestwoodDelay ticks. Then, unless a key
   aborted, it shows the four SSI logo cards in turn (each from gLogoFiles into slots 5/3,
   held for its gLogoDelays entry and dissolved from page 4 to page 0), and leaves a 50-tick
   delay set. */
/*@ ovl0:2A99 */ void PlayLogoSequence(void)
{
    int i;

    LoadPackedImage("WESTWOOD.CMP", 5, 3);
    gUnknown1EE2();
    LoadPalette("WESTWOOD.COL");
    f19EC_002A(3, 0, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    PlaySound(1);
    gFrameDelay = gWestwoodDelay;
    WaitFrameDelay();
    if (gAbort)
        return;
    for (i = 0; i < 4; i++) {
        gFrameDelay = gLogoDelays[i];
        LoadPackedImage(gLogoFiles[i], 5, 3);
        f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
        Dissolve(0, 0x32, 0x140, 0x66, 4, 0);
        WaitFrameDelay();
        if (gAbort)
            return;
    }
    gFrameDelay = 0x32;
}
/* ovl0:2B7F: loads a packed .CMP image into the buffer slot `dstKind`: slot `srcKind` is the
   read buffer, the file's first word is the packed length, and the rest is unpacked by the LCW
   decoder Unpack4 (0CE9:0009) into dstKind. The unpacked size is fixed at 0xFA00 (64000, the
   largest screen image) and the block's 8-byte header is skipped. The EOB twin is LoadPacked
   (src/E1B82.C:100), which dispatches on the format byte instead; these intro files are all
   format 4. QUIRK: the FileOpen result is not checked, and the count FileRead returns is
   discarded (as in the EOB twin). */
/*@ ovl0:2B7F */ void LoadPackedImage(char far *name, int srcKind, int dstKind)
{
    unsigned size;
    void far *src;
    void far *dst;
    int slot;

    src = f19EC_0002(srcKind);
    dst = f19EC_0002(dstKind);
    slot = FileOpen(name, 1);
    FileRead(slot, (char far *)&size, 2);
    FileRead(slot, (char far *)src, size);
    FileClose(slot);
    Unpack4((char far *)src + 8, dst, 0xFA00);
}
/* ovl0:2C0D: copies the rectangle (x, y, width, height) pixel by pixel from the page
   `fromPage` to the page `toPage`, in a random order: two permutation tables (gOrderX for the
   columns at ds:290C, gOrderY for the rows at ds:2B8C) are filled 0,1,2,... and then shuffled
   with RandomRange; the copy walks the columns in order and the shuffled rows cyclically, so
   the pixels appear to dissolve in. gDrawPage selects the read page for gGetPixel and the
   write page for gPutPixel and is restored at the end. Called by the logo sequence
   (ovl0:2A99). `value` is the swap temporary of the two shuffles and the pixel colour of the
   copy; `pos` is the shuffled table entry and then the x coordinate. */
/*@ ovl0:2C0D */ void Dissolve(int x, int y, unsigned width, unsigned height, int fromPage, int toPage)
{
    int value;
    int row;
    int pos;
    int i;
    int savedPage;
    int k;
    int col;

    for (col = 0; col < width; col++)
        gOrderX[col] = col;
    for (col = 0; col < height; col++)
        gOrderY[col] = col;
    for (col = 0; col < width; col++) {
        pos = RandomRange(0, width - 1);
        i = col;
        value = gOrderX[pos];
        gOrderX[pos] = gOrderX[i];
        gOrderX[i] = value;
    }
    for (col = 0; col < height; col++) {
        pos = RandomRange(0, height - 1);
        i = col;
        value = gOrderY[pos];
        gOrderY[pos] = gOrderY[i];
        gOrderY[i] = value;
    }
    savedPage = gDrawPage;
    for (row = 0; row < height; row++) {
        k = row;
        for (col = 0; col < width; col++) {
            pos = x + gOrderX[col];
            i = y + gOrderY[k];
            k++;
            if (k >= height)
                k = 0;
            gDrawPage = fromPage;
            value = gGetPixel(pos, i);
            gDrawPage = toPage;
            gPutPixel(pos, i, value);
        }
    }
    gDrawPage = savedPage;
}

/* ovl0:2D4E: plays the game sound numbered `sound` (the argument is an index, not a driver
   number). The driver number comes from gSoundBuf (a byte; 0xFF means silence). gSoundChoice
   selects the path: 1 (AdLib) queues the driver number and returns, 2 (PC speaker) and 3
   (Tandy) start the speaker routine the low seven bits select through GetSoundData; an
   unknown card does nothing. On the Tandy path a nonzero driver number resumes the driver
   only while gTandyOn is clear. This is the EOB PlaySound (src/E177D.C:166) with a one-based
   gSoundChoice, without the sound-on test and the 0x96 map cutoff.
   The map byte is kept both in mapByte and, zero extended, in driverSound (SI), which is why
   the index and the later tests share a register; the EOB twin keeps only the int.
   QUIRK 2DB4: like the EOB code the `driverSound < 0` test is dead (the map byte is zero
   extended), and the `driverSound > 0` branch stores 0 into gTandyOn where EOB stores 1. */
/*@ ovl0:2D4E */ void PlaySound(int sound)
{
    unsigned char mapByte;
    int driverSound;
    void far *routine;

    driverSound = sound;
    mapByte = gSoundBuf[driverSound];
    if (mapByte == 0xFF)
        mapByte = 0;
    driverSound = mapByte;
    switch (gSoundChoice) {
    case 1:
        QueueSound(driverSound);
        return;
    case 2:
        break;
    case 3:
        if (driverSound < 0 && gTandyOn) {
            gTandyOn = 0;
            SoundDriverResume();
        }
        if (driverSound > 0 && gTandyOn == 0) {
            gTandyOn = 0;
            SoundDriverResume();
        }
        break;
    default:
        return;
    }
    routine = GetSoundData(driverSound & 0x7F);
    StartSound(routine);
}