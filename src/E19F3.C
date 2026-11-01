/* Unit at EOB.EXE 19F3:000E (137 bytes, 1 function): adjust a drawing or copy request
   before passing it to the far routine behind the function pointer at ds:8052. All the
   compares are signed. e is capped at 40 and f at 200 when larger, as plain size caps not
   tied to any origin (a = 30, e = 40 passes unclipped). a and c are rejected, the function
   returns without calling, when they reach 40; b and d have no upper test and nothing is
   capped on that side beyond f. Each of a, b, c, d is lowered to 0 when negative, and that
   move shifts its partner on the same axis and the size by the same amount: a moves c and
   e, c moves a and e, b moves d and f, d moves b and f. The eight ints then go on
   unchanged in source order (their push order in the bytes is h, g, f, e, d, c, b, a).
   The names a..h are placeholders: a and c are two x origins with e their extent, b and d
   two y origins with f their extent, as their shared corrections show; g and h are passed
   through untouched. Call sites push the eight ints (add sp,10h): 082C:05DD passes
   a=b=c=d=0, e=40, f=200, g=2, h=0, and 0EF3:071E the same origins with e=22, f=120, g=0,
   h=2. What 40 and 200 measure, and what the far routine does, are still unknown. */

extern void (far *gFar8052)(int a, int b, int c, int d, int e, int f, int g, int h); /*@ ds:8052 */

/*@ 19F3:000E */ void ClipDrawRect(int a, int b, int c, int d, int e, int f, int g, int h)
{
    if (e > 40) e = 40;
    if (f > 200) f = 200;
    if (a < 0) {
        c -= a;
        e += a;
        a = 0;
    }
    if (a >= 40) return;
    if (c >= 40) return;
    if (c < 0) {
        a -= c;
        e += c;
        c = 0;
    }
    if (b < 0) {
        d -= b;
        f += b;
        b = 0;
    }
    if (d < 0) {
        b -= d;
        f += d;
        d = 0;
    }
    gFar8052(a, b, c, d, e, f, g, h);
}
