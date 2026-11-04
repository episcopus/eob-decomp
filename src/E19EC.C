/* Unit at EOB.EXE 19EC:0002 (124 bytes, 3 functions): lookups in the page/buffer segment table
   at ds:FC0C. GetPageBuffer returns MK_FP(slot kind|1, 0) and GetPageVideo is its even twin
   (slot kind & ~1); neither allocates, both only read a segment word from the table.
   CopyPage calls the far handler at ds:8046 when two distinct slots hold non-zero segments.
   The table is filled per video mode in main at 07C8:02A4..03D9 (switch on ds:8102, jump table
   cs:0580): screen segments (0xB800, 0xA000) and off-screen page/buffer segments (0xA200/0xA400/
   0xA800, or MemAlloc'd 0xFA00, 0x7D00, 0xCB20, 0xFDE8 byte buffers). The fill writes both
   members of a pair with the same segment (07C8:02A7/02AA, 02C0/02C6, 02DC/02E2, 02F8/02FE),
   hence the low bit forced on or off. */

/*@flags -ml -O -Z */

#include <dos.h>

extern unsigned gPageSegs[]; /*@ ds:FC0C */
extern void (far *gCopyPage)(); /*@ ds:8046 */

/* 19EC:0002: the memory copy of page `kind`: the segment in the odd slot of the pair in gPageSegs,
   as a far pointer to its first byte. */
/*@ 19EC:0002 */ void far *GetPageBuffer(int kind)
{
    return MK_FP(gPageSegs[kind | 1], 0);
}

/* 19EC:0016: the video memory of page `kind`: the segment in the even slot of the pair in
   gPageSegs; the twin of GetPageBuffer, with no direct caller found. */
/*@ 19EC:0016 */ void far *GetPageVideo(int kind)
{
    return MK_FP(gPageSegs[kind & ~1], 0);
}

/* 19EC:002A: copies page `from` to page `to` through the graphics driver's page copy gCopyPage
   (ds:8046, its export 7), when both pages exist and are not one segment; `driverArg3` and
   `driverArg4` go to the driver unchanged. */
/*@ 19EC:002A */ void CopyPage(int from, int to, int driverArg3, int driverArg4)
{
    if (gPageSegs[from] && gPageSegs[to] && gPageSegs[from] != gPageSegs[to])
        gCopyPage(gPageSegs[from], gPageSegs[to], driverArg3, driverArg4);
}
