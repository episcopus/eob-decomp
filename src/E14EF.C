/* Unit at EOB.EXE 14EF:0007 (1410 bytes, 6 functions): the shared option-list panel the overlays
   show for the in-game menus. A panel is a 16-byte record at ds:017F (eight of them) giving its
   x and y offsets from the origin ds:FE64/ds:FE66, its size in characters, the selected entry and
   two colours. The caller passes the array of far strings to show, a 32-bit mask of the selectable
   entries and a shift for that mask. FindMaskBit (14EF:0007) returns the n-th set bit of the mask;
   CountMaskBits (14EF:0107) counts the set bits of a range; PointInRect (14EF:00E1) is a point in
   rectangle test; BlinkText (14EF:0078) flashes one entry between two colours; DrawOptionList
   (14EF:0154) lays the list out and draws it; UpdateOptionList (14EF:0274) reads a key or a click
   and moves the selection. The panel records, the mask and the rectangle are all this code's
   evidence. Placeholder names. */

struct Panel {                  /* 16-byte records at ds:017F, eight of them */
    int x;                      /* +0: column offset from ds:FE64 (times eight pixels) */
    int y;                      /* +2: row offset from ds:FE66 */
    int width;                  /* +4: width in characters */
    int height;                 /* +6: height in lines */
    int sel;                    /* +8: index of the selected entry in the caller's string array */
    int colorA;                 /* +a: normal text colour */
    int colorB;                 /* +c: colour of the selected entry */
    int pad;                    /* +e: unused */
};

extern struct Panel gPanels[]; /*@ ds:017F */
/* the panel area's top-left corner: x is in eight-pixel cells, y in pixels (see DrawOptionList) */
extern int gFE64; /*@ ds:FE64 */
extern int gFE66; /*@ ds:FE66 */
extern int gFC78; /*@ ds:FC78: extra pixels between lines, written by DrawOptionList, read by UpdateOptionList */
extern int gFC7A; /*@ ds:FC7A: the key UpdateOptionList did not handle, read by its overlay callers */
extern int g5D1A; /*@ ds:5D1A: nonzero lets the pointer position pick the entry in DrawOptionList */
extern int g8140; /*@ ds:8140: x of the last click, tested by UpdateOptionList */
extern int g8142; /*@ ds:8142: y of the last click, tested by UpdateOptionList */
extern char gFontWidth1A47; /*@ ds:8100 */
extern char gFontHeight1A47; /*@ ds:8101 */
extern int g8102; /*@ ds:8102 */
extern int g814A; /*@ ds:814A */
extern int g8138; /*@ ds:8138: mouse x (E1A72.ASM) */
extern int g813A; /*@ ds:813A: mouse y (E1A72.ASM) */
extern int g8134; /*@ ds:8134: accumulated button changes (E1A72.ASM) */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */

extern void f082C_0898(char far *text, int a, int b, int c, int d); /*@ 082C:0898 */
extern void f1BBE_0307(void); /*@ 1BBE:0307: start a frame (redraws through the mouse callbacks) */
extern void f1BBE_0332(void); /*@ 1BBE:0332: end a frame */
extern int f1AAD_0471(void); /*@ 1AAD:0471: nonzero when a key is waiting */
extern void f1AAD_0480(void); /*@ 1AAD:0480: clear the pending key */
extern int f1AAD_05D3(void); /*@ 1AAD:05D3: read a key (the low eleven bits are the code) */
extern void WaitFrames(int n); /*@ 1A16:000B */

/* 14EF:0007: the index of the n-th (zero-based) set bit of mask, scanning bit numbers from shift up.
   The two loops first skip n set bits, then advance to the next set bit, so a mask entry means
   "selectable entry". Returns n itself when every bit is set (-1L), the "nothing to skip" case. */
/*@ 14EF:0007 */ int FindMaskBit(int n, long mask, int shift)
{
    int i;

    if (mask == -1L)
        return n;
    i = 0;
    while (n != 0) {
        if (mask & (1L << (i + shift)))
            n--;
        i++;
    }
    while (!(mask & (1L << (i + shift))))
        i++;
    return i;
}

/* 14EF:0078: blink one entry between its normal colour and its selected colour: three times, draw it
   with colorB, wait two frames, draw it with colorA, wait two frames, redrawing only inside the
   frame begun by 1BBE:0307 and ended by 1BBE:0332. Called by UpdateOptionList on the entry just
   chosen, so this is the "confirm" flash. */
/*@ 14EF:0078 */ void BlinkText(char far *text, int x, int y, char colorA, char colorB, char zero)
{
    int i;

    for (i = 0; i < 3; i++) {
        f1BBE_0307();
        gDrawText(text, x, y, colorB, zero);
        WaitFrames(2);
        gDrawText(text, x, y, colorA, zero);
        f1BBE_0332();
        WaitFrames(2);
    }
}

/* 14EF:00E1: 1 when (x, y) lies inside the rectangle [left, right] x [top, bottom], both ends
   inclusive, 0 otherwise. */
/*@ 14EF:00E1 */ int PointInRect(int x, int y, int left, int top, int right, int bottom)
{
    if (x >= left && x <= right && y >= top && y <= bottom)
        return 1;
    return 0;
}

/* 14EF:0107: the number of set bits of mask among bit numbers shift..shift+n-1. Returns n when every
   bit is set (-1L), the "all selectable" case. */
/*@ 14EF:0107 */ int CountMaskBits(int n, long mask, int shift)
{
    int i;
    int count;

    if (mask == -1L)
        return n;
    i = count = 0;
    for (; i < n; i++) {
        if (mask & (1 << (i + shift)))
            count++;
    }
    return count;
}

/* 14EF:0154: draw the option list of panel `panel`. It takes the panel's record (ds:017F), computes
   its origin from ds:FE64 (times eight, so a cell column) and ds:FE66, finds the currently selected
   entry with FindMaskBit, then draws every selectable entry (FindMaskBit for each row index) through
   082C:0898 with the panel colour, inside one frame (1BBE:0307/0332). The selected entry is drawn
   again with the panel's second colour when ds:5D1A is set (the pointer picks entries) or ds:814A is
   not (the handler runs). Finally it stores the row spacing in ds:FC78, clears the pending key
   (1AAD:0480) and the accumulated button changes (ds:8134). */
/*@ 14EF:0154 */ void DrawOptionList(int panel, char far * far *names, long mask, int shift,
                                   int spacing)
{
    struct Panel *panelDef;
    int y;
    int x;
    int sel;
    int n;
    int dy;
    register int i;
    register int glyph;

    panelDef = &gPanels[panel];
    y = gFE66 + panelDef->y;
    x = (gFE64 + panelDef->x) << 3;
    sel = FindMaskBit(panelDef->sel, mask, shift);
    n = panelDef->height;
    f1BBE_0307();
    for (i = 0; i < n; i++) {
        glyph = FindMaskBit(i, mask, shift);
        dy = y + gFontHeight1A47 * i + i * spacing;
        f082C_0898(names[glyph], x, dy, !g8102 ? 1 : panelDef->colorA, 0);
        if (glyph == sel && (g5D1A || g814A))
            gDrawText(names[glyph], x, dy, panelDef->colorB, 0);
    }
    gFC78 = spacing;
    f1BBE_0332();
    f1AAD_0480();
    g8134 = 0;
}
/* the key codes UpdateOptionList accepts; the switch table at 14EF:055D lists them. One game key and
   one keyboard key share each action, and the mouse buttons arrive as KEY_CLICK/KEY_CLICK_ALT. */
enum {
    KEY_CONFIRM   = 0x2B,
    KEY_CONFIRM2  = 0x3D,
    KEY_CONFIRM3  = 0x61,
    KEY_CLICK     = 0x41,
    KEY_CLICK2    = 0x42,
    KEY_FIRST     = 0x5B,
    KEY_LAST      = 0x5D,
    KEY_UP        = 0x60,
    KEY_DOWN      = 0x62,
    KEY_FIRST2    = 0x65,
    KEY_LAST2     = 0x67
};

/* 14EF:0274: Purpose unknown: the input half of the option-list panel. It reads a masked key from
   1AAD:05D3 when 1AAD:0471 says one is waiting and moves the panel's selection (ds:017F + n*16,
   field +8) according to it: up/down, first/last, a click inside the panel rectangle (ds:8140/8142
   against the rectangle built from ds:FE64/ds:FE66, ds:8100/8101 and ds:FC78) or the confirm keys.
   It redraws the old and new rows when the selection moved (one frame, 1BBE:0307/0332) and blinks
   the confirmed row (BlinkText). Returns the index of the confirmed string in `names`, -1 when
   nothing was confirmed, and stores the key it did not handle in ds:FC7A. `counter` is bumped once
   and `extra` is never read: both callers only ever pass zero. Every case body, the rectangle and
   the draws reproduce the original except the case tails: our -O merges the identical `row = 0`
   (down plus first) and `row = rows` (up plus last) tails, the original keeps all four copies
   (7 bytes longer; the same unresolved -O tail merge as 0ACD:091C, whitepaper 2.1.1). Placeholder
   names. */
/*@ 14EF:0274 draft */ int UpdateOptionList(int panel, char far * far *names, int counter, int extra,
                                   long mask, int shift)
{
    int rows;
    int picked;
    int key;
    int ybase;
    int xbase;
    int left;
    int right;
    int top;
    int bottom;
    int cursory;
    int halfspace;
    char colorA;
    char colorB;
    char prev;
    char row;
    char glyph;
    struct Panel *panelDef;
    register int lineheight;
    register int lineY;

    counter++;
    panelDef = &gPanels[panel];
    rows = panelDef->height - 1;
    row = prev = panelDef->sel % (rows + 1);
    picked = -1;
    lineheight = gFontHeight1A47 + gFC78;
    halfspace = gFC78 >> 1;
    ybase = gFE66 + panelDef->y;
    xbase = (gFE64 + panelDef->x) << 3;
    colorA = !g8102 ? 1 : panelDef->colorA;
    colorB = panelDef->colorB;
    key = 0;
    gFC7A = 0;
    if (f1AAD_0471())
        key = f1AAD_05D3() & 0x8ff;
    left = (gFE64 << 3) + panelDef->x * gFontWidth1A47;
    top = gFE66 + panelDef->y - halfspace;
    right = left + panelDef->width * gFontWidth1A47 - 1;
    bottom = top + panelDef->height * lineheight - 1;
    if (!g814A) {
        cursory = g813A;
        if (PointInRect(g8138, cursory, left, top, right, bottom) && g5D1A)
            row = (cursory - top) / lineheight;
    }
    switch (key) {
    case KEY_UP:
        row--;
        if (row < 0)
            row = rows;
        break;
    case KEY_DOWN:
        row++;
        if (row > rows)
            row = 0;
        break;
    case KEY_FIRST:
    case KEY_FIRST2:
        row = 0;
        break;
    case KEY_LAST:
    case KEY_LAST2:
        row = rows;
        break;
    case KEY_CLICK:
    case KEY_CLICK2:
        if (!PointInRect(g8140, g8142, left, top, right, bottom)) {
            gFC7A = key;
            break;
        }
        row = (g8142 - top) / lineheight;
        picked = row;
        break;
    case KEY_CONFIRM:
    case KEY_CONFIRM2:
    case KEY_CONFIRM3:
        picked = row;
        break;
    default:
        gFC7A = key;
        break;
    }
    if (row != prev) {
        f1BBE_0307();
        glyph = FindMaskBit(prev, mask, shift);
        lineY = ybase + lineheight * prev;
        gDrawText(names[glyph], xbase, lineY, colorA, 0);
        glyph = FindMaskBit(row, mask, shift);
        lineY = ybase + lineheight * row;
        gDrawText(names[glyph], xbase, lineY, colorB, 0);
        f1BBE_0332();
    }
    if (picked != -1) {
        glyph = FindMaskBit(picked, mask, shift);
        f1BBE_0307();
        lineY = ybase + lineheight * row;
        BlinkText(names[glyph], xbase, lineY, colorA, colorB, 0);
        f1BBE_0332();
        picked = glyph;
    }
    panelDef->sel = row;
    return picked;
}
