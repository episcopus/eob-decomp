/* Unit at EOB.EXE 19F3:000E (137 bytes, 1 function): clip a rectangle to the 40 by 200
   play area, shifting the paired origin so the kept part stays put, then hand the eight
   ints to the far routine behind the function pointer at ds:8052. Placeholder names. */

extern void (far *gDrawRect)(int a, int b, int c, int d, int e, int f, int g, int h); /*@ ds:8052 */

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
    gDrawRect(a, b, c, d, e, f, g, h);
}
