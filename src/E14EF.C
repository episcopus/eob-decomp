/* Unit at EOB.EXE 14EF:0007 (1410 bytes, 6 functions): the shared option-list panel the overlays
   show for the in-game menus. A panel is a 16-byte record at ds:017F (the array size is not
   established) giving its x and y offsets from the origin ds:FE64/ds:FE66, its size in characters,
   the selected row and two colours. The caller passes the array of far strings to show, a 32-bit
   mask of the selectable entries and a shift for that mask. FindMaskBit (14EF:0007) returns the
   n-th set bit of the mask; CountMaskBits (14EF:0107) counts the set bits of a range;
   IsPointInRect (14EF:00E1) is the point-in-rectangle test; BlinkText (14EF:0078) flashes one entry
   between two colours; DrawOptionList (14EF:0154) lays the list out and draws it; UpdateOptionList
   (14EF:0274) reads a key or a click and moves the selection. The 16-byte stride and field offsets
   come from the code at 14EF:0154 (the accesses at :134-:137); the mask and shift come from the
   callers. Every caller in src/ passes mask 0xffffffff and shift 0 (src/EOV0.C:475, :490, :491;
   src/EOV2.C:380, :383), so the general path of FindMaskBit (the bit scan) and CountMaskBits are
   reached only from callers outside src/ (ovl0:1F66, ovl0:207A, ovl0:2238). Placeholder names. */

/*@flags -ml -O -Z -B */

/* Assembler note: the four separate `row = 0` / `row = rows` case tails (14EF:03E6, 03EC, 03D2,
   03F2) show the unit was compiled through the assembler (`-B`: TCC -S, then TASM with SMART off),
   which keeps them. TCC's internal assembler cross-jumps the later copy into the earlier one and
   drops 7 bytes, so the switch table would land at 14EF:0556 instead of 14EF:055D; the file's -B
   flag makes the matcher do the same (playbook 30, E14EF 2026-11-01). The KEY_CLICK case is written
   in the positive form (`if (IsPointInRect) { ...; break; } gFC7A = key;`) so the success path falls
   through and the failure is taken, matching 14EF:0417 `je`; the negated form made the optimizer
   branch the other way. Same class as 0ACD:091C. */

struct Panel {                  /* 16-byte records at ds:017F */
    int x;                      /* +0: column offset from ds:FE64; eight-pixel cells in DrawOptionList,
                                   the font width in UpdateOptionList */
    int y;                      /* +2: row offset from ds:FE66 */
    int width;                  /* +4: width in characters */
    int height;                 /* +6: height in lines */
    int sel;                    /* +8: the selected row, counted among the selectable entries */
    int colorA;                 /* +a: normal text colour */
    int colorB;                 /* +c: colour of the selected entry */
    int pad;                    /* +e: unused */
};

extern struct Panel gPanels[]; /*@ ds:017F */
/* the panel area's top-left corner: x is in cell columns (eight pixels in DrawOptionList, the font
   width in UpdateOptionList), y in pixels */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern int gFC78; /*@ ds:FC78: extra pixels between lines, written by DrawOptionList, read by UpdateOptionList */
extern int gFC7A; /*@ ds:FC7A: the key UpdateOptionList did not handle; written here, its reader is not yet identified */
extern int g5D1A; /*@ ds:5D1A: nonzero lets the pointer position pick the entry in UpdateOptionList */
extern int g8140; /*@ ds:8140: x of the last click, tested by UpdateOptionList */
extern int g8142; /*@ ds:8142: y of the last click, tested by UpdateOptionList */
extern char gFontWidth; /*@ ds:8100: font cell width (src/E1A47.C) */
extern char gFontHeight; /*@ ds:8101: font cell height (src/E1A47.C) */
extern int g8102; /*@ ds:8102: nonzero: a colour row exists (src/E0BD2.C:100, src/E0D1C.C:51);
                     other units call it the video mode */
extern int gMouseDisabled; /*@ ds:814A: nonzero disables the mouse (src/E1A72.ASM:282) */
extern int gCursorX; /*@ ds:8138: mouse x (E1A72.ASM) */
extern int gCursorY; /*@ ds:813A: mouse y (E1A72.ASM) */
extern int g8134; /*@ ds:8134: accumulated button changes (E1A72.ASM) */
extern void (far *gDrawText)(char far *text, int x, int y, int color, int extra); /*@ ds:80CA */

extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void LockHidePointer(void); /*@ 1BBE:0307: start a frame (redraws through the mouse callbacks) */
extern void LockShowPointer(void); /*@ 1BBE:0332: end a frame */
extern int IsKeyPending(void); /*@ 1AAD:0471: nonzero when a key is waiting */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480: clear the pending key */
extern int WaitKeyEvent(void); /*@ 1AAD:05D3: read a key; returned unfiltered, the callers mask with
                                  0x8FF (bits 0-7 and bit 11) */
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
/*@ 14EF:0078 */ void BlinkText(char far *text, int x, int y, char colorA, char colorB, char extra)
{
    int i;

    for (i = 0; i < 3; i++) {
        LockHidePointer();
        gDrawText(text, x, y, colorB, extra);
        WaitFrames(2);
        gDrawText(text, x, y, colorA, extra);
        LockShowPointer();
        WaitFrames(2);
    }
}

/* 14EF:00E1: 1 when (x, y) lies inside the rectangle [left, right] x [top, bottom], both ends
   inclusive, 0 otherwise. */
/*@ 14EF:00E1 */ int IsPointInRect(int x, int y, int left, int top, int right, int bottom)
{
    if (x >= left && x <= right && y >= top && y <= bottom)
        return 1;
    return 0;
}

/* 14EF:0107: the number of set bits of mask among bit numbers shift..shift+n-1. Returns n when every
   bit is set (-1L), the "all selectable" case. The bit test uses the 16-bit `1 << (i + shift)`; the
   external callers (ovl0:207A, ovl0:2238) push shift 0 (disassembly: mov al,0; push ax), so the
   shift stays below 16 and the 16-bit form is safe. */
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
   its origin from ds:FE64 (times eight pixels, so a cell column) and ds:FE66, finds the currently
   selected row with FindMaskBit, then draws `height` rows; row i shows the i-th selectable entry
   (FindMaskBit(i)) through 082C:0898 with the normal colour (1 when ds:8102 is zero, else the
   panel's colorA), inside one frame (1BBE:0307/0332). The selected row is drawn again with the
   panel's second colour when ds:5D1A is set or ds:814A is nonzero (the mouse is disabled). Finally
   it stores the row spacing in ds:FC78, clears the pending key (1AAD:0480) and the accumulated
   button changes (ds:8134). */
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
    y = gTextTop + panelDef->y;
    x = (gTextLeft + panelDef->x) << 3;
    sel = FindMaskBit(panelDef->sel, mask, shift);
    n = panelDef->height;
    LockHidePointer();
    for (i = 0; i < n; i++) {
        glyph = FindMaskBit(i, mask, shift);
        dy = y + gFontHeight * i + i * spacing;
        DrawShadowText(names[glyph], x, dy, !g8102 ? 1 : panelDef->colorA, 0);
        if (glyph == sel && (g5D1A || gMouseDisabled))
            gDrawText(names[glyph], x, dy, panelDef->colorB, 0);
    }
    gFC78 = spacing;
    LockShowPointer();
    FlushKeyBuffer();
    g8134 = 0;
}
/* the key codes UpdateOptionList accepts; the switch table at 14EF:055D lists them. Confirm has three
   codes (0x2B, 0x3D, 0x61), the mouse buttons two (0x41, 0x42), first and last two each (0x5B/0x65,
   0x5D/0x67), up and down one each (0x60, 0x62). The mouse buttons arrive as KEY_CLICK/KEY_CLICK2:
   src/E1AAD.ASM:363-405 stores the click into ds:8140/ds:8142. */
enum {
    KEY_CONFIRM   = 0x2B,       /* confirm: the row is picked (:260) */
    KEY_CONFIRM2  = 0x3D,       /* second confirm code */
    KEY_CONFIRM3  = 0x61,       /* third confirm code */
    KEY_CLICK     = 0x41,       /* left mouse button (src/E1AAD.ASM:363-405) */
    KEY_CLICK2    = 0x42,       /* right mouse button */
    KEY_FIRST     = 0x5B,       /* first row */
    KEY_LAST      = 0x5D,       /* last row */
    KEY_UP        = 0x60,       /* previous row */
    KEY_DOWN      = 0x62,       /* next row */
    KEY_FIRST2    = 0x65,       /* another first-row key */
    KEY_LAST2     = 0x67        /* another last-row key */
};

/* 14EF:0274: the input half of the option-list panel: read the pending key (1AAD:0471/05D3,
   masked with 0x8ff) and move the panel's selected row accordingly, then redraw the rows that
   changed and blink a confirmed one. `panel` indexes the 16-byte records at ds:017F, `names` is
   the caller's array of far strings, `mask`/`shift` map a row to a bit of the selectable-entry
   mask. Returns the index of the confirmed string in `names` (FindMaskBit of the chosen row), -1
   when nothing was confirmed; stores the key it did not handle in ds:FC7A. The mouse goes through
   the rectangle built from ds:FE64/ds:FE66, ds:8100/8101 (the font width and height) and ds:FC78
   against the last click (ds:8140/8142) and the current cursor (ds:8138/813A), when ds:5D1A lets
   the pointer pick and the mouse is enabled (!gMouseDisabled, :227). The rectangle uses the font
   width, so it agrees with DrawOptionList's eight-pixel cells only while the cell is eight pixels
   wide. `counter` is a placeholder: it is bumped once and never read; `extra` is never read. Both
   callers pass a long zero (0L), which fills the same four bytes as this int pair (src/EOV0.C:475,
   :491; src/EOV2.C:383). */
/*@ 14EF:0274 */ int UpdateOptionList(int panel, char far * far *names, int counter, int extra,
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
    row = prev = panelDef->sel % (rows + 1); /* wrap a stored row into 0..rows; why it can be out of range is not established */
    picked = -1;
    lineheight = gFontHeight + gFC78;
    halfspace = gFC78 >> 1;
    ybase = gTextTop + panelDef->y;
    xbase = (gTextLeft + panelDef->x) << 3;
    colorA = !g8102 ? 1 : panelDef->colorA;
    colorB = panelDef->colorB;
    key = 0;
    gFC7A = 0;
    if (IsKeyPending())
        key = WaitKeyEvent() & 0x8ff;
    left = (gTextLeft << 3) + panelDef->x * gFontWidth;
    top = gTextTop + panelDef->y - halfspace;
    right = left + panelDef->width * gFontWidth - 1;
    bottom = top + panelDef->height * lineheight - 1;
    if (!gMouseDisabled) {
        cursory = gCursorY;
        if (IsPointInRect(gCursorX, cursory, left, top, right, bottom) && g5D1A)
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
        if (IsPointInRect(g8140, g8142, left, top, right, bottom)) {
            row = (g8142 - top) / lineheight;
            picked = row;
            break;
        }
        gFC7A = key;
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
        LockHidePointer();
        glyph = FindMaskBit(prev, mask, shift);
        lineY = ybase + lineheight * prev;
        gDrawText(names[glyph], xbase, lineY, colorA, 0);
        glyph = FindMaskBit(row, mask, shift);
        lineY = ybase + lineheight * row;
        gDrawText(names[glyph], xbase, lineY, colorB, 0);
        LockShowPointer();
    }
    if (picked != -1) {
        glyph = FindMaskBit(picked, mask, shift);
        LockHidePointer();
        lineY = ybase + lineheight * row;
        BlinkText(names[glyph], xbase, lineY, colorA, colorB, 0);
        LockShowPointer();
        picked = glyph;
    }
    panelDef->sel = row;
    return picked;
}
