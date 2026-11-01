/* Unit at EOB.EXE 19F3:000E (137 bytes, 1 function): adjust a rectangle request before
   passing it to the far routine behind the function pointer at ds:8052. The eight ints are
   two origins, a width and a height: x1 and x2 are two x origins sharing the width as their
   extent, y1 and y2 two y origins sharing the height, as their own corrections show. The
   units are the region table's (src/E1A29.C): x in 8-pixel columns, y in lines, so the cap
   of SCREEN_COLUMNS on the width is the 320-pixel screen and the cap of SCREEN_LINES on the
   height its 200 lines; region record 0 is 0,0,40,200, the full screen, matching the call at 082C:05DD.
   That reading of the units is an inference [H] from the caps and record 0; no code states
   it. All the compares are signed. The width and height are capped at SCREEN_COLUMNS and
   SCREEN_LINES when larger, as plain size caps not tied to any origin (x1 = 30, width = 40
   passes unclipped). x1 and x2 are rejected, the function returns without calling, when they
   reach SCREEN_COLUMNS. The negative-x1 shift runs first: it adds -x1 to x2 (x2 -= x1), adds
   x1 to the width and clears x1. The x1 test follows that shift, so it sees x1 already at 0
   when x1 was negative (a negative x1 never makes it return) but catches a positive x1 at or
   past SCREEN_COLUMNS columns. The x2 test comes next and sees x2 after the negative-x1 shift
   has moved it (x2 has had -x1 added), so a clipped x1 can push x2 to SCREEN_COLUMNS and make
   the function return.
   The negative-x2 shift comes after both tests. y1 and y2 have no upper test and nothing is
   capped on that side beyond the height.
   Each of x1, y1, x2, y2 is lowered to 0 when negative, and that move shifts its partner on
   the same axis and the size by the same amount: x1 moves x2 and the width, x2 moves x1 and
   the width, y1 moves y2 and the height, y2 moves y1 and the height. The eight ints then go
   on unchanged in source order (their push order in the bytes is extra2, extra1, height,
   width, y2, x2, y1, x1). extra1 and extra2 are passed through untouched; nothing in this
   unit reads them and their roles are unknown. Call sites push the eight ints (add sp,10h);
   for example 082C:05DD, 0EF3:071E, 0EF3:0860, 082C:006D and 082C:038A. 082C:05DD has two
   paths: when [8102] is 0 or 1 and si is 2 it passes x1=y1=x2=y2=0, width=40, height=200,
   extra1=4, extra2=2 (082C:058F, 0593..05CE); when [8102] is 2 and si is 0 it passes the
   same origins, width and height with extra1=2, extra2=0 (082C:05C8..05CE). 0EF3:071E passes
   x1=y1=x2=y2=0, width=22, height=120, extra1=0, extra2=2; 0EF3:0860 the same origins, width
   and height with extra1=2, extra2=0; 082C:006D pushes frame values (x1 = [bp+a] - 1, x2 = 0,
   y2 = 0xbf, i.e. 191, height = 9, width = [bp+e] + 2, so the bottom 9 lines are copied, 191 + 9
   = 200, evidence for the [H] reading of y in lines and y2 as a destination origin); 082C:038A
   pushes values taken from its frame. What the far routine does is still unknown. */

/* Screen limits. The region table's units are 8-pixel columns and lines (src/E1A29.C), so
   SCREEN_COLUMNS columns span the 320-pixel screen and SCREEN_LINES its 200 lines. Evidence:
   region record 0 is 0,0,40,200 (src/E1A29.C, EOB.EXE file offset 0x23B7F) and the DGROUP
   screen bounds and the 320x200 defaults at src/E1A72.ASM:176. */
#define SCREEN_COLUMNS 40
#define SCREEN_LINES 200

/* ds:8052: the far routine behind the pointer. The load image holds 00 00 00 00 here, so the
   pointer is zero in the file and some code sets it at run time; the setter and the target
   routine are not traced. */
extern void (far *gFar8052)(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ ds:8052 */

/* 19F3:000E: clips a rectangle request, then hands it to the far routine ds:8052. Takes the
   two origins (x1, y1 and x2, y2), a width and a height in the region table's units (x in
   8-pixel columns, y in lines; src/E1A29.C), and extra1 and extra2, which it forwards
   untouched. It caps the width at SCREEN_COLUMNS columns and the height at SCREEN_LINES
   lines, moves a negative origin to 0 together with its partner origin and the size, and
   returns early, without the call, when an x origin has reached SCREEN_COLUMNS columns. */
/*@ 19F3:000E */ void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2)
{
    if (width > SCREEN_COLUMNS) width = SCREEN_COLUMNS;
    if (height > SCREEN_LINES) height = SCREEN_LINES;
    if (x1 < 0) {
        x2 -= x1;
        width += x1;
        x1 = 0;
    }
    if (x1 >= SCREEN_COLUMNS) return;
    if (x2 >= SCREEN_COLUMNS) return;
    if (x2 < 0) {
        x1 -= x2;
        width += x2;
        x2 = 0;
    }
    if (y1 < 0) {
        y2 -= y1;
        height += y1;
        y1 = 0;
    }
    if (y2 < 0) {
        y1 -= y2;
        height += y2;
        y2 = 0;
    }
    gFar8052(x1, y1, x2, y2, width, height, extra1, extra2);
}
