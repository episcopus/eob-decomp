/* Unit at EOB.EXE 0ACD:0008 (4182 bytes, 35 functions); only one function decompiled so far.
   Placeholder names. */

extern signed char gA99A; /*@ ds:A99A */
extern char gF51A; /*@ ds:F51A */

extern int f0D1C_0670(int dir, int one); /*@ 0D1C:0670 */
extern void f1547_0991(int dir); /*@ 1547:0991 */

/* 0ACD:091C: a switch through a four-entry jump table at 0ACD:09E7, cases 1 and 3 shared */
/*@ 0ACD:091C draft */ void Turn091C(int how)
{
    int old = gA99A;

    switch (how) {
    case 0:
        if (gF51A)
            gF51A = 0;
        else
            for (;;) {
                gA99A -= 2;
                if (gA99A < 0)
                    gA99A += 6;
                if (f0D1C_0670(gA99A, 1)) {
                    gF51A = 1;
                    break;
                }
            }
        break;
    case 1:
    case 3:
        if (f0D1C_0670(gA99A, 1))
            gA99A ^= 1;
        break;
    case 2:
        if (!gF51A)
            gF51A = 1;
        else
            for (;;) {
                gA99A += 2;
                if (gA99A > 5)
                    gA99A -= 6;
                if (f0D1C_0670(gA99A, 1)) {
                    gF51A = 0;
                    break;
                }
            }
        break;
    }
    f1547_0991(old);
    if (gA99A != old)
        f1547_0991(gA99A);
}
