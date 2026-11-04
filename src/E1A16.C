/* Module at EOB.EXE 1A16:000B (43 bytes, 2 functions): wait for a number of display
   vertical retraces. WaitRetrace polls port 3DAh (the CRTC status register) for the
   retrace bit, so the whole unit is a frame delay. */

/*@flags -ml -O -Z -G */

extern int gRetraceInverted; /*@ ds:FE6C */

extern void far WaitRetrace(int mode); /*@ 1DE4:000D */

/* 1A16:000B: waits for `n` display frames, one vertical retrace each (WaitRetrace, in the
   sense gRetraceInverted says). */
/*@ 1A16:000B */ void WaitFrames(int n)
{
    while (n--) {
        WaitRetrace(gRetraceInverted);
    }
}

/* 1A16:0027: waits for one display frame: a single vertical retrace, WaitFrames(1). */
/*@ 1A16:0027 */ void WaitFrame(void)
{
    WaitFrames(1);
}
