/* Unit at overlay 0 of EOB.EXE (10873 bytes, 35 functions): the game's character panel and its
   drawing helpers. It builds the 17-cell icon table (ovl0:0000), draws a character's panel and
   equipment (ovl0:065B, 135D), the two option-list screens (ovl0:207A, 2238), the item/text field
   editor (ovl0:23EC) and the status/rest screens, and loads the overlay's screens (ovl0:2547,
   28F9). Placeholder names where no evidence. */

/*@flags -ml -O -Z */

extern char gOverlayBuf[]; /*@ ds:113F */ /* the overlay screen buffer the cell slicer reads */
extern void far * (far *gFn8082)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void farfree(void far *block); /*@ 0000:71EE */

struct IconDef9 {               /* the 17 9-byte icon definitions at 2084 (17 * 9 = 153 bytes) */
    unsigned char x;            /* +0: the cell's left edge */
    unsigned char y;            /* +1: the cell's top edge */
    unsigned char w;            /* +2: the cell's width, also the "defined" flag */
    unsigned char h;            /* +3: the cell's height */
    unsigned char m04;
    unsigned char m05;
    unsigned char m06;
    unsigned char m07;
    unsigned char m08;
};
extern struct IconDef9 far gIcons2084[]; /*@ 2084:0000 */

extern void far *g5E64[0x11]; /*@ ds:5E64 */ /* the 17 sliced icon pointers this unit builds and draws */

/* ovl0:0000: builds the 17-entry far-pointer table g5E64. For each of the 17 icon definitions in
   gIcons2084 whose +2 byte (the width) is nonzero, it slices a sprite cell out of the overlay buffer
   gOverlayBuf through ds:8082 (the definition's four leading bytes are the cell rectangle) and
   stores the pointer at g5E64[i]; every slot is cleared first. Called by ovl0:2547. */
/*@ ovl0:0000 */ void BuildIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        g5E64[i] = 0;
        if (gIcons2084[i].w)
            g5E64[i] = gFn8082(gIcons2084[i].x, gIcons2084[i].y, gIcons2084[i].w, gIcons2084[i].h,
                gOverlayBuf);
    }
}
/* ovl0:00AF: frees the 17 far pointers in g5E64 through farfree and clears every slot. Called by
   ovl0:28B0. */
/*@ ovl0:00AF */ void FreeIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        if (g5E64[i]) {
            farfree(g5E64[i]);
            g5E64[i] = 0;
        }
    }
}
/*@ ovl0:00F7 stub */ void Ovl0_00F7(void) {}
/*@ ovl0:0128 stub */ void Ovl0_0128(void) {}
/*@ ovl0:037A stub */ void Ovl0_037A(void) {}
/*@ ovl0:03B2 stub */ void Ovl0_03B2(void) {}
/*@ ovl0:042F stub */ void Ovl0_042F(void) {}
/*@ ovl0:0504 stub */ void Ovl0_0504(void) {}
/*@ ovl0:05BA stub */ void Ovl0_05BA(void) {}
/*@ ovl0:065B stub */ void Ovl0_065B(void) {}
/*@ ovl0:0A4F stub */ void Ovl0_0A4F(void) {}
/*@ ovl0:0A5E stub */ void Ovl0_0A5E(void) {}
/*@ ovl0:0D5B stub */ void Ovl0_0D5B(void) {}
/*@ ovl0:0DDC stub */ void Ovl0_0DDC(void) {}
/*@ ovl0:0EE5 stub */ void Ovl0_0EE5(void) {}
/*@ ovl0:1071 stub */ void Ovl0_1071(void) {}
/*@ ovl0:10F6 stub */ void Ovl0_10F6(void) {}
/*@ ovl0:1177 stub */ void Ovl0_1177(void) {}
/*@ ovl0:11BA stub */ void Ovl0_11BA(void) {}
/*@ ovl0:1224 stub */ void Ovl0_1224(void) {}
/*@ ovl0:135D stub */ void Ovl0_135D(void) {}
/*@ ovl0:1ADD stub */ void Ovl0_1ADD(void) {}
/*@ ovl0:1BF2 stub */ void Ovl0_1BF2(void) {}
/*@ ovl0:1EA9 stub */ void Ovl0_1EA9(void) {}
/*@ ovl0:1F66 stub */ void Ovl0_1F66(void) {}
/*@ ovl0:2051 stub */ void Ovl0_2051(void) {}
/*@ ovl0:207A stub */ void Ovl0_207A(void) {}
/*@ ovl0:2238 stub */ void Ovl0_2238(void) {}
/*@ ovl0:23EC stub */ void Ovl0_23EC(void) {}
/*@ ovl0:2547 stub */ void Ovl0_2547(void) {}
/*@ ovl0:26C6 stub */ void Ovl0_26C6(void) {}
/*@ ovl0:280D stub */ void Ovl0_280D(void) {}
/*@ ovl0:284A stub */ void Ovl0_284A(void) {}
/*@ ovl0:28B0 stub */ void Ovl0_28B0(void) {}
/*@ ovl0:28F9 stub */ void Ovl0_28F9(void) {}
