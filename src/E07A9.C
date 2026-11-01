/* Module at EOB.EXE 07A9:000A (487 bytes, the whole code segment). Second go/no-go module
   (SPEC 4.2): Turbo C++ 1.00, -ml -Z -O. StartupAlloc (07A9:000A) runs first: it makes the
   start-up far allocations (the unpack buffers, the ADLIB sound map at ds:DCAC and the 1600-byte
   ITEMRMP remap buffer at ds:F534, whitepaper 4), asks 19EC:0002 for three objects, then loads
   the sound (177D:000A), the item tables and the overlay-2 data. The second function (07A9:010C)
   redraws the item icon for the current square. Overlay entries are reached through the stub at
   2106 (entries 2 and 8). Names are placeholders. */

struct Entry14 {            /* 14-byte records at ds:8DD2 */
    unsigned char flags;    /* bit 0x80: draw an item for this record (other code tests 0x20, 0x08, 0x10) */
    signed char icon;       /* the item's icon number, an index into gIconsDDE4 (read at 07A9:0141) */
    char rest[12];
};

extern char gF43C; /*@ ds:F43C */
extern char gFB45; /*@ ds:FB45 */
extern void far *gBuf8DCA; /*@ ds:8DCA */
extern void far *gBufF43E; /*@ ds:F43E */
extern void far *gSoundMap; /*@ ds:DCAC */
extern void far *gBufDCFC; /*@ ds:DCFC */
extern char far *gBufF534; /*@ ds:F534 */
extern void far *gObjDCA6; /*@ ds:DCA6 */
extern void far *gObjDF4E; /*@ ds:DF4E */
extern void far *gObjEFC2; /*@ ds:EFC2 */
extern int gFC1A; /*@ ds:FC1A */
extern int g810E; /*@ ds:810E */
extern void far *gA9C0; /*@ ds:A9C0 */
extern int g811F; /*@ ds:811F */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern int g814A; /*@ ds:814A */
extern int gFBCE; /*@ ds:FBCE */
extern unsigned char gFBD2; /*@ ds:FBD2 */
extern int g8102; /*@ ds:8102 */
/* a far function pointer called with a pointer to a 16-byte colour table: a row of the 1600-byte
   ITEMRMP buffer (ds:F534 + n<<4), the one-byte dummy at ds:0090, or the tables at ds:1CD3 and
   ds:1CE3 that other callers pass. No writer of ds:80D2 in the resident code. */
extern void (far *gFn80D2)(void far *map); /*@ ds:80D2 */
extern void far *g80E2; /*@ ds:80E2 */
extern void far *gDF48; /*@ ds:DF48 */
/* far pointers to item icon images, not names: ovl2:03D5..0413 fills entries 1..0x58 from the call
   through [ds:8086] with a 16x16 cell position on the 20-per-row ITEMICN sheet (the string at
   ds:677D); index 0 is "none". */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */

extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void LoadSoundDriver(void); /*@ 177D:000A */
extern void SetUnpackBuffer(unsigned seg, unsigned size); /*@ 1B82:0217 */
extern void Ovl2Entry2(void); /*@ 2106:002A */
extern void f1A47_0169(void far *p); /*@ 1A47:0169 */
extern void Ovl2Entry8(void); /*@ 2106:0048 */
extern void f0ACD_0E8A(void); /*@ 0ACD:0E8A */
extern void f0D1C_0304(int n); /*@ 0D1C:0304 */
extern void f0BD2_05B5(int n); /*@ 0BD2:05B5 */
extern void f1DE7_060A(char far *text); /*@ 1DE7:060A */
extern void f17A6_197E(void far *icon, void far *a, void far *b, int c); /*@ 17A6:197E */
/* 1BB1:0002 stores the hot-spot offset and a pointer shape record; the caller passes the hot spot
   (8,8) when n > 0 and (0,0) otherwise, and the record gIconsDDE4[n] in both cases (see below). */
extern void SetMousePointerShape(int hotX, int hotY, void far *icon); /*@ 1BB1:0002 */

/* 07A9:000A: a start-up routine called from `main` (07C8:017F passes argc, argv through). It makes
   the far allocations through MemAlloc (7500 at ds:8DCA, 6000 at ds:F43E, 120 at ds:DCAC the ADLIB
   sound map, 4096 at ds:DCFC, 1600 at ds:F534 the ITEMRMP remap buffer, whitepaper 4), asks
   19EC:0002 for three objects (kinds 7, 5, 3), clears ds:810E, sets the unpack buffer (1B82:0217),
   and then loads sound (177D:000A), the item tables (Ovl2 stub entry 2) and the overlay-2 data
   (Ovl2 stub entry 8, then 0ACD:0E8A). Sets ds:F43C = 1 and ds:FB45 = 0. */
/*@ 07A9:000A */ void StartupAlloc(int argc, char far * far *argv)
{
    gF43C = 1;
    /* QUIRK 07A9:0012: `cmp [bp+6],0; jne; les; les; cmp es:[bx+1],0` and then no branch at all:
       the test's result is unused, an empty body (a macro compiled out, or a removed check). */
    if (argc == 0 && argv[1][1])
        ;
    gFB45 = 0;
    gBuf8DCA = MemAlloc(7500L, 0x20);
    gBufF43E = MemAlloc(6000L, 0);
    gSoundMap = MemAlloc(120L, 0);
    gBufDCFC = MemAlloc(4096L, 0);
    gBufF534 = MemAlloc(1600L, 0);
    gObjDCA6 = f19EC_0002(7);
    gObjDF4E = f19EC_0002(5);
    gObjEFC2 = f19EC_0002(3);
    LoadSoundDriver();
    SetUnpackBuffer(gFC1A, 0xFC00);
    g810E = 0;
    Ovl2Entry2();
    f1A47_0169(gA9C0);
    Ovl2Entry8();
    f0ACD_0E8A();
}

/* 07A9:010C: Purpose unknown: redraws the item icon for the current square. f0D1C_0304 stores the
   index n in ds:FBCE (gFBCE) unless g814A; a nonzero n also prints " taken.\r" (after 0BD2:05B5)
   when g811F == 7, and the record gTable8DD2[n] then supplies the icon number. When the current
   record gTable8DD2[gFBCE] has flag 0x80 and gFBD2 bit 0 is set, it selects the icon's 16-byte
   colour row (gBufF534 + (n << 4), or the dummy byte at ds:0090 when g8102 == 0), draws the item
   icon (17A6:197E) and then the icon (1BB1:0002) at (8,8), or at (0,0) when the number is 0. */
/*@ 07A9:010C */ void f07A9_010C(int n)
{
    f0D1C_0304(n);
    if (n) {
        if (g811F == 7) {
            f0BD2_05B5(n);
            f1DE7_060A(" taken.\r");
        }
        n = gTable8DD2[n].icon;
    }
    if (!g814A) {
        if ((gTable8DD2[gFBCE].flags & 0x80) && (gFBD2 & 1)) {
            if (g8102)
                gFn80D2(gBufF534 + (n << 4));   /* the icon's 16-byte colour row */
            else
                /* QUIRK 07A9:0184: the argument is the one-byte literal at ds:0090, a dummy passed
                   when g8102 == 0 (the real table is only built for g8102 != 0). */
                gFn80D2("");
            f17A6_197E(gIconsDDE4[n], gDF48, g80E2, g8102);
            n = 0x59;   /* one past the 1..0x58 that ovl2:03D5 fills; the filled icon set ends here */
        }
        if (n > 0)
            SetMousePointerShape(8, 8, gIconsDDE4[n]);
        else
            SetMousePointerShape(0, 0, gIconsDDE4[n]);
    }
}
