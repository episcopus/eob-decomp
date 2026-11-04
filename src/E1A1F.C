/* Unit at EOB.EXE 1A1F:000C (163 bytes, 1 function): clamp a rectangle to the 320x200 screen
   and draw its four edges. Each edge is a call to the far function pointer at ds:8062, which
   takes (x1, y1, x2, y2, colour) in that order (the callers in 0D1C:0454 and 16A5:07C3 pass a
   colour as the fifth argument). */

extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */

/* 1A1F:000C: draws the outline of the rectangle (x1, y1) to (x2, y2) in `colour`, each corner
   clamped to the 320 by 200 screen first, as four lines through gDrawLine. */
/*@ 1A1F:000C */ void DrawBox(int x1, int y1, int x2, int y2, int colour)
{
    if (x1 < 0) x1 = 0;
    if (x2 < 0) x2 = 0;
    if (x1 > 319) x1 = 319;
    if (x2 > 319) x2 = 319;
    if (y1 < 0) y1 = 0;
    if (y2 < 0) y2 = 0;
    if (y1 > 199) y1 = 199;
    if (y2 > 199) y2 = 199;
    gDrawLine(x1, y1, x2, y1, colour);
    gDrawLine(x1, y2, x2, y2, colour);
    gDrawLine(x1, y1, x1, y2, colour);
    gDrawLine(x2, y1, x2, y2, colour);
}
