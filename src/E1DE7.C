/* Unit at EOB.EXE 1DE7:0009 (2619 bytes, 9 functions): the text console. It keeps a text window
   (a rectangle of the region table) with its own cursor and colours, draws its border from a tile
   table, clears and redraws it, and prints strings and integers into it, scrolling at the bottom
   line. SetTextWindow (1DE7:0009) switches the active window, saving the current one's cursor
   and colours into its region record; SetTextWindowAndClear (1DE7:00B6) also clears the new
   one. DrawTextFrame (1DE7:00C9) draws the frame, DrawTileRun (1DE7:025E) the runs of tiles it is
   built from, SetRegionExtents (1DE7:02FE) writes a record, ClearTextRegion (1DE7:0344) blanks a
   window, DrawTextLine (1DE7:03B2) draws one line at the cursor, PrintIntText (1DE7:05DF) prints
   an integer and DrawText (1DE7:060A) renders a string with embedded control codes. Placeholder
   names until the callers name them. Compiled with -G (the two-byte argument clean-ups are
   `inc sp; inc sp`). */

/*@flags -ml -O -Z -G */

struct Region {             /* 16 bytes: four edges, then four words of saved state (1DE7:0009) */
    int left;               /* left edge, in 8-pixel columns */
    int top;                /* top edge, in pixel rows */
    int width;              /* width, in 8-pixel columns */
    int height;             /* height, in pixel rows */
    int saved0;             /* per-window state copied in and out by SelectTextRegion: two */
    int saved1;             /* attributes (saved0/saved1) and the cursor column and row (saved2/saved3) */
    int saved2;
    int saved3;
};

extern struct Region gRegions[];  /*@ ds:009F */  /* the region table (src/E1A29.C) */
extern int gCurRegion;            /*@ ds:8108 */  /* the region selected last (src/E1A29.C) */
void SelectRegion(int region);    /*@ 1A29:0189 */

/* The active window's live state. SelectTextRegion loads the four edges from the record and puts
   the record's saved words into the cursor and attribute globals; the drawing routines work from
   these, not from the record. */
extern int gTextLeft;      /*@ ds:FE64 */  /* the window's left edge, in 8-pixel columns */
extern int gTextTop;       /*@ ds:FE66 */  /* the window's top edge, in pixel rows */
extern int gTextWidth;     /*@ ds:FE62 */  /* the window's width, in 8-pixel columns */
extern int gTextHeight;    /*@ ds:FE60 */  /* the window's height, in pixel rows */
extern int gTextAttr1;     /*@ ds:811D */  /* first text attribute, passed to the text routine (saved0) */
extern int gTextAttr2;     /*@ ds:811B */  /* second text attribute, the colour of the frame and of */
                                           /* ClearTextRegion's fill (saved1) */
extern int gTextCol;       /*@ ds:FE52 */  /* cursor column, in font widths (saved2) */
extern int gTextRow;       /*@ ds:FE54 */  /* cursor row, in font heights (saved3) */
extern int gTextSaved;     /*@ ds:FEC0 */  /* 0 until the first SelectTextRegion, which then saves the */
                                           /* window switched away from */
extern int g811F;          /*@ ds:811F */  /* the current window's record index (src/E07A9.C reads it) */
extern int g8102;          /*@ ds:8102 */  /* the video mode (src/E07A9.C); 3 and 4 select the 6-bit cells */

/* The tile graphics the frame is drawn from: a cell's byte offset is the tile table's word for
   the cell shifted by 5 (or, for g8102 3 and 4, 6) and added to this far base. Only read here. */
extern void far *gTileBuf; /*@ ds:FE56 */

/* The font geometry bytes are signed here: the original sign-extends them (cbw), src/E1A47.C
   declares them unsigned. */
extern char gFontWidth1A47;  /*@ ds:8100 */  /* font cell width (src/E1A47.C) */
extern char gFontHeight1A47; /*@ ds:8101 */  /* font cell height (src/E1A47.C) */

/* ds:805A: a far routine the frame corners are drawn through, called with a tile source far
   pointer and a cell position (src, x, y). No writer or other caller. */
extern void (far *gTile805A)(void far *src, int x, int y); /*@ ds:805A */
/* ds:806A: a far routine called with (x1, y1, x2, y2, colour) like the line routine at ds:8062
   (src/E1A1F.C); ClearTextRegion and DrawTextLine fill and clip rectangles through it. */
extern void (far *gRect806A)(int x1, int y1, int x2, int y2, int colour); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */  /* src/E1AA6.C */
/* ds:8117: a far routine called when the cursor is on the last line of the window (the scroll
   hook); gScrollOn (ds:8110) gates it. */
extern void (far *gScroll8117)(void);  /*@ ds:8117 */
extern char gScrollOn;     /*@ ds:8110 */
/* ds:8115: written with immediates in many units of the resident code and by overlays; passed
   twice as the two extra arguments of ClipRect here, so the role is not established. */
extern int g8115;          /*@ ds:8115 */

/* 1DE7:0009: make `region` (an index into gRegions) the active text window. The first call only
   sets gTextSaved; after that, the window being left has its live attributes and cursor saved back
   into its record (the four words after the edges), and the new record's edges and saved words are
   loaded into the window globals and g811F. Called by many units (07C8, 0ACD, 0DE8, 1C0E, ovl0,
   ovl1, ovl2, ovl5, ovl6) and by 1DE7:00B6 here. */
/*@ 1DE7:0009 */ void SetTextWindow(int region)
{
    if (gTextSaved) {
        gRegions[g811F].saved0 = gTextAttr1;
        gRegions[g811F].saved1 = gTextAttr2;
        gRegions[g811F].saved2 = gTextCol;
        gRegions[g811F].saved3 = gTextRow;
    } else {
        gTextSaved = 1;
    }
    g811F = region;
    gTextLeft = gRegions[region].left;
    gTextTop = gRegions[region].top;
    gTextWidth = gRegions[region].width;
    gTextHeight = gRegions[region].height;
    gTextAttr1 = gRegions[region].saved0;
    gTextAttr2 = gRegions[region].saved1;
    gTextCol = gRegions[region].saved2;
    gTextRow = gRegions[region].saved3;
}

extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */

void ClearTextRegion(void);
void DrawText(char far *text);

extern char *itoa(int value, char far *string, int radix); /*@ 0000:62AA */

/* 1DE7:00B6: select the text window `region` and clear it. Called by ovl1:1153, ovl1:13F7,
   ovl6:0000 and ovl6:0B9B, which use it to open a window. */
/*@ 1DE7:00B6 */ void SetTextWindowAndClear(int region)
{
    SetTextWindow(region);
    ClearTextRegion();
}

/* The frame styles: one far pointer per style, each to a word record whose first four words are
   the four corner cells and whose further words are the repeating border tile numbers, terminated
   by 0xFF. Read only here (ds:113B, stride 4). */
extern unsigned far *gFrameTable[]; /*@ ds:113B */

/* 1DE7:00C9: draw the frame of the active text window in style `style`. The four corner cells come
   from gFrameTable[style] words 0..3 and the four edges from its words 4.. via DrawTileRun
   (1DE7:025E): the top and bottom edges run left to right over the window's width, the left and
   right edges run top to bottom over its height, inset by one cell. Each cell's source is
   gTileBuf + (number << shift) with shift 5, or 6 when g8102 is 3 or 4. Called by many units to
   draw a decorated box. QUIRK 1DE7:0135: the second and third edge runs take their x from the
   global gTextLeft, the corners and the first and fourth runs from the local copy, as the original
   bytes do. */
/*@ 1DE7:00C9 */ void DrawTextFrame(int style)
{
    int left;
    int shift;
    char far *tile;
    unsigned far *frame;
    int top;
    register int n;

    shift = 5;
    if (g8102 == 4 || g8102 == 3)
        shift = 6;
    left = gTextLeft;
    top = gTextTop;
    frame = gFrameTable[style];
    tile = (char far *)gTileBuf + (frame[0] << shift);
    gTile805A(tile, left - 1, top - 1);
    tile = (char far *)gTileBuf + (frame[1] << shift);
    gTile805A(tile, left + gTextWidth, top - 1);
    tile = (char far *)gTileBuf + (frame[2] << shift);
    gTile805A(tile, left - 1, top + gTextHeight);
    tile = (char far *)gTileBuf + (frame[3] << shift);
    gTile805A(tile, left + gTextWidth, top + gTextHeight);
    n = 4;
    n = DrawTileRun(frame, n, left, top - 1, gTextWidth, 1);
    n = DrawTileRun(frame, n, gTextLeft - 1, top, gTextHeight, 0);
    n = DrawTileRun(frame, n, gTextLeft + gTextWidth, top, gTextHeight, 0);
    n = DrawTileRun(frame, n, left, top + gTextHeight, gTextWidth, 1);
}
/* 1DE7:025E: draw a run of `count` tiles of a repeating pattern. `pattern` is a word array of tile
   numbers terminated by 0xFF; drawing starts at `start` and wraps back to it at each terminator.
   Each tile's source is gTileBuf + (number << shift) with shift 5, or 6 when g8102 is 3 or 4 (the
   same cell size DrawTextFrame uses), and is drawn through ds:805A at the next cell position: x
   advances when `horizontal` is set, y otherwise. After `count` tiles, the index is advanced to the
   end of the pattern (past its 0xFF) and returned, so the caller can continue on the next edge.
   Called by 1DE7:00C9 for each of the four frame edges. */
/*@ 1DE7:025E */ int DrawTileRun(unsigned far *pattern, int start, int x, int y, int count, int horizontal)
{
    int shift;
    int i;
    int n;
    char far *tile;

    shift = 5 + (g8102 == 4 || g8102 == 3);
    n = start;
    for (i = 0; i < count; i++) {
        tile = (char far *)gTileBuf + (pattern[n] << shift);
        gTile805A(tile, x, y);
        if (horizontal)
            x++;
        else
            y++;
        n++;
        if (pattern[n] == 0xff)
            n = start;
    }
    while (pattern[n] != 0xff)
        n++;
    return ++n;
}

/* 1DE7:02FE: write a region record's four edges (left, top, width, height). When `region` is the
   one currently selected, SelectRegion (1A29:0189) is called to load it into the driver's globals
   and hand the edges to the driver. Called by ovl4:0066, ovl4:021E and ovl4:1411; the text
   window's edges (ds:FE60..FE66) are not refreshed by it, only the record is. */
/*@ 1DE7:02FE */ void SetRegionExtents(int region, int left, int top, int width, int height)
{
    gRegions[region].left = left;
    gRegions[region].top = top;
    gRegions[region].width = width;
    gRegions[region].height = height;
    if (region == gCurRegion)
        SelectRegion(region);
}

/* 1DE7:0344: blank the active text window. The window rectangle is (left*8, top) to
   ((left + width)*8, top + height) in pixels; the mouse pointer is hidden over it, the rectangle
   is filled through ds:806A in the frame colour (gTextAttr2, one pixel short on the right and
   bottom), the pointer is shown again and the cursor is reset to (0, 0). Called by
   1DE7:00B6 here and by 1C0E:0ADD, ovl2:0846, ovl2:09ED, ovl5:021C, ovl5:025F and ovl6:0858. */
/*@ 1DE7:0344 */ void ClearTextRegion(void)
{
    int leftPx;
    int top;
    int rightPx;
    int bottom;

    leftPx = gTextLeft * 8;
    top = gTextTop;
    rightPx = (gTextLeft + gTextWidth) * 8;
    bottom = gTextTop + gTextHeight;
    HidePointerZone(leftPx, top, rightPx, bottom);
    gRect806A(leftPx, top, rightPx - 1, bottom - 1, gTextAttr2);
    ShowPointerZone();
    gTextCol = gTextRow = 0;
}
/* 1DE7:03B2: draw `text` at the cursor and consume it (text[0] is set to 0). The row is
   gTextRow * font height below the window top and the column gTextCol * font width right of its
   left edge. When that row has no room for a full line left, the scroll hook ds:8117 is called
   (gated by ds:8110), the last visible line is clipped and refilled (ClipRect, ds:806A) and the
   text drawn on it. `newline` non-zero advances to the next row at once; zero instead advances the
   column by strlen(text), wrapping to the next row only when the text no longer fits the window
   width. Called only by DrawText (1DE7:060A), which flushes each completed line through it. */
/*@ 1DE7:03B2 */ void DrawTextLine(char far *text, int newline)
{
    int y;
    int x;
    int line;

    if (gFontHeight1A47 * gTextRow > gTextHeight - gFontHeight1A47 && gScrollOn && gScroll8117)
        gScroll8117();
    y = gFontHeight1A47 * gTextRow;
    x = gFontWidth1A47 * gTextCol;
    if (gTextHeight - gFontHeight1A47 < y) {
        HidePointerZone(gTextLeft * 8, gTextTop, (gTextLeft + gTextWidth) * 8, gTextTop + gTextHeight);
        line = (gTextHeight / gFontHeight1A47 - 1) * gFontHeight1A47;
        ClipRect(gTextLeft, gTextTop + gFontHeight1A47, gTextLeft, gTextTop, gTextWidth, line, g8115, g8115);
        gRect806A(gTextLeft * 8, gTextTop + line, (gTextLeft + gTextWidth) * 8 - 1,
                  gTextTop + line + gFontHeight1A47 - 1, gTextAttr2);
        gDrawText(text, gTextLeft * 8, gTextTop + line, gTextAttr1, gTextAttr2);
        ShowPointerZone();
        if (newline) {
            gTextCol = 0;
            gTextRow++;
        } else {
            gTextRow = line / gFontHeight1A47;
            gTextCol += strlen(text);
            if (gTextWidth * 8 - gFontWidth1A47 < gFontWidth1A47 * gTextCol) {
                gTextCol = 0;
                gTextRow++;
            }
        }
    } else {
        y += gTextTop;
        HidePointerZone(gTextLeft * 8, y, (gTextLeft + gTextWidth) * 8, y + gFontHeight1A47);
        gDrawText(text, gTextLeft * 8 + x, y, gTextAttr1, gTextAttr2);
        ShowPointerZone();
        if (newline) {
            gTextCol = 0;
            gTextRow++;
        } else {
            gTextCol += strlen(text);
            if (gTextWidth * 8 - gFontWidth1A47 < gFontWidth1A47 * gTextCol) {
                gTextCol = 0;
                gTextRow++;
            }
        }
    }
    text[0] = 0;
}
/* 1DE7:05DF: print `value` in decimal into the text window. The value is formatted into a local
   30-byte buffer and handed to DrawText (1DE7:060A). No caller was found in the resident code. */
/*@ 1DE7:05DF */ void PrintIntText(int value)
{
    char buf[30];

    itoa(value, buf, 10);
    DrawText(buf);
}

extern char *strcat(char far *dest, char far *src); /*@ 0000:6721 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */

/* 1DE7:060A: render a string into the active text window. Text is accumulated in a pending word
   buffer and committed to a line buffer, which is flushed through DrawTextLine (1DE7:03B2) when
   the window width runs out or a control code asks for it. The control codes seen: 0x0D ends the
   line; 0x02 and 0x06 take the next byte as the second and first text attribute (ds:811B, ds:811D)
   and commit the line without a newline; 0x09 takes the next byte as a column and commits the line;
   0x13 takes the next byte as a target column and pads with spaces to it; 0x20 breaks the line at
   the current word; any other byte is appended to the current word. A byte with bit 0x80 set is
   stored with that bit cleared and ends the string after it (the stop flag). A trailing part word
   or line is flushed at the end. Called by 07A9:010C and many other units. */
/*@ 1DE7:060A */ void DrawText(char far *text)
{
    int pos;
    int n;
    int stop;
    char ch;
    char line[82];
    char word[82];
    int len;
    int col;

    pos = len = stop = 0;
    line[0] = 0;
    col = gTextCol;
    while ((ch = text[pos++]) != 0 && !stop) {
        if (ch & 0x80) {
            ch &= 0x7f;
            stop = 1;
        }
        if (ch == 0x0d) {
            word[len] = 0;
            if ((len + col) * gFontWidth1A47 <= gTextWidth * 8) {
                strcat(line, word);
                DrawTextLine(line, 1);
            } else {
                DrawTextLine(line, 1);
                DrawTextLine(word, 1);
            }
            col = len = 0;
        } else if (ch == 0x02) {
            word[len] = 0;
            if ((len + col) * gFontWidth1A47 <= gTextWidth * 8) {
                strcat(line, word);
                DrawTextLine(line, 0);
            } else {
                DrawTextLine(line, 1);
                DrawTextLine(word, 0);
            }
            len = 0;
            col = gTextCol;
            gTextAttr2 = text[pos++];
        } else if (ch == 0x06) {
            word[len] = 0;
            if ((len + col) * gFontWidth1A47 <= gTextWidth * 8) {
                strcat(line, word);
                DrawTextLine(line, 0);
            } else {
                DrawTextLine(line, 1);
                DrawTextLine(word, 0);
            }
            len = 0;
            col = gTextCol;
            gTextAttr1 = text[pos++];
        } else if (ch == 0x09) {
            word[len] = 0;
            if ((len + col) * gFontWidth1A47 <= gTextWidth * 8) {
                strcat(line, word);
                DrawTextLine(line, 0);
            } else {
                DrawTextLine(line, 1);
                DrawTextLine(word, 0);
            }
            len = 0;
            col = text[pos];
            gTextCol = col;
            pos++;
            if ((col + 1) * gFontWidth1A47 >= gTextWidth * 8)
                col = gTextCol = 0;
        } else if (ch == 0x13) {
            n = text[pos++];
            if (gTextCol < n) {
                while (gTextCol < n) {
                    word[len++] = ' ';
                    n--;
                }
                word[len] = 0;
                strcat(line, word);
                col += len;
                len = 0;
            }
        } else if (ch == ' ') {
            if ((len + col) * gFontWidth1A47 <= gTextWidth * 8) {
                if ((len + col + 1) * gFontWidth1A47 <= gTextWidth * 8 && len + col != 0)
                    word[len++] = ' ';
                word[len] = 0;
                strcat(line, word);
            } else {
                if ((len + 1) * gFontWidth1A47 <= gTextWidth * 8 && len != 0)
                    word[len++] = ' ';
                word[len] = 0;
                DrawTextLine(line, 1);
                col = 0;
                strcpy(line, word);
            }
            col += len;
            len = 0;
        } else {
            /* QUIRK 1DE7:09A6: the append is written in both arms of this if, so the compiler
               merges the two identical tails and emits a `jmp` to the join, which is the next
               instruction (EB 00). */
            if (len * gFontWidth1A47 >= gTextWidth * 8) {
                if (line[0])
                    DrawTextLine(line, 1);
                word[len] = 0;
                strcat(line, word);
                DrawTextLine(line, 1);
                col = len = 0;
                word[len++] = ch;
            } else {
                word[len++] = ch;
            }
        }
    }
    if (len != 0 || line[0] != 0) {
        word[len] = 0;
        if (gTextWidth * 8 - gFontWidth1A47 >= (len + col) * gFontWidth1A47) {
            strcat(line, word);
            DrawTextLine(line, 0);
        } else {
            DrawTextLine(line, 1);
            DrawTextLine(word, 0);
        }
    }
}
