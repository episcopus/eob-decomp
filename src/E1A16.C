/* Module at EOB.EXE 1A16:000B (43 bytes, 2 functions): wait for a number of display
   vertical retraces. f1DE4_000D polls port 3DAh (the CRTC status register) for the
   retrace bit, so the whole unit is a frame delay. Names are placeholders. */

/*@flags -ml -O -Z -G */

extern int gFE6C; /*@ ds:FE6C */

extern void far f1DE4_000D(int mode); /*@ 1DE4:000D */

/*@ 1A16:000B */ void WaitFrames(int n)
{
    while (n--) {
        f1DE4_000D(gFE6C);
    }
}

/*@ 1A16:0027 */ void WaitFrame(void)
{
    WaitFrames(1);
}
