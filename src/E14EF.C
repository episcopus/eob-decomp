/* Unit at EOB.EXE 14EF:0007 (1410 bytes, 6 functions): the shared option-list panel the overlays
   show for the in-game menus. A panel is a 16-byte record at ds:017F (the array size is not
   established) giving its x and y offsets from the origin ds:FE64/ds:FE66, its width in characters
   and height in lines, the selected row and two colours. The caller passes the array of far strings
   to show, a 32-bit mask of the selectable entries and a shift for that mask. FindMaskBit
   (14EF:0007) returns the index, counted from shift, of the n-th selectable entry; CountMaskBits (14EF:0107) counts the set bits of
   a range; IsPointInRect (14EF:00E1) is the point-in-rectangle test; BlinkText (14EF:0078) flashes
   one entry between two colours; DrawOptionList (14EF:0154) lays the list out and draws it;
   UpdateOptionList (14EF:0274) reads a key or a click and moves the selection. The 16-byte stride
   and field offsets come from DrawOptionList (14EF:0154), which reads y at :184, x at :185, sel at
   :186, height at :187, colorA at :192 and colorB at :194, and from UpdateOptionList (14EF:0274),
   which reads height at :263, sel at :264, y at :268, x at :269, colorA at :270, colorB at :271 and
   width at :278. The mask and shift come from the callers. PickOption (src/EOV0.C:1148, :1149) and
   LoadGamePanel (src/EOV2.C:380, :383) pass the all-ones mask 0xffffffff with shift 0, the case FindMaskBit
   returns n for at once; PickClass (src/EOV0.C:1203, :1215) and PickAlignment (src/EOV0.C:1253,
   :1257) pass gTable6081[race] & mask and gTable6099[classIndex] & mask with shift 0, masks other
   than -1 (mask -5 when a party member has align >= 6, :1189-1190; mask 0xfe3f when one has m20 ==
   2, :1241-1242), so FindMaskBit's bit scan is reached in play. CountMaskBits is called by PickClass
   with n 15 (:1193) and PickAlignment with n 9 (:1244), both with shift 0 (see its comment). The
   flags and the build through the assembler (`-B`) are whitepaper 2.1.1. Placeholder names. */

/*@flags -ml -O -Z -B */

/* Assembler note: the four separate `row = 0` / `row = lastRow` case tails (14EF:03E6, 03EC, 03D2,
   03F2) show the unit was compiled through the assembler (`-B`: TCC -S, then TASM with SMART off),
   which keeps them. TCC's internal assembler cross-jumps the later copy into the earlier one and
   drops 7 bytes, so the switch table would land at 14EF:0556 instead of 14EF:055D; the file's -B
   flag makes the matcher do the same (playbook 30, E14EF 2026-11-01). The KEY_CLICK case is written
   in the positive form (`if (IsPointInRect) { ...; break; } gFC7A = key;`) so the success path falls
   through and the failure is taken, matching 14EF:0417 `je`; the negated form made the optimizer
   branch the other way. Same class as 0ACD:091C. */

struct Panel {                  /* 16-byte records at ds:017F */
    int x;                      /* +0: column offset from ds:FE64; eight-pixel cells in both draw loops
                                   (<< 3 at :185 and :269); the hit rectangle uses the font width (:276) */
    int y;                      /* +2: row offset from ds:FE66 */
    int width;                  /* +4: width in characters */
    int height;                 /* +6: height in lines */
    int sel;                    /* +8: the selected row, counted among the selectable entries */
    int colorA;                 /* +a: normal text colour */
    int colorB;                 /* +c: colour of the selected entry */
    int pad;                    /* +e: the eighth word of the 16-byte record; not read in this unit */
};

extern struct Panel gPanels[]; /*@ ds:017F */
/* the panel area's top-left corner: x is in 8-pixel columns (<< 3 at :185, :269 and :276,
   src/E1C0E.C:60), y in pixels; the panel's x is eight-pixel cells in both draw loops (:185, :269)
   and font-width units in the hit rectangle (:276) */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern int gFC78; /*@ ds:FC78: extra pixels between lines, written by DrawOptionList, read by UpdateOptionList */
extern int gFC7A; /*@ ds:FC7A: the key UpdateOptionList did not handle; its reader is PickAlignment
                     (src/EOV0.C:1259, :1261), which tests it for 0x6e/0x32 and 0x41/0x42 before
                     IsPointInRect on the last click g8140/g8142 (:1262) */
extern int g5D1A; /*@ ds:5D1A: nonzero lets the pointer position pick the entry in UpdateOptionList and
                      makes DrawOptionList draw the selected row in colorB (:193-194) */
extern int g8140; /*@ ds:8140: x of the last click; the click event writes it (src/E1AAD.ASM:396) and
                      src/EOV0.C:1204 clears it; tested against a panel rectangle by UpdateOptionList (:306),
                      PickAlignment (src/EOV0.C:1262) and the wrapper src/E082C.C:465, and read into
                      gHitX by src/E16A5.C:224, :240 */
extern int g8142; /*@ ds:8142: y of the last click; the click event writes it (src/E1AAD.ASM:405) and
                      src/EOV0.C:1204 clears it; same readers as g8140 (:307, src/E16A5.C:225, :241) */
extern char gFontWidth; /*@ ds:8100: font cell width; src/E1A47.C declares it unsigned char, but
                            unsigned char here fails make match, so this file keeps char */
extern char gFontHeight; /*@ ds:8101: font cell height; src/E1A47.C declares it unsigned char (same) */
extern int g8102; /*@ ds:8102: a zero here makes the normal text colour 1 (:192, :270); what it means
                      is not established (src/E07C8.C:30 calls it the graphics choice, src/E1C0E.C:59
                      the video mode the game initialised) */
extern int gMouseDisabled; /*@ ds:814A: nonzero disables the mouse: the INT 33h handler at 1A72:017A
                              returns at once, and InitMouse sets it to 1 before it installs the
                              handler (1A72:0150, src/E1A72.ASM:100; store at src/E1A72.ASM:315) */
extern int gCursorX; /*@ ds:8138: mouse x in pixels (src/E1A72.ASM:65); tested with the pixel rectangle (:283) */
extern int gCursorY; /*@ ds:813A: mouse y in pixels (src/E1A72.ASM:73) */
extern int g8134; /*@ ds:8134: accumulated button changes; the INT 33h handler folds each change into
                      it (or ax,[g8134]; mov [g8134],ax, src/E1A72.ASM:400-401), this file clears it
                      (:199) and RunMapLoop clears it after the map screen is entered (src/E0ACD.C:886) */
extern void (far *gDrawText)(char far *text, int x, int y, int color, int extra); /*@ ds:80CA */

extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void LockHidePointer(void); /*@ 1BBE:0307: start a frame (redraws through the mouse callbacks) */
extern void LockShowPointer(void); /*@ 1BBE:0332: end a frame */
extern int IsKeyPending(void); /*@ 1AAD:0471: nonzero when an event waits in the ring (src/E1AAD.ASM:337-338) */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480: discard every buffered event (src/E1AAD.ASM:350-351) */
extern int WaitKeyEvent(void); /*@ 1AAD:05D3: read a key; returned unfiltered, the callers mask with
                                  KEY_CODE_MASK (bits 0-7 and bit 11) */
extern void WaitFrames(int n); /*@ 1A16:000B */

/* 14EF:0007: the index of the n-th (zero-based) selectable entry, as the bit number of the n-th set
   bit of `mask` scanning from bit `shift` up, minus shift (the result is an index counted from
   shift). The two loops first skip n set bits, then advance to the next set bit. A set mask bit
   means "selectable entry", so for the mask -1L every entry is selectable and the n-th entry is
   just n, returned at once. */
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

/* 14EF:0107: the number of set bits of `mask` among bit numbers shift..shift+n-1. Returns n when
   every bit is set (-1L), the "all selectable" case. The bit test is `mask & (1 << (i + shift))`, an
   int shift into a long. Its src/ callers PickClass (src/EOV0.C:1193) and PickAlignment
   (src/EOV0.C:1244) pass shift 0 with n 15 and 9 (the overlay disassembly at ovl0:20DD `mov ax,0xf`
   and ovl0:228E `mov ax,9` is a second witness), so i + shift is at most 14 and the shift's upper
   edge is reached by no caller. [H] recall, not proven on our bytes: with the unit's 16-bit int, a
   call that did shift by 16 would get 0 (bits 16 and up never counted), and the int `1 << 15`
   (-32768) would sign-extend to 0xFFFF8000 in the long AND, so bit 15 would count as "any of bits 15
   to 31"; a port that kept the original 16-bit shift semantics must keep both. */
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

/* 14EF:0154: draw the option list of panel `panel`. `names` is the caller's array of far strings,
   indexed by entry; `mask` and `shift` are the selectable-entry mask (entry e is selectable when bit
   e + shift of `mask` is set, see the header); `spacing` is the extra pixels between lines, stored
   in ds:FC78 for UpdateOptionList. It takes the panel's record (ds:017F), computes its origin from
   ds:FE64 (times eight pixels, so a cell column) and ds:FE66, finds the currently selected entry
   with FindMaskBit, then draws `height` rows; row i shows the i-th selectable entry (FindMaskBit(i))
   through 082C:0898 with the normal colour (1 when ds:8102 is zero, else the panel's colorA), inside
   one frame (1BBE:0307/0332). The selected row is drawn again with the panel's second colour when
   ds:5D1A is set or ds:814A is nonzero (the mouse is disabled). Finally it stores the row spacing in
   ds:FC78, discards the buffered events (1AAD:0480) and the accumulated button changes (ds:8134). */
/*@ 14EF:0154 */ void DrawOptionList(int panel, char far * far *names, long mask, int shift,
                                   int spacing)
{
    struct Panel *panelDef;
    int y;
    int x;
    int selEntry;
    int rows;
    int dy;
    register int i;
    register int entry;

    panelDef = &gPanels[panel];
    y = gTextTop + panelDef->y;
    x = (gTextLeft + panelDef->x) << 3;
    selEntry = FindMaskBit(panelDef->sel, mask, shift);
    rows = panelDef->height;
    LockHidePointer();
    for (i = 0; i < rows; i++) {
        entry = FindMaskBit(i, mask, shift);
        dy = y + gFontHeight * i + i * spacing;
        DrawShadowText(names[entry], x, dy, !g8102 ? 1 : panelDef->colorA, 0);
        if (entry == selEntry && (g5D1A || gMouseDisabled))
            gDrawText(names[entry], x, dy, panelDef->colorB, 0);
    }
    gFC78 = spacing;
    LockShowPointer();
    FlushKeyBuffer();
    g8134 = 0;
}
/* the key codes UpdateOptionList accepts; the switch table at 14EF:055D lists them. Confirm has three
   codes (0x2B, 0x3D, 0x61), the mouse buttons two (0x41, 0x42), first and last two each (0x5B/0x65,
   0x5D/0x67), up and down one each (0x60, 0x62). The mouse buttons arrive as KEY_CLICK/KEY_CLICK2:
   1AAD:0B0E derives both codes from a changed button bit (bit 0 gives 0x41, bit 1 gives 0x42) and
   1AAD:048D stores the click into ds:8140/ds:8142. */
enum {
    KEY_CODE_MASK = 0x8ff,      /* the callers' filter: bits 0-7 and bit 11 of the key word */
    KEY_CONFIRM   = 0x2B,       /* confirm: the row is picked (:316) */
    KEY_CONFIRM2  = 0x3D,       /* second confirm code */
    KEY_CONFIRM3  = 0x61,       /* third confirm code */
    KEY_CLICK     = 0x41,       /* mouse button 0 changed (1AAD:0B0E; INT 33h left [H]) */
    KEY_CLICK2    = 0x42,       /* mouse button 1 changed (1AAD:0B0E; INT 33h right [H]) */
    KEY_FIRST     = 0x5B,       /* first row */
    KEY_LAST      = 0x5D,       /* last row */
    KEY_UP        = 0x60,       /* previous row */
    KEY_DOWN      = 0x62,       /* next row */
    KEY_FIRST2    = 0x65,       /* another first-row key */
    KEY_LAST2     = 0x67        /* another last-row key */
};

/* 14EF:0274: the input half of the option-list panel: read the pending key (1AAD:0471/05D3, masked
   with KEY_CODE_MASK) and move the panel's selected row accordingly, then redraw the rows that
   changed and blink a confirmed one. `panel` indexes the 16-byte records at ds:017F, `names` is the
   caller's array of far strings, `mask`/`shift` map a row to a bit of the selectable-entry mask.
   Returns the index of the confirmed string in `names` (FindMaskBit of the chosen row), -1 when
   nothing was confirmed; stores the key it did not handle in ds:FC7A. `picked` holds the selectable
   row at first (:308, :316) and then, for the return, the string index (:338, with the comment at
   the assignment). The mouse goes through the rectangle built from ds:FE64/ds:FE66, ds:8100/8101
   (the font width and height) and ds:FC78 against the last click (ds:8140/8142) and the current
   cursor (ds:8138/813A), when ds:5D1A lets the pointer pick and the mouse is enabled
   (!gMouseDisabled, :280). The rectangle uses the font width, so it agrees with DrawOptionList's
   eight-pixel cells only while the cell is eight pixels wide. `counter` is a placeholder: it is
   bumped once and never read; `extra` is never read. Every caller passes a long zero (0L), which
   fills the same four bytes as this int pair (src/EOV0.C:1149, :1215, :1257; src/EOV2.C:383). */
/*@ 14EF:0274 */ int UpdateOptionList(int panel, char far * far *names, int counter, int extra,
                                   long mask, int shift)
{
    int lastRow;                /* the last selectable row index (height - 1) */
    int picked;                 /* the row picked by a key or click, then the confirmed string index */
    int key;                    /* the masked key or mouse event, 0 when none */
    int ybase;                  /* y of the panel's first row, in pixels */
    int xbase;                  /* x of the panel's first column, in pixels */
    int left;                   /* hit-rectangle edges, from the panel, the font and ds:FC78 */
    int right;
    int top;
    int bottom;
    int cursory;                /* the mouse y, read once for the pick test */
    int halfspace;              /* half the extra line spacing (ds:FC78 >> 1) */
    /* these five are char because the original keeps each in a byte (`mov byte ptr [bp-..]`); making
       any one an int fails make match (UpdateOptionList +0005 grows the frame, and the byte stores
       widen to word) */
    char colorA;                /* normal text colour (1 when ds:8102 is zero) */
    char colorB;                /* colour of the selected row */
    char prev;                  /* the row selected before this call */
    char row;                   /* the row being selected now */
    char entry;                 /* the string index a row maps to (FindMaskBit) */
    struct Panel *panelDef;
    register int lineheight;
    register int lineY;

    counter++;
    panelDef = &gPanels[panel];
    lastRow = panelDef->height - 1;
    row = prev = panelDef->sel % (lastRow + 1); /* wrap a stored row into 0..lastRow; why it can be out of range is not established */
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
        key = WaitKeyEvent() & KEY_CODE_MASK;
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
            row = lastRow;
        break;
    case KEY_DOWN:
        row++;
        if (row > lastRow)
            row = 0;
        break;
    case KEY_FIRST:
    case KEY_FIRST2:
        row = 0;
        break;
    case KEY_LAST:
    case KEY_LAST2:
        row = lastRow;
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
        entry = FindMaskBit(prev, mask, shift);
        lineY = ybase + lineheight * prev;
        gDrawText(names[entry], xbase, lineY, colorA, 0);
        entry = FindMaskBit(row, mask, shift);
        lineY = ybase + lineheight * row;
        gDrawText(names[entry], xbase, lineY, colorB, 0);
        LockShowPointer();
    }
    if (picked != -1) {
        entry = FindMaskBit(picked, mask, shift);
        LockHidePointer();
        lineY = ybase + lineheight * row;
        BlinkText(names[entry], xbase, lineY, colorA, colorB, 0);
        LockShowPointer();
        picked = entry; /* picked is now the string index, not the selectable row */
    }
    panelDef->sel = row;
    return picked;
}
