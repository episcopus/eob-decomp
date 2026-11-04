/* Unit at EOB.EXE 082C:000B (2976 bytes, 28 functions): the field editor and the shared drawing
   helpers. The main entry EditTextField (082C:000B) edits a string in place with a blinking
   cursor; DrawShadowText (082C:0898) and DrawShadowInt (082C:090A) draw text and numbers with a
   drop shadow; DrawRect (082C:04C4), DrawBevelBox (082C:074D) and DrawFilledPanel (082C:09D5)
   draw rectangles and bevelled panels; the four DrawRegion front ends (082C:063B/0681/06C7/070D)
   drive the region table at ds:009F (src/E1A29.C). LoadScreenImage (082C:051B) formats a name,
   loads a packed screen through LoadPacked and clips it; FormatName (082C:0AE8) builds numbered
   names; NumberWidthLong/NumberWidthInt (082C:0974/09A6) gives number widths; SwapChars
   (082C:07A4) exchanges two bytes. The tail holds RollDice (082C:0458), the tick helpers
   (082C:0425/0B43), the memory report (082C:081A/0B0E), ReadFileBytes (082C:0A7D), the block swap
   SwapBlocks (082C:07C5), ClampOrWrap (082C:0AB4), the rectangle hit tests (082C:0613/0A5D), the
   quit wrapper (082C:0A53) and SayAndPlaySound (082C:0B8F). Only the four DrawRegion front ends
   in this unit read the region table at ds:009F (src/E1A29.C); the unit also calls the draw-text
   routine pointer gDrawText at ds:80CA (src/E14EF.C), the line routine pointer gDrawLine at
   ds:8062 (src/E1A1F.C) and reads the game's
   32-bit tick counter at ds:848A (incremented by the INT 1Ch handler, src/E1D8F.ASM). The graphics
   mode gGraphicsChoice and the video modes are whitepaper 2.6; the -v build (debug information,
   which keeps identical code tails apart) is whitepaper 2.1.1. Placeholder names until the callers name them. */

/*@flags -ml -O -Z -v */

#include <dos.h>

struct Region {             /* 16-byte records at ds:009F (src/E1A29.C) */
    int left;               /* +0: left edge in 8-pixel columns */
    int top;                /* +2: top edge in lines */
    int width;              /* +4: width in 8-pixel columns */
    int height;             /* +6: height in lines */
    int textAttr1;              /* +8: text attribute 1 [H] (gTextAttr1, src/E1DE7.C:26) */
    int textAttr2;              /* +a: text attribute 2 [H] (gTextAttr2, src/E1DE7.C:27) */
    int textCol;             /* +c: cursor column (gTextCol) */
    int textRow;                /* +e: cursor row (gTextRow) */
};

/* The keys the text-field editor reacts to. WaitGameKey (1AAD:053A) returns a translated key
   code: it maps the scancode through kScanToKey (1AAD:0186) and TranslateKey (1AAD:0223). The
   keypad Del scancode 0x53 -> kScanToKey[0x53] = 0x68 -> kKeypadCodes index 0x68-0x4B = 0x1D =
   0xAD (1AAD:0357); EditTextField folds 0xAD and 0x7F onto KeyDelete. The value 0x7E is also the
   code of '~' (Shift+`, scancode 0x29 -> kScanToKey[0x29] = 1 -> kShiftedCodes[1] = 0x7E,
   1AAD:0319), and the editor treats it as the top of the printable range, so typing '~' deletes:
   QUIRK 082C:019F (the delete test) and 082C:01C1 (the printable-range top). KeyLeft = 0xB5 is
   the Left arrow (scancode 0x4B -> kScanToKey[0x4B] = 0x5C ->
   kKeypadCodes index 0x5C-0x4B = 0x11, 1AAD:0357) and KeyRight = 0xB3 the Right arrow (scancode
   0x4D -> kScanToKey[0x4D] = 0x66 -> kKeypadCodes index 0x66-0x4B = 0x1B, 1AAD:0357). */
enum {
    KeyBackspace = 8,
    KeyReturn = 0x0d,
    KeyEsc = 0x1b,
    KeySpace = 0x20,
    KeyDelete = 0x7e,          /* the code of '~' as well; the editor folds it onto delete (see above) */
    KeyKeypadDel = 0xad,       /* keypad Del (1AAD:0357); the editor folds it onto KeyDelete below */
    KeyAsciiDel = 0x7f,        /* ASCII DEL; the editor folds it onto KeyDelete below */
    KeyTilde = 0x7e,           /* the printable-range top; same value as KeyDelete (QUIRK 082C:01C1) */
    KeyLeft = 0xb5,
    KeyRight = 0xb3
};

/* DrawFilledPanel's "draw no interior" value; its callers pass -1 for it, for example
   src/EOV2.C:502, :503, src/E0D1C.C:720 and :725, src/E1547.C:226 and src/EOV2.C:364. */
#define NO_FILL (-1)

extern struct Region gRegions[]; /*@ ds:009F */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int colour); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern int gGraphicsChoice; /*@ ds:8102: the graphics mode chosen at start-up: 0 cga, 1 tandy, 2 ega,
                             3 mcga/VGA, 4 vga (src/E07C8.C:39-43, the list at :42; src/E17A6.ASM:233
                             names 3 MCGA; whitepaper 2.6: 1 Tandy, 3 VGA). LoadScreenImage picks CPS
                             for 3, the MCGA/VGA art suffix (the sprintf below) */
extern int gClickX; /*@ ds:8140: x of the last click, written by src/E1AAD.ASM:396 and also read by src/E14EF.C:325 */
extern int gClickY; /*@ ds:8142: y of the last click, written by src/E1AAD.ASM:405 and also read by src/E14EF.C:325 */
extern int gDrawPage; /*@ ds:8115: the graphics driver's draw page (2 while LoadOverlayScreen draws off screen) */
extern unsigned long gTicks; /*@ ds:848A: the game's own 32-bit tick counter, stepped by the INT 1Ch handler at src/E1D8F.ASM:76 (label 1D8F:0055); the increment of ds:848A is at src/E1D8F.ASM:91 (`inc word ptr ds:[848Ah]`) */
extern char gFileNameBuf[]; /*@ ds:FE6E */ /* the shared text buffer the filename/format helpers and the memory report build into */
extern char gCgaMapDefault[]; /*@ ds:113F */ /* the DGROUP image buffer, passed as LoadScreenImage's extra pointer to CopyPage, which hands it to the far handler at ds:8046 only when two slots hold non-zero segments that differ (src/E19EC.C:29-35); overlays cut sprite cells out of it (src/EOV2.C:209, src/EOV5.C:1021) */
extern char gCursorChar; /*@ ds:31AC: the one character drawn at the editing cursor */
extern char gCursorOn; /*@ ds:FC4C: nonzero while the cursor is drawn (blinks in the editor) */

extern int strlen(char far *s); /*@ 0000:67F6 */
extern int rand(void); /*@ 0000:55B7 */
extern unsigned long farcoreleft(void); /*@ 0000:5244 */
extern int heapcheck(void); /*@ 0000:74B0 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern int toupper(int c); /*@ 0000:5BD2 */
extern void far *memmove(void far *dst, void far *src, unsigned n); /*@ 0000:63F5 */
extern char *ltoa(long value, char *string, int radix); /*@ 0000:62F1 */
extern char *itoa(int value, char *string, int radix); /*@ 0000:62AA */
extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void CopyPage(int a, int b, int c, int d); /*@ 19EC:002A: called here with c/d = FP_OFF/FP_SEG(extra); src/E19EC.C:29-35 says c and d are unused by its own tests */
extern void LoadPacked(char far *name, int srcKind, int dstKind); /*@ 1B82:0041 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern long LargestFreeBlock(void); /*@ 19BC:023D */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern long FileRead(int slot, void far *buf, unsigned long len); /*@ 1C0E:0400: returns the byte count; `slot` is FileOpen's slot index, not a DOS handle (src/E1B82.C:56-63) */
extern void FileClose(int slot); /*@ 1C0E:03BB: `slot` is FileOpen's slot index, not a DOS handle (src/E1B82.C:56-64) */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void DrawText(char far *text); /*@ 1DE7:060A */  /* the text console's string renderer (src/E1DE7.C) */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void DrawShapeFlipped(int page, void far *src, int x, int y, int region); /*@ 1A29:004D */
extern void DrawShapeRemapped(int page, void far *src, int x, int y, int region); /*@ 1A29:008B */
extern void DrawShapeFlippedRemapped(int page, void far *src, int x, int y, int region); /*@ 1A29:00C9 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern int ReadKey(void); /*@ 1AAD:038E */
extern int WaitGameKey(void); /*@ 1AAD:053A */

/* siblings of this file called before they are defined */
void DrawShadowText(char far *text, int x, int y, int colour, int colour2);

/* 082C:000B: the interactive text-field editor. Draws the string `text` at (x, y) (x in 8-pixel
   cells, y in lines) with the character under the cursor, then loops: it waits for a key
   (1AAD:038E) while blinking the cursor every two ticks through gCursorOn and the tick counter
   ds:848A, edits `text` in place and redraws the field. Printable keys insert (uppercased; a
   space is refused at the start), KeyBackspace deletes to the left of the cursor, KeyDelete at
   it, the arrow keys move it, KeyReturn ends the edit and returns the new length and KeyEsc ends
   it and returns -1. `maxlen` is the highest buffer index (the buffer holds maxlen+1 bytes
   including the terminator); the two CopyRegion calls (082C:006D and 082C:038A) set the field's
   maxlen+2 by 9 clip rectangle (see the QUIRK below), and the text and cursor are drawn with
   DrawShadowText. `colour` is the text colour, `attr` is the second colour passed to the draw
   call (DrawShadowText's `colour2`, below) and `cursorAttr` is the cursor colour: ovl0 passes 0xf, 0
   and 8 (src/EOV0.C:1312), ovl2 passes gTextAttr1, gTextAttr2 and 8 (src/EOV2.C:504).
   Reached from FindWordPanel (ovl2:09ED) and ovl0:23EC. The unit is compiled
   with -v (debug information; the flags line), which keeps the insert path's `text[maxlen] = 0`
   and the append path's `text[cur] = 0` as two separate stores; without it TCC merges them into
   one shared store. Evidence: without -v, make match reports "DIFFER EditTextField +0182: ours
   jmp 0x278, original jmp 0x282" (27 of 28); with -v, 28 of 28 (2026-11-02; whitepaper 2.1.1). */
/*@ 082C:000B */ int EditTextField(char far *text, char x, char y, char maxlen, char colour,
                                    char attr, char cursorAttr)
{
    char cur;
    char len;
    unsigned long lastBlinkTick;
    int key;
    int i;

    len = strlen(text);
    if (len > maxlen) {
        len = maxlen;
        text[maxlen] = 0;
    }
    cur = len;
    if (cur >= maxlen)
        cur--;
    CopyRegion(x - 1, y, 0, 0xbf, maxlen + 2, 9, 0, 2);  /* 0xBF = 191: the screen's last line
                                                           (200) minus the field's 9 lines */
    DrawShadowText(text, x * 8, y, colour, attr);
    if (cur < len)
        gCursorChar = text[cur];
    else
        gCursorChar = ' ';
    DrawShadowText(&gCursorChar, (x + cur) * 8, y, colour, cursorAttr);
    gCursorOn = 1;
    lastBlinkTick = gTicks;
    for (;;) {
        while (!ReadKey()) {
            if (lastBlinkTick + 2 <= gTicks) {
                lastBlinkTick = gTicks;
                if (gCursorOn) {
                    DrawShadowText(&gCursorChar, (x + cur) * 8, y, colour, cursorAttr);
                    gCursorOn = 0;
                } else
                    gCursorOn = 1;
            }
        }
        key = WaitGameKey() & 0xff;
        if (key == KeyKeypadDel)
            key = KeyDelete;
        if (key == KeyAsciiDel)
            key = KeyDelete;
        if (key == KeyBackspace)
            goto special;
        if (key == KeyEsc)
            goto special;
        if (key == KeyReturn)
            goto special;
        if (key == KeyDelete)
            goto special;
        if (key == KeyLeft)
            goto special;
        if (key == KeyRight)
            goto special;
        if (key < KeySpace)
            goto special;
        if (key > KeyTilde)
            goto special;
        if (cur == 0 && key == KeySpace)
            goto redraw;
        if (cur < len) {
            for (i = maxlen - 1; cur <= i; i--)
                text[i + 1] = text[i];
            text[cur] = toupper(key);
            cur++;
            if (len == maxlen)
                text[maxlen] = 0;
        } else {
            if (cur == maxlen) {
                cur--;
                len--;
            }
            text[cur] = toupper(key);
            cur++;
            text[cur] = 0;
        }
        if (++len > maxlen)
            len = maxlen;
        if (cur > maxlen - 1)
            cur = maxlen - 1;
        goto redraw;
    special:
        if (key == KeyBackspace) {
            if (cur < len) {
                if (cur > 0) {
                    for (i = cur; i < maxlen; i++)
                        text[i - 1] = text[i];
                    cur--;
                    text[--len] = 0;
                } else
                    goto redraw;
            } else {
                if (len > 0) {
                    text[--len] = 0;
                    cur--;
                } else
                    goto redraw;
            }
        } else if (key == KeyLeft) {
            if (cur > 0)
                cur--;
        } else if (key == KeyRight) {
            if (cur < len && cur < maxlen - 1)
                cur++;
        } else if (key == KeyDelete) {
            if (text[cur] != 0) {
                memmove(&text[cur], &text[cur + 1], len - cur);
                len--;
            }
        }
    redraw:
        /* QUIRK 082C:038A: the second CopyRegion sets the same field rectangle as the first at
           082C:006D, but with the two corner pairs exchanged ((0, 0xbf) then (x - 1, y) here,
           where the first has (x - 1, y) then (0, 0xbf)) and the extras 2, 0 instead of 0, 2. */
        CopyRegion(0, 0xbf, x - 1, y, maxlen + 2, 9, 2, 0);  /* 0xBF as at 082C:006D */
        DrawShadowText(text, x * 8, y, colour, attr);
        if (cur < len)
            gCursorChar = text[cur];
        else
            gCursorChar = ' ';
        DrawShadowText(&gCursorChar, (x + cur) * 8, y, colour, cursorAttr);
        if (key == KeyReturn || key == KeyEsc)
            break;
    }
    if (key == KeyEsc)
        return -1;
    return len;
}

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

/* 082C:051B: build the art file name "%s.%s" (name plus CPS for the MCGA/VGA mode 3 at
   gGraphicsChoice, otherwise EGA) into gFileNameBuf, load it with LoadPacked
   (source/destination kind `srcKind`/`dstKind`), and, unless `mode` is -1, blit the loaded page
   through the driver pair 19EC:002A. In two special cases it also clips the frame with CopyRegion:
   graphics choice 0 or 1 with mode 2 (kind 4, extras 4 and 2) and graphics choice 2 with mode 0
   (kind 2, extras 2 and 0); every other case uses `mode` as the kind. `extra` is the pointer
   argument passed to 19EC:002A: a 32-byte colour map of values 0..3 (ds:113F, 2067:0000 and the
   like), which the handler at ds:8046 gets (src/E19EC.C:29-35).
   QUIRK 082C:05E5: the final `extra++` bumps the local pointer and has no later use, so it is
   dead. */
/*@ 082C:051B */ void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode)
{
    sprintf(gFileNameBuf, "%s.%s", name, gGraphicsChoice == 3 ? "CPS" : "EGA");
    LoadPacked(gFileNameBuf, srcKind, dstKind);
    if (mode == -1)
        goto done;
    if ((gGraphicsChoice == 0 || gGraphicsChoice == 1) && mode == 2) {
        CopyPage(dstKind, 4, FP_OFF(extra), FP_SEG(extra));
        CopyRegion(0, 0, 0, 0, 40, 200, 4, 2);  /* 40 by 200: the whole screen, in 8-pixel cells and lines */
    } else if (gGraphicsChoice != 2 || mode != 0)
        CopyPage(dstKind, mode, FP_OFF(extra), FP_SEG(extra));
    else {
        CopyPage(dstKind, 2, FP_OFF(extra), FP_SEG(extra));
        CopyRegion(0, 0, 0, 0, 40, 200, 2, 0);  /* 40 by 200: the whole screen, as above */
    }
done:
    extra++;
}

/* 082C:05EC: load the art page `name` into the slot `dstKind` (LoadPacked) and set the draw
   page gDrawPage to 2. With mode -1 LoadScreenImage skips the 19EC:002A blit, so nothing
   is drawn here: the page reaches the screen only when `dstKind` is itself a screen slot (segment
   0xB800 or 0xA000, src/E19EC.C:5-7). Called by many overlays, all with dstKind 3, for example
   src/EOV0.C:1328 and src/EOV2.C:208. `name` is the art name the caller passes (for example
   "ITEMICN", src/EOV2.C:208); gCgaMapDefault is the `extra` pointer passed on to LoadScreenImage. The
   "Overlay" in this name refers to that buffer, the DGROUP image buffer, not a VROOMM code overlay. */
/*@ 082C:05EC */ void LoadOverlayScreen(char far *name, int srcKind, int dstKind)
{
    LoadScreenImage(name, gCgaMapDefault, srcKind, dstKind, -1);
    gDrawPage = 2;
}

/* 082C:0613: the point (x, y) is inside the inclusive rectangle built from its left, top, right
   and bottom edges: 1 when all four tests hold, 0 otherwise. The name is a second name for the same
   test as IsPointInRect at 14EF:00E1 (src/E14EF.C:146); a naming pass should settle one name across
   the project. This copy is reached with the last click (ClickInRect). */
/*@ 082C:0613 */ int IsPointInsideRect(int x, int y, int left, int top, int right, int bottom)
{
    if (x < left || x > right)
        return 0;
    if (y < top || y > bottom)
        return 0;
    return 1;
}

/* 082C:063B: the region front end 1A29:000F is Purpose unknown, so this wrapper's name is only that
   front end's address. It makes the coordinates relative to `region`: x is first rounded down to an
   even value and the region's left edge in pixels (left * 8) is subtracted, y has the region's top
   line subtracted. */
/*@ 082C:063B */ void DrawShapeClipped(int page, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    DrawShape(page, src, x, y, region);
}

/* 082C:0681: like DrawShapeClipped (even rounding of x and the region-relative coordinates) but
   through the region front end 1A29:004D, which is Purpose unknown, so the name is only that
   front end's address. */
/*@ 082C:0681 */ void DrawShapeClippedFlipped(int page, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    DrawShapeFlipped(page, src, x, y, region);
}

/* 082C:06C7: like DrawShapeClipped (even rounding of x and the region-relative coordinates) but
   through the region front end 1A29:008B, which is Purpose unknown, so the name is only that
   front end's address. */
/*@ 082C:06C7 */ void DrawShapeClippedRemapped(int page, void far *src, int x, int y, int region)
{
    x &= -2;
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    DrawShapeRemapped(page, src, x, y, region);
}

/* 082C:070D: like DrawShapeClipped through the region front end 1A29:00C9, which is Purpose unknown,
   so the name is only that front end's address; unlike the other three wrappers it does not round
   x down to an even value. */
/*@ 082C:070D */ void DrawShapeClippedRemapFlipped(int page, void far *src, int x, int y, int region)
{
    x -= gRegions[region].left * 8;
    y -= gRegions[region].top;
    DrawShapeFlippedRemapped(page, src, x, y, region);
}

/* 082C:074D: draw a bevelled rectangle: the top and right edges in `colourA`, the left and bottom
   edges in `colourB`, each with a call to ds:8062. No caller found in src/. */
/*@ 082C:074D */ void DrawBevelBox(int x1, int y1, int x2, int y2, int colourA, int colourB)
{
    gDrawLine(x1, y1, x2, y1, colourA);
    gDrawLine(x1, y1, x1, y2, colourB);
    gDrawLine(x1, y2, x2, y2, colourB);
    gDrawLine(x2, y1, x2, y2, colourA);
}

/* 082C:07A4: exchange the bytes at the two far addresses. The only caller is SortSpellSlots
   (src/E0EF3.C:442). */
/*@ 082C:07A4 */ void SwapChars(char far *pa, char far *pb)
{
    char saved;

    saved = *pa;
    *pa = *pb;
    *pb = saved;
}

/* 082C:07C5: exchange two far blocks through a third: copy the first into the third, the second
   into the first, then the third (already holding the first) into the second, so pa and pb swap
   and pc holds the old pa. Uses Unpack0, the format-0 raw copy (src/E1B82.C:68), for each leg.
   `len` is a signed int widened to the long Unpack0 wants. The only caller passes pa = gChars[si],
   pb = gChars[di], pc = gBufDCFC (src/EOV1.ASM:200-219), so it swaps two party records through the
   scratch buffer. src/EOV1.ASM:157 still calls this a rotation: a note for that unit. */
/*@ 082C:07C5 */ void SwapBlocks(char far *pa, char far *pb, char far *pc, int len)
{
    Unpack0(pa, pc, len);
    Unpack0(pb, pa, len);
    Unpack0(pc, pb, len);
}

/* 082C:081A: draw a one-line memory report at (x, y) through the text routine ds:80CA and, when
   `wait` is set, wait for a key. On a healthy far heap (heapcheck >= 0; _HEAPOK 2 and
   _HEAPCORRUPT -1, build/toolchain/tc100/INCLUDE/ALLOC.H:12-17; 0x74B0 is both _heapcheck and
   _farheapcheck, build/re/rtl_compare.json) it prints the "%lu:%lu" report with the largest free
   block and then farcoreleft; when heapcheck reports corruption it prints the "HEAP CORRUPT!!"
   message with the largest free block. the draw page gDrawPage is set to 0 while the
   report is drawn and restored after. The report is built in gFileNameBuf, the shared text buffer.
   Both cited files sit under build/ (git-ignored, so absent from a clean clone): ALLOC.H is
   installed by `make tc-install` and rtl_compare.json written by `make rtl-compare`. */
/*@ 082C:081A */ void ShowMemoryDiagnostics(int x, int y, int wait)
{
    int saved;

    saved = gDrawPage;
    gDrawPage = 0;
    if (heapcheck() >= 0)
        sprintf(gFileNameBuf, "%lu:%lu", LargestFreeBlock(), farcoreleft());
    else
        sprintf(gFileNameBuf, "HEAP CORRUPT!! (free memory) = %lu\n", LargestFreeBlock());
    gDrawText(gFileNameBuf, x, y, 0xf, 0xc);  /* 0xF foreground, 0xC the shadow colour */
    if (wait)
        WaitGameKey();
    gDrawPage = saved;
}

/* 082C:0898: draw one text string at (x, y): first its drop shadow at (x-1, y), (x, y+1) and
   (x-1, y+1) in the shadow colour 0x0C, then the text itself in `colour`. `colour2` is forwarded as
   the fifth argument of the first shadow draw only, the argument of the draw hook gDrawText
   (ds:80CA) that is a colour in the hook's direct calls (gTextAttr2 at src/E1DE7.C:280 and :296,
   hiColour and fill at src/E0EF3.C:1937/1946), so `colour2` is a second colour; what the hook does
   with it is unknown (Purpose unknown). Called by EditTextField in this unit (082C:000B,
   src/E082C.C:129), the character panel (src/E1547.C:521-545), the name list (src/E14EF.C:208,
   DrawOptionList at 14EF:0154), the word-find editor (src/EOV2.C:500), src/EOV6.C:399, the ovl0
   panel (src/EOV0.C:387) and the ovl1 panels (src/EOV1.ASM:1259, :1672). */
/*@ 082C:0898 */ void DrawShadowText(char far *text, int x, int y, int colour, int colour2)
{
    gDrawText(text, x - 1, y, 0xc, colour2);
    gDrawText(text, x, y + 1, 0xc, 0);
    gDrawText(text, x - 1, y + 1, 0xc, 0);
    gDrawText(text, x, y, colour, 0);
}

/* 082C:090A: the integer twin of DrawShadowText: print `value` as decimal at (x, y) with the same
   drop shadow (0x0C) through PrintInt. No caller found in src/. */
/*@ 082C:090A */ void DrawShadowInt(int value, int x, int y, int colour, int colour2)
{
    PrintInt(value, x - 1, y, 0xc, colour2);
    PrintInt(value, x, y + 1, 0xc, 0);
    PrintInt(value, x - 1, y + 1, 0xc, 0);
    PrintInt(value, x, y, colour, 0);
}

/* 082C:0974: the width in pixels of the decimal form of the long `value`, at three pixels per
   character (the same width appears in src/EOV5.C:413; the font's three-pixel cell is [H] until a
   font record names it). Formats into a 20-byte buffer with ltoa. Its callers draw at
   `0xf7 - NumberWidthLong(value)` and `0x11e - NumberWidthInt(value)` (src/E1547.C:571-573), so
   the number ends at a fixed column: it is right-aligned, not centred. */
/*@ 082C:0974 */ int NumberWidthLong(long value)
{
    char buf[20];

    ltoa(value, buf, 10);
    return strlen(buf) * 3;
}

/* 082C:09A6: the int twin of NumberWidthLong (itoa into a 20-byte buffer, times three); used the
   same right-aligned way (src/E1547.C:571-573). */
/*@ 082C:09A6 */ int NumberWidthInt(int value)
{
    char buf[20];

    itoa(value, buf, 10);
    return strlen(buf) * 3;
}

/* 082C:09D5: draw a filled bevelled panel: when `fill` is not NO_FILL the interior (x1+1, y1+1)-(x2-1,
   y2-1) is filled through ds:806A, then the top and right edges are drawn in `colourA` (the top
   starting at x1+1 and the right ending at y2-1, so they miss the top-left and bottom-right
   corners) and the left and bottom edges in `colourB`. */
/*@ 082C:09D5 */ void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill)
{
    if (fill != NO_FILL)
        gFillRect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, fill);
    gDrawLine(x1 + 1, y1, x2, y1, colourA);
    gDrawLine(x2, y1, x2, y2 - 1, colourA);
    gDrawLine(x1, y1, x1, y2, colourB);
    gDrawLine(x1, y2, x2, y2, colourB);
}

/* 082C:0A53: the thin wrapper that calls ShutdownGame (07C8:058A), whose body ends in exit(0)
   (src/E07C8.C:478). No caller found in src/. */
/*@ 082C:0A53 */ void QuitGame(void)
{
    ShutdownGame();
}

/* 082C:0A5D: is the last click (ds:8140, ds:8142) inside the rectangle (left, top)-(right, bottom)?
   A wrapper over IsPointInsideRect with the click globals as the point. */
/*@ 082C:0A5D */ int ClickInRect(int left, int top, int right, int bottom)
{
    return IsPointInsideRect(gClickX, gClickY, left, top, right, bottom);
}

/* 082C:0A7D: read `len` bytes of the file `name` into `buf`: FileOpen in mode 1, FileRead, then
   FileClose. `len` is a long, the type of FileRead's `len` (src/E1C0E.C:277); LoadFile takes an
   `unsigned len` (src/E1B82.C:86) and only LoadPacked reads a length from the file
   (src/E1B82.C:95-96). QUIRK 082C:0A7D: the FileOpen result is not checked before FileRead, as in
   LoadFile/LoadPacked (src/E1B82.C:97-98), and the byte count FileRead returns is discarded, as
   LoadFile discards it (src/E1B82.C:78-79). No caller found in src/. */
/*@ 082C:0A7D */ void ReadFileBytes(char far *name, void far *buf, unsigned long len)
{
    int slot;

    slot = FileOpen(name, 1);
    FileRead(slot, buf, len);
    FileClose(slot);
}

/* 082C:0AB4: fold `value` into the range [low, high]. With `wrap` 0 it clamps: below low gives
   low and above high gives high. With `wrap` 1 it wraps: below low gives high and above high
   gives low. The only caller, src/E0EF3.C:2137, uses the wrap (a step of +-1 then a search
   loop). */
/*@ 082C:0AB4 */ int ClampOrWrap(int value, int low, int high, int wrap)
{
    if (value < low) {
        if (wrap)
            value = high;
        else
            value = low;
    }
    if (value > high) {
        if (!wrap)
            value = high;
        else
            value = low;
    }
    return value;
}

/* 082C:0AE8: format "%s%d.%s" into gFileNameBuf (a far string, an int and a far string): a numbered
   data file name, for example LEVEL<n>.INF (the only caller passes "LEVEL", a level number and
   "INF" and opens the result: src/EOV3.C:568-569). */
/*@ 082C:0AE8 */ void FormatName(char far *prefix, int number, char far *suffix)
{
    sprintf(gFileNameBuf, "%s%d.%s", prefix, number, suffix);
}

/* 082C:0B0E: draw one line of text at (x, y) with gDrawText in colour 0xf and then print the
   memory report directly below it (y + 8) with ShowMemoryDiagnostics, waiting for a key. The [H]
   reading is a debug status screen; no caller found in src/. */
/*@ 082C:0B0E */ void DrawTextAndMemoryReport(char far *text, int x, int y)
{
    gDrawText(text, x, y, 0xf, 0xc);  /* 0xF foreground, 0xC the shadow colour */
    ShowMemoryDiagnostics(x, y + 8, 1);
}

/* 082C:0B43: wait `seconds` seconds and then wait for a key: busy-wait on the tick counter, clear
   the pending key (1AAD:0480), block until 1AAD:038E reports one, and clear it again. The counter
   runs at about 18.2 BIOS ticks a second, and the target uses 18, a whole-tick approximation of
   18.2, so the wait ends about 1% early. */
/*@ 082C:0B43 */ void DelaySecondsThenKey(int seconds)
{
    unsigned long target;

    target = gTicks + (unsigned long)(seconds * 18);
    while (target > gTicks)
        ;
    FlushKeyBuffer();
    while (!ReadKey())
        ;
    FlushKeyBuffer();
}

/* 082C:0B8F: write `text` to the text window with the current attributes (DrawText, 1DE7:060A;
   callers name it "the message", src/E0EF3.C:672-677) and play sound 0x4F; no constant in src/
   names it, only a comment (src/E0EF3.C:672). */
/*@ 082C:0B8F */ void SayAndPlaySound(char far *text)
{
    DrawText(text);
    PlaySound(0x4f);  /* the refuse sound; PromptSpellTarget plays it again after this call
                         (src/E0EF3.C:677) */
}
