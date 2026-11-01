/* NEGATIVE FIXTURE (make match-gate): src/E07A9.C with one defect, a comparison: > written as >=. The matcher must
   report a difference; a build error or a match fails the gate. */
/* Module at EOB.EXE 07A9:000A (487 bytes, the whole code segment): start-up allocations and
   a level-related routine. Second go/no-go module (SPEC 4.2): Turbo C++ 1.00, -ml -Z -O.
   Names are placeholders. */

struct Entry14 {            /* 14-byte records at ds:8DD2 */
    unsigned char flags;
    signed char next;
    char rest[12];
};

extern char gF43C; /*@ ds:F43C */
extern char gFB45; /*@ ds:FB45 */
extern void far *gBuf8DCA; /*@ ds:8DCA */
extern void far *gBufF43E; /*@ ds:F43E */
extern void far *gBufDCAC; /*@ ds:DCAC */
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
extern void (far *gPrint80D2)(char far *text); /*@ ds:80D2 */
extern void far *g80E2; /*@ ds:80E2 */
extern void far *gDF48; /*@ ds:DF48 */
extern void far *gNamesDDE4[]; /*@ ds:DDE4 */

extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void far *ObjAlloc(int kind); /*@ 19EC:0002 */
extern void f177D_000A(void); /*@ 177D:000A */
extern void f1B82_0217(int a, unsigned b); /*@ 1B82:0217 */
extern void Ovl2Entry2(void); /*@ 2106:002A */
extern void f1A47_0169(void far *p); /*@ 1A47:0169 */
extern void Ovl2Entry8(void); /*@ 2106:0048 */
extern void f0ACD_0E8A(void); /*@ 0ACD:0E8A */
extern void f0D1C_0304(int n); /*@ 0D1C:0304 */
extern void f0BD2_05B5(int n); /*@ 0BD2:05B5 */
extern void f1DE7_060A(char far *text); /*@ 1DE7:060A */
extern void f17A6_197E(void far *name, void far *a, void far *b, int c); /*@ 17A6:197E */
extern void f1BB1_0002(int a, int b, void far *name); /*@ 1BB1:0002 */

/*@ 07A9:000A */ void StartupAlloc(int argc, char far * far *argv)
{
    gF43C = 1;
    if (argc == 0 && argv[1][1])
        ;
    gFB45 = 0;
    gBuf8DCA = MemAlloc(7500L, 0x20);
    gBufF43E = MemAlloc(6000L, 0);
    gBufDCAC = MemAlloc(120L, 0);
    gBufDCFC = MemAlloc(4096L, 0);
    gBufF534 = MemAlloc(1600L, 0);
    gObjDCA6 = ObjAlloc(7);
    gObjDF4E = ObjAlloc(5);
    gObjEFC2 = ObjAlloc(3);
    f177D_000A();
    f1B82_0217(gFC1A, 0xFC00);
    g810E = 0;
    Ovl2Entry2();
    f1A47_0169(gA9C0);
    Ovl2Entry8();
    f0ACD_0E8A();
}

/*@ 07A9:010C */ void Level07A9(int n)
{
    f0D1C_0304(n);
    if (n) {
        if (g811F == 7) {
            f0BD2_05B5(n);
            f1DE7_060A(" taken.\r");
        }
        n = gTable8DD2[n].next;
    }
    if (!g814A) {
        if ((gTable8DD2[gFBCE].flags & 0x80) && (gFBD2 & 1)) {
            if (g8102)
                gPrint80D2(gBufF534 + (n << 4));
            else
                gPrint80D2("");
            f17A6_197E(gNamesDDE4[n], gDF48, g80E2, g8102);
            n = 0x59;
        }
        if (n >= 0)
            f1BB1_0002(8, 8, gNamesDDE4[n]);
        else
            f1BB1_0002(0, 0, gNamesDDE4[n]);
    }
}
