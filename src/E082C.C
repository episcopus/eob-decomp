/* Unit at EOB.EXE 082C:000B (2976 bytes, 28 functions): a text-field panel and its drawing
   helpers. The main entry 082C:000B edits a string in place (a key loop that redraws the field
   and its cursor), a small set of 082C:0898/090A helpers draw text and numbers with a shadow,
   082C:04C4/074D draw rectangles for the panel frames and 082C:051B formats a name, loads a
   packed screen through LoadPacked and clips it. The whole unit reads the region table at ds:009F
   (src/E1A29.C), the screen text pointer at ds:80CA (src/E14EF.C), the line pointer at ds:8062
   (src/E1A1F.C) and the game's 32-bit tick counter at ds:848A (incremented by the INT 1Ch handler,
   src/E1D8F.ASM). Placeholder names until the callers name them. */

/*@flags -ml -O -Z */

#include <dos.h>

struct Region {             /* 16-byte records at ds:009F (src/E1A29.C) */
    int left;               /* +0: left edge in 8-pixel columns */
    int top;                /* +2: top edge in lines */
    int width;              /* +4: width in 8-pixel columns */
    int height;             /* +6: height in lines */
    int saved0;
    int saved1;
    int saved2;
    int saved3;
};

extern struct Region gRegions[]; /*@ ds:009F */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern int g8102; /*@ ds:8102: the video mode the game initialised (3 is CGA-compatible) */
extern int g8140; /*@ ds:8140: x of the last click (src/E14EF.C) */
extern int g8142; /*@ ds:8142: y of the last click (src/E14EF.C) */
extern int g8115; /*@ ds:8115: nonzero while a status line is up (set to 2 by LoadOverlayScreen) */
extern unsigned long gTicks; /*@ ds:848A: BIOS-like tick counter, stepped by the INT 1Ch handler */
extern char gNameBuf[]; /*@ ds:FE6E */ /* buffer the filename/format helpers build into */
extern char gOverlayBuf[]; /*@ ds:113F */ /* a DGROUP buffer the overlay loader is passed */

extern int strlen(char far *s); /*@ 0000:67F6 */
extern int rand(void); /*@ 0000:55B7 */
extern unsigned long farcoreleft(void); /*@ 0000:5244 */
extern int heapcheck(void); /*@ 0000:74B0 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern int toupper(int c); /*@ 0000:5BD2 */
extern void far *memmove(void far *dst, void far *src, unsigned n); /*@ 0000:63F5 */
extern char *ltoa(long value, char *string, int radix); /*@ 0000:62F1 */
extern char *itoa(int value, char *string, int radix); /*@ 0000:62AA */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void f19EC_002A(int a, int b, int c, int d); /*@ 19EC:002A */
extern void LoadPacked(char far *name, int srcKind, int dstKind); /*@ 1B82:0041 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern unsigned long LargestFreeBlock(void); /*@ 19BC:023D */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void f1DE7_060A(char far *text); /*@ 1DE7:060A */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void f1A29_004D(int mode, void far *src, int x, int y, int region); /*@ 1A29:004D */
extern void f1A29_008B(int mode, void far *src, int x, int y, int region); /*@ 1A29:008B */
extern void f1A29_00C9(int mode, void far *src, int x, int y, int region); /*@ 1A29:00C9 */
extern void f1AAD_0480(void); /*@ 1AAD:0480 */
extern int f1AAD_038E(void); /*@ 1AAD:038E */
extern int f1AAD_053A(void); /*@ 1AAD:053A */
extern void f1AAD_0009(void); /*@ 1AAD:0009 */

/* 082C:000B: Purpose unknown: the interactive text-field editor. It draws a string with its
   cursor, then loops reading a key (1AAD:038E waits for one, 1AAD:053A returns it) and editing
   the buffer in place with left/right/home/end/backspace/delete and the character keys, redrawing
   the field after every change and returning the cursor position, or -1 on Escape. Not written
   yet: 1050 bytes of key handling. */
/*@ 082C:000B stub */ void EditTextField(void) {}

/* 082C:0425: busy-waits until the 32-bit tick counter ds:848A has advanced `ticks` ticks. The
   counter is stepped by the INT 1Ch handler (src/E1D8F.ASM), so this is a delay in BIOS ticks. */
/*@ 082C:0425 */ void WaitTicks(int ticks)
{
    unsigned long target;

    target = gTicks + (unsigned long)ticks;
    while (gTicks < target)
        ;
}

/* 082C:0458: roll `count` dice with `sides` sides and add `bonus`: each roll is rand() scaled into
   1..sides (the overflow value sides+1 is folded back by -sides), the sum of the rolls plus the
   bonus is returned. Already named RollDice by its caller 0A72:01AE (src/E0A72.C). */
/*@ 082C:0458 */ int RollDice(char count, char sides, char bonus)
{
    int sum;
    int i;
    int roll;

    sum = 0;
    if (sides > 0 && count > 0) {
        for (i = 0; i < count; i++) {
            roll = (int)((long)rand() * sides / 32767) + 1;
            if (roll > sides)
                roll -= sides;
            sum += roll;
        }
    }
    return sum + bonus;
}

/* 082C:04C4: draw the four edges of the rectangle (x1, y1)-(x2, y2) in `colour` with four calls to
   the line routine ds:8062, with no clipping (unlike DrawBox at 1A1F:000C). */
/*@ 082C:04C4 */ void DrawRect(int x1, int y1, int x2, int y2, int colour)
{
    gDrawLine(x1, y1, x2, y1, colour);
    gDrawLine(x1, y2, x2, y2, colour);
    gDrawLine(x1, y1, x1, y2, colour);
    gDrawLine(x2, y1, x2, y2, colour);
}

/* 082C:051B: build the art file name "%s.%s" (name and CPS or EGA for the video mode at ds:8102)
   into gNameBuf, load it with LoadPacked (source and destination kind `srcKind`/`dstKind`), and
   when `mode` is not -1 copy the loaded page over the screen through the driver pair 19EC:002A and
   ClipRect, choosing the kind and clip extras by the video mode and `mode`. `extra` is passed to
   19EC:002A and then bumped. Draft: our shared ClipRect tail is placed before the frame that the
   mode-2 branch needs, so one branch byte differs (2 bytes; re/symbols.toml, e082c_051b). */
/*@ 082C:051B draft */ void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode)
{
    sprintf(gNameBuf, "%s.%s", name, g8102 == 3 ? "CPS" : "EGA");
    LoadPacked(gNameBuf, srcKind, dstKind);
    if (mode != -1) {
        if (g8102 == 0 || g8102 == 1) {
            if (mode == 2) {
                f19EC_002A(dstKind, 4, FP_OFF(extra), FP_SEG(extra));
                ClipRect(0, 0, 0, 0, 40, 200, 4, 2);
            } else
                f19EC_002A(dstKind, mode, FP_OFF(extra), FP_SEG(extra));
        } else if (g8102 == 2) {
            if (mode == 0) {
                f19EC_002A(dstKind, 2, FP_OFF(extra), FP_SEG(extra));
                ClipRect(0, 0, 0, 0, 40, 200, 2, 0);
            } else
                f19EC_002A(dstKind, mode, FP_OFF(extra), FP_SEG(extra));
        } else
            f19EC_002A(dstKind, mode, FP_OFF(extra), FP_SEG(extra));
    }
    extra++;
}

/* 082C:05EC: load and show a named art page with LoadScreenImage and mark the status line as up
   (g8115 = 2). Called by many overlays to put a picture on the screen. */
/*@ 082C:05EC */ void LoadOverlayScreen(char far *name, int srcKind, int dstKind)
{
    LoadScreenImage(name, gOverlayBuf, srcKind, dstKind, -1);
    g8115 = 2;
}

/* 082C:0613: the point (x, y) is inside the inclusive rectangle built from its left, top, right
   and bottom edges: 1 when all four tests hold, 0 otherwise. Same test as PointInRect at
   14EF:00E1; this copy is reached with the last click (ClickInRect). */
/*@ 082C:0613 */ int IsPointInsideRect(int x, int y, int left, int top, int right, int bottom)
{
    if (x < left || x > right)
        return 0;
    if (y < top || y > bottom)
        return 0;
    return 1;
}

/* 082C:063B: draw through the region front end 1A29:000F with the coordinates made relative to
   `region`: x is first rounded down to an even value and the region's left edge in pixels (left *
   8) is subtracted, y has the region's top line subtracted. */
/*@ 082C:063B */ void DrawRegion000F(int mode, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    f1A29_000F(mode, src, x, y, region);
}

/* 082C:0681: like DrawRegion000F but through the region front end 1A29:004D. */
/*@ 082C:0681 */ void DrawRegion004D(int mode, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    f1A29_004D(mode, src, x, y, region);
}

/* 082C:06C7: like DrawRegion000F but through the region front end 1A29:008B. */
/*@ 082C:06C7 */ void DrawRegion008B(int mode, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    f1A29_008B(mode, src, x, y, region);
}

/* 082C:070D: like DrawRegion000F but through the region front end 1A29:00C9 and without the even
   rounding of x. */
/*@ 082C:070D */ void DrawRegion00C9(int mode, void far *src, int x, int y, int region)
{
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    f1A29_00C9(mode, src, x, y, region);
}

/* 082C:074D: draw a bevelled rectangle: the top and right edges in `colourA`, the left and bottom
   edges in `colourB`, each with a call to ds:8062. */
/*@ 082C:074D */ void DrawBevelBox(int x1, int y1, int x2, int y2, int colourA, int colourB)
{
    gDrawLine(x1, y1, x2, y1, colourA);
    gDrawLine(x1, y1, x1, y2, colourB);
    gDrawLine(x1, y2, x2, y2, colourB);
    gDrawLine(x2, y1, x2, y2, colourA);
}

/* 082C:07A4: exchange the bytes at the two far addresses. */
/*@ 082C:07A4 */ void SwapChars(char far *pa, char far *pb)
{
    char saved;

    saved = *pa;
    *pa = *pb;
    *pb = saved;
}

/* 082C:07C5: copy `len` bytes between three far buffers as a rotation: the first into the third,
   the second into the first, then the third (already holding the first) into the second. Uses
   Unpack0, the format-0 raw copy, for each leg. `len` is a signed int widened to the long
   Unpack0 wants. */
/*@ 082C:07C5 */ void RotateBlocks(char far *pa, char far *pb, char far *pc, register int len)
{
    Unpack0(pa, pc, len);
    Unpack0(pb, pa, len);
    Unpack0(pc, pb, len);
}

/* 082C:081A: draw a one-line memory report at (x, y) through the text routine ds:80CA and, when
   `wait` is set, wait for a key. On a healthy far heap it prints "largest free:total free"; when
   heapcheck reports corruption it prints the "HEAP CORRUPT!!" message with the largest free block.
   g8115, the status-line flag, is cleared while the report is drawn and restored after. */
/*@ 082C:081A */ void ShowMemoryDiagnostics(int x, int y, int wait)
{
    int saved;

    saved = g8115;
    g8115 = 0;
    if (heapcheck() >= 0)
        sprintf(gNameBuf, "%lu:%lu", LargestFreeBlock(), farcoreleft());
    else
        sprintf(gNameBuf, "HEAP CORRUPT!! (free memory) = %lu\n", LargestFreeBlock());
    gDrawText(gNameBuf, x, y, 0xf, 0xc);
    if (wait)
        f1AAD_053A();
    g8115 = saved;
}

/* 082C:0898: draw one text string at (x, y): first its drop shadow at (x-1, y), (x, y+1) and
   (x-1, y+1) in the shadow colour 0x0C, then the text itself in `colour`. `extra` is forwarded as
   the fifth argument of the first shadow draw. Used by the option-list and field code. */
/*@ 082C:0898 */ void DrawShadowText(char far *text, int x, int y, int color, int extra)
{
    gDrawText(text, x - 1, y, 0xc, extra);
    gDrawText(text, x, y + 1, 0xc, 0);
    gDrawText(text, x - 1, y + 1, 0xc, 0);
    gDrawText(text, x, y, color, 0);
}

/* 082C:090A: the integer twin of DrawShadowText: print `value` as decimal at (x, y) with the same
   drop shadow (0x0C) through PrintInt. */
/*@ 082C:090A */ void DrawShadowInt(int value, int x, int y, int color, int extra)
{
    PrintInt(value, x - 1, y, 0xc, extra);
    PrintInt(value, x, y + 1, 0xc, 0);
    PrintInt(value, x - 1, y + 1, 0xc, 0);
    PrintInt(value, x, y, color, 0);
}

/* 082C:0974: the width in pixels of the decimal form of the long `value`, at three pixels per
   character (the font is three pixels wide). Formats into a 20-byte buffer with ltoa, so this is
   used to centre a number before drawing it. */
/*@ 082C:0974 */ int NumberWidthLong(long value)
{
    char buf[20];

    ltoa(value, buf, 10);
    return strlen(buf) * 3;
}

/* 082C:09A6: the int twin of NumberWidthLong (itoa into a 20-byte buffer, times three). */
/*@ 082C:09A6 */ int NumberWidthInt(int value)
{
    char buf[20];

    itoa(value, buf, 10);
    return strlen(buf) * 3;
}

/* 082C:09D5: draw a filled bevelled panel: when `fill` is not -1 the interior (x1+1, y1+1)-(x2-1,
   y2-1) is filled through ds:806A, then the top and right edges are drawn in `colourA` (the top
   starting at x1+1 and the right ending at y2-1, so they miss the far corners) and the left and
   bottom edges in `colourB`. */
/*@ 082C:09D5 */ void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill)
{
    if (fill != -1)
        gFillRect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, fill);
    gDrawLine(x1 + 1, y1, x2, y1, colourA);
    gDrawLine(x2, y1, x2, y2 - 1, colourA);
    gDrawLine(x1, y1, x1, y2, colourB);
    gDrawLine(x1, y2, x2, y2, colourB);
}

/* 082C:0A53: the thin wrapper the game quits through, calling ShutdownGame (07C8:058A). */
/*@ 082C:0A53 */ void QuitGame(void)
{
    ShutdownGame();
}

/* 082C:0A5D: is the last click (ds:8140, ds:8142) inside the rectangle (left, top)-(right, bottom)?
   A wrapper over IsPointInsideRect with the click globals as the point. */
/*@ 082C:0A5D */ int ClickInRect(int left, int top, int right, int bottom)
{
    return IsPointInsideRect(g8140, g8142, left, top, right, bottom);
}

/* 082C:0A7D: read `len` bytes of the file `name` into `buf`: FileOpen in mode 1, FileRead, then
   FileClose. The long length distinguishes it from LoadFile (1B82:0009). */
/*@ 082C:0A7D */ void ReadFileBytes(char far *name, void far *buf, unsigned long len)
{
    int handle;

    handle = FileOpen(name, 1);
    FileRead(handle, buf, len);
    FileClose(handle);
}

/* 082C:0AB4: fold `value` into the range [low, high], choosing which bound to snap to by `invert`.
   The two ifs share one result variable, so the same register is rewritten. */
/*@ 082C:0AB4 */ int ClampValue(int value, int low, int high, int invert)
{
    if (value < low) {
        if (invert)
            value = high;
        else
            value = low;
    }
    if (value > high) {
        if (!invert)
            value = high;
        else
            value = low;
    }
    return value;
}

/* 082C:0AE8: format "%s%d.%s" into gNameBuf (a far string, an int and a far string), the shape of
   a numbered art file name. */
/*@ 082C:0AE8 */ void FormatName(char far *prefix, int number, char far *suffix)
{
    sprintf(gNameBuf, "%s%d.%s", prefix, number, suffix);
}

/* 082C:0B0E: draw one line of text at (x, y) with gDrawText and then print the memory report
   directly below it (y + 8) with ShowMemoryDiagnostics, waiting for a key. A debug status screen. */
/*@ 082C:0B0E */ void ShowStatus(char far *text, int x, int y)
{
    gDrawText(text, x, y, 0xf, 0xc);
    ShowMemoryDiagnostics(x, y + 8, 1);
}

/* 082C:0B43: wait `seconds` seconds (18 BIOS ticks each) and then wait for a key: busy-wait on the
   tick counter, clear the pending key (1AAD:0480), block until 1AAD:038E reports one, and clear it
   again. */
/*@ 082C:0B43 */ void DelaySeconds(int seconds)
{
    unsigned long target;

    target = gTicks + (unsigned long)(seconds * 18);
    while (target > gTicks)
        ;
    f1AAD_0480();
    while (!f1AAD_038E())
        ;
    f1AAD_0480();
}

/* 082C:0B8F: write `text` to the message line (1DE7:060A) and play sound 0x4F. */
/*@ 082C:0B8F */ void SayAndBeep(char far *text)
{
    f1DE7_060A(text);
    PlaySound(0x4f);
}
